#include "entity/entity.hpp"
#include "entity/npc_animation_data.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

entity::entity(entity_id id, entity_type type) : id_(id), type_(type) {}

void entity::set_action(object_action action)
{
    // Dead entities can only be set to dying (any other action is a race condition)
    if (!alive_ && action != object_action::dying)
        return;

    current_action_ = action;

    // Map action to entity_anim_state for animation system
    entity_anim_state new_state;
    switch (action)
    {
    case object_action::stop_peace:
    case object_action::stop_combat:
        new_state = entity_anim_state::stop;
        break;
    case object_action::move_peace:
    case object_action::move_combat:
        new_state = entity_anim_state::move;
        break;
    case object_action::run:
        new_state = entity_anim_state::run;
        break;
    case object_action::attack_peace:
    case object_action::attack_combat:
    case object_action::attack_combat_bow:
        new_state = entity_anim_state::attack;
        break;
    case object_action::magic:
        new_state = entity_anim_state::magic;
        break;
    case object_action::get_item:
        new_state = entity_anim_state::get_item;
        break;
    case object_action::damage:
        new_state = entity_anim_state::damage;
        break;
    case object_action::dying:
        new_state = entity_anim_state::dying;
        break;
    default:
        new_state = entity_anim_state::stop;
        break;
    }

    // Debug: log any state change on dead entities that resets a finished dying animation
    if (new_state == entity_anim_state::dying && animation_.state == entity_anim_state::dying && animation_.finished)
    {
        spdlog::warn("Entity {} dying animation RESET (was finished, set_action called again)", id_);
    }

    animation_.set_state(new_state);

    // Apply NPC-specific frame overrides at state-change time.
    // This replaces the per-frame apply_npc_frame_data() call.
    if (auto entry = get_npc_frame_data(visual_type_, new_state))
    {
        if (entry->max_frame >= 0)
            animation_.frame_count = static_cast<uint8_t>(entry->max_frame + 1);
        if (entry->frame_time_ms > 0)
            animation_.frame_duration = static_cast<float>(entry->frame_time_ms) / 1000.0f;
    }
}

// Add helper to apply combat mode to actions
void entity::set_action_with_combat_mode(object_action base_action, bool combat_mode)
{
    object_action final_action = base_action;

    // Apply combat variants for compatible actions
    if (combat_mode)
    {
        switch (base_action)
        {
        case object_action::stop_peace:
            final_action = object_action::stop_combat;
            break;
        case object_action::move_peace:
            final_action = object_action::move_combat;
            break;
        case object_action::attack_peace:
            final_action = object_action::attack_combat;
            break;
        default:
            // Other actions don't have combat variants
            break;
        }
    }

    set_action(final_action);
}

void entity::set_move_target(int32_t x, int32_t y)
{
    target_x_ = x;
    target_y_ = y;
    if (!movement_.has_value())
    {
        add_movement();
    }
    movement_->target_x = x;
    movement_->target_y = y;
    movement_->moving = true;
}

void entity::set_move_speed(uint8_t speed)
{
    move_speed_ = speed;
    // Create movement component on-demand (consistent with set_move_target)
    if (!movement_.has_value())
    {
        add_movement();
    }
    movement_->speed = static_cast<float>(speed);
}

void entity::set_target(entity_id target)
{
    target_id_ = target;
    // Create combat component on-demand (consistent with set_attack_type)
    if (!combat_.has_value())
    {
        add_combat();
    }
    combat_->target_id = target;
}

void entity::set_attack_type(uint8_t attack_type)
{
    attack_type_ = attack_type;
    if (!combat_.has_value())
    {
        add_combat();
    }
    combat_->attack_type = attack_type;
}

void entity::set_casting_spell(uint16_t spell_id)
{
    casting_spell_ = spell_id;
}

void entity::set_type(uint16_t visual_type)
{
    visual_type_ = visual_type;
}

void entity::set_name(const std::string& entity_name)
{
    if (!name_.has_value())
    {
        add_name();
    }
    name_->name = entity_name;
}

void entity::begin_fade_out(float duration)
{
    if (fading_out_ || should_remove_)
        return;
    fading_out_ = true;
    fade_elapsed_ = 0.0f;
    fade_duration_ = duration;
}

void entity::update_fade(float delta_time)
{
    if (!fading_out_)
        return;

    fade_elapsed_ += delta_time;
    float t = fade_elapsed_ / fade_duration_;
    if (t >= 1.0f)
    {
        sprite_.alpha = 0.0f;
        mark_for_removal();
    }
    else
    {
        sprite_.alpha = 1.0f - t;
    }
}

} // namespace hb
