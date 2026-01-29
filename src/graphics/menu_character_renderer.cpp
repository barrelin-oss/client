#include "graphics/menu_character_renderer.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>
#include <array>

namespace hb {

// Equipment PAK loading table entry
struct pak_load_entry {
    const char* pak_name;
    uint32_t sprite_id;    // Global sprite ID offset
    uint32_t sprite_count; // Number of sprites to load from PAK
};

// Male body PAKs (skin colors 1-3)
static constexpr std::array<pak_load_entry, 3> male_body_paks = {{
    {"Bm", 500 + 0 * 120, 120},   // Skin 1 (Black)
    {"Wm", 500 + 1 * 120, 120},   // Skin 2 (White)
    {"Ym", 500 + 2 * 120, 120},   // Skin 3 (Yellow)
}};

// Female body PAKs (skin colors 1-3)
static constexpr std::array<pak_load_entry, 3> female_body_paks = {{
    {"Bw", 500 + 3 * 120, 120},   // Skin 1 (Black)
    {"Ww", 500 + 4 * 120, 120},   // Skin 2 (White)
    {"Yw", 500 + 5 * 120, 120},   // Skin 3 (Yellow)
}};

// Male underwear/pants base (Mpt.pak) - loaded at 4580, 8 color variants × 12 sprites
static constexpr pak_load_entry male_underwear_pak = {"Mpt", 4580, 96};

// Male hair (Mhr.pak) - loaded at 4820, 8 style variants × 12 sprites
static constexpr pak_load_entry male_hair_pak = {"Mhr", 4820, 96};

// Female underwear/pants base (Wpt.pak) - loaded at 14580
static constexpr pak_load_entry female_underwear_pak = {"Wpt", 14580, 96};

// Female hair (Whr.pak) - loaded at 14820
static constexpr pak_load_entry female_hair_pak = {"Whr", 14820, 96};

// Male body armor PAKs
static constexpr std::array<pak_load_entry, 7> male_body_armor_paks = {{
    {"MLArmor", 5060 + 15 * 1, 12},
    {"MCMail",  5060 + 15 * 2, 12},
    {"MSMail",  5060 + 15 * 3, 12},
    {"MPMail",  5060 + 15 * 4, 12},
    {"Mtunic",  5060 + 15 * 5, 12},
    {"MRobe1",  5060 + 15 * 6, 12},
    {"MSanta",  5060 + 15 * 7, 12},
}};

// Female body armor PAKs
static constexpr std::array<pak_load_entry, 8> female_body_armor_paks = {{
    {"WBodice1", 15060 + 15 * 1, 12},
    {"WBodice2", 15060 + 15 * 2, 12},
    {"WLArmor",  15060 + 15 * 3, 12},
    {"WCMail",   15060 + 15 * 4, 12},
    {"WSMail",   15060 + 15 * 5, 12},
    {"WPMail",   15060 + 15 * 6, 12},
    {"WRobe1",   15060 + 15 * 7, 12},
    {"WSanta",   15060 + 15 * 8, 12},
}};

// Male arm armor (shirts)
static constexpr std::array<pak_load_entry, 2> male_arm_armor_paks = {{
    {"MShirt",   5300 + 15 * 1, 12},
    {"MHauberk", 5300 + 15 * 2, 12},
}};

// Female arm armor (shirts)
static constexpr std::array<pak_load_entry, 3> female_arm_armor_paks = {{
    {"WChemiss", 15300 + 15 * 1, 12},
    {"WShirt",   15300 + 15 * 2, 12},
    {"WHauberk", 15300 + 15 * 3, 12},
}};

// Male pants/trousers
static constexpr std::array<pak_load_entry, 4> male_pants_paks = {{
    {"MTrouser",  5540 + 15 * 1, 12},
    {"MHTrouser", 5540 + 15 * 2, 12},
    {"MCHoses",   5540 + 15 * 3, 12},
    {"MLeggings", 5540 + 15 * 4, 12},
}};

// Female pants/skirts
static constexpr std::array<pak_load_entry, 5> female_pants_paks = {{
    {"WSkirt",    15540 + 15 * 1, 12},
    {"WTrouser",  15540 + 15 * 2, 12},
    {"WHTrouser", 15540 + 15 * 3, 12},
    {"WCHoses",   15540 + 15 * 4, 12},
    {"WLeggings", 15540 + 15 * 5, 12},
}};

// Male boots
static constexpr std::array<pak_load_entry, 2> male_boots_paks = {{
    {"MShoes",  5780 + 15 * 1, 12},
    {"MLBoots", 5780 + 15 * 2, 12},
}};

// Female boots
static constexpr std::array<pak_load_entry, 2> female_boots_paks = {{
    {"WShoes",  15780 + 15 * 1, 12},
    {"WLBoots", 15780 + 15 * 2, 12},
}};

// Male weapons - swords (loaded via loop at multiple offsets)
static constexpr std::array<pak_load_entry, 14> male_sword_paks = {{
    {"Msw",  6020 + 64 * 1, 56},
    {"Msw",  6020 + 64 * 2, 56},
    {"Msw",  6020 + 64 * 3, 56},
    {"Msw",  6020 + 64 * 4, 56},
    {"Mswx", 6020 + 64 * 5, 56},
    {"Msw",  6020 + 64 * 6, 56},
    {"Msw",  6020 + 64 * 7, 56},
    {"Msw",  6020 + 64 * 8, 56},
    {"Msw",  6020 + 64 * 9, 56},
    {"Msw",  6020 + 64 * 10, 56},
    {"Msw",  6020 + 64 * 11, 56},
    {"Msw",  6020 + 64 * 12, 56},
    {"Msw2", 6020 + 64 * 13, 56},
    {"Msw2", 6020 + 64 * 14, 56},
}};

// Male weapons - axes
static constexpr std::array<pak_load_entry, 8> male_axe_paks = {{
    {"MAxe1",     6020 + 64 * 20, 56},
    {"MAxe2",     6020 + 64 * 21, 56},
    {"MAxe3",     6020 + 64 * 22, 56},
    {"MAxe4",     6020 + 64 * 23, 56},
    {"MAxe5",     6020 + 64 * 24, 56},
    {"MPickAxe1", 6020 + 64 * 25, 56},
    {"MAxe6",     6020 + 64 * 26, 56},
    {"Mhoe",      6020 + 64 * 27, 56},
}};

// Male weapons - hammers and staffs
static constexpr std::array<pak_load_entry, 4> male_hammer_staff_paks = {{
    {"MHammer",  6020 + 64 * 30, 56},
    {"MBHammer", 6020 + 64 * 31, 56},
    {"Mstaff1",  6020 + 64 * 35, 56},
    {"Mstaff2",  6020 + 64 * 36, 56},
}};

// Male weapons - bow
static constexpr std::array<pak_load_entry, 2> male_bow_paks = {{
    {"Mbo", 6020 + 64 * 40, 56},
    {"Mbo", 6020 + 64 * 41, 56},
}};

// Female weapons - swords
static constexpr std::array<pak_load_entry, 14> female_sword_paks = {{
    {"Wsw",  16020 + 64 * 1, 56},
    {"Wsw",  16020 + 64 * 2, 56},
    {"Wsw",  16020 + 64 * 3, 56},
    {"Wsw",  16020 + 64 * 4, 56},
    {"Wswx", 16020 + 64 * 5, 56},
    {"Wsw",  16020 + 64 * 6, 56},
    {"Wsw",  16020 + 64 * 7, 56},
    {"Wsw",  16020 + 64 * 8, 56},
    {"Wsw",  16020 + 64 * 9, 56},
    {"Wsw",  16020 + 64 * 10, 56},
    {"Wsw",  16020 + 64 * 11, 56},
    {"Wsw",  16020 + 64 * 12, 56},
    {"Wsw2", 16020 + 64 * 13, 56},
    {"Wsw2", 16020 + 64 * 14, 56},
}};

// Female weapons - axes
static constexpr std::array<pak_load_entry, 8> female_axe_paks = {{
    {"WAxe1",     16020 + 64 * 20, 56},
    {"WAxe2",     16020 + 64 * 21, 56},
    {"WAxe3",     16020 + 64 * 22, 56},
    {"WAxe4",     16020 + 64 * 23, 56},
    {"WAxe5",     16020 + 64 * 24, 56},
    {"WpickAxe1", 16020 + 64 * 25, 56},
    {"WAxe6",     16020 + 64 * 26, 56},
    {"Whoe",      16020 + 64 * 27, 56},
}};

// Female weapons - hammers and staffs
static constexpr std::array<pak_load_entry, 4> female_hammer_staff_paks = {{
    {"WHammer",  16020 + 64 * 30, 56},
    {"WBHammer", 16020 + 64 * 31, 56},
    {"Wstaff1",  16020 + 64 * 35, 56},
    {"Wstaff2",  16020 + 64 * 36, 56},
}};

// Female weapons - bow
static constexpr std::array<pak_load_entry, 2> female_bow_paks = {{
    {"Wbo", 16020 + 64 * 40, 56},
    {"Wbo", 16020 + 64 * 41, 56},
}};

// Male shields (Msh.pak) - 9 types, 7 sprites each
static constexpr pak_load_entry male_shield_pak = {"Msh", 9100, 72};

// Female shields (Wsh.pak)
static constexpr pak_load_entry female_shield_pak = {"Wsh", 19100, 72};

// Male mantles
static constexpr std::array<pak_load_entry, 3> male_mantle_paks = {{
    {"Mmantle01", 9230 + 15 * 1, 12},
    {"Mmantle02", 9230 + 15 * 2, 12},
    {"Mmantle03", 9230 + 15 * 3, 12},
}};

// Female mantles
static constexpr std::array<pak_load_entry, 3> female_mantle_paks = {{
    {"Wmantle01", 19230 + 15 * 1, 12},
    {"Wmantle02", 19230 + 15 * 2, 12},
    {"Wmantle03", 19230 + 15 * 3, 12},
}};

// Male helmets
static constexpr std::array<pak_load_entry, 8> male_helmet_paks = {{
    {"MHelm1",  9300 + 15 * 1, 12},
    {"MHelm2",  9300 + 15 * 2, 12},
    {"MHelm3",  9300 + 15 * 3, 12},
    {"MHelm4",  9300 + 15 * 4, 12},
    {"NMHelm1", 9300 + 15 * 5, 12},
    {"NMHelm2", 9300 + 15 * 6, 12},
    {"NMHelm3", 9300 + 15 * 7, 12},
    {"NMHelm4", 9300 + 15 * 8, 12},
}};

// Female helmets
static constexpr std::array<pak_load_entry, 6> female_helmet_paks = {{
    {"WHelm1",  19300 + 15 * 1, 12},
    {"WHelm4",  19300 + 15 * 4, 12},
    {"NWHelm1", 19300 + 15 * 5, 12},
    {"NWHelm2", 19300 + 15 * 6, 12},
    {"NWHelm3", 19300 + 15 * 7, 12},
    {"NWHelm4", 19300 + 15 * 8, 12},
}};

// Helper to load a PAK and store sprites at global IDs
static bool load_pak_at_offset(sprite_manager& sprites, const pak_load_entry& entry) {
    std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";

    if (!sprites.load_pak(entry.pak_name, pak_path)) {
        spdlog::debug("Optional PAK not found: {}", pak_path);
        return false;
    }

    // Store each sprite at its global ID
    for (uint32_t i = 0; i < entry.sprite_count; ++i) {
        uint16_t global_id = static_cast<uint16_t>(entry.sprite_id + i);
        sprites.store_sprite_at_id(global_id, entry.pak_name, i);
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

bool menu_character_renderer::initialize(sprite_manager& sprites) {
    spdlog::info("Initializing menu character renderer with equipment support...");

    // Load body sprites
    load_pak_array(sprites, male_body_paks);
    load_pak_array(sprites, female_body_paks);

    // Load base appearance (underwear, hair)
    load_pak_at_offset(sprites, male_underwear_pak);
    load_pak_at_offset(sprites, male_hair_pak);
    load_pak_at_offset(sprites, female_underwear_pak);
    load_pak_at_offset(sprites, female_hair_pak);

    // Load armor
    load_pak_array(sprites, male_body_armor_paks);
    load_pak_array(sprites, female_body_armor_paks);
    load_pak_array(sprites, male_arm_armor_paks);
    load_pak_array(sprites, female_arm_armor_paks);
    load_pak_array(sprites, male_pants_paks);
    load_pak_array(sprites, female_pants_paks);
    load_pak_array(sprites, male_boots_paks);
    load_pak_array(sprites, female_boots_paks);

    // Load weapons
    load_pak_array(sprites, male_sword_paks);
    load_pak_array(sprites, male_axe_paks);
    load_pak_array(sprites, male_hammer_staff_paks);
    load_pak_array(sprites, male_bow_paks);
    load_pak_array(sprites, female_sword_paks);
    load_pak_array(sprites, female_axe_paks);
    load_pak_array(sprites, female_hammer_staff_paks);
    load_pak_array(sprites, female_bow_paks);

    // Load shields
    load_pak_at_offset(sprites, male_shield_pak);
    load_pak_at_offset(sprites, female_shield_pak);

    // Load mantles
    load_pak_array(sprites, male_mantle_paks);
    load_pak_array(sprites, female_mantle_paks);

    // Load helmets
    load_pak_array(sprites, male_helmet_paks);
    load_pak_array(sprites, female_helmet_paks);

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
        draw_hair(rend, sprites, x, y, gender, appearance.hair_style, action, dir, frm);
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

void menu_character_renderer::draw_hair(renderer& rend, sprite_manager& sprites, int32_t x, int32_t y,
                                        uint8_t gender, uint8_t hair_style, int32_t action, int32_t dir, int32_t frame) {
    uint16_t base = is_female(gender) ? female_hair_base : male_hair_base;
    int32_t style = std::clamp(int32_t(hair_style), 0, 7);

    // Hair sprite ID: base + style * 15 + action
    uint16_t sprite_id = static_cast<uint16_t>(base + style * hair_stride + action);
    int32_t frame_index = calc_frame(dir, frame);

    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr && spr->is_loaded()) {
        rend.draw_sprite(*spr, x, y, frame_index);
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
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
    if (spr && spr->is_loaded()) {
        rend.draw_sprite(*spr, x, y, frame_index);
    }
}

} // namespace hb
