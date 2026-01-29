#include "world/world.hpp"
#include "graphics/renderer.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>

namespace hb {

bool world::initialize(pak_file& terrain_pak, pak_file& object_pak) {
    if (!map_renderer_.initialize(terrain_pak, object_pak)) {
        spdlog::error("Failed to initialize map renderer");
        return false;
    }

    spdlog::info("World system initialized");
    return true;
}

void world::shutdown() {
    unload_map();
    map_renderer_.shutdown();
    spdlog::info("World system shutdown");
}

void world::update(float delta_time) {
    update_camera(delta_time);
    update_lighting();
}

void world::render(renderer& rend) {
    map_renderer_.render(rend, current_map_, camera_x_, camera_y_);

    // Render weather effects
    if (weather_ != weather_type::clear && weather_intensity_ > 0.0f) {
        // TODO: Render rain/snow particles
    }
}

bool world::load_map(std::string_view map_name) {
    std::string path = "maps/" + std::string(map_name) + ".map";

    std::string old_name(current_map_.name());

    if (!current_map_.load(path)) {
        spdlog::error("Failed to load map: {}", map_name);
        return false;
    }

    if (events_.on_map_changed) {
        events_.on_map_changed(old_name, map_name);
    }

    return true;
}

bool world::load_map_data(const uint8_t* data, size_t size, std::string_view name) {
    std::string old_name(current_map_.name());

    if (!current_map_.load_from_memory(data, size)) {
        spdlog::error("Failed to load map data: {}", name);
        return false;
    }

    if (events_.on_map_changed) {
        events_.on_map_changed(old_name, name);
    }

    return true;
}

void world::unload_map() {
    current_map_.clear();
}

void world::set_camera_position(int32_t x, int32_t y) {
    camera_x_ = x;
    camera_y_ = y;
    camera_target_x_ = x;
    camera_target_y_ = y;
    camera_following_ = false;
}

void world::set_camera_target(int32_t x, int32_t y) {
    camera_target_x_ = x;
    camera_target_y_ = y;
    camera_following_ = true;
}

void world::center_camera_on(int32_t world_x, int32_t world_y) {
    // Center the camera on a world position
    int32_t target_x = world_x - static_cast<int32_t>(screen_width / 2);
    int32_t target_y = world_y - static_cast<int32_t>(screen_height / 2);

    // Clamp to map bounds
    if (current_map_.is_loaded()) {
        int32_t max_x = current_map_.width() * tile_width - static_cast<int32_t>(screen_width);
        int32_t max_y = current_map_.height() * tile_height - static_cast<int32_t>(screen_height);
        target_x = std::clamp(target_x, 0, std::max(0, max_x));
        target_y = std::clamp(target_y, 0, std::max(0, max_y));
    }

    set_camera_target(target_x, target_y);
}

void world::set_weather(weather_type weather) {
    if (weather_ != weather) {
        weather_ = weather;
        if (events_.on_weather_changed) {
            events_.on_weather_changed(weather);
        }
    }
}

void world::set_time(time_of_day time) {
    if (time_ != time) {
        time_ = time;
        if (events_.on_time_changed) {
            events_.on_time_changed(time);
        }
    }
}

std::pair<int32_t, int32_t> world::screen_to_tile(int32_t screen_x, int32_t screen_y) const {
    return map_renderer_.screen_to_tile(screen_x, screen_y, camera_x_, camera_y_);
}

std::pair<int32_t, int32_t> world::screen_to_world(int32_t screen_x, int32_t screen_y) const {
    return {screen_x + camera_x_, screen_y + camera_y_};
}

std::pair<int32_t, int32_t> world::world_to_screen(int32_t world_x, int32_t world_y) const {
    return {world_x - camera_x_, world_y - camera_y_};
}

void world::update_camera(float delta_time) {
    if (!camera_following_) {
        return;
    }

    // Smooth camera movement
    float dx = static_cast<float>(camera_target_x_ - camera_x_);
    float dy = static_cast<float>(camera_target_y_ - camera_y_);
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f) {
        camera_x_ = camera_target_x_;
        camera_y_ = camera_target_y_;
        return;
    }

    float move = camera_speed_ * delta_time;
    if (move > dist) {
        move = dist;
    }

    camera_x_ += static_cast<int32_t>((dx / dist) * move);
    camera_y_ += static_cast<int32_t>((dy / dist) * move);
}

void world::update_lighting() {
    // Update light level based on time of day
    float light_level = 1.0f;

    switch (time_) {
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
    if (weather_ == weather_type::rain || weather_ == weather_type::storm) {
        light_level *= 0.8f;
    }

    auto config = map_renderer_.config();
    config.light_level = light_level;
    map_renderer_.set_config(config);
}

} // namespace hb
