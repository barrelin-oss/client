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

    init_resolution_options();
    init_framerate_options();
}

void settings_dialog::init_resolution_options() {
    resolution_options_ = {
        {640, 480, "640x480"},
        {800, 600, "800x600"},
        {1024, 768, "1024x768"},
        {1280, 720, "1280x720 (HD)"},
        {1280, 1024, "1280x1024"},
        {1366, 768, "1366x768"},
        {1600, 900, "1600x900"},
        {1920, 1080, "1920x1080 (Full HD)"}
    };
    selected_resolution_ = 0;  // Default to 640x480
}

void settings_dialog::init_framerate_options() {
    framerate_options_ = {
        {30, "30 FPS"},
        {60, "60 FPS"},
        {120, "120 FPS"},
        {144, "144 FPS"},
        {200, "200 FPS"},
        {240, "240 FPS"},
        {0, "Unlimited"}
    };
    selected_framerate_ = 1;  // Default to 60 FPS
}

void settings_dialog::set_framerate(uint32_t fps) {
    for (size_t i = 0; i < framerate_options_.size(); ++i) {
        if (framerate_options_[i].fps == fps) {
            selected_framerate_ = static_cast<int32_t>(i);
            return;
        }
    }
    // If not found, default to 60 FPS
    selected_framerate_ = 1;
}

uint32_t settings_dialog::get_framerate() const {
    if (selected_framerate_ >= 0 && selected_framerate_ < static_cast<int32_t>(framerate_options_.size())) {
        return framerate_options_[selected_framerate_].fps;
    }
    return 60;  // Default to 60 FPS
}

void settings_dialog::set_resolution(uint32_t width, uint32_t height) {
    for (size_t i = 0; i < resolution_options_.size(); ++i) {
        if (resolution_options_[i].width == width && resolution_options_[i].height == height) {
            selected_resolution_ = static_cast<int32_t>(i);
            applied_resolution_ = selected_resolution_;
            return;
        }
    }
    // If not found, default to first option
    selected_resolution_ = 0;
    applied_resolution_ = 0;
}

void settings_dialog::get_resolution(uint32_t& width, uint32_t& height) const {
    if (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size())) {
        width = resolution_options_[selected_resolution_].width;
        height = resolution_options_[selected_resolution_].height;
    } else {
        width = 640;
        height = 480;
    }
}

void settings_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;

    dialog::update(delta_time, inp);

    // Animate resolution dropdown
    if (resolution_dropdown_expanded_) {
        resolution_dropdown_animation_ += delta_time * dropdown_animation_speed_;
        if (resolution_dropdown_animation_ > 1.0f) resolution_dropdown_animation_ = 1.0f;
    } else {
        resolution_dropdown_animation_ -= delta_time * dropdown_animation_speed_;
        if (resolution_dropdown_animation_ < 0.0f) resolution_dropdown_animation_ = 0.0f;
    }

    // Animate framerate dropdown
    if (framerate_dropdown_expanded_) {
        framerate_dropdown_animation_ += delta_time * dropdown_animation_speed_;
        if (framerate_dropdown_animation_ > 1.0f) framerate_dropdown_animation_ = 1.0f;
    } else {
        framerate_dropdown_animation_ -= delta_time * dropdown_animation_speed_;
        if (framerate_dropdown_animation_ < 0.0f) framerate_dropdown_animation_ = 0.0f;
    }

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

    // Update resolution dropdown hover state
    if (resolution_dropdown_expanded_) {
        int32_t dropdown_y = bounds_.y + 40 + 25 + 28 + 40 + 25;  // Match render position
        int32_t dropdown_x = bounds_.x + 100;
        int32_t dropdown_width = 170;
        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;

        resolution_dropdown_hovered_ = -1;
        if (inp.mouse_x() >= dropdown_x && inp.mouse_x() < dropdown_x + dropdown_width) {
            int32_t mouse_y = inp.mouse_y();
            if (mouse_y >= list_y) {
                int32_t item_index = (mouse_y - list_y) / item_height;
                if (item_index >= 0 && item_index < static_cast<int32_t>(resolution_options_.size())) {
                    resolution_dropdown_hovered_ = item_index;
                }
            }
        }
    }

    // Update framerate dropdown hover state
    if (framerate_dropdown_expanded_) {
        int32_t dropdown_y = bounds_.y + 40 + 25 + 28 + 40 + 25 + 32;  // After resolution dropdown
        int32_t dropdown_x = bounds_.x + 100;
        int32_t dropdown_width = 170;
        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;

        framerate_dropdown_hovered_ = -1;
        if (inp.mouse_x() >= dropdown_x && inp.mouse_x() < dropdown_x + dropdown_width) {
            int32_t mouse_y = inp.mouse_y();
            if (mouse_y >= list_y) {
                int32_t item_index = (mouse_y - list_y) / item_height;
                if (item_index >= 0 && item_index < static_cast<int32_t>(framerate_options_.size())) {
                    framerate_dropdown_hovered_ = item_index;
                }
            }
        }
    }
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

    // Video section
    render_section_header(rend, y, "Video");
    y += 25;

    // Resolution dropdown
    std::string res_text = selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size())
        ? resolution_options_[selected_resolution_].label : "Select...";
    render_dropdown(rend, y, "Resolution", res_text,
                    hovered_element_ == elem_resolution_dropdown,
                    resolution_dropdown_animation_);
    y += 32;

    // Framerate dropdown
    std::string fps_text = selected_framerate_ >= 0 && selected_framerate_ < static_cast<int32_t>(framerate_options_.size())
        ? framerate_options_[selected_framerate_].label : "Select...";
    render_dropdown(rend, y, "Framerate", fps_text,
                    hovered_element_ == elem_framerate_dropdown,
                    framerate_dropdown_animation_);
    y += 32;

    // Fullscreen checkbox
    render_checkbox(rend, y, "Fullscreen",
                    fullscreen_,
                    hovered_element_ == elem_fullscreen_checkbox);
    y += 28;

    // VSync checkbox
    render_checkbox(rend, y, "VSync",
                    vsync_,
                    hovered_element_ == elem_vsync_checkbox);
    y += 28;

    // Remember window position checkbox
    render_checkbox(rend, y, "Remember Window Position",
                    remember_position_,
                    hovered_element_ == elem_remember_position_checkbox);
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

    // Render resolution dropdown list on top if expanded or animating
    if (resolution_dropdown_animation_ > 0.0f) {
        int32_t dropdown_y = bounds_.y + 40 + 25 + 28 + 40 + 25;  // Match render position
        int32_t dropdown_x = bounds_.x + 100;
        int32_t dropdown_width = 170;
        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;
        int32_t full_list_height = static_cast<int32_t>(resolution_options_.size()) * item_height;

        // Apply ease-out for smooth deceleration
        float eased = 1.0f - (1.0f - resolution_dropdown_animation_) * (1.0f - resolution_dropdown_animation_);
        int32_t current_height = static_cast<int32_t>(full_list_height * eased);

        if (current_height > 0) {
            // List background at animated height
            rend.draw_rect(dropdown_x, list_y, dropdown_width, current_height, sf::Color(35, 35, 50), true);

            // Scissor clip for items
            rend.push_scissor(dropdown_x, list_y, dropdown_width, current_height);

            // List items
            for (size_t i = 0; i < resolution_options_.size(); ++i) {
                int32_t item_y = list_y + static_cast<int32_t>(i) * item_height;

                // Highlight hovered or selected item
                if (static_cast<int32_t>(i) == resolution_dropdown_hovered_) {
                    rend.draw_rect(dropdown_x + 1, item_y, dropdown_width - 2, item_height, sf::Color(60, 80, 120), true);
                } else if (static_cast<int32_t>(i) == selected_resolution_) {
                    rend.draw_rect(dropdown_x + 1, item_y, dropdown_width - 2, item_height, sf::Color(50, 100, 160), true);
                }

                rend.draw_text(resolution_options_[i].label, dropdown_x + 8, item_y + 4, sf::Color::White, 11);
            }

            rend.pop_scissor();

            // Border at animated height (after scissor so not clipped)
            rend.draw_rect(dropdown_x, list_y, dropdown_width, current_height, sf::Color(80, 80, 100), false);
        }
    }

    // Render framerate dropdown list on top if expanded or animating
    if (framerate_dropdown_animation_ > 0.0f) {
        int32_t dropdown_y = bounds_.y + 40 + 25 + 28 + 40 + 25 + 32;  // After resolution dropdown
        int32_t dropdown_x = bounds_.x + 100;
        int32_t dropdown_width = 170;
        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;
        int32_t full_list_height = static_cast<int32_t>(framerate_options_.size()) * item_height;

        // Apply ease-out for smooth deceleration
        float eased = 1.0f - (1.0f - framerate_dropdown_animation_) * (1.0f - framerate_dropdown_animation_);
        int32_t current_height = static_cast<int32_t>(full_list_height * eased);

        if (current_height > 0) {
            // List background at animated height
            rend.draw_rect(dropdown_x, list_y, dropdown_width, current_height, sf::Color(35, 35, 50), true);

            // Scissor clip for items
            rend.push_scissor(dropdown_x, list_y, dropdown_width, current_height);

            // List items
            for (size_t i = 0; i < framerate_options_.size(); ++i) {
                int32_t item_y = list_y + static_cast<int32_t>(i) * item_height;

                // Highlight hovered or selected item
                if (static_cast<int32_t>(i) == framerate_dropdown_hovered_) {
                    rend.draw_rect(dropdown_x + 1, item_y, dropdown_width - 2, item_height, sf::Color(60, 80, 120), true);
                } else if (static_cast<int32_t>(i) == selected_framerate_) {
                    rend.draw_rect(dropdown_x + 1, item_y, dropdown_width - 2, item_height, sf::Color(50, 100, 160), true);
                }

                rend.draw_text(framerate_options_[i].label, dropdown_x + 8, item_y + 4, sf::Color::White, 11);
            }

            rend.pop_scissor();

            // Border at animated height (after scissor so not clipped)
            rend.draw_rect(dropdown_x, list_y, dropdown_width, current_height, sf::Color(80, 80, 100), false);
        }
    }
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

void settings_dialog::render_dropdown(renderer& rend, int32_t y, const char* label, const std::string& value, bool hovered, float animation) {
    int32_t label_x = bounds_.x + 25;
    int32_t dropdown_x = bounds_.x + 100;
    int32_t dropdown_width = 170;
    int32_t dropdown_height = 24;

    // Label
    rend.draw_text(label, label_x, y + 5, sf::Color(200, 200, 220), 11);

    // Dropdown header background - highlight when animating too
    sf::Color header_bg = (hovered || animation > 0.0f) ? sf::Color(60, 60, 80) : sf::Color(50, 50, 70);
    rend.draw_rect(dropdown_x, y, dropdown_width, dropdown_height, header_bg, true);
    rend.draw_rect(dropdown_x, y, dropdown_width, dropdown_height, sf::Color(80, 80, 100), false);

    // Selected value text
    rend.draw_text(value, dropdown_x + 8, y + 5, sf::Color::White, 11);

    // Dropdown arrow - animate based on animation parameter
    int32_t arrow_x = dropdown_x + dropdown_width - 20;
    int32_t arrow_y = y + dropdown_height / 2;

    // Interpolate arrow position: +3 when closed (down arrow), -3 when open (up arrow)
    float arrow_offset = 3.0f * (1.0f - 2.0f * animation);
    int32_t arrow_top = arrow_y + static_cast<int32_t>(arrow_offset);
    int32_t arrow_bottom = arrow_y - static_cast<int32_t>(arrow_offset);

    rend.draw_line(arrow_x, arrow_top, arrow_x + 6, arrow_bottom, sf::Color(180, 180, 200));
    rend.draw_line(arrow_x + 6, arrow_bottom, arrow_x + 12, arrow_top, sf::Color(180, 180, 200));
}

void settings_dialog::render_checkbox(renderer& rend, int32_t y, const char* label, bool checked, bool hovered) {
    int32_t x = bounds_.x + 25;
    int32_t checkbox_size = 16;

    // Checkbox background
    sf::Color checkbox_bg = hovered ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);
    rend.draw_rect(x, y + 2, checkbox_size, checkbox_size, checkbox_bg, true);
    rend.draw_rect(x, y + 2, checkbox_size, checkbox_size, sf::Color(100, 100, 130), false);

    // Checkmark if checked
    if (checked) {
        rend.draw_rect(x + 3, y + 5, checkbox_size - 6, checkbox_size - 6,
                       sf::Color(100, 200, 100), true);
    }

    // Label
    sf::Color text_color = hovered ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220);
    rend.draw_text(label, x + checkbox_size + 10, y + 3, text_color, 11);
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
    y += 40;  // Skip to video section

    // Video section header at y, then +25 for content
    y += 25;

    // Resolution dropdown
    int32_t dropdown_x = bounds_.x + 100;
    int32_t dropdown_width = 170;
    if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width && mouse_y >= y && mouse_y < y + 24) {
        return elem_resolution_dropdown;
    }

    // If resolution dropdown is expanded, check for item clicks
    if (resolution_dropdown_expanded_) {
        int32_t list_y = y + 24;
        int32_t item_height = 22;
        int32_t list_height = static_cast<int32_t>(resolution_options_.size()) * item_height;

        if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width &&
            mouse_y >= list_y && mouse_y < list_y + list_height) {
            int32_t item_index = (mouse_y - list_y) / item_height;
            return elem_resolution_item_base + item_index;
        }
    }
    y += 32;

    // Framerate dropdown
    if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width && mouse_y >= y && mouse_y < y + 24) {
        return elem_framerate_dropdown;
    }

    // If framerate dropdown is expanded, check for item clicks
    if (framerate_dropdown_expanded_) {
        int32_t list_y = y + 24;
        int32_t item_height = 22;
        int32_t list_height = static_cast<int32_t>(framerate_options_.size()) * item_height;

        if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width &&
            mouse_y >= list_y && mouse_y < list_y + list_height) {
            int32_t item_index = (mouse_y - list_y) / item_height;
            return elem_framerate_item_base + item_index;
        }
    }
    y += 32;

    // Fullscreen checkbox
    if (mouse_x >= x && mouse_x < x + 200 && mouse_y >= y && mouse_y < y + 24) {
        return elem_fullscreen_checkbox;
    }
    y += 28;

    // VSync checkbox
    if (mouse_x >= x && mouse_x < x + 200 && mouse_y >= y && mouse_y < y + 24) {
        return elem_vsync_checkbox;
    }
    y += 28;

    // Remember window position checkbox
    if (mouse_x >= x && mouse_x < x + 200 && mouse_y >= y && mouse_y < y + 24) {
        return elem_remember_position_checkbox;
    }
    y += 40;  // Skip to audio section

    // Audio section header at y, then +25 for content
    y += 25;

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

    // Helper to close all dropdowns
    auto close_all_dropdowns = [this]() {
        resolution_dropdown_expanded_ = false;
        framerate_dropdown_expanded_ = false;
    };

    // Check title bar for dragging and close button (delegate to base class)
    // Title bar is the first 28 pixels
    if (btn == sf::Mouse::Button::Left && y >= bounds_.y && y < bounds_.y + 28) {
        // Check close button first
        if (closeable_) {
            int32_t close_x = bounds_.x + bounds_.width - 24;
            int32_t close_y = bounds_.y + 4;
            if (x >= close_x && x < close_x + 20 && y >= close_y && y < close_y + 20) {
                close_all_dropdowns();
                close();
                if (on_close_cb_) on_close_cb_();
                return true;
            }
        }

        // Start dragging if in title bar area
        if (draggable_ && x >= bounds_.x && x < bounds_.x + bounds_.width) {
            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    if (btn == sf::Mouse::Button::Left) {
        int32_t clicked = get_hovered_element(x, y);

        // Handle framerate dropdown item selection
        if (clicked >= elem_framerate_item_base) {
            int32_t item_index = clicked - elem_framerate_item_base;
            if (item_index >= 0 && item_index < static_cast<int32_t>(framerate_options_.size())) {
                selected_framerate_ = item_index;
                framerate_dropdown_expanded_ = false;
                spdlog::debug("Framerate selected: {}", framerate_options_[item_index].label);
                // Apply framerate change immediately
                if (on_framerate_change_) {
                    on_framerate_change_(framerate_options_[item_index].fps);
                }
            }
            return true;
        }

        // Handle resolution dropdown item selection
        if (clicked >= elem_resolution_item_base) {
            int32_t item_index = clicked - elem_resolution_item_base;
            if (item_index >= 0 && item_index < static_cast<int32_t>(resolution_options_.size())) {
                selected_resolution_ = item_index;
                resolution_dropdown_expanded_ = false;
                spdlog::debug("Resolution selected: {}", resolution_options_[item_index].label);
            }
            return true;
        }

        switch (clicked) {
            case elem_style_classic:
                close_all_dropdowns();
                current_style_ = ui_style::classic;
                if (on_style_change_) on_style_change_(current_style_);
                return true;

            case elem_style_modern:
                close_all_dropdowns();
                current_style_ = ui_style::modern;
                if (on_style_change_) on_style_change_(current_style_);
                return true;

            case elem_resolution_dropdown:
                framerate_dropdown_expanded_ = false;  // Close other dropdown
                resolution_dropdown_expanded_ = !resolution_dropdown_expanded_;
                return true;

            case elem_framerate_dropdown:
                resolution_dropdown_expanded_ = false;  // Close other dropdown
                framerate_dropdown_expanded_ = !framerate_dropdown_expanded_;
                return true;

            case elem_fullscreen_checkbox:
                close_all_dropdowns();
                fullscreen_ = !fullscreen_;
                return true;

            case elem_vsync_checkbox:
                close_all_dropdowns();
                vsync_ = !vsync_;
                if (on_vsync_change_) on_vsync_change_(vsync_);
                return true;

            case elem_remember_position_checkbox:
                close_all_dropdowns();
                remember_position_ = !remember_position_;
                if (on_remember_position_change_) on_remember_position_change_(remember_position_);
                return true;

            case elem_music_slider:
                close_all_dropdowns();
                dragging_slider_ = true;
                dragging_slider_index_ = elem_music_slider;
                return true;

            case elem_sound_slider:
                close_all_dropdowns();
                dragging_slider_ = true;
                dragging_slider_index_ = elem_sound_slider;
                return true;

            case elem_apply_button:
                close_all_dropdowns();
                skip_close_after_apply_ = false;  // Reset flag
                // Only apply resolution change if settings actually changed
                if (on_resolution_change_ && selected_resolution_ >= 0 &&
                    selected_resolution_ < static_cast<int32_t>(resolution_options_.size()) &&
                    (selected_resolution_ != applied_resolution_ || fullscreen_ != applied_fullscreen_)) {
                    const auto& res = resolution_options_[selected_resolution_];
                    on_resolution_change_(res.width, res.height, fullscreen_);
                    // Update applied values
                    applied_resolution_ = selected_resolution_;
                    applied_fullscreen_ = fullscreen_;
                }
                if (on_apply_) on_apply_();
                // Only close if resolution callback didn't request to keep open
                if (!skip_close_after_apply_) {
                    close();
                }
                skip_close_after_apply_ = false;  // Reset for next time
                return true;

            default:
                // Click outside dropdown - close all
                if (resolution_dropdown_expanded_ || framerate_dropdown_expanded_) {
                    close_all_dropdowns();
                    return true;
                }
                break;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

} // namespace hb
