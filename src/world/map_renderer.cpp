#include "world/map_renderer.hpp"
#include "assets/pak_file.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

bool map_renderer::initialize(pak_file& terrain_pak, pak_file& object_pak) {
    terrain_pak_ = &terrain_pak;
    object_pak_ = &object_pak;

    spdlog::info("Map renderer initialized");
    return true;
}

void map_renderer::shutdown() {
    terrain_sprites_.clear();
    object_sprites_.clear();
    terrain_pak_ = nullptr;
    object_pak_ = nullptr;
}

void map_renderer::render(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y) {
    if (!m.is_loaded()) {
        return;
    }

    auto range = calculate_visible_range(m, camera_x, camera_y);

    // Layer 0: Terrain
    if (config_.show_terrain) {
        render_layer(rend, m, camera_x, camera_y, 0);
    }

    // Layer 1: Objects (trees, rocks, etc.)
    if (config_.show_objects) {
        render_layer(rend, m, camera_x, camera_y, 1);
    }

    // Debug: Show grid
    if (config_.show_grid) {
        for (int32_t y = range.start_y; y < range.end_y; ++y) {
            for (int32_t x = range.start_x; x < range.end_x; ++x) {
                auto [sx, sy] = tile_to_screen(x, y, camera_x, camera_y);
                rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(100, 100, 100, 50), false);
            }
        }
    }

    // Debug: Show walkability
    if (config_.show_walkability) {
        for (int32_t y = range.start_y; y < range.end_y; ++y) {
            for (int32_t x = range.start_x; x < range.end_x; ++x) {
                const auto& t = m.get_tile(x, y);
                auto [sx, sy] = tile_to_screen(x, y, camera_x, camera_y);

                if (!t.is_walkable()) {
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(255, 0, 0, 100), true);
                } else if (t.is_occupied()) {
                    rend.draw_rect(sx, sy, tile_width, tile_height, sf::Color(255, 255, 0, 100), true);
                }
            }
        }
    }
}

void map_renderer::render_layer(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y, int layer) {
    auto range = calculate_visible_range(m, camera_x, camera_y);

    for (int32_t y = range.start_y; y < range.end_y; ++y) {
        for (int32_t x = range.start_x; x < range.end_x; ++x) {
            const auto& t = m.get_tile(x, y);
            auto [sx, sy] = tile_to_screen(x, y, camera_x, camera_y);

            if (layer == 0) {
                // Terrain layer
                if (t.terrain_id != 0) {
                    const sprite* spr = get_terrain_sprite(t.terrain_id);
                    if (spr && spr->is_loaded()) {
                        // Apply lighting
                        float alpha = config_.light_level * (t.light_level / 255.0f);
                        if (alpha >= 1.0f) {
                            rend.draw_sprite(*spr, sx, sy, 0);
                        } else {
                            rend.draw_sprite_alpha(*spr, sx, sy, 0, alpha);
                        }
                    }
                }
            } else if (layer == 1) {
                // Object layer
                if (t.object_id != 0) {
                    const sprite* spr = get_object_sprite(t.object_id);
                    if (spr && spr->is_loaded()) {
                        rend.draw_sprite(*spr, sx, sy, 0);
                    }
                }
            }
        }
    }
}

const sprite* map_renderer::get_terrain_sprite(uint16_t id) {
    auto it = terrain_sprites_.find(id);
    if (it != terrain_sprites_.end()) {
        return it->second.get();
    }

    // Try to load from PAK
    if (terrain_pak_ && terrain_pak_->is_open()) {
        auto spr = std::make_unique<sprite>();
        if (spr->load_from_pak(*terrain_pak_, id)) {
            const sprite* ptr = spr.get();
            terrain_sprites_[id] = std::move(spr);
            return ptr;
        }
    }

    // Mark as failed to load (nullptr)
    terrain_sprites_[id] = nullptr;
    return nullptr;
}

const sprite* map_renderer::get_object_sprite(uint16_t id) {
    auto it = object_sprites_.find(id);
    if (it != object_sprites_.end()) {
        return it->second.get();
    }

    // Try to load from PAK
    if (object_pak_ && object_pak_->is_open()) {
        auto spr = std::make_unique<sprite>();
        if (spr->load_from_pak(*object_pak_, id)) {
            const sprite* ptr = spr.get();
            object_sprites_[id] = std::move(spr);
            return ptr;
        }
    }

    // Mark as failed to load
    object_sprites_[id] = nullptr;
    return nullptr;
}

map_renderer::visible_range map_renderer::calculate_visible_range(const map& m, int32_t camera_x, int32_t camera_y) const {
    visible_range range;

    // Calculate visible tile range with padding
    range.start_x = std::max(0, (camera_x / tile_width) - 1);
    range.start_y = std::max(0, (camera_y / tile_height) - 1);
    range.end_x = std::min(m.width(), range.start_x + visible_tiles_x + 2);
    range.end_y = std::min(m.height(), range.start_y + visible_tiles_y + 2);

    return range;
}

std::pair<int32_t, int32_t> map_renderer::screen_to_tile(int32_t screen_x, int32_t screen_y,
                                                          int32_t camera_x, int32_t camera_y) const {
    int32_t world_x = screen_x + camera_x;
    int32_t world_y = screen_y + camera_y;
    return {world_x / tile_width, world_y / tile_height};
}

std::pair<int32_t, int32_t> map_renderer::tile_to_screen(int32_t tile_x, int32_t tile_y,
                                                          int32_t camera_x, int32_t camera_y) const {
    return {tile_x * tile_width - camera_x, tile_y * tile_height - camera_y};
}

} // namespace hb
