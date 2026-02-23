#pragma once

#include <cstdint>
#include <string_view>

namespace hb
{

// Game state machine
enum class game_state : int8_t
{
    null_state = -2,
    quit = -1,
    main_menu = 0,
    connecting = 1,
    loading = 2,
    waiting_init_data = 3,
    playing = 4,
    connection_lost = 5,
    message = 6,
    create_account = 7,
    login = 8,
    query_force_login = 9,
    select_character = 10,
    create_character = 11,
    waiting_response = 12,
    query_delete_character = 13,
    log_response_message = 14,
    change_password = 15,
    version_mismatch = 17,
    introduction = 18,
    agreement = 19,
    select_server = 20,
    input_key_code = 21
};

// Server types
enum class server_type : uint8_t
{
    game = 1,
    log = 2
};

// Object actions - values match sprite set layout (0-11)
// Each value corresponds directly to a group of 8 direction sprites
enum class object_action : uint8_t
{
    stop_peace = 0,        // Sprites 0-7: Idle in peace mode
    stop_combat = 1,       // Sprites 8-15: Idle in combat mode
    move_peace = 2,        // Sprites 16-23: Walk in peace mode
    move_combat = 3,       // Sprites 24-31: Walk in combat mode
    run = 4,               // Sprites 32-39: Run (shared for both modes)
    attack_peace = 5,      // Sprites 40-47: Attack in peace mode
    attack_combat = 6,     // Sprites 48-55: Attack in combat mode
    attack_combat_bow = 7, // Sprites 56-63: Bow attack in combat mode
    magic = 8,             // Sprites 64-71: Magic casting
    get_item = 9,          // Sprites 72-79: Pick up item
    damage = 10,           // Sprites 80-87: Taking damage / knockback
    dying = 11,            // Sprites 88-95: Death animation

    null_action = 100
};

// Legacy action aliases for network protocol compatibility
inline constexpr object_action legacy_stop = object_action::stop_peace;
inline constexpr object_action legacy_move = object_action::move_peace;
inline constexpr object_action legacy_attack = object_action::attack_peace;
inline constexpr object_action legacy_attack_move = object_action::attack_peace;
inline constexpr object_action legacy_damage_move = object_action::damage;
inline constexpr object_action legacy_dead = object_action::dying;

// Cursor status
enum class cursor_status : uint8_t
{
    null_status = 0,
    pressed = 1,
    selected = 2,
    dragging = 3
};

// Selected object type
enum class selected_object_type : uint8_t
{
    none = 0,
    dialog_box = 1,
    item = 2
};

// Equipment position (v2 item system)
enum class equip_pos : uint8_t
{
    none = 0,
    head, body, arms, pants, boots,
    weapon, shield, twohand,
    ring_left, ring_right,
    amulet, cape, angel,
    full_body
};

inline constexpr std::string_view equip_pos_to_string(equip_pos pos)
{
    switch (pos)
    {
    case equip_pos::none:       return "none";
    case equip_pos::head:       return "head";
    case equip_pos::body:       return "body";
    case equip_pos::arms:       return "arms";
    case equip_pos::pants:      return "pants";
    case equip_pos::boots:      return "boots";
    case equip_pos::weapon:     return "weapon";
    case equip_pos::shield:     return "shield";
    case equip_pos::twohand:    return "twohand";
    case equip_pos::ring_left:  return "ring_left";
    case equip_pos::ring_right: return "ring_right";
    case equip_pos::amulet:     return "amulet";
    case equip_pos::cape:       return "cape";
    case equip_pos::angel:      return "angel";
    case equip_pos::full_body:  return "full_body";
    }
    return "none";
}

inline constexpr equip_pos equip_pos_from_string(std::string_view s)
{
    if (s == "head")       return equip_pos::head;
    if (s == "body")       return equip_pos::body;
    if (s == "arms")       return equip_pos::arms;
    if (s == "pants")      return equip_pos::pants;
    if (s == "boots")      return equip_pos::boots;
    if (s == "weapon")     return equip_pos::weapon;
    if (s == "shield")     return equip_pos::shield;
    if (s == "twohand")    return equip_pos::twohand;
    if (s == "ring_left")  return equip_pos::ring_left;
    if (s == "ring_right") return equip_pos::ring_right;
    if (s == "amulet")     return equip_pos::amulet;
    if (s == "cape")       return equip_pos::cape;
    if (s == "angel")      return equip_pos::angel;
    if (s == "full_body")  return equip_pos::full_body;
    return equip_pos::none;
}

// Item types (v2 item system)
enum class item_type : uint8_t
{
    none = 0,
    weapon,
    armor,
    accessory,
    consumable,
    material
};

inline constexpr std::string_view item_type_to_string(item_type type)
{
    switch (type)
    {
    case item_type::none:       return "none";
    case item_type::weapon:     return "weapon";
    case item_type::armor:      return "armor";
    case item_type::accessory:  return "accessory";
    case item_type::consumable: return "consumable";
    case item_type::material:   return "material";
    }
    return "none";
}

inline constexpr item_type item_type_from_string(std::string_view s)
{
    if (s == "weapon")     return item_type::weapon;
    if (s == "armor")      return item_type::armor;
    if (s == "accessory")  return item_type::accessory;
    if (s == "consumable") return item_type::consumable;
    if (s == "material")   return item_type::material;
    return item_type::none;
}

// Weapon types (v2 item system)
enum class weapon_type : uint8_t
{
    none = 0,
    sword, axe, hammer, staff, wand, bow, dagger, fist
};

inline constexpr std::string_view weapon_type_to_string(weapon_type type)
{
    switch (type)
    {
    case weapon_type::none:    return "none";
    case weapon_type::sword:   return "sword";
    case weapon_type::axe:     return "axe";
    case weapon_type::hammer:  return "hammer";
    case weapon_type::staff:   return "staff";
    case weapon_type::wand:    return "wand";
    case weapon_type::bow:     return "bow";
    case weapon_type::dagger:  return "dagger";
    case weapon_type::fist:    return "fist";
    }
    return "none";
}

inline constexpr weapon_type weapon_type_from_string(std::string_view s)
{
    if (s == "sword")  return weapon_type::sword;
    if (s == "axe")    return weapon_type::axe;
    if (s == "hammer") return weapon_type::hammer;
    if (s == "staff")  return weapon_type::staff;
    if (s == "wand")   return weapon_type::wand;
    if (s == "bow")    return weapon_type::bow;
    if (s == "dagger") return weapon_type::dagger;
    if (s == "fist")   return weapon_type::fist;
    return weapon_type::none;
}

// Magic types
enum class magic_type : uint8_t
{
    damage_spot = 1,
    hp_up_spot = 2,
    damage_area = 3,
    sp_down_spot = 4,
    sp_down_area = 5,
    sp_up_spot = 6,
    sp_up_area = 7,
    teleport = 8,
    summon = 9,
    create = 10,
    protect = 11,
    hold_object = 12,
    invisibility = 13,
    create_dynamic = 14,
    possession = 15,
    confuse = 16,
    poison = 17,
    berserk = 18,
    bloody_shock_wave = 19,
    polymorph = 20,
    damage_area_no_spot = 21,
    tremor = 22,
    ice = 23,
    earthworm_strike = 25,
    armor_break = 26,
    blizzard = 27,
    cancellation = 28,
    inhibition_casting = 29,
    earth_shock_wave = 30,
    mass_magic_missile = 31,
    resurrection = 32
};

// Direction (8-way, values 1-8 matching legacy code)
// Use std::optional<direction> where "no direction" is needed
enum class direction : uint8_t
{
    north = 1,
    north_east = 2,
    east = 3,
    south_east = 4,
    south = 5,
    south_west = 6,
    west = 7,
    north_west = 8
};

// Dynamic object types
enum class dynamic_object : uint8_t
{
    fire = 1,
    fish = 2,
    fish_object = 3,
    mineral1 = 4,
    mineral2 = 5,
    ice_storm = 8,
    spike = 9,
    poison_cloud_begin = 10,
    poison_cloud_loop = 11,
    poison_cloud_end = 12,
    fire2 = 13
};

// Login result codes (from login_response namespace in protocol.hpp)
enum class login_result : uint16_t
{
    success = 0x0F14,
    reject = 0x0F15,
    password_mismatch = 0x0F16,
    not_existing_account = 0x0F17,
    account_locked = 0x0F31,
    service_not_available = 0x0F32
};

// Character creation result codes
enum class character_result : uint16_t
{
    success = 0x0F1C,
    failed = 0x0F1D,
    already_exists = 0x0F1E,
    deleted = 0x0F1F
};

// Entity hostility (server-authoritative)
enum class hostility : uint8_t
{
    neutral = 0,
    friendly = 1,
    enemy = 2
};

// Player PK status
enum class pk_status : uint8_t
{
    innocent = 0,
    criminal = 1,
    murderer = 2
};

inline hostility hostility_from_string(std::string_view s)
{
    if (s == "friendly")
        return hostility::friendly;
    if (s == "enemy")
        return hostility::enemy;
    return hostility::neutral;
}

inline pk_status pk_status_from_string(std::string_view s)
{
    if (s == "criminal")
        return pk_status::criminal;
    if (s == "murderer")
        return pk_status::murderer;
    return pk_status::innocent;
}

} // namespace hb
