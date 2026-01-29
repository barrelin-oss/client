#pragma once

#include <SFML/Window.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace hb {

class input {
public:
    // Process a single event
    void process_event(const sf::Event& event);

    // Call at end of frame to reset per-frame state
    void end_frame();

    // Mouse state
    int32_t mouse_x() const { return mouse_x_; }
    int32_t mouse_y() const { return mouse_y_; }
    int32_t wheel_delta() const { return wheel_delta_; }

    bool is_mouse_down(sf::Mouse::Button btn) const;
    bool is_mouse_pressed(sf::Mouse::Button btn) const;
    bool is_mouse_released(sf::Mouse::Button btn) const;

    // Keyboard state
    bool is_key_down(sf::Keyboard::Key key) const;
    bool is_key_pressed(sf::Keyboard::Key key) const;
    bool is_key_released(sf::Keyboard::Key key) const;

    // Text input (accumulated during frame)
    std::string_view text_input() const { return text_input_; }
    void clear_text_input() { text_input_.clear(); }

    // Window state
    bool should_close() const { return should_close_; }
    bool has_focus() const { return has_focus_; }

private:
    static constexpr size_t max_mouse_buttons = 5;
    static constexpr size_t max_keys = 256;

    int32_t mouse_x_ = 0;
    int32_t mouse_y_ = 0;
    int32_t wheel_delta_ = 0;

    std::array<bool, max_mouse_buttons> mouse_down_{};
    std::array<bool, max_mouse_buttons> mouse_pressed_{};
    std::array<bool, max_mouse_buttons> mouse_released_{};

    std::array<bool, max_keys> key_down_{};
    std::array<bool, max_keys> key_pressed_{};
    std::array<bool, max_keys> key_released_{};

    std::string text_input_;

    bool should_close_ = false;
    bool has_focus_ = true;
};

} // namespace hb
