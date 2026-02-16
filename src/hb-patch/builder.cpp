#include "builder.h"
#include "compressor.h"
#include "hasher.h"
#include "keygen.h"
#include "manifest.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <future>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>

namespace hb::patch
{

namespace fs = std::filesystem;

namespace
{

auto make_timestamp() -> std::string
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

auto is_executable(const fs::path& path) -> bool
{
#ifdef _WIN32
    auto ext = path.extension().string();
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".exe" || ext == ".bat" || ext == ".cmd";
#else
    auto status = fs::status(path);
    return (status.permissions() & fs::perms::owner_exec) != fs::perms::none;
#endif
}

struct file_result
{
    std::string rel_path;
    file_entry entry;
    std::string error;
};

auto process_file(const fs::path& src_path,
                  const fs::path& files_dir,
                  const fs::path& rel) -> file_result
{
    file_result result;
    result.rel_path = rel.generic_string();

    auto hash = sha256_file(src_path);
    if (!hash.has_value())
    {
        result.error = "failed to hash " + result.rel_path + ": " + hash.error();
        return result;
    }

    std::error_code ec;
    auto size = fs::file_size(src_path, ec);
    if (ec)
    {
        result.error = "failed to get size of " + result.rel_path + ": " + ec.message();
        return result;
    }

    result.entry.sha256 = hash.value();
    result.entry.size = size;

    if (is_executable(src_path))
    {
        result.entry.permissions = "755";
    }

    auto dest_path = files_dir / rel;
    fs::create_directories(dest_path.parent_path(), ec);
    if (ec)
    {
        result.error = "failed to create directory for " + result.rel_path + ": " + ec.message();
        return result;
    }

    fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        result.error = "failed to copy " + result.rel_path + ": " + ec.message();
        return result;
    }

    auto zst_path = dest_path;
    zst_path += ".zst";
    auto compress_result = compress_file(src_path, zst_path);
    if (!compress_result.has_value())
    {
        result.error = "failed to compress " + result.rel_path + ": " + compress_result.error();
        return result;
    }

    return result;
}

} // namespace

auto build(const build_options& opts) -> std::expected<void, std::string>
{
    std::error_code ec;

    if (!fs::is_directory(opts.source, ec))
    {
        return std::unexpected("source is not a directory: " + opts.source.string());
    }

    // Output layout:
    //   shared:   out/{channel}/manifest.json + out/{channel}/files/
    //   platform: out/{channel}/{platform}/manifest.json + out/{channel}/{platform}/files/
    auto channel_dir = opts.output / opts.channel;
    auto build_dir = opts.platform.empty() ? channel_dir : channel_dir / opts.platform;
    auto files_dir = build_dir / "files";
    fs::create_directories(files_dir, ec);
    if (ec)
    {
        return std::unexpected("failed to create output directory: " + ec.message());
    }

    // Collect all files from source
    std::vector<std::pair<fs::path, fs::path>> file_list;
    for (auto& entry : fs::recursive_directory_iterator(opts.source, ec))
    {
        if (!entry.is_regular_file())
            continue;

        auto rel = fs::relative(entry.path(), opts.source, ec);
        if (ec)
        {
            return std::unexpected("failed to compute relative path: " + ec.message());
        }
        file_list.emplace_back(entry.path(), rel);
    }
    if (ec)
    {
        return std::unexpected("error iterating directory: " + ec.message());
    }

    // Process files in parallel
    auto concurrency = std::max(1u, std::thread::hardware_concurrency());

    std::vector<std::future<file_result>> futures;
    futures.reserve(file_list.size());

    for (auto& [src, rel] : file_list)
    {
        futures.push_back(std::async(std::launch::async,
            [&files_dir](fs::path s, fs::path r) -> file_result
            {
                return process_file(s, files_dir, r);
            },
            src, rel));
    }

    // Collect results, build manifest
    manifest m;
    m.version = 1;
    m.channel = opts.channel;
    m.game_version = opts.game_version;
    m.timestamp = make_timestamp();
    m.base_url = "/files/";

    uint64_t total_bytes = 0;

    for (size_t i = 0; i < futures.size(); ++i)
    {
        auto result = futures[i].get();
        if (!result.error.empty())
        {
            return std::unexpected(result.error);
        }

        total_bytes += result.entry.size;
        std::fprintf(stderr, "  %s (%lu bytes)\n",
            result.rel_path.c_str(), static_cast<unsigned long>(result.entry.size));

        m.files[std::move(result.rel_path)] = std::move(result.entry);
    }

    // Sign if key provided
    if (!opts.key_path.empty())
    {
        auto key_hex = read_key_file(opts.key_path);
        if (!key_hex.has_value())
        {
            return std::unexpected("failed to read key: " + key_hex.error());
        }

        auto sign_result = sign_manifest(m, key_hex.value());
        if (!sign_result.has_value())
        {
            return std::unexpected("failed to sign manifest: " + sign_result.error());
        }
    }

    // Write manifest
    auto manifest_path = build_dir / "manifest.json";
    auto write_result = write_manifest(m, manifest_path);
    if (!write_result.has_value())
    {
        return std::unexpected("failed to write manifest: " + write_result.error());
    }

    std::fprintf(stderr, "  %zu files, %lu bytes total (%u threads)\n",
        file_list.size(), static_cast<unsigned long>(total_bytes), concurrency);

    return {};
}

} // namespace hb::patch
