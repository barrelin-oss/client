#pragma once

#include <cstddef>
#include <cstdint>

namespace hb
{

// Protocol constants
inline constexpr uint16_t default_login_port = 2748;
inline constexpr uint16_t default_game_port = 2848;
inline constexpr size_t max_packet_size = 40960;  // 8192 * 5 from XSocket.cpp
inline constexpr size_t packet_header_size = 3;   // 1 byte key + 2 bytes size
inline constexpr uint32_t protocol_version = 351; // Client version from Game.h

// ============================================================================
// MESSAGE IDs - From NetMessages.h
// ============================================================================

// Init messages
inline constexpr uint32_t msg_request_init_player = 0x05040205;
inline constexpr uint32_t msg_response_init_player = 0x05040206;
inline constexpr uint32_t msg_request_init_data = 0x05080404;
inline constexpr uint32_t msg_response_init_data = 0x05080405;

// Motion/Movement messages
inline constexpr uint32_t msg_command_motion = 0x0FA314D5;
inline constexpr uint32_t msg_response_motion = 0x0FA314D6;
inline constexpr uint32_t msg_event_motion = 0x0FA314D7;
inline constexpr uint32_t msg_event_log = 0x0FA314D8;
inline constexpr uint32_t msg_event_common = 0x0FA314DB;
inline constexpr uint32_t msg_command_common = 0x0FA314DC;

// Notification message (main gameplay notifications)
inline constexpr uint32_t msg_notify = 0x0FA314D0;

// Configuration content messages
inline constexpr uint32_t msg_item_config_contents = 0x0FA314D9;
inline constexpr uint32_t msg_npc_config_contents = 0x0FA314DA;
inline constexpr uint32_t msg_magic_config_contents = 0x0FA314DB;
inline constexpr uint32_t msg_skill_config_contents = 0x0FA314DC;
inline constexpr uint32_t msg_player_item_list_contents = 0x0FA314DD;
inline constexpr uint32_t msg_portion_config_contents = 0x0FA314DE;
inline constexpr uint32_t msg_player_character_contents = 0x0FA40000;
inline constexpr uint32_t msg_quest_config_contents = 0x0FA40001;
inline constexpr uint32_t msg_build_item_config_contents = 0x0FA40002;

// Chat messages
inline constexpr uint32_t msg_check_connection = 0x03203203;
inline constexpr uint32_t msg_chat = 0x03203204;

// Login messages
inline constexpr uint32_t msg_request_login = 0x0FC94201;
inline constexpr uint32_t msg_request_create_account = 0x0FC94202;
inline constexpr uint32_t msg_response_log = 0x0FC94203;
inline constexpr uint32_t msg_request_create_character = 0x0FC94204;
inline constexpr uint32_t msg_request_enter_game = 0x0FC94205;
inline constexpr uint32_t msg_response_enter_game = 0x0FC94206;
inline constexpr uint32_t msg_request_delete_character = 0x0FC94207;

// Guild messages
inline constexpr uint32_t msg_request_create_guild = 0x0FC94208;
inline constexpr uint32_t msg_response_create_guild = 0x0FC94209;
inline constexpr uint32_t msg_request_disband_guild = 0x0FC9420A;
inline constexpr uint32_t msg_response_disband_guild = 0x0FC9420B;
inline constexpr uint32_t msg_request_guild_add_member = 0x0FC9420C;
inline constexpr uint32_t msg_request_guild_del_member = 0x0FC9420D;

// Password messages
inline constexpr uint32_t msg_request_change_password = 0x0FC94210;
inline constexpr uint32_t msg_response_change_password = 0x0FC94211;

// Player data messages
inline constexpr uint32_t msg_request_player_data = 0x0C152210;
inline constexpr uint32_t msg_response_player_data = 0x0C152211;
inline constexpr uint32_t msg_request_save_player_data = 0x0DF3076F;
inline constexpr uint32_t msg_request_full_object_data = 0x0DF40000;

// Teleport messages
inline constexpr uint32_t msg_request_teleport = 0x0EA03201;
inline constexpr uint32_t msg_request_teleport_list = 0x0EA03202;
inline constexpr uint32_t msg_response_teleport_list = 0x0EA03203;

// Server messages
inline constexpr uint32_t msg_game_server_alive = 0x12A01002;
inline constexpr uint32_t msg_admin_user = 0x12A01003;
inline constexpr uint32_t msg_game_server_down = 0x12A01004;
inline constexpr uint32_t msg_server_shutdown = 0x14000000;

// ============================================================================
// COMMON COMMAND TYPES (subtypes of msg_command_common / msg_event_common)
// ============================================================================

namespace common_type
{
inline constexpr uint16_t item_drop = 0x0A01;
inline constexpr uint16_t equip_item = 0x0A02;
inline constexpr uint16_t req_list_contents = 0x0A03;
inline constexpr uint16_t req_purchase_item = 0x0A04;
inline constexpr uint16_t give_item_to_char = 0x0A05;
inline constexpr uint16_t join_guild_approve = 0x0A06;
inline constexpr uint16_t join_guild_reject = 0x0A07;
inline constexpr uint16_t dismiss_guild_approve = 0x0A08;
inline constexpr uint16_t dismiss_guild_reject = 0x0A09;
inline constexpr uint16_t release_item = 0x0A0A;
inline constexpr uint16_t toggle_combat_mode = 0x0A0B;
inline constexpr uint16_t set_item = 0x0A0C;
inline constexpr uint16_t magic = 0x0A0D;
inline constexpr uint16_t req_study_magic = 0x0A0E;
inline constexpr uint16_t req_train_skill = 0x0A0F;
inline constexpr uint16_t req_get_reward_money = 0x0A10;
inline constexpr uint16_t req_use_item = 0x0A11;
inline constexpr uint16_t req_use_skill = 0x0A12;
inline constexpr uint16_t req_sell_item = 0x0A13;
inline constexpr uint16_t req_repair_item = 0x0A14;
inline constexpr uint16_t req_sell_item_confirm = 0x0A15;
inline constexpr uint16_t req_repair_item_confirm = 0x0A16;
inline constexpr uint16_t req_get_fish = 0x0A17;
inline constexpr uint16_t toggle_safe_attack_mode = 0x0A18;
inline constexpr uint16_t req_create_portion = 0x0A19;
inline constexpr uint16_t talk_to_npc = 0x0A1A;
inline constexpr uint16_t req_set_down_skill_index = 0x0A1B;
inline constexpr uint16_t exchange_item_to_char = 0x0A1E;
inline constexpr uint16_t set_exchange_item = 0x0A1F;
inline constexpr uint16_t confirm_exchange_item = 0x0A20;
inline constexpr uint16_t cancel_exchange_item = 0x0A21;
inline constexpr uint16_t quest_accepted = 0x0A22;
inline constexpr uint16_t build_item = 0x0A23;
inline constexpr uint16_t get_magic_ability = 0x0A24;
inline constexpr uint16_t request_exchange = 0x0A25;
inline constexpr uint16_t req_repair_all = 0x0A26;
inline constexpr uint16_t manu_item = 0x0A27;
inline constexpr uint16_t req_set_manu_item = 0x0A28;

// Party operations
inline constexpr uint16_t request_accept_join_party = 0x0A30;
inline constexpr uint16_t request_join_party = 0x0A31;
inline constexpr uint16_t response_join_party = 0x0A32;
inline constexpr uint16_t request_activate_special_ability = 0x0A40;
inline constexpr uint16_t request_cancel_quest = 0x0A50;
inline constexpr uint16_t request_select_crusade_duty = 0x0A51;
inline constexpr uint16_t request_map_status = 0x0A52;
inline constexpr uint16_t request_help = 0x0A53;
inline constexpr uint16_t set_guild_teleport_loc = 0x0A54;
inline constexpr uint16_t guild_teleport = 0x0A55;
inline constexpr uint16_t summon_war_unit = 0x0A56;
inline constexpr uint16_t upgrade_item = 0x0A58;
inline constexpr uint16_t request_hunt_mode = 0x0A60;
} // namespace common_type

// ============================================================================
// NOTIFICATION TYPES (subtypes of msg_notify)
// ============================================================================

namespace notify_type
{
inline constexpr uint16_t item_obtained = 0x0B01;
inline constexpr uint16_t query_join_guild_permission = 0x0B02;
inline constexpr uint16_t query_dismiss_guild_permission = 0x0B03;
inline constexpr uint16_t wait_for_guild_operation = 0x0B04;
inline constexpr uint16_t cannot_carry_more_item = 0x0B05;
inline constexpr uint16_t item_purchased = 0x0B06;
inline constexpr uint16_t hp = 0x0B07;
inline constexpr uint16_t not_enough_gold = 0x0B08;
inline constexpr uint16_t killed = 0x0B09;
inline constexpr uint16_t exp = 0x0B0A;
inline constexpr uint16_t guild_disbanded = 0x0B0B;
inline constexpr uint16_t event_msg_string = 0x0B0C;
inline constexpr uint16_t cannot_join_more_guildsman = 0x0B0D;
inline constexpr uint16_t new_guildsman = 0x0B0E;
inline constexpr uint16_t dismiss_guildsman = 0x0B0F;
inline constexpr uint16_t magic_study_success = 0x0B10;
inline constexpr uint16_t magic_study_fail = 0x0B11;
inline constexpr uint16_t skill_train_success = 0x0B12;
inline constexpr uint16_t skill_train_fail = 0x0B13;
inline constexpr uint16_t mp = 0x0B14;
inline constexpr uint16_t sp = 0x0B15;
inline constexpr uint16_t level_up = 0x0B16;
inline constexpr uint16_t item_lifespan_end = 0x0B17;
inline constexpr uint16_t limited_level = 0x0B18;
inline constexpr uint16_t item_to_bank = 0x0B19;
inline constexpr uint16_t pk_penalty = 0x0B1A;
inline constexpr uint16_t pk_captured = 0x0B1B;
inline constexpr uint16_t enemy_kill_reward = 0x0B1C;
inline constexpr uint16_t give_item_fin_erase_item = 0x0B1D;
inline constexpr uint16_t drop_item_fin_erase_item = 0x0B1F;
inline constexpr uint16_t item_depleted_erase_item = 0x0B20;
inline constexpr uint16_t new_dynamic_object = 0x0B21;
inline constexpr uint16_t del_dynamic_object = 0x0B22;
inline constexpr uint16_t skill = 0x0B23;
inline constexpr uint16_t server_change = 0x0B24;
inline constexpr uint16_t set_item_count = 0x0B25;
inline constexpr uint16_t cannot_item_to_bank = 0x0B26;
inline constexpr uint16_t magic_effect_on = 0x0B27;
inline constexpr uint16_t magic_effect_off = 0x0B28;
inline constexpr uint16_t total_users = 0x0B29;
inline constexpr uint16_t skill_using_end = 0x0B2A;
inline constexpr uint16_t show_map = 0x0B2B;
inline constexpr uint16_t cannot_sell_item = 0x0B2C;
inline constexpr uint16_t sell_item_price = 0x0B2D;
inline constexpr uint16_t cannot_repair_item = 0x0B2E;
inline constexpr uint16_t repair_item_price = 0x0B2F;
inline constexpr uint16_t item_repaired = 0x0B30;
inline constexpr uint16_t item_sold = 0x0B31;
inline constexpr uint16_t charisma = 0x0B32;
inline constexpr uint16_t player_on_game = 0x0B33;
inline constexpr uint16_t player_not_on_game = 0x0B34;
inline constexpr uint16_t whisper_mode_on = 0x0B35;
inline constexpr uint16_t whisper_mode_off = 0x0B36;
inline constexpr uint16_t player_profile = 0x0B37;
inline constexpr uint16_t hunger = 0x0B39;
inline constexpr uint16_t to_be_recalled = 0x0B40;
inline constexpr uint16_t time_change = 0x0B41;
inline constexpr uint16_t player_shutup = 0x0B42;
inline constexpr uint16_t fish_chance = 0x0B48;
inline constexpr uint16_t fish_success = 0x0B4A;
inline constexpr uint16_t fish_fail = 0x0B4B;
inline constexpr uint16_t weather_change = 0x0B4D;
inline constexpr uint16_t server_shutdown = 0x0B4E;
inline constexpr uint16_t reward_gold = 0x0B4F;
inline constexpr uint16_t safe_attack_mode = 0x0B51;
inline constexpr uint16_t super_attack_left = 0x0B52;
inline constexpr uint16_t portion_success = 0x0B56;
inline constexpr uint16_t npc_talk = 0x0B57;
inline constexpr uint16_t enemy_kills = 0x0B5A;
inline constexpr uint16_t item_released = 0x0B5C;
inline constexpr uint16_t open_exchange_window = 0x0B5E;
inline constexpr uint16_t set_exchange_item = 0x0B5F;
inline constexpr uint16_t cancel_exchange_item = 0x0B60;
inline constexpr uint16_t exchange_item_complete = 0x0B61;
inline constexpr uint16_t item_color_change = 0x0B65;
inline constexpr uint16_t quest_contents = 0x0B66;
inline constexpr uint16_t quest_completed = 0x0B68;
inline constexpr uint16_t quest_reward = 0x0B69;
inline constexpr uint16_t build_item_success = 0x0B70;
inline constexpr uint16_t build_item_fail = 0x0B71;
inline constexpr uint16_t observer_mode = 0x0B72;
inline constexpr uint16_t global_attack_mode = 0x0B73;
inline constexpr uint16_t damage_move = 0x0B74;
inline constexpr uint16_t force_disconn = 0x0B75;

// Party notifications
inline constexpr uint16_t response_create_new_party = 0x0B80;
inline constexpr uint16_t query_join_party = 0x0B81;

// Special ability notifications
inline constexpr uint16_t energy_sphere_created = 0x0B90;
inline constexpr uint16_t energy_sphere_goal_in = 0x0B91;
inline constexpr uint16_t special_ability_enabled = 0x0B92;
inline constexpr uint16_t special_ability_status = 0x0B93;

// Crusade/War notifications
inline constexpr uint16_t crusade = 0x0B94;
inline constexpr uint16_t locked_map = 0x0B95;
inline constexpr uint16_t duty_selected = 0x0B96;
inline constexpr uint16_t map_status_next = 0x0B97;
inline constexpr uint16_t map_status_last = 0x0B98;
inline constexpr uint16_t meteor_strike_coming = 0x0B9B;
inline constexpr uint16_t meteor_strike_hit = 0x0B9C;
inline constexpr uint16_t grand_magic_result = 0x0B9D;
inline constexpr uint16_t party = 0x0BA2;
inline constexpr uint16_t item_attribute_change = 0x0BA3;
inline constexpr uint16_t item_upgrade_fail = 0x0BA8;
} // namespace notify_type

// ============================================================================
// LOGIN RESPONSE TYPES
// ============================================================================

namespace login_response
{
inline constexpr uint16_t confirm = 0x0F14;
inline constexpr uint16_t reject = 0x0F15;
inline constexpr uint16_t password_mismatch = 0x0F16;
inline constexpr uint16_t not_existing_account = 0x0F17;
inline constexpr uint16_t new_account_created = 0x0F18;
inline constexpr uint16_t new_account_failed = 0x0F19;
inline constexpr uint16_t already_existing_account = 0x0F1A;
inline constexpr uint16_t not_existing_character = 0x0F1B;
inline constexpr uint16_t new_character_created = 0x0F1C;
inline constexpr uint16_t new_character_failed = 0x0F1D;
inline constexpr uint16_t already_existing_character = 0x0F1E;
inline constexpr uint16_t character_deleted = 0x0F1F;
inline constexpr uint16_t account_locked = 0x0F31;
inline constexpr uint16_t service_not_available = 0x0F32;
} // namespace login_response

// ============================================================================
// ENTER GAME RESPONSE TYPES
// ============================================================================

namespace enter_game_response
{
inline constexpr uint16_t playing = 0x0F20;
inline constexpr uint16_t reject = 0x0F21;
inline constexpr uint16_t confirm = 0x0F22;
inline constexpr uint16_t force_disconn = 0x0F23;
} // namespace enter_game_response

} // namespace hb
