#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <vector>

namespace hb::patcher
{

struct download_result
{
    std::vector<uint8_t> data;
    long http_status = 0;
};

/// Progress callback: (downloaded_bytes, total_bytes) -> return false to cancel
using progress_callback = std::function<bool(uint64_t downloaded, uint64_t total)>;

class downloader
{
public:
    explicit downloader(int timeout_seconds = 30);
    ~downloader();

    downloader(const downloader&) = delete;
    downloader& operator=(const downloader&) = delete;

    /// Fetch a URL, returning the response body
    auto fetch(const std::string& url, progress_callback on_progress = {})
        -> std::expected<download_result, std::string>;

    /// Try fetching url.zst first (decompress), fall back to raw url
    auto fetch_with_zstd_fallback(const std::string& url,
                                   uint64_t expected_size,
                                   progress_callback on_progress = {})
        -> std::expected<std::vector<uint8_t>, std::string>;

private:
    void* curl_ = nullptr; // CURL*
    int timeout_seconds_;
};

} // namespace hb::patcher
