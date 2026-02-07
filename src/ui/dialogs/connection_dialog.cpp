#include "ui/dialogs/connection_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include <cstring>

namespace hb {

connection_dialog::connection_dialog()
    : dialog(dialog_type::connection) {
    set_title("");
    set_bounds({dialog_x, dialog_y, dialog_width, dialog_height});
    set_closeable(false);
    set_draggable(false);
    set_modal(true);
    set_has_border(false);
    set_background_color(sf::Color(0, 0, 0, 0));
}

void connection_dialog::open() {
    dialog::open();
    // Reset timers when opened
    elapsed_time_ = 0.0f;
    dot_timer_ = 0.0f;
    dot_count_ = 0;
}

void connection_dialog::set_error_mode(bool error, std::string_view error_message) {
    error_mode_ = error;
    error_message_ = error_message;
}

void connection_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;

    if (!error_mode_) {
        // Track elapsed time for escape hint
        elapsed_time_ += delta_time;

        // Animate dots
        dot_timer_ += delta_time;
        if (dot_timer_ >= 0.4f) {
            dot_timer_ = 0.0f;
            dot_count_ = (dot_count_ + 1) % 4;
        }
    }

    // Update button hover state (cancel button is always visible)
    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();
    cancel_hovered_ = is_point_in_cancel_button(mx, my);
}

void connection_dialog::render(renderer& rend) {
    if (!visible_) return;

    // Draw semi-transparent dark background overlay
    rend.draw_rect(0, 0, 640, 480, sf::Color(0, 0, 0, 128), true);

    // Draw dialog box background
    rend.draw_rect(dialog_x, dialog_y, dialog_width, dialog_height,
                   sf::Color(32, 32, 48, 240), true);

    // Draw border
    rend.draw_rect(dialog_x, dialog_y, dialog_width, dialog_height,
                   sf::Color(80, 80, 120), false);
    rend.draw_rect(dialog_x + 1, dialog_y + 1, dialog_width - 2, dialog_height - 2,
                   sf::Color(60, 60, 90), false);

    // Outline color for all text
    sf::Color outline_color(0, 0, 0);

    if (error_mode_) {
        // Error mode: show error message and Cancel button

        // Draw error message (centered with offset fix)
        int32_t text_width = static_cast<int32_t>(error_message_.length()) * 7;
        int32_t text_x = dialog_x + (dialog_width - text_width) / 2 + 20;
        int32_t text_y = dialog_y + 30;

        rend.draw_text_outlined(error_message_, text_x, text_y,
                                sf::Color(255, 200, 200), outline_color);

        // Draw Cancel button
        int32_t btn_x = dialog_x + button_x;
        int32_t btn_y = dialog_y + button_y;

        sf::Color btn_bg = cancel_hovered_ ? sf::Color(70, 70, 100) : sf::Color(50, 50, 70);
        rend.draw_rect(btn_x, btn_y, button_width, button_height, btn_bg, true);
        rend.draw_rect(btn_x, btn_y, button_width, button_height, sf::Color(100, 100, 140), false);

        // Button text
        const char* btn_text = "Cancel";
        int32_t btn_text_width = static_cast<int32_t>(strlen(btn_text)) * 7;
        int32_t btn_text_x = btn_x + (button_width - btn_text_width) / 2 + 15;
        int32_t btn_text_y = btn_y + 4;
        rend.draw_text_outlined(btn_text, btn_text_x, btn_text_y,
                                cancel_hovered_ ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220),
                                outline_color);
    } else {
        // Normal mode: show waiting message

        // Build main message with animated dots
        std::string main_msg = "Waiting for response from the server";
        for (int i = 0; i < dot_count_; ++i) {
            main_msg += ".";
        }

        // Draw main message (centered with offset fix)
        int32_t main_width = static_cast<int32_t>(main_msg.length()) * 7;
        int32_t main_x = dialog_x + (dialog_width - main_width) / 2 + 20;
        int32_t main_y = dialog_y + 30;

        rend.draw_text_outlined(main_msg, main_x, main_y,
                                sf::Color(255, 255, 200), outline_color);

        // Draw Cancel button
        int32_t btn_x = dialog_x + button_x;
        int32_t btn_y = dialog_y + button_y;

        sf::Color btn_bg = cancel_hovered_ ? sf::Color(70, 70, 100) : sf::Color(50, 50, 70);
        rend.draw_rect(btn_x, btn_y, button_width, button_height, btn_bg, true);
        rend.draw_rect(btn_x, btn_y, button_width, button_height, sf::Color(100, 100, 140), false);

        // Button text
        const char* btn_text = "Cancel";
        int32_t btn_text_width = static_cast<int32_t>(strlen(btn_text)) * 7;
        int32_t btn_text_x = btn_x + (button_width - btn_text_width) / 2 + 15;
        int32_t btn_text_y = btn_y + 4;
        rend.draw_text_outlined(btn_text, btn_text_x, btn_text_y,
                                cancel_hovered_ ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220),
                                outline_color);
    }
}

bool connection_dialog::handle_key_press(sf::Keyboard::Key key) {
    if (!visible_) return false;

    // Only Escape cancels the connection (not Enter)
    // Enter is consumed but does nothing - prevents it from passing through
    if (key == sf::Keyboard::Key::Escape) {
        if (on_cancel_) on_cancel_();
        close();
        return true;
    }

    // In error mode, Enter also dismisses the dialog
    if (error_mode_ && key == sf::Keyboard::Key::Enter) {
        if (on_cancel_) on_cancel_();
        close();
        return true;
    }

    // Modal dialog consumes all key presses to prevent pass-through
    return true;
}

bool connection_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || btn != sf::Mouse::Button::Left) return false;

    // Cancel button is always clickable
    if (is_point_in_cancel_button(x, y)) {
        if (on_cancel_) on_cancel_();
        close();
        return true;
    }

    // Modal dialog consumes all clicks
    return true;
}

bool connection_dialog::is_point_in_cancel_button(int32_t x, int32_t y) const {
    int32_t btn_x = dialog_x + button_x;
    int32_t btn_y = dialog_y + button_y;
    return x >= btn_x && x < btn_x + button_width &&
           y >= btn_y && y < btn_y + button_height;
}

} // namespace hb
