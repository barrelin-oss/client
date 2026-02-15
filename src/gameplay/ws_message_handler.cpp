#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialogs/death_dialog.hpp"
#include "world/ground_item.hpp"
#include "audio/sound_types.hpp"
#include "graphics/renderer.hpp"
#include "graphics/color_utils.hpp"
#include "core/config.hpp"
#include "core/direction_utils.hpp"
#include "core/game_enums.hpp"
#include "chat/chat_message.hpp"
#include "ui/dialogs/chat_dialog.hpp"
#include "ui/dialogs/skills_dialog.hpp"
#include "ui/dialogs/spellbook_dialog.hpp"
#include "world/tile.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

namespace
{

time_of_day hour_to_time_of_day(uint8_t hour)
{
    if (hour < 5)
        return time_of_day::midnight;
    if (hour < 7)
        return time_of_day::dawn;
    if (hour < 10)
        return time_of_day::morning;
    if (hour < 14)
        return time_of_day::noon;
    if (hour < 17)
        return time_of_day::afternoon;
    if (hour < 19)
        return time_of_day::dusk;
    if (hour < 23)
        return time_of_day::night;
    return time_of_day::midnight;
}

} // anonymous namespace

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
    try
    {
        if (!message.contains("type"))
        {
            spdlog::warn("Received message without type field");
            return;
        }

        std::string type = message["type"].get<std::string>();
        spdlog::debug("[ws recv] {}: {}", type, message.dump());

        if (type == msg_type::login_response)
            handle_login_response_ws(message);
        else if (type == msg_type::get_characters_response)
            handle_get_characters_response(message);
        else if (type == msg_type::enter_game_response)
            handle_enter_game_response(message);
        else if (type == msg_type::create_character_response)
            handle_create_character_response(message);
        else if (type == msg_type::delete_character_response)
            handle_delete_character_response(message);
        else if (type == msg_type::player_pickup_response)
            handle_pickup_response(message);
        else if (type == msg_type::ground_item_removed)
            handle_ground_item_removed(message);
        else if (type == msg_type::player_position_update)
            handle_player_position_update(message);
        else if (type == msg_type::player_stop_response)
            handle_player_stop_response(message);
        else if (type == msg_type::player_move_response)
            handle_player_move_response(message);
        else if (type == msg_type::hunger_update)
            handle_hunger_update(message);
        else if (type == msg_type::npc_move)
            handle_npc_move(message);
        else if (type == msg_type::entity_info_response)
            handle_entity_info_response(message);
        else if (type == msg_type::set_render_mode)
            handle_set_render_mode(message);
        else if (type == msg_type::chat_message_broadcast)
            handle_chat_message_broadcast(message);
        else if (type == msg_type::combat_attack_broadcast)
            handle_combat_attack_broadcast(message);
        else if (type == msg_type::player_attack_response)
            handle_player_attack_response(message);
        else if (type == msg_type::npc_attack)
            handle_npc_attack(message);
        else if (type == msg_type::entity_death)
            handle_entity_death(message);
        else if (type == msg_type::entity_despawn)
            handle_entity_despawn(message);
        else if (type == msg_type::combat_effect)
            handle_combat_effect(message);
        else if (type == msg_type::player_death_info)
            handle_player_death_info(message);
        else if (type == msg_type::respawn_response)
            handle_respawn_response(message);
        else if (type == msg_type::player_teleport)
            handle_player_teleport(message);
        else if (type == msg_type::player_magic_response)
            handle_player_magic_response(message);
        else if (type == msg_type::spell_list_update)
            handle_spell_list_update(message);
        else if (type == msg_type::chat_message)
        {
        } // Ack from server, already handled via local echo
        else if (type == msg_type::environment_update)
            handle_environment_update(message);
        else if (type == msg_type::view_range_update)
            handle_view_range_update(message);
        else if (type == msg_type::command_response)
            handle_command_response(message);
        else if (type == msg_type::entity_spawn)
            handle_entity_spawn(message);
        else if (type == msg_type::npc_spawn)
            handle_npc_spawn(message);
        else if (type == msg_type::npc_despawn)
            handle_npc_despawn(message);
        else if (type == msg_type::ground_item_spawn)
            handle_ground_item_spawn(message);
        else if (type == msg_type::stat_update)
            handle_stat_update(message);
        else if (type == msg_type::entity_hp_update)
            handle_entity_hp_update(message);
        else if (type == msg_type::equipment_change_broadcast)
            handle_equipment_change_broadcast(message);
        else if (type == msg_type::combat_mode_change_response)
            handle_combat_mode_change_response(message);
        else if (type == msg_type::combat_mode_change_broadcast)
            handle_combat_mode_change_broadcast(message);
        else if (type == msg_type::player_action_broadcast)
            handle_player_action_broadcast(message);
        else if (type == msg_type::player_equip_response)
            handle_player_equip_response(message);
        else if (type == msg_type::player_unequip_response)
            handle_player_unequip_response(message);
        else if (type == msg_type::inventory_data)
            handle_inventory_data(message);
        else if (type == msg_type::equipment_data)
            handle_equipment_data(message);
        else if (type == msg_type::skills_data)
            handle_skills_data(message);
        else if (type == msg_type::player_skill_response)
            handle_player_skill_response(message);
        else if (type == msg_type::player_interact_response)
            handle_player_interact_response(message);
        else if (type == msg_type::fish_skill_response)
            handle_fish_skill_response(message);
        else if (type == msg_type::fish_engaged)
            handle_fish_engaged(message);
        else if (type == msg_type::fish_chance_update)
            handle_fish_chance_update(message);
        else if (type == msg_type::fish_catch_response)
            handle_fish_catch_response(message);
        else if (type == msg_type::fish_spawn_broadcast)
            handle_fish_spawn_broadcast(message);
        else if (type == msg_type::fish_despawn_broadcast)
            handle_fish_despawn_broadcast(message);
        else if (type == msg_type::guild_create_response)
            handle_guild_create_response(message);
        else if (type == msg_type::guild_disband_response)
            handle_guild_disband_response(message);
        else if (type == msg_type::guild_leave_response)
            handle_guild_leave_response(message);
        else if (type == msg_type::guild_kick_response)
            handle_guild_kick_response(message);
        else if (type == msg_type::guild_invite_response)
            handle_guild_invite_response(message);
        else if (type == msg_type::guild_invite_received)
            handle_guild_invite_received(message);
        else if (type == msg_type::guild_invite_respond_response)
            handle_guild_invite_respond_response(message);
        else if (type == msg_type::guild_promote_response)
            handle_guild_promote_response(message);
        else if (type == msg_type::guild_demote_response)
            handle_guild_demote_response(message);
        else if (type == msg_type::guild_set_motd_response)
            handle_guild_set_motd_response(message);
        else if (type == msg_type::guild_info_response)
            handle_guild_info_response(message);
        else if (type == msg_type::guild_update)
            handle_guild_update(message);
        else if (type == msg_type::available_commands)
            handle_available_commands(message);
        else if (type == msg_type::command_availability_update)
            handle_command_availability_update(message);
        else if (type == msg_type::pong)
            handle_pong(message);
        else if (type == msg_type::error_msg)
            handle_error(message);
        else
            spdlog::warn("Unknown message type: {}", type);
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

void ws_message_handler::handle_login_response_ws(const json& message)
{
    auto response = login_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Login successful! Session: {}", response.session_token);
        session_token_ = response.session_token;

        pending_username_.clear();
        pending_password_.clear();

        game_->ui().show_connection_dialog(nullptr);
        request_characters();
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Login failed" : response.error_message;
        spdlog::warn("Login failed: {}", error_msg);
        game_->show_error(error_msg);

        pending_username_.clear();
        pending_password_.clear();
    }
}

void ws_message_handler::handle_get_characters_response(const json& message)
{
    auto response = get_characters_response_data::from_json(message);

    game_->ui().hide_connection_dialog();

    if (response.success)
    {
        std::vector<character_info> chars;
        for (const auto& sc : response.characters)
        {
            character_info info;
            info.id = sc.id;
            info.name = sc.name;
            info.level = static_cast<uint16_t>(sc.level);
            info.warrior = (sc.class_type == 0);
            info.gender = static_cast<uint8_t>(sc.gender == 0 ? 1 : 2);
            info.skin_color = static_cast<uint8_t>(sc.skin_color);
            info.hair_style = static_cast<uint8_t>(sc.hair_style);
            info.hair_color = static_cast<uint8_t>(sc.hair_color);
            info.underwear_color = static_cast<uint8_t>(sc.underwear_color);
            info.body_armor = sc.body_armor;
            info.arm_armor = sc.arm_armor;
            info.pants = sc.pants;
            info.boots = sc.boots;
            info.helmet = sc.helmet;
            info.mantle = sc.mantle;
            info.weapon = sc.weapon;
            info.shield = sc.shield;
            chars.push_back(info);
        }

        game_->set_characters(std::move(chars));
        spdlog::info("Received {} characters", game_->characters().size());
        game_->change_state(game_state::select_character);
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Failed to get characters" : response.error_message;
        spdlog::warn("Get characters failed: {}", error_msg);
        game_->show_error(error_msg);
    }
}

void ws_message_handler::handle_enter_game_response(const json& message)
{
    auto response = enter_game_response_data::from_json(message);

    game_->ui().hide_connection_dialog();

    if (!response.success)
    {
        std::string error_msg = response.error_message.empty() ? "Failed to enter game" : response.error_message;
        spdlog::warn("Enter game failed: {}", error_msg);

        if (response.error_message == "account_already_in_game")
        {
            int32_t char_id = pending_enter_game_character_id_;
            game_->ui().create_confirm_box("Session Conflict",
                                           "This account is already logged in.\nDisconnect other session?",
                                           [this, char_id](bool confirmed)
                                           {
                                               if (confirmed)
                                               {
                                                   request_enter_game(char_id, true);
                                               }
                                           });
            return;
        }

        game_->show_error(error_msg);
        return;
    }

    const auto& ch = response.character;
    spdlog::info("Entering game as '{}' on map: {} at ({}, {})", ch.name, ch.map_name, ch.pos_x, ch.pos_y);

    auto& entities = game_->entities();
    auto& sprites = game_->sprites();

    // Create local player entity
    auto& player = entities.create_entity_with_id(ch.id, entity_type::player);
    entities.set_local_player(ch.id);

    auto& transform = player.transform();
    transform.tile_x = ch.pos_x;
    transform.tile_y = ch.pos_y;
    transform.move_start_x = ch.pos_x;
    transform.move_start_y = ch.pos_y;
    transform.x = ch.pos_x * 32 + 16;
    transform.y = ch.pos_y * 32 + 16;
    spdlog::info("PLAYER SPAWN: tile=({},{}) world=({},{})", ch.pos_x, ch.pos_y, transform.x, transform.y);

    auto& name = player.name();
    name.name = ch.name;
    name.faction = ch.faction;
    name.guild_name = ch.guild_name;
    name.guild_tag = ch.guild_tag;
    name.guild_rank_id = ch.guild_rank;

    // Initialize guild system from enter_game data
    if (!ch.guild_name.empty())
    {
        game_->guild().set_guild(ch.guild_name, ch.guild_tag, ch.guild_rank);
    }

    auto& stats = player.stats();
    stats.level = static_cast<uint16_t>(ch.level);
    stats.strength = static_cast<uint16_t>(ch.str);
    stats.vitality = static_cast<uint16_t>(ch.vit);
    stats.dexterity = static_cast<uint16_t>(ch.dex);
    stats.intelligence = static_cast<uint16_t>(ch.int_);
    stats.magic = static_cast<uint16_t>(ch.mag);
    stats.charisma = static_cast<uint16_t>(ch.cha);
    stats.hp = ch.hp;
    stats.max_hp = ch.hp_max;
    stats.mp = ch.mp;
    stats.max_mp = ch.mp_max;
    stats.sp = ch.sp;
    stats.max_sp = ch.sp_max;
    stats.experience = ch.experience;
    stats.hunger = static_cast<uint8_t>(ch.hunger_level);

    auto& combat = player.combat();
    combat.pk_count = ch.pk_count;

    auto& sprite = player.sprite();
    sprite.skin_color = static_cast<uint8_t>(ch.skin_color);
    sprite.hair_style = static_cast<uint8_t>(ch.hair_style);
    sprite.hair_color = static_cast<uint8_t>(ch.hair_color);
    sprite.underwear_color = static_cast<uint8_t>(ch.underwear_color);
    sprite.gender = (ch.gender == 0) ? 1 : 2;

    entities.load_character_sprites(player, sprites);

    auto& anim = player.animation();
    anim.set_state(entity_anim_state::stop);

    spdlog::debug("Player stats - Level: {}, STR: {}, DEX: {}, VIT: {}, INT: {}, MAG: {}, CHA: {}",
                  stats.level,
                  stats.strength,
                  stats.dexterity,
                  stats.vitality,
                  stats.intelligence,
                  stats.magic,
                  stats.charisma);
    spdlog::debug("Player vitals - HP: {}/{}, MP: {}/{}, SP: {}/{}",
                  stats.hp,
                  stats.max_hp,
                  stats.mp,
                  stats.max_mp,
                  stats.sp,
                  stats.max_sp);

    // Populate inventory
    auto& inventory = game_->inventory();
    inventory.clear();
    inventory.set_gold(static_cast<uint32_t>(ch.gold));

    for (const auto& inv_item : response.inventory.items)
    {
        item itm;
        itm.id = inv_item.item_id;
        itm.type_id = static_cast<uint16_t>(inv_item.item_id);
        itm.name = inv_item.name;
        itm.amount = static_cast<uint32_t>(inv_item.count);
        itm.durability = static_cast<uint16_t>(inv_item.durability);
        itm.max_durability = static_cast<uint16_t>(inv_item.max_durability);
        inventory.set_item_at(inv_item.slot, itm);
    }
    spdlog::debug("Loaded {} inventory items, {} gold", response.inventory.items.size(), ch.gold);

    // Set equipped items
    for (const auto& eq_item : response.equipment)
    {
        item itm;
        itm.id = eq_item.item_id;
        itm.type_id = static_cast<uint16_t>(eq_item.item_id);
        itm.name = eq_item.name;
        itm.durability = static_cast<uint16_t>(eq_item.durability);
        itm.max_durability = static_cast<uint16_t>(eq_item.max_durability);

        auto slot = equip_slot_from_server(eq_item.slot);
        if (slot != equip_slot::none)
        {
            inventory.set_equipped(slot, itm);
        }
    }
    spdlog::debug("Loaded {} equipped items", response.equipment.size());

    // Set skill levels
    auto& skills = game_->skills();
    spdlog::info("enter_game_response contains {} skills", response.skills.size());
    for (const auto& sk : response.skills)
    {
        uint8_t mastery = static_cast<uint8_t>(std::min(static_cast<int16_t>(100), sk.level));
        skills.set_mastery(sk.skill_id, mastery);

        float progress = 0.0f;
        if (sk.uses_to_next_level > 0)
            progress = static_cast<float>(sk.uses_this_level) / static_cast<float>(sk.uses_to_next_level);
        skills.set_sub_progress(sk.skill_id, progress);

        spdlog::info("  enter_game skill: id={} level={} mastery={} uses={}/{} progress={:.2f}",
                     sk.skill_id,
                     sk.level,
                     mastery,
                     sk.uses_this_level,
                     sk.uses_to_next_level,
                     progress);
    }

    // Populate skills dialog
    if (auto* skill_dlg = dynamic_cast<skills_dialog*>(game_->ui().get_dialog(dialog_type::skills)))
    {
        auto all = skills.get_all_skills();
        std::vector<skill> skill_list;
        skill_list.reserve(all.size());
        for (const auto* s : all)
            skill_list.push_back(*s);
        skill_dlg->set_skills(skill_list);
        spdlog::info("Pushed {} skills to dialog from enter_game", skill_list.size());
    }
    else
    {
        spdlog::warn("Skills dialog not found during enter_game!");
    }

    // Set known spells from server
    auto& magic = game_->magic();
    for (const auto& sp : response.spells)
    {
        magic.learn_spell(sp.spell_id);
        magic.set_spell_mastery(sp.spell_id, static_cast<uint8_t>(sp.level));
        magic.set_spell_total_casts(sp.spell_id, sp.total_casts);
    }
    spdlog::info(
        "Loaded {} spells from server, {} learned total", response.spells.size(), magic.get_learned_spells().size());

    // Auto-populate spell hotbar from learned spells
    game_->auto_populate_spell_hotbar();

    // Populate spellbook dialog with all spell definitions
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(game_->ui().get_dialog(dialog_type::spellbook)))
    {
        spell_dlg->clear_spells();
        auto all_spells = magic.get_all_spells();
        for (const auto* sp : all_spells)
        {
            spell_dlg->add_spell(*sp);
        }
        spdlog::info("Populated spellbook dialog with {} spells ({} learned)",
                     all_spells.size(),
                     magic.get_learned_spells().size());
    }
    else
    {
        spdlog::warn("Spellbook dialog not found!");
    }

    // Load quest data
    auto& quests = game_->quests();
    quests.clear();
    for (const auto& aq : response.quests.active)
    {
        active_quest quest;
        quest.quest_id = aq.quest_id;
        quest.status = aq.status;
        for (const auto& obj : aq.objectives)
        {
            quest.objectives.push_back({obj.id, obj.status, obj.current, obj.required});
        }
        quests.active.push_back(std::move(quest));
    }
    quests.completed = response.quests.completed;
    spdlog::debug("Loaded {} active quests, {} completed quests", quests.active.size(), quests.completed.size());

    // Nearby entities are now sent as individual entity_spawn/npc_spawn/ground_item_spawn
    // messages by the server, so no entity list parsing needed here.

    // Load map
    auto& world = game_->game_world();
    if (!world.load_map(ch.map_name))
    {
        spdlog::warn("Failed to load map data for '{}', continuing without tile data", ch.map_name);
    }

    // Apply initial environment state from enter_game_response
    {
        uint8_t hour = response.world.time_hour;
        uint8_t minute = response.world.time_minute;
        uint8_t weather = response.world.weather;

        world.set_clock(hour, minute);
        world.set_time(hour_to_time_of_day(hour));

        if (weather <= static_cast<uint8_t>(weather_type::heavy_snow))
            world.set_weather(static_cast<weather_type>(weather));

        spdlog::info("Initial environment: {}:{:02d}, weather={}", hour, minute, weather);
    }

    int32_t player_world_x = ch.pos_x * 32 + 16;
    int32_t player_world_y = ch.pos_y * 32 + 16;
    world.set_player_position(player_world_x, player_world_y);
    spdlog::debug(
        "Player position set at tile ({},{}) -> world ({},{})", ch.pos_x, ch.pos_y, player_world_x, player_world_y);

    spdlog::info("Entering game world: {}", ch.map_name);

    send_view_range();
    send_chat_preferences();
    game_->change_state(game_state::playing);
}

void ws_message_handler::handle_create_character_response(const json& message)
{
    auto response = create_character_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Character created successfully! ID: {}", response.character_id);
        request_characters();
        game_->change_state(game_state::select_character);
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Failed to create character" : response.error_message;
        spdlog::warn("Character creation failed: {}", error_msg);
        game_->show_error(error_msg);
    }
}

void ws_message_handler::handle_delete_character_response(const json& message)
{
    auto response = delete_character_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Character deleted successfully");
        // Server sends an unsolicited get_characters_response with the updated list,
        // so we don't need to request_characters() here.
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Failed to delete character" : response.error_message;
        spdlog::warn("Character deletion failed: {}", error_msg);
        game_->show_error(error_msg);
    }
}

void ws_message_handler::handle_pickup_response(const json& message)
{
    auto response = player_pickup_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info(
            "Picked up item: {} x{} (slot {})", response.item_name, response.quantity, response.inventory_slot);
    }
    else
    {
        spdlog::debug("Pickup failed: {}", response.error_message);
    }

    if (entity* player = game_->local_player())
    {
        player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());
    }
}

void ws_message_handler::handle_ground_item_removed(const json& message)
{
    auto data = ground_item_removed_data::from_json(message);

    spdlog::debug("Ground item removed: {} picked up {} at ({},{})", data.picker_name, data.item_name, data.x, data.y);

    game_->ground_items().remove(data.item_id);
}

void ws_message_handler::handle_player_position_update(const json& message)
{
    auto data = player_position_update_data::from_json(message);
    auto& entities = game_->entities();

    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
    {
        spdlog::debug("Position update for unknown entity {}, requesting info", data.entity_id);
        request_entity_info(data.entity_id);
        return;
    }

    auto& t = ent->transform();

    if (data.entity_id == entities.local_player_id())
    {
        if (t.moving)
        {
            spdlog::debug("Ignoring position update for moving local player (server: {},{}, tile: {},{})",
                          data.x,
                          data.y,
                          t.tile_x,
                          t.tile_y);
            return;
        }
    }

    t.facing = direction_from_protocol(data.direction).value_or(direction::south);

    bool is_local = (data.entity_id == entities.local_player_id());
    bool has_dest = (data.dest_x >= 0 && data.dest_y >= 0) && (data.dest_x != data.x || data.dest_y != data.y);

    if (is_local || !ent->has_movement())
    {
        // Local player or no movement component: just snap
        t.tile_x = data.x;
        t.tile_y = data.y;
        t.move_start_x = data.x;
        t.move_start_y = data.y;
        t.x = data.x * hb::tile_width + 16;
        t.y = data.y * hb::tile_height + 16;
    }
    else
    {
        auto& m = ent->movement();
        m.running = data.is_running;

        if (has_dest)
        {
            m.target_x = data.dest_x;
            m.target_y = data.dest_y;
        }

        // Already interpolating toward this position — let it finish
        if (t.moving && t.tile_x == data.x && t.tile_y == data.y)
        {
            // Nothing to do — arrival will handle transition
        }
        // Adjacent tile (1 step away) — interpolate from current to (x,y)
        else if (std::abs(t.tile_x - data.x) <= 1 && std::abs(t.tile_y - data.y) <= 1 &&
                 (t.tile_x != data.x || t.tile_y != data.y))
        {
            t.move_start_x = t.tile_x;
            t.move_start_y = t.tile_y;
            t.tile_x = data.x;
            t.tile_y = data.y;
            t.moving = true;
            t.move_progress = 0.0f;
            m.path.clear();
            m.path_index = 0;

            if (data.is_running)
            {
                ent->set_action(object_action::run);
            }
            else
            {
                bool combat = ent->has_combat() && ent->combat().combat_stance;
                ent->set_action_with_combat_mode(object_action::move_peace, combat);
            }
        }
        else
        {
            // Too far or same tile — snap
            t.tile_x = data.x;
            t.tile_y = data.y;
            t.move_start_x = data.x;
            t.move_start_y = data.y;
            t.x = data.x * hb::tile_width + 16;
            t.y = data.y * hb::tile_height + 16;
            t.moving = false;
            t.move_progress = 0.0f;
            m.path.clear();
            m.path_index = 0;
        }
    }

    spdlog::debug("Entity {} position updated: ({},{}) dir={} running={} dest=({},{})",
                  data.entity_id,
                  data.x,
                  data.y,
                  static_cast<int>(t.facing),
                  data.is_running,
                  data.dest_x,
                  data.dest_y);
}

void ws_message_handler::handle_player_stop_response(const json& message)
{
    auto data = player_stop_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::debug("Player stop request rejected by server");
        return;
    }

    entity* player = game_->local_player();
    if (!player)
        return;

    auto& t = player->transform();
    t.tile_x = data.x;
    t.tile_y = data.y;
    t.move_start_x = data.x;
    t.move_start_y = data.y;
    t.facing = direction_from_protocol(data.direction).value_or(direction::south);
    t.x = data.x * hb::tile_width + 16;
    t.y = data.y * hb::tile_height + 16;

    spdlog::debug("Player stop confirmed: ({},{}) dir={}", data.x, data.y, static_cast<int>(t.facing));
}

void ws_message_handler::handle_player_move_response(const json& message)
{
    auto data = player_move_response_data::from_json(message);

    entity* player = game_->local_player();
    if (!player)
        return;

    auto& input = game_->input_handler();

    if (!data.success)
    {
        spdlog::debug("Movement rejected: {}", data.error);

        auto& t = player->transform();
        // Revert tile position back to origin
        t.tile_x = t.move_start_x;
        t.tile_y = t.move_start_y;
        t.x = t.tile_x * hb::tile_width + 16;
        t.y = t.tile_y * hb::tile_height + 16;
        t.moving = false;
        t.move_progress = 0.0f;

        if (data.x != 0 || data.y != 0)
        {
            // Server says we're actually here - snap to corrective position
            t.tile_x = data.x;
            t.tile_y = data.y;
            t.move_start_x = data.x;
            t.move_start_y = data.y;
            t.x = data.x * hb::tile_width + 16;
            t.y = data.y * hb::tile_height + 16;
        }

        player->set_action_with_combat_mode(object_action::stop_peace, input.is_combat_mode());

        if (data.error == "blocked_occupied")
        {
            input.set_blocked_movement_cooldown(input_handler::blocked_movement_cooldown_duration);
            input.set_move_dest(-1, -1);

            uint8_t gender = player->sprite().gender;
            int sound_num = (gender == 2) ? 13 : 12;
            game_->sounds().play_sound('C', sound_num, 0);

            spdlog::debug("Movement blocked by entity, cooldown applied, playing C{}", sound_num);
        }

        if (data.error == "blocked_terrain")
        {
            input.set_blocked_movement_cooldown(input_handler::blocked_movement_cooldown_duration);
            input.set_move_dest(-1, -1);
            spdlog::debug("Movement blocked by terrain (server), cooldown applied");
        }

        return;
    }

    auto& t = player->transform();
    // tile_x/y is already the destination in our system
    bool position_mismatch = (t.tile_x != data.x || t.tile_y != data.y);

    if (position_mismatch)
    {
        spdlog::warn(
            "Movement position mismatch: client dest ({},{}) vs server ({},{})", t.tile_x, t.tile_y, data.x, data.y);
        t.tile_x = data.x;
        t.tile_y = data.y;
        t.move_start_x = data.x;
        t.move_start_y = data.y;
        t.x = data.x * hb::tile_width + 16;
        t.y = data.y * hb::tile_height + 16;
        t.moving = false;
        t.move_progress = 0.0f;
        player->set_action_with_combat_mode(object_action::stop_peace, input.is_combat_mode());

        input.set_blocked_movement_cooldown(input_handler::blocked_movement_cooldown_duration);
        input.set_move_dest(-1, -1);
    }

    spdlog::debug(
        "Movement confirmed: ({},{}) dir={} (interpolating={})", data.x, data.y, static_cast<int>(t.facing), t.moving);
}

void ws_message_handler::handle_hunger_update(const json& message)
{
    auto data = hunger_update_data::from_json(message);

    entity* player = game_->local_player();
    if (!player)
        return;

    player->stats().hunger = static_cast<uint8_t>(std::max(0, static_cast<int>(data.level)));

    auto& sl = game_->get_status_log();
    if (data.is_starving)
    {
        sl.set_message("hunger", "Starving! Regeneration blocked.", status_severity::critical);
        spdlog::warn("Player is starving! Regeneration blocked.");
    }
    else if (data.level < 30)
    {
        sl.set_message("hunger", "Hungry - regeneration delayed", status_severity::warning);
        spdlog::debug("Hunger low: {} - regeneration delayed", data.level);
    }
    else
    {
        sl.remove_message("hunger");
        spdlog::debug("Hunger updated: {}", data.level);
    }
}

void ws_message_handler::handle_npc_move(const json& message)
{
    auto data = npc_move_data::from_json(message);

    // Never apply NPC moves to the local player (guards against server ID collisions)
    if (data.entity_id == game_->entities().local_player_id())
        return;

    entity* ent = game_->entities().get_entity(data.entity_id);
    if (!ent)
    {
        spdlog::debug("NPC move for unknown entity {}, requesting info", data.entity_id);
        request_entity_info(data.entity_id);
        return;
    }

    auto& t = ent->transform();
    t.facing = direction_from_protocol(data.direction).value_or(direction::south);

    // If the NPC hasn't been positioned yet (just spawned), snap to destination
    bool first_position = (t.tile_x == 0 && t.tile_y == 0 && t.x == 0 && t.y == 0);

    if (first_position || !ent->has_movement())
    {
        // Snap directly
        t.tile_x = data.x;
        t.tile_y = data.y;
        t.move_start_x = data.x;
        t.move_start_y = data.y;
        t.x = data.x * hb::tile_width + 16;
        t.y = data.y * hb::tile_height + 16;
    }
    else
    {
        // Save origin, immediately set tile to destination
        t.move_start_x = t.tile_x;
        t.move_start_y = t.tile_y;
        t.tile_x = data.x;
        t.tile_y = data.y;
        t.moving = true;
        t.move_progress = 0.0f;

        // Set movement target so idle reset triggers on arrival
        auto& m = ent->movement();
        m.target_x = data.x;
        m.target_y = data.y;

        bool combat = ent->has_combat() && ent->combat().combat_stance;
        ent->set_action_with_combat_mode(object_action::move_peace, combat);
    }
}

void ws_message_handler::handle_entity_info_response(const json& message)
{
    auto data = entity_info_response_data::from_json(message);
    auto& entities = game_->entities();

    if (!data.success)
    {
        spdlog::debug("Entity info request failed: {}", data.error);
        return;
    }

    if (entities.get_entity(data.entity_id))
    {
        spdlog::debug("Entity {} already exists, ignoring info response", data.entity_id);
        return;
    }

    entity_type type = entity_type::character;
    if (data.entity_type == "npc")
        type = entity_type::npc;
    else if (data.entity_type == "monster")
        type = entity_type::monster;

    auto& ent = entities.create_entity_with_id(data.entity_id, type);

    init_entity_transform(ent, data.x, data.y, data.direction);

    if (ent.has_name())
    {
        ent.name().name = data.name;
        ent.name().faction = data.faction;
        ent.name().hostile = hostility_from_string(data.hostility);
    }

    init_entity_visual_type(ent, data.sprite_id, data.template_id, data.hostility);

    if (ent.has_stats())
    {
        auto& ent_stats = ent.stats();
        ent_stats.hp = data.hp;
        ent_stats.max_hp = data.hp_max;
        ent_stats.level = static_cast<uint16_t>(data.level);
    }

    uint16_t visual_type =
        data.sprite_id > 0 ? static_cast<uint16_t>(data.sprite_id) : static_cast<uint16_t>(data.template_id);
    spdlog::info("Created entity {} ({}) '{}' at ({},{}) sprite={} hostility={} from info response",
                 data.entity_id,
                 data.entity_type,
                 data.name,
                 data.x,
                 data.y,
                 visual_type,
                 data.hostility);
}

void ws_message_handler::request_characters()
{
    spdlog::info("Requesting character list");
    json msg = make_get_characters_request();
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_enter_game(int32_t character_id, bool force_disconnect)
{
    spdlog::info("Requesting to enter game with character ID: {}{}",
                 character_id,
                 force_disconnect ? " (force disconnect)" : "");

    pending_enter_game_character_id_ = character_id;

    game_->ui().show_connection_dialog(nullptr);

    json msg = make_enter_game_request(character_id, force_disconnect);
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_create_character(const character_create_data& data)
{
    spdlog::info("Requesting to create character: {}", data.name);

    json msg;
    msg["type"] = "create_character_request";
    msg["data"] = {{"name", data.name},
                   {"gender", data.gender},
                   {"skin_color", data.skin_color},
                   {"hair_style", data.hair_style},
                   {"hair_color", data.hair_color},
                   {"underwear_color", data.underwear_color},
                   {"strength", data.strength},
                   {"vitality", data.vitality},
                   {"dexterity", data.dexterity},
                   {"intelligence", data.intelligence},
                   {"magic", data.magic},
                   {"charisma", data.charisma}};
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_delete_character(int32_t character_id)
{
    spdlog::info("Requesting to delete character ID: {}", character_id);

    json msg;
    msg["type"] = "delete_character_request";
    msg["data"] = {{"character_id", character_id}};
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_entity_info(uint32_t entity_id)
{
    spdlog::debug("Requesting entity info for entity {}", entity_id);
    json msg = make_entity_info_request(entity_id);
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_combat_mode_toggle()
{
    json msg = make_combat_mode_change_request();
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_attack(uint32_t target_id, uint8_t attack_type)
{
    auto* player = game_->local_player();
    if (!player)
        return;

    const auto& t = player->transform();

    // Determine target type: 2 = npc/monster, 1 = player
    uint8_t target_type = 1;
    entity* target = game_->entities().find(target_id);
    if (target && (target->has_npc() || target->has_monster()))
        target_type = 2;

    // Calculate direction toward target (don't rely on t.facing which may be stale)
    uint8_t dir = static_cast<uint8_t>(direction_to_protocol(t.facing));
    if (target)
    {
        auto calc_dir = calculate_direction(t.tile_x, t.tile_y, target->transform().tile_x, target->transform().tile_y);
        if (calc_dir)
            dir = static_cast<uint8_t>(direction_to_protocol(*calc_dir));
    }

    spdlog::debug("Requesting attack on entity {} (type {} target_type {})", target_id, attack_type, target_type);
    json msg = make_player_attack_request(target_id, target_type, t.tile_x, t.tile_y, attack_type, dir);
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id)
{
    auto* player = game_->local_player();
    if (!player)
        return;

    const auto& t = player->transform();

    // Determine target_type string based on target entity
    std::string target_type_str = "none";
    if (target_id != 0)
    {
        entity* target = game_->entities().find(target_id);
        if (target)
        {
            if (target->has_npc() || target->has_monster())
                target_type_str = "npc";
            else
                target_type_str = "player";
        }
    }
    else if (target_x >= 0 && target_y >= 0)
    {
        target_type_str = "ground";
    }

    uint8_t dir = static_cast<uint8_t>(direction_to_protocol(t.facing));

    spdlog::debug("Requesting magic: spell={} target=({},{}) entity={} type={}",
                  spell_id,
                  target_x,
                  target_y,
                  target_id,
                  target_type_str);
    json msg =
        make_player_magic_request(t.tile_x, t.tile_y, dir, spell_id, target_type_str, target_id, target_x, target_y);
    game_->ws_connection().send(msg);
}

void ws_message_handler::handle_player_magic_response(const json& message)
{
    auto data = player_magic_response_data::from_json(message);

    if (!data.success)
    {
        if (!data.error.empty())
        {
            game_->get_status_log().add_event(data.error, message_color::red);
            spdlog::debug("Magic failed: {}", data.error);
        }
        return;
    }

    auto& entities = game_->entities();
    entity* player = game_->local_player();
    if (!player)
        return;

    // Update local player MP (server-authoritative)
    player->stats().mp = data.caster_mp;

    // Look up spell definition
    const spell* sp = game_->magic().get_spell(data.spell_id);

    // Get target position for floating text
    float target_wx = static_cast<float>(player->transform().x);
    float target_wy = static_cast<float>(player->transform().y);

    if (data.target_id != 0)
    {
        entity* target = entities.find(data.target_id);
        if (target)
        {
            target_wx = static_cast<float>(target->transform().x);
            target_wy = static_cast<float>(target->transform().y);
        }
    }

    // Floating text for damage/heal
    if (data.damage > 0)
    {
        game_->floating_text().add_damage(data.damage, target_wx, target_wy);
    }
    if (data.heal > 0)
    {
        game_->floating_text().add_heal(data.heal, target_wx, target_wy);
    }

    // Status log message
    std::string spell_name = sp ? sp->name : ("Spell #" + std::to_string(data.spell_id));
    if (data.damage > 0)
    {
        game_->get_status_log().add_event(spell_name + " hits for " + std::to_string(data.damage) + " damage",
                                          message_color::blue);
    }
    else if (data.heal > 0)
    {
        game_->get_status_log().add_event(spell_name + " restores " + std::to_string(data.heal) + " HP",
                                          message_color::green);
    }
    else
    {
        game_->get_status_log().add_event(spell_name + " cast successfully", message_color::blue);
    }

    // Update target animation (HP is updated by entity_hp_update)
    if (data.target_id != 0)
    {
        entity* target = entities.find(data.target_id);
        if (target && target->is_alive() && data.damage > 0)
        {
            target->set_action(object_action::damage);
        }
    }

    spdlog::debug("Magic response: spell={} damage={} heal={} target={} mp={}",
                  data.spell_id,
                  data.damage,
                  data.heal,
                  data.target_id,
                  data.caster_mp);
}

void ws_message_handler::handle_spell_list_update(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    if (!d.contains("spells") || !d["spells"].is_array())
        return;

    auto& magic = game_->magic();

    for (const auto& sp : d["spells"])
    {
        auto spell_data = enter_game_spell::from_json(sp);
        magic.learn_spell(spell_data.spell_id);
        magic.set_spell_mastery(spell_data.spell_id, static_cast<uint8_t>(spell_data.level));
        magic.set_spell_total_casts(spell_data.spell_id, spell_data.total_casts);
    }

    // Refresh spellbook dialog if open
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(game_->ui().get_dialog(dialog_type::spellbook)))
    {
        spell_dlg->clear_spells();
        for (const auto* sp : magic.get_all_spells())
        {
            spell_dlg->add_spell(*sp);
        }
    }

    // Re-populate hotbar
    game_->auto_populate_spell_hotbar();

    spdlog::info("Spell list updated: {} spells received", d["spells"].size());
}

void ws_message_handler::handle_set_render_mode(const json& message)
{
    if (!message.contains("data"))
    {
        spdlog::warn("set_render_mode: missing data");
        return;
    }
    const auto& d = message["data"];

    auto* rend = game_->get_renderer();
    if (!rend)
        return;

    // Parse mode
    if (d.contains("mode"))
    {
        std::string mode_str = d["mode"].get<std::string>();
        if (mode_str == "scaled")
            rend->set_view_mode(view_mode::scaled);
        else if (mode_str == "extended")
            rend->set_view_mode(view_mode::extended);
        else
            rend->set_view_mode(view_mode::special);
    }

    // Parse fair resolution
    if (d.contains("fair_width") && d.contains("fair_height"))
    {
        uint32_t fw = d["fair_width"].get<uint32_t>();
        uint32_t fh = d["fair_height"].get<uint32_t>();
        rend->set_internal_resolution(fw, fh);
    }

    // Update world screen size to match the new scene dimensions
    game_->game_world().set_screen_size(rend->scene_width(), rend->scene_height());

    // Inform server of our effective view range
    send_view_range();

    spdlog::info("Render mode set: mode={}, fair={}x{}",
                 static_cast<int>(rend->current_view_mode()),
                 rend->scene_width(),
                 rend->scene_height());
}

void ws_message_handler::handle_view_range_update(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    int16_t radius_x = d.value("radius_x", static_cast<int16_t>(40));
    int16_t radius_y = d.value("radius_y", static_cast<int16_t>(40));
    bool sees_all = d.value("sees_all", false);

    // Store visibility radii — these control how far the server streams entities,
    // NOT the fair zone size (that comes from set_render_mode).
    game_->set_view_radius(radius_x, radius_y, sees_all);

    spdlog::info("View range update: radius={}x{} tiles, sees_all={}", radius_x, radius_y, sees_all);
}

void ws_message_handler::handle_command_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];
    std::string msg_text = d.value("message", "");
    bool success = d.value("success", false);

    if (!msg_text.empty())
    {
        game_->get_status_log().add_event(msg_text, success ? message_color::green : message_color::red);
    }

    spdlog::info("Command response: success={}, message={}", success, msg_text);
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

void ws_message_handler::send_chat_message(std::string_view content,
                                           std::string_view channel,
                                           std::string_view recipient)
{
    json msg = make_chat_message_request(content, channel, recipient);
    game_->ws_connection().send(msg);
    spdlog::debug("Sent chat: channel={} content='{}'", channel, content);
}

void ws_message_handler::handle_chat_message_broadcast(const json& message)
{
    auto data = chat_message_broadcast_data::from_json(message);

    // Skip our own messages (local echo already handled)
    auto& entities = game_->entities();
    if (data.sender_id == entities.local_player_id())
        return;

    // Map channel string to chat_type
    chat_type type = chat_type::normal;
    if (data.channel == "local")
        type = chat_type::normal;
    else if (data.channel == "shout")
        type = chat_type::shout;
    else if (data.channel == "whisper")
        type = chat_type::whisper;
    else if (data.channel == "guild")
        type = chat_type::guild;
    else if (data.channel == "party")
        type = chat_type::party;
    else if (data.channel == "gm")
        type = chat_type::gm;
    else if (data.channel == "faction")
        type = chat_type::faction;
    else if (data.channel == "global")
        type = chat_type::global;
    else if (data.channel == "trade")
        type = chat_type::trade;

    // Check flags for overrides
    for (const auto& flag : data.flags)
    {
        if (flag == "system")
        {
            type = chat_type::system;
            break;
        }
        if (flag == "gm")
        {
            type = chat_type::gm;
            break;
        }
        if (flag == "emote")
        {
            type = chat_type::emote;
            break;
        }
    }

    chat_message msg;
    msg.type = type;
    msg.sender = data.sender_name;
    msg.content = data.content;
    msg.timestamp = std::chrono::system_clock::now();
    msg.recipient = data.recipient_name;

    if (auto* chat_dlg = dynamic_cast<chat_dialog*>(game_->ui().get_dialog(dialog_type::chat)))
    {
        chat_dlg->add_message(msg);
    }

    // Set chat bubble on sender entity
    if (entity* sender = entities.get_entity(data.sender_id))
    {
        if (sender->has_name())
        {
            auto& name = sender->name();
            name.chat_message = data.content;
            name.chat_timer = 4.0f;
            name.chat_elapsed = 0.0f;
            name.chat_style = get_chat_bubble_style(data.channel);

            // Map flags to text effects
            for (const auto& flag : data.flags)
            {
                if (flag == "gm")
                {
                    name.chat_style.effect = text_effect::glow;
                    break;
                }
                if (flag == "emote")
                {
                    name.chat_style.effect = text_effect::wave;
                    break;
                }
            }
        }
    }

    spdlog::debug("Chat from '{}' [{}]: {}", data.sender_name, data.channel, data.content);
}

void ws_message_handler::handle_combat_attack_broadcast(const json& message)
{
    auto data = combat_attack_broadcast_data::from_json(message);

    auto& entities = game_->entities();

    // Set attacker animation and facing
    entity* attacker = entities.find(data.attacker_id);
    if (attacker)
    {
        // Use server-authoritative direction if provided, otherwise calculate from positions
        auto dir = direction_from_protocol(data.direction);
        if (!dir)
            dir = calculate_direction(data.attacker_x, data.attacker_y, data.target_x, data.target_y);
        if (dir)
            attacker->transform().facing = *dir;

        // Local player handles their own attack animation client-side (input_handler.cpp)
        if (attacker->id() != entities.local_player_id())
        {
            if (data.is_ranged())
                attacker->set_action(object_action::attack_combat_bow);
            else
                attacker->set_action(object_action::attack_peace);
        }

        attacker->set_target(data.target_id);
    }

    // Spawn arrow projectile for ranged attacks
    if (data.is_ranged() && !data.projectile_type.empty())
    {
        game_->effects().add_effect(
            effect_type_id::arrow, data.attacker_x, data.attacker_y, data.target_x, data.target_y);
    }

    // Apply damage to target
    entity* target = entities.find(data.target_id);
    if (target && data.hit && data.damage > 0)
    {
        // Flinch animation — only if target is still alive (death animation takes priority)
        if (target->is_alive())
            target->set_action(object_action::damage);

        const auto& t = target->transform();
        if (data.critical)
            game_->floating_text().add_critical(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
        else
            game_->floating_text().add_damage(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
    }
    else if (target && data.hit && data.damage == 0)
    {
        const auto& t = target->transform();
        game_->floating_text().add_damage(0, static_cast<float>(t.x), static_cast<float>(t.y));
    }
    else if (target && !data.hit)
    {
        // Show MISS floating text
        const auto& t = target->transform();
        game_->floating_text().add_text(
            "MISS", static_cast<float>(t.x), static_cast<float>(t.y), sf::Color(180, 180, 180));
    }

    // Play weapon hit sound on successful hits
    if (data.hit && attacker)
    {
        uint16_t weapon_type = 0;
        if (data.attacker_id == entities.local_player_id())
        {
            if (const auto* weapon = game_->inventory().get_equipped(equip_slot::right_hand))
                weapon_type = weapon->type_id;
        }
        character_sound sound = get_weapon_swing_sound(weapon_type);
        const auto& at = attacker->transform();
        game_->sounds().play_character_sound_at(sound, at.x, at.y);
    }

    spdlog::debug("Combat broadcast: {} -> {} ({} {} dmg={})",
                  data.attacker_id,
                  data.target_id,
                  data.attack_mode,
                  data.hit ? "HIT" : "MISS",
                  data.damage);
}

void ws_message_handler::handle_player_attack_response(const json& message)
{
    auto data = player_attack_response_data::from_json(message);

    if (!data.success)
    {
        if (!data.error.empty())
        {
            game_->get_status_log().add_event(data.error, message_color::red);
            spdlog::debug("Attack failed: {}", data.error);
        }

        // Reset local player to idle on attack rejection
        if (entity* player = game_->local_player())
        {
            player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());
        }
    }
}

void ws_message_handler::handle_npc_attack(const json& message)
{
    auto data = npc_attack_data::from_json(message);

    auto& entities = game_->entities();

    // Set NPC attacker animation and facing
    entity* attacker = entities.find(data.npc_id);
    if (attacker)
    {
        auto dir = calculate_direction(data.npc_x, data.npc_y, data.target_x, data.target_y);
        if (dir)
            attacker->transform().facing = *dir;

        if (data.is_ranged)
            attacker->set_action(object_action::attack_combat_bow);
        else
            attacker->set_action(object_action::attack_peace);

        attacker->set_target(data.target_id);
    }

    // Spawn arrow projectile for ranged NPC attacks
    if (data.is_ranged && !data.projectile_type.empty())
    {
        game_->effects().add_effect(effect_type_id::arrow, data.npc_x, data.npc_y, data.target_x, data.target_y);
    }

    // Apply damage to target
    entity* target = entities.find(data.target_id);
    if (target && data.hit() && data.damage > 0)
    {
        // Flinch animation — only if target is still alive (death animation takes priority)
        if (target->is_alive())
            target->set_action(object_action::damage);

        const auto& t = target->transform();
        if (data.critical)
            game_->floating_text().add_critical(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
        else
            game_->floating_text().add_damage(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
    }
    else if (target && !data.hit())
    {
        // Show MISS floating text
        const auto& t = target->transform();
        game_->floating_text().add_text(
            "MISS", static_cast<float>(t.x), static_cast<float>(t.y), sf::Color(180, 180, 180));
    }

    spdlog::debug("NPC attack: {} -> {} ({} {} dmg={})",
                  data.npc_id,
                  data.target_id,
                  data.is_ranged ? "ranged" : "melee",
                  data.hit() ? "HIT" : "MISS",
                  data.damage);
}

void ws_message_handler::handle_entity_death(const json& message)
{
    auto data = entity_death_data::from_json(message);

    auto& entities = game_->entities();
    entity* victim = entities.find(data.victim_id);
    if (!victim)
        return;

    // NPCs/monsters can only die once; players can "die" multiple times (pretend corpse)
    bool is_npc = (victim->type() == entity_type::npc || victim->type() == entity_type::monster);
    if (is_npc && !victim->is_alive())
        return;

    if (victim->has_stats())
        victim->stats().hp = 0;

    // Transition to dead immediately (handles per-tile corpse cleanup),
    // then play the dying animation once
    game_->entities().transition_to_dead(data.victim_id);
    victim->set_action(object_action::dying);

    if (is_npc)
    {
        std::string name = victim->has_name() ? victim->name().name : "unknown";
        uint16_t npc_type = 0;
        std::string category;
        if (victim->has_npc())
        {
            npc_type = victim->npc().npc_type;
        }
        if (victim->has_monster())
        {
            npc_type = victim->monster().monster_type;
            category = victim->monster().category;
        }
        const auto& t = victim->transform();
        spdlog::debug("NPC death: id={} name='{}' type={} category='{}' "
                      "tile=({},{}) world=({},{}) killer={} facing={} alive_was=true",
                      data.victim_id,
                      name,
                      npc_type,
                      category,
                      t.tile_x,
                      t.tile_y,
                      t.x,
                      t.y,
                      data.killer_id,
                      static_cast<int>(t.facing));
    }
    else
    {
        spdlog::debug("Entity death: victim={} killer={} at ({},{})", data.victim_id, data.killer_id, data.x, data.y);
    }
}

void ws_message_handler::handle_entity_despawn(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    uint32_t entity_id = d.value("entity_id", static_cast<uint32_t>(0));
    if (entity_id == 0)
        return;

    game_->entities().remove_entity(entity_id);

    spdlog::debug("Entity despawned: {}", entity_id);
}

void ws_message_handler::handle_combat_effect(const json& message)
{
    auto data = combat_effect_data::from_json(message);

    auto& entities = game_->entities();

    // Skip effects where we are the source - we already handle our own
    // attacks/spells via player_attack_response / player_magic_response
    if (data.source_id != 0 && data.source_id == entities.local_player_id())
    {
        spdlog::debug("Skipping combat_effect for local player source");
        return;
    }

    // Determine world position for floating text
    float wx = static_cast<float>(data.target_x * 32 + 16);
    float wy = static_cast<float>(data.target_y * 32 + 16);

    // Try to get more accurate position from the target entity
    if (entity* target = entities.find(data.target_id))
    {
        wx = static_cast<float>(target->transform().x);
        wy = static_cast<float>(target->transform().y);
    }

    // Spell visual effects — only for actual spells (spell_id > 0)
    if (data.spell_id > 0)
    {
        const spell* sp = game_->magic().get_spell(data.spell_id);

        // Set caster animation and spell name
        entity* caster = entities.find(data.source_id);
        if (caster)
        {
            caster->set_action(object_action::magic);

            if (sp && caster->has_name())
            {
                auto& name = caster->name();
                name.chat_message = sp->name + "!";
                name.chat_timer = 3.0f;
                name.chat_elapsed = 0.0f;
                name.chat_style = {sf::Color::Red, sf::Color::Black, 1.0f, 14};
            }
        }

        // Spawn projectile and/or impact effects using world pixel coords
        if (sp)
        {
            // Caster world position
            float src_wx = static_cast<float>(data.target_x * 32 + 16);
            float src_wy = static_cast<float>(data.target_y * 32 + 16);
            if (caster)
            {
                src_wx = static_cast<float>(caster->transform().x);
                src_wy = static_cast<float>(caster->transform().y);
            }

            // Target world position
            float dest_wx = static_cast<float>(data.target_x * 32 + 16);
            float dest_wy = static_cast<float>(data.target_y * 32 + 16);
            if (data.target_id != 0)
            {
                entity* target = entities.find(data.target_id);
                if (target)
                {
                    dest_wx = static_cast<float>(target->transform().x);
                    dest_wy = static_cast<float>(target->transform().y);
                }
            }

            if (sp->projectile_effect != 0)
            {
                game_->effects().add_effect_world(
                    static_cast<effect_type_id>(sp->projectile_effect), src_wx, src_wy, dest_wx, dest_wy);
            }
            else if (sp->effect_sprite != 0)
            {
                // Area/ground spells always render at the target tile position
                // Single-target spells with no target entity render on the caster
                float fx = dest_wx;
                float fy = dest_wy;
                if (data.target_id == 0 && sp->target_type == spell_target::single)
                {
                    fx = src_wx;
                    fy = src_wy;
                }
                game_->effects().add_effect_at_pixel(static_cast<effect_type_id>(sp->effect_sprite), fx, fy);
            }
        }
    }

    // Floating text
    auto& ft = game_->floating_text();

    if (data.effect_type == "damage")
    {
        if (data.is_critical)
            ft.add_critical(data.value, wx, wy);
        else
            ft.add_damage(data.value, wx, wy);
    }
    else if (data.effect_type == "heal")
    {
        ft.add_heal(data.value, wx, wy);
    }
    else if (data.effect_type == "miss")
    {
        ft.add_text("MISS", wx, wy, sf::Color(180, 180, 180));
    }
    else if (data.effect_type == "dodge")
    {
        ft.add_text("DODGE", wx, wy, sf::Color(120, 180, 255));
    }
    else if (data.effect_type == "block")
    {
        ft.add_text("BLOCK", wx, wy, sf::Color(180, 180, 180));
    }
    else if (data.effect_type == "resist")
    {
        ft.add_text("RESIST", wx, wy, sf::Color(180, 100, 255));
    }
    else
    {
        spdlog::debug("Unhandled combat effect type: {}", data.effect_type);
    }

    // Update target animation (HP is updated by entity_hp_update)
    if (data.target_id != 0)
    {
        entity* target = entities.find(data.target_id);
        if (target && target->is_alive() && data.effect_type == "damage" && data.value > 0)
        {
            target->set_action(object_action::damage);
        }
    }

    spdlog::debug("Combat effect: {} -> {} type={} value={} spell={} critical={}",
                  data.source_id,
                  data.target_id,
                  data.effect_type,
                  data.value,
                  data.spell_id,
                  data.is_critical);
}

void ws_message_handler::handle_player_death_info(const json& message)
{
    auto data = player_death_info_data::from_json(message);

    // Create or reuse death dialog
    auto& ui = game_->ui();
    auto* dlg = dynamic_cast<death_dialog*>(ui.get_dialog(dialog_type::death));
    if (!dlg)
    {
        auto new_dlg = std::make_unique<death_dialog>();
        dlg = new_dlg.get();
        ui.add_dialog(dialog_type::death, std::move(new_dlg));
    }

    dlg->set_death_info(data.killer_name, data.is_pvp, data.xp_lost);
    dlg->set_on_restart(
        [this]()
        {
            game_->ui().close_dialog(dialog_type::death);
            json msg = make_player_respawn_request();
            game_->ws_connection().send(msg);
            spdlog::info("Sent player respawn request");
        });
    dlg->set_on_resurrect(
        [this]()
        {
            // TODO: send accept resurrection request
            spdlog::info("Accept resurrection requested (not yet implemented)");
        });
    dlg->open();

    // Mark local player as dead and play dying animation
    if (entity* player = game_->local_player())
    {
        player->set_alive(false);
        if (player->has_stats())
            player->stats().hp = 0;
        player->set_action(object_action::dying);
    }

    spdlog::info("Player died! Killer: {} (pvp={}) XP lost: {} Respawn: {} ({},{})",
                 data.killer_name,
                 data.is_pvp,
                 data.xp_lost,
                 data.respawn_map,
                 data.respawn_x,
                 data.respawn_y);
}

void ws_message_handler::handle_respawn_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    bool success = d.value("success", false);
    if (!success)
    {
        std::string error = d.value("error", "unknown");
        spdlog::warn("Respawn failed: {}", error);
        return;
    }

    // Success — the server will follow up with a player_teleport message
    // which handles map change, position reset, and reviving the player.
    spdlog::info("Respawn accepted, awaiting teleport");
}

void ws_message_handler::handle_player_teleport(const json& message)
{
    auto data = player_teleport_data::from_json(message);

    auto& entities = game_->entities();
    auto& world = game_->game_world();

    // Close any open death dialog
    game_->ui().close_dialog(dialog_type::death);

    // Store local player ID before clearing
    entity_id local_id = entities.local_player_id();

    // Clear all non-local entities
    std::vector<entity_id> to_remove;
    // We can't iterate and remove, so collect IDs to remove
    for (auto* ent : entities.get_entities_of_type(entity_type::character))
        if (ent->id() != local_id)
            to_remove.push_back(ent->id());
    for (auto* ent : entities.get_entities_of_type(entity_type::monster))
        to_remove.push_back(ent->id());
    for (auto* ent : entities.get_entities_of_type(entity_type::npc))
        to_remove.push_back(ent->id());
    for (auto id : to_remove)
        entities.remove_entity(id);

    // Clear ground items (separate from entity system)
    game_->ground_items().clear();

    // Load new map if different, or if current map data isn't loaded (retry failed loads)
    if (!data.dest_map.empty() && (data.dest_map != world.current_map().name() || !world.current_map().is_loaded()))
    {
        if (!world.load_map(data.dest_map))
        {
            spdlog::warn("Failed to load map '{}' during teleport", data.dest_map);
        }
    }

    // Reset local player position and state
    entity* player = entities.local_player();
    if (player)
    {
        auto& t = player->transform();
        t.tile_x = data.dest_x;
        t.tile_y = data.dest_y;
        t.move_start_x = data.dest_x;
        t.move_start_y = data.dest_y;
        t.x = data.dest_x * 32 + 16;
        t.y = data.dest_y * 32 + 16;
        t.moving = false;
        t.move_progress = 0.0f;
        t.facing = direction_from_protocol(data.dest_dir).value_or(direction::south);

        // Reset to alive idle state
        player->set_alive(true);
        player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());

        // Update camera
        world.set_player_position(t.x, t.y);
    }

    // Nearby entities are now sent as individual entity_spawn/npc_spawn/ground_item_spawn
    // messages by the server, so no entity list parsing needed here.

    // Clear floating text and effects from old map
    game_->floating_text().clear();
    game_->effects().clear();

    // Reset input state
    game_->input_handler().set_move_dest(-1, -1);

    spdlog::info("Teleported to {} at ({},{}) dir={}", data.dest_map, data.dest_x, data.dest_y, data.dest_dir);
}

void ws_message_handler::handle_environment_update(const json& message)
{
    if (!game_ || !message.contains("data"))
        return;

    const auto& d = message["data"];

    // Weather (0-6 maps directly to weather_type enum)
    if (d.contains("weather"))
    {
        uint8_t w = d["weather"].get<uint8_t>();
        if (w <= static_cast<uint8_t>(weather_type::heavy_snow))
        {
            game_->game_world().set_weather(static_cast<weather_type>(w));
        }
    }

    // Time of day - map hour to time_of_day enum
    if (d.contains("hour"))
    {
        uint8_t hour = d["hour"].get<uint8_t>();
        uint8_t minute = d.contains("minute") ? d["minute"].get<uint8_t>() : 0;
        game_->game_world().set_clock(hour, minute);

        game_->game_world().set_time(hour_to_time_of_day(hour));
    }
}

void ws_message_handler::handle_entity_spawn(const json& message)
{
    auto data = entity_spawn_data::from_json(message);
    auto& entities = game_->entities();

    // Skip if entity already exists
    if (entities.get_entity(data.entity_id))
    {
        spdlog::debug("entity_spawn: entity {} already exists, ignoring", data.entity_id);
        return;
    }

    // Skip spawning ourselves
    if (data.entity_id == entities.local_player_id())
        return;

    entity_type type = entity_type::character;
    if (data.type == "npc")
        type = entity_type::npc;
    else if (data.type == "monster")
        type = entity_type::monster;

    auto& ent = entities.create_entity_with_id(data.entity_id, type);

    init_entity_transform(ent, data.x, data.y, data.direction);

    if (ent.has_name())
    {
        ent.name().name = data.name;
        ent.name().faction = data.faction;
        ent.name().hostile = hostility_from_string(data.hostility);
        ent.name().pk = pk_status_from_string(data.pk_status);
        ent.name().guild_name = data.guild_name;
        ent.name().guild_tag = data.guild_tag;
    }

    if (ent.has_stats())
    {
        ent.stats().hp = data.hp_percent;
        ent.stats().max_hp = 100;
    }

    if (ent.has_combat())
    {
        ent.combat().combat_stance = data.combat_mode;
    }

    if (data.is_dead)
    {
        init_entity_dead_state(ent, entities);
    }
    else
    {
        ent.animation().set_state(entity_anim_state::stop);
        if (data.combat_mode)
        {
            ent.set_action(object_action::stop_combat);
        }
    }

    spdlog::info("Entity spawned: {} '{}' id={} faction={} combat={} dead={} at ({},{})",
                 data.type,
                 data.name,
                 data.entity_id,
                 data.faction,
                 data.combat_mode,
                 data.is_dead,
                 data.x,
                 data.y);
}

void ws_message_handler::handle_npc_spawn(const json& message)
{
    auto data = npc_spawn_data::from_json(message);
    auto& entities = game_->entities();

    // Skip if entity already exists
    if (entities.get_entity(data.entity_id))
    {
        spdlog::debug("npc_spawn: entity {} already exists, ignoring", data.entity_id);
        return;
    }

    auto& ent = entities.create_entity_with_id(data.entity_id, entity_type::monster);

    init_entity_transform(ent, data.x, data.y, data.direction);

    if (ent.has_name())
    {
        ent.name().name = data.name;
        ent.name().hostile = hostility_from_string(data.hostility);
    }

    init_entity_visual_type(ent, data.sprite_id, data.template_id, data.hostility);

    // NPC-specific monster attributes (not in entity_info_response)
    if (ent.has_monster())
    {
        ent.monster().attributes = data.attributes;
        ent.monster().category = data.category;
    }

    if (ent.has_stats())
    {
        ent.stats().hp = data.hp;
        ent.stats().max_hp = data.max_hp;
        ent.stats().level = static_cast<uint16_t>(data.level);
    }

    if (data.is_dead)
    {
        init_entity_dead_state(ent, entities);
    }
    else
    {
        ent.animation().set_state(entity_anim_state::stop);
    }

    uint16_t visual_type =
        data.sprite_id > 0 ? static_cast<uint16_t>(data.sprite_id) : static_cast<uint16_t>(data.template_id);
    spdlog::info("NPC spawned: '{}' id={} sprite={} at ({},{}) hp={}/{} dead={}",
                 data.name,
                 data.entity_id,
                 visual_type,
                 data.x,
                 data.y,
                 data.hp,
                 data.max_hp,
                 data.is_dead);
}

void ws_message_handler::handle_npc_despawn(const json& message)
{
    auto data = npc_despawn_data::from_json(message);

    spdlog::debug("NPC despawned: id={}", data.entity_id);
    game_->entities().destroy(data.entity_id);
}

void ws_message_handler::handle_ground_item_spawn(const json& message)
{
    auto data = ground_item_spawn_data::from_json(message);
    auto& items = game_->ground_items();

    // Skip if item already exists (duplicate spawn)
    if (items.get(data.item_id))
    {
        spdlog::debug("ground_item_spawn: item {} already exists, ignoring", data.item_id);
        return;
    }

    ground_item item;
    item.item_id = data.item_id;
    item.template_id = data.template_id;
    item.name = std::move(data.item_name);
    item.count = data.count;
    item.tile_x = data.x;
    item.tile_y = data.y;
    item.freshly_dropped = (data.reason == "drop");

    items.add(std::move(item));

    spdlog::debug("Ground item spawned: '{}' id={} x{} at ({},{})",
                  items.get(data.item_id)->name,
                  data.item_id,
                  data.count,
                  data.x,
                  data.y);
}

void ws_message_handler::handle_stat_update(const json& message)
{
    auto data = stat_update_data::from_json(message);

    entity* player = game_->local_player();
    if (!player)
        return;

    auto& stats = player->stats();
    stats.max_hp = data.max_hp;
    stats.max_mp = data.max_mp;
    stats.max_sp = data.max_sp;
    stats.attack_power = data.attack_power;
    stats.magic_power = data.magic_power;
    stats.defense = data.defense;
    stats.magic_resist = data.magic_defense;
    stats.hit_ratio = data.hit_rate;
    stats.dodge_ratio = data.dodge_rate;
    stats.critical_ratio = data.critical_rate;

    // Full stat updates (teleport/respawn) include current vitals
    if (data.hp)
        stats.hp = *data.hp;
    if (data.mp)
        stats.mp = *data.mp;
    if (data.sp)
        stats.sp = *data.sp;
    if (data.experience)
        stats.experience = *data.experience;
    if (data.level)
        stats.level = *data.level;
    if (data.hunger_level)
        stats.hunger = *data.hunger_level;
    if (data.pk_count && player->has_combat())
        player->combat().pk_count = *data.pk_count;

    game_->update_icon_panel();

    spdlog::debug("Stats updated: max_hp={} max_mp={} max_sp={} atk={} def={}",
                  data.max_hp,
                  data.max_mp,
                  data.max_sp,
                  data.attack_power,
                  data.defense);
}

void ws_message_handler::handle_entity_hp_update(const json& message)
{
    auto data = entity_hp_update_data::from_json(message);
    auto& entities = game_->entities();

    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
    {
        spdlog::debug("HP update for unknown entity {}", data.entity_id);
        return;
    }

    if (ent->has_stats())
    {
        ent->stats().hp = data.hp;
        if (data.hp_max > 0)
            ent->stats().max_hp = data.hp_max;
    }

    // Update HUD if this is the local player
    if (data.entity_id == entities.local_player_id())
        game_->update_icon_panel();

    spdlog::debug("Entity {} HP: {}/{}", data.entity_id, data.hp, data.hp_max);
}

void ws_message_handler::handle_equipment_change_broadcast(const json& message)
{
    auto data = equipment_change_broadcast_data::from_json(message);

    // Skip our own changes — handled by equip/unequip response
    if (data.entity_id == game_->entities().local_player_id())
        return;

    spdlog::debug("Entity {} equipment slot {} changed: item={} template={}",
                  data.entity_id,
                  data.slot,
                  data.item_id,
                  data.template_id);

    // TODO: Update other player's visual equipment when character rendering supports it
}

void ws_message_handler::handle_combat_mode_change_broadcast(const json& message)
{
    auto data = combat_mode_change_broadcast_data::from_json(message);

    auto& entities = game_->entities();
    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
        return;

    if (!ent->has_combat())
        return;

    ent->combat().combat_stance = data.combat_mode;

    // Update current animation to reflect the stance change
    auto& anim = ent->animation();
    if (anim.state == entity_anim_state::stop)
    {
        ent->set_action(data.combat_mode ? object_action::stop_combat : object_action::stop_peace);
    }
    else if (anim.state == entity_anim_state::move)
    {
        ent->set_action(data.combat_mode ? object_action::move_combat : object_action::move_peace);
    }

    spdlog::debug("Entity {} combat stance: {}", data.entity_id, data.combat_mode);
}

void ws_message_handler::handle_combat_mode_change_response(const json& message)
{
    auto data = combat_mode_change_response_data::from_json(message);

    // Apply the server-confirmed combat mode to the local player
    auto& input = game_->input_handler();
    input.set_combat_mode(data.combat_mode);

    entity* player = game_->local_player();
    if (player)
    {
        if (player->has_combat())
            player->combat().combat_stance = data.combat_mode;

        auto& anim = player->animation();
        if (anim.state == entity_anim_state::stop)
        {
            player->set_action(data.combat_mode ? object_action::stop_combat : object_action::stop_peace);
        }
        else if (anim.state == entity_anim_state::move)
        {
            player->set_action(data.combat_mode ? object_action::move_combat : object_action::move_peace);
        }
    }

    spdlog::debug("Combat mode confirmed: {}", data.combat_mode);
}

void ws_message_handler::handle_player_action_broadcast(const json& message)
{
    auto data = player_action_broadcast_data::from_json(message);
    auto& entities = game_->entities();

    // Don't apply to local player — we already play our own animations
    if (data.entity_id == entities.local_player_id())
        return;

    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
        return;

    auto& t = ent->transform();
    t.facing = direction_from_protocol(data.direction).value_or(t.facing);

    if (data.action == "attack")
    {
        bool combat = ent->has_combat() && ent->combat().combat_stance;
        ent->set_action_with_combat_mode(object_action::attack_peace, combat);
    }
    else if (data.action == "dash_attack")
    {
        bool combat = ent->has_combat() && ent->combat().combat_stance;
        ent->set_action_with_combat_mode(object_action::attack_peace, combat);
        ent->animation().set_state(entity_anim_state::attack_move);
    }
    else if (data.action == "magic")
    {
        ent->set_action(object_action::magic);
    }
    else if (data.action == "pickup")
    {
        ent->set_action(object_action::get_item);
    }

    spdlog::debug("Entity {} action broadcast: {} dir={}", data.entity_id, data.action, data.direction);
}

void ws_message_handler::handle_player_equip_response(const json& message)
{
    auto data = player_equip_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::debug("Equip failed: {}", data.error);
        return;
    }

    spdlog::info("Equipped '{}' to slot {}", data.item_name, data.slot);

    // Server will send stat_update and inventory_data/equipment_data
    // to refresh our state, so we don't need to manually update here
}

void ws_message_handler::handle_player_unequip_response(const json& message)
{
    auto data = player_unequip_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::debug("Unequip failed: {}", data.error);
        return;
    }

    spdlog::info("Unequipped '{}' from slot {} -> inventory slot {}", data.item_name, data.slot, data.inventory_slot);
}


void ws_message_handler::handle_inventory_data(const json& message)
{
    auto data = inventory_data_msg::from_json(message);

    auto& inventory = game_->inventory();
    inventory.clear();
    inventory.set_gold(static_cast<uint32_t>(data.gold));

    for (const auto& inv_item : data.items)
    {
        item itm;
        itm.id = inv_item.item_id;
        itm.type_id = static_cast<uint16_t>(inv_item.item_id);
        itm.name = inv_item.name;
        itm.amount = static_cast<uint32_t>(inv_item.count);
        itm.durability = static_cast<uint16_t>(inv_item.durability);
        itm.max_durability = static_cast<uint16_t>(inv_item.max_durability);
        inventory.set_item_at(inv_item.slot, itm);
    }

    spdlog::debug("Inventory refreshed: {} items, {} gold", data.items.size(), data.gold);
}

void ws_message_handler::handle_equipment_data(const json& message)
{
    auto data = equipment_data_msg::from_json(message);

    auto& inventory = game_->inventory();

    // Clear all equipped slots first
    for (int i = 1; i <= 11; ++i)
        inventory.clear_equipped(static_cast<equip_slot>(i));

    for (const auto& eq_item : data.equipment)
    {
        item itm;
        itm.id = eq_item.item_id;
        itm.type_id = static_cast<uint16_t>(eq_item.item_id);
        itm.name = eq_item.name;
        itm.durability = static_cast<uint16_t>(eq_item.durability);
        itm.max_durability = static_cast<uint16_t>(eq_item.max_durability);

        auto slot = equip_slot_from_server(eq_item.slot);
        if (slot != equip_slot::none)
            inventory.set_equipped(slot, itm);
    }

    spdlog::debug("Equipment refreshed: {} items", data.equipment.size());
}

void ws_message_handler::handle_skills_data(const json& message)
{
    auto data = skills_data_msg::from_json(message);

    auto& skills = game_->skills();
    for (const auto& sk : data.skills)
    {
        uint8_t mastery = static_cast<uint8_t>(std::min(static_cast<int16_t>(100), sk.level));
        skills.set_mastery(sk.skill_id, mastery);

        float progress = 0.0f;
        if (sk.uses_to_next_level > 0)
            progress = static_cast<float>(sk.uses_this_level) / static_cast<float>(sk.uses_to_next_level);
        skills.set_sub_progress(sk.skill_id, progress);

        spdlog::info("  skill_id={} level={} mastery={} uses={}/{} progress={:.2f}",
                     sk.skill_id,
                     sk.level,
                     mastery,
                     sk.uses_this_level,
                     sk.uses_to_next_level,
                     progress);
    }

    // Push skill data to the dialog
    if (auto* dlg = dynamic_cast<skills_dialog*>(game_->ui().get_dialog(dialog_type::skills)))
    {
        auto all = skills.get_all_skills();
        std::vector<skill> skill_list;
        skill_list.reserve(all.size());
        for (const auto* s : all)
            skill_list.push_back(*s);
        dlg->set_skills(skill_list);
        spdlog::info("Pushed {} skills to dialog", skill_list.size());
    }
    else
    {
        spdlog::warn("Skills dialog not found when trying to push data");
    }
}

void ws_message_handler::handle_player_skill_response(const json& message)
{
    if (message.contains("data"))
    {
        const auto& d = message["data"];
        bool success = d.value("success", false);
        if (!success)
        {
            std::string error = d.value("error", "Unknown error");
            spdlog::debug("Skill use failed: {}", error);
            return;
        }

        if (d.contains("result"))
        {
            const auto& r = d["result"];
            uint32_t skill_id = r.value("skill_id", 0u);
            int32_t effect = r.value("effect_value", 0);
            spdlog::debug("Skill {} used, effect={}", skill_id, effect);
        }
    }
}

void ws_message_handler::handle_player_interact_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    bool success = d.value("success", false);
    if (!success)
    {
        std::string error = d.value("error", "Interaction failed");
        spdlog::debug("Interaction failed: {}", error);
        return;
    }

    if (!d.contains("result"))
        return;
    const auto& result = d["result"];

    std::string interaction_type = result.value("interaction_type", "");

    if (interaction_type == "shop")
    {
        spdlog::info("Shop opened: {}", result["interaction_data"].value("npc_name", ""));
        // TODO: Open shop dialog with items from interaction_data
    }
    else if (interaction_type == "bank")
    {
        spdlog::info("Bank opened: {}", result["interaction_data"].value("npc_name", ""));
        // TODO: Open bank dialog with items from interaction_data
    }
    else if (interaction_type == "dialog")
    {
        spdlog::info("NPC dialog: {}", result["interaction_data"].value("npc_name", ""));
        // TODO: Open NPC dialog with text and options from interaction_data
    }
    else
    {
        spdlog::debug("Unknown interaction type: {}", interaction_type);
    }
}

void ws_message_handler::handle_pong(const json& message)
{
    if (message.contains("data"))
    {
        int64_t timestamp = message["data"].value("timestamp", static_cast<int64_t>(0));
        (void)timestamp;
        // Could calculate latency here if we stored the ping send time
    }
}

void ws_message_handler::handle_error(const json& message)
{
    if (message.contains("data"))
    {
        const auto& d = message["data"];
        std::string code = d.value("error_code", "UNKNOWN");
        std::string msg = d.value("message", "Unknown error");
        spdlog::warn("Server error [{}]: {}", code, msg);
    }
}

// =============================================================================
// Fishing
// =============================================================================

void ws_message_handler::request_fish_skill()
{
    json msg = make_fish_skill_request();
    game_->ws_connection().send(msg);
    spdlog::debug("Sent fish_skill_request");
}

void ws_message_handler::request_fish_catch()
{
    json msg = make_fish_catch_request();
    game_->ws_connection().send(msg);
    spdlog::debug("Sent fish_catch_request");
}

void ws_message_handler::handle_fish_skill_response(const json& message)
{
    auto data = fish_skill_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Fish skill failed: {}", data.error);
        game_->get_status_log().add_event(data.error, message_color::red);
        return;
    }

    spdlog::info("Fish skill activated - waiting for fish...");
}

void ws_message_handler::handle_fish_engaged(const json& message)
{
    auto data = fish_engaged_data::from_json(message);

    if (auto* dlg = dynamic_cast<fishing_dialog*>(game_->ui().get_dialog(dialog_type::fishing)))
    {
        dlg->open_fishing(data.fish_name, data.visual_type, data.catch_chance);
    }
}

void ws_message_handler::handle_fish_chance_update(const json& message)
{
    auto data = fish_chance_update_data::from_json(message);

    if (auto* dlg = dynamic_cast<fishing_dialog*>(game_->ui().get_dialog(dialog_type::fishing)))
    {
        if (dlg->is_open())
        {
            dlg->update_catch_chance(data.catch_chance);
        }
    }
}

void ws_message_handler::handle_fish_catch_response(const json& message)
{
    auto data = fish_catch_response_data::from_json(message);

    // Close the fishing dialog
    if (auto* dlg = dynamic_cast<fishing_dialog*>(game_->ui().get_dialog(dialog_type::fishing)))
    {
        dlg->close_fishing();
    }

    if (data.result == "success")
    {
        std::string msg = "Caught " + data.item_name + "!";
        spdlog::info("{}", msg);
        game_->get_status_log().add_event(msg, message_color::green);
    }
    else if (data.result == "fail")
    {
        spdlog::info("Fish got away!");
        game_->get_status_log().add_event("The fish got away!", message_color::yellow);
    }
    else if (data.result == "canceled")
    {
        spdlog::info("Fishing canceled");
    }
}

void ws_message_handler::handle_fish_spawn_broadcast(const json& message)
{
    auto data = fish_spawn_broadcast_data::from_json(message);
    game_->effects().add_fish_node(data.fish_index, data.x, data.y);
}

void ws_message_handler::handle_fish_despawn_broadcast(const json& message)
{
    auto data = fish_despawn_broadcast_data::from_json(message);
    game_->effects().remove_fish_node(data.fish_index);
}

// =============================================================================
// Guild
// =============================================================================

void ws_message_handler::request_guild_create(std::string_view name, std::string_view tag)
{
    game_->ws_connection().send(make_guild_create_request(name, tag));
    spdlog::info("Sent guild_create_request: name='{}' tag='{}'", name, tag);
}

void ws_message_handler::request_guild_disband()
{
    game_->ws_connection().send(make_guild_disband_request());
    spdlog::info("Sent guild_disband_request");
}

void ws_message_handler::request_guild_leave()
{
    game_->ws_connection().send(make_guild_leave_request());
    spdlog::info("Sent guild_leave_request");
}

void ws_message_handler::request_guild_kick(std::string_view target)
{
    game_->ws_connection().send(make_guild_kick_request(target));
    spdlog::info("Sent guild_kick_request: target='{}'", target);
}

void ws_message_handler::request_guild_invite(std::string_view target)
{
    game_->ws_connection().send(make_guild_invite_request(target));
    spdlog::info("Sent guild_invite_request: target='{}'", target);
}

void ws_message_handler::request_guild_invite_respond(bool accept)
{
    game_->ws_connection().send(make_guild_invite_respond_request(accept));
    spdlog::info("Sent guild_invite_respond_request: accept={}", accept);
}

void ws_message_handler::request_guild_promote(std::string_view target)
{
    game_->ws_connection().send(make_guild_promote_request(target));
    spdlog::info("Sent guild_promote_request: target='{}'", target);
}

void ws_message_handler::request_guild_demote(std::string_view target)
{
    game_->ws_connection().send(make_guild_demote_request(target));
    spdlog::info("Sent guild_demote_request: target='{}'", target);
}

void ws_message_handler::request_guild_set_motd(std::string_view motd)
{
    game_->ws_connection().send(make_guild_set_motd_request(motd));
    spdlog::info("Sent guild_set_motd_request");
}

void ws_message_handler::request_guild_info()
{
    game_->ws_connection().send(make_guild_info_request());
    spdlog::debug("Sent guild_info_request");
}

void ws_message_handler::handle_guild_create_response(const json& message)
{
    auto data = guild_create_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild create failed: {}", data.error);
        game_->get_status_log().add_event("Guild creation failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild created: {} [{}]", data.guild_name, data.tag);
    game_->guild().set_guild(data.guild_name, data.tag, 0);
    game_->get_status_log().add_event("Guild '" + data.guild_name + "' created!", message_color::green);

    // Update local player name component
    if (auto* player = game_->local_player())
    {
        player->name().guild_name = data.guild_name;
        player->name().guild_tag = data.tag;
        player->name().guild_rank_id = 0;
    }
}

void ws_message_handler::handle_guild_disband_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild disband failed: {}", data.error);
        game_->get_status_log().add_event("Disband failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild disbanded");
    game_->guild().clear_guild();
    game_->get_status_log().add_event("Guild has been disbanded.", message_color::yellow);

    if (auto* player = game_->local_player())
    {
        player->name().guild_name.clear();
        player->name().guild_tag.clear();
        player->name().guild_rank_id = 255;
    }
}

void ws_message_handler::handle_guild_leave_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild leave failed: {}", data.error);
        game_->get_status_log().add_event("Leave failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Left guild");
    game_->guild().clear_guild();
    game_->get_status_log().add_event("You have left the guild.", message_color::yellow);

    if (auto* player = game_->local_player())
    {
        player->name().guild_name.clear();
        player->name().guild_tag.clear();
        player->name().guild_rank_id = 255;
    }
}

void ws_message_handler::handle_guild_kick_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild kick failed: {}", data.error);
        game_->get_status_log().add_event("Kick failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Kick successful");
    game_->get_status_log().add_event("Player kicked from guild.", message_color::yellow);
}

void ws_message_handler::handle_guild_invite_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild invite failed: {}", data.error);
        game_->get_status_log().add_event("Invite failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild invite sent");
    game_->get_status_log().add_event("Guild invite sent.", message_color::green);
}

void ws_message_handler::handle_guild_invite_received(const json& message)
{
    auto data = guild_invite_received_data::from_json(message);

    spdlog::info("Received guild invite from '{}' to join [{}] {}", data.inviter_name, data.guild_tag, data.guild_name);

    game_->guild().set_pending_invite(data.guild_name, data.guild_tag, data.inviter_name);
    game_->get_status_log().add_event(
        data.inviter_name + " invites you to join [" + data.guild_tag + "] " + data.guild_name, message_color::green);
}

void ws_message_handler::handle_guild_invite_respond_response(const json& message)
{
    auto data = guild_invite_respond_response_data::from_json(message);

    game_->guild().clear_pending_invite();

    if (!data.success)
    {
        spdlog::info("Guild invite respond failed: {}", data.error);
        game_->get_status_log().add_event("Join failed: " + data.error, message_color::red);
        return;
    }

    if (data.accepted)
    {
        spdlog::info("Joined guild: {} [{}]", data.guild_name, data.guild_tag);
        game_->guild().set_guild(data.guild_name, data.guild_tag, 4); // recruit rank
        game_->get_status_log().add_event("You joined [" + data.guild_tag + "] " + data.guild_name + "!",
                                          message_color::green);

        if (auto* player = game_->local_player())
        {
            player->name().guild_name = data.guild_name;
            player->name().guild_tag = data.guild_tag;
            player->name().guild_rank_id = 4;
        }

        // Request full guild info now that we've joined
        request_guild_info();
    }
    else
    {
        spdlog::info("Declined guild invite");
        game_->get_status_log().add_event("Guild invite declined.", message_color::yellow);
    }
}

void ws_message_handler::handle_guild_promote_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild promote failed: {}", data.error);
        game_->get_status_log().add_event("Promote failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild promote successful");
    game_->get_status_log().add_event("Member promoted.", message_color::green);
}

void ws_message_handler::handle_guild_demote_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild demote failed: {}", data.error);
        game_->get_status_log().add_event("Demote failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild demote successful");
    game_->get_status_log().add_event("Member demoted.", message_color::yellow);
}

void ws_message_handler::handle_guild_set_motd_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild set MOTD failed: {}", data.error);
        game_->get_status_log().add_event("Set MOTD failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild MOTD updated");
    game_->get_status_log().add_event("Guild MOTD updated.", message_color::green);
}

void ws_message_handler::handle_guild_info_response(const json& message)
{
    auto data = guild_info_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild info failed: {}", data.error);
        return;
    }

    std::vector<guild_member_info> members;
    members.reserve(data.members.size());
    for (const auto& m : data.members)
    {
        members.push_back({
            .name = m.name,
            .rank_id = m.rank,
            .rank_name = m.rank_name,
            .is_online = m.is_online,
        });
    }

    game_->guild().set_guild_info(data.guild_name, data.tag, data.motd, data.master_name, std::move(members));

    spdlog::info("Guild info: {} [{}] - {} members", data.guild_name, data.tag, data.member_count);
}

void ws_message_handler::handle_guild_update(const json& message)
{
    auto data = guild_update_data::from_json(message);
    auto& guild = game_->guild();

    spdlog::info("Guild update: action='{}' player='{}'", data.action, data.player_name);

    if (data.action == "member_joined")
    {
        game_->get_status_log().add_event(data.player_name + " joined the guild.", message_color::green);
        request_guild_info();
    }
    else if (data.action == "member_left")
    {
        game_->get_status_log().add_event(data.player_name + " left the guild.", message_color::yellow);
        request_guild_info();
    }
    else if (data.action == "member_kicked")
    {
        game_->get_status_log().add_event(data.player_name + " was kicked from the guild.", message_color::yellow);
        request_guild_info();
    }
    else if (data.action == "you_were_kicked")
    {
        guild.clear_guild();
        game_->get_status_log().add_event("You were kicked from the guild!", message_color::red);

        if (auto* player = game_->local_player())
        {
            player->name().guild_name.clear();
            player->name().guild_tag.clear();
            player->name().guild_rank_id = 255;
        }
    }
    else if (data.action == "guild_disbanded")
    {
        guild.clear_guild();
        game_->get_status_log().add_event("The guild has been disbanded.", message_color::red);

        if (auto* player = game_->local_player())
        {
            player->name().guild_name.clear();
            player->name().guild_tag.clear();
            player->name().guild_rank_id = 255;
        }
    }
    else if (data.action == "member_promoted")
    {
        game_->get_status_log().add_event(data.player_name + " was promoted.", message_color::green);
        request_guild_info();
    }
    else if (data.action == "member_demoted")
    {
        game_->get_status_log().add_event(data.player_name + " was demoted.", message_color::yellow);
        request_guild_info();
    }
    else if (data.action == "motd_changed")
    {
        game_->get_status_log().add_event("Guild MOTD: " + data.motd, message_color::green);
        request_guild_info();
    }
}

void ws_message_handler::handle_available_commands(const json& message)
{
    if (!message.contains("data") || !message["data"].contains("commands"))
    {
        spdlog::warn("available_commands: missing data.commands");
        return;
    }

    std::vector<command_entry> commands;
    for (const auto& cmd_json : message["data"]["commands"])
    {
        command_entry entry;
        if (cmd_json.contains("name"))
            entry.name = cmd_json["name"].get<std::string>();
        if (cmd_json.contains("description"))
            entry.description = cmd_json["description"].get<std::string>();
        if (cmd_json.contains("usage"))
            entry.usage = cmd_json["usage"].get<std::string>();
        if (cmd_json.contains("category"))
            entry.category = cmd_json["category"].get<std::string>();
        if (cmd_json.contains("enabled"))
            entry.enabled = cmd_json["enabled"].get<bool>();
        commands.push_back(std::move(entry));
    }

    spdlog::info("Received {} available commands from server", commands.size());
    game_->chat_input().set_commands(std::move(commands));
}

void ws_message_handler::handle_command_availability_update(const json& message)
{
    if (!message.contains("data") || !message["data"].contains("commands"))
    {
        spdlog::warn("command_availability_update: missing data.commands");
        return;
    }

    auto& chat = game_->chat_input();
    for (const auto& cmd_json : message["data"]["commands"])
    {
        if (cmd_json.contains("name") && cmd_json.contains("enabled"))
        {
            chat.update_command_availability(cmd_json["name"].get<std::string>(), cmd_json["enabled"].get<bool>());
        }
    }
}

} // namespace hb
