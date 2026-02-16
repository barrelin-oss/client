#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace hb::patch
{

auto sha256_file(const std::filesystem::path& path) -> std::expected<std::string, std::string>;

} // namespace hb::patch
