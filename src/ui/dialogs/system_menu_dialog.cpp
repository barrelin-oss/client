#include "ui/dialogs/system_menu_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include "core/config.hpp"
#include <algorithm>
#include <format>
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

    // Enumerate monitors on construction
    set_monitors(enumerate_monitors());
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
        {1920, 1080, "1920x1080 (Full HD)"},
        {2560, 1440, "2560x1440 (QHD)"},
        {3840, 2160, "3840x2160 (4K)"}
    };
    selected_resolution_ = 0;  // Default to 640x480
}

void settings_dialog::rebuild_resolution_options() {
    // Get the selected monitor's native resolution
    int32_t max_w = 1920;
    int32_t max_h = 1080;
    if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size())) {
        max_w = monitor_options_[selected_monitor_].width;
        max_h = monitor_options_[selected_monitor_].height;
    }

    // Standard resolutions to offer
    struct standard_res { uint32_t w, h; const char* suffix; };
    static constexpr standard_res all_resolutions[] = {
        {640, 480, ""},
        {800, 600, ""},
        {1024, 768, ""},
        {1280, 720, " (HD)"},
        {1280, 1024, ""},
        {1366, 768, ""},
        {1600, 900, ""},
        {1920, 1080, " (Full HD)"},
        {2560, 1440, " (QHD)"},
        {3840, 2160, " (4K)"},
    };

    // Save current selection to try to preserve it
    uint32_t prev_w = 640, prev_h = 480;
    if (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size())) {
        prev_w = resolution_options_[selected_resolution_].width;
        prev_h = resolution_options_[selected_resolution_].height;
    }

    resolution_options_.clear();

    bool native_included = false;
    for (const auto& r : all_resolutions) {
        if (static_cast<int32_t>(r.w) <= max_w && static_cast<int32_t>(r.h) <= max_h) {
            std::string label = std::to_string(r.w) + "x" + std::to_string(r.h) + r.suffix;
            resolution_options_.push_back({r.w, r.h, std::move(label)});
            if (static_cast<int32_t>(r.w) == max_w && static_cast<int32_t>(r.h) == max_h) {
                native_included = true;
            }
        }
    }

    // Add the monitor's native resolution if not already in the list
    if (!native_included && max_w > 0 && max_h > 0) {
        std::string label = std::to_string(max_w) + "x" + std::to_string(max_h) + " (Native)";
        resolution_options_.push_back({static_cast<uint32_t>(max_w), static_cast<uint32_t>(max_h), std::move(label)});
    }

    // Restore previous selection if still valid
    selected_resolution_ = 0;
    for (size_t i = 0; i < resolution_options_.size(); ++i) {
        if (resolution_options_[i].width == prev_w && resolution_options_[i].height == prev_h) {
            selected_resolution_ = static_cast<int32_t>(i);
            break;
        }
    }
    applied_resolution_ = selected_resolution_;
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

void settings_dialog::set_monitors(std::vector<monitor_info> monitors) {
    monitors_ = std::move(monitors);
    monitor_options_.clear();

    for (size_t i = 0; i < monitors_.size(); ++i) {
        const auto& m = monitors_[i];
        monitor_option opt;
        opt.index = m.index;
        opt.x = m.x;
        opt.y = m.y;
        opt.width = m.width;
        opt.height = m.height;

        // Format: "Monitor 1 - 1920x1080 (primary)" or "Monitor 1 - 1920x1080 (0,0)"
        std::string label = "Monitor " + std::to_string(i + 1) + " - "
            + std::to_string(m.width) + "x" + std::to_string(m.height);
        if (m.primary) {
            label += " (primary)";
        } else {
            label += " (" + std::to_string(m.x) + "," + std::to_string(m.y) + ")";
        }
        opt.label = std::move(label);
        monitor_options_.push_back(std::move(opt));
    }

    if (monitor_options_.empty()) {
        // Fallback: create a single "unknown" monitor entry
        monitor_options_.push_back({0, "Monitor 1 - Unknown", 0, 0, 1920, 1080});
    }

    // Default to the primary monitor, not index 0
    selected_monitor_ = 0;
    for (size_t i = 0; i < monitors_.size(); ++i) {
        if (monitors_[i].primary) {
            selected_monitor_ = static_cast<int32_t>(i);
            break;
        }
    }
    applied_monitor_ = selected_monitor_;

    rebuild_resolution_options();
}

void settings_dialog::set_monitor_index(int32_t index) {
    for (size_t i = 0; i < monitor_options_.size(); ++i) {
        if (monitor_options_[i].index == index) {
            selected_monitor_ = static_cast<int32_t>(i);
            applied_monitor_ = selected_monitor_;
            rebuild_resolution_options();
            return;
        }
    }
    // Fall back to primary
    for (size_t i = 0; i < monitors_.size(); ++i) {
        if (monitors_[i].primary) {
            selected_monitor_ = static_cast<int32_t>(i);
            applied_monitor_ = selected_monitor_;
            rebuild_resolution_options();
            return;
        }
    }
    selected_monitor_ = 0;
    applied_monitor_ = 0;
    rebuild_resolution_options();
}

void settings_dialog::set_display_mode(bool fullscreen, bool borderless) {
    if (borderless) {
        selected_display_mode_ = 1;
    } else if (fullscreen) {
        selected_display_mode_ = 2;
    } else {
        selected_display_mode_ = 0;
    }
    applied_display_mode_ = selected_display_mode_;

    // Keep fullscreen_ in sync for backward compat
    fullscreen_ = fullscreen;
    applied_fullscreen_ = fullscreen;
}

void settings_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;

    dialog::update(delta_time, inp);

    // Revert countdown timer
    if (revert_countdown_active_) {
        revert_countdown_timer_ -= delta_time;
        if (revert_countdown_timer_ <= 0.0f) {
            // Timer expired - revert to previous display settings
            revert_countdown_active_ = false;
            selected_resolution_ = revert_state_.resolution_index;
            selected_display_mode_ = revert_state_.display_mode_index;
            selected_monitor_ = revert_state_.monitor_index;
            fullscreen_ = revert_state_.fullscreen;

            if (on_resolution_change_) {
                on_resolution_change_(revert_state_.width, revert_state_.height,
                                      revert_state_.fullscreen, revert_state_.borderless,
                                      revert_state_.monitor_x, revert_state_.monitor_y);
            }
            applied_resolution_ = selected_resolution_;
            applied_display_mode_ = selected_display_mode_;
            applied_monitor_ = selected_monitor_;
            applied_fullscreen_ = fullscreen_;
            spdlog::info("Display settings reverted (timeout)");
        }
    }

    // Animate all dropdowns
    auto animate = [&](bool expanded, float& animation) {
        if (expanded) {
            animation += delta_time * dropdown_animation_speed_;
            if (animation > 1.0f) animation = 1.0f;
        } else {
            animation -= delta_time * dropdown_animation_speed_;
            if (animation < 0.0f) animation = 0.0f;
        }
    };

    animate(monitor_dropdown_expanded_, monitor_dropdown_animation_);
    animate(display_mode_dropdown_expanded_, display_mode_dropdown_animation_);
    animate(resolution_dropdown_expanded_, resolution_dropdown_animation_);
    animate(framerate_dropdown_expanded_, framerate_dropdown_animation_);

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

    // Helper for dropdown hover
    auto update_dropdown_hover = [&](bool expanded, int32_t dropdown_y, int32_t count, int32_t& hovered) {
        if (!expanded) return;
        int32_t dropdown_x = bounds_.x + 100;
        int32_t dropdown_width = 170;
        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;

        hovered = -1;
        if (inp.mouse_x() >= dropdown_x && inp.mouse_x() < dropdown_x + dropdown_width) {
            int32_t my = inp.mouse_y();
            if (my >= list_y) {
                int32_t idx = (my - list_y) / item_height;
                if (idx >= 0 && idx < count) {
                    hovered = idx;
                }
            }
        }
    };

    // Compute Y positions for dropdown hover tracking
    // These must match the render layout exactly:
    // y starts at bounds_.y + 40
    // UI section: header(+25), classic(+28), modern(+40) = 93
    // Video section: header(+25), display_mode(+32), monitor(+32), resolution(+32), framerate(+32)
    int32_t video_y = bounds_.y + 40 + 25 + 28 + 40 + 25;  // First dropdown (display mode)
    update_dropdown_hover(display_mode_dropdown_expanded_, video_y,
                          display_mode_count_, display_mode_dropdown_hovered_);

    int32_t monitor_y = video_y + 32;
    update_dropdown_hover(monitor_dropdown_expanded_, monitor_y,
                          static_cast<int32_t>(monitor_options_.size()), monitor_dropdown_hovered_);

    int32_t resolution_y = monitor_y + 32;
    update_dropdown_hover(resolution_dropdown_expanded_, resolution_y,
                          static_cast<int32_t>(resolution_options_.size()), resolution_dropdown_hovered_);

    int32_t framerate_y = resolution_y + 32;
    update_dropdown_hover(framerate_dropdown_expanded_, framerate_y,
                          static_cast<int32_t>(framerate_options_.size()), framerate_dropdown_hovered_);
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

    render_toggle_option(rend, y, "Classic (Original)",
                         current_style_ == ui_style::classic,
                         hovered_element_ == elem_style_classic);
    y += 28;

    render_toggle_option(rend, y, "Modern",
                         current_style_ == ui_style::modern,
                         hovered_element_ == elem_style_modern);
    y += 40;

    // Video section
    render_section_header(rend, y, "Video");
    y += 25;

    // Display mode dropdown (first)
    std::string mode_text = (selected_display_mode_ >= 0 && selected_display_mode_ < display_mode_count_)
        ? display_mode_labels_[selected_display_mode_] : "Select...";
    render_dropdown(rend, y, "Mode", mode_text,
                    hovered_element_ == elem_display_mode_dropdown,
                    display_mode_dropdown_animation_);
    y += 32;

    // Monitor dropdown (disabled when windowed - only relevant for borderless/fullscreen)
    bool mon_disabled = (selected_display_mode_ == 0);  // Windowed
    std::string mon_text = (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size()))
        ? monitor_options_[selected_monitor_].label : "Select...";
    render_dropdown(rend, y, "Monitor", mon_text,
                    !mon_disabled && hovered_element_ == elem_monitor_dropdown,
                    monitor_dropdown_animation_);
    if (mon_disabled) {
        rend.draw_rect(bounds_.x + 100, y, 170, 24, sf::Color(30, 30, 45, 150), true);
    }
    y += 32;

    // Resolution dropdown (disabled when borderless)
    bool res_disabled = (selected_display_mode_ == 1);  // Borderless
    std::string res_text;
    if (res_disabled) {
        // Show native resolution of selected monitor
        if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size())) {
            const auto& mon = monitor_options_[selected_monitor_];
            res_text = std::to_string(mon.width) + "x" + std::to_string(mon.height) + " (Native)";
        } else {
            res_text = "Native";
        }
    } else {
        res_text = (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size()))
            ? resolution_options_[selected_resolution_].label : "Select...";
    }
    render_dropdown(rend, y, "Resolution", res_text,
                    !res_disabled && hovered_element_ == elem_resolution_dropdown,
                    resolution_dropdown_animation_);
    if (res_disabled) {
        rend.draw_rect(bounds_.x + 100, y, 170, 24, sf::Color(30, 30, 45, 150), true);
    }
    y += 32;

    // Framerate dropdown
    std::string fps_text = (selected_framerate_ >= 0 && selected_framerate_ < static_cast<int32_t>(framerate_options_.size()))
        ? framerate_options_[selected_framerate_].label : "Select...";
    render_dropdown(rend, y, "Framerate", fps_text,
                    hovered_element_ == elem_framerate_dropdown,
                    framerate_dropdown_animation_);
    y += 32;

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

    render_slider(rend, y, "Music", music_volume_, hovered_element_ == elem_music_slider);
    y += 35;

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

    // Revert countdown overlay
    if (revert_countdown_active_) {
        // Semi-transparent overlay covering the dialog
        rend.draw_rect(bounds_.x + 1, bounds_.y + 29, bounds_.width - 2, bounds_.height - 30,
                       sf::Color(0, 0, 0, 180), true);

        // Countdown text
        int32_t countdown_secs = static_cast<int32_t>(revert_countdown_timer_) + 1;
        std::string countdown_text = "Keep these display settings?";
        std::string timer_text = "Reverting in " + std::to_string(countdown_secs) + "s...";

        int32_t center_x = bounds_.x + dialog_width / 2;
        int32_t center_y = bounds_.y + dialog_height / 2 - 40;

        // Center the text
        int32_t text1_w = static_cast<int32_t>(countdown_text.size()) * 7;
        int32_t text2_w = static_cast<int32_t>(timer_text.size()) * 6;
        rend.draw_text(countdown_text, center_x - text1_w / 2, center_y, sf::Color::White, 13);
        rend.draw_text(timer_text, center_x - text2_w / 2, center_y + 22, sf::Color(255, 200, 100), 11);

        // Keep Changes button
        int32_t keep_w = 120, keep_h = 30;
        int32_t keep_x = center_x - keep_w - 10;
        int32_t keep_y = center_y + 55;
        bool keep_hovered = hovered_element_ == elem_keep_changes_button;
        sf::Color keep_bg = keep_hovered ? sf::Color(70, 120, 70) : sf::Color(50, 90, 50);
        rend.draw_rect(keep_x, keep_y, keep_w, keep_h, keep_bg, true);
        rend.draw_rect(keep_x, keep_y, keep_w, keep_h, sf::Color(100, 160, 100), false);
        rend.draw_text("Keep Changes", keep_x + 14, keep_y + 8,
                       keep_hovered ? sf::Color(200, 255, 200) : sf::Color::White, 12);

        // Revert button
        int32_t rev_w = 120, rev_h = 30;
        int32_t rev_x = center_x + 10;
        int32_t rev_y = center_y + 55;
        bool rev_hovered = hovered_element_ == elem_revert_button;
        sf::Color rev_bg = rev_hovered ? sf::Color(120, 60, 60) : sf::Color(90, 50, 50);
        rend.draw_rect(rev_x, rev_y, rev_w, rev_h, rev_bg, true);
        rend.draw_rect(rev_x, rev_y, rev_w, rev_h, sf::Color(160, 100, 100), false);
        rend.draw_text("Revert", rev_x + 36, rev_y + 8,
                       rev_hovered ? sf::Color(255, 200, 200) : sf::Color::White, 12);
    }

    // Helper to render dropdown list overlay
    auto render_dropdown_list = [&](float animation, int32_t dropdown_y, int32_t count,
                                    auto& options, int32_t hovered, int32_t selected,
                                    auto get_label) {
        if (animation <= 0.0f) return;

        int32_t dropdown_x = bounds_.x + 100;
        int32_t dropdown_width = 170;
        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;
        int32_t full_list_height = count * item_height;

        float eased = 1.0f - (1.0f - animation) * (1.0f - animation);
        int32_t current_height = static_cast<int32_t>(full_list_height * eased);

        if (current_height > 0) {
            rend.draw_rect(dropdown_x, list_y, dropdown_width, current_height, sf::Color(35, 35, 50), true);
            rend.push_scissor(dropdown_x, list_y, dropdown_width, current_height);

            for (int32_t i = 0; i < count; ++i) {
                int32_t item_y = list_y + i * item_height;
                if (i == hovered) {
                    rend.draw_rect(dropdown_x + 1, item_y, dropdown_width - 2, item_height, sf::Color(60, 80, 120), true);
                } else if (i == selected) {
                    rend.draw_rect(dropdown_x + 1, item_y, dropdown_width - 2, item_height, sf::Color(50, 100, 160), true);
                }
                rend.draw_text(get_label(i), dropdown_x + 8, item_y + 4, sf::Color::White, 11);
            }

            rend.pop_scissor();
            rend.draw_rect(dropdown_x, list_y, dropdown_width, current_height, sf::Color(80, 80, 100), false);
        }
    };

    // Y positions for each dropdown overlay (must match render layout above)
    // Order: display_mode, monitor, resolution, framerate
    int32_t video_start_y = bounds_.y + 40 + 25 + 28 + 40 + 25;

    // Display mode dropdown list (first)
    render_dropdown_list(display_mode_dropdown_animation_, video_start_y,
        display_mode_count_, display_mode_labels_,
        display_mode_dropdown_hovered_, selected_display_mode_,
        [](int32_t i) -> const char* { return display_mode_labels_[i]; });

    // Monitor dropdown list (second)
    render_dropdown_list(monitor_dropdown_animation_, video_start_y + 32,
        static_cast<int32_t>(monitor_options_.size()), monitor_options_,
        monitor_dropdown_hovered_, selected_monitor_,
        [&](int32_t i) -> const std::string& { return monitor_options_[i].label; });

    // Resolution dropdown list (third)
    render_dropdown_list(resolution_dropdown_animation_, video_start_y + 64,
        static_cast<int32_t>(resolution_options_.size()), resolution_options_,
        resolution_dropdown_hovered_, selected_resolution_,
        [&](int32_t i) -> const std::string& { return resolution_options_[i].label; });

    // Framerate dropdown list (fourth)
    render_dropdown_list(framerate_dropdown_animation_, video_start_y + 96,
        static_cast<int32_t>(framerate_options_.size()), framerate_options_,
        framerate_dropdown_hovered_, selected_framerate_,
        [&](int32_t i) -> const std::string& { return framerate_options_[i].label; });
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
    // When revert countdown is active, only the Keep/Revert buttons are interactive
    if (revert_countdown_active_) {
        int32_t center_x = bounds_.x + dialog_width / 2;
        int32_t center_y = bounds_.y + dialog_height / 2 - 40;
        int32_t btn_w = 120, btn_h = 30;
        int32_t btn_y = center_y + 55;

        // Keep Changes button
        int32_t keep_x = center_x - btn_w - 10;
        if (mouse_x >= keep_x && mouse_x < keep_x + btn_w &&
            mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
            return elem_keep_changes_button;
        }

        // Revert button
        int32_t rev_x = center_x + 10;
        if (mouse_x >= rev_x && mouse_x < rev_x + btn_w &&
            mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
            return elem_revert_button;
        }

        return -1;
    }

    int32_t y = bounds_.y + 65;
    int32_t x = bounds_.x + 25;
    int32_t dropdown_x = bounds_.x + 100;
    int32_t dropdown_width = 170;

    // Helper to check dropdown items
    auto check_dropdown_items = [&](bool expanded, int32_t dd_y, int32_t count, int32_t base) -> int32_t {
        if (!expanded) return -1;
        int32_t list_y = dd_y + 24;
        int32_t item_height = 22;
        int32_t list_height = count * item_height;
        if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width &&
            mouse_y >= list_y && mouse_y < list_y + list_height) {
            return base + (mouse_y - list_y) / item_height;
        }
        return -1;
    };

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
    y += 25;  // Video section header

    // Display mode dropdown (first in video section)
    if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width && mouse_y >= y && mouse_y < y + 24) {
        return elem_display_mode_dropdown;
    }
    int32_t item = check_dropdown_items(display_mode_dropdown_expanded_, y, display_mode_count_, elem_display_mode_item_base);
    if (item >= 0) return item;
    y += 32;

    // Monitor dropdown (disabled when windowed)
    bool mon_disabled = (selected_display_mode_ == 0);
    if (!mon_disabled && mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width && mouse_y >= y && mouse_y < y + 24) {
        return elem_monitor_dropdown;
    }
    if (!mon_disabled) {
        item = check_dropdown_items(monitor_dropdown_expanded_, y,
                                     static_cast<int32_t>(monitor_options_.size()), elem_monitor_item_base);
        if (item >= 0) return item;
    }
    y += 32;

    // Resolution dropdown (disabled when borderless)
    bool res_disabled = (selected_display_mode_ == 1);
    if (!res_disabled && mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width && mouse_y >= y && mouse_y < y + 24) {
        return elem_resolution_dropdown;
    }
    if (!res_disabled) {
        item = check_dropdown_items(resolution_dropdown_expanded_, y,
                                     static_cast<int32_t>(resolution_options_.size()), elem_resolution_item_base);
        if (item >= 0) return item;
    }
    y += 32;

    // Framerate dropdown
    if (mouse_x >= dropdown_x && mouse_x < dropdown_x + dropdown_width && mouse_y >= y && mouse_y < y + 24) {
        return elem_framerate_dropdown;
    }
    item = check_dropdown_items(framerate_dropdown_expanded_, y,
                                 static_cast<int32_t>(framerate_options_.size()), elem_framerate_item_base);
    if (item >= 0) return item;
    y += 32;

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
    y += 25;  // Audio section header

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

    auto close_all_dropdowns = [this]() {
        monitor_dropdown_expanded_ = false;
        display_mode_dropdown_expanded_ = false;
        resolution_dropdown_expanded_ = false;
        framerate_dropdown_expanded_ = false;
    };

    // Check title bar for dragging and close button
    if (btn == sf::Mouse::Button::Left && y >= bounds_.y && y < bounds_.y + 28) {
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

        if (draggable_ && x >= bounds_.x && x < bounds_.x + bounds_.width) {
            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    if (btn == sf::Mouse::Button::Left) {
        int32_t clicked = get_hovered_element(x, y);

        // Handle revert countdown buttons first
        if (revert_countdown_active_) {
            if (clicked == elem_keep_changes_button) {
                revert_countdown_active_ = false;
                spdlog::info("Display settings kept");
                return true;
            }
            if (clicked == elem_revert_button) {
                revert_countdown_active_ = false;
                selected_resolution_ = revert_state_.resolution_index;
                selected_display_mode_ = revert_state_.display_mode_index;
                selected_monitor_ = revert_state_.monitor_index;
                fullscreen_ = revert_state_.fullscreen;

                if (on_resolution_change_) {
                    on_resolution_change_(revert_state_.width, revert_state_.height,
                                          revert_state_.fullscreen, revert_state_.borderless,
                                          revert_state_.monitor_x, revert_state_.monitor_y);
                }
                applied_resolution_ = selected_resolution_;
                applied_display_mode_ = selected_display_mode_;
                applied_monitor_ = selected_monitor_;
                applied_fullscreen_ = fullscreen_;
                spdlog::info("Display settings reverted (user)");
                return true;
            }
            // Block all other clicks during countdown
            return true;
        }

        // Handle framerate dropdown item selection
        if (clicked >= elem_framerate_item_base) {
            int32_t item_index = clicked - elem_framerate_item_base;
            if (item_index >= 0 && item_index < static_cast<int32_t>(framerate_options_.size())) {
                selected_framerate_ = item_index;
                framerate_dropdown_expanded_ = false;
                spdlog::debug("Framerate selected: {}", framerate_options_[item_index].label);
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

        // Handle display mode dropdown item selection
        if (clicked >= elem_display_mode_item_base && clicked < elem_resolution_item_base) {
            int32_t item_index = clicked - elem_display_mode_item_base;
            if (item_index >= 0 && item_index < display_mode_count_) {
                selected_display_mode_ = item_index;
                display_mode_dropdown_expanded_ = false;
                // Close monitor dropdown if switching to windowed (monitor is disabled)
                if (item_index == 0) {
                    monitor_dropdown_expanded_ = false;
                }
                // Close resolution dropdown if switching to borderless (resolution is disabled)
                if (item_index == 1) {
                    resolution_dropdown_expanded_ = false;
                }
                // Update fullscreen_ for backward compat
                fullscreen_ = (item_index == 2);
                spdlog::debug("Display mode selected: {}", display_mode_labels_[item_index]);
            }
            return true;
        }

        // Handle monitor dropdown item selection
        if (clicked >= elem_monitor_item_base && clicked < elem_display_mode_item_base) {
            int32_t item_index = clicked - elem_monitor_item_base;
            if (item_index >= 0 && item_index < static_cast<int32_t>(monitor_options_.size())) {
                selected_monitor_ = item_index;
                monitor_dropdown_expanded_ = false;
                spdlog::debug("Monitor selected: {}", monitor_options_[item_index].label);
                rebuild_resolution_options();
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

            case elem_monitor_dropdown:
                display_mode_dropdown_expanded_ = false;
                resolution_dropdown_expanded_ = false;
                framerate_dropdown_expanded_ = false;
                monitor_dropdown_expanded_ = !monitor_dropdown_expanded_;
                return true;

            case elem_display_mode_dropdown:
                monitor_dropdown_expanded_ = false;
                resolution_dropdown_expanded_ = false;
                framerate_dropdown_expanded_ = false;
                display_mode_dropdown_expanded_ = !display_mode_dropdown_expanded_;
                return true;

            case elem_resolution_dropdown:
                monitor_dropdown_expanded_ = false;
                display_mode_dropdown_expanded_ = false;
                framerate_dropdown_expanded_ = false;
                resolution_dropdown_expanded_ = !resolution_dropdown_expanded_;
                return true;

            case elem_framerate_dropdown:
                monitor_dropdown_expanded_ = false;
                display_mode_dropdown_expanded_ = false;
                resolution_dropdown_expanded_ = false;
                framerate_dropdown_expanded_ = !framerate_dropdown_expanded_;
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

            case elem_apply_button: {
                close_all_dropdowns();
                skip_close_after_apply_ = false;

                if (on_resolution_change_) {
                    bool is_borderless = (selected_display_mode_ == 1);
                    bool is_fullscreen = (selected_display_mode_ == 2);

                    // Determine new resolution and monitor position
                    uint32_t res_w = 640, res_h = 480;
                    int32_t mon_x = 0, mon_y = 0;

                    if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size())) {
                        mon_x = monitor_options_[selected_monitor_].x;
                        mon_y = monitor_options_[selected_monitor_].y;
                    }

                    if (is_borderless) {
                        if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size())) {
                            res_w = static_cast<uint32_t>(monitor_options_[selected_monitor_].width);
                            res_h = static_cast<uint32_t>(monitor_options_[selected_monitor_].height);
                        }
                    } else if (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size())) {
                        res_w = resolution_options_[selected_resolution_].width;
                        res_h = resolution_options_[selected_resolution_].height;
                    }

                    bool changed = (selected_resolution_ != applied_resolution_ ||
                                    selected_display_mode_ != applied_display_mode_ ||
                                    selected_monitor_ != applied_monitor_);

                    if (changed) {
                        // Save current state for potential revert
                        bool old_borderless = (applied_display_mode_ == 1);
                        bool old_fullscreen = (applied_display_mode_ == 2);
                        uint32_t old_w = 640, old_h = 480;
                        int32_t old_mon_x = 0, old_mon_y = 0;

                        // Look up old resolution from applied index
                        if (old_borderless) {
                            // Was borderless - use the monitor's native res
                            // (applied_resolution_ index may not be valid for this)
                            auto& video = config::instance().video();
                            old_w = video.screen_width;
                            old_h = video.screen_height;
                        } else if (applied_resolution_ >= 0 && applied_resolution_ < static_cast<int32_t>(resolution_options_.size())) {
                            old_w = resolution_options_[applied_resolution_].width;
                            old_h = resolution_options_[applied_resolution_].height;
                        } else {
                            auto& video = config::instance().video();
                            old_w = video.screen_width;
                            old_h = video.screen_height;
                        }

                        // Look up old monitor position from config
                        auto& video = config::instance().video();
                        for (const auto& m : monitors_) {
                            if (m.index == video.monitor_index) {
                                old_mon_x = m.x;
                                old_mon_y = m.y;
                                break;
                            }
                        }

                        // Find the old monitor's option index
                        int32_t old_monitor_option_idx = 0;
                        for (size_t i = 0; i < monitor_options_.size(); ++i) {
                            if (monitor_options_[i].index == video.monitor_index) {
                                old_monitor_option_idx = static_cast<int32_t>(i);
                                break;
                            }
                        }

                        revert_state_ = {
                            old_w, old_h, old_fullscreen, old_borderless,
                            old_mon_x, old_mon_y,
                            applied_resolution_, applied_display_mode_, old_monitor_option_idx
                        };

                        // Apply the new settings
                        on_resolution_change_(res_w, res_h, is_fullscreen, is_borderless, mon_x, mon_y);
                        applied_resolution_ = selected_resolution_;
                        applied_display_mode_ = selected_display_mode_;
                        applied_monitor_ = selected_monitor_;
                        applied_fullscreen_ = is_fullscreen;

                        // Start revert countdown
                        revert_countdown_active_ = true;
                        revert_countdown_timer_ = revert_countdown_duration_;
                    }
                }

                if (on_apply_) on_apply_();
                if (!skip_close_after_apply_ && !revert_countdown_active_) {
                    close();
                }
                skip_close_after_apply_ = false;
                return true;
            }

            default:
                if (monitor_dropdown_expanded_ || display_mode_dropdown_expanded_ ||
                    resolution_dropdown_expanded_ || framerate_dropdown_expanded_) {
                    close_all_dropdowns();
                    return true;
                }
                break;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

} // namespace hb
