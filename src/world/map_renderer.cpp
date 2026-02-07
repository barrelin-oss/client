#include "world/map_renderer.hpp"
#include "assets/tile_sprite_registry.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace hb {

bool map_renderer::initialize(tile_sprite_registry& registry)
{
    registry_ = &registry;
    spdlog::info("Map renderer initialized");
    return true;
}

void map_renderer::shutdown()
{
    chunks_.clear();
    registry_ = nullptr;
}

void map_renderer::invalidate_chunks()
{
    for (auto& [key, chunk] : chunks_)
    {
        chunk.valid = false;
    }
}


map_renderer::chunk_data& map_renderer::get_or_create_chunk(int32_t chunk_x, int32_t chunk_y)
{
    chunk_key key{chunk_x, chunk_y};
    auto it = chunks_.find(key);
    if (it != chunks_.end())
    {
        return it->second;
    }

    // Create new chunk
    auto& chunk = chunks_[key];
    chunk.texture = std::make_unique<sf::RenderTexture>();

    uint32_t chunk_pixel_size = chunk_size_ * tile_width;
    if (!chunk.texture->resize({chunk_pixel_size, chunk_pixel_size}))
    {
        spdlog::error("Failed to create chunk texture {}x{}", chunk_pixel_size, chunk_pixel_size);
        chunk.texture.reset();
    }

    return chunk;
}

void map_renderer::render_chunk(const map& m, int32_t chunk_x, int32_t chunk_y)
{
    auto& chunk = get_or_create_chunk(chunk_x, chunk_y);
    if (!chunk.texture) return;

    chunk.texture->clear(sf::Color::Transparent);

    int32_t start_tile_x = chunk_x * chunk_size_;
    int32_t start_tile_y = chunk_y * chunk_size_;
    int32_t end_tile_x = std::min(start_tile_x + chunk_size_, m.width());
    int32_t end_tile_y = std::min(start_tile_y + chunk_size_, m.height());

    // Render terrain layer ONLY - objects are rendered separately for proper layering
    if (config_.show_terrain)
    {
        for (int32_t y = start_tile_y; y < end_tile_y; ++y)
        {
            for (int32_t x = start_tile_x; x < end_tile_x; ++x)
            {
                const auto& t = m.get_tile(x, y);
                // Note: terrain_id=0 is valid (first sprite in maptiles1.pak)
                const sprite* spr = get_tile_sprite(t.terrain_id);
                if (spr)
                {
                    uint32_t frame = static_cast<uint32_t>(t.terrain_frame);
                    if (frame >= spr->frame_count())
                    {
                        continue;  // Skip if frame index is out of range
                    }

                    int32_t local_x = (x - start_tile_x) * tile_width;
                    int32_t local_y = (y - start_tile_y) * tile_height;

                    float alpha = config_.light_level * (t.light_level / 255.0f);
                    if (alpha >= 1.0f)
                    {
                        spr->draw_no_color_key(*chunk.texture, local_x, local_y, frame);
                    }
                    else
                    {
                        spr->draw_alpha_no_color_key(*chunk.texture, local_x, local_y, frame, alpha);
                    }
                }
            }
        }
    }

    chunk.texture->display();
    chunk.valid = true;
}

void map_renderer::render(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y)
{
    // Full render: terrain + all objects + debug overlays
    render_terrain(rend, m, camera_x, camera_y);

    if (config_.show_objects)
    {
        auto range = calculate_visible_range(m, camera_x, camera_y);
        for (int32_t y = range.start_y; y < range.end_y; ++y)
        {
            render_objects_row(rend, m, y, camera_x, camera_y);
        }
    }
}

void map_renderer::render_terrain(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y)
{
    if (!m.is_loaded())
    {
        static bool warned = false;
        if (!warned)
        {
            spdlog::warn("map_renderer::render_terrain called but map is not loaded");
            warned = true;
        }
        return;
    }

    auto range = calculate_visible_range(m, camera_x, camera_y);

    // Chunk-based terrain rendering at all zoom levels
    if (config_.show_terrain)
    {
        int32_t chunk_start_x = range.start_x / chunk_size_;
        int32_t chunk_start_y = range.start_y / chunk_size_;
        int32_t chunk_end_x = (range.end_x + chunk_size_ - 1) / chunk_size_;
        int32_t chunk_end_y = (range.end_y + chunk_size_ - 1) / chunk_size_;

        for (int32_t cy = chunk_start_y; cy <= chunk_end_y; ++cy)
        {
            for (int32_t cx = chunk_start_x; cx <= chunk_end_x; ++cx)
            {
                auto& chunk = get_or_create_chunk(cx, cy);

                if (!chunk.valid)
                {
                    render_chunk(m, cx, cy);
                }

                if (chunk.texture && chunk.valid)
                {
                    int32_t world_x = cx * chunk_size_ * tile_width;
                    int32_t world_y = cy * chunk_size_ * tile_height;
                    int32_t screen_x = world_x - camera_x;
                    int32_t screen_y = world_y - camera_y;

                    rend.draw_texture(chunk.texture->getTexture(), screen_x, screen_y);
                }
            }
        }
    }

    // Debug overlays (always drawn directly)
    if (config_.show_grid)
    {
        constexpr int32_t line_thickness = 2;
        sf::Color grid_color(0, 0, 0, 200);  // Black with high visibility

        // Draw horizontal lines
        for (int32_t y = range.start_y; y <= range.end_y; ++y)
        {
            auto [sx_start, sy] = tile_to_screen(range.start_x, y, camera_x, camera_y);
            auto [sx_end, sy2] = tile_to_screen(range.end_x, y, camera_x, camera_y);
            rend.draw_rect(sx_start, sy, sx_end - sx_start, line_thickness, grid_color, true);
        }

        // Draw vertical lines
        for (int32_t x = range.start_x; x <= range.end_x; ++x)
        {
            auto [sx, sy_start] = tile_to_screen(x, range.start_y, camera_x, camera_y);
            auto [sx2, sy_end] = tile_to_screen(x, range.end_y, camera_x, camera_y);
            rend.draw_rect(sx, sy_start, line_thickness, sy_end - sy_start, grid_color, true);
        }
    }

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
                    // Red tint for blocked/non-walkable tiles
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(255, 0, 0, 100), true);
                }
                else if (t.is_teleport())
                {
                    // Green tint for teleport tiles
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(0, 255, 0, 100), true);
                }
                else if (t.is_occupied())
                {
                    // Yellow tint for occupied tiles
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(255, 255, 0, 100), true);
                }
            }
        }

        // Pathfinding trace overlay (cyan)
        for (const auto& [tx, ty] : pathfinding_trace_)
        {
            if (tx >= range.start_x && tx < range.end_x &&
                ty >= range.start_y && ty < range.end_y)
            {
                auto [sx, sy] = tile_to_screen(tx, ty, camera_x, camera_y);
                rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(0, 255, 255, 100), true);
            }
        }
    }
}

void map_renderer::render_objects_row(renderer& rend, const map& m, int32_t row_y, int32_t camera_x, int32_t camera_y)
{
    if (!config_.show_objects) return;
    if (row_y < 0 || row_y >= m.height()) return;

    auto range = calculate_visible_range(m, camera_x, camera_y);
    int32_t start_x = std::max(range.start_x, 0);
    int32_t end_x = std::min(range.end_x, m.width());

    for (int32_t x = start_x; x < end_x; ++x)
    {
        const auto& t = m.get_tile(x, row_y);
        if (t.object_id != 0)
        {
            auto [sx, sy] = tile_to_screen(x, row_y, camera_x, camera_y);
            const sprite* spr = get_tile_sprite(t.object_id);
            if (spr)
            {
                rend.draw_sprite(*spr, sx, sy, 0);
            }
        }
    }
}

map_renderer::visible_range map_renderer::get_visible_range(const map& m, int32_t camera_x, int32_t camera_y) const
{
    return calculate_visible_range(m, camera_x, camera_y);
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

    // Calculate effective screen size based on zoom level
    // When zoomed out (zoom > 1), we see more tiles
    // When zoomed in (zoom < 1), we see fewer tiles
    float effective_width = static_cast<float>(screen_width_) * zoom_level_;
    float effective_height = static_cast<float>(screen_height_) * zoom_level_;

    // Calculate visible tiles based on effective dimensions
    int32_t tiles_x = static_cast<int32_t>(effective_width / tile_width) + 2;
    int32_t tiles_y = static_cast<int32_t>(effective_height / tile_height) + 2;

    // Extra padding for zoom (more padding when zoomed out to ensure coverage)
    int32_t zoom_padding = static_cast<int32_t>(std::ceil(zoom_level_)) + 2;

    // The view is centered at screen center, so camera represents the top-left of the unzoomed view
    // When zoomed, the visible area extends equally in all directions from screen center
    // We need to offset the start position to account for the centered view
    float half_extra_width = (effective_width - static_cast<float>(screen_width_)) / 2.0f;
    float half_extra_height = (effective_height - static_cast<float>(screen_height_)) / 2.0f;

    int32_t adjusted_camera_x = camera_x - static_cast<int32_t>(half_extra_width);
    int32_t adjusted_camera_y = camera_y - static_cast<int32_t>(half_extra_height);

    // Calculate base start position from adjusted camera
    int32_t base_start_x = adjusted_camera_x / tile_width;
    int32_t base_start_y = adjusted_camera_y / tile_height;

    // Calculate visible tile range with padding
    range.start_x = std::max(0, base_start_x - zoom_padding);
    range.start_y = std::max(0, base_start_y - zoom_padding);
    range.end_x = std::min(m.width(), base_start_x + tiles_x + zoom_padding * 2);
    range.end_y = std::min(m.height(), base_start_y + tiles_y + zoom_padding * 2);

    return range;
}

void map_renderer::set_screen_size(uint32_t width, uint32_t height)
{
    screen_width_ = width;
    screen_height_ = height;
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
