#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace hb
{

class renderer;
class sprite_manager;

struct ground_item
{
    uint32_t item_id = 0;
    uint32_t template_id = 0;
    std::string name;
    int16_t count = 1;
    int16_t tile_x = 0;
    int16_t tile_y = 0;
    int16_t ground_sprite = 0;       // Sprite category (1=swords, 6=misc, etc.)
    int16_t ground_sprite_frame = 0; // Frame within sprite category
    int8_t item_color = 0;           // Color tint index (0 = no tint)
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

    void render_sprites(
        renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y, uint16_t sprite_base);
    void render_labels(renderer& rend, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y);

    ground_item* hit_test(int32_t mouse_x, int32_t mouse_y, int32_t camera_x, int32_t camera_y);

private:
    std::unordered_map<uint32_t, ground_item> items_;
};

// Base sprite ID for ground item PAK sprites (legacy DEF_SPRID_ITEMGROUND_PIVOTPOINT)
inline constexpr uint16_t item_ground_sprite_base = 100;
// Base sprite ID for inventory-style item PAK sprites (legacy DEF_SPRID_ITEMPACK_PIVOTPOINT)
inline constexpr uint16_t item_pack_sprite_base = 300;

} // namespace hb
