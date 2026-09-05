#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "core/config.hpp"
#include "core/direction_utils.hpp"
#include "graphics/renderer.hpp"
#include <spdlog/spdlog.h>
#include <string_view>
#include <unordered_map>

namespace hb
{

void ws_message_handler::init_entity_transform(entity& ent, int16_t x, int16_t y, int direction)
{
    auto& t = ent.transform();
    t.tile_x = x;
    t.tile_y = y;
    t.move_start_x = x;
    t.move_start_y = y;
    t.x = x * hb::tile_width + 16;
    t.y = y * hb::tile_height + 16;
    t.facing = direction_from_protocol(direction).value_or(direction::south);
}

void ws_message_handler::init_entity_visual_type(entity& ent, int16_t sprite_id, uint32_t template_id,
                                                  const std::string& hostility)
{
    uint16_t visual_type =
        sprite_id > 0 ? static_cast<uint16_t>(sprite_id) : static_cast<uint16_t>(template_id);

    if (visual_type > 0)
    {
        ent.set_type(visual_type);
        if (ent.has_npc())
            ent.npc().npc_type = visual_type;
        if (ent.has_monster())
        {
            ent.monster().monster_type = visual_type;
            ent.monster().hostile = hostility_from_string(hostility);
        }
    }
}

void ws_message_handler::init_entity_dead_state(entity& ent, entity_manager& entities)
{
    ent.set_action(object_action::dying);
    auto& anim = ent.animation();
    anim.current_frame = anim.frame_count > 0 ? anim.frame_count - 1 : 0;
    anim.finished = true;
    entities.transition_to_dead(ent.id());
}

void ws_message_handler::initialize(game_state_manager& game)
{
    game_ = &game;
}

void ws_message_handler::clear()
{
    session_token_.clear();
    pending_username_.clear();
    pending_password_.clear();
    pending_login_on_connect_.store(false);
    pending_enter_game_character_id_ = 0;
}

void ws_message_handler::clear_session()
{
    session_token_.clear();
    pending_username_.clear();
    pending_password_.clear();
}

void ws_message_handler::set_pending_login(const std::string& username, const std::string& password)
{
    pending_username_ = username;
    pending_password_ = password;
    pending_login_on_connect_.store(true);
}

bool ws_message_handler::consume_pending_login(std::string& username, std::string& password)
{
    if (pending_login_on_connect_.exchange(false))
    {
        username = pending_username_;
        password = pending_password_;
        return true;
    }
    return false;
}

void ws_message_handler::handle_message(const json& message)
{
    using handler_fn = void (ws_message_handler::*)(const json&);

    // clang-format off
    static const std::unordered_map<std::string_view, handler_fn> dispatch = {
        // Auth / lobby
        {msg_type::login_response,              &ws_message_handler::handle_login_response_ws},
        {msg_type::get_characters_response,     &ws_message_handler::handle_get_characters_response},
        {msg_type::enter_game_response,         &ws_message_handler::handle_enter_game_response},
        {msg_type::create_character_response,   &ws_message_handler::handle_create_character_response},
        {msg_type::delete_character_response,   &ws_message_handler::handle_delete_character_response},

        // Player movement / position
        {msg_type::player_position_update,      &ws_message_handler::handle_player_position_update},
        {msg_type::player_stop_response,        &ws_message_handler::handle_player_stop_response},
        {msg_type::player_move_response,        &ws_message_handler::handle_player_move_response},
        {msg_type::player_teleport,             &ws_message_handler::handle_player_teleport},

        // Items / ground
        {msg_type::pickup_result,               &ws_message_handler::handle_pickup_result},
        {msg_type::drop_result,                 &ws_message_handler::handle_drop_result},
        {msg_type::ground_item_removed,         &ws_message_handler::handle_ground_item_removed},
        {msg_type::ground_item_spawn,           &ws_message_handler::handle_ground_item_spawn},

        // Entity lifecycle
        {msg_type::entity_spawn,                &ws_message_handler::handle_entity_spawn},
        {msg_type::entity_despawn,              &ws_message_handler::handle_entity_despawn},
        {msg_type::entity_death,                &ws_message_handler::handle_entity_death},
        {msg_type::entity_info_response,        &ws_message_handler::handle_entity_info_response},
        {msg_type::entity_hp_update,            &ws_message_handler::handle_entity_hp_update},

        // NPC
        {msg_type::npc_spawn,                   &ws_message_handler::handle_npc_spawn},
        {msg_type::npc_despawn,                 &ws_message_handler::handle_npc_despawn},
        {msg_type::npc_move,                    &ws_message_handler::handle_npc_move},
        {msg_type::npc_attack,                  &ws_message_handler::handle_npc_attack},

        // Combat
        {msg_type::combat_attack_broadcast,     &ws_message_handler::handle_combat_attack_broadcast},
        {msg_type::player_attack_response,      &ws_message_handler::handle_player_attack_response},
        {msg_type::combat_effect,               &ws_message_handler::handle_combat_effect},
        {msg_type::combat_mode_change_response, &ws_message_handler::handle_combat_mode_change_response},
        {msg_type::combat_mode_change_broadcast,&ws_message_handler::handle_combat_mode_change_broadcast},
        {msg_type::player_death_info,           &ws_message_handler::handle_player_death_info},
        {msg_type::respawn_response,            &ws_message_handler::handle_respawn_response},

        // Magic
        {msg_type::player_magic_response,       &ws_message_handler::handle_player_magic_response},
        {msg_type::spell_list_update,           &ws_message_handler::handle_spell_list_update},

        // Chat
        {msg_type::chat_message_broadcast,      &ws_message_handler::handle_chat_message_broadcast},
        {msg_type::chat_message,                nullptr}, // ack from server, handled via local echo

        // Stats / inventory / equipment
        {msg_type::stat_update,                 &ws_message_handler::handle_stat_update},
        {msg_type::hunger_update,               &ws_message_handler::handle_hunger_update},
        {msg_type::inventory_data,              &ws_message_handler::handle_inventory_data},
        {msg_type::inventory_item_add,          &ws_message_handler::handle_inventory_item_add},
        {msg_type::inventory_item_update,       &ws_message_handler::handle_inventory_item_update},
        {msg_type::inventory_item_removed,      &ws_message_handler::handle_inventory_item_removed},
        {msg_type::inventory_item_delta,        &ws_message_handler::handle_inventory_item_delta},
        {msg_type::inventory_gold_update,       &ws_message_handler::handle_inventory_gold_update},
        {msg_type::inventory_weight_update,     &ws_message_handler::handle_inventory_weight_update},
        {msg_type::equip_result,                &ws_message_handler::handle_equip_result},
        {msg_type::unequip_result,              &ws_message_handler::handle_unequip_result},
        {msg_type::force_unequip,               &ws_message_handler::handle_force_unequip},
        {msg_type::equipment_change,            &ws_message_handler::handle_equipment_change},

        // Skills
        {msg_type::skills_data,                 &ws_message_handler::handle_skills_data},
        {msg_type::skill_progress,              &ws_message_handler::handle_skill_progress},
        {msg_type::player_skill_response,       &ws_message_handler::handle_player_skill_response},

        // Actions / interaction
        {msg_type::player_action_broadcast,     &ws_message_handler::handle_player_action_broadcast},
        {msg_type::player_interact_response,    &ws_message_handler::handle_player_interact_response},

        // Fishing
        {msg_type::fish_skill_response,         &ws_message_handler::handle_fish_skill_response},
        {msg_type::fish_engaged,                &ws_message_handler::handle_fish_engaged},
        {msg_type::fish_chance_update,          &ws_message_handler::handle_fish_chance_update},
        {msg_type::fish_catch_response,         &ws_message_handler::handle_fish_catch_response},
        {msg_type::fish_spawn_broadcast,        &ws_message_handler::handle_fish_spawn_broadcast},
        {msg_type::fish_despawn_broadcast,      &ws_message_handler::handle_fish_despawn_broadcast},

        // Guild
        {msg_type::guild_create_response,       &ws_message_handler::handle_guild_create_response},
        {msg_type::guild_disband_response,      &ws_message_handler::handle_guild_disband_response},
        {msg_type::guild_leave_response,        &ws_message_handler::handle_guild_leave_response},
        {msg_type::guild_kick_response,         &ws_message_handler::handle_guild_kick_response},
        {msg_type::guild_invite_response,       &ws_message_handler::handle_guild_invite_response},
        {msg_type::guild_invite_received,       &ws_message_handler::handle_guild_invite_received},
        {msg_type::guild_invite_respond_response, &ws_message_handler::handle_guild_invite_respond_response},
        {msg_type::guild_promote_response,      &ws_message_handler::handle_guild_promote_response},
        {msg_type::guild_demote_response,       &ws_message_handler::handle_guild_demote_response},
        {msg_type::guild_set_motd_response,     &ws_message_handler::handle_guild_set_motd_response},
        {msg_type::guild_info_response,         &ws_message_handler::handle_guild_info_response},
        {msg_type::guild_update,                &ws_message_handler::handle_guild_update},

        // NPC dialog, quests, party
        {msg_type::dialog_choice_response,      &ws_message_handler::handle_dialog_choice_response},
        {msg_type::quest_list_response,         &ws_message_handler::handle_quest_list_response},
        {msg_type::quest_accept_response,       &ws_message_handler::handle_quest_accept_response},
        {msg_type::quest_abandon_response,      &ws_message_handler::handle_quest_abandon_response},
        {msg_type::quest_complete_response,     &ws_message_handler::handle_quest_complete_response},
        {msg_type::quest_journal_response,      &ws_message_handler::handle_quest_journal_response},
        {msg_type::quest_update,                &ws_message_handler::handle_quest_update},
        {msg_type::party_invite_response,       &ws_message_handler::handle_party_invite_response},
        {msg_type::party_invite_notice,         &ws_message_handler::handle_party_invite_notice},
        {msg_type::party_accept_response,       &ws_message_handler::handle_party_accept_response},
        {msg_type::party_leave_response,        &ws_message_handler::handle_party_leave_response},
        {msg_type::party_update,                &ws_message_handler::handle_party_update},

        // Shops and bank
        {msg_type::shop_buy_response,           &ws_message_handler::handle_shop_buy_response},
        {msg_type::shop_sell_response,          &ws_message_handler::handle_shop_sell_response},
        {msg_type::shop_sell_confirm_response,  &ws_message_handler::handle_shop_sell_confirm_response},
        {msg_type::bank_deposit_response,       &ws_message_handler::handle_bank_deposit_response},
        {msg_type::bank_withdraw_response,      &ws_message_handler::handle_bank_withdraw_response},

        // Environment / rendering
        {msg_type::environment_update,          &ws_message_handler::handle_environment_update},
        {msg_type::set_render_mode,             &ws_message_handler::handle_set_render_mode},
        {msg_type::view_range_update,           &ws_message_handler::handle_view_range_update},

        // Commands
        {msg_type::command_response,            &ws_message_handler::handle_command_response},
        {msg_type::available_commands,          &ws_message_handler::handle_available_commands},
        {msg_type::command_availability_update, &ws_message_handler::handle_command_availability_update},

        // Misc
        {msg_type::pong,                        &ws_message_handler::handle_pong},
        {msg_type::error_msg,                   &ws_message_handler::handle_error},
    };
    // clang-format on

    try
    {
        if (!message.contains("type"))
        {
            spdlog::warn("Received message without type field");
            return;
        }

        const auto& type = message["type"].get_ref<const std::string&>();
        spdlog::debug("[ws recv] {}: {}", type, message.dump());

        auto it = dispatch.find(type);
        if (it == dispatch.end())
        {
            spdlog::warn("Unknown message type: {}", type);
            return;
        }

        if (it->second)
            (this->*(it->second))(message);
    }
    catch (const nlohmann::json::exception& e)
    {
        spdlog::error("JSON parse error handling message: {}", e.what());
    }
    catch (const std::exception& e)
    {
        spdlog::error("Error handling message: {}", e.what());
    }
}

void ws_message_handler::send_view_range()
{
    // Send actual screen resolution so the server can compute visibility radii.
    auto* rend = game_->get_renderer();
    uint32_t w = rend ? rend->display_width() : config::instance().video().screen_width;
    uint32_t h = rend ? rend->display_height() : config::instance().video().screen_height;
    json msg = make_set_view_range_request(w, h);
    game_->ws_connection().send(msg);
    spdlog::info("Sent view range: {}x{}", w, h);
}

void ws_message_handler::send_chat_preferences()
{
    bool filter = config::instance().chat().filter_profanity;
    json msg = make_set_chat_preferences_request(filter);
    game_->ws_connection().send(msg);
    spdlog::info("Sent chat preferences: filter_profanity={}", filter);
}

} // namespace hb
