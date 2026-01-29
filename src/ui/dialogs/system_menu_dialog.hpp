#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <string>

namespace hb {

// System menu dialog - accessed from the rightmost icon panel button
// Provides access to game settings, help, logout, and exit
class system_menu_dialog : public dialog {
public:
    system_menu_dialog();
    ~system_menu_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    // Callbacks
    using callback = std::function<void()>;

    void set_on_settings(callback cb) { on_settings_ = std::move(cb); }
    void set_on_help(callback cb) { on_help_ = std::move(cb); }
    void set_on_logout(callback cb) { on_logout_ = std::move(cb); }
    void set_on_exit(callback cb) { on_exit_ = std::move(cb); }

private:
    void render_button(renderer& rend, int32_t index, const char* text, bool hovered);
    int32_t get_hovered_button(int32_t mouse_x, int32_t mouse_y) const;

    callback on_settings_;
    callback on_help_;
    callback on_logout_;
    callback on_exit_;

    int32_t hovered_button_ = -1;

    // Layout
    static constexpr int32_t dialog_width = 160;
    static constexpr int32_t dialog_height = 200;
    static constexpr int32_t button_width = 120;
    static constexpr int32_t button_height = 32;
    static constexpr int32_t button_spacing = 8;
    static constexpr int32_t button_start_y = 40;
    static constexpr int32_t button_count = 4;
};

// Settings dialog - new dialog for game configuration
// Includes UI style toggle and other options
class settings_dialog : public dialog {
public:
    settings_dialog();
    ~settings_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    // Current settings
    void set_ui_style(ui_style style) { current_style_ = style; }
    ui_style get_ui_style() const { return current_style_; }

    void set_music_volume(float volume) { music_volume_ = volume; }
    float get_music_volume() const { return music_volume_; }

    void set_sound_volume(float volume) { sound_volume_ = volume; }
    float get_sound_volume() const { return sound_volume_; }

    // Callbacks
    using style_callback = std::function<void(ui_style)>;
    using volume_callback = std::function<void(float)>;
    using callback = std::function<void()>;

    void set_on_style_change(style_callback cb) { on_style_change_ = std::move(cb); }
    void set_on_music_volume_change(volume_callback cb) { on_music_volume_change_ = std::move(cb); }
    void set_on_sound_volume_change(volume_callback cb) { on_sound_volume_change_ = std::move(cb); }
    void set_on_apply(callback cb) { on_apply_ = std::move(cb); }
    void set_on_close_callback(callback cb) { on_close_cb_ = std::move(cb); }

private:
    void render_section_header(renderer& rend, int32_t y, const char* text);
    void render_toggle_option(renderer& rend, int32_t y, const char* label, bool selected, bool hovered);
    void render_slider(renderer& rend, int32_t y, const char* label, float value, bool hovered);
    int32_t get_hovered_element(int32_t mouse_x, int32_t mouse_y) const;

    // Settings values
    ui_style current_style_ = ui_style::classic;
    float music_volume_ = 1.0f;
    float sound_volume_ = 1.0f;

    // Callbacks
    style_callback on_style_change_;
    volume_callback on_music_volume_change_;
    volume_callback on_sound_volume_change_;
    callback on_apply_;
    callback on_close_cb_;

    int32_t hovered_element_ = -1;
    bool dragging_slider_ = false;
    int32_t dragging_slider_index_ = -1;

    // Layout
    static constexpr int32_t dialog_width = 300;
    static constexpr int32_t dialog_height = 280;

    // Element indices for hit testing
    static constexpr int32_t elem_style_classic = 0;
    static constexpr int32_t elem_style_modern = 1;
    static constexpr int32_t elem_music_slider = 2;
    static constexpr int32_t elem_sound_slider = 3;
    static constexpr int32_t elem_apply_button = 4;
};

} // namespace hb
