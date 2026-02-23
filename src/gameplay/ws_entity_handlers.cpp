#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "world/ground_item.hpp"
#include "core/direction_utils.hpp"
#include "world/tile.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

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

    if (!ent->is_alive())
        return;

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

void ws_message_handler::handle_entity_spawn(const json& message)
{
    auto data = entity_spawn_data::from_json(message);
    auto& entities = game_->entities();

    // Skip if entity already exists (unless it's fading out — let the respawn replace it)
    if (auto* existing = entities.get_entity(data.entity_id))
    {
        if (!existing->is_fading_out())
        {
            spdlog::debug("entity_spawn: entity {} already exists, ignoring", data.entity_id);
            return;
        }
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

    // Skip if entity already exists (unless it's fading out — let the respawn replace it)
    if (auto* existing = entities.get_entity(data.entity_id))
    {
        if (!existing->is_fading_out())
        {
            spdlog::debug("npc_spawn: entity {} already exists, ignoring", data.entity_id);
            return;
        }
    }

    // Determine entity type from category: service NPCs (merchant, quest, trainer, banker, warehouse)
    // are non-combatant entity_type::npc; everything else (monster, boss, guard, pet, summon) is
    // entity_type::monster with full combat/stats components
    auto type = entity_type::monster;
    if (data.category == "merchant" || data.category == "quest" || data.category == "trainer" ||
        data.category == "banker" || data.category == "warehouse")
    {
        type = entity_type::npc;
    }

    auto& ent = entities.create_entity_with_id(data.entity_id, type);

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

void ws_message_handler::handle_ground_item_spawn(const json& message)
{
    auto data = ground_item_spawn_msg::from_json(message);
    auto& items = game_->ground_items();

    if (items.get(data.item_data.item_id))
    {
        spdlog::debug("ground_item_spawn: item {} already exists, ignoring", data.item_data.item_id);
        return;
    }

    ground_item gi;
    gi.data = std::move(data.item_data);
    gi.tile_x = data.x;
    gi.tile_y = data.y;
    gi.freshly_dropped = false;

    auto item_id = gi.data.item_id;
    auto item_name = gi.data.name;
    auto count = gi.data.count;

    items.add(std::move(gi));

    spdlog::debug("Ground item spawned: '{}' id={} x{} at ({},{})",
                  item_name, item_id, count, data.x, data.y);
}

} // namespace hb
