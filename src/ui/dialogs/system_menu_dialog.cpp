#include "ui/dialogs/system_menu_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace hb {

// =============================================================================
// System Menu Dialog
// =============================================================================

system_menu_dialog::system_menu_dialog()
    : dialog(dialog_type::system_menu) {
    set_title("System Menu");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - dialog_width / 2,
        static_cast<int32_t>(screen_height) / 2 - dialog_height / 2,
        dialog_width,
        dialog_height
    });
    set_modal(true);
    set_closeable(true);
    set_draggable(true);
}

void system_menu_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;

    dialog::update(delta_time, inp);

    hovered_button_ = get_hovered_button(inp.mouse_x(), inp.mouse_y());
}

void system_menu_dialog::render(renderer& rend) {
    if (!visible_) return;

    // Draw dialog background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(30, 30, 45, 240), true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(80, 80, 100), false);

    // Title bar
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, 28,
                   sf::Color(50, 50, 70), true);
    rend.draw_text(title_, bounds_.x + 10, bounds_.y + 6, sf::Color::White, 12);

    // Close button
    if (closeable_) {
        int32_t close_x = bounds_.x + bounds_.width - 24;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 20, 20, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 6, close_y + 3, sf::Color::White, 12);
    }

    // Menu buttons
    static const char* button_labels[] = { "Settings", "Help", "Logout", "Exit Game" };

    for (int32_t i = 0; i < button_count; ++i) {
        render_button(rend, i, button_labels[i], hovered_button_ == i);
    }
}

void system_menu_dialog::render_button(renderer& rend, int32_t index, const char* text, bool hovered) {
    int32_t btn_x = bounds_.x + (dialog_width - button_width) / 2;
    int32_t btn_y = bounds_.y + button_start_y + index * (button_height + button_spacing);

    // Button background
    sf::Color bg_color = hovered ? sf::Color(70, 70, 100) : sf::Color(45, 45, 65);
    rend.draw_rect(btn_x, btn_y, button_width, button_height, bg_color, true);

    // Button border with 3D effect
    sf::Color border_light = hovered ? sf::Color(120, 120, 160) : sf::Color(90, 90, 120);
    sf::Color border_dark = sf::Color(30, 30, 40);

    rend.draw_line(btn_x, btn_y, btn_x + button_width - 1, btn_y, border_light);
    rend.draw_line(btn_x, btn_y, btn_x, btn_y + button_height - 1, border_light);
    rend.draw_line(btn_x + button_width - 1, btn_y, btn_x + button_width - 1, btn_y + button_height - 1, border_dark);
    rend.draw_line(btn_x, btn_y + button_height - 1, btn_x + button_width - 1, btn_y + button_height - 1, border_dark);

    // Button text (centered)
    int32_t text_width = static_cast<int32_t>(strlen(text)) * 7;
    int32_t text_x = btn_x + (button_width - text_width) / 2;
    int32_t text_y = btn_y + (button_height - 12) / 2;

    // Text shadow
    rend.draw_text(text, text_x + 1, text_y + 1, sf::Color(0, 0, 0), 12);
    rend.draw_text(text, text_x, text_y, hovered ? sf::Color(255, 255, 200) : sf::Color::White, 12);
}

int32_t system_menu_dialog::get_hovered_button(int32_t mouse_x, int32_t mouse_y) const {
    int32_t btn_x = bounds_.x + (dialog_width - button_width) / 2;

    for (int32_t i = 0; i < button_count; ++i) {
        int32_t btn_y = bounds_.y + button_start_y + i * (button_height + button_spacing);

        if (mouse_x >= btn_x && mouse_x < btn_x + button_width &&
            mouse_y >= btn_y && mouse_y < btn_y + button_height) {
            return i;
        }
    }

    return -1;
}

bool system_menu_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check close button first
    if (closeable_ && btn == sf::Mouse::Button::Left) {
        int32_t close_x = bounds_.x + bounds_.width - 24;
        int32_t close_y = bounds_.y + 4;
        if (x >= close_x && x < close_x + 20 && y >= close_y && y < close_y + 20) {
            close();
            return true;
        }
    }

    // Check menu buttons
    if (btn == sf::Mouse::Button::Left) {
        int32_t clicked = get_hovered_button(x, y);
        switch (clicked) {
            case 0:  // Settings
                if (on_settings_) on_settings_();
                close();
                return true;
            case 1:  // Help
                if (on_help_) on_help_();
                close();
                return true;
            case 2:  // Logout
                if (on_logout_) on_logout_();
                close();
                return true;
            case 3:  // Exit
                if (on_exit_) on_exit_();
                return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

// =============================================================================
// Settings Dialog
// =============================================================================

settings_dialog::settings_dialog()
    : dialog(dialog_type::options) {
    set_title("Settings");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - dialog_width / 2,
        static_cast<int32_t>(screen_height) / 2 - dialog_height / 2,
        dialog_width,
        dialog_height
    });
    set_modal(true);
    set_closeable(true);
    set_draggable(true);
}

void settings_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;

    dialog::update(delta_time, inp);

    // Handle slider dragging
    if (dragging_slider_ && inp.is_mouse_down(sf::Mouse::Button::Left)) {
        int32_t slider_x = bounds_.x + 120;
        int32_t slider_width = 150;

        float value = static_cast<float>(inp.mouse_x() - slider_x) / slider_width;
        value = std::clamp(value, 0.0f, 1.0f);

        if (dragging_slider_index_ == elem_music_slider) {
            music_volume_ = value;
            if (on_music_volume_change_) on_music_volume_change_(value);
        } else if (dragging_slider_index_ == elem_sound_slider) {
            sound_volume_ = value;
            if (on_sound_volume_change_) on_sound_volume_change_(value);
        }
    } else {
        dragging_slider_ = false;
        dragging_slider_index_ = -1;
    }

    hovered_element_ = get_hovered_element(inp.mouse_x(), inp.mouse_y());
}

void settings_dialog::render(renderer& rend) {
    if (!visible_) return;

    // Draw dialog background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(30, 30, 45, 245), true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(80, 80, 100), false);

    // Title bar
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, 28,
                   sf::Color(50, 50, 70), true);
    rend.draw_text(title_, bounds_.x + 10, bounds_.y + 6, sf::Color::White, 12);

    // Close button
    if (closeable_) {
        int32_t close_x = bounds_.x + bounds_.width - 24;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 20, 20, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 6, close_y + 3, sf::Color::White, 12);
    }

    int32_t y = bounds_.y + 40;

    // UI Style section
    render_section_header(rend, y, "User Interface");
    y += 25;

    // Classic option
    render_toggle_option(rend, y, "Classic (Original)",
                         current_style_ == ui_style::classic,
                         hovered_element_ == elem_style_classic);
    y += 28;

    // Modern option
    render_toggle_option(rend, y, "Modern",
                         current_style_ == ui_style::modern,
                         hovered_element_ == elem_style_modern);
    y += 40;

    // Audio section
    render_section_header(rend, y, "Audio");
    y += 25;

    // Music volume
    render_slider(rend, y, "Music", music_volume_, hovered_element_ == elem_music_slider);
    y += 35;

    // Sound volume
    render_slider(rend, y, "Sound", sound_volume_, hovered_element_ == elem_sound_slider);
    y += 45;

    // Apply button
    int32_t btn_width = 100;
    int32_t btn_height = 30;
    int32_t btn_x = bounds_.x + (dialog_width - btn_width) / 2;
    int32_t btn_y = bounds_.y + dialog_height - btn_height - 15;

    bool btn_hovered = hovered_element_ == elem_apply_button;
    sf::Color btn_bg = btn_hovered ? sf::Color(70, 100, 70) : sf::Color(50, 80, 50);
    rend.draw_rect(btn_x, btn_y, btn_width, btn_height, btn_bg, true);
    rend.draw_rect(btn_x, btn_y, btn_width, btn_height, sf::Color(100, 140, 100), false);

    rend.draw_text("Apply", btn_x + 32, btn_y + 8, btn_hovered ? sf::Color(200, 255, 200) : sf::Color::White, 12);
}

void settings_dialog::render_section_header(renderer& rend, int32_t y, const char* text) {
    int32_t x = bounds_.x + 15;

    // Section text
    rend.draw_text(text, x, y, sf::Color(180, 180, 220), 11);

    // Underline
    int32_t text_width = static_cast<int32_t>(strlen(text)) * 7;
    rend.draw_line(x, y + 14, x + text_width + 10, y + 14, sf::Color(80, 80, 120));
}

void settings_dialog::render_toggle_option(renderer& rend, int32_t y, const char* label, bool selected, bool hovered) {
    int32_t x = bounds_.x + 25;

    // Radio button circle
    int32_t radio_x = x;
    int32_t radio_y = y + 3;
    int32_t radio_size = 14;

    sf::Color circle_bg = hovered ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);
    rend.draw_rect(radio_x, radio_y, radio_size, radio_size, circle_bg, true);
    rend.draw_rect(radio_x, radio_y, radio_size, radio_size, sf::Color(100, 100, 130), false);

    // Selected indicator
    if (selected) {
        rend.draw_rect(radio_x + 3, radio_y + 3, radio_size - 6, radio_size - 6,
                       sf::Color(100, 200, 100), true);
    }

    // Label
    sf::Color text_color = hovered ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220);
    rend.draw_text(label, x + radio_size + 10, y + 2, text_color, 11);
}

void settings_dialog::render_slider(renderer& rend, int32_t y, const char* label, float value, bool hovered) {
    int32_t label_x = bounds_.x + 25;
    int32_t slider_x = bounds_.x + 120;
    int32_t slider_width = 150;
    int32_t slider_height = 16;

    // Label
    rend.draw_text(label, label_x, y + 2, sf::Color(200, 200, 220), 11);

    // Slider track
    sf::Color track_bg = hovered ? sf::Color(50, 50, 70) : sf::Color(35, 35, 50);
    rend.draw_rect(slider_x, y, slider_width, slider_height, track_bg, true);
    rend.draw_rect(slider_x, y, slider_width, slider_height, sf::Color(80, 80, 100), false);

    // Slider fill
    int32_t fill_width = static_cast<int32_t>(slider_width * value);
    if (fill_width > 0) {
        sf::Color fill_color = hovered ? sf::Color(80, 120, 180) : sf::Color(60, 100, 160);
        rend.draw_rect(slider_x, y, fill_width, slider_height, fill_color, true);
    }

    // Slider handle
    int32_t handle_x = slider_x + fill_width - 4;
    handle_x = std::clamp(handle_x, slider_x, slider_x + slider_width - 8);
    rend.draw_rect(handle_x, y - 2, 8, slider_height + 4, sf::Color(200, 200, 220), true);

    // Percentage text
    int32_t percent = static_cast<int32_t>(value * 100);
    std::string percent_str = std::to_string(percent) + "%";
    rend.draw_text(percent_str, slider_x + slider_width + 10, y + 2, sf::Color(150, 150, 180), 10);
}

int32_t settings_dialog::get_hovered_element(int32_t mouse_x, int32_t mouse_y) const {
    // Check UI style options
    int32_t y = bounds_.y + 65;
    int32_t x = bounds_.x + 25;

    // Classic option
    if (mouse_x >= x && mouse_x < x + 200 && mouse_y >= y && mouse_y < y + 24) {
        return elem_style_classic;
    }
    y += 28;

    // Modern option
    if (mouse_x >= x && mouse_x < x + 200 && mouse_y >= y && mouse_y < y + 24) {
        return elem_style_modern;
    }
    y += 65;  // Skip to audio section

    // Music slider
    int32_t slider_x = bounds_.x + 120;
    int32_t slider_width = 150;
    if (mouse_x >= slider_x && mouse_x < slider_x + slider_width && mouse_y >= y && mouse_y < y + 20) {
        return elem_music_slider;
    }
    y += 35;

    // Sound slider
    if (mouse_x >= slider_x && mouse_x < slider_x + slider_width && mouse_y >= y && mouse_y < y + 20) {
        return elem_sound_slider;
    }

    // Apply button
    int32_t btn_width = 100;
    int32_t btn_height = 30;
    int32_t btn_x = bounds_.x + (dialog_width - btn_width) / 2;
    int32_t btn_y = bounds_.y + dialog_height - btn_height - 15;

    if (mouse_x >= btn_x && mouse_x < btn_x + btn_width &&
        mouse_y >= btn_y && mouse_y < btn_y + btn_height) {
        return elem_apply_button;
    }

    return -1;
}

bool settings_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check close button
    if (closeable_ && btn == sf::Mouse::Button::Left) {
        int32_t close_x = bounds_.x + bounds_.width - 24;
        int32_t close_y = bounds_.y + 4;
        if (x >= close_x && x < close_x + 20 && y >= close_y && y < close_y + 20) {
            close();
            if (on_close_cb_) on_close_cb_();
            return true;
        }
    }

    if (btn == sf::Mouse::Button::Left) {
        int32_t clicked = get_hovered_element(x, y);

        switch (clicked) {
            case elem_style_classic:
                current_style_ = ui_style::classic;
                if (on_style_change_) on_style_change_(current_style_);
                return true;

            case elem_style_modern:
                current_style_ = ui_style::modern;
                if (on_style_change_) on_style_change_(current_style_);
                return true;

            case elem_music_slider:
                dragging_slider_ = true;
                dragging_slider_index_ = elem_music_slider;
                return true;

            case elem_sound_slider:
                dragging_slider_ = true;
                dragging_slider_index_ = elem_sound_slider;
                return true;

            case elem_apply_button:
                if (on_apply_) on_apply_();
                close();
                return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

} // namespace hb
