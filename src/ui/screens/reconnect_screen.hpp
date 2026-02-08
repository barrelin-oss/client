#pragma once

#include "ui/screens/screen_base.hpp"
#include <functional>
#include <string>

namespace hb {

// Screen shown when launcher-provided credentials are available.
// Displays the HELBREATH XTREME fire title with a single "Reconnect" button.
// On first show, auto-fires the reconnect callback. On subsequent shows
// (after disconnect), the user must click the button.
class reconnect_screen : public screen_base
{
public:
    using reconnect_callback = std::function<void()>;
    using exit_callback = std::function<void()>;

    reconnect_screen() = default;
    ~reconnect_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;

    void set_on_reconnect(reconnect_callback callback) { on_reconnect_ = std::move(callback); }
    void set_on_exit(exit_callback callback) { on_exit_game_ = std::move(callback); }

    // When true, the screen will auto-fire the reconnect callback on the first update
    void set_auto_connect(bool auto_connect) { auto_connect_ = auto_connect; }

    // Set optional status text below the button (e.g. "Connecting...", "Connection failed")
    void set_status(std::string_view status) { status_text_ = status; }

private:
    static constexpr int32_t button_width_ = 160;
    static constexpr int32_t button_height_ = 28;

    reconnect_callback on_reconnect_;
    exit_callback on_exit_game_;
    float elapsed_time_ = 0.0f;
    bool auto_connect_ = false;
    std::string status_text_;
    int32_t screen_width_ = 0;
    int32_t screen_height_ = 0;
};

} // namespace hb
