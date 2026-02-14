#pragma once

#include "world/map.hpp"
#include "world/map_renderer.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

namespace hb {

class renderer;
class tile_sprite_registry;

// Day/night tint color (RGBA)
struct tint_color
{
    uint8_t r = 0, g = 0, b = 0, a = 0;
};

// Weather types (matches legacy server values 0-6)
enum class weather_type : uint8_t
{
    clear = 0,
    light_rain = 1,
    medium_rain = 2,
    heavy_rain = 3,
    light_snow = 4,
    medium_snow = 5,
    heavy_snow = 6,
};

// Time of day
enum class time_of_day : uint8_t
{
    dawn = 0,
    morning = 1,
    noon = 2,
    afternoon = 3,
    dusk = 4,
    night = 5,
    midnight = 6,
};

// World events
struct world_events
{
    std::function<void(std::string_view old_map, std::string_view new_map)> on_map_changed;
    std::function<void(weather_type weather)> on_weather_changed;
    std::function<void(time_of_day time)> on_time_changed;
};

class world
{
public:
    world() = default;
    ~world() = default;

    world(const world&) = delete;
    world& operator=(const world&) = delete;

    // Initialize with tile sprite registry
    bool initialize(tile_sprite_registry& registry);
    void shutdown();

    // Update world state
    void update(float delta_time);

    // Render world (terrain + all objects + debug overlays)
    void render(renderer& rend);

    // Render terrain only (chunks + debug overlays, no objects)
    void render_terrain(renderer& rend);

    // Render objects for a single tile row
    // If player_bounds is provided, objects overlapping it are drawn semi-transparently
    void render_objects_row(renderer& rend, int32_t row_y,
                            const sf::IntRect* player_bounds = nullptr);

    // Get visible tile range for current camera position
    map_renderer::visible_range get_visible_tile_range() const;

    // Map management
    bool load_map(std::string_view map_name);
    bool load_map_data(const uint8_t* data, size_t size, std::string_view name);
    void unload_map();
    const map& current_map() const { return current_map_; }
    map& current_map_mut() { return current_map_; }
    std::string_view current_map_name() const { return current_map_.name(); }

    // Camera control
    // Set the player position the camera should follow (in world pixels)
    void set_player_position(int32_t world_x, int32_t world_y);

    // Camera shake (offset from player position, decays over time)
    void add_camera_shake(float intensity, float duration);

    // Cinematic mode - detaches camera from player for free movement
    void set_cinematic_mode(bool enabled);
    bool is_cinematic_mode() const { return cinematic_mode_; }

    // Direct camera control (only works in cinematic mode)
    void set_camera_position(int32_t x, int32_t y);
    void move_camera(int32_t dx, int32_t dy);

    // Drag-to-pan (cinematic mode only)
    void start_drag(int32_t mouse_x, int32_t mouse_y);
    void update_drag(int32_t mouse_x, int32_t mouse_y);
    void end_drag();
    bool is_dragging() const { return drag_active_; }

    // Update screen dimensions (call after resolution change)
    void set_screen_size(uint32_t width, uint32_t height);

    // Force re-center camera on player (useful after resolution change)
    void recenter_camera();

    // Get effective camera position (includes shake offset)
    int32_t camera_x() const { return static_cast<int32_t>(camera_x_) + shake_offset_x_; }
    int32_t camera_y() const { return static_cast<int32_t>(camera_y_) + shake_offset_y_; }

    // World state
    void set_weather(weather_type weather);
    weather_type weather() const { return weather_; }

    void set_time(time_of_day time);
    time_of_day time() const { return time_; }

    // Server clock (for display purposes)
    void set_clock(uint8_t hour, uint8_t minute);
    uint8_t clock_hour() const { return clock_hour_; }
    uint8_t clock_minute() const { return clock_minute_; }

    // Render day/night tint overlay (call after all scene rendering, before UI)
    void render_overlay(renderer& rend);

    // Tint overlay visibility toggle
    void set_tint_visible(bool v) { tint_visible_ = v; }
    bool tint_visible() const { return tint_visible_; }

    // Coordinate helpers
    std::pair<int32_t, int32_t> screen_to_tile(int32_t screen_x, int32_t screen_y) const;
    std::pair<int32_t, int32_t> screen_to_world(int32_t screen_x, int32_t screen_y) const;
    std::pair<int32_t, int32_t> world_to_screen(int32_t world_x, int32_t world_y) const;

    // Events
    void set_events(const world_events& events) { events_ = events; }

    // Renderer configuration
    void set_render_config(const map_render_config& config) { map_renderer_.set_config(config); }
    const map_render_config& render_config() const { return map_renderer_.config(); }

    // Per-frame object render count
    int32_t objects_rendered() const { return map_renderer_.objects_rendered(); }
    void reset_objects_rendered() { map_renderer_.reset_objects_rendered(); }

    // Debug stats
    size_t chunk_count() const { return map_renderer_.chunk_count(); }

    // Pathfinding debug trace
    void set_pathfinding_trace(std::vector<std::pair<int32_t, int32_t>> trace)
    {
        map_renderer_.set_pathfinding_trace(std::move(trace));
    }
    void clear_pathfinding_trace() { map_renderer_.clear_pathfinding_trace(); }

    // Occupied tile debug overlay
    void set_occupied_tiles(std::vector<std::pair<int32_t, int32_t>> tiles)
    {
        map_renderer_.set_occupied_tiles(std::move(tiles));
    }
    void clear_occupied_tiles() { map_renderer_.clear_occupied_tiles(); }

    // Zoom control
    void set_zoom_mode_enabled(bool enabled);
    bool is_zoom_mode_enabled() const { return zoom_mode_enabled_; }

    // Global render mode (cinematic mode only) - renders all entities regardless of distance
    void set_global_render_mode(bool enabled) { global_render_mode_ = enabled; }
    bool is_global_render_mode() const { return global_render_mode_; }
    // Adjust zoom level, optionally with cursor position for zoom-to-cursor effect
    void adjust_zoom(float delta, int32_t cursor_x = -1, int32_t cursor_y = -1);
    float zoom_level() const { return static_cast<float>(zoom_level_); }

    // View management for zoom (call before/after rendering world content)
    void apply_zoom_view(renderer& rend);
    void reset_zoom_view(renderer& rend);

private:
    void update_camera(float delta_time);
    void update_clock(float delta_time);
    void update_lighting();
    void update_tint(float delta_time);
    void center_on_player();

    static tint_color tint_for_time(time_of_day t);

    map current_map_;
    map_renderer map_renderer_;

    // Camera - follows player unless in cinematic mode (doubles for precision at high zoom)
    double camera_x_ = 0.0;
    double camera_y_ = 0.0;
    int32_t player_world_x_ = 0;  // Player position to follow
    int32_t player_world_y_ = 0;
    bool cinematic_mode_ = false;

    // Screen dimensions (updated on resolution change)
    uint32_t screen_width_ = 640;
    uint32_t screen_height_ = 480;

    // Camera shake
    int32_t shake_offset_x_ = 0;
    int32_t shake_offset_y_ = 0;
    float shake_intensity_ = 0.0f;
    float shake_duration_ = 0.0f;
    float shake_timer_ = 0.0f;

    // World state
    weather_type weather_ = weather_type::clear;
    time_of_day time_ = time_of_day::noon;
    float weather_intensity_ = 0.0f;
    uint8_t clock_hour_ = 12;
    uint8_t clock_minute_ = 0;
    float clock_accumulator_ = 0.0f;  // Accumulates real seconds; 1 real sec = 1 game minute
    bool tint_visible_ = true;

    // Day/night tint transition (current stored as float to avoid integer truncation stalls)
    float current_tint_r_ = 0.0f;
    float current_tint_g_ = 0.0f;
    float current_tint_b_ = 0.0f;
    float current_tint_a_ = 0.0f;
    tint_color target_tint_{};

    // Events
    world_events events_;

    // Zoom state
    double zoom_level_ = 1.0;           // Current zoom (interpolates toward target)
    double zoom_target_ = 1.0;          // Target zoom level
    bool zoom_mode_enabled_ = false;    // Server-controlled flag (Ctrl+Q for testing)

    // Zoom anchor - world point that stays fixed under cursor during zoom
    double zoom_anchor_world_x_ = 0.0;
    double zoom_anchor_world_y_ = 0.0;
    double zoom_anchor_screen_x_ = 0.0;
    double zoom_anchor_screen_y_ = 0.0;
    bool has_zoom_anchor_ = false;

    // Global render mode - renders all entities without distance culling (cinematic mode only)
    bool global_render_mode_ = false;

    // Drag-to-pan state (cinematic mode)
    bool drag_active_ = false;
    int32_t drag_start_mouse_x_ = 0;
    int32_t drag_start_mouse_y_ = 0;
    int32_t drag_start_camera_x_ = 0;
    int32_t drag_start_camera_y_ = 0;
};

} // namespace hb
