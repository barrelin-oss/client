#include "gameplay/magic.hpp"
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
    spdlog::debug("Creating visual effect {} at ({}, {})", effect_id, x, y);
    // TODO: Create effect entity in entity_manager
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
    // Attack spells - Circle 1
    {
        spell s;
        s.id = spell_id::energy_bolt;
        s.name = "Energy Bolt";
        s.description = "Basic magic attack";
        s.category = spell_category::attack;
        s.target_type = spell_target::single;
        s.type = magic_type::damage_spot;
        s.element = magic_element::none;
        s.circle = 1;
        s.level_req = 1;
        s.mp_cost = 5;
        s.base_damage = 10;
        s.cooldown = 1.0f;
        s.range = 8;
        s.cast_time = 0.5f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::magic_missile;
        s.name = "Magic Missile";
        s.description = "Fast magic projectile";
        s.category = spell_category::attack;
        s.target_type = spell_target::single;
        s.type = magic_type::damage_spot;
        s.element = magic_element::none;
        s.circle = 2;
        s.level_req = 5;
        s.mp_cost = 10;
        s.base_damage = 25;
        s.cooldown = 1.5f;
        s.range = 10;
        s.cast_time = 0.7f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::fire_ball;
        s.name = "Fire Ball";
        s.description = "Explosive fire magic";
        s.category = spell_category::attack;
        s.target_type = spell_target::area;
        s.type = magic_type::damage_area;
        s.element = magic_element::fire;
        s.circle = 3;
        s.level_req = 15;
        s.mp_cost = 30;
        s.base_damage = 50;
        s.cooldown = 3.0f;
        s.range = 8;
        s.aoe_radius = 2;
        s.cast_time = 1.0f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::ice_bolt;
        s.name = "Ice Bolt";
        s.description = "Freezing attack that slows";
        s.category = spell_category::attack;
        s.target_type = spell_target::single;
        s.type = magic_type::ice;
        s.element = magic_element::ice;
        s.circle = 2;
        s.level_req = 10;
        s.mp_cost = 15;
        s.base_damage = 20;
        s.duration = 3.0f;
        s.cooldown = 2.0f;
        s.range = 8;
        s.cast_time = 0.8f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::lightning;
        s.name = "Lightning";
        s.description = "Fast lightning strike";
        s.category = spell_category::attack;
        s.target_type = spell_target::single;
        s.type = magic_type::damage_spot;
        s.element = magic_element::lightning;
        s.circle = 3;
        s.level_req = 20;
        s.mp_cost = 35;
        s.base_damage = 60;
        s.cooldown = 2.5f;
        s.range = 10;
        s.cast_time = 0.5f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::blizzard;
        s.name = "Blizzard";
        s.description = "Massive ice storm";
        s.category = spell_category::attack;
        s.target_type = spell_target::area;
        s.type = magic_type::damage_area;
        s.element = magic_element::ice;
        s.circle = 6;
        s.level_req = 40;
        s.magic_req = 50;
        s.mp_cost = 100;
        s.base_damage = 150;
        s.cooldown = 8.0f;
        s.range = 10;
        s.aoe_radius = 4;
        s.cast_time = 2.5f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::meteor_strike;
        s.name = "Meteor Strike";
        s.description = "Call down a meteor";
        s.category = spell_category::attack;
        s.target_type = spell_target::area;
        s.type = magic_type::damage_area;
        s.element = magic_element::fire;
        s.circle = 6;
        s.level_req = 50;
        s.magic_req = 60;
        s.mp_cost = 150;
        s.base_damage = 250;
        s.cooldown = 15.0f;
        s.range = 12;
        s.aoe_radius = 5;
        s.cast_time = 3.0f;
        spells_[s.id] = s;
    }

    // Healing spells
    {
        spell s;
        s.id = spell_id::healing;
        s.name = "Healing";
        s.description = "Restore health";
        s.category = spell_category::healing;
        s.target_type = spell_target::single;
        s.type = magic_type::hp_up_spot;
        s.circle = 1;
        s.level_req = 1;
        s.mp_cost = 8;
        s.base_healing = 30;
        s.cooldown = 2.0f;
        s.range = 6;
        s.cast_time = 0.8f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::greater_healing;
        s.name = "Greater Healing";
        s.description = "Restore significant health";
        s.category = spell_category::healing;
        s.target_type = spell_target::single;
        s.type = magic_type::hp_up_spot;
        s.circle = 3;
        s.level_req = 20;
        s.mp_cost = 25;
        s.base_healing = 100;
        s.cooldown = 3.0f;
        s.range = 6;
        s.cast_time = 1.0f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::complete_healing;
        s.name = "Complete Healing";
        s.description = "Full health restoration";
        s.category = spell_category::healing;
        s.target_type = spell_target::single;
        s.type = magic_type::hp_up_spot;
        s.circle = 5;
        s.level_req = 40;
        s.mp_cost = 80;
        s.base_healing = 500;
        s.cooldown = 10.0f;
        s.range = 6;
        s.cast_time = 2.0f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::cure;
        s.name = "Cure";
        s.description = "Remove poison and debuffs";
        s.category = spell_category::healing;
        s.target_type = spell_target::single;
        s.type = magic_type::confuse;
        s.circle = 2;
        s.level_req = 10;
        s.mp_cost = 15;
        s.cooldown = 5.0f;
        s.range = 6;
        s.cast_time = 0.8f;
        spells_[s.id] = s;
    }

    // Buff spells
    {
        spell s;
        s.id = spell_id::protection;
        s.name = "Protection";
        s.description = "Increase defense";
        s.category = spell_category::buff;
        s.target_type = spell_target::single;
        s.type = magic_type::protect;
        s.circle = 2;
        s.level_req = 5;
        s.mp_cost = 15;
        s.duration = 180.0f;
        s.cooldown = 5.0f;
        s.range = 6;
        s.cast_time = 1.0f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::haste;
        s.name = "Haste";
        s.description = "Increase movement speed";
        s.category = spell_category::buff;
        s.target_type = spell_target::single;
        s.type = magic_type::protect;
        s.circle = 3;
        s.level_req = 15;
        s.mp_cost = 30;
        s.duration = 120.0f;
        s.cooldown = 10.0f;
        s.range = 6;
        s.cast_time = 1.2f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::strength;
        s.name = "Strength";
        s.description = "Increase attack power";
        s.category = spell_category::buff;
        s.target_type = spell_target::single;
        s.type = magic_type::protect;
        s.circle = 3;
        s.level_req = 15;
        s.mp_cost = 25;
        s.duration = 180.0f;
        s.cooldown = 10.0f;
        s.range = 6;
        s.cast_time = 1.0f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::invisibility;
        s.name = "Invisibility";
        s.description = "Become invisible";
        s.category = spell_category::buff;
        s.target_type = spell_target::self;
        s.type = magic_type::invisibility;
        s.circle = 4;
        s.level_req = 25;
        s.mp_cost = 50;
        s.duration = 60.0f;
        s.cooldown = 30.0f;
        s.range = 0;
        s.cast_time = 2.0f;
        spells_[s.id] = s;
    }

    // Debuff spells
    {
        spell s;
        s.id = spell_id::poison;
        s.name = "Poison";
        s.description = "Damage over time";
        s.category = spell_category::debuff;
        s.target_type = spell_target::single;
        s.type = magic_type::poison;
        s.circle = 2;
        s.level_req = 10;
        s.mp_cost = 20;
        s.base_damage = 50;  // Total DoT damage
        s.duration = 30.0f;
        s.cooldown = 5.0f;
        s.range = 8;
        s.cast_time = 0.8f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::paralyze;
        s.name = "Paralyze";
        s.description = "Immobilize target";
        s.category = spell_category::debuff;
        s.target_type = spell_target::single;
        s.type = magic_type::hold_object;
        s.circle = 4;
        s.level_req = 25;
        s.mp_cost = 40;
        s.duration = 5.0f;
        s.cooldown = 15.0f;
        s.range = 6;
        s.cast_time = 1.5f;
        spells_[s.id] = s;
    }

    // Utility spells
    {
        spell s;
        s.id = spell_id::recall;
        s.name = "Recall";
        s.description = "Teleport to town";
        s.category = spell_category::utility;
        s.target_type = spell_target::self;
        s.type = magic_type::teleport;
        s.circle = 1;
        s.level_req = 1;
        s.mp_cost = 20;
        s.cooldown = 60.0f;
        s.cast_time = 5.0f;
        spells_[s.id] = s;
    }

    {
        spell s;
        s.id = spell_id::detect_monster;
        s.name = "Detect Monster";
        s.description = "Reveal nearby monsters";
        s.category = spell_category::utility;
        s.target_type = spell_target::self;
        s.type = magic_type::create_dynamic;
        s.circle = 1;
        s.level_req = 5;
        s.mp_cost = 10;
        s.duration = 30.0f;
        s.cooldown = 10.0f;
        s.cast_time = 0.5f;
        spells_[s.id] = s;
    }
}

} // namespace hb
