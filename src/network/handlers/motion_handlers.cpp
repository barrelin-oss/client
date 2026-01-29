#include "network/handlers/motion_handlers.hpp"
#include "gameplay/game_state.hpp"
#include "entity/entity_manager.hpp"
#include "core/game_enums.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void motion_handler::initialize(game_state_manager& game) {
    game_ = &game;
    spdlog::info("Motion handler initialized");
}

void motion_handler::handle_motion_response(packet_reader& reader) {
    if (!game_) return;

    // Response to our own motion command
    auto result = reader.read_u8();
    if (!result) return;

    if (*result == 0) {
        // Motion accepted
        auto x = reader.read_i16();
        auto y = reader.read_i16();
        auto dir = reader.read_u8();

        if (x && y && dir) {
            if (auto* player = game_->local_player()) {
                player->transform().tile_x = *x;
                player->transform().tile_y = *y;
                player->transform().direction = static_cast<direction>(*dir);
            }
        }
    } else {
        // Motion rejected - rubber band back
        spdlog::debug("Motion rejected");
    }
}

void motion_handler::handle_motion_event(packet_reader& reader) {
    if (!game_) return;

    // Read entity ID
    auto entity_id = reader.read_u32();
    if (!entity_id) return;

    // Read action type
    auto action_type = reader.read_u8();
    if (!action_type) return;

    // Dispatch based on action
    object_action action = static_cast<object_action>(*action_type);

    switch (action) {
        case object_action::stop:
            process_stop(*entity_id, reader);
            break;
        case object_action::move:
            process_move(*entity_id, reader);
            break;
        case object_action::run:
            process_run(*entity_id, reader);
            break;
        case object_action::attack:
            process_attack(*entity_id, reader);
            break;
        case object_action::attack_move:
            process_attack_move(*entity_id, reader);
            break;
        case object_action::damage:
            process_damage(*entity_id, reader);
            break;
        case object_action::damage_move:
            process_damage_move(*entity_id, reader);
            break;
        case object_action::magic:
            process_magic(*entity_id, reader);
            break;
        case object_action::get_item:
            process_get_item(*entity_id, reader);
            break;
        case object_action::dying:
            process_dying(*entity_id, reader);
            break;
        case object_action::dead:
            process_dead(*entity_id, reader);
            break;
        default:
            spdlog::debug("Unknown motion action: {}", *action_type);
            break;
    }
}

void motion_handler::handle_common_event(packet_reader& reader) {
    if (!game_) return;

    // Read event subtype
    auto subtype = reader.read_u16();
    if (!subtype) return;

    // Dispatch based on subtype
    switch (*subtype) {
        case 1:  // Spawn object
            process_spawn_object(reader);
            break;
        case 2:  // Despawn object
            process_despawn_object(reader);
            break;
        case 3:  // Magic effect
            process_magic_effect(reader);
            break;
        case 4:  // Generic effect
            process_effect(reader);
            break;
        default:
            spdlog::debug("Unknown common event: 0x{:04X}", *subtype);
            break;
    }
}

void motion_handler::process_stop(uint32_t entity_id, packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();

    if (!x || !y || !dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) {
        // Create new entity if not exists
        ent = &game_->entities().create(entity_id);
    }

    ent->transform().tile_x = *x;
    ent->transform().tile_y = *y;
    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::stop);
}

void motion_handler::process_move(uint32_t entity_id, packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto speed = reader.read_u8();

    if (!x || !y || !dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) {
        ent = &game_->entities().create(entity_id);
    }

    ent->set_move_target(*x, *y);
    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::move);
    if (speed) {
        ent->set_move_speed(*speed);
    }
}

void motion_handler::process_run(uint32_t entity_id, packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();

    if (!x || !y || !dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) {
        ent = &game_->entities().create(entity_id);
    }

    ent->set_move_target(*x, *y);
    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::run);
}

void motion_handler::process_attack(uint32_t entity_id, packet_reader& reader) {
    auto dir = reader.read_u8();
    auto target_id = reader.read_u32();
    auto attack_type = reader.read_u8();

    if (!dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::attack);
    if (target_id) {
        ent->set_target(*target_id);
    }
    if (attack_type) {
        ent->set_attack_type(*attack_type);
    }
}

void motion_handler::process_attack_move(uint32_t entity_id, packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto target_id = reader.read_u32();

    if (!x || !y || !dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->set_move_target(*x, *y);
    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::attack_move);
    if (target_id) {
        ent->set_target(*target_id);
    }
}

void motion_handler::process_damage(uint32_t entity_id, packet_reader& reader) {
    auto damage = reader.read_i32();
    auto attacker_id = reader.read_u32();

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->set_action(object_action::damage);
    if (damage) {
        // Show damage number
        spdlog::debug("Entity {} took {} damage", entity_id, *damage);
    }
}

void motion_handler::process_damage_move(uint32_t entity_id, packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto damage = reader.read_i32();

    if (!x || !y || !dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->set_move_target(*x, *y);
    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::damage_move);
}

void motion_handler::process_magic(uint32_t entity_id, packet_reader& reader) {
    auto spell_id = reader.read_u16();
    auto dir = reader.read_u8();
    auto target_x = reader.read_i16();
    auto target_y = reader.read_i16();

    if (!spell_id || !dir) return;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->transform().direction = static_cast<direction>(*dir);
    ent->set_action(object_action::magic);
    ent->set_casting_spell(*spell_id);

    // Create magic effect at target location
    if (target_x && target_y) {
        game_->magic().create_effect(*spell_id, *target_x, *target_y);
    }
}

void motion_handler::process_get_item(uint32_t entity_id, packet_reader& reader) {
    (void)reader;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->set_action(object_action::get_item);
}

void motion_handler::process_dying(uint32_t entity_id, packet_reader& reader) {
    auto dir = reader.read_u8();

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    if (dir) {
        ent->transform().direction = static_cast<direction>(*dir);
    }
    ent->set_action(object_action::dying);
}

void motion_handler::process_dead(uint32_t entity_id, packet_reader& reader) {
    (void)reader;

    entity* ent = game_->entities().find(entity_id);
    if (!ent) return;

    ent->set_action(object_action::dead);
    // Entity will be removed after death animation completes
}

void motion_handler::process_spawn_object(packet_reader& reader) {
    auto entity_id = reader.read_u32();
    auto entity_type = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();
    auto dir = reader.read_u8();
    auto name = reader.read_string(20);

    if (!entity_id || !entity_type || !x || !y) return;

    entity& ent = game_->entities().create(*entity_id);
    ent.set_type(*entity_type);
    ent.transform().tile_x = *x;
    ent.transform().tile_y = *y;
    if (dir) {
        ent.transform().direction = static_cast<direction>(*dir);
    }
    if (name) {
        ent.set_name(*name);
    }

    spdlog::debug("Spawned entity {} at ({}, {})", *entity_id, *x, *y);
}

void motion_handler::process_despawn_object(packet_reader& reader) {
    auto entity_id = reader.read_u32();
    if (!entity_id) return;

    game_->entities().destroy(*entity_id);
    spdlog::debug("Despawned entity {}", *entity_id);
}

void motion_handler::process_magic_effect(packet_reader& reader) {
    auto effect_id = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!effect_id || !x || !y) return;

    game_->magic().create_effect(*effect_id, *x, *y);
}

void motion_handler::process_effect(packet_reader& reader) {
    auto effect_type = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!effect_type || !x || !y) return;

    // Generic visual effect
    spdlog::debug("Effect {} at ({}, {})", *effect_type, *x, *y);
}

} // namespace hb
