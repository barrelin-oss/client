#include "entity/entity.hpp"

namespace hb {

entity::entity(entity_id id, entity_type type)
    : id_(id), type_(type) {
}

void entity::set_action(object_action action) {
    current_action_ = action;
    animation_.current_frame = 0;
    animation_.frame_timer = 0.0f;
}

void entity::set_move_target(int32_t x, int32_t y) {
    target_x_ = x;
    target_y_ = y;
    if (!movement_.has_value()) {
        add_movement();
    }
    movement_->target_x = x;
    movement_->target_y = y;
    movement_->moving = true;
}

void entity::set_move_speed(uint8_t speed) {
    move_speed_ = speed;
    if (movement_.has_value()) {
        movement_->speed = static_cast<float>(speed);
    }
}

void entity::set_target(entity_id target) {
    target_id_ = target;
    if (combat_.has_value()) {
        combat_->target_id = target;
    }
}

void entity::set_attack_type(uint8_t attack_type) {
    attack_type_ = attack_type;
    if (!combat_.has_value()) {
        add_combat();
    }
    combat_->attack_type = attack_type;
}

void entity::set_casting_spell(uint16_t spell_id) {
    casting_spell_ = spell_id;
}

void entity::set_type(uint16_t visual_type) {
    visual_type_ = visual_type;
}

void entity::set_name(const std::string& entity_name) {
    if (!name_.has_value()) {
        add_name();
    }
    name_->name = entity_name;
}

} // namespace hb
