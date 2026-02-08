#pragma once

#include "ui/ui_system.hpp"
#include "platform/monitor.hpp"
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

// Monitor option for settings dropdown
struct monitor_option {
    int32_t index;
    std::string label;
    int32_t x, y, width, height;
};

// Tab categories for settings dialog
enum class settings_tab : uint8_t {
    game,
    video,
    audio,
    social,
    keybindings,
    help,
    system,
    debug,
    count  // sentinel
};

// Settings dialog - tabbed dialog for all game configuration
// Replaces both the old settings_dialog and system_menu_dialog
class settings_dialog : public dialog {
public:
    settings_dialog();
    ~settings_dialog() override = default;

    void open() override;
    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_key_press(sf::Keyboard::Key key) override;

    // === Setters for current state ===

    // Game tab
    void set_ui_style(ui_style style) { current_style_ = style; }
    ui_style get_ui_style() const { return current_style_; }
    void set_show_damage_numbers(bool v) { show_damage_numbers_ = v; }
    void set_show_names(bool v) { show_names_ = v; }
    void set_show_guild_names(bool v) { show_guild_names_ = v; }
    void set_show_hp_bars(bool v) { show_hp_bars_ = v; }
    void set_camera_shake(bool v) { camera_shake_ = v; }
    void set_show_weather(bool v) { show_weather_ = v; }
    void set_show_tint(bool v) { show_tint_ = v; }
    void set_type_to_chat(bool v) { type_to_chat_ = v; }

    // Video tab
    void set_resolution(uint32_t width, uint32_t height);
    void get_resolution(uint32_t& width, uint32_t& height) const;
    void set_fullscreen(bool fullscreen) { fullscreen_ = fullscreen; applied_fullscreen_ = fullscreen; }
    bool get_fullscreen() const { return fullscreen_; }
    void set_display_mode(bool fullscreen, bool borderless);
    int32_t get_display_mode() const { return selected_display_mode_; }
    void set_monitors(std::vector<monitor_info> monitors);
    void set_monitor_index(int32_t index);
    void set_vsync(bool vsync) { vsync_ = vsync; }
    void set_remember_position(bool remember) { remember_position_ = remember; }
    void set_framerate(uint32_t fps);
    uint32_t get_framerate() const;
    void set_aspect_mode(uint8_t mode) { aspect_mode_ = mode; }
    void set_scale_filter(uint8_t filter) { scale_filter_ = filter; }
    void set_ui_scale(float scale) { ui_scale_ = scale; }

    // Audio tab
    void set_master_volume(float v) { master_volume_ = v; }
    float get_master_volume() const { return master_volume_; }
    void set_music_volume(float v) { music_volume_ = v; }
    float get_music_volume() const { return music_volume_; }
    void set_sound_volume(float v) { sound_volume_ = v; }
    float get_sound_volume() const { return sound_volume_; }
    void set_music_enabled(bool v) { music_enabled_ = v; }
    void set_sfx_enabled(bool v) { sfx_enabled_ = v; }

    // Social tab
    void set_show_timestamps(bool v) { show_timestamps_ = v; }
    void set_filter_profanity(bool v) { filter_profanity_ = v; }
    void set_block_spam(bool v) { block_spam_ = v; }

    // Debug tab
    void set_show_debug_stats(bool v) { show_debug_stats_ = v; }
    void set_show_fps(bool v) { show_fps_ = v; }

    // GM flag (controls debug tab visibility)
    void set_is_gm(bool v) { is_gm_ = v; }

    // Call from resolution callback to prevent dialog from closing after Apply
    void keep_open_after_apply() { skip_close_after_apply_ = true; }

    // === Callbacks ===
    using style_callback = std::function<void(ui_style)>;
    using resolution_callback = std::function<void(uint32_t, uint32_t, bool, bool, int32_t, int32_t)>;
    using framerate_callback = std::function<void(uint32_t)>;
    using bool_callback = std::function<void(bool)>;
    using volume_callback = std::function<void(float)>;
    using float_callback = std::function<void(float)>;
    using uint8_callback = std::function<void(uint8_t)>;
    using callback = std::function<void()>;

    // Game tab callbacks
    void set_on_style_change(style_callback cb) { on_style_change_ = std::move(cb); }
    void set_on_show_damage_numbers_change(bool_callback cb) { on_show_damage_numbers_change_ = std::move(cb); }
    void set_on_show_names_change(bool_callback cb) { on_show_names_change_ = std::move(cb); }
    void set_on_show_guild_names_change(bool_callback cb) { on_show_guild_names_change_ = std::move(cb); }
    void set_on_show_hp_bars_change(bool_callback cb) { on_show_hp_bars_change_ = std::move(cb); }
    void set_on_camera_shake_change(bool_callback cb) { on_camera_shake_change_ = std::move(cb); }
    void set_on_show_weather_change(bool_callback cb) { on_show_weather_change_ = std::move(cb); }
    void set_on_show_tint_change(bool_callback cb) { on_show_tint_change_ = std::move(cb); }
    void set_on_type_to_chat_change(bool_callback cb) { on_type_to_chat_change_ = std::move(cb); }

    // Video tab callbacks
    void set_on_resolution_change(resolution_callback cb) { on_resolution_change_ = std::move(cb); }
    void set_on_framerate_change(framerate_callback cb) { on_framerate_change_ = std::move(cb); }
    void set_on_vsync_change(bool_callback cb) { on_vsync_change_ = std::move(cb); }
    void set_on_remember_position_change(bool_callback cb) { on_remember_position_change_ = std::move(cb); }
    void set_on_aspect_mode_change(uint8_callback cb) { on_aspect_mode_change_ = std::move(cb); }
    void set_on_scale_filter_change(uint8_callback cb) { on_scale_filter_change_ = std::move(cb); }
    void set_on_ui_scale_change(float_callback cb) { on_ui_scale_change_ = std::move(cb); }
    void set_on_apply(callback cb) { on_apply_ = std::move(cb); }

    // Audio tab callbacks
    void set_on_master_volume_change(volume_callback cb) { on_master_volume_change_ = std::move(cb); }
    void set_on_music_volume_change(volume_callback cb) { on_music_volume_change_ = std::move(cb); }
    void set_on_sound_volume_change(volume_callback cb) { on_sound_volume_change_ = std::move(cb); }
    void set_on_music_enabled_change(bool_callback cb) { on_music_enabled_change_ = std::move(cb); }
    void set_on_sfx_enabled_change(bool_callback cb) { on_sfx_enabled_change_ = std::move(cb); }

    // Social tab callbacks
    void set_on_show_timestamps_change(bool_callback cb) { on_show_timestamps_change_ = std::move(cb); }
    void set_on_filter_profanity_change(bool_callback cb) { on_filter_profanity_change_ = std::move(cb); }
    void set_on_block_spam_change(bool_callback cb) { on_block_spam_change_ = std::move(cb); }

    // Debug tab callbacks
    void set_on_show_debug_stats_change(bool_callback cb) { on_show_debug_stats_change_ = std::move(cb); }
    void set_on_show_fps_change(bool_callback cb) { on_show_fps_change_ = std::move(cb); }

    // System tab callbacks
    void set_on_logout(callback cb) { on_logout_ = std::move(cb); }
    void set_on_exit(callback cb) { on_exit_ = std::move(cb); }

    // Help tab callbacks
    void set_on_help(callback cb) { on_help_ = std::move(cb); }

    // Close callback
    void set_on_close_callback(callback cb) { on_close_cb_ = std::move(cb); }

private:
    // Rendering helpers
    void render_tab_bar(renderer& rend);
    void render_game_tab(renderer& rend, int32_t content_y);
    void render_video_tab(renderer& rend, int32_t content_y);
    void render_audio_tab(renderer& rend, int32_t content_y);
    void render_social_tab(renderer& rend, int32_t content_y);
    void render_keybindings_tab(renderer& rend, int32_t content_y);
    void render_help_tab(renderer& rend, int32_t content_y);
    void render_system_tab(renderer& rend, int32_t content_y);
    void render_debug_tab(renderer& rend, int32_t content_y);

    // Shared rendering primitives
    void render_section_header(renderer& rend, int32_t y, const char* text);
    void render_toggle_option(renderer& rend, int32_t y, const char* label, bool selected, bool hovered);
    void render_slider(renderer& rend, int32_t y, const char* label, float value, bool hovered);
    void render_dropdown(renderer& rend, int32_t y, const char* label, const std::string& value, bool hovered, float animation);
    void render_checkbox(renderer& rend, int32_t y, const char* label, bool checked, bool hovered);
    void render_button_widget(renderer& rend, int32_t x, int32_t y, int32_t w, int32_t h,
                              const char* text, bool hovered, sf::Color normal, sf::Color hover, sf::Color border);

    // Hit testing
    int32_t get_hovered_element(int32_t mouse_x, int32_t mouse_y) const;
    int32_t get_hovered_tab(int32_t mouse_x, int32_t mouse_y) const;
    int32_t get_hovered_element_game(int32_t mx, int32_t my, int32_t content_y) const;
    int32_t get_hovered_element_video(int32_t mx, int32_t my, int32_t content_y) const;
    int32_t get_hovered_element_audio(int32_t mx, int32_t my, int32_t content_y) const;
    int32_t get_hovered_element_social(int32_t mx, int32_t my, int32_t content_y) const;
    int32_t get_hovered_element_help(int32_t mx, int32_t my, int32_t content_y) const;
    int32_t get_hovered_element_system(int32_t mx, int32_t my, int32_t content_y) const;
    int32_t get_hovered_element_debug(int32_t mx, int32_t my, int32_t content_y) const;

    // Input handling per tab
    bool handle_game_tab_click(int32_t elem);
    bool handle_video_tab_click(int32_t elem);
    bool handle_audio_tab_click(int32_t elem, int32_t x);
    bool handle_social_tab_click(int32_t elem);
    bool handle_help_tab_click(int32_t elem);
    bool handle_system_tab_click(int32_t elem);
    bool handle_debug_tab_click(int32_t elem);

    // Resolution/framerate helpers
    void init_resolution_options();
    void init_framerate_options();
    void rebuild_resolution_options();

    // === Tab state ===
    settings_tab active_tab_ = settings_tab::game;
    int32_t hovered_tab_ = -1;
    bool is_gm_ = false;

    // === Game tab state ===
    ui_style current_style_ = ui_style::classic;
    bool show_damage_numbers_ = true;
    bool show_names_ = true;
    bool show_guild_names_ = true;
    bool show_hp_bars_ = true;
    bool camera_shake_ = true;
    bool show_weather_ = true;
    bool show_tint_ = true;
    bool type_to_chat_ = false;

    // === Video tab state ===
    std::vector<monitor_info> monitors_;
    std::vector<monitor_option> monitor_options_;
    int32_t selected_monitor_ = 0;
    int32_t applied_monitor_ = 0;
    bool monitor_dropdown_expanded_ = false;
    float monitor_dropdown_animation_ = 0.0f;
    int32_t monitor_dropdown_hovered_ = -1;

    int32_t selected_display_mode_ = 0;
    int32_t applied_display_mode_ = 0;
    bool display_mode_dropdown_expanded_ = false;
    float display_mode_dropdown_animation_ = 0.0f;
    int32_t display_mode_dropdown_hovered_ = -1;
    static constexpr int32_t display_mode_count_ = 3;
    static constexpr const char* display_mode_labels_[] = {"Windowed", "Borderless Windowed", "Fullscreen"};

    std::vector<resolution_option> resolution_options_;
    int32_t selected_resolution_ = 0;
    int32_t applied_resolution_ = 0;
    bool resolution_dropdown_expanded_ = false;
    int32_t resolution_dropdown_hovered_ = -1;
    bool fullscreen_ = false;
    bool applied_fullscreen_ = false;
    bool vsync_ = true;
    bool remember_position_ = false;
    bool skip_close_after_apply_ = false;

    std::vector<framerate_option> framerate_options_;
    int32_t selected_framerate_ = 1;
    bool framerate_dropdown_expanded_ = false;
    int32_t framerate_dropdown_hovered_ = -1;

    float resolution_dropdown_animation_ = 0.0f;
    float framerate_dropdown_animation_ = 0.0f;
    static constexpr float dropdown_animation_speed_ = 10.0f;

    // View mode settings
    uint8_t aspect_mode_ = 0;
    uint8_t scale_filter_ = 0;
    float ui_scale_ = 1.0f;

    // Revert confirmation countdown after display mode changes
    struct revert_state {
        uint32_t width;
        uint32_t height;
        bool fullscreen;
        bool borderless;
        int32_t monitor_x;
        int32_t monitor_y;
        int32_t resolution_index;
        int32_t display_mode_index;
        int32_t monitor_index;
    };
    bool revert_countdown_active_ = false;
    float revert_countdown_timer_ = 0.0f;
    revert_state revert_state_{};
    static constexpr float revert_countdown_duration_ = 5.0f;

    // === Audio tab state ===
    float master_volume_ = 1.0f;
    float music_volume_ = 1.0f;
    float sound_volume_ = 1.0f;
    bool music_enabled_ = true;
    bool sfx_enabled_ = true;

    // === Social tab state ===
    bool show_timestamps_ = false;
    bool filter_profanity_ = true;
    bool block_spam_ = true;

    // === Debug tab state ===
    bool show_debug_stats_ = false;
    bool show_fps_ = false;

    // === Callbacks ===
    // Game
    style_callback on_style_change_;
    bool_callback on_show_damage_numbers_change_;
    bool_callback on_show_names_change_;
    bool_callback on_show_guild_names_change_;
    bool_callback on_show_hp_bars_change_;
    bool_callback on_camera_shake_change_;
    bool_callback on_show_weather_change_;
    bool_callback on_show_tint_change_;
    bool_callback on_type_to_chat_change_;
    // Video
    resolution_callback on_resolution_change_;
    framerate_callback on_framerate_change_;
    bool_callback on_vsync_change_;
    bool_callback on_remember_position_change_;
    uint8_callback on_aspect_mode_change_;
    uint8_callback on_scale_filter_change_;
    float_callback on_ui_scale_change_;
    callback on_apply_;
    // Audio
    volume_callback on_master_volume_change_;
    volume_callback on_music_volume_change_;
    volume_callback on_sound_volume_change_;
    bool_callback on_music_enabled_change_;
    bool_callback on_sfx_enabled_change_;
    // Social
    bool_callback on_show_timestamps_change_;
    bool_callback on_filter_profanity_change_;
    bool_callback on_block_spam_change_;
    // Debug
    bool_callback on_show_debug_stats_change_;
    bool_callback on_show_fps_change_;
    // System
    callback on_logout_;
    callback on_exit_;
    // Help
    callback on_help_;
    // Close
    callback on_close_cb_;

    // === Interaction state ===
    int32_t hovered_element_ = -1;
    bool dragging_slider_ = false;
    int32_t dragging_slider_index_ = -1;

    // === Layout constants ===
    static constexpr int32_t dialog_width = 500;
    static constexpr int32_t dialog_height = 420;
    static constexpr int32_t title_bar_height = 28;
    static constexpr int32_t tab_bar_height = 28;
    static constexpr int32_t content_start_y_offset = title_bar_height + tab_bar_height;
    static constexpr int32_t content_padding = 15;
    static constexpr int32_t checkbox_row_height = 26;
    static constexpr int32_t slider_row_height = 32;
    static constexpr int32_t dropdown_row_height = 32;
    static constexpr int32_t section_header_height = 25;

    // Element indices for hit testing (per-tab, reused across tabs)
    // Game tab elements
    static constexpr int32_t elem_style_classic = 0;
    static constexpr int32_t elem_style_modern = 1;
    static constexpr int32_t elem_show_damage = 2;
    static constexpr int32_t elem_show_names = 3;
    static constexpr int32_t elem_show_guild_names = 4;
    static constexpr int32_t elem_show_hp_bars = 5;
    static constexpr int32_t elem_camera_shake = 6;
    static constexpr int32_t elem_show_weather = 7;
    static constexpr int32_t elem_show_tint = 8;
    static constexpr int32_t elem_type_to_chat = 9;

    // Video tab elements
    static constexpr int32_t elem_display_mode_dropdown = 20;
    static constexpr int32_t elem_monitor_dropdown = 21;
    static constexpr int32_t elem_resolution_dropdown = 22;
    static constexpr int32_t elem_framerate_dropdown = 23;
    static constexpr int32_t elem_vsync_checkbox = 24;
    static constexpr int32_t elem_remember_position_checkbox = 25;
    static constexpr int32_t elem_aspect_mode_dropdown = 26;
    static constexpr int32_t elem_scale_filter_dropdown = 27;
    static constexpr int32_t elem_ui_scale_slider = 28;
    static constexpr int32_t elem_apply_button = 29;
    static constexpr int32_t elem_keep_changes_button = 30;
    static constexpr int32_t elem_revert_button = 31;

    // Audio tab elements
    static constexpr int32_t elem_master_slider = 40;
    static constexpr int32_t elem_music_slider = 41;
    static constexpr int32_t elem_sound_slider = 42;
    static constexpr int32_t elem_music_enabled = 43;
    static constexpr int32_t elem_sfx_enabled = 44;

    // Social tab elements
    static constexpr int32_t elem_show_timestamps = 50;
    static constexpr int32_t elem_filter_profanity = 51;
    static constexpr int32_t elem_block_spam = 52;

    // Help tab elements
    static constexpr int32_t elem_open_help = 70;
    static constexpr int32_t elem_forum_link = 71;
    static constexpr int32_t elem_discord_link = 72;

    // System tab elements
    static constexpr int32_t elem_logout_button = 80;
    static constexpr int32_t elem_exit_button = 81;

    // Debug tab elements
    static constexpr int32_t elem_debug_stats = 90;
    static constexpr int32_t elem_show_fps_cb = 91;

    // Dropdown item base indices
    static constexpr int32_t elem_monitor_item_base = 100;
    static constexpr int32_t elem_display_mode_item_base = 150;
    static constexpr int32_t elem_resolution_item_base = 200;
    static constexpr int32_t elem_framerate_item_base = 300;
};

} // namespace hb
