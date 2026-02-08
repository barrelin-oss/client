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
// Tab labels and metadata
// =============================================================================

static constexpr const char* tab_labels[] = {
    "Game", "Video", "Audio", "Social", "Keys", "Help", "System", "Debug"
};
static constexpr int32_t tab_count = static_cast<int32_t>(settings_tab::count);

// =============================================================================
// Settings Dialog - Construction
// =============================================================================

settings_dialog::settings_dialog()
    : dialog(dialog_type::options)
{
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
    set_monitors(enumerate_monitors());
}

// =============================================================================
// Resolution / Framerate / Monitor helpers (unchanged from old code)
// =============================================================================

void settings_dialog::init_resolution_options()
{
    resolution_options_ = {
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
    selected_resolution_ = 0;
}

void settings_dialog::rebuild_resolution_options()
{
    int32_t max_w = 1920;
    int32_t max_h = 1080;
    if (selected_display_mode_ == 0)
    {
        // Windowed mode: allow resolutions up to the largest monitor
        for (const auto& mon : monitor_options_)
        {
            max_w = std::max(max_w, mon.width);
            max_h = std::max(max_h, mon.height);
        }
    }
    else if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size()))
    {
        // Fullscreen/borderless: cap to the selected monitor
        max_w = monitor_options_[selected_monitor_].width;
        max_h = monitor_options_[selected_monitor_].height;
    }

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

    uint32_t prev_w = 640, prev_h = 480;
    if (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size()))
    {
        prev_w = resolution_options_[selected_resolution_].width;
        prev_h = resolution_options_[selected_resolution_].height;
    }

    resolution_options_.clear();

    bool native_included = false;
    for (const auto& r : all_resolutions)
    {
        if (static_cast<int32_t>(r.w) <= max_w && static_cast<int32_t>(r.h) <= max_h)
        {
            std::string label = std::to_string(r.w) + "x" + std::to_string(r.h) + r.suffix;
            resolution_options_.push_back({r.w, r.h, std::move(label)});
            if (static_cast<int32_t>(r.w) == max_w && static_cast<int32_t>(r.h) == max_h)
            {
                native_included = true;
            }
        }
    }

    if (!native_included && max_w > 0 && max_h > 0)
    {
        std::string label = std::to_string(max_w) + "x" + std::to_string(max_h) + " (Native)";
        resolution_options_.push_back({static_cast<uint32_t>(max_w), static_cast<uint32_t>(max_h), std::move(label)});
    }

    selected_resolution_ = 0;
    for (size_t i = 0; i < resolution_options_.size(); ++i)
    {
        if (resolution_options_[i].width == prev_w && resolution_options_[i].height == prev_h)
        {
            selected_resolution_ = static_cast<int32_t>(i);
            break;
        }
    }
    applied_resolution_ = selected_resolution_;
}

void settings_dialog::init_framerate_options()
{
    framerate_options_ = {
        {30, "30 FPS"},
        {60, "60 FPS"},
        {120, "120 FPS"},
        {144, "144 FPS"},
        {200, "200 FPS"},
        {240, "240 FPS"},
        {0, "Unlimited"}
    };
    selected_framerate_ = 1;
}

void settings_dialog::set_framerate(uint32_t fps)
{
    for (size_t i = 0; i < framerate_options_.size(); ++i)
    {
        if (framerate_options_[i].fps == fps)
        {
            selected_framerate_ = static_cast<int32_t>(i);
            return;
        }
    }
    selected_framerate_ = 1;
}

uint32_t settings_dialog::get_framerate() const
{
    if (selected_framerate_ >= 0 && selected_framerate_ < static_cast<int32_t>(framerate_options_.size()))
    {
        return framerate_options_[selected_framerate_].fps;
    }
    return 60;
}

void settings_dialog::set_resolution(uint32_t width, uint32_t height)
{
    for (size_t i = 0; i < resolution_options_.size(); ++i)
    {
        if (resolution_options_[i].width == width && resolution_options_[i].height == height)
        {
            selected_resolution_ = static_cast<int32_t>(i);
            applied_resolution_ = selected_resolution_;
            return;
        }
    }
    selected_resolution_ = 0;
    applied_resolution_ = 0;
}

void settings_dialog::get_resolution(uint32_t& width, uint32_t& height) const
{
    if (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size()))
    {
        width = resolution_options_[selected_resolution_].width;
        height = resolution_options_[selected_resolution_].height;
    }
    else
    {
        width = 640;
        height = 480;
    }
}

void settings_dialog::set_monitors(std::vector<monitor_info> monitors)
{
    monitors_ = std::move(monitors);
    monitor_options_.clear();

    for (size_t i = 0; i < monitors_.size(); ++i)
    {
        const auto& m = monitors_[i];
        monitor_option opt;
        opt.index = m.index;
        opt.x = m.x;
        opt.y = m.y;
        opt.width = m.width;
        opt.height = m.height;

        std::string label = "Monitor " + std::to_string(i + 1) + " - "
            + std::to_string(m.width) + "x" + std::to_string(m.height);
        if (m.primary)
        {
            label += " (primary)";
        }
        else
        {
            label += " (" + std::to_string(m.x) + "," + std::to_string(m.y) + ")";
        }
        opt.label = std::move(label);
        monitor_options_.push_back(std::move(opt));
    }

    if (monitor_options_.empty())
    {
        monitor_options_.push_back({0, "Monitor 1 - Unknown", 0, 0, 1920, 1080});
    }

    selected_monitor_ = 0;
    for (size_t i = 0; i < monitors_.size(); ++i)
    {
        if (monitors_[i].primary)
        {
            selected_monitor_ = static_cast<int32_t>(i);
            break;
        }
    }
    applied_monitor_ = selected_monitor_;
    rebuild_resolution_options();
}

void settings_dialog::set_monitor_index(int32_t index)
{
    for (size_t i = 0; i < monitor_options_.size(); ++i)
    {
        if (monitor_options_[i].index == index)
        {
            selected_monitor_ = static_cast<int32_t>(i);
            applied_monitor_ = selected_monitor_;
            rebuild_resolution_options();
            return;
        }
    }
    for (size_t i = 0; i < monitors_.size(); ++i)
    {
        if (monitors_[i].primary)
        {
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

void settings_dialog::set_display_mode(bool fullscreen, bool borderless)
{
    if (borderless)
    {
        selected_display_mode_ = 1;
    }
    else if (fullscreen)
    {
        selected_display_mode_ = 2;
    }
    else
    {
        selected_display_mode_ = 0;
    }
    applied_display_mode_ = selected_display_mode_;
    fullscreen_ = fullscreen;
    applied_fullscreen_ = fullscreen;
}

// =============================================================================
// Update
// =============================================================================

void settings_dialog::update(float delta_time, const input& inp)
{
    if (!visible_) return;

    dialog::update(delta_time, inp);

    // Revert countdown (video tab)
    if (revert_countdown_active_)
    {
        revert_countdown_timer_ -= delta_time;
        if (revert_countdown_timer_ <= 0.0f)
        {
            revert_countdown_active_ = false;
            selected_resolution_ = revert_state_.resolution_index;
            selected_display_mode_ = revert_state_.display_mode_index;
            selected_monitor_ = revert_state_.monitor_index;
            fullscreen_ = revert_state_.fullscreen;

            if (on_resolution_change_)
            {
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

    // Animate dropdowns (video tab only)
    if (active_tab_ == settings_tab::video)
    {
        auto animate = [&](bool expanded, float& animation) {
            if (expanded)
            {
                animation += delta_time * dropdown_animation_speed_;
                if (animation > 1.0f) animation = 1.0f;
            }
            else
            {
                animation -= delta_time * dropdown_animation_speed_;
                if (animation < 0.0f) animation = 0.0f;
            }
        };

        animate(monitor_dropdown_expanded_, monitor_dropdown_animation_);
        animate(display_mode_dropdown_expanded_, display_mode_dropdown_animation_);
        animate(resolution_dropdown_expanded_, resolution_dropdown_animation_);
        animate(framerate_dropdown_expanded_, framerate_dropdown_animation_);

        // Dropdown hover tracking
        int32_t content_y = bounds_.y + content_start_y_offset + content_padding;
        // Display mode is at content_y + section_header (25) + 0
        int32_t dd_y = content_y + section_header_height;

        auto update_dropdown_hover = [&](bool expanded, int32_t dropdown_y, int32_t count, int32_t& hovered) {
            if (!expanded) return;
            int32_t dropdown_x = bounds_.x + content_padding + 100;
            int32_t dropdown_width = dialog_width - content_padding * 2 - 100;
            int32_t item_height = 22;
            int32_t list_y = dropdown_y + 24;

            hovered = -1;
            if (inp.mouse_x() >= dropdown_x && inp.mouse_x() < dropdown_x + dropdown_width)
            {
                int32_t my = inp.mouse_y();
                if (my >= list_y)
                {
                    int32_t idx = (my - list_y) / item_height;
                    if (idx >= 0 && idx < count)
                    {
                        hovered = idx;
                    }
                }
            }
        };

        update_dropdown_hover(display_mode_dropdown_expanded_, dd_y,
                              display_mode_count_, display_mode_dropdown_hovered_);
        dd_y += dropdown_row_height;
        update_dropdown_hover(monitor_dropdown_expanded_, dd_y,
                              static_cast<int32_t>(monitor_options_.size()), monitor_dropdown_hovered_);
        dd_y += dropdown_row_height;
        update_dropdown_hover(resolution_dropdown_expanded_, dd_y,
                              static_cast<int32_t>(resolution_options_.size()), resolution_dropdown_hovered_);
        dd_y += dropdown_row_height;
        update_dropdown_hover(framerate_dropdown_expanded_, dd_y,
                              static_cast<int32_t>(framerate_options_.size()), framerate_dropdown_hovered_);
    }

    // Slider dragging
    if (dragging_slider_ && inp.is_mouse_down(sf::Mouse::Button::Left))
    {
        int32_t slider_x = bounds_.x + content_padding + 120;
        int32_t slider_width = dialog_width - content_padding * 2 - 120 - 50;

        float value = static_cast<float>(inp.mouse_x() - slider_x) / slider_width;
        value = std::clamp(value, 0.0f, 1.0f);

        if (dragging_slider_index_ == elem_master_slider)
        {
            master_volume_ = value;
            if (on_master_volume_change_) on_master_volume_change_(value);
        }
        else if (dragging_slider_index_ == elem_music_slider)
        {
            music_volume_ = value;
            if (on_music_volume_change_) on_music_volume_change_(value);
        }
        else if (dragging_slider_index_ == elem_sound_slider)
        {
            sound_volume_ = value;
            if (on_sound_volume_change_) on_sound_volume_change_(value);
        }
        else if (dragging_slider_index_ == elem_ui_scale_slider)
        {
            ui_scale_ = 0.5f + value * 2.5f;  // 0.5 to 3.0 range
            if (on_ui_scale_change_) on_ui_scale_change_(ui_scale_);
        }
    }
    else
    {
        dragging_slider_ = false;
        dragging_slider_index_ = -1;
    }

    hovered_tab_ = get_hovered_tab(inp.mouse_x(), inp.mouse_y());
    hovered_element_ = get_hovered_element(inp.mouse_x(), inp.mouse_y());
}

// =============================================================================
// Render
// =============================================================================

void settings_dialog::render(renderer& rend)
{
    if (!visible_) return;

    // Dialog background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(30, 30, 45, 245), true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(80, 80, 100), false);

    // Title bar
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, title_bar_height,
                   sf::Color(50, 50, 70), true);
    rend.draw_text(title_, bounds_.x + 10, bounds_.y + 6, sf::Color::White, 12);

    // Close button
    if (closeable_)
    {
        int32_t close_x = bounds_.x + bounds_.width - 24;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 20, 20, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 6, close_y + 3, sf::Color::White, 12);
    }

    // Tab bar
    render_tab_bar(rend);

    // Content area
    int32_t content_y = bounds_.y + content_start_y_offset + content_padding;

    switch (active_tab_)
    {
        case settings_tab::game:        render_game_tab(rend, content_y); break;
        case settings_tab::video:       render_video_tab(rend, content_y); break;
        case settings_tab::audio:       render_audio_tab(rend, content_y); break;
        case settings_tab::social:      render_social_tab(rend, content_y); break;
        case settings_tab::keybindings: render_keybindings_tab(rend, content_y); break;
        case settings_tab::help:        render_help_tab(rend, content_y); break;
        case settings_tab::system:      render_system_tab(rend, content_y); break;
        case settings_tab::debug:       render_debug_tab(rend, content_y); break;
        default: break;
    }
}

void settings_dialog::render_tab_bar(renderer& rend)
{
    int32_t tab_y = bounds_.y + title_bar_height;

    // Tab bar background
    rend.draw_rect(bounds_.x, tab_y, bounds_.width, tab_bar_height,
                   sf::Color(40, 40, 58), true);
    // Bottom border
    rend.draw_line(bounds_.x, tab_y + tab_bar_height - 1,
                   bounds_.x + bounds_.width, tab_y + tab_bar_height - 1,
                   sf::Color(80, 80, 100));

    int32_t visible_tabs = is_gm_ ? tab_count : (tab_count - 1);  // Hide debug tab for non-GMs
    int32_t tab_width = bounds_.width / visible_tabs;

    for (int32_t i = 0; i < visible_tabs; ++i)
    {
        int32_t tx = bounds_.x + i * tab_width;
        int32_t tw = (i == visible_tabs - 1) ? (bounds_.width - i * tab_width) : tab_width;

        bool active = (static_cast<int32_t>(active_tab_) == i);
        bool hovered = (hovered_tab_ == i);

        sf::Color bg;
        if (active)
        {
            bg = sf::Color(30, 30, 45);  // Match content bg
        }
        else if (hovered)
        {
            bg = sf::Color(55, 55, 75);
        }
        else
        {
            bg = sf::Color(40, 40, 58);
        }

        rend.draw_rect(tx, tab_y, tw, tab_bar_height, bg, true);

        if (active)
        {
            // Active tab indicator
            rend.draw_line(tx, tab_y + tab_bar_height - 1,
                           tx + tw, tab_y + tab_bar_height - 1,
                           sf::Color(30, 30, 45));  // Erase bottom border
            rend.draw_line(tx, tab_y, tx + tw, tab_y, sf::Color(100, 140, 200));  // Top highlight
        }

        // Tab separator
        if (i > 0)
        {
            rend.draw_line(tx, tab_y + 4, tx, tab_y + tab_bar_height - 5,
                           sf::Color(60, 60, 80));
        }

        // Tab label
        int32_t text_w = static_cast<int32_t>(strlen(tab_labels[i])) * 7;
        int32_t text_x = tx + (tw - text_w) / 2;
        sf::Color text_color = active ? sf::Color(200, 220, 255) : (hovered ? sf::Color(220, 220, 240) : sf::Color(160, 160, 180));
        rend.draw_text(tab_labels[i], text_x, tab_y + 7, text_color, 11);
    }
}

// =============================================================================
// Per-Tab Rendering
// =============================================================================

void settings_dialog::render_game_tab(renderer& rend, int32_t content_y)
{
    int32_t y = content_y;

    render_section_header(rend, y, "User Interface");
    y += section_header_height;

    render_toggle_option(rend, y, "Classic (Original)", current_style_ == ui_style::classic,
                         hovered_element_ == elem_style_classic);
    y += checkbox_row_height;

    render_toggle_option(rend, y, "Modern", current_style_ == ui_style::modern,
                         hovered_element_ == elem_style_modern);
    y += checkbox_row_height + 10;

    render_section_header(rend, y, "Gameplay");
    y += section_header_height;

    render_checkbox(rend, y, "Show Damage Numbers", show_damage_numbers_, hovered_element_ == elem_show_damage);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Show Names", show_names_, hovered_element_ == elem_show_names);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Show Guild Names", show_guild_names_, hovered_element_ == elem_show_guild_names);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Show HP Bars", show_hp_bars_, hovered_element_ == elem_show_hp_bars);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Camera Shake", camera_shake_, hovered_element_ == elem_camera_shake);
    y += checkbox_row_height;

    render_checkbox(rend, y, "Type to Chat (Legacy)", type_to_chat_, hovered_element_ == elem_type_to_chat);
    rend.draw_text("Disables WASD movement when enabled",
                   bounds_.x + content_padding + 41, y + 18, sf::Color(120, 120, 150), 9);
}

void settings_dialog::render_video_tab(renderer& rend, int32_t content_y)
{
    int32_t y = content_y;
    int32_t dd_x = bounds_.x + content_padding + 100;
    int32_t dd_width = dialog_width - content_padding * 2 - 100;

    render_section_header(rend, y, "Display");
    y += section_header_height;

    // Display mode dropdown
    std::string mode_text = (selected_display_mode_ >= 0 && selected_display_mode_ < display_mode_count_)
        ? display_mode_labels_[selected_display_mode_] : "Select...";
    render_dropdown(rend, y, "Mode", mode_text,
                    hovered_element_ == elem_display_mode_dropdown,
                    display_mode_dropdown_animation_);
    y += dropdown_row_height;

    // Monitor dropdown
    bool mon_disabled = (selected_display_mode_ == 0);
    std::string mon_text = (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size()))
        ? monitor_options_[selected_monitor_].label : "Select...";
    render_dropdown(rend, y, "Monitor", mon_text,
                    !mon_disabled && hovered_element_ == elem_monitor_dropdown,
                    monitor_dropdown_animation_);
    if (mon_disabled)
    {
        rend.draw_rect(dd_x, y, dd_width, 24, sf::Color(30, 30, 45, 150), true);
    }
    y += dropdown_row_height;

    // Resolution dropdown
    bool res_disabled = (selected_display_mode_ == 1);
    std::string res_text;
    if (res_disabled)
    {
        if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size()))
        {
            const auto& mon = monitor_options_[selected_monitor_];
            res_text = std::to_string(mon.width) + "x" + std::to_string(mon.height) + " (Native)";
        }
        else
        {
            res_text = "Native";
        }
    }
    else
    {
        res_text = (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size()))
            ? resolution_options_[selected_resolution_].label : "Select...";
    }
    render_dropdown(rend, y, "Resolution", res_text,
                    !res_disabled && hovered_element_ == elem_resolution_dropdown,
                    resolution_dropdown_animation_);
    if (res_disabled)
    {
        rend.draw_rect(dd_x, y, dd_width, 24, sf::Color(30, 30, 45, 150), true);
    }
    y += dropdown_row_height;

    // Framerate dropdown
    std::string fps_text = (selected_framerate_ >= 0 && selected_framerate_ < static_cast<int32_t>(framerate_options_.size()))
        ? framerate_options_[selected_framerate_].label : "Select...";
    render_dropdown(rend, y, "Framerate", fps_text,
                    hovered_element_ == elem_framerate_dropdown,
                    framerate_dropdown_animation_);
    y += dropdown_row_height;

    // Checkboxes
    render_checkbox(rend, y, "VSync", vsync_, hovered_element_ == elem_vsync_checkbox);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Remember Window Position", remember_position_, hovered_element_ == elem_remember_position_checkbox);
    y += checkbox_row_height + 8;

    // Apply button
    int32_t btn_width = 100;
    int32_t btn_height = 28;
    int32_t btn_x = bounds_.x + (dialog_width - btn_width) / 2;

    bool btn_hovered = hovered_element_ == elem_apply_button;
    render_button_widget(rend, btn_x, y, btn_width, btn_height, "Apply",
                         btn_hovered, sf::Color(50, 80, 50), sf::Color(70, 100, 70), sf::Color(100, 140, 100));

    // Revert countdown overlay
    if (revert_countdown_active_)
    {
        int32_t overlay_y = bounds_.y + content_start_y_offset;
        int32_t overlay_h = bounds_.height - content_start_y_offset;
        rend.draw_rect(bounds_.x + 1, overlay_y, bounds_.width - 2, overlay_h,
                       sf::Color(0, 0, 0, 180), true);

        int32_t countdown_secs = static_cast<int32_t>(revert_countdown_timer_) + 1;
        std::string countdown_text = "Keep these display settings?";
        std::string timer_text = "Reverting in " + std::to_string(countdown_secs) + "s...";

        int32_t center_x = bounds_.x + dialog_width / 2;
        int32_t center_y = bounds_.y + dialog_height / 2 - 30;

        int32_t text1_w = static_cast<int32_t>(countdown_text.size()) * 7;
        int32_t text2_w = static_cast<int32_t>(timer_text.size()) * 6;
        rend.draw_text(countdown_text, center_x - text1_w / 2, center_y, sf::Color::White, 13);
        rend.draw_text(timer_text, center_x - text2_w / 2, center_y + 22, sf::Color(255, 200, 100), 11);

        int32_t keep_w = 120, keep_h = 30;
        int32_t keep_x = center_x - keep_w - 10;
        int32_t keep_y = center_y + 55;
        render_button_widget(rend, keep_x, keep_y, keep_w, keep_h, "Keep Changes",
                             hovered_element_ == elem_keep_changes_button,
                             sf::Color(50, 90, 50), sf::Color(70, 120, 70), sf::Color(100, 160, 100));

        int32_t rev_w = 120, rev_h = 30;
        int32_t rev_x = center_x + 10;
        int32_t rev_y = center_y + 55;
        render_button_widget(rend, rev_x, rev_y, rev_w, rev_h, "Revert",
                             hovered_element_ == elem_revert_button,
                             sf::Color(90, 50, 50), sf::Color(120, 60, 60), sf::Color(160, 100, 100));
    }

    // Dropdown overlays (must render last, on top of everything)
    auto render_dropdown_list = [&](float animation, int32_t dropdown_y, int32_t count,
                                    int32_t hovered, int32_t selected,
                                    auto get_label) {
        if (animation <= 0.0f) return;

        int32_t item_height = 22;
        int32_t list_y = dropdown_y + 24;
        int32_t full_list_height = count * item_height;

        float eased = 1.0f - (1.0f - animation) * (1.0f - animation);
        int32_t current_height = static_cast<int32_t>(full_list_height * eased);

        if (current_height > 0)
        {
            rend.draw_rect(dd_x, list_y, dd_width, current_height, sf::Color(35, 35, 50), true);
            rend.push_scissor(dd_x, list_y, dd_width, current_height);

            for (int32_t i = 0; i < count; ++i)
            {
                int32_t item_y = list_y + i * item_height;
                if (i == hovered)
                {
                    rend.draw_rect(dd_x + 1, item_y, dd_width - 2, item_height, sf::Color(60, 80, 120), true);
                }
                else if (i == selected)
                {
                    rend.draw_rect(dd_x + 1, item_y, dd_width - 2, item_height, sf::Color(50, 100, 160), true);
                }
                rend.draw_text(get_label(i), dd_x + 8, item_y + 4, sf::Color::White, 11);
            }

            rend.pop_scissor();
            rend.draw_rect(dd_x, list_y, dd_width, current_height, sf::Color(80, 80, 100), false);
        }
    };

    int32_t dd_base_y = content_y + section_header_height;
    render_dropdown_list(display_mode_dropdown_animation_, dd_base_y,
        display_mode_count_, display_mode_dropdown_hovered_, selected_display_mode_,
        [](int32_t i) -> const char* { return display_mode_labels_[i]; });

    render_dropdown_list(monitor_dropdown_animation_, dd_base_y + dropdown_row_height,
        static_cast<int32_t>(monitor_options_.size()), monitor_dropdown_hovered_, selected_monitor_,
        [&](int32_t i) -> const std::string& { return monitor_options_[i].label; });

    render_dropdown_list(resolution_dropdown_animation_, dd_base_y + dropdown_row_height * 2,
        static_cast<int32_t>(resolution_options_.size()), resolution_dropdown_hovered_, selected_resolution_,
        [&](int32_t i) -> const std::string& { return resolution_options_[i].label; });

    render_dropdown_list(framerate_dropdown_animation_, dd_base_y + dropdown_row_height * 3,
        static_cast<int32_t>(framerate_options_.size()), framerate_dropdown_hovered_, selected_framerate_,
        [&](int32_t i) -> const std::string& { return framerate_options_[i].label; });
}

void settings_dialog::render_audio_tab(renderer& rend, int32_t content_y)
{
    int32_t y = content_y;

    render_section_header(rend, y, "Volume");
    y += section_header_height;

    render_slider(rend, y, "Master", master_volume_, hovered_element_ == elem_master_slider);
    y += slider_row_height;
    render_slider(rend, y, "Music", music_volume_, hovered_element_ == elem_music_slider);
    y += slider_row_height;
    render_slider(rend, y, "Sound Effects", sound_volume_, hovered_element_ == elem_sound_slider);
    y += slider_row_height + 10;

    render_section_header(rend, y, "Toggles");
    y += section_header_height;

    render_checkbox(rend, y, "Music Enabled", music_enabled_, hovered_element_ == elem_music_enabled);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Sound Effects Enabled", sfx_enabled_, hovered_element_ == elem_sfx_enabled);
}

void settings_dialog::render_social_tab(renderer& rend, int32_t content_y)
{
    int32_t y = content_y;

    render_section_header(rend, y, "Chat Options");
    y += section_header_height;

    render_checkbox(rend, y, "Show Timestamps", show_timestamps_, hovered_element_ == elem_show_timestamps);
    rend.draw_text("Prepend [HH:MM] to messages in chat history",
                   bounds_.x + content_padding + 41, y + 18, sf::Color(120, 120, 150), 9);
    y += checkbox_row_height + 12;

    render_checkbox(rend, y, "Filter Profanity", filter_profanity_, hovered_element_ == elem_filter_profanity);
    rend.draw_text("Replace profane words with asterisks",
                   bounds_.x + content_padding + 41, y + 18, sf::Color(120, 120, 150), 9);
    y += checkbox_row_height + 12;

    render_checkbox(rend, y, "Block Spam", block_spam_, hovered_element_ == elem_block_spam);
    rend.draw_text("Suppress rapid repeated messages from the same sender",
                   bounds_.x + content_padding + 41, y + 18, sf::Color(120, 120, 150), 9);
}

void settings_dialog::render_keybindings_tab(renderer& rend, int32_t content_y)
{
    int32_t center_x = bounds_.x + dialog_width / 2;
    int32_t y = content_y + 80;

    std::string text = "Keybinding configuration";
    int32_t tw = static_cast<int32_t>(text.size()) * 7;
    rend.draw_text(text, center_x - tw / 2, y, sf::Color(160, 160, 180), 12);

    std::string text2 = "Coming soon...";
    int32_t tw2 = static_cast<int32_t>(text2.size()) * 6;
    rend.draw_text(text2, center_x - tw2 / 2, y + 24, sf::Color(120, 120, 150), 11);
}

void settings_dialog::render_help_tab(renderer& rend, int32_t content_y)
{
    int32_t y = content_y;

    render_section_header(rend, y, "Community");
    y += section_header_height;

    // Forum link
    rend.draw_text("Forum", bounds_.x + content_padding + 10, y + 3, sf::Color(200, 200, 220), 11);
    bool forum_hovered = hovered_element_ == elem_forum_link;
    sf::Color forum_color = forum_hovered ? sf::Color(150, 200, 255) : sf::Color(100, 160, 220);
    rend.draw_text("https://forum.helbreathx.net", bounds_.x + content_padding + 70, y + 3, forum_color, 11);
    if (forum_hovered)
    {
        int32_t link_x = bounds_.x + content_padding + 70;
        int32_t link_w = static_cast<int32_t>(strlen("https://forum.helbreathx.net")) * 7;
        rend.draw_line(link_x, y + 16, link_x + link_w, y + 16, forum_color);
    }
    y += checkbox_row_height + 4;

    // Discord link
    rend.draw_text("Discord", bounds_.x + content_padding + 10, y + 3, sf::Color(200, 200, 220), 11);
    bool discord_hovered = hovered_element_ == elem_discord_link;
    sf::Color discord_color = discord_hovered ? sf::Color(150, 200, 255) : sf::Color(100, 160, 220);
    rend.draw_text("https://discord.gg/helbreath", bounds_.x + content_padding + 70, y + 3, discord_color, 11);
    if (discord_hovered)
    {
        int32_t link_x = bounds_.x + content_padding + 70;
        int32_t link_w = static_cast<int32_t>(strlen("https://discord.gg/helbreath")) * 7;
        rend.draw_line(link_x, y + 16, link_x + link_w, y + 16, discord_color);
    }
    y += checkbox_row_height + 20;

    render_section_header(rend, y, "In-Game Help");
    y += section_header_height;

    int32_t btn_x = bounds_.x + content_padding + 10;
    render_button_widget(rend, btn_x, y, 120, 28, "Open Help",
                         hovered_element_ == elem_open_help,
                         sf::Color(50, 70, 90), sf::Color(70, 90, 120), sf::Color(100, 130, 160));
}

void settings_dialog::render_system_tab(renderer& rend, int32_t content_y)
{
    int32_t center_x = bounds_.x + dialog_width / 2;
    int32_t y = content_y + 60;

    int32_t btn_w = 160;
    int32_t btn_h = 36;
    int32_t btn_x = center_x - btn_w / 2;

    render_button_widget(rend, btn_x, y, btn_w, btn_h, "Logout",
                         hovered_element_ == elem_logout_button,
                         sf::Color(60, 60, 80), sf::Color(80, 80, 110), sf::Color(100, 100, 140));
    y += btn_h + 20;

    render_button_widget(rend, btn_x, y, btn_w, btn_h, "Exit Game",
                         hovered_element_ == elem_exit_button,
                         sf::Color(90, 50, 50), sf::Color(120, 60, 60), sf::Color(160, 90, 90));
}

void settings_dialog::render_debug_tab(renderer& rend, int32_t content_y)
{
    int32_t y = content_y;

    render_section_header(rend, y, "Debug Options");
    y += section_header_height;

    render_checkbox(rend, y, "Show Debug Stats", show_debug_stats_, hovered_element_ == elem_debug_stats);
    y += checkbox_row_height;
    render_checkbox(rend, y, "Show FPS Counter", show_fps_, hovered_element_ == elem_show_fps_cb);
}

// =============================================================================
// Shared Rendering Primitives
// =============================================================================

void settings_dialog::render_section_header(renderer& rend, int32_t y, const char* text)
{
    int32_t x = bounds_.x + content_padding;
    rend.draw_text(text, x, y, sf::Color(180, 180, 220), 11);
    int32_t text_width = static_cast<int32_t>(strlen(text)) * 7;
    rend.draw_line(x, y + 14, x + text_width + 10, y + 14, sf::Color(80, 80, 120));
}

void settings_dialog::render_toggle_option(renderer& rend, int32_t y, const char* label, bool selected, bool hovered)
{
    int32_t x = bounds_.x + content_padding + 10;
    int32_t radio_size = 14;

    sf::Color circle_bg = hovered ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);
    rend.draw_rect(x, y + 3, radio_size, radio_size, circle_bg, true);
    rend.draw_rect(x, y + 3, radio_size, radio_size, sf::Color(100, 100, 130), false);

    if (selected)
    {
        rend.draw_rect(x + 3, y + 6, radio_size - 6, radio_size - 6,
                       sf::Color(100, 200, 100), true);
    }

    sf::Color text_color = hovered ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220);
    rend.draw_text(label, x + radio_size + 10, y + 2, text_color, 11);
}

void settings_dialog::render_slider(renderer& rend, int32_t y, const char* label, float value, bool hovered)
{
    int32_t label_x = bounds_.x + content_padding + 10;
    int32_t slider_x = bounds_.x + content_padding + 120;
    int32_t slider_width = dialog_width - content_padding * 2 - 120 - 50;
    int32_t slider_height = 16;

    rend.draw_text(label, label_x, y + 2, sf::Color(200, 200, 220), 11);

    sf::Color track_bg = hovered ? sf::Color(50, 50, 70) : sf::Color(35, 35, 50);
    rend.draw_rect(slider_x, y, slider_width, slider_height, track_bg, true);
    rend.draw_rect(slider_x, y, slider_width, slider_height, sf::Color(80, 80, 100), false);

    int32_t fill_width = static_cast<int32_t>(slider_width * value);
    if (fill_width > 0)
    {
        sf::Color fill_color = hovered ? sf::Color(80, 120, 180) : sf::Color(60, 100, 160);
        rend.draw_rect(slider_x, y, fill_width, slider_height, fill_color, true);
    }

    int32_t handle_x = slider_x + fill_width - 4;
    handle_x = std::clamp(handle_x, slider_x, slider_x + slider_width - 8);
    rend.draw_rect(handle_x, y - 2, 8, slider_height + 4, sf::Color(200, 200, 220), true);

    int32_t percent = static_cast<int32_t>(value * 100);
    std::string percent_str = std::to_string(percent) + "%";
    rend.draw_text(percent_str, slider_x + slider_width + 10, y + 2, sf::Color(150, 150, 180), 10);
}

void settings_dialog::render_dropdown(renderer& rend, int32_t y, const char* label, const std::string& value, bool hovered, float animation)
{
    int32_t label_x = bounds_.x + content_padding + 10;
    int32_t dropdown_x = bounds_.x + content_padding + 100;
    int32_t dropdown_width = dialog_width - content_padding * 2 - 100;
    int32_t dropdown_height = 24;

    rend.draw_text(label, label_x, y + 5, sf::Color(200, 200, 220), 11);

    sf::Color header_bg = (hovered || animation > 0.0f) ? sf::Color(60, 60, 80) : sf::Color(50, 50, 70);
    rend.draw_rect(dropdown_x, y, dropdown_width, dropdown_height, header_bg, true);
    rend.draw_rect(dropdown_x, y, dropdown_width, dropdown_height, sf::Color(80, 80, 100), false);

    rend.draw_text(value, dropdown_x + 8, y + 5, sf::Color::White, 11);

    int32_t arrow_x = dropdown_x + dropdown_width - 20;
    int32_t arrow_y = y + dropdown_height / 2;
    float arrow_offset = 3.0f * (1.0f - 2.0f * animation);
    int32_t arrow_top = arrow_y + static_cast<int32_t>(arrow_offset);
    int32_t arrow_bottom = arrow_y - static_cast<int32_t>(arrow_offset);
    rend.draw_line(arrow_x, arrow_top, arrow_x + 6, arrow_bottom, sf::Color(180, 180, 200));
    rend.draw_line(arrow_x + 6, arrow_bottom, arrow_x + 12, arrow_top, sf::Color(180, 180, 200));
}

void settings_dialog::render_checkbox(renderer& rend, int32_t y, const char* label, bool checked, bool hovered)
{
    int32_t x = bounds_.x + content_padding + 10;
    int32_t checkbox_size = 16;

    sf::Color checkbox_bg = hovered ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);
    rend.draw_rect(x, y + 2, checkbox_size, checkbox_size, checkbox_bg, true);
    rend.draw_rect(x, y + 2, checkbox_size, checkbox_size, sf::Color(100, 100, 130), false);

    if (checked)
    {
        rend.draw_rect(x + 3, y + 5, checkbox_size - 6, checkbox_size - 6,
                       sf::Color(100, 200, 100), true);
    }

    sf::Color text_color = hovered ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220);
    rend.draw_text(label, x + checkbox_size + 10, y + 3, text_color, 11);
}

void settings_dialog::render_button_widget(renderer& rend, int32_t x, int32_t y, int32_t w, int32_t h,
                                           const char* text, bool hovered,
                                           sf::Color normal, sf::Color hover, sf::Color border)
{
    sf::Color bg = hovered ? hover : normal;
    rend.draw_rect(x, y, w, h, bg, true);
    rend.draw_rect(x, y, w, h, border, false);

    int32_t text_w = static_cast<int32_t>(strlen(text)) * 7;
    int32_t text_x = x + (w - text_w) / 2;
    int32_t text_y = y + (h - 12) / 2;

    rend.draw_text(text, text_x + 1, text_y + 1, sf::Color(0, 0, 0), 12);
    rend.draw_text(text, text_x, text_y, hovered ? sf::Color(255, 255, 200) : sf::Color::White, 12);
}

// =============================================================================
// Hit Testing
// =============================================================================

int32_t settings_dialog::get_hovered_tab(int32_t mouse_x, int32_t mouse_y) const
{
    int32_t tab_y = bounds_.y + title_bar_height;
    if (mouse_y < tab_y || mouse_y >= tab_y + tab_bar_height) return -1;
    if (mouse_x < bounds_.x || mouse_x >= bounds_.x + bounds_.width) return -1;

    int32_t visible_tabs = is_gm_ ? tab_count : (tab_count - 1);
    int32_t tab_width = bounds_.width / visible_tabs;

    int32_t index = (mouse_x - bounds_.x) / tab_width;
    if (index >= 0 && index < visible_tabs) return index;
    return -1;
}

int32_t settings_dialog::get_hovered_element(int32_t mx, int32_t my) const
{
    int32_t content_y = bounds_.y + content_start_y_offset + content_padding;

    // Check revert countdown first
    if (revert_countdown_active_ && active_tab_ == settings_tab::video)
    {
        int32_t center_x = bounds_.x + dialog_width / 2;
        int32_t center_y = bounds_.y + dialog_height / 2 - 30;
        int32_t btn_w = 120, btn_h = 30;
        int32_t btn_y = center_y + 55;

        int32_t keep_x = center_x - btn_w - 10;
        if (mx >= keep_x && mx < keep_x + btn_w && my >= btn_y && my < btn_y + btn_h)
            return elem_keep_changes_button;

        int32_t rev_x = center_x + 10;
        if (mx >= rev_x && mx < rev_x + btn_w && my >= btn_y && my < btn_y + btn_h)
            return elem_revert_button;

        return -1;
    }

    switch (active_tab_)
    {
        case settings_tab::game:    return get_hovered_element_game(mx, my, content_y);
        case settings_tab::video:   return get_hovered_element_video(mx, my, content_y);
        case settings_tab::audio:   return get_hovered_element_audio(mx, my, content_y);
        case settings_tab::social:  return get_hovered_element_social(mx, my, content_y);
        case settings_tab::help:    return get_hovered_element_help(mx, my, content_y);
        case settings_tab::system:  return get_hovered_element_system(mx, my, content_y);
        case settings_tab::debug:   return get_hovered_element_debug(mx, my, content_y);
        default: return -1;
    }
}

int32_t settings_dialog::get_hovered_element_game(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t x = bounds_.x + content_padding;
    int32_t w = dialog_width - content_padding * 2;
    int32_t y = content_y + section_header_height;

    // Classic toggle
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_style_classic;
    y += checkbox_row_height;
    // Modern toggle
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_style_modern;
    y += checkbox_row_height + 10 + section_header_height;

    // Gameplay checkboxes
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_show_damage;
    y += checkbox_row_height;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_show_names;
    y += checkbox_row_height;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_show_guild_names;
    y += checkbox_row_height;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_show_hp_bars;
    y += checkbox_row_height;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_camera_shake;
    y += checkbox_row_height;

    // Type to chat
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_type_to_chat;

    return -1;
}

int32_t settings_dialog::get_hovered_element_video(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t dd_x = bounds_.x + content_padding + 100;
    int32_t dd_w = dialog_width - content_padding * 2 - 100;
    int32_t x = bounds_.x + content_padding;
    int32_t w = dialog_width - content_padding * 2;

    int32_t y = content_y + section_header_height;

    // Dropdown item helper
    auto check_dropdown_items = [&](bool expanded, int32_t dd_y, int32_t count, int32_t base) -> int32_t {
        if (!expanded) return -1;
        int32_t list_y = dd_y + 24;
        int32_t item_height = 22;
        int32_t list_height = count * item_height;
        if (mx >= dd_x && mx < dd_x + dd_w && my >= list_y && my < list_y + list_height)
        {
            return base + (my - list_y) / item_height;
        }
        return -1;
    };

    // Display mode
    if (mx >= dd_x && mx < dd_x + dd_w && my >= y && my < y + 24) return elem_display_mode_dropdown;
    int32_t item = check_dropdown_items(display_mode_dropdown_expanded_, y, display_mode_count_, elem_display_mode_item_base);
    if (item >= 0) return item;
    y += dropdown_row_height;

    // Monitor
    bool mon_disabled = (selected_display_mode_ == 0);
    if (!mon_disabled && mx >= dd_x && mx < dd_x + dd_w && my >= y && my < y + 24) return elem_monitor_dropdown;
    if (!mon_disabled)
    {
        item = check_dropdown_items(monitor_dropdown_expanded_, y,
                                     static_cast<int32_t>(monitor_options_.size()), elem_monitor_item_base);
        if (item >= 0) return item;
    }
    y += dropdown_row_height;

    // Resolution
    bool res_disabled = (selected_display_mode_ == 1);
    if (!res_disabled && mx >= dd_x && mx < dd_x + dd_w && my >= y && my < y + 24) return elem_resolution_dropdown;
    if (!res_disabled)
    {
        item = check_dropdown_items(resolution_dropdown_expanded_, y,
                                     static_cast<int32_t>(resolution_options_.size()), elem_resolution_item_base);
        if (item >= 0) return item;
    }
    y += dropdown_row_height;

    // Framerate
    if (mx >= dd_x && mx < dd_x + dd_w && my >= y && my < y + 24) return elem_framerate_dropdown;
    item = check_dropdown_items(framerate_dropdown_expanded_, y,
                                 static_cast<int32_t>(framerate_options_.size()), elem_framerate_item_base);
    if (item >= 0) return item;
    y += dropdown_row_height;

    // VSync
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_vsync_checkbox;
    y += checkbox_row_height;

    // Remember position
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_remember_position_checkbox;
    y += checkbox_row_height + 8;

    // Apply button
    int32_t btn_w_apply = 100;
    int32_t btn_h_apply = 28;
    int32_t btn_x = bounds_.x + (dialog_width - btn_w_apply) / 2;
    if (mx >= btn_x && mx < btn_x + btn_w_apply && my >= y && my < y + btn_h_apply) return elem_apply_button;

    return -1;
}

int32_t settings_dialog::get_hovered_element_audio(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t slider_x = bounds_.x + content_padding + 120;
    int32_t slider_w = dialog_width - content_padding * 2 - 120 - 50;
    int32_t x = bounds_.x + content_padding;
    int32_t w = dialog_width - content_padding * 2;

    int32_t y = content_y + section_header_height;

    if (mx >= slider_x && mx < slider_x + slider_w && my >= y && my < y + 20) return elem_master_slider;
    y += slider_row_height;
    if (mx >= slider_x && mx < slider_x + slider_w && my >= y && my < y + 20) return elem_music_slider;
    y += slider_row_height;
    if (mx >= slider_x && mx < slider_x + slider_w && my >= y && my < y + 20) return elem_sound_slider;
    y += slider_row_height + 10 + section_header_height;

    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_music_enabled;
    y += checkbox_row_height;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_sfx_enabled;

    return -1;
}

int32_t settings_dialog::get_hovered_element_social(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t x = bounds_.x + content_padding;
    int32_t w = dialog_width - content_padding * 2;
    int32_t y = content_y + section_header_height;

    // Checkbox rows with description text (checkbox_row_height + 12 spacing)
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_show_timestamps;
    y += checkbox_row_height + 12;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_filter_profanity;
    y += checkbox_row_height + 12;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_block_spam;

    return -1;
}

int32_t settings_dialog::get_hovered_element_help(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t y = content_y + section_header_height;

    // Forum link area
    int32_t link_x = bounds_.x + content_padding + 70;
    int32_t link_w = static_cast<int32_t>(strlen("https://forum.helbreathx.net")) * 7;
    if (mx >= link_x && mx < link_x + link_w && my >= y && my < y + checkbox_row_height) return elem_forum_link;
    y += checkbox_row_height + 4;

    // Discord link area
    link_w = static_cast<int32_t>(strlen("https://discord.gg/helbreath")) * 7;
    if (mx >= link_x && mx < link_x + link_w && my >= y && my < y + checkbox_row_height) return elem_discord_link;
    y += checkbox_row_height + 20 + section_header_height;

    // Open Help button
    int32_t btn_x = bounds_.x + content_padding + 10;
    if (mx >= btn_x && mx < btn_x + 120 && my >= y && my < y + 28) return elem_open_help;

    return -1;
}

int32_t settings_dialog::get_hovered_element_system(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t center_x = bounds_.x + dialog_width / 2;
    int32_t y = content_y + 60;
    int32_t btn_w = 160, btn_h = 36;
    int32_t btn_x = center_x - btn_w / 2;

    if (mx >= btn_x && mx < btn_x + btn_w && my >= y && my < y + btn_h) return elem_logout_button;
    y += btn_h + 20;
    if (mx >= btn_x && mx < btn_x + btn_w && my >= y && my < y + btn_h) return elem_exit_button;

    return -1;
}

int32_t settings_dialog::get_hovered_element_debug(int32_t mx, int32_t my, int32_t content_y) const
{
    int32_t x = bounds_.x + content_padding;
    int32_t w = dialog_width - content_padding * 2;
    int32_t y = content_y + section_header_height;

    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_debug_stats;
    y += checkbox_row_height;
    if (mx >= x && mx < x + w && my >= y && my < y + checkbox_row_height) return elem_show_fps_cb;

    return -1;
}

// =============================================================================
// Input Handling
// =============================================================================

bool settings_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_) return false;

    auto close_all_dropdowns = [this]() {
        monitor_dropdown_expanded_ = false;
        display_mode_dropdown_expanded_ = false;
        resolution_dropdown_expanded_ = false;
        framerate_dropdown_expanded_ = false;
    };

    // Title bar: close button and dragging
    if (btn == sf::Mouse::Button::Left && y >= bounds_.y && y < bounds_.y + title_bar_height)
    {
        if (closeable_)
        {
            int32_t close_x = bounds_.x + bounds_.width - 24;
            int32_t close_y = bounds_.y + 4;
            if (x >= close_x && x < close_x + 20 && y >= close_y && y < close_y + 20)
            {
                close_all_dropdowns();
                close();
                if (on_close_cb_) on_close_cb_();
                return true;
            }
        }

        if (draggable_ && x >= bounds_.x && x < bounds_.x + bounds_.width)
        {
            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    // Tab bar click
    if (btn == sf::Mouse::Button::Left)
    {
        int32_t tab = get_hovered_tab(x, y);
        if (tab >= 0)
        {
            close_all_dropdowns();
            active_tab_ = static_cast<settings_tab>(tab);
            return true;
        }
    }

    // Content area click
    if (btn == sf::Mouse::Button::Left)
    {
        int32_t clicked = get_hovered_element(x, y);

        // Revert countdown buttons
        if (revert_countdown_active_ && active_tab_ == settings_tab::video)
        {
            if (clicked == elem_keep_changes_button)
            {
                revert_countdown_active_ = false;
                spdlog::info("Display settings kept");
                return true;
            }
            if (clicked == elem_revert_button)
            {
                revert_countdown_active_ = false;
                selected_resolution_ = revert_state_.resolution_index;
                selected_display_mode_ = revert_state_.display_mode_index;
                selected_monitor_ = revert_state_.monitor_index;
                fullscreen_ = revert_state_.fullscreen;

                if (on_resolution_change_)
                {
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
            return true;  // Block all other clicks during countdown
        }

        // Dispatch to per-tab handlers
        switch (active_tab_)
        {
            case settings_tab::game:
                if (handle_game_tab_click(clicked)) return true;
                break;
            case settings_tab::video:
                if (handle_video_tab_click(clicked)) return true;
                break;
            case settings_tab::audio:
                if (handle_audio_tab_click(clicked, x)) return true;
                break;
            case settings_tab::social:
                if (handle_social_tab_click(clicked)) return true;
                break;
            case settings_tab::help:
                if (handle_help_tab_click(clicked)) return true;
                break;
            case settings_tab::system:
                if (handle_system_tab_click(clicked)) return true;
                break;
            case settings_tab::debug:
                if (handle_debug_tab_click(clicked)) return true;
                break;
            default: break;
        }

        // Close dropdowns on any unhandled click
        if (monitor_dropdown_expanded_ || display_mode_dropdown_expanded_ ||
            resolution_dropdown_expanded_ || framerate_dropdown_expanded_)
        {
            close_all_dropdowns();
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool settings_dialog::handle_game_tab_click(int32_t elem)
{
    switch (elem)
    {
        case elem_style_classic:
            current_style_ = ui_style::classic;
            if (on_style_change_) on_style_change_(current_style_);
            return true;
        case elem_style_modern:
            current_style_ = ui_style::modern;
            if (on_style_change_) on_style_change_(current_style_);
            return true;
        case elem_show_damage:
            show_damage_numbers_ = !show_damage_numbers_;
            if (on_show_damage_numbers_change_) on_show_damage_numbers_change_(show_damage_numbers_);
            return true;
        case elem_show_names:
            show_names_ = !show_names_;
            if (on_show_names_change_) on_show_names_change_(show_names_);
            return true;
        case elem_show_guild_names:
            show_guild_names_ = !show_guild_names_;
            if (on_show_guild_names_change_) on_show_guild_names_change_(show_guild_names_);
            return true;
        case elem_show_hp_bars:
            show_hp_bars_ = !show_hp_bars_;
            if (on_show_hp_bars_change_) on_show_hp_bars_change_(show_hp_bars_);
            return true;
        case elem_camera_shake:
            camera_shake_ = !camera_shake_;
            if (on_camera_shake_change_) on_camera_shake_change_(camera_shake_);
            return true;
        case elem_type_to_chat:
            type_to_chat_ = !type_to_chat_;
            if (on_type_to_chat_change_) on_type_to_chat_change_(type_to_chat_);
            return true;
        default:
            return false;
    }
}

bool settings_dialog::handle_video_tab_click(int32_t elem)
{
    auto close_all_dropdowns = [this]() {
        monitor_dropdown_expanded_ = false;
        display_mode_dropdown_expanded_ = false;
        resolution_dropdown_expanded_ = false;
        framerate_dropdown_expanded_ = false;
    };

    // Framerate dropdown items
    if (elem >= elem_framerate_item_base)
    {
        int32_t idx = elem - elem_framerate_item_base;
        if (idx >= 0 && idx < static_cast<int32_t>(framerate_options_.size()))
        {
            selected_framerate_ = idx;
            framerate_dropdown_expanded_ = false;
            if (on_framerate_change_) on_framerate_change_(framerate_options_[idx].fps);
        }
        return true;
    }

    // Resolution dropdown items
    if (elem >= elem_resolution_item_base)
    {
        int32_t idx = elem - elem_resolution_item_base;
        if (idx >= 0 && idx < static_cast<int32_t>(resolution_options_.size()))
        {
            selected_resolution_ = idx;
            resolution_dropdown_expanded_ = false;
        }
        return true;
    }

    // Display mode dropdown items
    if (elem >= elem_display_mode_item_base && elem < elem_resolution_item_base)
    {
        int32_t idx = elem - elem_display_mode_item_base;
        if (idx >= 0 && idx < display_mode_count_)
        {
            selected_display_mode_ = idx;
            display_mode_dropdown_expanded_ = false;
            if (idx == 0) monitor_dropdown_expanded_ = false;
            if (idx == 1) resolution_dropdown_expanded_ = false;
            fullscreen_ = (idx == 2);
        }
        return true;
    }

    // Monitor dropdown items
    if (elem >= elem_monitor_item_base && elem < elem_display_mode_item_base)
    {
        int32_t idx = elem - elem_monitor_item_base;
        if (idx >= 0 && idx < static_cast<int32_t>(monitor_options_.size()))
        {
            selected_monitor_ = idx;
            monitor_dropdown_expanded_ = false;
            rebuild_resolution_options();
        }
        return true;
    }

    switch (elem)
    {
        case elem_display_mode_dropdown:
            monitor_dropdown_expanded_ = false;
            resolution_dropdown_expanded_ = false;
            framerate_dropdown_expanded_ = false;
            display_mode_dropdown_expanded_ = !display_mode_dropdown_expanded_;
            return true;
        case elem_monitor_dropdown:
            display_mode_dropdown_expanded_ = false;
            resolution_dropdown_expanded_ = false;
            framerate_dropdown_expanded_ = false;
            monitor_dropdown_expanded_ = !monitor_dropdown_expanded_;
            return true;
        case elem_resolution_dropdown:
            display_mode_dropdown_expanded_ = false;
            monitor_dropdown_expanded_ = false;
            framerate_dropdown_expanded_ = false;
            resolution_dropdown_expanded_ = !resolution_dropdown_expanded_;
            return true;
        case elem_framerate_dropdown:
            display_mode_dropdown_expanded_ = false;
            monitor_dropdown_expanded_ = false;
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
        case elem_apply_button:
        {
            close_all_dropdowns();
            skip_close_after_apply_ = false;

            if (on_resolution_change_)
            {
                bool is_borderless = (selected_display_mode_ == 1);
                bool is_fullscreen = (selected_display_mode_ == 2);

                uint32_t res_w = 640, res_h = 480;
                int32_t mon_x = 0, mon_y = 0;

                if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size()))
                {
                    mon_x = monitor_options_[selected_monitor_].x;
                    mon_y = monitor_options_[selected_monitor_].y;
                }

                if (is_borderless)
                {
                    if (selected_monitor_ >= 0 && selected_monitor_ < static_cast<int32_t>(monitor_options_.size()))
                    {
                        res_w = static_cast<uint32_t>(monitor_options_[selected_monitor_].width);
                        res_h = static_cast<uint32_t>(monitor_options_[selected_monitor_].height);
                    }
                }
                else if (selected_resolution_ >= 0 && selected_resolution_ < static_cast<int32_t>(resolution_options_.size()))
                {
                    res_w = resolution_options_[selected_resolution_].width;
                    res_h = resolution_options_[selected_resolution_].height;
                }

                bool changed = (selected_resolution_ != applied_resolution_ ||
                                selected_display_mode_ != applied_display_mode_ ||
                                selected_monitor_ != applied_monitor_);

                if (changed)
                {
                    bool old_borderless = (applied_display_mode_ == 1);
                    bool old_fullscreen = (applied_display_mode_ == 2);
                    uint32_t old_w = 640, old_h = 480;
                    int32_t old_mon_x = 0, old_mon_y = 0;

                    if (old_borderless)
                    {
                        auto& video = config::instance().video();
                        old_w = video.screen_width;
                        old_h = video.screen_height;
                    }
                    else if (applied_resolution_ >= 0 && applied_resolution_ < static_cast<int32_t>(resolution_options_.size()))
                    {
                        old_w = resolution_options_[applied_resolution_].width;
                        old_h = resolution_options_[applied_resolution_].height;
                    }
                    else
                    {
                        auto& video = config::instance().video();
                        old_w = video.screen_width;
                        old_h = video.screen_height;
                    }

                    auto& video = config::instance().video();
                    for (const auto& m : monitors_)
                    {
                        if (m.index == video.monitor_index)
                        {
                            old_mon_x = m.x;
                            old_mon_y = m.y;
                            break;
                        }
                    }

                    int32_t old_monitor_option_idx = 0;
                    for (size_t i = 0; i < monitor_options_.size(); ++i)
                    {
                        if (monitor_options_[i].index == video.monitor_index)
                        {
                            old_monitor_option_idx = static_cast<int32_t>(i);
                            break;
                        }
                    }

                    revert_state_ = {
                        old_w, old_h, old_fullscreen, old_borderless,
                        old_mon_x, old_mon_y,
                        applied_resolution_, applied_display_mode_, old_monitor_option_idx
                    };

                    on_resolution_change_(res_w, res_h, is_fullscreen, is_borderless, mon_x, mon_y);
                    applied_resolution_ = selected_resolution_;
                    applied_display_mode_ = selected_display_mode_;
                    applied_monitor_ = selected_monitor_;
                    applied_fullscreen_ = is_fullscreen;

                    revert_countdown_active_ = true;
                    revert_countdown_timer_ = revert_countdown_duration_;
                }
            }

            if (on_apply_) on_apply_();
            if (!skip_close_after_apply_ && !revert_countdown_active_)
            {
                // Don't close - stay on the video tab
            }
            skip_close_after_apply_ = false;
            return true;
        }
        default:
            return false;
    }
}

bool settings_dialog::handle_audio_tab_click(int32_t elem, int32_t /*x*/)
{
    switch (elem)
    {
        case elem_master_slider:
            dragging_slider_ = true;
            dragging_slider_index_ = elem_master_slider;
            return true;
        case elem_music_slider:
            dragging_slider_ = true;
            dragging_slider_index_ = elem_music_slider;
            return true;
        case elem_sound_slider:
            dragging_slider_ = true;
            dragging_slider_index_ = elem_sound_slider;
            return true;
        case elem_music_enabled:
            music_enabled_ = !music_enabled_;
            if (on_music_enabled_change_) on_music_enabled_change_(music_enabled_);
            return true;
        case elem_sfx_enabled:
            sfx_enabled_ = !sfx_enabled_;
            if (on_sfx_enabled_change_) on_sfx_enabled_change_(sfx_enabled_);
            return true;
        default:
            return false;
    }
}

bool settings_dialog::handle_social_tab_click(int32_t elem)
{
    switch (elem)
    {
        case elem_show_timestamps:
            show_timestamps_ = !show_timestamps_;
            if (on_show_timestamps_change_) on_show_timestamps_change_(show_timestamps_);
            return true;
        case elem_filter_profanity:
            filter_profanity_ = !filter_profanity_;
            if (on_filter_profanity_change_) on_filter_profanity_change_(filter_profanity_);
            return true;
        case elem_block_spam:
            block_spam_ = !block_spam_;
            if (on_block_spam_change_) on_block_spam_change_(block_spam_);
            return true;
        default:
            return false;
    }
}

bool settings_dialog::handle_help_tab_click(int32_t elem)
{
    switch (elem)
    {
        case elem_forum_link:
            spdlog::info("Forum link clicked: https://forum.helbreathx.net");
            return true;
        case elem_discord_link:
            spdlog::info("Discord link clicked: https://discord.gg/helbreath");
            return true;
        case elem_open_help:
            if (on_help_) on_help_();
            return true;
        default:
            return false;
    }
}

bool settings_dialog::handle_system_tab_click(int32_t elem)
{
    switch (elem)
    {
        case elem_logout_button:
            close();
            if (on_logout_) on_logout_();
            return true;
        case elem_exit_button:
            if (on_exit_) on_exit_();
            return true;
        default:
            return false;
    }
}

bool settings_dialog::handle_debug_tab_click(int32_t elem)
{
    switch (elem)
    {
        case elem_debug_stats:
            show_debug_stats_ = !show_debug_stats_;
            if (on_show_debug_stats_change_) on_show_debug_stats_change_(show_debug_stats_);
            return true;
        case elem_show_fps_cb:
            show_fps_ = !show_fps_;
            if (on_show_fps_change_) on_show_fps_change_(show_fps_);
            return true;
        default:
            return false;
    }
}

} // namespace hb
