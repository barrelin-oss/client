#include "compressor.h"

#include <zstd.h>

#include <fstream>
#include <vector>

namespace hb::patch
{

auto compress_file(const std::filesystem::path& src, const std::filesystem::path& dst)
    -> std::expected<void, std::string>
{
    // Read source file
    std::ifstream in(src, std::ios::binary | std::ios::ate);
    if (!in)
    {
        return std::unexpected("failed to open source file: " + src.string());
    }

    auto file_size = in.tellg();
    in.seekg(0);

    std::vector<char> input(static_cast<size_t>(file_size));
    if (file_size > 0)
    {
        in.read(input.data(), file_size);
        if (!in)
        {
            return std::unexpected("failed to read source file: " + src.string());
        }
    }
    in.close();

    // Compress
    auto bound = ZSTD_compressBound(input.size());
    std::vector<char> output(bound);

    auto compressed_size = ZSTD_compress(
        output.data(), output.size(),
        input.data(), input.size(),
        3);

    if (ZSTD_isError(compressed_size))
    {
        return std::unexpected(std::string("zstd compression failed: ") + ZSTD_getErrorName(compressed_size));
    }

    // Create parent directories
    auto parent = dst.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec)
        {
            return std::unexpected("failed to create directory: " + parent.string() + ": " + ec.message());
        }
    }

    // Write compressed output
    std::ofstream out(dst, std::ios::binary);
    if (!out)
    {
        return std::unexpected("failed to open destination file: " + dst.string());
    }

    out.write(output.data(), static_cast<std::streamsize>(compressed_size));
    if (!out)
    {
        return std::unexpected("failed to write destination file: " + dst.string());
    }

    return {};
}

auto decompress_file(const std::filesystem::path& path)
    -> std::expected<std::string, std::string>
{
    // Read compressed file
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
    {
        return std::unexpected("failed to open compressed file: " + path.string());
    }

    auto file_size = in.tellg();
    in.seekg(0);

    std::vector<char> compressed(static_cast<size_t>(file_size));
    if (file_size > 0)
    {
        in.read(compressed.data(), file_size);
        if (!in)
        {
            return std::unexpected("failed to read compressed file: " + path.string());
        }
    }
    in.close();

    // Handle empty compressed files
    if (compressed.empty())
    {
        return std::unexpected("compressed file is empty: " + path.string());
    }

    // Get decompressed size
    auto decompressed_size = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR)
    {
        return std::unexpected("not a valid zstd frame: " + path.string());
    }
    if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        return std::unexpected("zstd frame has unknown content size: " + path.string());
    }

    // Decompress
    std::string result(decompressed_size, '\0');
    if (decompressed_size > 0)
    {
        auto actual_size = ZSTD_decompress(result.data(), result.size(), compressed.data(), compressed.size());
        if (ZSTD_isError(actual_size))
        {
            return std::unexpected(std::string("zstd decompression failed: ") + ZSTD_getErrorName(actual_size));
        }
        result.resize(actual_size);
    }

    return result;
}

} // namespace hb::patch
