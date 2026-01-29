#pragma once

#include "world/map.hpp"
#include "world/map_renderer.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

namespace hb {

class renderer;
class pak_file;

// Weather types
enum class weather_type : uint8_t {
    clear = 0,
    rain = 1,
    snow = 2,
    storm = 3,
};

// Time of day
enum class time_of_day : uint8_t {
    dawn = 0,
    morning = 1,
    noon = 2,
    afternoon = 3,
    dusk = 4,
    night = 5,
    midnight = 6,
};

// World events
struct world_events {
    std::function<void(std::string_view old_map, std::string_view new_map)> on_map_changed;
    std::function<void(weather_type weather)> on_weather_changed;
    std::function<void(time_of_day time)> on_time_changed;
};

class world {
public:
    world() = default;
    ~world() = default;

    world(const world&) = delete;
    world& operator=(const world&) = delete;

    // Initialize with asset packs
    bool initialize(pak_file& terrain_pak, pak_file& object_pak);
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
    void set_camera_position(int32_t x, int32_t y);
    void set_camera_target(int32_t x, int32_t y);
    void center_camera_on(int32_t world_x, int32_t world_y);
    int32_t camera_x() const { return camera_x_; }
    int32_t camera_y() const { return camera_y_; }

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

    map current_map_;
    map_renderer map_renderer_;

    // Camera
    int32_t camera_x_ = 0;
    int32_t camera_y_ = 0;
    int32_t camera_target_x_ = 0;
    int32_t camera_target_y_ = 0;
    float camera_speed_ = 500.0f;  // Pixels per second for smooth scrolling
    bool camera_following_ = false;

    // World state
    weather_type weather_ = weather_type::clear;
    time_of_day time_ = time_of_day::noon;
    float weather_intensity_ = 0.0f;

    // Events
    world_events events_;
};

} // namespace hb
