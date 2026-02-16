#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace hb::patcher
{

/// SHA-256 hash a file on disk, returns lowercase hex string
auto hash_file_sha256(const std::filesystem::path& path) -> std::expected<std::string, std::string>;

/// SHA-256 hash a memory buffer, returns lowercase hex string
auto hash_buffer_sha256(const void* data, size_t len) -> std::string;

/// Write data to path atomically: write .tmp, verify SHA-256, rename over target.
/// On Linux, sets file permissions if specified (octal string like "755").
auto atomic_write_verified(const std::filesystem::path& path,
                           const void* data,
                           size_t len,
                           const std::string& expected_sha256,
                           const std::string& permissions = "") -> std::expected<void, std::string>;

/// Decompress zstd-compressed data
auto decompress_zstd(const void* data, size_t len) -> std::expected<std::vector<uint8_t>, std::string>;

/// Remove a file, ignoring "not found" errors
auto remove_file(const std::filesystem::path& path) -> std::expected<void, std::string>;

/// Clean up .old files left from in-use DLL replacement (Windows)
void cleanup_old_files(const std::filesystem::path& dir);

} // namespace hb::patcher
