#pragma once

#include "gameplay/item.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace hb
{

class renderer;
class sprite_manager;

struct ground_item
{
    item data;               // Full item object from server
    int16_t tile_x = 0;
    int16_t tile_y = 0;
    bool freshly_dropped = false;
};

class ground_item_manager
{
public:
    void add(ground_item item);
    void remove(uint32_t item_id);
    void clear();
    ground_item* get(uint32_t item_id);
    ground_item* get_at_tile(int16_t tile_x, int16_t tile_y);
    bool empty() const { return items_.empty(); }
    size_t size() const { return items_.size(); }

    void render_sprites(renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y,
                        std::string_view item_pak);
    void render_labels(renderer& rend, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y);

    ground_item* hit_test(int32_t mouse_x, int32_t mouse_y, int32_t camera_x, int32_t camera_y);

private:
    std::unordered_map<uint32_t, ground_item> items_;
};

} // namespace hb
