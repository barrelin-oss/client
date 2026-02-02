#include "world/map_renderer.hpp"
#include "assets/tile_sprite_registry.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

bool map_renderer::initialize(tile_sprite_registry& registry)
{
    registry_ = &registry;
    spdlog::info("Map renderer initialized");
    return true;
}

void map_renderer::shutdown()
{
    registry_ = nullptr;
}

void map_renderer::render(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y)
{
    if (!m.is_loaded())
    {
        static bool warned = false;
        if (!warned)
        {
            spdlog::warn("map_renderer::render called but map is not loaded");
            warned = true;
        }
        return;
    }

    auto range = calculate_visible_range(m, camera_x, camera_y);

    // Layer 0: Terrain
    if (config_.show_terrain)
    {
        render_layer(rend, m, camera_x, camera_y, 0);
    }

    // Layer 1: Objects (trees, rocks, etc.)
    if (config_.show_objects)
    {
        render_layer(rend, m, camera_x, camera_y, 1);
    }

    // Debug: Show grid
    if (config_.show_grid)
    {
        for (int32_t y = range.start_y; y < range.end_y; ++y)
        {
            for (int32_t x = range.start_x; x < range.end_x; ++x)
            {
                auto [sx, sy] = tile_to_screen(x, y, camera_x, camera_y);
                rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(100, 100, 100, 50), false);
            }
        }
    }

    // Debug: Show walkability
    if (config_.show_walkability)
    {
        for (int32_t y = range.start_y; y < range.end_y; ++y)
        {
            for (int32_t x = range.start_x; x < range.end_x; ++x)
            {
                const auto& t = m.get_tile(x, y);
                auto [sx, sy] = tile_to_screen(x, y, camera_x, camera_y);

                if (!t.is_walkable())
                {
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(255, 0, 0, 100), true);
                }
                else if (t.is_occupied())
                {
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(255, 255, 0, 100), true);
                }
            }
        }
    }
}

void map_renderer::render_layer(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y, int layer)
{
    auto range = calculate_visible_range(m, camera_x, camera_y);

    static bool logged_once = false;
    if (!logged_once && layer == 0)
    {
        spdlog::info("map_renderer: rendering layer 0, visible range ({},{}) to ({},{})",
                     range.start_x, range.start_y, range.end_x, range.end_y);
        // Log first few terrain IDs
        for (int32_t y = range.start_y; y < std::min(range.start_y + 3, range.end_y); ++y)
        {
            for (int32_t x = range.start_x; x < std::min(range.start_x + 3, range.end_x); ++x)
            {
                const auto& t = m.get_tile(x, y);
                spdlog::info("  Tile ({},{}) terrain_id={} object_id={}", x, y, t.terrain_id, t.object_id);
            }
        }
        logged_once = true;
    }

    for (int32_t y = range.start_y; y < range.end_y; ++y)
    {
        for (int32_t x = range.start_x; x < range.end_x; ++x)
        {
            const auto& t = m.get_tile(x, y);
            auto [sx, sy] = tile_to_screen(x, y, camera_x, camera_y);

            if (layer == 0)
            {
                // Terrain layer - use no color key (solid background tiles)
                if (t.terrain_id != 0)
                {
                    const sprite* spr = get_tile_sprite(t.terrain_id);
                    if (spr)
                    {
                        // Use terrain_frame for animation frame
                        uint32_t frame = static_cast<uint32_t>(t.terrain_frame);

                        // Debug: log first sprite render
                        static bool logged_render = false;
                        if (!logged_render)
                        {
                            spdlog::info("Rendering first tile: id={} frame={} at screen ({},{}) sprite has {} frames, bitmap {}x{}",
                                         t.terrain_id, frame, sx, sy, spr->frame_count(),
                                         spr->bitmap_width(), spr->bitmap_height());
                            logged_render = true;
                        }

                        // Apply lighting
                        float alpha = config_.light_level * (t.light_level / 255.0f);
                        if (alpha >= 1.0f)
                        {
                            rend.draw_sprite_no_color_key(*spr, sx, sy, frame);
                        }
                        else
                        {
                            rend.draw_sprite_alpha_no_color_key(*spr, sx, sy, frame, alpha);
                        }
                    }
                }
            }
            else if (layer == 1)
            {
                // Object layer
                if (t.object_id != 0)
                {
                    const sprite* spr = get_tile_sprite(t.object_id);
                    if (spr)
                    {
                        rend.draw_sprite(*spr, sx, sy, 0);
                    }
                }
            }
        }
    }
}

const sprite* map_renderer::get_tile_sprite(int16_t id)
{
    if (!registry_)
    {
        static bool warned = false;
        if (!warned)
        {
            spdlog::error("map_renderer::get_tile_sprite called but registry is null!");
            warned = true;
        }
        return nullptr;
    }
    return registry_->get_sprite(id);
}

map_renderer::visible_range map_renderer::calculate_visible_range(const map& m, int32_t camera_x, int32_t camera_y) const
{
    visible_range range;

    // Calculate visible tile range with padding
    range.start_x = std::max(0, (camera_x / tile_width) - 1);
    range.start_y = std::max(0, (camera_y / tile_height) - 1);
    range.end_x = std::min(m.width(), range.start_x + visible_tiles_x + 2);
    range.end_y = std::min(m.height(), range.start_y + visible_tiles_y + 2);

    return range;
}

std::pair<int32_t, int32_t> map_renderer::screen_to_tile(int32_t screen_x, int32_t screen_y,
                                                          int32_t camera_x, int32_t camera_y) const
{
    int32_t world_x = screen_x + camera_x;
    int32_t world_y = screen_y + camera_y;
    return {world_x / tile_width, world_y / tile_height};
}

std::pair<int32_t, int32_t> map_renderer::tile_to_screen(int32_t tile_x, int32_t tile_y,
                                                          int32_t camera_x, int32_t camera_y) const
{
    return {tile_x * tile_width - camera_x, tile_y * tile_height - camera_y};
}

} // namespace hb
