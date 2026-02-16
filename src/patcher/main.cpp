#include "patcher/downloader.hpp"
#include "patcher/file_ops.hpp"
#include "patcher/manifest.hpp"
#include "patcher/patcher_config.hpp"
#include "patcher/ui.hpp"

#include <curl/curl.h>
#include <sodium.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace
{

namespace fs = std::filesystem;

auto get_patcher_name() -> std::string
{
#ifdef _WIN32
    return "helbreath.exe";
#else
    return "helbreath";
#endif
}

auto get_client_name(const hb::patcher::patcher_config& config) -> std::string
{
#ifdef _WIN32
    if (config.client_binary.find(".exe") == std::string::npos)
        return config.client_binary + ".exe";
#endif
    return config.client_binary;
}

auto get_platform_name() -> std::string
{
#ifdef _WIN32
    return "windows";
#else
    return "linux";
#endif
}

/// Build the full download base URL from config + manifest's base_url field.
auto build_base_url(const hb::patcher::patcher_config& config,
                    const hb::patcher::manifest& m) -> std::string
{
    if (!m.base_url.empty() && m.base_url.starts_with("http"))
        return m.base_url;

    auto base = config.server_url + "/" + config.channel;
    if (!m.base_url.empty())
        base += m.base_url;
    else
        base += "/files/";

    if (base.back() != '/')
        base += '/';

    return base;
}

/// Check for .new binary from a previous self-update and swap it in
auto handle_self_update() -> bool
{
    auto patcher_name = get_patcher_name();
    auto new_path = fs::path(patcher_name + ".new");
    auto old_path = fs::path(patcher_name + ".old");
    auto current_path = fs::path(patcher_name);

    // Clean up old binary from previous update
    if (fs::exists(old_path))
    {
        std::error_code ec;
        fs::remove(old_path, ec);
    }

    // If .new exists, swap it in
    if (fs::exists(new_path))
    {
        spdlog::info("self-update: swapping in new patcher binary");

        std::error_code ec;

        // Rename current -> .old
        if (fs::exists(current_path))
        {
            fs::rename(current_path, old_path, ec);
            if (ec)
            {
                spdlog::error("self-update: failed to rename current binary: {}", ec.message());
                return false;
            }
        }

        // Rename .new -> current
        fs::rename(new_path, current_path, ec);
        if (ec)
        {
            spdlog::error("self-update: failed to rename new binary: {}", ec.message());
            // Try to restore
            if (fs::exists(old_path))
            {
                fs::rename(old_path, current_path);
            }
            return false;
        }

#ifndef _WIN32
        // Set executable permission
        chmod(current_path.c_str(), 0755);

        // Re-exec
        spdlog::info("self-update: re-executing patcher");
        auto path_str = current_path.string();
        char* argv[] = {path_str.data(), nullptr};
        execv(path_str.c_str(), argv);
        // execv only returns on failure
        spdlog::error("self-update: execv failed");
#else
        // On Windows, launch new process and exit
        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};

        auto wide_path = current_path.wstring();
        if (CreateProcessW(wide_path.c_str(), nullptr, nullptr, nullptr,
                           FALSE, 0, nullptr, nullptr, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            std::exit(0);
        }
        spdlog::error("self-update: CreateProcess failed");
#endif
        return false;
    }

    return true;
}

auto launch_client(const std::string& client_binary) -> bool
{
    spdlog::info("launching client: {}", client_binary);

#ifdef _WIN32
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    auto wide_path = fs::path(client_binary).wstring();
    if (!CreateProcessW(wide_path.c_str(), nullptr, nullptr, nullptr,
                        FALSE, 0, nullptr, nullptr, &si, &pi))
    {
        spdlog::error("failed to launch client: error {}", GetLastError());
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    auto pid = fork();
    if (pid < 0)
    {
        spdlog::error("fork failed");
        return false;
    }
    if (pid == 0)
    {
        // Child process
        execl(client_binary.c_str(), client_binary.c_str(), nullptr);
        _exit(1); // execl only returns on failure
    }
    // Parent continues to exit
#endif

    return true;
}

auto load_local_manifest() -> std::optional<hb::patcher::manifest>
{
    auto path = fs::path("manifest.json");
    if (!fs::exists(path))
    {
        return std::nullopt;
    }

    std::ifstream file(path);
    if (!file)
    {
        return std::nullopt;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    auto result = hb::patcher::parse_manifest(content);
    if (!result)
    {
        spdlog::warn("failed to parse local manifest: {}", result.error());
        return std::nullopt;
    }

    return *result;
}

auto save_local_manifest(const hb::patcher::manifest& m) -> bool
{
    auto content = hb::patcher::serialize_manifest(m);
    std::ofstream file("manifest.json", std::ios::trunc);
    if (!file)
    {
        spdlog::error("failed to write local manifest");
        return false;
    }
    file << content;
    return file.good();
}

auto format_bytes(uint64_t bytes) -> std::string
{
    if (bytes < 1024)
        return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024)
        return std::to_string(bytes / 1024) + " KB";
    if (bytes < 1024ULL * 1024 * 1024)
    {
        auto mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f MB", mb);
        return buf;
    }
    auto gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f GB", gb);
    return buf;
}

enum class update_result
{
    up_to_date,
    updated,
    patcher_updated,
    failed,
    aborted,
};

// Thread-safe bridge between the background update thread and the main UI thread.
// The background thread pushes state changes; the main thread applies them to the UI.
struct ui_bridge
{
    std::mutex mutex;

    // Pending UI state (written by bg thread, read+cleared by main thread)
    std::string status;
    std::string detail;
    double progress = 0.0;
    bool channel_enabled = true;
    bool dirty = false;

    // Error dialog: bg thread sets pending_error and waits on error_cv.
    // Main thread shows the dialog, then sets error_dismissed and notifies.
    struct error_info
    {
        std::string title;
        std::string message;
    };
    std::optional<error_info> pending_error;
    std::condition_variable error_cv;
    bool error_dismissed = false;

    // Background thread result
    std::atomic<bool> finished{false};
    update_result result = update_result::failed;

    // Abort signal from main thread to background thread
    std::atomic<bool> abort_requested{false};

    // -- Called from background thread --

    void post_status(const std::string& s)
    {
        std::lock_guard lock(mutex);
        status = s;
        dirty = true;
    }

    void post_detail(const std::string& d)
    {
        std::lock_guard lock(mutex);
        detail = d;
        dirty = true;
    }

    void post_progress(double p)
    {
        std::lock_guard lock(mutex);
        progress = p;
        dirty = true;
    }

    void post_channel_enabled(bool enabled)
    {
        std::lock_guard lock(mutex);
        channel_enabled = enabled;
        dirty = true;
    }

    void post_error(const std::string& title, const std::string& message)
    {
        std::unique_lock lock(mutex);
        pending_error = error_info{title, message};
        error_dismissed = false;
        dirty = true;
        // Wait for main thread to show and dismiss the dialog
        error_cv.wait(lock, [this] { return error_dismissed; });
    }

    bool should_abort() const
    {
        return abort_requested.load();
    }

    // -- Called from main thread --

    void apply(hb::patcher::patcher_ui& ui)
    {
        std::lock_guard lock(mutex);
        if (!dirty)
            return;

        ui.set_status(status);
        ui.set_detail(detail);
        ui.set_progress(progress);
        ui.set_channel_enabled(channel_enabled);
        dirty = false;
    }

    auto take_error() -> std::optional<error_info>
    {
        std::lock_guard lock(mutex);
        if (!pending_error)
            return std::nullopt;
        auto err = std::move(*pending_error);
        pending_error.reset();
        return err;
    }

    void dismiss_error()
    {
        std::lock_guard lock(mutex);
        error_dismissed = true;
        error_cv.notify_one();
    }

    void reset()
    {
        std::lock_guard lock(mutex);
        status.clear();
        detail.clear();
        progress = 0.0;
        channel_enabled = true;
        dirty = false;
        pending_error.reset();
        error_dismissed = false;
        finished.store(false);
        result = update_result::failed;
        abort_requested.store(false);
    }
};

/// Fetch a manifest with retries.  Returns the parsed manifest or an error string.
auto fetch_manifest(const std::string& url,
                    const hb::patcher::patcher_config& config,
                    ui_bridge& bridge,
                    hb::patcher::downloader& dl) -> std::expected<hb::patcher::manifest, std::string>
{
    int retries = 0;
    while (true)
    {
        if (bridge.should_abort())
            return std::unexpected("aborted");

        auto result = dl.fetch(url);
        if (result)
        {
            std::string text(result->data.begin(), result->data.end());
            auto manifest = hb::patcher::parse_manifest(text);
            if (!manifest)
                return std::unexpected("failed to parse manifest: " + manifest.error());
            return *manifest;
        }

        retries++;
        if (retries > config.max_retries)
            return std::unexpected(result.error());

        spdlog::warn("manifest fetch attempt {} for {} failed: {}, retrying...", retries, url, result.error());
        bridge.post_detail("Retrying... (" + std::to_string(retries) + "/" +
                           std::to_string(config.max_retries) + ")");

        for (int s = 0; s < retries * 20 && !bridge.should_abort(); ++s)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/// Run the update check and download flow (called on background thread).
auto run_update(hb::patcher::patcher_config& config,
                ui_bridge& bridge,
                hb::patcher::downloader& dl) -> update_result
{
    auto platform = get_platform_name();
    auto shared_url = config.server_url + "/" + config.channel + "/manifest.json";
    auto platform_url = config.server_url + "/" + config.channel + "/" + platform + "/manifest.json";

    bridge.post_status("Checking for updates...");
    bridge.post_detail(shared_url);
    bridge.post_progress(0.0);
    bridge.post_channel_enabled(false);

    // --- Fetch and verify shared manifest ---

    auto shared_result = fetch_manifest(shared_url, config, bridge, dl);
    if (!shared_result)
    {
        if (bridge.should_abort())
            return update_result::aborted;

        spdlog::error("failed to fetch shared manifest: {}", shared_result.error());
        bridge.post_error("Update Check Failed",
                          "Could not reach the update server.\n\n" + shared_result.error() +
                          "\n\nThe game will launch with existing files.");
        bridge.post_channel_enabled(true);
        return update_result::failed;
    }

    if (config.verify_signatures)
    {
        if (config.public_key_hex.empty())
        {
            spdlog::error("signature verification enabled but no public key configured");
            bridge.post_error("Configuration Error",
                              "Signature verification is enabled but no public key is configured.\n"
                              "Set public_key_hex in patcher_config.json or set verify_signatures to false.");
            bridge.post_channel_enabled(true);
            return update_result::failed;
        }

        auto verify = hb::patcher::verify_manifest(*shared_result, config.public_key_hex);
        if (!verify)
        {
            spdlog::error("shared manifest signature FAILED: {}", verify.error());
            bridge.post_error("Security Error",
                              "The update manifest signature is invalid.\n\n"
                              "This could indicate a compromised update server.\n"
                              "The game will NOT be launched for your safety.\n\n" +
                              verify.error());
            bridge.post_channel_enabled(true);
            return update_result::failed;
        }
        spdlog::info("shared manifest signature verified OK");
    }

    if (bridge.should_abort())
        return update_result::aborted;

    // --- Fetch and verify platform manifest (optional — 404 is fine) ---

    bridge.post_detail(platform_url);
    std::optional<hb::patcher::manifest> platform_manifest;

    auto platform_result = fetch_manifest(platform_url, config, bridge, dl);
    if (platform_result)
    {
        if (config.verify_signatures)
        {
            auto verify = hb::patcher::verify_manifest(*platform_result, config.public_key_hex);
            if (!verify)
            {
                spdlog::error("{} manifest signature FAILED: {}", platform, verify.error());
                bridge.post_error("Security Error",
                                  "The " + platform + " manifest signature is invalid.\n\n" +
                                  verify.error());
                bridge.post_channel_enabled(true);
                return update_result::failed;
            }
            spdlog::info("{} manifest signature verified OK", platform);
        }
        platform_manifest = std::move(*platform_result);
    }
    else
    {
        spdlog::info("no {} manifest ({}), shared-only update", platform, platform_result.error());
    }

    if (bridge.should_abort())
        return update_result::aborted;

    // --- Merge manifests and build download URL map ---

    // Combined manifest: start with shared, overlay platform files
    hb::patcher::manifest combined = *shared_result;
    auto shared_base = build_base_url(config, *shared_result);

    // Map each file to its download base URL
    std::unordered_map<std::string, std::string> download_base;
    for (auto& [path, _] : combined.files)
        download_base[path] = shared_base;

    if (platform_manifest)
    {
        auto platform_base = build_base_url(config, *platform_manifest);
        for (auto& [path, entry] : platform_manifest->files)
        {
            combined.files[path] = entry; // platform overrides shared
            download_base[path] = platform_base;
        }

        // Use platform's game_version if present (binaries define the version)
        if (!platform_manifest->game_version.empty())
            combined.game_version = platform_manifest->game_version;
    }

    // --- Diff against local manifest ---

    auto local_manifest = load_local_manifest();
    hb::patcher::diff_result diff;

    if (local_manifest)
    {
        diff = hb::patcher::diff_manifests(combined, *local_manifest);
    }
    else
    {
        for (auto& [path, entry] : combined.files)
        {
            diff.to_download.push_back(path);
            diff.total_download_bytes += entry.size;
        }
        std::sort(diff.to_download.begin(), diff.to_download.end());
    }

    if (diff.to_download.empty() && diff.to_delete.empty())
    {
        spdlog::info("game is up to date (version {})", combined.game_version);
        bridge.post_status("Game is up to date!");
        bridge.post_progress(1.0);
        bridge.post_detail("Version " + combined.game_version);
        bridge.post_channel_enabled(true);
        return update_result::up_to_date;
    }

    // --- Download changed files ---

    spdlog::info("need to download {} files ({} total)", diff.to_download.size(),
                 format_bytes(diff.total_download_bytes));

    bridge.post_status("Downloading updates...");

    uint64_t total_downloaded = 0;
    bool patcher_updated = false;
    auto patcher_name = get_patcher_name();

    for (size_t i = 0; i < diff.to_download.size(); ++i)
    {
        if (bridge.should_abort())
        {
            spdlog::info("update aborted by user");
            return update_result::aborted;
        }

        auto& file_path = diff.to_download[i];
        auto& entry = combined.files.at(file_path);
        auto file_url = download_base.at(file_path) + file_path;

        bridge.post_detail("Downloading " + file_path +
                           " (" + std::to_string(i + 1) + "/" + std::to_string(diff.to_download.size()) + ")");

        // Download with retries
        std::vector<uint8_t> file_data;
        int retries = 0;
        bool download_ok = false;

        while (retries <= config.max_retries)
        {
            auto progress_cb = [&](uint64_t downloaded, uint64_t total) -> bool
            {
                double file_fraction = (total > 0) ? static_cast<double>(downloaded) / static_cast<double>(total) : 0.0;
                double file_contribution = static_cast<double>(entry.size) * file_fraction;
                double overall = static_cast<double>(total_downloaded + static_cast<uint64_t>(file_contribution)) /
                                 static_cast<double>(diff.total_download_bytes);
                bridge.post_progress(overall);
                return !bridge.should_abort();
            };

            auto result = dl.fetch_with_zstd_fallback(file_url, entry.size, progress_cb);
            if (result)
            {
                file_data = std::move(*result);
                download_ok = true;
                break;
            }

            retries++;
            if (retries > config.max_retries)
            {
                spdlog::error("failed to download {} after {} attempts: {}", file_path, retries, result.error());
                bridge.post_error("Download Error",
                                  "Failed to download: " + file_path + "\n\n" + result.error());
                bridge.post_channel_enabled(true);
                return update_result::failed;
            }

            spdlog::warn("download attempt {} for {} failed, retrying...", retries, file_path);

            for (int s = 0; s < retries * 10 && !bridge.should_abort(); ++s)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!download_ok)
        {
            continue;
        }

        // Determine write path
        auto write_path = fs::path(file_path);
        bool is_self_update = (file_path == patcher_name);

        if (is_self_update)
        {
            write_path = fs::path(patcher_name + ".new");
            patcher_updated = true;
        }

        // Atomic write with SHA-256 verification
        auto write_result = hb::patcher::atomic_write_verified(
            write_path, file_data.data(), file_data.size(),
            entry.sha256, entry.permissions);

        if (!write_result)
        {
            spdlog::error("failed to write {}: {}", file_path, write_result.error());

            // SHA-256 mismatch — retry download once more
            if (write_result.error().find("SHA-256 mismatch") != std::string::npos && retries == 0)
            {
                spdlog::warn("hash mismatch, retrying download of {}", file_path);
                auto retry = dl.fetch_with_zstd_fallback(file_url, entry.size);
                if (retry)
                {
                    auto retry_write = hb::patcher::atomic_write_verified(
                        write_path, retry->data(), retry->size(),
                        entry.sha256, entry.permissions);
                    if (retry_write)
                    {
                        total_downloaded += entry.size;
                        continue;
                    }
                }
            }

            bridge.post_error("Write Error",
                              "Failed to write file: " + file_path + "\n\n" + write_result.error());
            bridge.post_channel_enabled(true);
            return update_result::failed;
        }

        total_downloaded += entry.size;
    }

    // Delete removed files
    for (auto& file_path : diff.to_delete)
    {
        spdlog::info("removing: {}", file_path);
        hb::patcher::remove_file(file_path);
    }

    // Save combined local manifest (includes both shared + platform files)
    bridge.post_status("Finalizing...");
    bridge.post_progress(1.0);
    bridge.post_detail("");

    save_local_manifest(combined);

    if (patcher_updated)
    {
        return update_result::patcher_updated;
    }

    bridge.post_channel_enabled(true);
    return update_result::updated;
}

/// Launch run_update on a background thread.
auto start_update_thread(hb::patcher::patcher_config& config,
                         ui_bridge& bridge,
                         hb::patcher::downloader& dl) -> std::thread
{
    return std::thread(
        [&config, &bridge, &dl]
        {
            auto r = run_update(config, bridge, dl);
            bridge.result = r;
            bridge.finished.store(true);
        });
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    // Set working directory to the binary's directory so all relative paths resolve correctly
    // regardless of where the patcher is launched from
    {
        std::error_code ec;
#ifdef _WIN32
        wchar_t exe_path[MAX_PATH];
        if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH) != 0)
        {
            auto bin_dir = fs::path(exe_path).parent_path();
            fs::current_path(bin_dir, ec);
        }
#else
        auto exe_link = fs::read_symlink("/proc/self/exe", ec);
        if (!ec)
        {
            auto bin_dir = exe_link.parent_path();
            fs::current_path(bin_dir, ec);
        }
#endif
        if (ec)
        {
            // Non-fatal — fall back to whatever cwd we have
        }
    }

    // Init libraries
    spdlog::set_level(spdlog::level::info);

    if (sodium_init() < 0)
    {
        spdlog::error("failed to initialize libsodium");
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Clean up .old files from previous in-use DLL replacements
    hb::patcher::cleanup_old_files(fs::current_path());

    // Self-update swap
    handle_self_update();

    // Load config
    auto config_result = hb::patcher::load_patcher_config();
    if (!config_result)
    {
        spdlog::error("config error: {}", config_result.error());
        return 1;
    }
    auto config = *config_result;

    // Create UI
    auto ui = hb::patcher::create_patcher_ui();
    if (!ui->create())
    {
        spdlog::error("failed to create UI");
        return 1;
    }

    // Set up channel selector
    ui->set_channels({"stable", "test"}, config.channel);

    // Shared state
    ui_bridge bridge;
    bool channel_changed = false;

    ui->set_on_channel_changed(
        [&](const std::string& new_channel)
        {
            if (new_channel == config.channel)
                return;

            spdlog::info("channel changed: {} -> {}", config.channel, new_channel);
            config.channel = new_channel;
            hb::patcher::save_patcher_config(config);
            channel_changed = true;
        });

    // Create downloader
    hb::patcher::downloader dl(config.timeout_seconds);

    // Start initial update on background thread
    bridge.reset();
    auto worker = start_update_thread(config, bridge, dl);

    // For deferred launch after update completes
    int launch_countdown = -1; // -1 = not launching, >= 0 = ticks until launch
    update_result pending_result = update_result::failed;

    // Hand control to the platform event loop. The tick callback runs ~60x/sec
    // within the native event loop (g_timeout_add on GTK, WM_TIMER on Win32),
    // so the WM always sees us as responsive.
    ui->run_loop(
        [&]() -> bool
        {
            // Apply pending UI updates from background thread
            bridge.apply(*ui);

            // Show error dialogs (bg thread blocks until dismissed)
            if (auto err = bridge.take_error())
            {
                ui->show_error(err->title, err->message);
                bridge.dismiss_error();
            }

            // Handle channel switch: abort current work and restart
            if (channel_changed)
            {
                channel_changed = false;
                launch_countdown = -1;

                bridge.abort_requested.store(true);
                bridge.dismiss_error();
                if (worker.joinable())
                    worker.join();

                bridge.reset();
                worker = start_update_thread(config, bridge, dl);
                return true;
            }

            // Deferred launch countdown (gives user ~500ms to switch channels)
            if (launch_countdown > 0)
            {
                launch_countdown--;
                return true;
            }
            if (launch_countdown == 0)
            {
                launch_countdown = -1;

                if (pending_result == update_result::patcher_updated)
                {
                    spdlog::info("patcher binary updated — restarting");
                    ui->set_status("Patcher updated! Restarting...");
                    // Close will exit run_loop; we re-exec below
                    ui->close();
                    return false;
                }

                spdlog::info("launching client");
                ui->set_status("Launching game...");
                auto client = get_client_name(config);
                launch_client(client);
                ui->close();
                return false;
            }

            // Check if background work finished
            if (bridge.finished.load())
            {
                auto result = bridge.result;
                bridge.finished.store(false);

                if (worker.joinable())
                    worker.join();

                switch (result)
                {
                case update_result::up_to_date:
                case update_result::updated:
                case update_result::patcher_updated:
                    pending_result = result;
                    launch_countdown = 30; // ~500ms at 60fps
                    return true;

                case update_result::failed:
                    // Stay open — user can switch channels or close
                    return true;

                case update_result::aborted:
                    ui->close();
                    return false;
                }
            }

            return true;
        });

    // Clean up background thread
    bridge.abort_requested.store(true);
    bridge.dismiss_error();
    if (worker.joinable())
        worker.join();

    // Handle patcher self-update re-exec
    if (pending_result == update_result::patcher_updated)
    {
        curl_global_cleanup();
        auto patcher_name = get_patcher_name();
#ifdef _WIN32
        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        auto wide_path = fs::path(patcher_name).wstring();
        if (CreateProcessW(wide_path.c_str(), nullptr, nullptr, nullptr,
                           FALSE, 0, nullptr, nullptr, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
#else
        execl(patcher_name.c_str(), patcher_name.c_str(), nullptr);
#endif
        return 0;
    }

    curl_global_cleanup();
    return 0;
}
