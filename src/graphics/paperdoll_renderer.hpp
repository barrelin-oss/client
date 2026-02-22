#pragma once

#include <cstdint>

namespace hb
{

class renderer;
class sprite_manager;
class inventory_system;

// Renders the paperdoll (flat front-facing character with equipment) for the character dialog.
// Uses dedicated item-equip PAK sprites (not the in-world animated character model).
class paperdoll_renderer
{
public:
    bool initialize(sprite_manager& sprites);

    // Draw paperdoll at anchor point (character center/feet).
    // x, y: screen position corresponding to legacy (sX+171, sY+290)
    void draw(renderer& rend,
              sprite_manager& sprites,
              int32_t x,
              int32_t y,
              uint8_t gender,
              uint8_t skin_color,
              uint8_t hair_style,
              uint8_t hair_color,
              uint8_t underwear_color,
              const inventory_system* inventory);

private:
    // Draw an equipment layer with optional color tinting
    void draw_equip_layer(renderer& rend,
                          sprite_manager& sprites,
                          int32_t x,
                          int32_t y,
                          uint16_t sprite_id,
                          uint16_t frame,
                          uint8_t color,
                          bool is_weapon);

    bool initialized_ = false;

    // Sprite ID base for paperdoll equipment
    static constexpr uint16_t equip_base = 200;
    static constexpr uint16_t female_offset = 40;
};

} // namespace hb
