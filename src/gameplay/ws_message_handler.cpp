#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "graphics/renderer.hpp"
#include "core/config.hpp"
#include "core/direction_utils.hpp"
#include "chat/chat_message.hpp"
#include "ui/dialogs/chat_dialog.hpp"
#include "ui/dialogs/spellbook_dialog.hpp"
#include "world/tile.hpp"
#include <spdlog/spdlog.h>

namespace hb {

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
    if (!message.contains("type"))
    {
        spdlog::warn("Received message without type field");
        return;
    }

    std::string type = message["type"].get<std::string>();

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
    else if (type == msg_type::chat_message)
        {} // Ack from server, already handled via local echo
    else
        spdlog::warn("Unknown message type: {}", type);
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
            game_->ui().create_confirm_box(
                "Session Conflict",
                "This account is already logged in.\nDisconnect other session?",
                [this, char_id](bool confirmed) {
                    if (confirmed)
                    {
                        request_enter_game(char_id, true);
                    }
                }
            );
            return;
        }

        game_->show_error(error_msg);
        return;
    }

    const auto& ch = response.character;
    spdlog::info("Entering game as '{}' on map: {} at ({}, {})",
                 ch.name, ch.map_name, ch.pos_x, ch.pos_y);

    auto& entities = game_->entities();
    auto& sprites = game_->sprites();

    // Create local player entity
    auto& player = entities.create_entity_with_id(ch.id, entity_type::player);
    entities.set_local_player(ch.id);

    auto& transform = player.transform();
    transform.tile_x = ch.pos_x;
    transform.tile_y = ch.pos_y;
    transform.x = ch.pos_x * 32 + 16;
    transform.y = ch.pos_y * 32 + 16;
    spdlog::info("PLAYER SPAWN: tile=({},{}) world=({},{})", ch.pos_x, ch.pos_y, transform.x, transform.y);

    auto& name = player.name();
    name.name = ch.name;

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
    stats.experience = static_cast<uint32_t>(ch.experience);
    stats.hunger = static_cast<uint8_t>(ch.hunger_level);

    auto& combat = player.combat();
    combat.pk_count = ch.pk_count;

    auto& sprite = player.sprite();
    sprite.skin_color = static_cast<uint8_t>(ch.skin_color);
    sprite.hair_style = static_cast<uint8_t>(ch.hair_style);
    sprite.hair_color = static_cast<uint8_t>(ch.hair_color);
    sprite.gender = (ch.gender == 0) ? 1 : 2;

    entities.load_character_sprites(player, sprites);

    auto& anim = player.animation();
    anim.set_state(entity_anim_state::stop);

    spdlog::debug("Player stats - Level: {}, STR: {}, DEX: {}, VIT: {}, INT: {}, MAG: {}, CHA: {}",
                  stats.level, stats.strength, stats.dexterity, stats.vitality,
                  stats.intelligence, stats.magic, stats.charisma);
    spdlog::debug("Player vitals - HP: {}/{}, MP: {}/{}, SP: {}/{}",
                  stats.hp, stats.max_hp, stats.mp, stats.max_mp, stats.sp, stats.max_sp);

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
    spdlog::debug("Loaded {} inventory items, {} gold",
                  response.inventory.items.size(), ch.gold);

    // Set equipped items
    for (const auto& eq_item : response.equipment)
    {
        item itm;
        itm.id = eq_item.item_id;
        itm.type_id = static_cast<uint16_t>(eq_item.item_id);
        itm.name = eq_item.name;
        itm.durability = static_cast<uint16_t>(eq_item.durability);
        itm.max_durability = static_cast<uint16_t>(eq_item.max_durability);

        equip_slot slot = equip_slot::none;
        switch (eq_item.slot)
        {
            case 0: slot = equip_slot::head; break;
            case 1: slot = equip_slot::body; break;
            case 2: slot = equip_slot::arms; break;
            case 3: slot = equip_slot::pants; break;
            case 4: slot = equip_slot::boots; break;
            case 5: slot = equip_slot::right_hand; break;
            case 6: slot = equip_slot::left_hand; break;
            case 7: slot = equip_slot::right_finger; break;
            case 8: slot = equip_slot::left_finger; break;
            case 9: slot = equip_slot::neck; break;
            case 10: slot = equip_slot::back; break;
            case 11: slot = equip_slot::none; break;
            default: slot = equip_slot::none; break;
        }
        if (slot != equip_slot::none)
        {
            inventory.set_equipped(slot, itm);
        }
    }
    spdlog::debug("Loaded {} equipped items", response.equipment.size());

    // Set skill levels
    auto& skills = game_->skills();
    for (const auto& sk : response.skills)
    {
        uint8_t mastery = static_cast<uint8_t>(std::min(sk.level / 2, 100));
        skills.set_mastery(sk.skill_id, mastery);
    }
    spdlog::debug("Loaded {} skills", response.skills.size());

    // Set known spells from server
    auto& magic = game_->magic();
    for (const auto& sp : response.spells)
    {
        magic.learn_spell(sp.spell_id);
        magic.set_spell_mastery(sp.spell_id, static_cast<uint8_t>(sp.level));
        magic.set_spell_total_casts(sp.spell_id, sp.total_casts);
    }
    spdlog::info("Loaded {} spells from server, {} learned total",
                 response.spells.size(), magic.get_learned_spells().size());

    // Auto-populate spell hotbar from learned spells
    game_->auto_populate_spell_hotbar();

    // Populate spellbook dialog with all spell definitions
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(
            game_->ui().get_dialog(dialog_type::spellbook)))
    {
        spell_dlg->clear_spells();
        auto all_spells = magic.get_all_spells();
        for (const auto* sp : all_spells)
        {
            spell_dlg->add_spell(*sp);
        }
        spdlog::info("Populated spellbook dialog with {} spells ({} learned)",
                     all_spells.size(), magic.get_learned_spells().size());
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
    spdlog::debug("Loaded {} active quests, {} completed quests",
                  quests.active.size(), quests.completed.size());

    // Spawn nearby entities
    for (const auto& ent : response.world.entities)
    {
        // Skip entities that already exist (handles self + ID collisions)
        if (entities.get_entity(ent.entity_id)) continue;

        entity_type type = entity_type::character;
        if (ent.type == "npc") type = entity_type::npc;
        else if (ent.type == "monster") type = entity_type::monster;

        auto& world_entity = entities.create_entity_with_id(ent.entity_id, type);

        auto& ent_transform = world_entity.transform();
        ent_transform.tile_x = ent.x;
        ent_transform.tile_y = ent.y;
        ent_transform.x = ent.x * 32;
        ent_transform.y = ent.y * 32;
        ent_transform.facing = direction_from_protocol(ent.direction).value_or(direction::south);

        if (world_entity.has_name())
        {
            world_entity.name().name = ent.name;
        }

        if (ent.visual_type > 0)
        {
            world_entity.set_type(ent.visual_type);
            if (world_entity.has_npc())
                world_entity.npc().npc_type = ent.visual_type;
            else if (world_entity.has_monster())
                world_entity.monster().monster_type = ent.visual_type;
        }

        if (world_entity.has_stats())
        {
            auto& ent_stats = world_entity.stats();
            ent_stats.hp = ent.hp_percent;
            ent_stats.max_hp = 100;
        }
    }
    spdlog::debug("Spawned {} nearby entities", response.world.entities.size());

    // Load map
    auto& world = game_->game_world();
    if (!world.load_map(ch.map_name))
    {
        spdlog::warn("Failed to load map data for '{}', continuing without tile data", ch.map_name);
        world.current_map_mut().set_name(ch.map_name);
    }

    int32_t player_world_x = ch.pos_x * 32 + 16;
    int32_t player_world_y = ch.pos_y * 32 + 16;
    world.set_player_position(player_world_x, player_world_y);
    spdlog::debug("Player position set at tile ({},{}) -> world ({},{})",
                  ch.pos_x, ch.pos_y, player_world_x, player_world_y);

    spdlog::info("Entering game world: {}", ch.map_name);

    send_view_range();
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
        spdlog::info("Picked up item: {} x{} (slot {})",
                     response.item_name, response.quantity, response.inventory_slot);
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

    spdlog::debug("Ground item removed: {} picked up {} at ({},{})",
                  data.picker_name, data.item_name, data.x, data.y);

    game_->entities().destroy(data.item_id);
}

void ws_message_handler::handle_player_position_update(const json& message)
{
    auto data = player_position_update_data::from_json(message);
    auto& entities = game_->entities();

    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
    {
        spdlog::debug("Position update for unknown entity {}", data.entity_id);
        return;
    }

    auto& t = ent->transform();

    if (data.entity_id == entities.local_player_id())
    {
        if (t.moving)
        {
            spdlog::debug("Ignoring position update for moving local player (server: {},{}, dest: {},{})",
                          data.x, data.y, t.dest_tile_x, t.dest_tile_y);
            return;
        }
    }

    t.tile_x = data.x;
    t.tile_y = data.y;
    t.facing = direction_from_protocol(data.direction).value_or(direction::south);
    t.x = data.x * hb::tile_width + 16;
    t.y = data.y * hb::tile_height + 16;

    if (ent->has_movement())
    {
        ent->movement().running = data.is_running;
        if (data.dest_x >= 0 && data.dest_y >= 0)
        {
            ent->movement().target_x = data.dest_x;
            ent->movement().target_y = data.dest_y;
        }
    }

    spdlog::debug("Entity {} position updated: ({},{}) dir={} running={} dest=({},{})",
                  data.entity_id, data.x, data.y, static_cast<int>(t.facing), data.is_running,
                  data.dest_x, data.dest_y);
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
    if (!player) return;

    auto& t = player->transform();
    t.tile_x = data.x;
    t.tile_y = data.y;
    t.facing = direction_from_protocol(data.direction).value_or(direction::south);
    t.x = data.x * hb::tile_width + 16;
    t.y = data.y * hb::tile_height + 16;

    spdlog::debug("Player stop confirmed: ({},{}) dir={}",
                  data.x, data.y, static_cast<int>(t.facing));
}

void ws_message_handler::handle_player_move_response(const json& message)
{
    auto data = player_move_response_data::from_json(message);

    entity* player = game_->local_player();
    if (!player) return;

    auto& action_q = game_->action_queue();
    auto& input = game_->input_handler();

    if (!data.success)
    {
        spdlog::debug("Movement rejected: {}", data.error);

        auto& t = player->transform();
        t.moving = false;
        t.move_progress = 0.0f;
        t.dest_tile_x = t.tile_x;
        t.dest_tile_y = t.tile_y;

        if (data.x != 0 || data.y != 0)
        {
            t.tile_x = data.x;
            t.tile_y = data.y;
            t.x = data.x * hb::tile_width + 16;
            t.y = data.y * hb::tile_height + 16;
        }

        player->set_action_with_combat_mode(object_action::stop_peace, input.is_combat_mode());

        if (data.error == "blocked_occupied")
        {
            action_q.set_blocked_movement_cooldown(action_queue::blocked_movement_cooldown_duration);
            input.set_move_dest(-1, -1);

            uint8_t gender = player->sprite().gender;
            int sound_num = (gender == 2) ? 13 : 12;
            game_->sounds().play_sound('C', sound_num, 0);

            spdlog::debug("Movement blocked by entity, cooldown applied, playing C{}", sound_num);
        }

        if (data.error == "blocked_terrain")
        {
            action_q.set_blocked_movement_cooldown(action_queue::blocked_movement_cooldown_duration);
            input.set_move_dest(-1, -1);
            spdlog::debug("Movement blocked by terrain (server), cooldown applied");
        }

        return;
    }

    auto& t = player->transform();
    bool position_mismatch = (t.dest_tile_x != data.x || t.dest_tile_y != data.y);

    if (position_mismatch)
    {
        spdlog::warn("Movement position mismatch: client dest ({},{}) vs server ({},{})",
                     t.dest_tile_x, t.dest_tile_y, data.x, data.y);
        t.tile_x = data.x;
        t.tile_y = data.y;
        t.dest_tile_x = data.x;
        t.dest_tile_y = data.y;
        t.x = data.x * hb::tile_width + 16;
        t.y = data.y * hb::tile_height + 16;
        t.moving = false;
        t.move_progress = 0.0f;
        player->set_action_with_combat_mode(object_action::stop_peace, input.is_combat_mode());

        action_q.set_blocked_movement_cooldown(action_queue::blocked_movement_cooldown_duration);
        input.set_move_dest(-1, -1);
    }

    spdlog::debug("Movement confirmed: ({},{}) dir={} (interpolating={})",
                  data.x, data.y, static_cast<int>(t.facing), t.moving);
}

void ws_message_handler::handle_hunger_update(const json& message)
{
    auto data = hunger_update_data::from_json(message);

    entity* player = game_->local_player();
    if (!player) return;

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
    if (data.entity_id == game_->entities().local_player_id()) return;

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
        t.x = data.x * hb::tile_width + 16;
        t.y = data.y * hb::tile_height + 16;
    }
    else
    {
        // Set up interpolated movement (same as player characters)
        t.dest_tile_x = data.x;
        t.dest_tile_y = data.y;
        t.moving = true;
        t.move_progress = 0.0f;

        ent->set_action(object_action::move_peace);
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
    if (data.entity_type == "npc") type = entity_type::npc;
    else if (data.entity_type == "monster") type = entity_type::monster;

    auto& ent = entities.create_entity_with_id(data.entity_id, type);

    auto& t = ent.transform();
    t.tile_x = data.x;
    t.tile_y = data.y;
    t.x = data.x * hb::tile_width + 16;
    t.y = data.y * hb::tile_height + 16;
    t.facing = direction_from_protocol(data.direction).value_or(direction::south);

    if (ent.has_name())
    {
        ent.name().name = data.name;
    }

    if (data.template_id > 0)
    {
        ent.set_type(static_cast<uint16_t>(data.template_id));
        if (ent.has_npc())
            ent.npc().npc_type = static_cast<uint16_t>(data.template_id);
        else if (ent.has_monster())
            ent.monster().monster_type = static_cast<uint16_t>(data.template_id);
    }

    if (ent.has_stats())
    {
        auto& ent_stats = ent.stats();
        ent_stats.hp = data.hp;
        ent_stats.max_hp = data.hp_max;
        ent_stats.level = static_cast<uint16_t>(data.level);
    }

    spdlog::info("Created entity {} ({}) '{}' at ({},{}) from info response",
                 data.entity_id, data.entity_type, data.name, data.x, data.y);
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
                 character_id, force_disconnect ? " (force disconnect)" : "");

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
    msg["data"] = {
        {"name", data.name},
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
        {"charisma", data.charisma}
    };
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

void ws_message_handler::handle_set_render_mode(const json& message)
{
    if (!message.contains("data"))
    {
        spdlog::warn("set_render_mode: missing data");
        return;
    }
    const auto& d = message["data"];

    auto* rend = game_->get_renderer();
    if (!rend) return;

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
                 rend->scene_width(), rend->scene_height());
}

void ws_message_handler::send_view_range()
{
    // Send the effective game resolution (scene dimensions account for view mode)
    auto* rend = game_->get_renderer();
    uint32_t w = rend ? rend->scene_width() : config::instance().video().screen_width;
    uint32_t h = rend ? rend->scene_height() : config::instance().video().screen_height;
    json msg = make_set_view_range_request(w, h);
    game_->ws_connection().send(msg);
    spdlog::info("Sent view range: {}x{}", w, h);
}

void ws_message_handler::send_chat_message(std::string_view content, std::string_view channel,
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
    if (data.channel == "local") type = chat_type::normal;
    else if (data.channel == "shout") type = chat_type::shout;
    else if (data.channel == "whisper") type = chat_type::whisper;
    else if (data.channel == "guild") type = chat_type::guild;
    else if (data.channel == "party") type = chat_type::party;
    else if (data.channel == "gm") type = chat_type::gm;
    else if (data.channel == "faction") type = chat_type::guild;  // Faction uses guild color
    else if (data.channel == "global") type = chat_type::global;
    else if (data.channel == "trade") type = chat_type::trade;

    // Check flags for overrides
    for (const auto& flag : data.flags)
    {
        if (flag == "system") { type = chat_type::system; break; }
        if (flag == "gm") { type = chat_type::gm; break; }
        if (flag == "emote") { type = chat_type::emote; break; }
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
                if (flag == "gm")    { name.chat_style.effect = text_effect::glow; break; }
                if (flag == "emote") { name.chat_style.effect = text_effect::wave; break; }
            }
        }
    }

    spdlog::debug("Chat from '{}' [{}]: {}", data.sender_name, data.channel, data.content);
}

} // namespace hb
