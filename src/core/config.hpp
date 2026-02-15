#pragma once

#include "localization/localization.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <functional>

namespace hb
{

// Video settings
struct video_config
{
    uint32_t screen_width = 640;
    uint32_t screen_height = 480;
    bool fullscreen = false;
    bool borderless = false;   // borderless windowed mode
    int32_t monitor_index = 0; // which monitor to use (0 = primary)
    bool vsync = true;
    uint32_t framerate_limit = 60; // 0 = unlimited
    bool show_fps = false;
    bool show_debug_stats = false;
    bool show_entity_info = false;
    bool remember_position = false;
    int32_t window_x = -1; // -1 = center on primary monitor
    int32_t window_y = -1;

    // View mode settings (player-configurable)
    uint8_t aspect_mode = 0;  // 0=letterbox, 1=stretch (scaled mode only)
    uint8_t scale_filter = 0; // 0=nearest, 1=bilinear (scaled mode only)
    float ui_scale = 1.0f;    // 1.0 = native, max ~3.0 (all modes)
};

// Audio settings
struct audio_config
{
    float master_volume = 1.0f;
    float music_volume = 0.7f;
    float sfx_volume = 1.0f;
    bool muted = false;
    bool music_enabled = true;
    bool sfx_enabled = true;
};

// Network settings
struct network_config
{
    std::string login_server_host = "127.0.0.1";
    uint16_t login_server_port = 2848;
    uint32_t connection_timeout_ms = 10000;
    uint32_t reconnect_attempts = 3;
    uint32_t reconnect_delay_ms = 2000;
};

// Chat settings
struct chat_config_settings
{
    size_t max_history = 500;
    size_t max_message_length = 200;
    bool show_timestamps = false;
    bool filter_profanity = true;
    bool block_spam = true;
    float spam_delay = 0.5f;

    // Channel visibility
    bool show_normal = true;
    bool show_shout = true;
    bool show_whisper = true;
    bool show_guild = true;
    bool show_party = true;
    bool show_system = true;
    bool show_trade = true;
    bool show_global = true;
};

// Game settings
struct game_config
{
    language ui_language = language::english;
    bool auto_attack = true;
    bool show_damage_numbers = true;
    bool show_names = true;
    bool show_guild_names = true;
    bool show_hp_bars = true;
    bool camera_shake = true;
    bool show_weather = true;
    bool show_tint = true;
    float camera_speed = 1.0f;
    bool type_to_chat = false; // Any key press opens chat (legacy behavior, disables WASD movement)
};

// Control settings
struct control_config
{
    // Movement
    int32_t move_up_key = 0;    // Default W
    int32_t move_down_key = 0;  // Default S
    int32_t move_left_key = 0;  // Default A
    int32_t move_right_key = 0; // Default D

    // Actions
    int32_t attack_key = 0;     // Default left mouse
    int32_t skill_key = 0;      // Default right mouse
    int32_t inventory_key = 0;  // Default I
    int32_t skills_key = 0;     // Default K
    int32_t spells_key = 0;     // Default M
    int32_t chat_key = 0;       // Default Enter
    int32_t screenshot_key = 0; // Default F12

    // Mouse settings
    float mouse_sensitivity = 1.0f;
    bool invert_mouse_y = false;
};

// Configuration change callback
using config_change_callback = std::function<void()>;

class config
{
public:
    config() = default;
    ~config() = default;

    // Initialize with default values
    void initialize();

    // Load/save from file
    bool load(std::string_view path);
    bool save(std::string_view path) const;
    bool save() const; // Save to last loaded path

    // Access configuration sections
    video_config& video() { return video_; }
    const video_config& video() const { return video_; }

    audio_config& audio() { return audio_; }
    const audio_config& audio() const { return audio_; }

    network_config& network() { return network_; }
    const network_config& network() const { return network_; }

    chat_config_settings& chat() { return chat_; }
    const chat_config_settings& chat() const { return chat_; }

    game_config& game() { return game_; }
    const game_config& game() const { return game_; }

    control_config& controls() { return controls_; }
    const control_config& controls() const { return controls_; }

    // Reset to defaults
    void reset_video();
    void reset_audio();
    void reset_network();
    void reset_chat();
    void reset_game();
    void reset_controls();
    void reset_all();

    // Change notification
    void on_changed(config_change_callback callback);
    void notify_changed();

    // Get config file path
    std::string_view config_path() const { return config_path_; }

    // Singleton access
    static config& instance();

private:
    video_config video_;
    audio_config audio_;
    network_config network_;
    chat_config_settings chat_;
    game_config game_;
    control_config controls_;

    std::string config_path_;
    std::vector<config_change_callback> callbacks_;
};

} // namespace hb
