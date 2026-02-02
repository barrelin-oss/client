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

    // Smooth zoom interpolation (time-based)
    if (zoom_mode_enabled_ && has_zoom_anchor_)
    {
        constexpr double zoom_speed = 10.0;
        double t = 1.0 - std::exp(-zoom_speed * static_cast<double>(delta_time));

        zoom_level_ += (zoom_target_ - zoom_level_) * t;

        double screen_center_x = static_cast<double>(screen_width_) / 2.0;
        double screen_center_y = static_cast<double>(screen_height_) / 2.0;

        // Camera position that keeps anchor_world at anchor_screen with current zoom
        camera_x_ = zoom_anchor_world_x_ - screen_center_x -
                    (zoom_anchor_screen_x_ - screen_center_x) * zoom_level_;
        camera_y_ = zoom_anchor_world_y_ - screen_center_y -
                    (zoom_anchor_screen_y_ - screen_center_y) * zoom_level_;

        // Clear anchor when close enough - no explicit snap to avoid jump
        if (std::abs(zoom_level_ - zoom_target_) < 0.001)
        {
            has_zoom_anchor_ = false;
        }
    }
}

void world::render(renderer& rend)
{
    // Update map renderer with current zoom level for proper tile culling
    map_renderer_.set_zoom_level(zoom_mode_enabled_ ? static_cast<float>(zoom_level_) : 1.0f);

    map_renderer_.render(rend, current_map_, camera_x_, camera_y_);

    // Render weather effects
    if (weather_ != weather_type::clear && weather_intensity_ > 0.0f)
    {
        // TODO: Render rain/snow particles
    }
}

void world::apply_zoom_view(renderer& rend)
{
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        float screen_center_x = static_cast<float>(screen_width_) / 2.0f;
        float screen_center_y = static_cast<float>(screen_height_) / 2.0f;

        // View is always centered at screen center - camera position handles panning
        rend.set_zoom_view(static_cast<float>(zoom_level_), screen_center_x, screen_center_y);
    }
}

void world::reset_zoom_view(renderer& rend)
{
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        rend.reset_to_default_view();
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
    // Center the camera on player position using current screen dimensions
    double target_x = static_cast<double>(player_world_x_) - static_cast<double>(screen_width_) / 2.0;
    double target_y = static_cast<double>(player_world_y_) - static_cast<double>(screen_height_) / 2.0;

    // Clamp to map bounds
    if (current_map_.is_loaded())
    {
        double max_x = static_cast<double>(current_map_.width() * tile_width) - static_cast<double>(screen_width_);
        double max_y = static_cast<double>(current_map_.height() * tile_height) - static_cast<double>(screen_height_);
        target_x = std::clamp(target_x, 0.0, std::max(0.0, max_x));
        target_y = std::clamp(target_y, 0.0, std::max(0.0, max_y));
    }

    camera_x_ = target_x;
    camera_y_ = target_y;
    has_zoom_anchor_ = false;  // Clear anchor when camera is repositioned
}

void world::set_screen_size(uint32_t width, uint32_t height)
{
    screen_width_ = width;
    screen_height_ = height;
    spdlog::debug("World screen size updated: {}x{}", width, height);

    // Update map renderer screen size for proper tile rendering
    map_renderer_.set_screen_size(width, height);

    // Re-center camera with new dimensions
    if (!cinematic_mode_)
    {
        center_on_player();
    }
}

void world::recenter_camera()
{
    if (!cinematic_mode_)
    {
        center_on_player();
    }
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
        camera_x_ = static_cast<double>(x);
        camera_y_ = static_cast<double>(y);
        has_zoom_anchor_ = false;
    }
}

void world::move_camera(int32_t dx, int32_t dy)
{
    // Only allow direct camera control in cinematic mode
    if (cinematic_mode_)
    {
        camera_x_ += static_cast<double>(dx);
        camera_y_ += static_cast<double>(dy);
        has_zoom_anchor_ = false;
    }
}

void world::start_drag(int32_t mouse_x, int32_t mouse_y)
{
    if (!cinematic_mode_) return;

    drag_active_ = true;
    drag_start_mouse_x_ = mouse_x;
    drag_start_mouse_y_ = mouse_y;
    drag_start_camera_x_ = static_cast<int32_t>(camera_x_);
    drag_start_camera_y_ = static_cast<int32_t>(camera_y_);
}

void world::update_drag(int32_t mouse_x, int32_t mouse_y)
{
    if (!drag_active_ || !cinematic_mode_) return;

    // Calculate delta from drag start (inverted - drag right moves camera left)
    double dx = static_cast<double>(drag_start_mouse_x_ - mouse_x);
    double dy = static_cast<double>(drag_start_mouse_y_ - mouse_y);

    // Apply zoom factor to drag distance (so dragging feels consistent at any zoom level)
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        dx *= zoom_level_;
        dy *= zoom_level_;
    }

    camera_x_ = static_cast<double>(drag_start_camera_x_) + dx;
    camera_y_ = static_cast<double>(drag_start_camera_y_) + dy;
    has_zoom_anchor_ = false;  // Clear anchor when panning
}

void world::end_drag()
{
    drag_active_ = false;
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
    return {screen_x + static_cast<int32_t>(camera_x_), screen_y + static_cast<int32_t>(camera_y_)};
}

std::pair<int32_t, int32_t> world::world_to_screen(int32_t world_x, int32_t world_y) const
{
    return {world_x - static_cast<int32_t>(camera_x_), world_y - static_cast<int32_t>(camera_y_)};
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

void world::set_zoom_mode_enabled(bool enabled)
{
    zoom_mode_enabled_ = enabled;
    if (!enabled)
    {
        // Reset zoom state
        zoom_level_ = 1.0;
        zoom_target_ = 1.0;
        has_zoom_anchor_ = false;
    }
}

void world::adjust_zoom(float delta, int32_t cursor_x, int32_t cursor_y)
{
    if (!zoom_mode_enabled_) return;

    // Zoom limits: 0.5 = 2x zoom in, 8.0 = 8x zoom out
    double new_zoom = std::clamp(zoom_target_ + static_cast<double>(delta), 0.5, 8.0);

    if (new_zoom == zoom_target_) return;

    // If cursor position provided, set up anchor to keep tile center under cursor stationary
    if (cursor_x >= 0 && cursor_y >= 0)
    {
        double screen_center_x = static_cast<double>(screen_width_) / 2.0;
        double screen_center_y = static_cast<double>(screen_height_) / 2.0;

        // Calculate world point currently under cursor
        double cursor_from_center_x = static_cast<double>(cursor_x) - screen_center_x;
        double cursor_from_center_y = static_cast<double>(cursor_y) - screen_center_y;

        double world_x = camera_x_ + screen_center_x + cursor_from_center_x * zoom_level_;
        double world_y = camera_y_ + screen_center_y + cursor_from_center_y * zoom_level_;

        // Snap to tile center for stability
        int32_t tile_x = static_cast<int32_t>(world_x) / tile_width;
        int32_t tile_y = static_cast<int32_t>(world_y) / tile_height;
        zoom_anchor_world_x_ = static_cast<double>(tile_x * tile_width + tile_width / 2);
        zoom_anchor_world_y_ = static_cast<double>(tile_y * tile_height + tile_height / 2);

        // Calculate where this tile center currently appears on screen
        // screen = screen_center + (world - camera - screen_center) / zoom
        zoom_anchor_screen_x_ = screen_center_x + (zoom_anchor_world_x_ - camera_x_ - screen_center_x) / zoom_level_;
        zoom_anchor_screen_y_ = screen_center_y + (zoom_anchor_world_y_ - camera_y_ - screen_center_y) / zoom_level_;

        has_zoom_anchor_ = true;
    }

    zoom_target_ = new_zoom;
}

} // namespace hb
