#pragma once

#include <expected>
#include <string>

namespace hb::patcher
{

struct patcher_config
{
    std::string server_url = "https://patch.helbreath.dev";
    std::string channel = "stable";
    std::string public_key_hex = "6d4da125cb89ba438d2111789fa6907258699850bc2e4d07a4dbeaf13d1a244c";
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
