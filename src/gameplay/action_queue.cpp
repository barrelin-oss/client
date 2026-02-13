#include "gameplay/action_queue.hpp"
#include "gameplay/game_state.hpp"
#include "core/direction_utils.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void action_queue::initialize(game_state_manager& game)
{
    game_ = &game;
}

void action_queue::clear()
{
    pending_action_ = queued_action{};
    blocked_movement_cooldown_ = 0.0f;
}

void action_queue::update_cooldown(float delta_time)
{
    if (blocked_movement_cooldown_ > 0.0f)
    {
        blocked_movement_cooldown_ -= delta_time;
        if (blocked_movement_cooldown_ < 0.0f)
        {
            blocked_movement_cooldown_ = 0.0f;
        }
    }
}

bool action_queue::can_perform_action() const
{
    if (blocked_movement_cooldown_ > 0.0f)
    {
        return false;
    }

    const entity* player = game_->local_player();
    if (!player) return false;

    if (player->transform().moving)
    {
        return false;
    }

    const auto& anim = player->animation();

    if (!anim.looping && !anim.finished)
    {
        return false;
    }

    return true;
}

void action_queue::queue_action(queued_action action)
{
    if (!can_perform_action())
    {
        bool same_action = (pending_action_.type == action.type &&
                           pending_action_.target_id == action.target_id &&
                           pending_action_.target_x == action.target_x &&
                           pending_action_.target_y == action.target_y &&
                           pending_action_.face_dir == action.face_dir);
        pending_action_ = action;
        if (!same_action)
        {
            spdlog::debug("Queued action type {}", static_cast<int>(action.type));
        }
    }
}

void action_queue::process_pending()
{
    if (pending_action_.type == queued_action_type::none)
    {
        return;
    }

    if (!can_perform_action())
    {
        return;
    }

    entity* player = game_->local_player();
    if (!player)
    {
        pending_action_.type = queued_action_type::none;
        return;
    }

    spdlog::debug("Executing queued action type {}", static_cast<int>(pending_action_.type));

    bool combat_mode = game_->is_combat_mode();

    switch (pending_action_.type)
    {
        case queued_action_type::move:
            game_->set_move_dest(pending_action_.target_x, pending_action_.target_y);
            break;

        case queued_action_type::attack:
            if (pending_action_.target_id != 0)
            {
                game_->network().request_attack(pending_action_.target_id, pending_action_.attack_type);

                // Immediate local attack animation
                if (entity* target = game_->entities().find(pending_action_.target_id))
                {
                    auto dir = calculate_direction(
                        player->transform().tile_x, player->transform().tile_y,
                        target->transform().tile_x, target->transform().tile_y);
                    if (dir)
                        player->transform().facing = *dir;

                    if (pending_action_.attack_type == static_cast<uint8_t>(attack_type::ranged))
                        player->set_action(object_action::attack_combat_bow);
                    else
                        player->set_action_with_combat_mode(object_action::attack_peace, combat_mode);
                }
            }
            break;

        case queued_action_type::pickup:
            game_->request_pickup(pending_action_.target_x, pending_action_.target_y);
            break;

        case queued_action_type::magic:
        {
            game_->ws_handler().request_magic(pending_action_.spell_id,
                                              pending_action_.target_x, pending_action_.target_y,
                                              pending_action_.target_id);

            // Play cast effects locally
            entity* player = game_->local_player();
            if (player)
            {
                player->set_action(object_action::magic);
                game_->magic().trigger_cooldown(pending_action_.spell_id);

                const spell* sp = game_->magic().get_spell(pending_action_.spell_id);
                if (sp)
                {
                    const auto& pt = player->transform();
                    float src_wx = static_cast<float>(pt.x);
                    float src_wy = static_cast<float>(pt.y);

                    // Target world position from entity or tile center
                    float dest_wx, dest_wy;
                    if (pending_action_.target_id != 0)
                    {
                        entity* target_ent = game_->entities().find(pending_action_.target_id);
                        if (target_ent)
                        {
                            dest_wx = static_cast<float>(target_ent->transform().x);
                            dest_wy = static_cast<float>(target_ent->transform().y);
                        }
                        else
                        {
                            dest_wx = static_cast<float>(pending_action_.target_x * 32 + 16);
                            dest_wy = static_cast<float>(pending_action_.target_y * 32 + 16);
                        }
                    }
                    else
                    {
                        dest_wx = static_cast<float>(pending_action_.target_x * 32 + 16);
                        dest_wy = static_cast<float>(pending_action_.target_y * 32 + 16);
                    }

                    if (sp->projectile_effect != 0)
                    {
                        game_->effects().add_effect_world(
                            static_cast<effect_type_id>(sp->projectile_effect),
                            src_wx, src_wy, dest_wx, dest_wy);
                    }
                    else if (sp->effect_sprite != 0)
                    {
                        game_->effects().add_effect_at_pixel(
                            static_cast<effect_type_id>(sp->effect_sprite),
                            dest_wx, dest_wy);
                    }
                }
            }
            break;
        }

        case queued_action_type::face_direction:
        {
            game_->set_move_dest(-1, -1);
            if (entity* p = game_->local_player(); p && pending_action_.face_dir)
            {
                auto& t = p->transform();
                json msg = make_player_stop_request(t.tile_x, t.tile_y,
                                                    static_cast<uint8_t>(direction_to_protocol(*pending_action_.face_dir)));
                game_->ws_connection().send(msg);
                t.moving = false;
                t.facing = *pending_action_.face_dir;
                p->set_action_with_combat_mode(object_action::stop_peace, combat_mode);
            }
            break;
        }

        case queued_action_type::stop:
        {
            game_->set_move_dest(-1, -1);
            if (entity* p = game_->local_player())
            {
                auto& t = p->transform();
                if (t.moving)
                {
                    json msg = make_player_stop_request(t.tile_x, t.tile_y,
                                                        static_cast<uint8_t>(direction_to_protocol(t.facing)));
                    game_->ws_connection().send(msg);
                }
                t.moving = false;
                p->set_action_with_combat_mode(object_action::stop_peace, combat_mode);
            }
            break;
        }

        default:
            break;
    }

    pending_action_.type = queued_action_type::none;
}

} // namespace hb
