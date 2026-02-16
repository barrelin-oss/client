#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

namespace hb::patcher
{

struct file_entry
{
    std::string sha256;
    uint64_t size = 0;
    std::string permissions; // octal string like "755", empty if not set
};

struct manifest
{
    int version = 1;
    std::string channel;
    std::string game_version;
    std::string timestamp;
    std::string base_url;
    std::unordered_map<std::string, file_entry> files;
    std::string signature; // Ed25519 hex signature
};

struct diff_result
{
    std::vector<std::string> to_download; // files to download (new or changed)
    std::vector<std::string> to_delete;   // files present locally but not remotely
    uint64_t total_download_bytes = 0;
};

/// Parse a manifest from JSON text.
/// Platform-specific files ("windows"/"linux" objects) are merged into the files map
/// for the current platform. The signature covers the original JSON structure.
auto parse_manifest(const std::string& json_text) -> std::expected<manifest, std::string>;

/// Verify Ed25519 signature of manifest's files object.
/// Rebuilds canonical JSON from the parsed files map (matching hb-patch's files_to_json).
auto verify_manifest(const manifest& m,
                     const std::string& public_key_hex) -> std::expected<void, std::string>;

/// Diff remote manifest against local manifest to determine what needs updating
auto diff_manifests(const manifest& remote, const manifest& local) -> diff_result;

/// Serialize a manifest to JSON (for saving locally)
auto serialize_manifest(const manifest& m) -> std::string;

} // namespace hb::patcher
