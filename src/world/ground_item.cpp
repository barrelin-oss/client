#include "world/ground_item.hpp"
#include "world/tile.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void ground_item_manager::add(ground_item item)
{
    // Remove any existing item at the same tile (one visible item per tile)
    for (auto it = items_.begin(); it != items_.end(); ++it)
    {
        if (it->second.tile_x == item.tile_x && it->second.tile_y == item.tile_y && it->first != item.item_id)
        {
            items_.erase(it);
            break;
        }
    }

    auto id = item.item_id;
    items_.insert_or_assign(id, std::move(item));
}

void ground_item_manager::remove(uint32_t item_id)
{
    items_.erase(item_id);
}

void ground_item_manager::clear()
{
    items_.clear();
}

ground_item* ground_item_manager::get(uint32_t item_id)
{
    auto it = items_.find(item_id);
    return it != items_.end() ? &it->second : nullptr;
}

ground_item* ground_item_manager::get_at_tile(int16_t tile_x, int16_t tile_y)
{
    for (auto& [id, item] : items_)
    {
        if (item.tile_x == tile_x && item.tile_y == tile_y)
            return &item;
    }
    return nullptr;
}

void ground_item_manager::render_sprites(
    renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y, uint16_t sprite_base)
{
    for (auto& [id, item] : items_)
    {
        if (item.ground_sprite <= 0)
            continue;

        uint16_t sprite_id = sprite_base + static_cast<uint16_t>(item.ground_sprite);
        auto* spr = sprites.get_sprite_by_id(sprite_id);
        if (!spr)
            continue;

        // Position at center of tile
        int32_t screen_x = item.tile_x * tile_width + tile_width / 2 - camera_x;
        int32_t screen_y = item.tile_y * tile_height + tile_height / 2 - camera_y;

        uint32_t frame = static_cast<uint32_t>(item.ground_sprite_frame);
        if (spr->frame_count() > 0 && frame >= spr->frame_count())
            frame = 0;

        rend.draw_sprite(*spr, screen_x, screen_y, frame);
    }
}

void ground_item_manager::render_labels(
    renderer& rend, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y)
{
    for (auto& [id, item] : items_)
    {
        int32_t world_x = item.tile_x * tile_width + 16;
        int32_t world_y = item.tile_y * tile_height + 16;
        int32_t screen_x = world_x - camera_x;
        int32_t screen_y = world_y - camera_y;

        // Only render labels for items near the mouse cursor (32x32 hit area)
        bool hovered = (mouse_x >= screen_x - 16 && mouse_x < screen_x + 16 && mouse_y >= screen_y - 16 &&
                        mouse_y < screen_y + 16);
        if (!hovered)
            continue;

        // Draw item name centered above the tile
        auto label = item.name;
        if (item.count > 1)
            label += " x" + std::to_string(item.count);

        rend.draw_text_outlined(label, screen_x, screen_y - 20, sf::Color(200, 200, 100), sf::Color::Black, 12, 1.0f);
    }
}

ground_item* ground_item_manager::hit_test(int32_t mouse_x, int32_t mouse_y, int32_t camera_x, int32_t camera_y)
{
    for (auto& [id, item] : items_)
    {
        int32_t world_x = item.tile_x * tile_width + 16;
        int32_t world_y = item.tile_y * tile_height + 16;
        int32_t screen_x = world_x - camera_x;
        int32_t screen_y = world_y - camera_y;

        if (mouse_x >= screen_x - 16 && mouse_x < screen_x + 16 && mouse_y >= screen_y - 16 && mouse_y < screen_y + 16)
        {
            return &item;
        }
    }
    return nullptr;
}

} // namespace hb
