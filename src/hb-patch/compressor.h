#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace hb::patch
{

auto compress_file(const std::filesystem::path& src, const std::filesystem::path& dst)
    -> std::expected<void, std::string>;

// For testing only
auto decompress_file(const std::filesystem::path& path)
    -> std::expected<std::string, std::string>;

} // namespace hb::patch
