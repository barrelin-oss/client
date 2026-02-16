#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace hb::patch
{

struct build_options
{
    std::filesystem::path source;
    std::filesystem::path output;
    std::string channel = "stable";
    std::string game_version;
    std::filesystem::path key_path; // empty = no signing
    std::string platform;           // empty = shared, "linux"/"windows" = platform-specific
};

auto build(const build_options& opts) -> std::expected<void, std::string>;

} // namespace hb::patch
