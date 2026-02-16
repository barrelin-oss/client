#include "patcher/file_ops.hpp"

#include <sodium.h>
#include <zstd.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace hb::patcher
{

namespace
{

auto to_hex(const unsigned char* data, size_t len) -> std::string
{
    static constexpr char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i)
    {
        result.push_back(hex_chars[data[i] >> 4]);
        result.push_back(hex_chars[data[i] & 0x0f]);
    }
    return result;
}

} // namespace

auto hash_file_sha256(const std::filesystem::path& path) -> std::expected<std::string, std::string>
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return std::unexpected("failed to open file: " + path.string());
    }

    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);

    std::array<char, 8192> buffer;
    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0)
    {
        crypto_hash_sha256_update(&state,
                                  reinterpret_cast<const unsigned char*>(buffer.data()),
                                  static_cast<size_t>(file.gcount()));
    }

    std::array<unsigned char, crypto_hash_sha256_BYTES> hash;
    crypto_hash_sha256_final(&state, hash.data());

    return to_hex(hash.data(), hash.size());
}

auto hash_buffer_sha256(const void* data, size_t len) -> std::string
{
    std::array<unsigned char, crypto_hash_sha256_BYTES> hash;
    crypto_hash_sha256(hash.data(), static_cast<const unsigned char*>(data), len);
    return to_hex(hash.data(), hash.size());
}

auto atomic_write_verified(const std::filesystem::path& path,
                           const void* data,
                           size_t len,
                           const std::string& expected_sha256,
                           const std::string& permissions) -> std::expected<void, std::string>
{
    // Verify hash before writing
    auto actual_hash = hash_buffer_sha256(data, len);
    if (actual_hash != expected_sha256)
    {
        return std::unexpected("SHA-256 mismatch: expected " + expected_sha256 + ", got " + actual_hash);
    }

    // Ensure parent directory exists
    auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec)
        {
            return std::unexpected("failed to create directory " + parent.string() + ": " + ec.message());
        }
    }

    // Write to temporary file
    auto tmp_path = path;
    tmp_path += ".tmp";

    {
        std::ofstream file(tmp_path, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            return std::unexpected("failed to create temp file: " + tmp_path.string());
        }
        file.write(static_cast<const char*>(data), static_cast<std::streamsize>(len));
        if (!file)
        {
            return std::unexpected("failed to write temp file: " + tmp_path.string());
        }
    }

    // Verify written file
    auto written_hash = hash_file_sha256(tmp_path);
    if (!written_hash)
    {
        std::filesystem::remove(tmp_path);
        return std::unexpected("failed to verify written file: " + written_hash.error());
    }
    if (*written_hash != expected_sha256)
    {
        std::filesystem::remove(tmp_path);
        return std::unexpected("written file hash mismatch after write");
    }

    // Atomic rename
    std::error_code ec;
    std::filesystem::rename(tmp_path, path, ec);
    if (ec)
    {
#ifdef _WIN32
        // On Windows, the target may be a DLL locked by the running process.
        // Windows allows renaming locked files, so move it aside and retry.
        auto old_path = path;
        old_path += ".old";
        std::error_code ec2;
        std::filesystem::remove(old_path, ec2); // clean up any previous .old
        std::filesystem::rename(path, old_path, ec2);
        if (!ec2)
        {
            std::filesystem::rename(tmp_path, path, ec2);
            if (!ec2)
            {
                return {}; // success — .old will be cleaned up on next launch
            }
            // Restore original if we couldn't put the new one in place
            std::filesystem::rename(old_path, path, ec);
        }
#endif
        std::filesystem::remove(tmp_path);
        return std::unexpected("failed to rename temp file: " + ec.message());
    }

    // Set permissions on Linux
#ifndef _WIN32
    if (!permissions.empty())
    {
        auto perms = static_cast<std::filesystem::perms>(std::stoi(permissions, nullptr, 8));
        std::filesystem::permissions(path, perms, ec);
        if (ec)
        {
            // Non-fatal, just log
        }
    }
#endif

    return {};
}

auto decompress_zstd(const void* data, size_t len) -> std::expected<std::vector<uint8_t>, std::string>
{
    auto decompressed_size = ZSTD_getFrameContentSize(data, len);
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR)
    {
        return std::unexpected("not valid zstd data");
    }

    std::vector<uint8_t> output;
    if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        // Streaming decompression for unknown size
        auto* dctx = ZSTD_createDCtx();
        if (!dctx)
        {
            return std::unexpected("failed to create zstd decompression context");
        }

        ZSTD_inBuffer input_buf = {data, len, 0};
        std::vector<uint8_t> tmp(ZSTD_DStreamOutSize());

        while (input_buf.pos < input_buf.size)
        {
            ZSTD_outBuffer output_buf = {tmp.data(), tmp.size(), 0};
            auto ret = ZSTD_decompressStream(dctx, &output_buf, &input_buf);
            if (ZSTD_isError(ret))
            {
                ZSTD_freeDCtx(dctx);
                return std::unexpected(std::string("zstd decompression error: ") + ZSTD_getErrorName(ret));
            }
            output.insert(output.end(), tmp.begin(), tmp.begin() + output_buf.pos);
        }

        ZSTD_freeDCtx(dctx);
    }
    else
    {
        output.resize(decompressed_size);
        auto result = ZSTD_decompress(output.data(), output.size(), data, len);
        if (ZSTD_isError(result))
        {
            return std::unexpected(std::string("zstd decompression error: ") + ZSTD_getErrorName(result));
        }
        output.resize(result);
    }

    return output;
}

auto remove_file(const std::filesystem::path& path) -> std::expected<void, std::string>
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec && ec != std::errc::no_such_file_or_directory)
    {
        return std::unexpected("failed to remove " + path.string() + ": " + ec.message());
    }
    return {};
}

void cleanup_old_files(const std::filesystem::path& dir)
{
    std::error_code ec;
    for (auto& entry : std::filesystem::recursive_directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".old")
        {
            std::filesystem::remove(entry.path(), ec);
        }
    }
}

} // namespace hb::patcher
