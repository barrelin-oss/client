#pragma once

#include "ui/ui_system.hpp"
#include <string>
#include <functional>

namespace hb {

// Simple connection status dialog
// Shows "Waiting for response from the server" with animated dots
// After 7 seconds, shows "press escape to cancel"
class connection_dialog : public dialog {
public:
    connection_dialog();
    ~connection_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_key_press(sf::Keyboard::Key key) override;

    // Reset timer when opening the dialog
    void open();

    // Callback when cancelled (via escape or cancel button)
    using cancel_callback = std::function<void()>;
    void set_on_cancel(cancel_callback cb) { on_cancel_ = std::move(cb); }

    // Show as error dialog with Cancel button instead of waiting message
    void set_error_mode(bool error, std::string_view error_message = "");

private:
    cancel_callback on_cancel_;

    // Error mode
    bool error_mode_ = false;
    std::string error_message_;

    // Timing
    float elapsed_time_ = 0.0f;
    float dot_timer_ = 0.0f;
    int32_t dot_count_ = 0;

    // Button hover state
    bool cancel_hovered_ = false;

    // Layout constants
    static constexpr int32_t dialog_width = 320;
    static constexpr int32_t dialog_height = 100;

    // Timing constants
    static constexpr float escape_hint_delay = 7.0f;

    // Button layout (relative to dialog origin)
    static constexpr int32_t button_width = 80;
    static constexpr int32_t button_height = 22;
    static constexpr int32_t button_rel_x = (dialog_width - button_width) / 2;
    static constexpr int32_t button_rel_y = 65;

    // Dynamic dialog position (centered on actual window, updated each render)
    int32_t actual_x_ = 0;
    int32_t actual_y_ = 0;
    uint32_t window_width_ = 640;
    uint32_t window_height_ = 480;

    bool is_point_in_cancel_button(int32_t x, int32_t y) const;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
};

} // namespace hb
