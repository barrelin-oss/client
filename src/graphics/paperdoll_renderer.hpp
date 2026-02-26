#pragma once

#include "core/game_enums.hpp"
#include <cstdint>
#include <optional>

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
              const inventory_system* inventory,
              std::optional<equip_pos> skip_slot = std::nullopt);

    // Per-pixel hit test against equipment sprites drawn on the paperdoll.
    // Tests layers in reverse render order (topmost first).
    // Returns the equip_pos of the first equipment sprite whose opaque pixel
    // contains (screen_x, screen_y), or nullopt if no equipment hit.
    auto hit_test(sprite_manager& sprites,
                  int32_t screen_x,
                  int32_t screen_y,
                  int32_t anchor_x,
                  int32_t anchor_y,
                  uint8_t gender,
                  const inventory_system* inventory) const -> std::optional<equip_pos>;

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

    // Per-pixel test of a single equipment sprite at draw position (dx, dy)
    bool test_layer(sprite_manager& sprites,
                    int32_t screen_x,
                    int32_t screen_y,
                    int32_t draw_x,
                    int32_t draw_y,
                    uint16_t sprite_id,
                    uint16_t frame) const;

    bool initialized_ = false;

    // Sprite ID base for paperdoll equipment
    static constexpr uint16_t equip_base = 200;
    static constexpr uint16_t female_offset = 40;
};

} // namespace hb
