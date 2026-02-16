#include "patcher/downloader.hpp"
#include "patcher/file_ops.hpp"

#include <curl/curl.h>
#include <spdlog/spdlog.h>

namespace hb::patcher
{

namespace
{

struct curl_write_context
{
    std::vector<uint8_t>* buffer;
};

auto curl_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t
{
    auto* ctx = static_cast<curl_write_context*>(userdata);
    auto bytes = size * nmemb;
    ctx->buffer->insert(ctx->buffer->end(), ptr, ptr + bytes);
    return bytes;
}

struct curl_progress_context
{
    progress_callback* callback;
};

auto curl_progress_callback(void* clientp,
                             curl_off_t dltotal,
                             curl_off_t dlnow,
                             curl_off_t /*ultotal*/,
                             curl_off_t /*ulnow*/) -> int
{
    auto* ctx = static_cast<curl_progress_context*>(clientp);
    if (ctx->callback && *ctx->callback)
    {
        bool should_continue = (*ctx->callback)(static_cast<uint64_t>(dlnow),
                                                 static_cast<uint64_t>(dltotal));
        return should_continue ? 0 : 1; // return non-zero to abort
    }
    return 0;
}

} // namespace

downloader::downloader(int timeout_seconds)
    : timeout_seconds_(timeout_seconds)
{
    curl_ = curl_easy_init();
}

downloader::~downloader()
{
    if (curl_)
    {
        curl_easy_cleanup(static_cast<CURL*>(curl_));
    }
}

auto downloader::fetch(const std::string& url, progress_callback on_progress)
    -> std::expected<download_result, std::string>
{
    if (!curl_)
    {
        return std::unexpected("curl not initialized");
    }

    auto* handle = static_cast<CURL*>(curl_);

    download_result result;
    curl_write_context write_ctx{&result.data};
    curl_progress_context progress_ctx{&on_progress};

    curl_easy_reset(handle);
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &write_ctx);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds_));
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 0L);

    // User agent
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "HelbreathPatcher/1.0");

    // Progress callback
    if (on_progress)
    {
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, curl_progress_callback);
        curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &progress_ctx);
    }

    auto res = curl_easy_perform(handle);
    if (res != CURLE_OK)
    {
        return std::unexpected(std::string("download failed: ") + curl_easy_strerror(res));
    }

    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result.http_status);

    if (result.http_status >= 400)
    {
        return std::unexpected("HTTP " + std::to_string(result.http_status) + " for " + url);
    }

    return result;
}

auto downloader::fetch_with_zstd_fallback(const std::string& url,
                                           uint64_t /*expected_size*/,
                                           progress_callback on_progress)
    -> std::expected<std::vector<uint8_t>, std::string>
{
    // Try .zst first
    auto zst_url = url + ".zst";
    auto zst_result = fetch(zst_url, on_progress);

    if (zst_result && zst_result->http_status == 200)
    {
        spdlog::debug("downloaded compressed: {}", zst_url);
        auto decompressed = decompress_zstd(zst_result->data.data(), zst_result->data.size());
        if (decompressed)
        {
            return *decompressed;
        }
        spdlog::warn("zstd decompression failed for {}: {}, falling back to raw", zst_url, decompressed.error());
    }

    // Fall back to raw
    auto raw_result = fetch(url, on_progress);
    if (!raw_result)
    {
        return std::unexpected(raw_result.error());
    }

    return std::move(raw_result->data);
}

} // namespace hb::patcher
