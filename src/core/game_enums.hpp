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

// Equipment slots
enum class equip_slot : uint8_t
{
    none = 0,
    head = 1,
    body = 2,
    arms = 3,
    pants = 4,
    boots = 5,
    neck = 6,
    left_hand = 7,
    right_hand = 8,
    two_hand = 9,
    right_finger = 10,
    left_finger = 11,
    back = 12,
    full_body = 13
};

// Convert a server equipment slot index to equip_slot.
// Server protocol uses 0=head,1=body,...,10=back; returns equip_slot::none for unknown values.
inline constexpr equip_slot equip_slot_from_server(int server_slot)
{
    switch (server_slot)
    {
    case 0: return equip_slot::head;
    case 1: return equip_slot::body;
    case 2: return equip_slot::arms;
    case 3: return equip_slot::pants;
    case 4: return equip_slot::boots;
    case 5: return equip_slot::right_hand;
    case 6: return equip_slot::left_hand;
    case 7: return equip_slot::right_finger;
    case 8: return equip_slot::left_finger;
    case 9: return equip_slot::neck;
    case 10: return equip_slot::back;
    default: return equip_slot::none;
    }
}

// Item types
enum class item_type : uint8_t
{
    none = 0,
    equip = 1,
    apply = 2,
    use_deplete = 3,
    install = 4,
    consume = 5,
    arrow = 6,
    eat = 7,
    use_skill = 8,
    use_perm = 9,
    use_skill_enable_dialog = 10,
    use_deplete_dest = 11,
    material = 12
};

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
