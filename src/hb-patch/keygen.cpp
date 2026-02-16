#include "keygen.h"

#include <sodium.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>

namespace hb::patch
{

namespace
{

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

} // namespace

auto generate_keypair() -> std::expected<std::pair<std::string, std::string>, std::string>
{
    if (!init_sodium())
    {
        return std::unexpected("failed to initialize libsodium");
    }

    std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> pub_key{};
    std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key{};

    crypto_sign_keypair(pub_key.data(), secret_key.data());

    auto pub_hex = to_hex(pub_key.data(), pub_key.size());
    auto priv_hex = to_hex(secret_key.data(), secret_key.size());

    sodium_memzero(secret_key.data(), secret_key.size());

    return std::pair{std::move(pub_hex), std::move(priv_hex)};
}

auto write_keypair(const std::filesystem::path& dir,
                   const std::string& pub_hex,
                   const std::string& priv_hex) -> std::expected<void, std::string>
{
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec)
    {
        return std::unexpected("failed to create directory: " + dir.string() + ": " + ec.message());
    }

    auto pub_path = dir / "signing_key.pub";
    std::ofstream pub_file(pub_path);
    if (!pub_file)
    {
        return std::unexpected("failed to write public key: " + pub_path.string());
    }
    pub_file << pub_hex << '\n';
    pub_file.close();

    auto priv_path = dir / "signing_key.priv";
    std::ofstream priv_file(priv_path);
    if (!priv_file)
    {
        return std::unexpected("failed to write private key: " + priv_path.string());
    }
    priv_file << priv_hex << '\n';
    priv_file.close();

    return {};
}

auto read_key_file(const std::filesystem::path& path) -> std::expected<std::string, std::string>
{
    std::ifstream file(path);
    if (!file)
    {
        return std::unexpected("failed to open key file: " + path.string());
    }

    std::string line;
    if (!std::getline(file, line))
    {
        return std::unexpected("key file is empty: " + path.string());
    }

    // Trim whitespace
    auto start = line.find_first_not_of(" \t\r\n");
    auto end = line.find_last_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return std::unexpected("key file contains only whitespace: " + path.string());
    }

    return line.substr(start, end - start + 1);
}

} // namespace hb::patch
