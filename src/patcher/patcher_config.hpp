#pragma once

#include <expected>
#include <string>

namespace hb::patcher
{

struct patcher_config
{
    std::string server_url = "https://patch.helbreath.dev";
    std::string channel = "stable";
    std::string public_key_hex = "7031632a8c73786756cd7aa0704a2e40b88d5ff1a29cc843e3d44491213552a5";
    bool verify_signatures = true;
    int timeout_seconds = 30;
    int max_retries = 3;
    std::string client_binary = "helbreath_client";
};

/// Load config from patcher_config.json in the current directory.
/// Returns defaults if file is missing.
auto load_patcher_config() -> std::expected<patcher_config, std::string>;

/// Save config back to patcher_config.json.
auto save_patcher_config(const patcher_config& config) -> std::expected<void, std::string>;

} // namespace hb::patcher
