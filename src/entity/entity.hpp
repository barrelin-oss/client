#pragma once

#include "entity/components.hpp"
#include "core/game_enums.hpp"
#include <cassert>
#include <cstdint>
#include <optional>
#include <variant>
#include <string>

namespace hb {

// Entity types
enum class entity_type : uint8_t {
    none = 0,
    player = 1,       // Local player
    character = 2,    // Other players
    npc = 3,
    monster = 4,
    item = 5,
    effect = 6,
};

// Unique entity ID
using entity_id = uint32_t;
inline constexpr entity_id invalid_entity_id = 0;

// Base entity class
class entity {
public:
    entity() = default;
    explicit entity(entity_id id, entity_type type);
    ~entity() = default;

    entity(const entity&) = delete;
    entity& operator=(const entity&) = delete;
    entity(entity&&) noexcept = default;
    entity& operator=(entity&&) noexcept = default;

    // Identity
    entity_id id() const { return id_; }
    entity_type type() const { return type_; }
    bool valid() const { return id_ != invalid_entity_id; }

    // Component access - all entities have these
    transform_component& transform() { return transform_; }
    const transform_component& transform() const { return transform_; }

    sprite_component& sprite() { return sprite_; }
    const sprite_component& sprite() const { return sprite_; }

    animation_component& animation() { return animation_; }
    const animation_component& animation() const { return animation_; }

    // Optional components - check has_X() before accessing, or use assertions
    bool has_stats() const { return stats_.has_value(); }
    stats_component& stats() { assert(stats_.has_value() && "stats component required"); return stats_.value(); }
    const stats_component& stats() const { assert(stats_.has_value() && "stats component required"); return stats_.value(); }
    void add_stats() { stats_.emplace(); }

    bool has_combat() const { return combat_.has_value(); }
    combat_component& combat() { assert(combat_.has_value() && "combat component required"); return combat_.value(); }
    const combat_component& combat() const { assert(combat_.has_value() && "combat component required"); return combat_.value(); }
    void add_combat() { combat_.emplace(); }

    bool has_name() const { return name_.has_value(); }
    name_component& name() { assert(name_.has_value() && "name component required"); return name_.value(); }
    const name_component& name() const { assert(name_.has_value() && "name component required"); return name_.value(); }
    void add_name() { name_.emplace(); }

    bool has_movement() const { return movement_.has_value(); }
    movement_component& movement() { assert(movement_.has_value() && "movement component required"); return movement_.value(); }
    const movement_component& movement() const { assert(movement_.has_value() && "movement component required"); return movement_.value(); }
    void add_movement() { movement_.emplace(); }

    bool has_npc() const { return npc_.has_value(); }
    npc_component& npc() { assert(npc_.has_value() && "npc component required"); return npc_.value(); }
    const npc_component& npc() const { assert(npc_.has_value() && "npc component required"); return npc_.value(); }
    void add_npc() { npc_.emplace(); }

    bool has_monster() const { return monster_.has_value(); }
    monster_component& monster() { assert(monster_.has_value() && "monster component required"); return monster_.value(); }
    const monster_component& monster() const { assert(monster_.has_value() && "monster component required"); return monster_.value(); }
    void add_monster() { monster_.emplace(); }

    bool has_item() const { return item_.has_value(); }
    item_component& item() { assert(item_.has_value() && "item component required"); return item_.value(); }
    const item_component& item() const { assert(item_.has_value() && "item component required"); return item_.value(); }
    void add_item() { item_.emplace(); }

    bool has_effect() const { return effect_.has_value(); }
    effect_component& effect() { assert(effect_.has_value() && "effect component required"); return effect_.value(); }
    const effect_component& effect() const { assert(effect_.has_value() && "effect component required"); return effect_.value(); }
    void add_effect() { effect_.emplace(); }

    // Lifecycle
    bool is_alive() const { return alive_; }
    void set_alive(bool alive) { alive_ = alive; }
    bool should_remove() const { return should_remove_; }
    void mark_for_removal() { should_remove_ = true; }

    // Convenience methods for motion handling
    void set_action(object_action action);
    void set_action_with_combat_mode(object_action base_action, bool combat_mode);
    object_action current_action() const { return current_action_; }
    void set_move_target(int32_t x, int32_t y);
    void set_move_speed(uint8_t speed);
    void set_target(entity_id target);
    void set_attack_type(uint8_t attack_type);
    void set_casting_spell(uint16_t spell_id);
    void set_type(uint16_t visual_type);
    void set_name(const std::string& name);

private:
    entity_id id_ = invalid_entity_id;
    entity_type type_ = entity_type::none;
    bool alive_ = true;
    bool should_remove_ = false;

    // Core components (always present)
    transform_component transform_;
    sprite_component sprite_;
    animation_component animation_;

    // Optional components
    std::optional<stats_component> stats_;
    std::optional<combat_component> combat_;
    std::optional<name_component> name_;
    std::optional<movement_component> movement_;
    std::optional<npc_component> npc_;
    std::optional<monster_component> monster_;
    std::optional<item_component> item_;
    std::optional<effect_component> effect_;

    // Motion state
    object_action current_action_ = object_action::stop_peace;
    int32_t target_x_ = 0;
    int32_t target_y_ = 0;
    uint8_t move_speed_ = 1;
    entity_id target_id_ = invalid_entity_id;
    uint8_t attack_type_ = 0;
    uint16_t casting_spell_ = 0;
    uint16_t visual_type_ = 0;
};

} // namespace hb
