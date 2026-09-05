#pragma once

#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hb
{

using json = nlohmann::json;

// Message types
namespace msg_type
{
inline constexpr const char* login_request = "login_request";
inline constexpr const char* login_response = "login_response";
inline constexpr const char* get_characters_request = "get_characters_request";
inline constexpr const char* get_characters_response = "get_characters_response";
inline constexpr const char* enter_game_request = "enter_game_request";
inline constexpr const char* enter_game_response = "enter_game_response";
inline constexpr const char* create_character_request = "create_character_request";
inline constexpr const char* create_character_response = "create_character_response";
inline constexpr const char* delete_character_request = "delete_character_request";
inline constexpr const char* delete_character_response = "delete_character_response";
inline constexpr const char* set_view_range = "set_view_range";

// Item pickup / drop (v2)
inline constexpr const char* pickup_request = "pickup_request";
inline constexpr const char* pickup_result = "pickup_result";
inline constexpr const char* drop_request = "drop_request";
inline constexpr const char* drop_result = "drop_result";
inline constexpr const char* ground_item_removed = "ground_item_removed";

// Player actions
inline constexpr const char* player_move_request = "player_move_request";
inline constexpr const char* player_move_response = "player_move_response";
inline constexpr const char* player_stop_request = "player_stop_request";
inline constexpr const char* player_stop_response = "player_stop_response";
inline constexpr const char* player_position_update = "player_position_update";

// Player state updates
inline constexpr const char* hunger_update = "hunger_update";
inline constexpr const char* stat_update = "stat_update";
inline constexpr const char* entity_hp_update = "entity_hp_update";

// Inventory (v2)
inline constexpr const char* inventory_data = "inventory_data";
inline constexpr const char* inventory_item_add = "inventory_item_add";
inline constexpr const char* inventory_item_update = "inventory_item_update";
inline constexpr const char* inventory_item_removed = "inventory_item_removed";
inline constexpr const char* inventory_item_delta = "inventory_item_delta";
inline constexpr const char* inventory_gold_update = "inventory_gold_update";
inline constexpr const char* inventory_weight_update = "inventory_weight_update";
inline constexpr const char* inventory_reposition_request = "inventory_reposition_request";

// Equipment (v2)
inline constexpr const char* equip_request = "equip_request";
inline constexpr const char* equip_result = "equip_result";
inline constexpr const char* unequip_request = "unequip_request";
inline constexpr const char* unequip_result = "unequip_result";
inline constexpr const char* force_unequip = "force_unequip";
inline constexpr const char* equipment_change = "equipment_change";

// Skills
inline constexpr const char* skills_data = "skills_data";
inline constexpr const char* skill_progress = "skill_progress";
inline constexpr const char* player_skill_response = "player_skill_response";

// NPC messages
inline constexpr const char* npc_move = "npc_move";
inline constexpr const char* npc_spawn = "npc_spawn";
inline constexpr const char* npc_despawn = "npc_despawn";
inline constexpr const char* npc_death = "npc_death";

// Entity visibility
inline constexpr const char* entity_spawn = "entity_spawn";

// Ground items
inline constexpr const char* ground_item_spawn = "ground_item_spawn";

// NPC interaction
inline constexpr const char* player_interact_request = "player_interact_request";
inline constexpr const char* player_interact_response = "player_interact_response";

// System
inline constexpr const char* pong = "pong";
inline constexpr const char* error_msg = "error";

// View range (fixing literal strings in dispatch)
inline constexpr const char* view_range_update = "view_range_update";
inline constexpr const char* command_response = "command_response";

// Entity info request/response (for when we receive updates for unknown entities)
inline constexpr const char* entity_info_request = "entity_info_request";
inline constexpr const char* entity_info_response = "entity_info_response";

// Chat
inline constexpr const char* chat_message = "chat_message";
inline constexpr const char* chat_message_broadcast = "chat_message_broadcast";

// Server-controlled render mode
inline constexpr const char* set_render_mode = "set_render_mode";

// Environment
inline constexpr const char* environment_update = "environment_update";

// Client preferences
inline constexpr const char* set_chat_preferences = "set_chat_preferences";

// Combat
inline constexpr const char* player_attack_request = "player_attack_request";
inline constexpr const char* player_attack_response = "player_attack_response";
inline constexpr const char* combat_attack_broadcast = "combat_attack_broadcast";
inline constexpr const char* npc_attack = "npc_attack";
inline constexpr const char* entity_death = "entity_death";
inline constexpr const char* entity_despawn = "entity_despawn";
inline constexpr const char* combat_effect = "combat_effect";
inline constexpr const char* player_death_info = "player_death_info";
inline constexpr const char* player_respawn_request = "respawn_request";
inline constexpr const char* respawn_response = "respawn_response";
inline constexpr const char* player_teleport = "player_teleport";
inline constexpr const char* combat_mode_change_request = "combat_mode_change_request";
inline constexpr const char* combat_mode_change_response = "combat_mode_change_response";
inline constexpr const char* combat_mode_change_broadcast = "combat_mode_change_broadcast";
inline constexpr const char* player_action_broadcast = "player_action_broadcast";

// Magic
inline constexpr const char* player_magic_request = "player_magic_request";
inline constexpr const char* player_magic_response = "player_magic_response";
inline constexpr const char* spell_list_update = "spell_list_update";

// Fishing
inline constexpr const char* fish_skill_request = "fish_skill_request";
inline constexpr const char* fish_skill_response = "fish_skill_response";
inline constexpr const char* fish_engaged = "fish_engaged";
inline constexpr const char* fish_chance_update = "fish_chance_update";
inline constexpr const char* fish_catch_request = "fish_catch_request";
inline constexpr const char* fish_catch_response = "fish_catch_response";
inline constexpr const char* fish_spawn_broadcast = "fish_spawn_broadcast";
inline constexpr const char* fish_despawn_broadcast = "fish_despawn_broadcast";

// Guild
inline constexpr const char* guild_create_request = "guild_create_request";
inline constexpr const char* guild_create_response = "guild_create_response";
inline constexpr const char* guild_disband_request = "guild_disband_request";
inline constexpr const char* guild_disband_response = "guild_disband_response";
inline constexpr const char* guild_leave_request = "guild_leave_request";
inline constexpr const char* guild_leave_response = "guild_leave_response";
inline constexpr const char* guild_kick_request = "guild_kick_request";
inline constexpr const char* guild_kick_response = "guild_kick_response";
inline constexpr const char* guild_invite_request = "guild_invite_request";
inline constexpr const char* guild_invite_response = "guild_invite_response";
inline constexpr const char* guild_invite_received = "guild_invite_received";
inline constexpr const char* guild_invite_respond_request = "guild_invite_respond_request";
inline constexpr const char* guild_invite_respond_response = "guild_invite_respond_response";
inline constexpr const char* guild_promote_request = "guild_promote_request";
inline constexpr const char* guild_promote_response = "guild_promote_response";
inline constexpr const char* guild_demote_request = "guild_demote_request";
inline constexpr const char* guild_demote_response = "guild_demote_response";
inline constexpr const char* guild_set_motd_request = "guild_set_motd_request";
inline constexpr const char* guild_set_motd_response = "guild_set_motd_response";
inline constexpr const char* guild_info_request = "guild_info_request";
inline constexpr const char* guild_info_response = "guild_info_response";
inline constexpr const char* guild_update = "guild_update";

// NPC dialog choices, quests and party (JSON) - network/messages/quest.hpp
inline constexpr const char* dialog_choice_request = "dialog_choice_request";
inline constexpr const char* dialog_choice_response = "dialog_choice_response";
inline constexpr const char* quest_list_request = "quest_list_request";
inline constexpr const char* quest_list_response = "quest_list_response";
inline constexpr const char* quest_accept_request = "quest_accept_request";
inline constexpr const char* quest_accept_response = "quest_accept_response";
inline constexpr const char* quest_abandon_request = "quest_abandon_request";
inline constexpr const char* quest_abandon_response = "quest_abandon_response";
inline constexpr const char* quest_complete_request = "quest_complete_request";
inline constexpr const char* quest_complete_response = "quest_complete_response";
inline constexpr const char* quest_journal_request = "quest_journal_request";
inline constexpr const char* quest_journal_response = "quest_journal_response";
inline constexpr const char* quest_update = "quest_update";
inline constexpr const char* party_invite_request = "party_invite_request";
inline constexpr const char* party_invite_response = "party_invite_response";
inline constexpr const char* party_invite_notice = "party_invite_notice";
inline constexpr const char* party_accept_request = "party_accept_request";
inline constexpr const char* party_accept_response = "party_accept_response";
inline constexpr const char* party_leave_request = "party_leave_request";
inline constexpr const char* party_leave_response = "party_leave_response";
inline constexpr const char* party_update = "party_update";

// Command autocomplete
inline constexpr const char* available_commands = "available_commands";
inline constexpr const char* command_availability_update = "command_availability_update";
} // namespace msg_type

// Message builder helper
class message_builder
{
public:
    message_builder(const char* type)
    {
        msg_["type"] = type;
        msg_["seq"] = next_seq_++;
    }

    template<typename T> message_builder& set(const std::string& key, const T& value)
    {
        if (!msg_.contains("data"))
        {
            msg_["data"] = json::object();
        }
        msg_["data"][key] = value;
        return *this;
    }

    json build() const { return msg_; }

    static void reset_sequence() { next_seq_ = 1; }

private:
    json msg_;
    static inline uint32_t next_seq_ = 1;
};

} // namespace hb
