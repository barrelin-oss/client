#include "world/world.hpp"
#include "graphics/renderer.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>

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
    update_clock(delta_time);
    update_lighting();
    update_tint(delta_time);

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

    map_renderer_.render(rend, current_map_, static_cast<int32_t>(camera_x_), static_cast<int32_t>(camera_y_));

    // Weather particles and day/night overlay are rendered by weather_system
    // in game_state_manager::render_playing() after the scene is drawn
}

void world::render_terrain(renderer& rend)
{
    map_renderer_.set_zoom_level(zoom_mode_enabled_ ? static_cast<float>(zoom_level_) : 1.0f);
    map_renderer_.render_terrain(rend, current_map_, static_cast<int32_t>(camera_x_), static_cast<int32_t>(camera_y_));
}

void world::render_objects_row(renderer& rend, int32_t row_y, const sf::IntRect* player_bounds)
{
    map_renderer_.render_objects_row(rend, current_map_, row_y, static_cast<int32_t>(camera_x_), static_cast<int32_t>(camera_y_), player_bounds);
}

map_renderer::visible_range world::get_visible_tile_range() const
{
    return map_renderer_.get_visible_range(current_map_, static_cast<int32_t>(camera_x_), static_cast<int32_t>(camera_y_));
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

// Cached directory listing for case-insensitive AMD file lookups on Linux.
// Maps lowercase_filename -> actual_path within the mapdata directory.
static std::unordered_map<std::string, std::string> amd_file_cache;
static bool amd_cache_built = false;

static std::string find_amd_case_insensitive(const std::string& dir, const std::string& lowercase_filename)
{
    if (!amd_cache_built)
    {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
        {
            auto name = entry.path().filename().string();
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            amd_file_cache[lower] = entry.path().string();
        }
        amd_cache_built = true;
    }

    auto it = amd_file_cache.find(lowercase_filename);
    if (it != amd_file_cache.end())
    {
        return it->second;
    }
    return {};
}

bool world::load_map(std::string_view map_name)
{
    // Convert map name to lowercase for file lookup
    std::string name_lower(map_name);
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // AMD files are in assets/data/mapdata/ directory relative to the binary
    std::string mapdata_dir = "./assets/data/mapdata";
    std::string path = mapdata_dir + "/" + name_lower + ".amd";

    // Case-insensitive fallback (handles ARESDEN.AMD vs aresden.amd on Linux)
    if (!std::filesystem::exists(path))
    {
        auto resolved = find_amd_case_insensitive(mapdata_dir, name_lower + ".amd");
        if (!resolved.empty())
        {
            path = resolved;
            spdlog::debug("AMD case-insensitive match: {}.amd -> {}", name_lower,
                         std::filesystem::path(resolved).filename().string());
        }
    }

    std::string old_name(current_map_.name());

    if (!current_map_.load(path))
    {
        spdlog::error("Failed to load map: {} (path: {})", map_name, path);
        // Set name so we know what map we're supposed to be on, but clear tile data
        // so is_loaded() returns false and we can retry later
        current_map_.clear();
        current_map_.set_name(map_name);
        return false;
    }

    // Set name after successful load
    current_map_.set_name(map_name);

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

void world::set_clock(uint8_t hour, uint8_t minute)
{
    clock_hour_ = hour;
    clock_minute_ = minute;
    clock_accumulator_ = 0.0f;  // Reset accumulator on server sync
}

static time_of_day hour_to_time_of_day(uint8_t hour)
{
    if (hour < 5)  return time_of_day::midnight;
    if (hour < 7)  return time_of_day::dawn;
    if (hour < 10) return time_of_day::morning;
    if (hour < 14) return time_of_day::noon;
    if (hour < 17) return time_of_day::afternoon;
    if (hour < 19) return time_of_day::dusk;
    if (hour < 23) return time_of_day::night;
    return time_of_day::midnight;
}

void world::update_clock(float delta_time)
{
    // Server game clock: 1 real second = 1 game minute (60x speed)
    clock_accumulator_ += delta_time;
    if (clock_accumulator_ < 1.0f) return;

    // Advance by whole minutes
    auto minutes = static_cast<int>(clock_accumulator_);
    clock_accumulator_ -= static_cast<float>(minutes);

    clock_minute_ += static_cast<uint8_t>(minutes);
    while (clock_minute_ >= 60)
    {
        clock_minute_ -= 60;
        clock_hour_ = (clock_hour_ + 1) % 24;

        // Recompute time_of_day when the hour changes
        auto new_tod = hour_to_time_of_day(clock_hour_);
        if (new_tod != time_)
        {
            set_time(new_tod);
        }
    }
}

void world::set_time(time_of_day time)
{
    if (time_ != time)
    {
        time_ = time;
        target_tint_ = tint_for_time(time);
        if (events_.on_time_changed)
        {
            events_.on_time_changed(time);
        }
    }
}

std::pair<int32_t, int32_t> world::screen_to_tile(int32_t screen_x, int32_t screen_y) const
{
    auto [wx, wy] = screen_to_world(screen_x, screen_y);
    return {wx / tile_width, wy / tile_height};
}

std::pair<int32_t, int32_t> world::screen_to_world(int32_t screen_x, int32_t screen_y) const
{
    // When zoomed, the SFML view scales around screen center.
    // A screen pixel at offset (dx, dy) from center maps to (dx * zoom, dy * zoom) in view space.
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        double cx = static_cast<double>(screen_width_) / 2.0;
        double cy = static_cast<double>(screen_height_) / 2.0;
        return {
            static_cast<int32_t>(camera_x_ + cx + (static_cast<double>(screen_x) - cx) * zoom_level_),
            static_cast<int32_t>(camera_y_ + cy + (static_cast<double>(screen_y) - cy) * zoom_level_)
        };
    }
    return {screen_x + static_cast<int32_t>(camera_x_), screen_y + static_cast<int32_t>(camera_y_)};
}

std::pair<int32_t, int32_t> world::world_to_screen(int32_t world_x, int32_t world_y) const
{
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        double cx = static_cast<double>(screen_width_) / 2.0;
        double cy = static_cast<double>(screen_height_) / 2.0;
        return {
            static_cast<int32_t>(cx + (static_cast<double>(world_x) - camera_x_ - cx) / zoom_level_),
            static_cast<int32_t>(cy + (static_cast<double>(world_y) - camera_y_ - cy) / zoom_level_)
        };
    }
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
    // Day/night is handled by the tint overlay (render_overlay).
    // The map renderer light_level only applies per-tile light variation
    // (e.g. torchlight); keep it at full brightness for the global level.
    auto config = map_renderer_.config();
    config.light_level = 1.0f;
    map_renderer_.set_config(config);
}

void world::update_tint(float delta_time)
{
    // Transition speed: ~2 seconds to fully change
    float t = std::min(delta_time * 0.5f, 1.0f);
    current_tint_r_ += (static_cast<float>(target_tint_.r) - current_tint_r_) * t;
    current_tint_g_ += (static_cast<float>(target_tint_.g) - current_tint_g_) * t;
    current_tint_b_ += (static_cast<float>(target_tint_.b) - current_tint_b_) * t;
    current_tint_a_ += (static_cast<float>(target_tint_.a) - current_tint_a_) * t;
}

void world::render_overlay(renderer& rend)
{
    if (!tint_visible_) return;
    auto a = static_cast<uint8_t>(std::clamp(current_tint_a_, 0.0f, 255.0f));
    if (a == 0) return;

    sf::Color tint(
        static_cast<uint8_t>(std::clamp(current_tint_r_, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(current_tint_g_, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(current_tint_b_, 0.0f, 255.0f)),
        a);
    rend.draw_rect(0, 0,
        static_cast<int32_t>(rend.scene_width()),
        static_cast<int32_t>(rend.scene_height()),
        tint, true);
}

tint_color world::tint_for_time(time_of_day t)
{
    switch (t)
    {
        case time_of_day::dawn:
            return {200, 140, 80, 35};       // Warm orange
        case time_of_day::morning:
            return {0, 0, 0, 0};             // No tint
        case time_of_day::noon:
            return {0, 0, 0, 0};             // No tint
        case time_of_day::afternoon:
            return {200, 160, 100, 15};      // Warm
        case time_of_day::dusk:
            return {160, 60, 40, 55};        // Orange-red
        case time_of_day::night:
            return {10, 10, 50, 110};        // Dark blue
        case time_of_day::midnight:
            return {5, 5, 30, 140};          // Deep blue
    }
    return {0, 0, 0, 0};
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
