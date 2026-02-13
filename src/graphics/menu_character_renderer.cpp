#include "graphics/menu_character_renderer.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>
#include <array>

namespace hb {

// Hair color RGB offsets (from legacy _GetHairColorRGB)
// Legacy values are for RGB565 (5-bit R, 6-bit G, 5-bit B).
// Pre-normalized to 0-1 range: R,B divided by 31, G divided by 63.
struct hair_color_offset
{
    float r, g, b;
};

static constexpr hair_color_offset menu_hair_colors[16] = {
    { 14.0f/31,  -5.0f/63,  -5.0f/31},  //  0: dark red
    { 20.0f/31,   0.0f/63,   0.0f/31},  //  1: orange
    { 22.0f/31,  13.0f/63, -10.0f/31},  //  2: light brown
    {  0.0f/31,  10.0f/63,   0.0f/31},  //  3: green
    {  0.0f/31,   0.0f/63,  22.0f/31},  //  4: flashy blue
    { -5.0f/31,  -5.0f/63,  15.0f/31},  //  5: dark blue
    { 15.0f/31,  -5.0f/63,  16.0f/31},  //  6: mauve
    { -6.0f/31,  -6.0f/63,  -6.0f/31},  //  7: black
    {-10.0f/31,   0.0f/63,   0.0f/31},  //  8: dark
    {  0.0f/31,   0.0f/63,   0.0f/31},  //  9: natural
    {  0.0f/31,   0.0f/63,   0.0f/31},  // 10: natural
    { 22.0f/31,  22.0f/63,  22.0f/31},  // 11: white/bright
    { 22.0f/31,  17.0f/63,   0.0f/31},  // 12: golden
    { -5.0f/31,   0.0f/63,  22.0f/31},  // 13: cyan-blue
    {  0.0f/31,   0.0f/63,   0.0f/31},  // 14: natural
    {  0.0f/31,  22.0f/63,   0.0f/31},  // 15: green
};

// Equipment PAK loading table entry
struct pak_load_entry {
    const char* pak_name;
    uint32_t sprite_id;        // Global sprite ID offset
    uint32_t sprite_count;     // Number of sprites to load
    uint32_t pak_start_index;  // Starting index within the PAK file
};

// Male body PAKs (skin colors 1-3)
static constexpr std::array<pak_load_entry, 3> male_body_paks = {{
    {"Bm", 500 + 0 * 120, 120, 0},   // Skin 1 (Black)
    {"Wm", 500 + 1 * 120, 120, 0},   // Skin 2 (White)
    {"Ym", 500 + 2 * 120, 120, 0},   // Skin 3 (Yellow)
}};

// Female body PAKs (skin colors 1-3)
static constexpr std::array<pak_load_entry, 3> female_body_paks = {{
    {"Bw", 500 + 3 * 120, 120, 0},   // Skin 1 (Black)
    {"Ww", 500 + 4 * 120, 120, 0},   // Skin 2 (White)
    {"Yw", 500 + 5 * 120, 120, 0},   // Skin 3 (Yellow)
}};

// Male underwear/pants (Mpt.pak) - 8 color variants, 12 sprites per variant
// PAK stores 96 sprites contiguously (8 × 12), but global IDs use stride 15
// Legacy: m_pSprite[UNDIES_M + i + 15*color] = CSprite("Mpt", i + 12*color)
static constexpr std::array<pak_load_entry, 8> male_underwear_paks = {{
    {"Mpt", 4580 + 15 * 0, 12, 12 * 0},
    {"Mpt", 4580 + 15 * 1, 12, 12 * 1},
    {"Mpt", 4580 + 15 * 2, 12, 12 * 2},
    {"Mpt", 4580 + 15 * 3, 12, 12 * 3},
    {"Mpt", 4580 + 15 * 4, 12, 12 * 4},
    {"Mpt", 4580 + 15 * 5, 12, 12 * 5},
    {"Mpt", 4580 + 15 * 6, 12, 12 * 6},
    {"Mpt", 4580 + 15 * 7, 12, 12 * 7},
}};

// Male hair (Mhr.pak) - 8 style variants, 12 sprites per variant
static constexpr std::array<pak_load_entry, 8> male_hair_paks = {{
    {"Mhr", 4820 + 15 * 0, 12, 12 * 0},
    {"Mhr", 4820 + 15 * 1, 12, 12 * 1},
    {"Mhr", 4820 + 15 * 2, 12, 12 * 2},
    {"Mhr", 4820 + 15 * 3, 12, 12 * 3},
    {"Mhr", 4820 + 15 * 4, 12, 12 * 4},
    {"Mhr", 4820 + 15 * 5, 12, 12 * 5},
    {"Mhr", 4820 + 15 * 6, 12, 12 * 6},
    {"Mhr", 4820 + 15 * 7, 12, 12 * 7},
}};

// Female underwear/pants (Wpt.pak) - 8 color variants
static constexpr std::array<pak_load_entry, 8> female_underwear_paks = {{
    {"Wpt", 14580 + 15 * 0, 12, 12 * 0},
    {"Wpt", 14580 + 15 * 1, 12, 12 * 1},
    {"Wpt", 14580 + 15 * 2, 12, 12 * 2},
    {"Wpt", 14580 + 15 * 3, 12, 12 * 3},
    {"Wpt", 14580 + 15 * 4, 12, 12 * 4},
    {"Wpt", 14580 + 15 * 5, 12, 12 * 5},
    {"Wpt", 14580 + 15 * 6, 12, 12 * 6},
    {"Wpt", 14580 + 15 * 7, 12, 12 * 7},
}};

// Female hair (Whr.pak) - 8 style variants
static constexpr std::array<pak_load_entry, 8> female_hair_paks = {{
    {"Whr", 14820 + 15 * 0, 12, 12 * 0},
    {"Whr", 14820 + 15 * 1, 12, 12 * 1},
    {"Whr", 14820 + 15 * 2, 12, 12 * 2},
    {"Whr", 14820 + 15 * 3, 12, 12 * 3},
    {"Whr", 14820 + 15 * 4, 12, 12 * 4},
    {"Whr", 14820 + 15 * 5, 12, 12 * 5},
    {"Whr", 14820 + 15 * 6, 12, 12 * 6},
    {"Whr", 14820 + 15 * 7, 12, 12 * 7},
}};

// Male body armor PAKs
static constexpr std::array<pak_load_entry, 7> male_body_armor_paks = {{
    {"MLArmor", 5060 + 15 * 1, 12, 0},
    {"MCMail",  5060 + 15 * 2, 12, 0},
    {"MSMail",  5060 + 15 * 3, 12, 0},
    {"MPMail",  5060 + 15 * 4, 12, 0},
    {"Mtunic",  5060 + 15 * 5, 12, 0},
    {"MRobe1",  5060 + 15 * 6, 12, 0},
    {"MSanta",  5060 + 15 * 7, 12, 0},
}};

// Female body armor PAKs
static constexpr std::array<pak_load_entry, 8> female_body_armor_paks = {{
    {"WBodice1", 15060 + 15 * 1, 12, 0},
    {"WBodice2", 15060 + 15 * 2, 12, 0},
    {"WLArmor",  15060 + 15 * 3, 12, 0},
    {"WCMail",   15060 + 15 * 4, 12, 0},
    {"WSMail",   15060 + 15 * 5, 12, 0},
    {"WPMail",   15060 + 15 * 6, 12, 0},
    {"WRobe1",   15060 + 15 * 7, 12, 0},
    {"WSanta",   15060 + 15 * 8, 12, 0},
}};

// Male arm armor (shirts)
static constexpr std::array<pak_load_entry, 2> male_arm_armor_paks = {{
    {"MShirt",   5300 + 15 * 1, 12, 0},
    {"MHauberk", 5300 + 15 * 2, 12, 0},
}};

// Female arm armor (shirts)
static constexpr std::array<pak_load_entry, 3> female_arm_armor_paks = {{
    {"WChemiss", 15300 + 15 * 1, 12, 0},
    {"WShirt",   15300 + 15 * 2, 12, 0},
    {"WHauberk", 15300 + 15 * 3, 12, 0},
}};

// Male pants/trousers
static constexpr std::array<pak_load_entry, 4> male_pants_paks = {{
    {"MTrouser",  5540 + 15 * 1, 12, 0},
    {"MHTrouser", 5540 + 15 * 2, 12, 0},
    {"MCHoses",   5540 + 15 * 3, 12, 0},
    {"MLeggings", 5540 + 15 * 4, 12, 0},
}};

// Female pants/skirts
static constexpr std::array<pak_load_entry, 5> female_pants_paks = {{
    {"WSkirt",    15540 + 15 * 1, 12, 0},
    {"WTrouser",  15540 + 15 * 2, 12, 0},
    {"WHTrouser", 15540 + 15 * 3, 12, 0},
    {"WCHoses",   15540 + 15 * 4, 12, 0},
    {"WLeggings", 15540 + 15 * 5, 12, 0},
}};

// Male boots
static constexpr std::array<pak_load_entry, 2> male_boots_paks = {{
    {"MShoes",  5780 + 15 * 1, 12, 0},
    {"MLBoots", 5780 + 15 * 2, 12, 0},
}};

// Female boots
static constexpr std::array<pak_load_entry, 2> female_boots_paks = {{
    {"WShoes",  15780 + 15 * 1, 12, 0},
    {"WLBoots", 15780 + 15 * 2, 12, 0},
}};

// Male weapons - swords (loaded via loop at multiple offsets)
static constexpr std::array<pak_load_entry, 14> male_sword_paks = {{
    {"Msw",  6020 + 64 * 1, 56, 0},
    {"Msw",  6020 + 64 * 2, 56, 0},
    {"Msw",  6020 + 64 * 3, 56, 0},
    {"Msw",  6020 + 64 * 4, 56, 0},
    {"Mswx", 6020 + 64 * 5, 56, 0},
    {"Msw",  6020 + 64 * 6, 56, 0},
    {"Msw",  6020 + 64 * 7, 56, 0},
    {"Msw",  6020 + 64 * 8, 56, 0},
    {"Msw",  6020 + 64 * 9, 56, 0},
    {"Msw",  6020 + 64 * 10, 56, 0},
    {"Msw",  6020 + 64 * 11, 56, 0},
    {"Msw",  6020 + 64 * 12, 56, 0},
    {"Msw2", 6020 + 64 * 13, 56, 0},
    {"Msw2", 6020 + 64 * 14, 56, 0},
}};

// Male weapons - axes
static constexpr std::array<pak_load_entry, 8> male_axe_paks = {{
    {"MAxe1",     6020 + 64 * 20, 56, 0},
    {"MAxe2",     6020 + 64 * 21, 56, 0},
    {"MAxe3",     6020 + 64 * 22, 56, 0},
    {"MAxe4",     6020 + 64 * 23, 56, 0},
    {"MAxe5",     6020 + 64 * 24, 56, 0},
    {"MPickAxe1", 6020 + 64 * 25, 56, 0},
    {"MAxe6",     6020 + 64 * 26, 56, 0},
    {"Mhoe",      6020 + 64 * 27, 56, 0},
}};

// Male weapons - hammers and staffs
static constexpr std::array<pak_load_entry, 4> male_hammer_staff_paks = {{
    {"MHammer",  6020 + 64 * 30, 56, 0},
    {"MBHammer", 6020 + 64 * 31, 56, 0},
    {"Mstaff1",  6020 + 64 * 35, 56, 0},
    {"Mstaff2",  6020 + 64 * 36, 56, 0},
}};

// Male weapons - bow
static constexpr std::array<pak_load_entry, 2> male_bow_paks = {{
    {"Mbo", 6020 + 64 * 40, 56, 0},
    {"Mbo", 6020 + 64 * 41, 56, 0},
}};

// Female weapons - swords
static constexpr std::array<pak_load_entry, 14> female_sword_paks = {{
    {"Wsw",  16020 + 64 * 1, 56, 0},
    {"Wsw",  16020 + 64 * 2, 56, 0},
    {"Wsw",  16020 + 64 * 3, 56, 0},
    {"Wsw",  16020 + 64 * 4, 56, 0},
    {"Wswx", 16020 + 64 * 5, 56, 0},
    {"Wsw",  16020 + 64 * 6, 56, 0},
    {"Wsw",  16020 + 64 * 7, 56, 0},
    {"Wsw",  16020 + 64 * 8, 56, 0},
    {"Wsw",  16020 + 64 * 9, 56, 0},
    {"Wsw",  16020 + 64 * 10, 56, 0},
    {"Wsw",  16020 + 64 * 11, 56, 0},
    {"Wsw",  16020 + 64 * 12, 56, 0},
    {"Wsw2", 16020 + 64 * 13, 56, 0},
    {"Wsw2", 16020 + 64 * 14, 56, 0},
}};

// Female weapons - axes
static constexpr std::array<pak_load_entry, 8> female_axe_paks = {{
    {"WAxe1",     16020 + 64 * 20, 56, 0},
    {"WAxe2",     16020 + 64 * 21, 56, 0},
    {"WAxe3",     16020 + 64 * 22, 56, 0},
    {"WAxe4",     16020 + 64 * 23, 56, 0},
    {"WAxe5",     16020 + 64 * 24, 56, 0},
    {"WpickAxe1", 16020 + 64 * 25, 56, 0},
    {"WAxe6",     16020 + 64 * 26, 56, 0},
    {"Whoe",      16020 + 64 * 27, 56, 0},
}};

// Female weapons - hammers and staffs
static constexpr std::array<pak_load_entry, 4> female_hammer_staff_paks = {{
    {"WHammer",  16020 + 64 * 30, 56, 0},
    {"WBHammer", 16020 + 64 * 31, 56, 0},
    {"Wstaff1",  16020 + 64 * 35, 56, 0},
    {"Wstaff2",  16020 + 64 * 36, 56, 0},
}};

// Female weapons - bow
static constexpr std::array<pak_load_entry, 2> female_bow_paks = {{
    {"Wbo", 16020 + 64 * 40, 56, 0},
    {"Wbo", 16020 + 64 * 41, 56, 0},
}};

// Male shields (Msh.pak) - 9 types, 7 sprites each
static constexpr pak_load_entry male_shield_pak = {"Msh", 9100, 72, 0};

// Female shields (Wsh.pak)
static constexpr pak_load_entry female_shield_pak = {"Wsh", 19100, 72, 0};

// Male mantles
static constexpr std::array<pak_load_entry, 3> male_mantle_paks = {{
    {"Mmantle01", 9230 + 15 * 1, 12, 0},
    {"Mmantle02", 9230 + 15 * 2, 12, 0},
    {"Mmantle03", 9230 + 15 * 3, 12, 0},
}};

// Female mantles
static constexpr std::array<pak_load_entry, 3> female_mantle_paks = {{
    {"Wmantle01", 19230 + 15 * 1, 12, 0},
    {"Wmantle02", 19230 + 15 * 2, 12, 0},
    {"Wmantle03", 19230 + 15 * 3, 12, 0},
}};

// Male helmets
static constexpr std::array<pak_load_entry, 8> male_helmet_paks = {{
    {"MHelm1",  9300 + 15 * 1, 12, 0},
    {"MHelm2",  9300 + 15 * 2, 12, 0},
    {"MHelm3",  9300 + 15 * 3, 12, 0},
    {"MHelm4",  9300 + 15 * 4, 12, 0},
    {"NMHelm1", 9300 + 15 * 5, 12, 0},
    {"NMHelm2", 9300 + 15 * 6, 12, 0},
    {"NMHelm3", 9300 + 15 * 7, 12, 0},
    {"NMHelm4", 9300 + 15 * 8, 12, 0},
}};

// Female helmets
static constexpr std::array<pak_load_entry, 6> female_helmet_paks = {{
    {"WHelm1",  19300 + 15 * 1, 12, 0},
    {"WHelm4",  19300 + 15 * 4, 12, 0},
    {"NWHelm1", 19300 + 15 * 5, 12, 0},
    {"NWHelm2", 19300 + 15 * 6, 12, 0},
    {"NWHelm3", 19300 + 15 * 7, 12, 0},
    {"NWHelm4", 19300 + 15 * 8, 12, 0},
}};

// Helper to load a PAK and store sprites at global IDs
static bool load_pak_at_offset(sprite_manager& sprites, const pak_load_entry& entry) {
    std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";

    if (!sprites.load_pak(entry.pak_name, pak_path)) {
        spdlog::debug("Optional PAK not found: {}", pak_path);
        return false;
    }

    // Store each sprite at its global ID, using pak_start_index for multi-variant PAKs
    for (uint32_t i = 0; i < entry.sprite_count; ++i) {
        uint16_t global_id = static_cast<uint16_t>(entry.sprite_id + i);
        sprites.store_sprite_at_id(global_id, entry.pak_name, entry.pak_start_index + i);
    }

    return true;
}

// Helper to load array of PAKs
template<size_t N>
static void load_pak_array(sprite_manager& sprites, const std::array<pak_load_entry, N>& entries) {
    for (const auto& entry : entries) {
        load_pak_at_offset(sprites, entry);
    }
}

std::vector<equipment_pak_entry> menu_character_renderer::get_pak_load_list() {
    std::vector<equipment_pak_entry> list;

    auto add = [&](const pak_load_entry& e) {
        list.push_back({e.pak_name, e.sprite_id, e.sprite_count, e.pak_start_index});
    };
    auto add_array = [&](auto& arr) {
        for (const auto& e : arr) add(e);
    };

    // Body sprites
    add_array(male_body_paks);
    add_array(female_body_paks);

    // Base appearance (underwear, hair) - loaded with stride-15 gaps per variant
    add_array(male_underwear_paks);
    add_array(male_hair_paks);
    add_array(female_underwear_paks);
    add_array(female_hair_paks);

    // Armor
    add_array(male_body_armor_paks);
    add_array(female_body_armor_paks);
    add_array(male_arm_armor_paks);
    add_array(female_arm_armor_paks);
    add_array(male_pants_paks);
    add_array(female_pants_paks);
    add_array(male_boots_paks);
    add_array(female_boots_paks);

    // Weapons
    add_array(male_sword_paks);
    add_array(male_axe_paks);
    add_array(male_hammer_staff_paks);
    add_array(male_bow_paks);
    add_array(female_sword_paks);
    add_array(female_axe_paks);
    add_array(female_hammer_staff_paks);
    add_array(female_bow_paks);

    // Shields
    add(male_shield_pak);
    add(female_shield_pak);

    // Mantles
    add_array(male_mantle_paks);
    add_array(female_mantle_paks);

    // Helmets
    add_array(male_helmet_paks);
    add_array(female_helmet_paks);

    return list;
}

bool menu_character_renderer::initialize(sprite_manager& sprites) {
    spdlog::info("Initializing menu character renderer with equipment support...");

    for (const auto& entry : get_pak_load_list()) {
        load_pak_at_offset(sprites, {entry.pak_name, entry.sprite_id, entry.sprite_count, entry.pak_start_index});
    }

    initialized_ = true;
    spdlog::info("Menu character renderer initialized (with equipment support)");
    return true;
}

int32_t menu_character_renderer::get_owner_type(uint8_t gender, uint8_t skin_color) const {
    uint8_t skin = std::clamp(skin_color, uint8_t(1), uint8_t(3));
    return (gender == 1) ? skin : (3 + skin);
}

int32_t menu_character_renderer::calc_frame(int32_t direction, int32_t frame) const {
    return (direction - 1) * 8 + frame;
}

void menu_character_renderer::draw(renderer& rend, sprite_manager& sprites,
                                   int32_t x, int32_t y,
                                   const character_appearance& appearance,
                                   int32_t direction, int32_t frame) {
    if (!initialized_) return;

    int32_t dir = std::clamp(direction, 1, 8);
    int32_t frm = std::clamp(frame, 0, 7);
    uint8_t gender = std::clamp(appearance.gender, uint8_t(1), uint8_t(2));
    uint8_t skin = std::clamp(appearance.skin_color, uint8_t(1), uint8_t(3));

    int32_t action = action_walk;
    bool weapon_behind = (drawing_order[dir] == 1);
    int8_t mantle_order = mantle_drawing_order[dir];
    bool is_skirt = (appearance.pants == 1) && is_female(gender);

    // Draw weapon FIRST if facing away
    if (weapon_behind && appearance.weapon > 0) {
        draw_weapon(rend, sprites, x, y, gender, appearance.weapon, action, dir, frm);
    }

    // Draw body
    draw_body(rend, sprites, x, y, gender, skin, action, dir, frm);

    // Draw mantle early
    if (mantle_order == 0 && appearance.mantle > 0) {
        draw_mantle(rend, sprites, x, y, gender, appearance.mantle, action, dir, frm);
    }

    // Draw underwear
    draw_underwear(rend, sprites, x, y, gender, appearance.underwear_color, action, dir, frm);

    // Draw hair (if no helmet)
    if (appearance.helmet == 0) {
        draw_hair(rend, sprites, x, y, gender, appearance.hair_style, appearance.hair_color,
                  action, dir, frm);
    }

    // Draw boots early if wearing skirt
    if (is_skirt && appearance.boots > 0) {
        draw_boots(rend, sprites, x, y, gender, appearance.boots, action, dir, frm);
    }

    // Draw pants
    if (appearance.pants > 0) {
        draw_pants(rend, sprites, x, y, gender, appearance.pants, action, dir, frm);
    }

    // Draw arm armor
    if (appearance.arm_armor > 0) {
        draw_arm_armor(rend, sprites, x, y, gender, appearance.arm_armor, action, dir, frm);
    }

    // Draw boots late if not wearing skirt
    if (!is_skirt && appearance.boots > 0) {
        draw_boots(rend, sprites, x, y, gender, appearance.boots, action, dir, frm);
    }

    // Draw body armor
    if (appearance.body_armor > 0) {
        draw_body_armor(rend, sprites, x, y, gender, appearance.body_armor, action, dir, frm);
    }

    // Draw helmet
    if (appearance.helmet > 0) {
        draw_helmet(rend, sprites, x, y, gender, appearance.helmet, action, dir, frm);
    }

    // Draw mantle position 2
    if (mantle_order == 2 && appearance.mantle > 0) {
        draw_mantle(rend, sprites, x, y, gender, appearance.mantle, action, dir, frm);
    }

    // Draw shield
    if (appearance.shield > 0) {
        draw_shield(rend, sprites, x, y, gender, appearance.shield, action, dir, frm);
    }

    // Draw mantle position 1
    if (mantle_order == 1 && appearance.mantle > 0) {
        draw_mantle(rend, sprites, x, y, gender, appearance.mantle, action, dir, frm);
    }

    // Draw weapon LAST if facing toward camera
    if (!weapon_behind && appearance.weapon > 0) {
        draw_weapon(rend, sprites, x, y, gender, appearance.weapon, action, dir, frm);
    }
}

void menu_character_renderer::draw_with_shadow(renderer& rend, sprite_manager& sprites,
                                               int32_t x, int32_t y,
                                               const character_appearance& appearance,
                                               int32_t direction, int32_t frame) {
    draw(rend, sprites, x, y, appearance, direction, frame);
}

void menu_character_renderer::draw_body(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                        uint8_t gender, uint8_t skin_color, int32_t action, int32_t dir, int32_t frame) {
    // Body sprite ID: 500 + (owner_type - 1) * 120 + action * 8 + (dir - 1)
    int32_t owner_type = get_owner_type(gender, skin_color);
    uint16_t sprite_id = static_cast<uint16_t>(body_base + (owner_type - 1) * body_stride + action * 8 + (dir - 1));

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame);
    }
}

void menu_character_renderer::draw_underwear(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                             uint8_t gender, uint8_t underwear_color, int32_t action, int32_t dir, int32_t frame) {
    uint16_t base = is_female(gender) ? female_underwear_base : male_underwear_base;
    int32_t color = std::clamp(int32_t(underwear_color), 0, 7);

    // Underwear sprite ID: base + color * 15 + action
    uint16_t sprite_id = static_cast<uint16_t>(base + color * underwear_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_hair(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                        uint8_t gender, uint8_t hair_style, uint8_t hair_color,
                                        int32_t action, int32_t dir, int32_t frame) {
    uint16_t base = is_female(gender) ? female_hair_base : male_hair_base;
    int32_t style = std::clamp(int32_t(hair_style), 0, 7);

    // Hair sprite ID: base + style * 15 + action
    uint16_t sprite_id = static_cast<uint16_t>(base + style * hair_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        uint8_t hc = std::clamp(hair_color, uint8_t(0), uint8_t(15));
        const auto& tint = menu_hair_colors[hc];
        if (tint.r == 0.0f && tint.g == 0.0f && tint.b == 0.0f) {
            rend.draw_sprite(*spr, x, y, frame_index);
        } else {
            rend.draw_sprite_tinted(*spr, x, y, frame_index, tint.r, tint.g, tint.b);
        }
    }
}

void menu_character_renderer::draw_body_armor(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                              uint8_t gender, uint8_t armor_type, int32_t action, int32_t dir, int32_t frame) {
    if (armor_type == 0) return;

    uint16_t base = is_female(gender) ? female_body_armor_base : male_body_armor_base;
    int32_t type = std::clamp(int32_t(armor_type), 1, 15);

    // Body armor sprite ID: base + type * 15 + action
    uint16_t sprite_id = static_cast<uint16_t>(base + type * body_armor_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_arm_armor(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                             uint8_t gender, uint8_t armor_type, int32_t action, int32_t dir, int32_t frame) {
    if (armor_type == 0) return;

    uint16_t base = is_female(gender) ? female_arm_armor_base : male_arm_armor_base;
    int32_t type = std::clamp(int32_t(armor_type), 1, 15);

    uint16_t sprite_id = static_cast<uint16_t>(base + type * arm_armor_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_pants(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                         uint8_t gender, uint8_t pants_type, int32_t action, int32_t dir, int32_t frame) {
    if (pants_type == 0) return;

    uint16_t base = is_female(gender) ? female_pants_base : male_pants_base;
    int32_t type = std::clamp(int32_t(pants_type), 1, 15);

    uint16_t sprite_id = static_cast<uint16_t>(base + type * pants_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_boots(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                         uint8_t gender, uint8_t boots_type, int32_t action, int32_t dir, int32_t frame) {
    if (boots_type == 0) return;

    uint16_t base = is_female(gender) ? female_boots_base : male_boots_base;
    int32_t type = std::clamp(int32_t(boots_type), 1, 15);

    uint16_t sprite_id = static_cast<uint16_t>(base + type * boots_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_helmet(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                          uint8_t gender, uint8_t helmet_type, int32_t action, int32_t dir, int32_t frame) {
    if (helmet_type == 0) return;

    uint16_t base = is_female(gender) ? female_helmet_base : male_helmet_base;
    int32_t type = std::clamp(int32_t(helmet_type), 1, 15);

    uint16_t sprite_id = static_cast<uint16_t>(base + type * helmet_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_mantle(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                          uint8_t gender, uint8_t mantle_type, int32_t action, int32_t dir, int32_t frame) {
    if (mantle_type == 0) return;

    uint16_t base = is_female(gender) ? female_mantle_base : male_mantle_base;
    int32_t type = std::clamp(int32_t(mantle_type), 1, 15);

    uint16_t sprite_id = static_cast<uint16_t>(base + type * mantle_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_weapon(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                          uint8_t gender, uint8_t weapon_type, int32_t action, int32_t dir, int32_t frame) {
    if (weapon_type == 0) return;

    uint16_t base = is_female(gender) ? female_weapon_base : male_weapon_base;
    int32_t type = std::clamp(int32_t(weapon_type), 1, 255);

    // Weapon sprite ID: base + type * 64 + action * 8 + (dir - 1)
    uint16_t sprite_id = static_cast<uint16_t>(base + type * weapon_stride + action * 8 + (dir - 1));

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame);
    }
}

void menu_character_renderer::draw_shield(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                          uint8_t gender, uint8_t shield_type, int32_t action, int32_t dir, int32_t frame) {
    if (shield_type == 0) return;

    uint16_t base = is_female(gender) ? female_shield_base : male_shield_base;
    int32_t type = std::clamp(int32_t(shield_type), 1, 9);

    // Shield sprite ID: base + type * 8 + action
    // Shields have 7 sprites per type (not 8)
    uint16_t sprite_id = static_cast<uint16_t>(base + type * shield_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

} // namespace hb
