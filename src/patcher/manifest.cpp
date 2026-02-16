#include "patcher/manifest.hpp"

#include <nlohmann/json.hpp>
#include <sodium.h>
#include <spdlog/spdlog.h>

namespace hb::patcher
{

namespace
{

auto hex_to_bytes(const std::string& hex) -> std::expected<std::vector<uint8_t>, std::string>
{
    if (hex.size() % 2 != 0)
    {
        return std::unexpected("hex string has odd length");
    }

    std::vector<uint8_t> bytes(hex.size() / 2);
    if (sodium_hex2bin(bytes.data(), bytes.size(),
                       hex.c_str(), hex.size(),
                       nullptr, nullptr, nullptr) != 0)
    {
        return std::unexpected("invalid hex string");
    }
    return bytes;
}

auto current_platform() -> std::string
{
#ifdef _WIN32
    return "windows";
#else
    return "linux";
#endif
}

void parse_files_object(const nlohmann::json& files_json, std::unordered_map<std::string, file_entry>& out)
{
    for (auto& [path, entry_json] : files_json.items())
    {
        file_entry entry;
        if (entry_json.contains("sha256"))
            entry.sha256 = entry_json["sha256"].get<std::string>();
        if (entry_json.contains("size"))
            entry.size = entry_json["size"].get<uint64_t>();
        if (entry_json.contains("permissions"))
            entry.permissions = entry_json["permissions"].get<std::string>();

        out[path] = std::move(entry);
    }
}

} // namespace

auto parse_manifest(const std::string& json_text) -> std::expected<manifest, std::string>
{
    nlohmann::json j;
    try
    {
        j = nlohmann::json::parse(json_text);
    }
    catch (const nlohmann::json::exception& e)
    {
        return std::unexpected(std::string("manifest parse error: ") + e.what());
    }

    manifest m;

    if (j.contains("version"))
        m.version = j["version"].get<int>();
    if (j.contains("channel"))
        m.channel = j["channel"].get<std::string>();
    if (j.contains("game_version"))
        m.game_version = j["game_version"].get<std::string>();
    if (j.contains("timestamp"))
        m.timestamp = j["timestamp"].get<std::string>();
    if (j.contains("base_url"))
        m.base_url = j["base_url"].get<std::string>();
    if (j.contains("signature"))
        m.signature = j["signature"].get<std::string>();

    if (!j.contains("files") || !j["files"].is_object())
    {
        return std::unexpected("manifest missing 'files' object");
    }

    // Shared files
    parse_files_object(j["files"], m.files);

    // Platform-specific files (merged into the same map, overriding shared if duplicated)
    auto platform = current_platform();
    if (j.contains(platform) && j[platform].is_object())
    {
        parse_files_object(j[platform], m.files);
        spdlog::info("merged {} platform-specific file entries", j[platform].size());
    }

    return m;
}

auto verify_manifest(const manifest& m,
                     const std::string& public_key_hex) -> std::expected<void, std::string>
{
    if (public_key_hex.empty())
    {
        return std::unexpected("no public key configured");
    }

    if (m.signature.empty())
    {
        return std::unexpected("manifest has no signature");
    }

    // Decode public key
    auto pk_bytes = hex_to_bytes(public_key_hex);
    if (!pk_bytes)
    {
        return std::unexpected("invalid public key: " + pk_bytes.error());
    }
    if (pk_bytes->size() != crypto_sign_PUBLICKEYBYTES)
    {
        return std::unexpected("public key wrong size: expected " +
                               std::to_string(crypto_sign_PUBLICKEYBYTES) +
                               ", got " + std::to_string(pk_bytes->size()));
    }

    // Decode signature
    auto sig_bytes = hex_to_bytes(m.signature);
    if (!sig_bytes)
    {
        return std::unexpected("invalid signature hex: " + sig_bytes.error());
    }
    if (sig_bytes->size() != crypto_sign_BYTES)
    {
        return std::unexpected("signature wrong size");
    }

    // Build canonical form from the parsed manifest struct.
    // This must exactly match hb-patch's files_to_json(m.files).dump():
    //   - nlohmann::json::object() uses std::map → sorted keys
    //   - dump() with no args → compact, no indent
    //   - only include "permissions" if non-empty
    nlohmann::json files_json = nlohmann::json::object();
    for (const auto& [path, entry] : m.files)
    {
        auto file_obj = nlohmann::json::object();
        file_obj["sha256"] = entry.sha256;
        file_obj["size"] = entry.size;
        if (!entry.permissions.empty())
        {
            file_obj["permissions"] = entry.permissions;
        }
        files_json[path] = file_obj;
    }
    auto canonical = files_json.dump();

    // Verify Ed25519 signature
    if (crypto_sign_verify_detached(sig_bytes->data(),
                                     reinterpret_cast<const unsigned char*>(canonical.data()),
                                     canonical.size(),
                                     pk_bytes->data()) != 0)
    {
        return std::unexpected("Ed25519 signature verification failed");
    }

    return {};
}

auto diff_manifests(const manifest& remote, const manifest& local) -> diff_result
{
    diff_result result;

    // Files to download: in remote but not local, or hash differs
    for (auto& [path, remote_entry] : remote.files)
    {
        auto it = local.files.find(path);
        if (it == local.files.end() || it->second.sha256 != remote_entry.sha256)
        {
            result.to_download.push_back(path);
            result.total_download_bytes += remote_entry.size;
        }
    }

    // Files to delete: in local but not remote
    for (auto& [path, local_entry] : local.files)
    {
        if (!remote.files.contains(path))
        {
            result.to_delete.push_back(path);
        }
    }

    // Sort for deterministic order
    std::sort(result.to_download.begin(), result.to_download.end());
    std::sort(result.to_delete.begin(), result.to_delete.end());

    return result;
}

auto serialize_manifest(const manifest& m) -> std::string
{
    nlohmann::json j;
    j["version"] = m.version;
    j["channel"] = m.channel;
    j["game_version"] = m.game_version;
    j["timestamp"] = m.timestamp;
    j["base_url"] = m.base_url;
    j["signature"] = m.signature;

    nlohmann::json files_json = nlohmann::json::object();
    for (auto& [path, entry] : m.files)
    {
        nlohmann::json entry_json;
        entry_json["sha256"] = entry.sha256;
        entry_json["size"] = entry.size;
        if (!entry.permissions.empty())
        {
            entry_json["permissions"] = entry.permissions;
        }
        files_json[path] = std::move(entry_json);
    }
    j["files"] = std::move(files_json);

    return j.dump(4);
}

} // namespace hb::patcher
