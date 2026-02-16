#include "patcher/patcher_config.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

namespace hb::patcher
{

auto load_patcher_config() -> std::expected<patcher_config, std::string>
{
    patcher_config config;

    const auto path = std::filesystem::path("patcher_config.json");
    if (!std::filesystem::exists(path))
    {
        spdlog::info("patcher_config.json not found, using defaults");
        return config;
    }

    std::ifstream file(path);
    if (!file)
    {
        return std::unexpected("failed to open patcher_config.json");
    }

    nlohmann::json j;
    try
    {
        file >> j;
    }
    catch (const nlohmann::json::exception& e)
    {
        return std::unexpected(std::string("failed to parse patcher_config.json: ") + e.what());
    }

    if (j.contains("server_url"))
        config.server_url = j["server_url"].get<std::string>();
    if (j.contains("channel"))
        config.channel = j["channel"].get<std::string>();
    if (j.contains("public_key_hex"))
        config.public_key_hex = j["public_key_hex"].get<std::string>();
    if (j.contains("verify_signatures"))
        config.verify_signatures = j["verify_signatures"].get<bool>();
    if (j.contains("timeout_seconds"))
        config.timeout_seconds = j["timeout_seconds"].get<int>();
    if (j.contains("max_retries"))
        config.max_retries = j["max_retries"].get<int>();
    if (j.contains("client_binary"))
        config.client_binary = j["client_binary"].get<std::string>();

    spdlog::info("loaded patcher config: server={}, channel={}", config.server_url, config.channel);
    return config;
}

auto save_patcher_config(const patcher_config& config) -> std::expected<void, std::string>
{
    nlohmann::json j;
    j["server_url"] = config.server_url;
    j["channel"] = config.channel;
    if (!config.public_key_hex.empty())
        j["public_key_hex"] = config.public_key_hex;
    j["verify_signatures"] = config.verify_signatures;
    j["timeout_seconds"] = config.timeout_seconds;
    j["max_retries"] = config.max_retries;
    j["client_binary"] = config.client_binary;

    std::ofstream file("patcher_config.json", std::ios::trunc);
    if (!file)
    {
        return std::unexpected(std::string("failed to open patcher_config.json for writing"));
    }

    file << j.dump(4);
    if (!file.good())
    {
        return std::unexpected(std::string("failed to write patcher_config.json"));
    }

    spdlog::info("saved patcher config: channel={}", config.channel);
    return {};
}

} // namespace hb::patcher
