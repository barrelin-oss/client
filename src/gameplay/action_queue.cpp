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
            }
            break;

        case queued_action_type::pickup:
            game_->request_pickup(pending_action_.target_x, pending_action_.target_y);
            break;

        case queued_action_type::magic:
            game_->network().request_magic(pending_action_.spell_id,
                                           pending_action_.target_x, pending_action_.target_y,
                                           pending_action_.target_id);
            break;

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
