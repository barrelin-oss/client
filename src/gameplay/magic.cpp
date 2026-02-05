#include "gameplay/magic.hpp"
#include "gameplay/effect_system.hpp"
#include "gameplay/combat.hpp"
#include "entity/entity_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace hb {

void magic_system::initialize() {
    load_spell_definitions();
    spdlog::info("Magic system initialized with {} spells", spells_.size());
}

void magic_system::initialize_with_systems(entity_manager* entities, combat_system* combat) {
    entities_ = entities;
    combat_ = combat;
    load_spell_definitions();
    spdlog::info("Magic system initialized with {} spells", spells_.size());
}

void magic_system::clear() {
    for (auto& [id, spell] : spells_) {
        spell.learned = false;
        spell.mastery_level = 0;
        spell.experience = 0;
    }
    active_effects_.clear();
    casting_entities_.clear();
    cooldowns_.clear();
}

void magic_system::learn_spell(uint16_t spell_id) {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        it->second.learned = true;
        spdlog::debug("Learned spell: {}", it->second.name);
    }
}

void magic_system::forget_spell(uint16_t spell_id) {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        it->second.learned = false;
    }
}

bool magic_system::has_spell(uint16_t spell_id) const {
    auto it = spells_.find(spell_id);
    return it != spells_.end() && it->second.learned;
}

bool magic_system::can_cast(uint16_t spell_id, int32_t mp, int32_t sp, int32_t hp) const {
    auto it = spells_.find(spell_id);
    if (it == spells_.end() || !it->second.learned) {
        return false;
    }

    const auto& spell = it->second;
    return mp >= spell.mp_cost && sp >= spell.sp_cost && hp > spell.hp_cost;
}

bool magic_system::can_cast_at_target(uint16_t spell_id, uint32_t caster_id, uint32_t target_id) const {
    const spell* s = get_spell(spell_id);
    if (!s || !s->learned) return false;
    if (is_on_cooldown(spell_id)) return false;

    // Check range
    if (!check_range(caster_id, target_id, s->range)) return false;

    // Self-target spells must target self
    if (s->target_type == spell_target::self && caster_id != target_id) return false;

    return true;
}

bool magic_system::can_cast_at_location(uint16_t spell_id, uint32_t caster_id, int32_t x, int32_t y) const {
    const spell* s = get_spell(spell_id);
    if (!s || !s->learned) return false;
    if (is_on_cooldown(spell_id)) return false;

    // Check range
    if (!check_range_location(caster_id, x, y, s->range)) return false;

    // Must be ground-targeted or area spell
    if (s->target_type != spell_target::ground && s->target_type != spell_target::area) return false;

    return true;
}

bool magic_system::start_casting(uint16_t spell_id, uint32_t caster_id, uint32_t target_id) {
    if (!can_cast_at_target(spell_id, caster_id, target_id)) return false;

    const spell* s = get_spell(spell_id);
    if (!s) return false;

    // Set up casting info
    casting_info info;
    info.spell_id = spell_id;
    info.target_id = target_id;
    info.cast_progress = 0.0f;
    info.cast_duration = s->cast_time;
    info.state = casting_state::casting;

    casting_entities_[caster_id] = info;

    if (on_casting_started_) {
        on_casting_started_(spell_id, caster_id);
    }

    spdlog::debug("Entity {} started casting {} on {}", caster_id, s->name, target_id);
    return true;
}

bool magic_system::start_casting_at_location(uint16_t spell_id, uint32_t caster_id, int32_t x, int32_t y) {
    if (!can_cast_at_location(spell_id, caster_id, x, y)) return false;

    const spell* s = get_spell(spell_id);
    if (!s) return false;

    casting_info info;
    info.spell_id = spell_id;
    info.target_x = x;
    info.target_y = y;
    info.cast_progress = 0.0f;
    info.cast_duration = s->cast_time;
    info.state = casting_state::casting;

    casting_entities_[caster_id] = info;

    if (on_casting_started_) {
        on_casting_started_(spell_id, caster_id);
    }

    return true;
}

void magic_system::cancel_casting(uint32_t caster_id) {
    auto it = casting_entities_.find(caster_id);
    if (it != casting_entities_.end()) {
        it->second.state = casting_state::idle;
        casting_entities_.erase(it);
    }
}

void magic_system::interrupt_casting(uint32_t caster_id) {
    auto it = casting_entities_.find(caster_id);
    if (it != casting_entities_.end()) {
        it->second.state = casting_state::interrupted;
        if (on_casting_interrupted_) {
            on_casting_interrupted_(caster_id);
        }
        casting_entities_.erase(it);
    }
}

void magic_system::update_casting(float delta_time) {
    for (auto it = casting_entities_.begin(); it != casting_entities_.end(); ) {
        auto& info = it->second;

        if (info.state == casting_state::casting) {
            info.cast_progress += delta_time;

            if (info.cast_progress >= info.cast_duration) {
                finish_casting(it->first);
                it = casting_entities_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

casting_state magic_system::get_casting_state(uint32_t caster_id) const {
    auto it = casting_entities_.find(caster_id);
    if (it != casting_entities_.end()) {
        return it->second.state;
    }
    return casting_state::idle;
}

float magic_system::get_casting_progress(uint32_t caster_id) const {
    auto it = casting_entities_.find(caster_id);
    if (it != casting_entities_.end() && it->second.cast_duration > 0) {
        return it->second.cast_progress / it->second.cast_duration;
    }
    return 0.0f;
}

void magic_system::finish_casting(uint32_t caster_id) {
    auto it = casting_entities_.find(caster_id);
    if (it == casting_entities_.end()) return;

    const auto& info = it->second;
    int32_t magic_power = 0;

    // Get caster magic power
    if (entities_) {
        entity* caster = entities_->get_entity(caster_id);
        if (caster && caster->has_stats()) {
            magic_power = caster->stats().magic_power;
        }
    }

    // Execute the spell
    spell_cast_result result;
    if (info.target_id != 0) {
        result = execute_spell(info.spell_id, caster_id, info.target_id, magic_power);
    } else {
        result = execute_spell_at_location(info.spell_id, caster_id, info.target_x, info.target_y, magic_power);
    }

    if (on_spell_cast_) {
        on_spell_cast_(info.spell_id, caster_id, result);
    }
}

spell_cast_result magic_system::execute_spell(uint16_t spell_id, uint32_t caster_id, uint32_t target_id, int32_t magic_power) {
    spell_cast_result result;

    const spell* s = get_spell(spell_id);
    if (!s) {
        result.error_message = "Spell not found";
        return result;
    }

    result.mp_cost = s->calculate_mp_cost(0);  // Would use caster intelligence
    result.affected_targets.push_back(target_id);

    // Apply spell effect based on type
    switch (s->type) {
        case magic_type::damage_spot: {
            result.damage_dealt = s->calculate_damage(magic_power);
            if (combat_ && entities_) {
                combat_->apply_damage(target_id, result.damage_dealt);
            }
            break;
        }

        case magic_type::hp_up_spot: {
            result.healing_done = s->calculate_healing(magic_power);
            if (combat_) {
                combat_->heal_entity(target_id, result.healing_done);
            }
            break;
        }

        case magic_type::poison: {
            // Apply poison effect
            spell_effect effect;
            effect.spell_id = spell_id;
            effect.caster_id = caster_id;
            effect.target_id = target_id;
            effect.remaining_duration = s->duration;
            effect.power = s->calculate_damage(magic_power) / 10;  // Damage per second
            add_effect(effect);
            break;
        }

        case magic_type::hold_object:
        case magic_type::ice: {
            // Apply paralysis/freeze
            if (entities_) {
                entity* target = entities_->get_entity(target_id);
                if (target && target->has_combat()) {
                    if (s->type == magic_type::hold_object) {
                        target->combat().paralyzed = true;
                    } else {
                        target->combat().frozen = true;
                    }
                    target->combat().status_duration = s->duration;
                }
            }
            break;
        }

        case magic_type::protect:
        case magic_type::create: {
            // Apply buff effect
            spell_effect effect;
            effect.spell_id = spell_id;
            effect.caster_id = caster_id;
            effect.target_id = target_id;
            effect.remaining_duration = s->duration;
            effect.power = magic_power / 10;
            add_effect(effect);
            break;
        }

        case magic_type::confuse: {
            // Remove debuffs
            if (entities_) {
                entity* target = entities_->get_entity(target_id);
                if (target && target->has_combat()) {
                    target->combat().poisoned = false;
                    target->combat().paralyzed = false;
                    target->combat().frozen = false;
                }
            }
            break;
        }

        case magic_type::invisibility: {
            // Apply invisibility
            if (entities_) {
                entity* target = entities_->get_entity(target_id);
                if (target && target->has_combat()) {
                    target->combat().invisible = true;
                    target->combat().status_duration = s->duration;
                }
            }
            spell_effect effect;
            effect.spell_id = spell_id;
            effect.caster_id = caster_id;
            effect.target_id = target_id;
            effect.remaining_duration = s->duration;
            add_effect(effect);
            break;
        }

        default:
            break;
    }

    // Spawn visual effect at target position
    if (s->effect_sprite != 0 && entities_)
    {
        entity* target = entities_->get_entity(target_id);
        if (target)
        {
            const auto& t = target->transform();
            create_effect(s->effect_sprite, t.tile_x, t.tile_y);
        }
    }

    // Trigger cooldown
    trigger_cooldown(spell_id);

    // Add spell experience
    add_spell_experience(spell_id, 10);

    result.success = true;
    spdlog::debug("Spell {} executed: damage={}, healing={}", s->name, result.damage_dealt, result.healing_done);
    return result;
}

spell_cast_result magic_system::execute_spell_at_location(uint16_t spell_id, [[maybe_unused]] uint32_t caster_id, int32_t x, int32_t y, int32_t magic_power) {
    spell_cast_result result;

    const spell* s = get_spell(spell_id);
    if (!s) {
        result.error_message = "Spell not found";
        return result;
    }

    result.mp_cost = s->calculate_mp_cost(0);

    // For AoE spells, find all targets in radius
    if (s->target_type == spell_target::area && s->aoe_radius > 0 && entities_) {
        auto targets = entities_->get_entities_in_range(x, y, s->aoe_radius * 32);
        for (entity* target : targets) {
            result.affected_targets.push_back(target->id());

            if (s->type == magic_type::damage_area) {
                int32_t damage = s->calculate_damage(magic_power);
                result.damage_dealt += damage;
                if (combat_) {
                    combat_->apply_damage(target->id(), damage);
                }
            } else if (s->type == magic_type::sp_up_area) {
                int32_t healing = s->calculate_healing(magic_power);
                result.healing_done += healing;
                if (combat_) {
                    combat_->heal_entity(target->id(), healing);
                }
            }
        }
    }

    // Create visual effect
    create_effect(s->effect_sprite, x, y);

    trigger_cooldown(spell_id);
    add_spell_experience(spell_id, 10);

    result.success = true;
    return result;
}

const spell* magic_system::get_spell(uint16_t spell_id) const {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        return &it->second;
    }
    return nullptr;
}

spell* magic_system::get_spell_mut(uint16_t spell_id) {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        return &it->second;
    }
    return nullptr;
}

const std::vector<const spell*> magic_system::get_learned_spells() const {
    std::vector<const spell*> result;
    for (const auto& [id, spell] : spells_) {
        if (spell.learned) {
            result.push_back(&spell);
        }
    }
    return result;
}

std::vector<const spell*> magic_system::get_all_spells() const {
    std::vector<const spell*> result;
    result.reserve(spells_.size());
    for (const auto& [id, spell] : spells_) {
        result.push_back(&spell);
    }
    return result;
}

const std::vector<const spell*> magic_system::get_spells_by_category(spell_category category) const {
    std::vector<const spell*> result;
    for (const auto& [id, spell] : spells_) {
        if (spell.learned && spell.category == category) {
            result.push_back(&spell);
        }
    }
    return result;
}

std::vector<const spell*> magic_system::get_spells_by_circle(uint8_t circle) const {
    std::vector<const spell*> result;
    for (const auto& [id, spell] : spells_) {
        if (spell.learned && spell.circle == circle) {
            result.push_back(&spell);
        }
    }
    return result;
}

void magic_system::add_spell_experience(uint16_t spell_id, uint32_t exp) {
    auto it = spells_.find(spell_id);
    if (it == spells_.end()) return;

    auto& spell = it->second;
    spell.experience += exp;

    // Check for mastery level up (every 1000 exp)
    uint8_t new_level = static_cast<uint8_t>(std::min(20u, spell.experience / 1000));
    if (new_level > spell.mastery_level) {
        spell.mastery_level = new_level;
        spdlog::info("Spell {} reached mastery level {}", spell.name, new_level);
    }
}

void magic_system::set_spell_mastery(uint16_t spell_id, uint8_t level) {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        it->second.mastery_level = std::min(static_cast<uint8_t>(20), level);
        it->second.experience = level * 1000;
    }
}

void magic_system::set_spell_total_casts(uint16_t spell_id, int32_t total_casts) {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        it->second.total_casts = total_casts;
    }
}

uint8_t magic_system::get_mastery_level(uint16_t spell_id) const {
    auto it = spells_.find(spell_id);
    if (it != spells_.end()) {
        return it->second.mastery_level;
    }
    return 0;
}

void magic_system::add_effect(const spell_effect& effect) {
    // Remove existing effect of same type on same target
    remove_effect(effect.spell_id, effect.target_id);
    active_effects_.push_back(effect);
}

void magic_system::remove_effect(uint16_t spell_id, uint32_t target_id) {
    active_effects_.erase(
        std::remove_if(active_effects_.begin(), active_effects_.end(),
            [spell_id, target_id](const spell_effect& e) {
                return e.spell_id == spell_id && e.target_id == target_id;
            }),
        active_effects_.end()
    );
}

void magic_system::update_effects(float delta_time) {
    for (auto it = active_effects_.begin(); it != active_effects_.end(); ) {
        it->remaining_duration -= delta_time;

        // Apply periodic damage for DoT effects
        const spell* s = get_spell(it->spell_id);
        if (s && s->type == magic_type::poison && it->power > 0) {
            // Deal damage every second
            static float dot_timer = 0.0f;
            dot_timer += delta_time;
            if (dot_timer >= 1.0f) {
                dot_timer = 0.0f;
                if (combat_) {
                    combat_->apply_damage(it->target_id, it->power);
                }
            }
        }

        if (it->remaining_duration <= 0) {
            if (on_effect_expired_) {
                on_effect_expired_(*it);
            }
            it = active_effects_.erase(it);
        } else {
            ++it;
        }
    }

    // Update casting progress
    update_casting(delta_time);

    // Update cooldowns
    update_cooldowns(delta_time);
}

std::vector<spell_effect> magic_system::get_effects_on_target(uint32_t target_id) const {
    std::vector<spell_effect> result;
    for (const auto& effect : active_effects_) {
        if (effect.target_id == target_id) {
            result.push_back(effect);
        }
    }
    return result;
}

bool magic_system::has_effect(uint32_t target_id, uint16_t spell_id) const {
    for (const auto& effect : active_effects_) {
        if (effect.target_id == target_id && effect.spell_id == spell_id) {
            return true;
        }
    }
    return false;
}

void magic_system::create_effect(uint16_t effect_id, int32_t x, int32_t y) {
    if (effects_)
    {
        effects_->add_effect_at(static_cast<effect_type_id>(effect_id), x, y);
    }
    else
    {
        spdlog::debug("Creating visual effect {} at ({}, {}) - no effect system", effect_id, x, y);
    }
}

void magic_system::add_active_effect(uint16_t effect_id, uint32_t duration) {
    spdlog::debug("Adding active effect {} for {} ms", effect_id, duration);
}

void magic_system::remove_active_effect(uint16_t effect_id) {
    spdlog::debug("Removing active effect {}", effect_id);
}

bool magic_system::is_on_cooldown(uint16_t spell_id) const {
    return cooldowns_.find(spell_id) != cooldowns_.end();
}

float magic_system::get_cooldown_remaining(uint16_t spell_id) const {
    auto it = cooldowns_.find(spell_id);
    if (it != cooldowns_.end()) {
        return it->second;
    }
    return 0.0f;
}

void magic_system::trigger_cooldown(uint16_t spell_id) {
    const spell* s = get_spell(spell_id);
    if (s && s->cooldown > 0) {
        cooldowns_[spell_id] = s->cooldown;
    }
}

void magic_system::update_cooldowns(float delta_time) {
    for (auto it = cooldowns_.begin(); it != cooldowns_.end(); ) {
        it->second -= delta_time;
        if (it->second <= 0) {
            it = cooldowns_.erase(it);
        } else {
            ++it;
        }
    }
}

bool magic_system::check_range(uint32_t caster_id, uint32_t target_id, int32_t range) const {
    if (!entities_) return true;

    entity* caster = entities_->get_entity(caster_id);
    entity* target = entities_->get_entity(target_id);

    if (!caster || !target) return false;

    const auto& caster_t = caster->transform();
    const auto& target_t = target->transform();

    int32_t dx = std::abs(caster_t.tile_x - target_t.tile_x);
    int32_t dy = std::abs(caster_t.tile_y - target_t.tile_y);
    int32_t distance = std::max(dx, dy);

    return distance <= range;
}

bool magic_system::check_range_location(uint32_t caster_id, int32_t x, int32_t y, int32_t range) const {
    if (!entities_) return true;

    entity* caster = entities_->get_entity(caster_id);
    if (!caster) return false;

    const auto& caster_t = caster->transform();

    int32_t target_tile_x = x / 32;
    int32_t target_tile_y = y / 32;

    int32_t dx = std::abs(caster_t.tile_x - target_tile_x);
    int32_t dy = std::abs(caster_t.tile_y - target_tile_y);
    int32_t distance = std::max(dx, dy);

    return distance <= range;
}

void magic_system::load_spell_definitions() {
    load_default_spells();
}

void magic_system::load_default_spells() {
    // Helper: derive circle from spell ID (id / 10 + 1)
    auto set_circle = [](spell& s) { s.circle = static_cast<uint8_t>(s.id / 10 + 1); };

    // Helper to add a spell and auto-set circle
    auto add = [&](spell s) {
        set_circle(s);
        s.cast_sprite = 57;
        spells_[s.id] = s;
    };

    // === Level 1 (IDs 0-9) ===

    add({.id = spell_id::magic_missile, .name = "Magic Missile",
         .description = "Fast magic projectile",
         .category = spell_category::attack, .target_type = spell_target::single,
         .type = magic_type::damage_spot, .element = magic_element::lightning,
         .int_req = 18, .mp_cost = 8, .base_damage = 8, .range = 8,
         .projectile_effect = 100, .effect_sprite = 7, .cast_time = 0.5f});

    add({.id = spell_id::heal, .name = "Heal",
         .description = "Restore health",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::hp_up_spot,
         .int_req = 20, .mp_cost = 15, .base_healing = 22, .range = 6,
         .effect_sprite = 101, .cast_time = 0.8f});

    add({.id = spell_id::create_food, .name = "Create Food",
         .description = "Conjure food from nothing",
         .category = spell_category::utility, .target_type = spell_target::self,
         .type = magic_type::create,
         .int_req = 18, .mp_cost = 18,
         .effect_sprite = 114, .cast_time = 1.0f});

    // === Level 2 (IDs 10-19) ===

    add({.id = spell_id::energy_bolt, .name = "Energy Bolt",
         .description = "Concentrated energy blast",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::lightning,
         .int_req = 24, .mp_cost = 15, .base_damage = 10, .range = 8, .aoe_radius = 2,
         .projectile_effect = 110, .effect_sprite = 6, .cast_time = 0.7f});

    add({.id = spell_id::staminar_drain, .name = "Stamina Drain",
         .description = "Drain target stamina",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::sp_down_area,
         .int_req = 22, .mp_cost = 14, .base_damage = 34, .range = 8,
         .effect_sprite = 62, .cast_time = 0.7f});

    add({.id = spell_id::recall, .name = "Recall",
         .description = "Teleport to town",
         .category = spell_category::utility, .target_type = spell_target::self,
         .type = magic_type::teleport,
         .int_req = 20, .mp_cost = 15,
         .effect_sprite = 112, .cast_time = 5.0f});

    add({.id = spell_id::defense_shield, .name = "Defense Shield",
         .description = "Increase physical defense",
         .category = spell_category::buff, .target_type = spell_target::single,
         .type = magic_type::protect,
         .int_req = 26, .mp_cost = 19, .duration = 60.0f, .range = 6,
         .effect_sprite = 113, .cast_time = 1.0f});

    add({.id = spell_id::celebrating_light, .name = "Celebrating Light",
         .description = "Healing light",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::hp_up_spot, .element = magic_element::lightning,
         .int_req = 25, .mp_cost = 20, .base_healing = 10, .range = 6,
         .effect_sprite = 114, .cast_time = 0.8f});

    // === Level 3 (IDs 20-29) ===

    add({.id = spell_id::fire_ball, .name = "Fire Ball",
         .description = "Explosive fire magic",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::fire,
         .int_req = 26, .mp_cost = 27, .base_damage = 16, .range = 8, .aoe_radius = 2,
         .projectile_effect = 120, .effect_sprite = 5, .cast_time = 1.0f});

    add({.id = spell_id::great_heal, .name = "Great Heal",
         .description = "Restore significant health",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::hp_up_spot,
         .int_req = 28, .mp_cost = 28, .base_healing = 60, .range = 6,
         .effect_sprite = 121, .cast_time = 1.0f});

    add({.id = spell_id::staminar_recovery, .name = "Stamina Recovery",
         .description = "Restore stamina to target",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::sp_up_area,
         .int_req = 20, .mp_cost = 20, .base_healing = 40, .range = 8,
         .effect_sprite = 114, .cast_time = 0.8f});

    add({.id = spell_id::protection_from_arrow, .name = "Protection from Arrow",
         .description = "Shield from ranged attacks",
         .category = spell_category::buff, .target_type = spell_target::single,
         .type = magic_type::protect,
         .int_req = 20, .mp_cost = 22, .duration = 60.0f, .range = 6,
         .effect_sprite = 52, .cast_time = 1.0f});

    add({.id = spell_id::hold_person, .name = "Hold Person",
         .description = "Paralyze a target briefly",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::hold_object,
         .int_req = 26, .mp_cost = 24, .duration = 30.0f, .range = 6,
         .effect_sprite = 135, .cast_time = 1.0f});

    add({.id = spell_id::possession, .name = "Possession",
         .description = "Possess a creature",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::possession,
         .int_req = 26, .mp_cost = 25, .range = 6,
         .effect_sprite = 62, .cast_time = 1.5f});

    add({.id = spell_id::poison, .name = "Poison",
         .description = "Damage over time",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::poison, .element = magic_element::earth,
         .int_req = 29, .mp_cost = 28, .base_damage = 15, .range = 8,
         .effect_sprite = 62, .cast_time = 0.8f});

    add({.id = spell_id::great_staminar_recovery, .name = "Great Stamina Recovery",
         .description = "Restore large amount of stamina",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::sp_up_area,
         .int_req = 30, .mp_cost = 45, .base_healing = 80, .range = 8,
         .effect_sprite = 114, .cast_time = 1.0f});

    // === Level 4 (IDs 30-39) ===

    add({.id = spell_id::fire_strike, .name = "Fire Strike",
         .description = "Powerful fire attack",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::fire,
         .int_req = 34, .mp_cost = 36, .base_damage = 22, .range = 8, .aoe_radius = 2,
         .projectile_effect = 120, .effect_sprite = 5, .cast_time = 1.0f});

    add({.id = spell_id::summon_creature, .name = "Summon Creature",
         .description = "Summon a creature to fight",
         .category = spell_category::summon, .target_type = spell_target::ground,
         .type = magic_type::summon, .element = magic_element::earth,
         .int_req = 38, .mp_cost = 35, .range = 6,
         .effect_sprite = 62, .cast_time = 2.0f});

    add({.id = spell_id::invisibility, .name = "Invisibility",
         .description = "Become invisible",
         .category = spell_category::buff, .target_type = spell_target::self,
         .type = magic_type::invisibility,
         .int_req = 30, .mp_cost = 31, .duration = 60.0f,
         .effect_sprite = 132, .cast_time = 2.0f});

    add({.id = spell_id::protection_from_magic, .name = "Protection from Magic",
         .description = "Shield from magic attacks",
         .category = spell_category::buff, .target_type = spell_target::single,
         .type = magic_type::protect,
         .int_req = 32, .mp_cost = 35, .duration = 60.0f, .range = 6,
         .effect_sprite = 52, .cast_time = 1.0f});

    add({.id = spell_id::detect_invisibility, .name = "Detect Invisibility",
         .description = "Reveal invisible creatures",
         .category = spell_category::utility, .target_type = spell_target::self,
         .type = magic_type::invisibility,
         .int_req = 30, .mp_cost = 33,
         .effect_sprite = 113, .cast_time = 0.8f});

    add({.id = spell_id::paralyze, .name = "Paralyze",
         .description = "Immobilize target",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::hold_object,
         .int_req = 36, .mp_cost = 35, .duration = 15.0f, .range = 6,
         .effect_sprite = 135, .cast_time = 1.5f});

    add({.id = spell_id::cure, .name = "Cure",
         .description = "Remove poison and debuffs",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::poison, .element = magic_element::earth,
         .int_req = 35, .mp_cost = 32, .range = 6,
         .effect_sprite = 136, .cast_time = 0.8f});

    add({.id = spell_id::lightning_arrow, .name = "Lightning Arrow",
         .description = "Electric bolt attack",
         .category = spell_category::attack, .target_type = spell_target::single,
         .type = magic_type::damage_spot, .element = magic_element::lightning,
         .int_req = 38, .mp_cost = 32, .base_damage = 30, .range = 8,
         .projectile_effect = 137, .effect_sprite = 63, .cast_time = 0.5f});

    add({.id = spell_id::tremor, .name = "Tremor",
         .description = "Shake the earth",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::tremor, .element = magic_element::earth,
         .int_req = 33, .mp_cost = 34, .base_damage = 18, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 1.0f});

    // === Level 5 (IDs 40-49) ===

    add({.id = spell_id::fire_wall, .name = "Fire Wall",
         .description = "Create a wall of fire",
         .category = spell_category::attack, .target_type = spell_target::ground,
         .type = magic_type::create_dynamic, .element = magic_element::fire,
         .int_req = 45, .mp_cost = 42, .base_damage = 16, .duration = 30.0f, .range = 8,
         .effect_sprite = 5, .cast_time = 1.5f});

    add({.id = spell_id::fire_field, .name = "Fire Field",
         .description = "Create a field of flames",
         .category = spell_category::attack, .target_type = spell_target::ground,
         .type = magic_type::create_dynamic, .element = magic_element::fire,
         .int_req = 48, .mp_cost = 48, .base_damage = 16, .duration = 30.0f, .range = 8,
         .effect_sprite = 5, .cast_time = 1.5f});

    add({.id = spell_id::confuse_language, .name = "Confuse Language",
         .description = "Scramble target speech",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::confuse,
         .int_req = 42, .mp_cost = 40, .duration = 120.0f, .range = 6,
         .effect_sprite = 62, .cast_time = 1.0f});

    add({.id = spell_id::lightning, .name = "Lightning",
         .description = "Powerful lightning bolt",
         .category = spell_category::attack, .target_type = spell_target::single,
         .type = magic_type::damage_spot, .element = magic_element::lightning,
         .int_req = 47, .mp_cost = 44, .base_damage = 40, .range = 8,
         .projectile_effect = 137, .effect_sprite = 63, .cast_time = 0.5f});

    add({.id = spell_id::great_defense_shield, .name = "Great Defense Shield",
         .description = "Powerful defense buff",
         .category = spell_category::buff, .target_type = spell_target::single,
         .type = magic_type::protect,
         .int_req = 46, .mp_cost = 45, .duration = 40.0f, .range = 6,
         .effect_sprite = 113, .cast_time = 1.0f});

    add({.id = spell_id::chill_wind, .name = "Chill Wind",
         .description = "Icy wind that damages and slows",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::ice, .element = magic_element::ice,
         .int_req = 50, .mp_cost = 48, .base_damage = 22, .range = 8, .aoe_radius = 2,
         .effect_sprite = 40, .cast_time = 1.0f});

    add({.id = spell_id::poison_cloud, .name = "Poison Cloud",
         .description = "Create a cloud of poison",
         .category = spell_category::attack, .target_type = spell_target::ground,
         .type = magic_type::create_dynamic, .element = magic_element::earth,
         .int_req = 49, .mp_cost = 48, .base_damage = 15, .duration = 30.0f, .range = 8,
         .effect_sprite = 62, .cast_time = 1.5f});

    add({.id = spell_id::triple_energy_bolt, .name = "Triple Energy Bolt",
         .description = "Three energy bolts at once",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::lightning,
         .int_req = 45, .mp_cost = 40, .base_damage = 14, .range = 8, .aoe_radius = 2,
         .projectile_effect = 110, .effect_sprite = 6, .cast_time = 0.8f});

    // === Level 6 (IDs 50-59) ===

    add({.id = spell_id::berserk, .name = "Berserk",
         .description = "Enter a berserk rage",
         .category = spell_category::buff, .target_type = spell_target::self,
         .type = magic_type::berserk,
         .int_req = 59, .mp_cost = 57, .duration = 40.0f,
         .effect_sprite = 150, .cast_time = 1.5f});

    add({.id = spell_id::lightning_bolt, .name = "Lightning Bolt",
         .description = "Devastating lightning",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::lightning,
         .int_req = 58, .mp_cost = 58, .base_damage = 36, .range = 8, .aoe_radius = 2,
         .projectile_effect = 137, .effect_sprite = 63, .cast_time = 0.8f});

    add({.id = spell_id::mass_poison, .name = "Mass Poison",
         .description = "Poison multiple targets",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::poison, .element = magic_element::earth,
         .int_req = 52, .mp_cost = 54, .base_damage = 40, .range = 8,
         .effect_sprite = 62, .cast_time = 1.0f});

    add({.id = spell_id::spike_field, .name = "Spike Field",
         .description = "Create a field of spikes",
         .category = spell_category::attack, .target_type = spell_target::ground,
         .type = magic_type::create_dynamic, .element = magic_element::earth,
         .int_req = 56, .mp_cost = 56, .base_damage = 16, .duration = 30.0f, .range = 8,
         .effect_sprite = 62, .cast_time = 1.5f});

    add({.id = spell_id::ice_storm, .name = "Ice Storm",
         .description = "Frozen area denial",
         .category = spell_category::attack, .target_type = spell_target::ground,
         .type = magic_type::create_dynamic, .element = magic_element::ice,
         .int_req = 59, .mp_cost = 58, .duration = 30.0f, .range = 8,
         .effect_sprite = 40, .cast_time = 1.5f});

    add({.id = spell_id::mass_lightning_arrow, .name = "Mass Lightning Arrow",
         .description = "Lightning strikes all nearby",
         .category = spell_category::attack, .target_type = spell_target::single,
         .type = magic_type::damage_spot, .element = magic_element::lightning,
         .int_req = 53, .mp_cost = 55, .base_damage = 50, .range = 8,
         .projectile_effect = 137, .effect_sprite = 63, .cast_time = 0.5f});

    add({.id = spell_id::ice_strike, .name = "Ice Strike",
         .description = "Powerful ice attack",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::ice, .element = magic_element::ice,
         .int_req = 60, .mp_cost = 59, .base_damage = 42, .range = 8, .aoe_radius = 2,
         .effect_sprite = 40, .cast_time = 1.0f});

    // === Level 7 (IDs 60-69) ===

    add({.id = spell_id::energy_strike, .name = "Energy Strike",
         .description = "Massive energy attack",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area_no_spot, .element = magic_element::lightning,
         .int_req = 67, .mp_cost = 65, .base_damage = 59, .range = 8, .aoe_radius = 2,
         .effect_sprite = 6, .cast_time = 1.0f});

    add({.id = spell_id::mass_fire_strike, .name = "Mass Fire Strike",
         .description = "Fire rains from the sky",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::fire,
         .int_req = 85, .mp_cost = 80, .base_damage = 54, .range = 8, .aoe_radius = 2,
         .projectile_effect = 120, .effect_sprite = 5, .cast_time = 1.5f});

    add({.id = spell_id::confusion, .name = "Confusion",
         .description = "Confuse nearby enemies",
         .category = spell_category::debuff, .target_type = spell_target::area,
         .type = magic_type::confuse,
         .int_req = 75, .mp_cost = 78, .duration = 20.0f, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 1.0f});

    add({.id = spell_id::mass_chill_wind, .name = "Mass Chill Wind",
         .description = "Massive freezing wind",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::ice, .element = magic_element::ice,
         .int_req = 93, .mp_cost = 90, .base_damage = 48, .range = 8, .aoe_radius = 2,
         .effect_sprite = 40, .cast_time = 1.5f});

    add({.id = spell_id::earthworm_strike, .name = "Earthworm Strike",
         .description = "Earth erupts under enemies",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::earthworm_strike, .element = magic_element::earth,
         .int_req = 97, .mp_cost = 80, .base_damage = 59, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 1.5f});

    add({.id = spell_id::absolute_magic_protection, .name = "Absolute Magic Protection",
         .description = "Immunity to magic",
         .category = spell_category::buff, .target_type = spell_target::single,
         .type = magic_type::protect,
         .int_req = 112, .mp_cost = 90, .duration = 60.0f, .range = 6,
         .effect_sprite = 52, .cast_time = 1.5f});

    add({.id = spell_id::armor_break, .name = "Armor Break",
         .description = "Destroy target armor",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::armor_break, .element = magic_element::earth,
         .int_req = 97, .mp_cost = 90, .base_damage = 59, .range = 6,
         .effect_sprite = 62, .cast_time = 1.0f});

    // === Level 8 (IDs 70-79) ===

    add({.id = spell_id::bloody_shock_wave, .name = "Bloody Shock Wave",
         .description = "Devastating shockwave",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::bloody_shock_wave, .element = magic_element::lightning,
         .int_req = 105, .mp_cost = 180, .base_damage = 60, .range = 8, .aoe_radius = 2,
         .effect_sprite = 6, .cast_time = 2.0f});

    add({.id = spell_id::mass_confusion, .name = "Mass Confusion",
         .description = "Confuse all nearby enemies",
         .category = spell_category::debuff, .target_type = spell_target::area,
         .type = magic_type::confuse,
         .int_req = 130, .mp_cost = 125, .duration = 60.0f, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 1.5f});

    add({.id = spell_id::mass_ice_strike, .name = "Mass Ice Strike",
         .description = "Devastating ice attack",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::ice, .element = magic_element::ice,
         .int_req = 133, .mp_cost = 120, .base_damage = 59, .range = 8, .aoe_radius = 2,
         .effect_sprite = 40, .cast_time = 1.5f});

    add({.id = spell_id::cloud_kill, .name = "Cloud Kill",
         .description = "Deadly poison cloud",
         .category = spell_category::attack, .target_type = spell_target::ground,
         .type = magic_type::create_dynamic, .element = magic_element::earth,
         .int_req = 120, .mp_cost = 130, .base_damage = 40, .duration = 30.0f, .range = 8,
         .effect_sprite = 62, .cast_time = 2.0f});

    add({.id = spell_id::lightning_strike, .name = "Lightning Strike",
         .description = "Massive lightning bolt",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area_no_spot, .element = magic_element::lightning,
         .int_req = 123, .mp_cost = 90, .base_damage = 69, .range = 8, .aoe_radius = 2,
         .effect_sprite = 63, .cast_time = 1.0f});

    add({.id = spell_id::cancellation, .name = "Cancellation",
         .description = "Remove all buffs from target",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::cancellation,
         .int_req = 135, .mp_cost = 120, .duration = 30.0f, .range = 6,
         .effect_sprite = 62, .cast_time = 1.0f});

    add({.id = spell_id::illusion_movement, .name = "Illusion Movement",
         .description = "Create moving illusion",
         .category = spell_category::debuff, .target_type = spell_target::area,
         .type = magic_type::confuse,
         .int_req = 160, .mp_cost = 150, .duration = 30.0f, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 1.5f});

    // === Level 9 (IDs 80-89) ===

    add({.id = spell_id::illusion, .name = "Illusion",
         .description = "Create a powerful illusion",
         .category = spell_category::debuff, .target_type = spell_target::area,
         .type = magic_type::confuse,
         .int_req = 150, .mp_cost = 143, .duration = 20.0f, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 1.5f});

    add({.id = spell_id::meteor_strike, .name = "Meteor Strike",
         .description = "Call down a meteor",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::damage_area, .element = magic_element::fire,
         .int_req = 169, .mp_cost = 120, .base_damage = 72, .range = 10, .aoe_radius = 2,
         .effect_sprite = 61, .cast_time = 3.0f});

    add({.id = spell_id::mass_magic_missile, .name = "Mass Magic Missile",
         .description = "Magic missiles strike all",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::mass_magic_missile, .element = magic_element::lightning,
         .int_req = 185, .mp_cost = 160, .base_damage = 72, .range = 8, .aoe_radius = 2,
         .projectile_effect = 100, .effect_sprite = 7, .cast_time = 1.5f});

    add({.id = spell_id::inhibition_casting, .name = "Inhibition Casting",
         .description = "Prevent target from casting",
         .category = spell_category::debuff, .target_type = spell_target::single,
         .type = magic_type::inhibition_casting,
         .int_req = 172, .mp_cost = 160, .duration = 40.0f, .range = 6,
         .effect_sprite = 62, .cast_time = 1.5f});

    // === Level 10 (IDs 90-99) ===

    add({.id = spell_id::mass_illusion, .name = "Mass Illusion",
         .description = "Powerful mass illusion",
         .category = spell_category::debuff, .target_type = spell_target::area,
         .type = magic_type::confuse,
         .int_req = 180, .mp_cost = 200, .duration = 60.0f, .range = 10, .aoe_radius = 3,
         .effect_sprite = 62, .cast_time = 2.0f});

    add({.id = spell_id::blizzard, .name = "Blizzard",
         .description = "Devastating ice storm",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::blizzard, .element = magic_element::ice,
         .int_req = 195, .mp_cost = 170, .base_damage = 72, .range = 8, .aoe_radius = 2,
         .effect_sprite = 72, .cast_time = 2.5f});

    add({.id = spell_id::resurrection, .name = "Resurrection",
         .description = "Bring the dead back to life",
         .category = spell_category::healing, .target_type = spell_target::single,
         .type = magic_type::resurrection, .element = magic_element::ice,
         .int_req = 0, .mp_cost = 200, .base_healing = 60, .range = 6,
         .effect_sprite = 121, .cast_time = 5.0f});

    add({.id = spell_id::mass_illusion_movement, .name = "Mass Illusion Movement",
         .description = "Moving illusions for all",
         .category = spell_category::debuff, .target_type = spell_target::area,
         .type = magic_type::confuse,
         .int_req = 200, .mp_cost = 200, .duration = 60.0f, .range = 10, .aoe_radius = 3,
         .effect_sprite = 62, .cast_time = 2.0f});

    add({.id = spell_id::earth_shock_wave, .name = "Earth Shock Wave",
         .description = "Devastating earth attack",
         .category = spell_category::attack, .target_type = spell_target::area,
         .type = magic_type::earth_shock_wave, .element = magic_element::earth,
         .int_req = 200, .mp_cost = 180, .base_damage = 72, .range = 8, .aoe_radius = 2,
         .effect_sprite = 62, .cast_time = 2.0f});
}

} // namespace hb
