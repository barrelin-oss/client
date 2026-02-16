#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hb::patcher
{

/// Abstract patcher UI interface
class patcher_ui
{
public:
    virtual ~patcher_ui() = default;

    /// Create and show the window
    virtual auto create() -> bool = 0;

    /// Set the main status text (e.g., "Checking for updates...")
    virtual void set_status(const std::string& text) = 0;

    /// Set the detail text (e.g., "Downloading monster1.pak (3/12)")
    virtual void set_detail(const std::string& text) = 0;

    /// Set the progress bar value (0.0 to 1.0)
    virtual void set_progress(double fraction) = 0;

    /// Show an error dialog. Returns when user dismisses it.
    virtual void show_error(const std::string& title, const std::string& message) = 0;

    /// Run the platform event loop. Calls tick_callback ~60 times/sec.
    /// Returns when the window is closed or tick_callback returns false.
    virtual void run_loop(std::function<bool()> tick_callback) = 0;

    /// Close and destroy the window
    virtual void close() = 0;

    /// Check if the user closed the window
    virtual auto is_closed() const -> bool = 0;

    /// Set available channels and select the active one
    virtual void set_channels(const std::vector<std::string>& channels, const std::string& active) = 0;

    /// Enable or disable the channel selector (disable during downloads)
    virtual void set_channel_enabled(bool enabled) = 0;

    /// Register callback for when user changes the channel
    void set_on_channel_changed(std::function<void(const std::string&)> callback)
    {
        on_channel_changed_ = std::move(callback);
    }

protected:
    std::function<void(const std::string&)> on_channel_changed_;
};

/// Create the platform-specific UI implementation
auto create_patcher_ui() -> std::unique_ptr<patcher_ui>;

} // namespace hb::patcher
