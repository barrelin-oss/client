#pragma once

#include <cstdint>

namespace hb {

// Game state machine
enum class game_state : int8_t {
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
enum class server_type : uint8_t {
    game = 1,
    log = 2
};

// Object actions
enum class object_action : uint8_t {
    stop = 0,
    move = 1,
    run = 2,
    attack = 3,
    magic = 4,
    get_item = 5,
    damage = 6,
    damage_move = 7,
    attack_move = 8,
    dying = 10,
    null_action = 100,
    dead = 101
};

// Cursor status
enum class cursor_status : uint8_t {
    null_status = 0,
    pressed = 1,
    selected = 2,
    dragging = 3
};

// Selected object type
enum class selected_object_type : uint8_t {
    none = 0,
    dialog_box = 1,
    item = 2
};

// Equipment slots
enum class equip_slot : uint8_t {
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

// Item types
enum class item_type : uint8_t {
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
enum class magic_type : uint8_t {
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
    polymorph = 20,
    damage_area_no_spot = 21,
    tremor = 22,
    ice = 23
};

// Direction (8-way + none)
enum class direction : uint8_t {
    none = 0,
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
enum class dynamic_object : uint8_t {
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
enum class login_result : uint16_t {
    success = 0x0F14,
    reject = 0x0F15,
    password_mismatch = 0x0F16,
    not_existing_account = 0x0F17,
    account_locked = 0x0F31,
    service_not_available = 0x0F32
};

// Character creation result codes
enum class character_result : uint16_t {
    success = 0x0F1C,
    failed = 0x0F1D,
    already_exists = 0x0F1E,
    deleted = 0x0F1F
};

} // namespace hb
