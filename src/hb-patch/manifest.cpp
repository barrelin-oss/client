#include "manifest.h"

#include <sodium.h>

#include <array>
#include <fstream>
#include <sstream>

namespace hb::patch
{

namespace
{

auto init_sodium() -> bool
{
    static bool initialized = false;
    static bool init_result = false;

    if (!initialized)
    {
        init_result = (sodium_init() >= 0);
        initialized = true;
    }

    return init_result;
}

auto to_hex(const unsigned char* data, size_t len) -> std::string
{
    static constexpr char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i)
    {
        result.push_back(hex_chars[(data[i] >> 4) & 0x0F]);
        result.push_back(hex_chars[data[i] & 0x0F]);
    }
    return result;
}

auto hex_to_bytes(const std::string& hex, std::vector<unsigned char>& out) -> bool
{
    if (hex.size() % 2 != 0)
    {
        return false;
    }

    out.resize(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        unsigned int byte = 0;
        auto sub = hex.substr(i, 2);
        std::istringstream iss(sub);
        iss >> std::hex >> byte;
        if (iss.fail())
        {
            return false;
        }
        out[i / 2] = static_cast<unsigned char>(byte);
    }

    return true;
}

} // namespace

auto files_to_json(const std::map<std::string, file_entry>& files) -> nlohmann::json
{
    auto j = nlohmann::json::object();

    for (const auto& [path, entry] : files)
    {
        auto file_obj = nlohmann::json::object();
        file_obj["sha256"] = entry.sha256;
        file_obj["size"] = entry.size;
        if (!entry.permissions.empty())
        {
            file_obj["permissions"] = entry.permissions;
        }
        j[path] = file_obj;
    }

    return j;
}

auto to_json(const manifest& m) -> nlohmann::json
{
    auto j = nlohmann::json::object();
    j["version"] = m.version;
    j["channel"] = m.channel;
    j["game_version"] = m.game_version;
    j["timestamp"] = m.timestamp;
    j["base_url"] = m.base_url;
    j["files"] = files_to_json(m.files);
    if (!m.signature.empty())
    {
        j["signature"] = m.signature;
    }
    return j;
}

auto write_manifest(const manifest& m, const std::filesystem::path& path)
    -> std::expected<void, std::string>
{
    std::error_code ec;
    if (path.has_parent_path())
    {
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
        {
            return std::unexpected("failed to create directories: " + ec.message());
        }
    }

    std::ofstream ofs(path);
    if (!ofs)
    {
        return std::unexpected("failed to open file for writing: " + path.string());
    }

    ofs << to_json(m).dump(4) << '\n';
    ofs.close();

    if (ofs.fail())
    {
        return std::unexpected("failed to write manifest: " + path.string());
    }

    return {};
}

auto read_manifest_json(const nlohmann::json& j)
    -> std::expected<manifest, std::string>
{
    manifest m;
    m.version = j.value("version", 1);
    m.channel = j.value("channel", std::string{});
    m.game_version = j.value("game_version", std::string{});
    m.timestamp = j.value("timestamp", std::string{});
    m.base_url = j.value("base_url", std::string{"/files/"});
    m.signature = j.value("signature", std::string{});

    if (j.contains("files") && j["files"].is_object())
    {
        for (const auto& [key, val] : j["files"].items())
        {
            file_entry entry;
            entry.sha256 = val.value("sha256", std::string{});
            entry.size = val.value("size", uint64_t{0});
            entry.permissions = val.value("permissions", std::string{});
            m.files[key] = std::move(entry);
        }
    }

    return m;
}

auto read_manifest(const std::filesystem::path& path)
    -> std::expected<manifest, std::string>
{
    std::ifstream ifs(path);
    if (!ifs)
    {
        return std::unexpected("failed to open manifest: " + path.string());
    }

    nlohmann::json j;
    try
    {
        ifs >> j;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        return std::unexpected(std::string("failed to parse manifest JSON: ") + e.what());
    }

    return read_manifest_json(j);
}

auto sign_manifest(manifest& m, const std::string& priv_hex)
    -> std::expected<void, std::string>
{
    if (!init_sodium())
    {
        return std::unexpected("failed to initialize libsodium");
    }

    std::vector<unsigned char> secret_key;
    if (!hex_to_bytes(priv_hex, secret_key) || secret_key.size() != crypto_sign_SECRETKEYBYTES)
    {
        return std::unexpected("invalid private key: expected 128 hex chars ("
            + std::to_string(crypto_sign_SECRETKEYBYTES) + " bytes)");
    }

    // Sign the deterministic JSON of the files section
    auto message = files_to_json(m.files).dump();
    auto msg_data = reinterpret_cast<const unsigned char*>(message.data());

    std::array<unsigned char, crypto_sign_BYTES> sig{};
    unsigned long long sig_len = 0;

    if (crypto_sign_detached(sig.data(), &sig_len, msg_data, message.size(),
                             secret_key.data()) != 0)
    {
        sodium_memzero(secret_key.data(), secret_key.size());
        return std::unexpected("signing failed");
    }

    sodium_memzero(secret_key.data(), secret_key.size());
    m.signature = to_hex(sig.data(), sig_len);

    return {};
}

auto verify_manifest(const manifest& m, const std::string& pub_hex) -> bool
{
    if (!init_sodium())
    {
        return false;
    }

    std::vector<unsigned char> pub_key;
    if (!hex_to_bytes(pub_hex, pub_key) || pub_key.size() != crypto_sign_PUBLICKEYBYTES)
    {
        return false;
    }

    std::vector<unsigned char> sig_bytes;
    if (!hex_to_bytes(m.signature, sig_bytes) || sig_bytes.size() != crypto_sign_BYTES)
    {
        return false;
    }

    auto message = files_to_json(m.files).dump();
    auto msg_data = reinterpret_cast<const unsigned char*>(message.data());

    return crypto_sign_verify_detached(sig_bytes.data(), msg_data, message.size(),
                                       pub_key.data()) == 0;
}

} // namespace hb::patch
