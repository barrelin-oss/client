#include "world/world.hpp"
#include "graphics/renderer.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>
#include <cctype>

namespace hb {

bool world::initialize(tile_sprite_registry& registry)
{
    if (!map_renderer_.initialize(registry))
    {
        spdlog::error("Failed to initialize map renderer");
        return false;
    }

    spdlog::info("World system initialized");
    return true;
}

void world::shutdown()
{
    unload_map();
    map_renderer_.shutdown();
    spdlog::info("World system shutdown");
}

void world::update(float delta_time)
{
    update_camera(delta_time);
    update_lighting();
}

void world::render(renderer& rend)
{
    map_renderer_.render(rend, current_map_, camera_x_, camera_y_);

    // Render weather effects
    if (weather_ != weather_type::clear && weather_intensity_ > 0.0f)
    {
        // TODO: Render rain/snow particles
    }
}

bool world::load_map(std::string_view map_name)
{
    // Convert map name to lowercase (AMD files are all lowercase)
    std::string name_lower(map_name);
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // AMD files are in assets/data/mapdata/ directory relative to the binary
    std::string path = "./assets/data/mapdata/" + name_lower + ".amd";

    std::string old_name(current_map_.name());

    // Set the name first (in case the file doesn't include it)
    current_map_.set_name(map_name);

    if (!current_map_.load(path))
    {
        spdlog::error("Failed to load map: {} (path: {})", map_name, path);
        return false;
    }

    if (events_.on_map_changed)
    {
        events_.on_map_changed(old_name, map_name);
    }

    return true;
}

bool world::load_map_data(const uint8_t* data, size_t size, std::string_view name)
{
    std::string old_name(current_map_.name());

    if (!current_map_.load_from_memory(data, size))
    {
        spdlog::error("Failed to load map data: {}", name);
        return false;
    }

    if (events_.on_map_changed)
    {
        events_.on_map_changed(old_name, name);
    }

    return true;
}

void world::unload_map()
{
    current_map_.clear();
}

void world::set_player_position(int32_t world_x, int32_t world_y)
{
    player_world_x_ = world_x;
    player_world_y_ = world_y;

    // If not in cinematic mode, snap camera to player
    if (!cinematic_mode_)
    {
        center_on_player();
    }
}

void world::center_on_player()
{
    // Center the camera on player position
    int32_t target_x = player_world_x_ - static_cast<int32_t>(screen_width / 2);
    int32_t target_y = player_world_y_ - static_cast<int32_t>(screen_height / 2);

    // Clamp to map bounds
    if (current_map_.is_loaded())
    {
        int32_t max_x = current_map_.width() * tile_width - static_cast<int32_t>(screen_width);
        int32_t max_y = current_map_.height() * tile_height - static_cast<int32_t>(screen_height);
        target_x = std::clamp(target_x, 0, std::max(0, max_x));
        target_y = std::clamp(target_y, 0, std::max(0, max_y));
    }

    camera_x_ = target_x;
    camera_y_ = target_y;
}

void world::add_camera_shake(float intensity, float duration)
{
    shake_intensity_ = intensity;
    shake_duration_ = duration;
    shake_timer_ = 0.0f;
}

void world::set_cinematic_mode(bool enabled)
{
    cinematic_mode_ = enabled;

    // When exiting cinematic mode, snap back to player
    if (!enabled)
    {
        center_on_player();
    }
}

void world::set_camera_position(int32_t x, int32_t y)
{
    // Only allow direct camera control in cinematic mode
    if (cinematic_mode_)
    {
        camera_x_ = x;
        camera_y_ = y;
    }
}

void world::move_camera(int32_t dx, int32_t dy)
{
    // Only allow direct camera control in cinematic mode
    if (cinematic_mode_)
    {
        camera_x_ += dx;
        camera_y_ += dy;
    }
}

void world::set_weather(weather_type weather)
{
    if (weather_ != weather)
    {
        weather_ = weather;
        if (events_.on_weather_changed)
        {
            events_.on_weather_changed(weather);
        }
    }
}

void world::set_time(time_of_day time)
{
    if (time_ != time)
    {
        time_ = time;
        if (events_.on_time_changed)
        {
            events_.on_time_changed(time);
        }
    }
}

std::pair<int32_t, int32_t> world::screen_to_tile(int32_t screen_x, int32_t screen_y) const
{
    return map_renderer_.screen_to_tile(screen_x, screen_y, camera_x_, camera_y_);
}

std::pair<int32_t, int32_t> world::screen_to_world(int32_t screen_x, int32_t screen_y) const
{
    return {screen_x + camera_x_, screen_y + camera_y_};
}

std::pair<int32_t, int32_t> world::world_to_screen(int32_t world_x, int32_t world_y) const
{
    return {world_x - camera_x_, world_y - camera_y_};
}

void world::update_camera(float delta_time)
{
    // Update camera shake
    if (shake_duration_ > 0.0f)
    {
        shake_timer_ += delta_time;
        if (shake_timer_ >= shake_duration_)
        {
            // Shake finished
            shake_offset_x_ = 0;
            shake_offset_y_ = 0;
            shake_duration_ = 0.0f;
            shake_intensity_ = 0.0f;
        }
        else
        {
            // Calculate shake offset with decay
            float progress = shake_timer_ / shake_duration_;
            float current_intensity = shake_intensity_ * (1.0f - progress);

            // Random offset within intensity range
            shake_offset_x_ = static_cast<int32_t>((static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * current_intensity);
            shake_offset_y_ = static_cast<int32_t>((static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * current_intensity);
        }
    }

    // Camera follows player unless in cinematic mode
    // (player position is set externally via set_player_position)
}

void world::update_lighting()
{
    // Update light level based on time of day
    float light_level = 1.0f;

    switch (time_)
    {
        case time_of_day::dawn:
            light_level = 0.6f;
            break;
        case time_of_day::morning:
            light_level = 0.8f;
            break;
        case time_of_day::noon:
            light_level = 1.0f;
            break;
        case time_of_day::afternoon:
            light_level = 0.9f;
            break;
        case time_of_day::dusk:
            light_level = 0.5f;
            break;
        case time_of_day::night:
            light_level = 0.3f;
            break;
        case time_of_day::midnight:
            light_level = 0.2f;
            break;
    }

    // Weather affects lighting
    if (weather_ == weather_type::rain || weather_ == weather_type::storm)
    {
        light_level *= 0.8f;
    }

    auto config = map_renderer_.config();
    config.light_level = light_level;
    map_renderer_.set_config(config);
}

} // namespace hb
