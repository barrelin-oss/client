#include "hasher.h"

#include <sodium.h>

#include <array>
#include <fstream>

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

} // namespace

auto sha256_file(const std::filesystem::path& path) -> std::expected<std::string, std::string>
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return std::unexpected("failed to open file: " + path.string());
    }

    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);

    std::array<char, 8192> buffer{};
    while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || file.gcount() > 0)
    {
        auto bytes_read = file.gcount();
        crypto_hash_sha256_update(
            &state,
            reinterpret_cast<const unsigned char*>(buffer.data()),
            static_cast<unsigned long long>(bytes_read));

        if (file.eof())
        {
            break;
        }
    }

    std::array<unsigned char, crypto_hash_sha256_BYTES> hash{};
    crypto_hash_sha256_final(&state, hash.data());

    return to_hex(hash.data(), hash.size());
}

} // namespace hb::patch
