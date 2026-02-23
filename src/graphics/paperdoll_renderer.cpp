#include "graphics/paperdoll_renderer.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include "gameplay/inventory.hpp"
#include <spdlog/spdlog.h>
#include <array>

namespace hb
{

// Hair color RGB offsets (from legacy _GetHairColorRGB)
// Pre-normalized to 0-1 range: R,B divided by 31, G divided by 63.
struct hair_color_offset
{
    float r, g, b;
};

static constexpr std::array<hair_color_offset, 16> hair_colors = {{
    {14.0f / 31, -5.0f / 63, -5.0f / 31},  //  0: dark red
    {20.0f / 31, 0.0f / 63, 0.0f / 31},     //  1: orange
    {22.0f / 31, 13.0f / 63, -10.0f / 31},  //  2: light brown
    {0.0f / 31, 10.0f / 63, 0.0f / 31},     //  3: green
    {0.0f / 31, 0.0f / 63, 22.0f / 31},     //  4: flashy blue
    {-5.0f / 31, -5.0f / 63, 15.0f / 31},   //  5: dark blue
    {15.0f / 31, -5.0f / 63, 16.0f / 31},   //  6: mauve
    {-6.0f / 31, -6.0f / 63, -6.0f / 31},   //  7: black
    {-10.0f / 31, 0.0f / 63, 0.0f / 31},    //  8: dark
    {0.0f / 31, 0.0f / 63, 0.0f / 31},      //  9: natural
    {0.0f / 31, 0.0f / 63, 0.0f / 31},      // 10: natural
    {22.0f / 31, 22.0f / 63, 22.0f / 31},   // 11: white/bright
    {22.0f / 31, 17.0f / 63, 0.0f / 31},    // 12: golden
    {-5.0f / 31, 0.0f / 63, 22.0f / 31},    // 13: cyan-blue
    {0.0f / 31, 0.0f / 63, 0.0f / 31},      // 14: natural
    {0.0f / 31, 22.0f / 63, 0.0f / 31},     // 15: green
}};

// Item armor color table (from legacy m_wR/G/B)
// Pre-computed as (color_RGB - base_RGB) / 255.0 offsets
// Base color[0] = RGB(100, 100, 100)
struct color_offset
{
    float r, g, b;
};

static constexpr std::array<color_offset, 16> armor_colors = {{
    {0.0f, 0.0f, 0.0f},                                           //  0: base (no tint)
    {-60.0f / 255, -60.0f / 255, -4.0f / 255},                   //  1: blue
    {-21.0f / 255, -21.0f / 255, -38.0f / 255},                  //  2: custom weapon
    {35.0f / 255, 4.0f / 255, -70.0f / 255},                     //  3: gold
    {27.0f / 255, -82.0f / 255, -100.0f / 255},                  //  4: orange
    {-90.0f / 255, -40.0f / 255, -90.0f / 255},                  //  5: green
    {-60.0f / 255, -60.0f / 255, -60.0f / 255},                  //  6: gray
    {-53.0f / 255, -21.0f / 255, -20.0f / 255},                  //  7: teal
    {27.0f / 255, -48.0f / 255, -10.0f / 255},                   //  8: pink
    {0.0f, 0.0f, 0.0f},                                           //  9: unused
    {0.0f, 0.0f, 0.0f},                                           // 10: unused
    {0.0f, 0.0f, 0.0f},                                           // 11: unused
    {0.0f, 0.0f, 0.0f},                                           // 12: unused
    {0.0f, 0.0f, 0.0f},                                           // 13: unused
    {0.0f, 0.0f, 0.0f},                                           // 14: unused
    {0.0f, 0.0f, 0.0f},                                           // 15: unused
}};

// Weapon color table (from legacy m_wWR/G/B, offset from m_wR[0])
// Base color[0] = RGB(100, 100, 100)
static constexpr std::array<color_offset, 16> weapon_colors = {{
    {0.0f, 0.0f, 0.0f},                                           //  0: no tint
    {-30.0f / 255, -30.0f / 255, -20.0f / 255},                  //  1: light-blue
    {-30.0f / 255, -30.0f / 255, -20.0f / 255},                  //  2: light-blue
    {-30.0f / 255, -30.0f / 255, -20.0f / 255},                  //  3: light-blue
    {-30.0f / 255, 0.0f / 255, -30.0f / 255},                    //  4: green
    {30.0f / 255, -10.0f / 255, -90.0f / 255},                   //  5: golden
    {-58.0f / 255, -47.0f / 255, 11.0f / 255},                   //  6: heavy-blue
    {45.0f / 255, 45.0f / 255, 45.0f / 255},                     //  7: white
    {20.0f / 255, 0.0f / 255, 20.0f / 255},                      //  8: violet
    {-25.0f / 255, -90.0f / 255, -90.0f / 255},                  //  9: heavy-red
    {0.0f, 0.0f, 0.0f},                                           // 10: unused
    {0.0f, 0.0f, 0.0f},                                           // 11: unused
    {0.0f, 0.0f, 0.0f},                                           // 12: unused
    {0.0f, 0.0f, 0.0f},                                           // 13: unused
    {0.0f, 0.0f, 0.0f},                                           // 14: unused
    {0.0f, 0.0f, 0.0f},                                           // 15: unused
}};

// PAK loading entries for item-equip sprites
// Maps global sprite IDs to PAK indices (sparse mapping — not all IDs are used)
struct equip_pak_entry
{
    uint16_t global_offset; // Offset from equip_base (200)
    uint16_t pak_index;     // Sprite index within the PAK
};

static constexpr std::array<equip_pak_entry, 15> male_pak_entries = {{
    {0, 0},   // Body (3 frames: skin 1/2/3)
    {1, 1},   // Swords
    {2, 2},   // Bows
    {3, 3},   // Shields
    {4, 4},   // Tunics
    {5, 5},   // Shoes
    {7, 6},   // Berkah (hose cap)
    {8, 7},   // Hose/pants
    {9, 8},   // Body armor
    {15, 11}, // Axes/hammers
    {17, 12}, // Wands
    {18, 9},  // Hair (8 frames: styles 0-7)
    {19, 10}, // Underwear (8 frames: colors 0-7)
    {20, 13}, // Capes/mantles
    {21, 14}, // Helmets
}};

static constexpr std::array<equip_pak_entry, 15> female_pak_entries = {{
    {40, 0},  // Body (3 frames: skin 4/5/6 → frame 0/1/2)
    {41, 1},  // Swords
    {42, 2},  // Bows
    {43, 3},  // Shields
    {45, 4},  // Shoes
    {50, 5},  // Tunics
    {51, 6},  // Berkah
    {52, 7},  // Hose/pants
    {53, 8},  // Body armor
    {55, 11}, // Axes/hammers
    {57, 12}, // Wands
    {58, 9},  // Hair
    {59, 10}, // Underwear
    {60, 13}, // Capes/mantles
    {61, 14}, // Helmets
}};

bool paperdoll_renderer::initialize(sprite_manager& sprites)
{
    // Load male equipment PAK
    if (sprites.load_pak("item-equipM", "sprites/item-equipM.pak"))
    {
        for (const auto& entry : male_pak_entries)
        {
            uint16_t global_id = equip_base + entry.global_offset;
            sprites.store_sprite_at_id(global_id, "item-equipM", entry.pak_index);
        }
        spdlog::info("Paperdoll: loaded item-equipM.pak ({} sprites)", male_pak_entries.size());
    }
    else
    {
        spdlog::warn("Paperdoll: failed to load item-equipM.pak");
    }

    // Load female equipment PAK
    if (sprites.load_pak("item-equipW", "sprites/item-equipW.pak"))
    {
        for (const auto& entry : female_pak_entries)
        {
            uint16_t global_id = equip_base + entry.global_offset;
            sprites.store_sprite_at_id(global_id, "item-equipW", entry.pak_index);
        }
        spdlog::info("Paperdoll: loaded item-equipW.pak ({} sprites)", female_pak_entries.size());
    }
    else
    {
        spdlog::warn("Paperdoll: failed to load item-equipW.pak");
    }

    initialized_ = true;
    return true;
}

void paperdoll_renderer::draw(renderer& rend,
                              sprite_manager& sprites,
                              int32_t x,
                              int32_t y,
                              uint8_t gender,
                              uint8_t skin_color,
                              uint8_t hair_style,
                              uint8_t hair_color,
                              uint8_t underwear_color,
                              const inventory_system* inventory)
{
    if (!initialized_)
        return;

    bool female = (gender == 2);
    uint16_t g = female ? female_offset : 0; // 40 for female, 0 for male

    // Position offsets from anchor for equipment that isn't at (0,0)
    // These vary between male and female for some slots
    int32_t back_dx = female ? -126 : -130;
    int32_t back_dy = female ? -147 : -153;
    int32_t lhand_dx = female ? -87 : -81;
    int32_t lhand_dy = female ? -115 : -120;
    int32_t rhand_dx = female ? -111 : -114;
    int32_t rhand_dy = female ? -99 : -104;
    int32_t head_dx = -99;
    int32_t head_dy = female ? -151 : -155;
    int32_t neck_dx = -136;
    int32_t neck_dy = -170;
    int32_t rfinger_dx = -139;
    int32_t rfinger_dy = -97;

    // --- Layer 1: Body ---
    uint16_t body_id = equip_base + g; // 200 (male) or 240 (female)
    uint32_t body_frame = 0;
    if (!female)
        body_frame = (skin_color >= 1 && skin_color <= 3) ? skin_color - 1 : 0;
    else
        body_frame = (skin_color >= 1 && skin_color <= 3) ? skin_color - 1 : 0;

    if (auto* spr = sprites.get_sprite_by_id(body_id))
        rend.draw_sprite(*spr, x, y, body_frame);

    // --- Layer 2: Hair (only if no helmet equipped) ---
    bool has_helmet = inventory && inventory->get_equipped_item(equip_pos::head) != nullptr;
    if (!has_helmet)
    {
        uint16_t hair_id = equip_base + (female ? 58 : 18);
        if (auto* spr = sprites.get_sprite_by_id(hair_id))
        {
            uint32_t hair_frame = (hair_style <= 7) ? hair_style : 0;
            uint8_t hc = (hair_color < 16) ? hair_color : 0;
            const auto& hco = hair_colors[hc];
            rend.draw_sprite_tinted(*spr, x, y, hair_frame, hco.r, hco.g, hco.b);
        }
    }

    // --- Layer 3: Underwear ---
    uint16_t underwear_id = equip_base + (female ? 59 : 19);
    if (auto* spr = sprites.get_sprite_by_id(underwear_id))
    {
        uint32_t uw_frame = (underwear_color <= 7) ? underwear_color : 0;
        rend.draw_sprite(*spr, x, y, uw_frame);
    }

    // If no inventory, we're done (no equipment to draw)
    if (!inventory)
        return;

    // Female skirt logic: if pants equipped with sprite_id==12 && frame==0,
    // boots render early (before pants) instead of after arms
    bool skirt_draw = false;
    if (female)
    {
        if (const auto* pants = inventory->get_equipped_item(equip_pos::pants))
        {
            if (pants->sprite_id == 12)
                skirt_draw = true;
        }
    }

    // --- Layer 4: Back (cape/mantle) ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::cape))
    {
        draw_equip_layer(
            rend, sprites, x + back_dx, y + back_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Female skirt: boots drawn early if skirt ---
    if (female && skirt_draw)
    {
        if (const auto* item = inventory->get_equipped_item(equip_pos::boots))
        {
            draw_equip_layer(rend, sprites, x, y, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
        }
    }

    // --- Layer 5: Pants ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::pants))
    {
        draw_equip_layer(rend, sprites, x, y, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Layer 6: Arms ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::arms))
    {
        draw_equip_layer(rend, sprites, x, y, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Layer 7: Boots (normal position, skip if female skirt already drew them) ---
    if (!(female && skirt_draw))
    {
        if (const auto* item = inventory->get_equipped_item(equip_pos::boots))
        {
            draw_equip_layer(rend, sprites, x, y, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
        }
    }

    // --- Layer 8: Body armor ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::body))
    {
        draw_equip_layer(rend, sprites, x, y, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Layer 9: Full body armor ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::full_body))
    {
        draw_equip_layer(rend, sprites, x, y, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Layer 10: Left hand (shield) ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::shield))
    {
        draw_equip_layer(
            rend, sprites, x + lhand_dx, y + lhand_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, true);
    }

    // --- Layer 11: Right hand (weapon) ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::weapon))
    {
        draw_equip_layer(
            rend, sprites, x + rhand_dx, y + rhand_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, true);
    }

    // --- Layer 12: Two-hand weapon ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::twohand))
    {
        draw_equip_layer(
            rend, sprites, x + rhand_dx, y + rhand_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, true);
    }

    // --- Layer 13: Neck ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::amulet))
    {
        draw_equip_layer(
            rend, sprites, x + neck_dx, y + neck_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Layer 14: Right finger ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::ring_right))
    {
        draw_equip_layer(
            rend, sprites, x + rfinger_dx, y + rfinger_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }

    // --- Layer 15: Head (helmet) ---
    if (const auto* item = inventory->get_equipped_item(equip_pos::head))
    {
        draw_equip_layer(
            rend, sprites, x + head_dx, y + head_dy, equip_base + item->sprite_id + g, item->sprite_frame, item->color, false);
    }
}

void paperdoll_renderer::draw_equip_layer(renderer& rend,
                                          sprite_manager& sprites,
                                          int32_t x,
                                          int32_t y,
                                          uint16_t sprite_id,
                                          uint16_t frame,
                                          uint8_t color,
                                          bool is_weapon)
{
    auto* spr = sprites.get_sprite_by_id(sprite_id);
    if (!spr)
        return;

    if (color != 0 && color < 16)
    {
        const auto& table = is_weapon ? weapon_colors : armor_colors;
        const auto& co = table[color];
        rend.draw_sprite_tinted(*spr, x, y, frame, co.r, co.g, co.b);
    }
    else
    {
        rend.draw_sprite(*spr, x, y, frame);
    }
}

} // namespace hb
