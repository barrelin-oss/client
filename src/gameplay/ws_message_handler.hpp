#pragma once

#include "network/messages.hpp"
#include "ui/screens/character_create_screen.hpp"
#include <atomic>
#include <mutex>
#include <string>

namespace hb
{

class entity;
class entity_manager;
class game_state_manager;

// Handles all WebSocket JSON messages received from the server.
// Owns session/disconnect state that bridges the background WS thread to the main thread.
class ws_message_handler
{
public:
    ws_message_handler() = default;

    void initialize(game_state_manager& game);
    void clear();

    // Main dispatch entry point (called from game_state_manager::update)
    void handle_message(const json& message);

    // Session state
    const std::string& session_token() const { return session_token_; }
    void clear_session();

    // Pending login state (set before connect, consumed on connect event)
    void set_pending_login(const std::string& username, const std::string& password);
    bool consume_pending_login(std::string& username, std::string& password);

    // WebSocket requests
    void request_characters();
    void request_enter_game(int32_t character_id, bool force_disconnect = false);
    void request_create_character(const character_create_data& data);
    void request_delete_character(int32_t character_id);
    void request_entity_info(uint32_t entity_id);
    void send_view_range();
    void send_chat_preferences();

    // Combat
    void request_combat_mode_toggle();
    void request_attack(uint32_t target_id, uint8_t attack_type = 0);

    // Magic
    void request_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id = 0);

    // Chat
    void send_chat_message(std::string_view content, std::string_view channel, std::string_view recipient = "");

    // Fishing
    void request_fish_skill();
    void request_fish_catch();

    // Guild
    void request_guild_create(std::string_view name, std::string_view tag);
    void request_guild_disband();
    void request_guild_leave();
    void request_guild_kick(std::string_view target);
    void request_guild_invite(std::string_view target);
    void request_guild_invite_respond(bool accept);
    void request_guild_promote(std::string_view target);
    void request_guild_demote(std::string_view target);
    void request_guild_set_motd(std::string_view motd);
    void request_guild_info();

private:
    // Individual message handlers
    void handle_login_response_ws(const json& message);
    void handle_get_characters_response(const json& message);
    void handle_enter_game_response(const json& message);
    void handle_create_character_response(const json& message);
    void handle_delete_character_response(const json& message);
    void handle_pickup_response(const json& message);
    void handle_ground_item_removed(const json& message);
    void handle_player_position_update(const json& message);
    void handle_player_stop_response(const json& message);
    void handle_player_move_response(const json& message);
    void handle_hunger_update(const json& message);
    void handle_npc_move(const json& message);
    void handle_entity_info_response(const json& message);
    void handle_set_render_mode(const json& message);
    void handle_view_range_update(const json& message);
    void handle_command_response(const json& message);
    void handle_chat_message_broadcast(const json& message);
    void handle_combat_attack_broadcast(const json& message);
    void handle_player_attack_response(const json& message);
    void handle_npc_attack(const json& message);
    void handle_environment_update(const json& message);
    void handle_entity_death(const json& message);
    void handle_entity_despawn(const json& message);
    void handle_combat_effect(const json& message);
    void handle_player_death_info(const json& message);
    void handle_respawn_response(const json& message);
    void handle_player_teleport(const json& message);
    void handle_player_magic_response(const json& message);
    void handle_spell_list_update(const json& message);
    void handle_entity_spawn(const json& message);
    void handle_npc_spawn(const json& message);
    void handle_npc_despawn(const json& message);
    void handle_ground_item_spawn(const json& message);
    void handle_stat_update(const json& message);
    void handle_entity_hp_update(const json& message);
    void handle_equipment_change_broadcast(const json& message);
    void handle_combat_mode_change_response(const json& message);
    void handle_combat_mode_change_broadcast(const json& message);
    void handle_player_action_broadcast(const json& message);
    void handle_player_equip_response(const json& message);
    void handle_player_unequip_response(const json& message);
    void handle_inventory_data(const json& message);
    void handle_equipment_data(const json& message);
    void handle_drop_item_response(const json& message);
    void handle_inventory_item_update(const json& message);
    void handle_inventory_item_removed(const json& message);
    void handle_inventory_weight_update(const json& message);
    void handle_skills_data(const json& message);
    void handle_skill_progress(const json& message);
    void handle_player_skill_response(const json& message);
    void handle_player_interact_response(const json& message);
    void handle_fish_skill_response(const json& message);
    void handle_fish_engaged(const json& message);
    void handle_fish_chance_update(const json& message);
    void handle_fish_catch_response(const json& message);
    void handle_fish_spawn_broadcast(const json& message);
    void handle_fish_despawn_broadcast(const json& message);
    void handle_guild_create_response(const json& message);
    void handle_guild_disband_response(const json& message);
    void handle_guild_leave_response(const json& message);
    void handle_guild_kick_response(const json& message);
    void handle_guild_invite_response(const json& message);
    void handle_guild_invite_received(const json& message);
    void handle_guild_invite_respond_response(const json& message);
    void handle_guild_promote_response(const json& message);
    void handle_guild_demote_response(const json& message);
    void handle_guild_set_motd_response(const json& message);
    void handle_guild_info_response(const json& message);
    void handle_guild_update(const json& message);
    void handle_available_commands(const json& message);
    void handle_command_availability_update(const json& message);
    void handle_pong(const json& message);
    void handle_error(const json& message);

    // Shared spawn helpers
    static void init_entity_transform(entity& ent, int16_t x, int16_t y, int direction);
    static void init_entity_visual_type(entity& ent, int16_t sprite_id, uint32_t template_id,
                                        const std::string& hostility);
    static void init_entity_dead_state(entity& ent, entity_manager& entities);

    game_state_manager* game_ = nullptr;

    // Session state
    std::string session_token_;
    std::string pending_username_;
    std::string pending_password_;
    std::atomic<bool> pending_login_on_connect_{false};

    // Character enter-game retry
    int32_t pending_enter_game_character_id_ = 0;
};

} // namespace hb
