#pragma once

#include <cstdint>
#include <string>

namespace hb {

class renderer;
class entity;

namespace debug {

// Debug statistics overlay - displays FPS, camera info, and other debug data
// Toggle via Settings dialog
class debug_stats
{
public:
    static debug_stats& instance();

    // Non-copyable
    debug_stats(const debug_stats&) = delete;
    debug_stats& operator=(const debug_stats&) = delete;

    // Toggle visibility
    void toggle() { visible_ = !visible_; }
    void set_visible(bool visible) { visible_ = visible; }
    bool visible() const { return visible_; }

    // Update stats (call every frame)
    void update(float delta_time);

    // Render the stats overlay
    void render(renderer& rend);

    // === Camera & World ===
    void set_camera_bounds(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void set_player_position(int32_t tile_x, int32_t tile_y, int32_t world_x, int32_t world_y);
    void set_entity_count(int32_t count) { entity_count_ = count; }
    void set_map_name(const std::string& name) { map_name_ = name; }
    void set_zoom_level(float level) { zoom_level_ = level; }
    void set_chunk_count(int32_t count) { chunk_count_ = count; }
    void set_weather(const std::string& weather) { weather_ = weather; }
    void set_time_of_day(const std::string& time) { time_of_day_ = time; }

    // === Network Stats ===
    void set_network_connected(bool connected) { network_connected_ = connected; }
    void set_ping(int32_t ping_ms) { ping_ms_ = ping_ms; }
    void set_network_message_totals(uint64_t total_received, uint64_t total_sent)
    {
        net_total_received_ = total_received;
        net_total_sent_ = total_sent;
    }

    // === Rendering Stats ===
    void set_sprites_rendered(int32_t count) { sprites_rendered_ = count; }
    void set_tiles_rendered(int32_t count) { tiles_rendered_ = count; }
    void set_objects_rendered(int32_t count) { objects_rendered_ = count; }
    void set_draw_calls(uint32_t count) { draw_calls_ = count; }
    void add_sprites_rendered(int32_t count) { sprites_rendered_ += count; }
    void add_tiles_rendered(int32_t count) { tiles_rendered_ += count; }
    void reset_frame_counters() { sprites_rendered_ = 0; tiles_rendered_ = 0; objects_rendered_ = 0; }

    // === Memory/Assets ===
    void set_sprite_cache_count(int32_t count) { sprite_cache_count_ = count; }
    void set_pak_files_loaded(int32_t count) { pak_files_loaded_ = count; }

    // === Input ===
    void set_mouse_screen_pos(int32_t x, int32_t y) { mouse_screen_x_ = x; mouse_screen_y_ = y; }
    void set_mouse_world_pos(int32_t x, int32_t y) { mouse_world_x_ = x; mouse_world_y_ = y; }
    void set_mouse_tile_pos(int32_t x, int32_t y) { mouse_tile_x_ = x; mouse_tile_y_ = y; }
    void set_hovered_entity(const std::string& info) { hovered_entity_ = info; }

    // === Entity Info Overlay ===
    void set_entity_info_visible(bool visible) { entity_info_visible_ = visible; }
    bool entity_info_visible() const { return entity_info_visible_; }
    void set_hovered_entity_ptr(const entity* ent) { hovered_entity_ptr_ = ent; }
    void set_pinned_entity(const entity* ent) { pinned_entity_ = ent; }
    const entity* pinned_entity() const { return pinned_entity_; }
    void clear_pinned_entity() { pinned_entity_ = nullptr; }
    void render_entity_info(renderer& rend, int32_t screen_width, int32_t screen_height);

    // === Game State ===
    void set_game_state(const std::string& state) { game_state_ = state; }
    void set_combat_mode(bool attack_mode, bool safe_mode);

    // === Player Stats ===
    void set_player_stats(int32_t hp, int32_t max_hp, int32_t mp, int32_t max_mp, uint16_t level);
    void set_player_movement(const std::string& direction, bool moving, float progress, bool running);

    // === UI ===
    void set_open_dialog_count(int32_t count) { open_dialog_count_ = count; }

    // === Audio ===
    void set_active_sounds(int32_t count) { active_sounds_ = count; }
    void set_bgm_track(const std::string& track) { bgm_track_ = track; }

private:
    debug_stats() = default;

    void render_section(renderer& rend, int32_t& y, const char* title);

    bool visible_ = false;

    // FPS tracking
    float fps_timer_ = 0.0f;
    int32_t frame_count_ = 0;
    float current_fps_ = 0.0f;
    float delta_time_ms_ = 0.0f;

    // Camera bounds
    int32_t camera_left_ = 0;
    int32_t camera_top_ = 0;
    int32_t camera_right_ = 0;
    int32_t camera_bottom_ = 0;

    // Player position
    int32_t player_tile_x_ = 0;
    int32_t player_tile_y_ = 0;
    int32_t player_world_x_ = 0;
    int32_t player_world_y_ = 0;

    // Entity/map info
    int32_t entity_count_ = 0;
    std::string map_name_;
    float zoom_level_ = 1.0f;
    int32_t chunk_count_ = 0;
    std::string weather_;
    std::string time_of_day_;

    // Network stats
    bool network_connected_ = false;
    int32_t ping_ms_ = 0;
    int32_t messages_received_per_sec_ = 0;
    int32_t messages_sent_per_sec_ = 0;
    uint64_t net_total_received_ = 0;
    uint64_t net_total_sent_ = 0;
    uint64_t net_prev_received_ = 0;
    uint64_t net_prev_sent_ = 0;
    float network_stats_timer_ = 0.0f;

    // Rendering stats
    int32_t sprites_rendered_ = 0;
    int32_t tiles_rendered_ = 0;
    int32_t objects_rendered_ = 0;
    uint32_t draw_calls_ = 0;

    // Memory/Assets
    int32_t sprite_cache_count_ = 0;
    int32_t pak_files_loaded_ = 0;

    // Input
    int32_t mouse_screen_x_ = 0;
    int32_t mouse_screen_y_ = 0;
    int32_t mouse_world_x_ = 0;
    int32_t mouse_world_y_ = 0;
    int32_t mouse_tile_x_ = 0;
    int32_t mouse_tile_y_ = 0;
    std::string hovered_entity_;

    // Game state
    std::string game_state_;
    bool combat_mode_ = false;
    bool safe_attack_mode_ = false;

    // Player stats
    int32_t player_hp_ = 0;
    int32_t player_max_hp_ = 0;
    int32_t player_mp_ = 0;
    int32_t player_max_mp_ = 0;
    uint16_t player_level_ = 0;
    std::string player_direction_;
    bool player_moving_ = false;
    bool player_running_ = false;
    float player_move_progress_ = 0.0f;

    // UI
    int32_t open_dialog_count_ = 0;

    // Audio
    int32_t active_sounds_ = 0;
    std::string bgm_track_;

    // Entity info overlay
    bool entity_info_visible_ = false;
    const entity* hovered_entity_ptr_ = nullptr;
    const entity* pinned_entity_ = nullptr;
    static constexpr int32_t entity_info_width_ = 300;
    static constexpr int32_t icon_panel_height_ = 46;

    // Layout
    static constexpr int32_t padding_ = 10;
    static constexpr int32_t line_height_ = 14;
    static constexpr int32_t section_spacing_ = 6;
    static constexpr int32_t box_width_ = 260;
};

} // namespace debug
} // namespace hb
