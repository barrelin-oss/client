#include "network/handlers/motion_handlers.hpp"
#include "gameplay/game_state.hpp"
#include "gameplay/effect_types.hpp"
#include "entity/entity_manager.hpp"
#include "core/game_enums.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hb
{

void motion_handler::initialize(game_state_manager& game)
{
    game_ = &game;
    spdlog::info("Motion handler initialized");
}

void motion_handler::handle_motion_response(packet_reader& reader)
{
    if (!game_)
        return;

    auto result = reader.read_u8();
    if (!result)
        return;

    auto* player = game_->local_player();
    if (!player)
        return;

    if (*result == 0)
    {
        // Motion ACCEPTED - server confirms
        auto x = reader.read_i16();
        auto y = reader.read_i16();
        auto dir = reader.read_u8();

        if (x && y && dir)
        {
            auto& t = player->transform();

            // tile_x/y is already the destination; verify it matches server
            t.facing = static_cast<direction>(*dir);

            // Rubber-band correction if server position differs significantly
            int32_t dx_diff = std::abs(t.tile_x - *x);
            int32_t dy_diff = std::abs(t.tile_y - *y);
            if (dx_diff > 1 || dy_diff > 1)
            {
                // Snap to server position
                t.tile_x = *x;
                t.tile_y = *y;
                t.move_start_x = *x;
                t.move_start_y = *y;
                t.x = *x * 32 + 16;
                t.y = *y * 32 + 16;
                t.move_progress = 0.0f;
            }
        }
    }
    else
    {
        // Motion REJECTED - cancel prediction, revert to origin
        spdlog::debug("Motion rejected - code: {}", *result);

        auto& t = player->transform();

        // Revert tile position back to origin
        t.tile_x = t.move_start_x;
        t.tile_y = t.move_start_y;
        t.x = t.tile_x * 32 + 16;
        t.y = t.tile_y * 32 + 16;
        t.moving = false;
        t.move_progress = 0.0f;

        // Return to idle with current combat stance
        player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());
    }
}

void motion_handler::handle_motion_event(packet_reader& reader)
{
    if (!game_)
        return;

    // Read entity ID
    auto entity_id = reader.read_u32();
    if (!entity_id)
        return;

    // Read action type
    auto action_type = reader.read_u8();
    if (!action_type)
        return;

    // Dispatch based on action
    object_action action = static_cast<object_action>(*action_type);

    switch (action)
    {
    case object_action::stop_peace:
    case object_action::stop_combat:
        process_stop(*entity_id, reader);
        break;
    case object_action::move_peace:
    case object_action::move_combat:
        process_move(*entity_id, reader);
        break;
    case object_action::run:
        process_run(*entity_id, reader);
        break;
    case object_action::attack_peace:
    case object_action::attack_combat:
    case object_action::attack_combat_bow:
        process_attack(*entity_id, reader);
        break;
    case object_action::damage:
        process_damage(*entity_id, reader);
        break;
    case object_action::magic:
        process_magic(*entity_id, reader);
        break;
    case object_action::get_item:
        process_get_item(*entity_id, reader);
        break;
    case object_action::dying:
        process_dead(*entity_id, reader);
        break;
    default:
        spdlog::debug("Unknown motion action: {}", *action_type);
        break;
    }
}

void motion_handler::handle_common_event(packet_reader& reader)
{
    if (!game_)
        return;

    // Read event subtype
    auto subtype = reader.read_u16();
    if (!subtype)
        return;

    // Dispatch based on subtype
    switch (*subtype)
    {
    case 1: // Spawn object
        process_spawn_object(reader);
        break;
    case 2: // Despawn object
        process_despawn_object(reader);
        break;
    case 3: // Magic effect
        process_magic_effect(reader);
        break;
    case 4: // Generic effect
        process_effect(reader);
        break;
    default:
        spdlog::debug("Unknown common event: 0x{:04X}", *subtype);
        break;
    }
}

void motion_handler::process_stop(uint32_t entity_id, packet_reader& reader)
{
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();

    if (!x || !y || !dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
    {
        // Create new entity if not exists
        ent = &game_->entities().create(entity_id);
    }

    auto& t = ent->transform();
    t.tile_x = *x;
    t.tile_y = *y;
    t.move_start_x = *x;
    t.move_start_y = *y;
    t.x = *x * 32 + 16;
    t.y = *y * 32 + 16;
    t.facing = static_cast<direction>(*dir);
    ent->set_action(object_action::stop_peace);
}

void motion_handler::process_move(uint32_t entity_id, packet_reader& reader)
{
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto speed = reader.read_u8();

    if (!x || !y || !dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
    {
        ent = &game_->entities().create(entity_id);
    }

    // Initialize world coordinates if not set
    auto& t = ent->transform();
    if (t.x == 0 && t.y == 0 && (t.tile_x != 0 || t.tile_y != 0))
    {
        t.x = t.tile_x * 32 + 16;
        t.y = t.tile_y * 32 + 16;
    }

    ent->set_move_target(*x, *y);
    // Save origin, immediately set tile to destination
    t.move_start_x = t.tile_x;
    t.move_start_y = t.tile_y;
    t.tile_x = *x;
    t.tile_y = *y;
    t.facing = static_cast<direction>(*dir);
    t.moving = true;
    t.move_progress = 0.0f;
    ent->set_action(object_action::move_peace);
    if (speed)
    {
        ent->set_move_speed(*speed);
    }
}

void motion_handler::process_run(uint32_t entity_id, packet_reader& reader)
{
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();

    if (!x || !y || !dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
    {
        ent = &game_->entities().create(entity_id);
    }

    // Initialize world coordinates if not set
    auto& t = ent->transform();
    if (t.x == 0 && t.y == 0 && (t.tile_x != 0 || t.tile_y != 0))
    {
        t.x = t.tile_x * 32 + 16;
        t.y = t.tile_y * 32 + 16;
    }

    ent->set_move_target(*x, *y);
    // Save origin, immediately set tile to destination
    t.move_start_x = t.tile_x;
    t.move_start_y = t.tile_y;
    t.tile_x = *x;
    t.tile_y = *y;
    t.facing = static_cast<direction>(*dir);
    t.moving = true;
    t.move_progress = 0.0f;
    ent->set_action(object_action::run);
}

void motion_handler::process_attack(uint32_t entity_id, packet_reader& reader)
{
    auto dir = reader.read_u8();
    auto target_id = reader.read_u32();
    auto attack_type = reader.read_u8();

    if (!dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->transform().facing = static_cast<direction>(*dir);
    ent->set_action(object_action::attack_peace);
    if (target_id)
    {
        ent->set_target(*target_id);
    }
    if (attack_type)
    {
        ent->set_attack_type(*attack_type);
    }
}

void motion_handler::process_attack_move(uint32_t entity_id, packet_reader& reader)
{
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto target_id = reader.read_u32();

    if (!x || !y || !dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->set_move_target(*x, *y);
    ent->transform().facing = static_cast<direction>(*dir);
    ent->set_action(object_action::attack_peace);
    if (target_id)
    {
        ent->set_target(*target_id);
    }
}

void motion_handler::process_damage(uint32_t entity_id, packet_reader& reader)
{
    auto damage = reader.read_i32();
    [[maybe_unused]] auto attacker_id = reader.read_u32();

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->set_action(object_action::damage);
    if (damage)
    {
        // Show damage number
        spdlog::debug("Entity {} took {} damage", entity_id, *damage);
    }
}

void motion_handler::process_damage_move(uint32_t entity_id, packet_reader& reader)
{
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    [[maybe_unused]] auto damage = reader.read_i32();

    if (!x || !y || !dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->set_move_target(*x, *y);
    ent->transform().facing = static_cast<direction>(*dir);
    ent->set_action(object_action::damage);
}

void motion_handler::process_magic(uint32_t entity_id, packet_reader& reader)
{
    auto spell_id = reader.read_u16();
    auto dir = reader.read_u8();
    auto target_x = reader.read_i16();
    auto target_y = reader.read_i16();

    if (!spell_id || !dir)
        return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->transform().facing = static_cast<direction>(*dir);
    ent->set_action(object_action::magic);
    ent->set_casting_spell(*spell_id);

    // Look up spell definition for proper effect chain
    const auto* sp = game_->magic().get_spell(*spell_id);

    // Caster position in tile coordinates
    const auto& caster_t = ent->transform();
    int32_t caster_tx = caster_t.tile_x;
    int32_t caster_ty = caster_t.tile_y;

    // Spawn casting effect on caster
    if (sp && sp->cast_sprite != 0)
    {
        game_->effects().add_effect_at(static_cast<effect_type_id>(sp->cast_sprite), caster_tx, caster_ty);
    }

    if (target_x && target_y)
    {
        // Spawn projectile from caster to target (if applicable)
        if (sp && sp->projectile_effect != 0)
        {
            game_->effects().add_effect(
                static_cast<effect_type_id>(sp->projectile_effect), caster_tx, caster_ty, *target_x, *target_y);
        }

        // Spawn impact effect at target
        if (sp && sp->effect_sprite != 0)
        {
            game_->effects().add_effect_at(static_cast<effect_type_id>(sp->effect_sprite), *target_x, *target_y);
        }
        else if (!sp)
        {
            // Fallback: no spell definition, use raw spell_id as before
            game_->magic().create_effect(*spell_id, *target_x, *target_y);
        }
    }
}

void motion_handler::process_get_item(uint32_t entity_id, packet_reader& reader)
{
    (void)reader;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->set_action(object_action::get_item);
}

void motion_handler::process_dying(uint32_t entity_id, packet_reader& reader)
{
    auto dir = reader.read_u8();

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    if (dir)
    {
        ent->transform().facing = static_cast<direction>(*dir);
    }
    ent->set_action(object_action::dying);
}

void motion_handler::process_dead(uint32_t entity_id, packet_reader& reader)
{
    (void)reader;

    entity* ent = game_->entities().find(entity_id);
    if (!ent)
        return;

    ent->set_action(object_action::dying);
    // Entity will be removed after death animation completes
}

void motion_handler::process_spawn_object(packet_reader& reader)
{
    auto entity_id = reader.read_u32();
    auto entity_type = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto name = reader.read_string(20);

    if (!entity_id || !entity_type || !x || !y)
        return;

    entity& ent = game_->entities().create(*entity_id);
    ent.set_type(*entity_type);

    auto& t = ent.transform();
    t.tile_x = *x;
    t.tile_y = *y;
    t.move_start_x = *x;
    t.move_start_y = *y;
    t.x = *x * 32 + 16;
    t.y = *y * 32 + 32;

    if (dir)
    {
        t.facing = static_cast<direction>(*dir);
    }
    if (name)
    {
        ent.set_name(*name);
    }

    spdlog::debug("Spawned entity {} at tile ({}, {}) -> world ({}, {})", *entity_id, *x, *y, t.x, t.y);
}

void motion_handler::process_despawn_object(packet_reader& reader)
{
    auto entity_id = reader.read_u32();
    if (!entity_id)
        return;

    game_->entities().destroy(*entity_id);
    spdlog::debug("Despawned entity {}", *entity_id);
}

void motion_handler::process_magic_effect(packet_reader& reader)
{
    auto effect_id = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!effect_id || !x || !y)
        return;

    game_->magic().create_effect(*effect_id, *x, *y);
}

void motion_handler::process_effect(packet_reader& reader)
{
    auto effect_type = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!effect_type || !x || !y)
        return;

    game_->effects().add_effect(static_cast<effect_type_id>(*effect_type), *x, *y, *x, *y);
}

} // namespace hb
