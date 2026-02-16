#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <string>

namespace hb::patch
{

struct file_entry
{
    std::string sha256;
    uint64_t size = 0;
    std::string permissions; // empty if not executable
};

using file_map = std::map<std::string, file_entry>;

struct manifest
{
    int version = 1;
    std::string channel;
    std::string game_version;
    std::string timestamp;
    std::string base_url = "/files/";
    file_map files;
    std::string signature;
};

auto to_json(const manifest& m) -> nlohmann::json;
auto files_to_json(const file_map& files) -> nlohmann::json;

auto write_manifest(const manifest& m, const std::filesystem::path& path)
    -> std::expected<void, std::string>;
auto read_manifest(const std::filesystem::path& path)
    -> std::expected<manifest, std::string>;
auto read_manifest_json(const nlohmann::json& j)
    -> std::expected<manifest, std::string>;

auto sign_manifest(manifest& m, const std::string& priv_hex)
    -> std::expected<void, std::string>;
auto verify_manifest(const manifest& m, const std::string& pub_hex) -> bool;

} // namespace hb::patch
