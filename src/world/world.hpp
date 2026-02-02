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

// Weather types
enum class weather_type : uint8_t
{
    clear = 0,
    rain = 1,
    snow = 2,
    storm = 3,
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

    // Render world
    void render(renderer& rend);

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

    // Get effective camera position (includes shake offset)
    int32_t camera_x() const { return camera_x_ + shake_offset_x_; }
    int32_t camera_y() const { return camera_y_ + shake_offset_y_; }

    // World state
    void set_weather(weather_type weather);
    weather_type weather() const { return weather_; }

    void set_time(time_of_day time);
    time_of_day time() const { return time_; }

    // Coordinate helpers
    std::pair<int32_t, int32_t> screen_to_tile(int32_t screen_x, int32_t screen_y) const;
    std::pair<int32_t, int32_t> screen_to_world(int32_t screen_x, int32_t screen_y) const;
    std::pair<int32_t, int32_t> world_to_screen(int32_t world_x, int32_t world_y) const;

    // Events
    void set_events(const world_events& events) { events_ = events; }

    // Renderer configuration
    void set_render_config(const map_render_config& config) { map_renderer_.set_config(config); }
    const map_render_config& render_config() const { return map_renderer_.config(); }

private:
    void update_camera(float delta_time);
    void update_lighting();
    void center_on_player();

    map current_map_;
    map_renderer map_renderer_;

    // Camera - follows player unless in cinematic mode
    int32_t camera_x_ = 0;
    int32_t camera_y_ = 0;
    int32_t player_world_x_ = 0;  // Player position to follow
    int32_t player_world_y_ = 0;
    bool cinematic_mode_ = false;

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

    // Events
    world_events events_;
};

} // namespace hb
