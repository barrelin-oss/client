#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <string>
#include <vector>

namespace hb {

// Resolution option for settings
struct resolution_option {
    uint32_t width;
    uint32_t height;
    std::string label;
};

// Framerate option for settings
struct framerate_option {
    uint32_t fps;       // 0 = unlimited
    std::string label;
};

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

    void set_resolution(uint32_t width, uint32_t height);
    void get_resolution(uint32_t& width, uint32_t& height) const;

    void set_fullscreen(bool fullscreen) { fullscreen_ = fullscreen; applied_fullscreen_ = fullscreen; }
    bool get_fullscreen() const { return fullscreen_; }

    void set_vsync(bool vsync) { vsync_ = vsync; }
    bool get_vsync() const { return vsync_; }

    void set_framerate(uint32_t fps);
    uint32_t get_framerate() const;

    // Call from resolution callback to prevent dialog from closing after Apply
    void keep_open_after_apply() { skip_close_after_apply_ = true; }

    void set_music_volume(float volume) { music_volume_ = volume; }
    float get_music_volume() const { return music_volume_; }

    void set_sound_volume(float volume) { sound_volume_ = volume; }
    float get_sound_volume() const { return sound_volume_; }

    // Callbacks
    using style_callback = std::function<void(ui_style)>;
    using resolution_callback = std::function<void(uint32_t, uint32_t, bool)>;
    using framerate_callback = std::function<void(uint32_t)>;
    using bool_callback = std::function<void(bool)>;
    using volume_callback = std::function<void(float)>;
    using callback = std::function<void()>;

    void set_on_style_change(style_callback cb) { on_style_change_ = std::move(cb); }
    void set_on_resolution_change(resolution_callback cb) { on_resolution_change_ = std::move(cb); }
    void set_on_framerate_change(framerate_callback cb) { on_framerate_change_ = std::move(cb); }
    void set_on_vsync_change(bool_callback cb) { on_vsync_change_ = std::move(cb); }
    void set_on_music_volume_change(volume_callback cb) { on_music_volume_change_ = std::move(cb); }
    void set_on_sound_volume_change(volume_callback cb) { on_sound_volume_change_ = std::move(cb); }
    void set_on_apply(callback cb) { on_apply_ = std::move(cb); }
    void set_on_close_callback(callback cb) { on_close_cb_ = std::move(cb); }

private:
    void render_section_header(renderer& rend, int32_t y, const char* text);
    void render_toggle_option(renderer& rend, int32_t y, const char* label, bool selected, bool hovered);
    void render_slider(renderer& rend, int32_t y, const char* label, float value, bool hovered);
    void render_dropdown(renderer& rend, int32_t y, const char* label, const std::string& value, bool hovered, float animation);
    void render_checkbox(renderer& rend, int32_t y, const char* label, bool checked, bool hovered);
    int32_t get_hovered_element(int32_t mouse_x, int32_t mouse_y) const;
    void init_resolution_options();
    void init_framerate_options();

    // Settings values
    ui_style current_style_ = ui_style::classic;
    float music_volume_ = 1.0f;
    float sound_volume_ = 1.0f;

    // Resolution settings
    std::vector<resolution_option> resolution_options_;
    int32_t selected_resolution_ = 0;
    int32_t applied_resolution_ = 0;  // Last applied resolution (to detect changes)
    bool resolution_dropdown_expanded_ = false;
    int32_t resolution_dropdown_hovered_ = -1;
    bool fullscreen_ = false;
    bool applied_fullscreen_ = false;  // Last applied fullscreen state
    bool vsync_ = true;
    bool skip_close_after_apply_ = false;  // Set by resolution callback to keep dialog open

    // Framerate settings
    std::vector<framerate_option> framerate_options_;
    int32_t selected_framerate_ = 1;  // Default to 60 fps (index 1)
    bool framerate_dropdown_expanded_ = false;
    int32_t framerate_dropdown_hovered_ = -1;

    // Dropdown animations
    float resolution_dropdown_animation_ = 0.0f;  // 0.0 = closed, 1.0 = fully open
    float framerate_dropdown_animation_ = 0.0f;
    static constexpr float dropdown_animation_speed_ = 10.0f;

    // Callbacks
    style_callback on_style_change_;
    resolution_callback on_resolution_change_;
    framerate_callback on_framerate_change_;
    bool_callback on_vsync_change_;
    volume_callback on_music_volume_change_;
    volume_callback on_sound_volume_change_;
    callback on_apply_;
    callback on_close_cb_;

    int32_t hovered_element_ = -1;
    bool dragging_slider_ = false;
    int32_t dragging_slider_index_ = -1;

    // Layout
    static constexpr int32_t dialog_width = 300;
    static constexpr int32_t dialog_height = 448;

    // Element indices for hit testing
    static constexpr int32_t elem_style_classic = 0;
    static constexpr int32_t elem_style_modern = 1;
    static constexpr int32_t elem_resolution_dropdown = 2;
    static constexpr int32_t elem_framerate_dropdown = 3;
    static constexpr int32_t elem_fullscreen_checkbox = 4;
    static constexpr int32_t elem_vsync_checkbox = 5;
    static constexpr int32_t elem_music_slider = 6;
    static constexpr int32_t elem_sound_slider = 7;
    static constexpr int32_t elem_apply_button = 8;
    // Resolution dropdown items start at this index
    static constexpr int32_t elem_resolution_item_base = 100;
    // Framerate dropdown items start at this index
    static constexpr int32_t elem_framerate_item_base = 200;
};

} // namespace hb
