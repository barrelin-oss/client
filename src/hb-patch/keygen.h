#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <utility>

namespace hb::patch
{

auto generate_keypair() -> std::expected<std::pair<std::string, std::string>, std::string>;

auto write_keypair(const std::filesystem::path& dir,
                   const std::string& pub_hex,
                   const std::string& priv_hex) -> std::expected<void, std::string>;

auto read_key_file(const std::filesystem::path& path) -> std::expected<std::string, std::string>;

} // namespace hb::patch
