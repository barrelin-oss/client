#include "gameplay/skills.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

void skills_system::initialize() {
    load_skill_definitions();
    spdlog::info("Skills system initialized with {} skills", skills_.size());
}

void skills_system::clear() {
    for (auto& [id, skill] : skills_) {
        skill.experience = 0;
    }
    cooldowns_.clear();
}

const skill* skills_system::get_skill(uint16_t skill_id) const {
    auto it = skills_.find(skill_id);
    if (it != skills_.end()) {
        return &it->second;
    }
    return nullptr;
}

skill* skills_system::get_skill_mut(uint16_t skill_id) {
    auto it = skills_.find(skill_id);
    if (it != skills_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const skill*> skills_system::get_skills_by_category(skill_category category) const {
    std::vector<const skill*> result;
    for (const auto& [id, skill] : skills_) {
        if (skill.category == category) {
            result.push_back(&skill);
        }
    }
    return result;
}

std::vector<const skill*> skills_system::get_all_skills() const {
    std::vector<const skill*> result;
    for (const auto& [id, skill] : skills_) {
        result.push_back(&skill);
    }
    return result;
}

void skills_system::add_skill_experience(uint16_t skill_id, uint32_t exp) {
    auto it = skills_.find(skill_id);
    if (it == skills_.end()) return;

    auto& skill = it->second;
    uint8_t old_level = skill.level();

    skill.experience = std::min(skill.experience + exp, skill::max_experience);

    uint8_t new_level = skill.level();
    if (new_level > old_level && on_skill_up_) {
        on_skill_up_(skill_id, new_level);
    }
}

void skills_system::set_skill_experience(uint16_t skill_id, uint32_t exp) {
    auto it = skills_.find(skill_id);
    if (it != skills_.end()) {
        it->second.experience = std::min(exp, skill::max_experience);
    }
}

void skills_system::set_mastery(uint16_t skill_id, uint8_t mastery) {
    auto it = skills_.find(skill_id);
    if (it != skills_.end()) {
        // Convert mastery (0-100) to experience (0-1000000)
        it->second.experience = static_cast<uint32_t>(mastery) * 10000;
    }
}

uint8_t skills_system::get_skill_level(uint16_t skill_id) const {
    auto it = skills_.find(skill_id);
    if (it != skills_.end()) {
        return it->second.level();
    }
    return 0;
}

float skills_system::get_damage_bonus() const {
    float bonus = 0.0f;

    // Attack skill provides base damage bonus
    if (auto* s = get_skill(skill_id::attack)) {
        bonus += s->level() * 0.5f;
    }

    // Critical hit skill provides critical damage bonus
    if (auto* s = get_skill(skill_id::critical_hit)) {
        bonus += s->level() * 0.2f;
    }

    return bonus;
}

float skills_system::get_defense_bonus() const {
    float bonus = 0.0f;

    if (auto* s = get_skill(skill_id::defense)) {
        bonus += s->level() * 0.3f;
    }

    if (auto* s = get_skill(skill_id::shield_block)) {
        bonus += s->level() * 0.2f;
    }

    return bonus;
}

float skills_system::get_crafting_success_rate(uint16_t recipe_id) const {
    (void)recipe_id;  // Would use recipe database

    float base_rate = 50.0f;

    if (auto* s = get_skill(skill_id::manufacturing)) {
        base_rate += s->level() * 0.5f;
    }

    return std::min(base_rate, 95.0f);
}

float skills_system::get_gathering_bonus() const {
    float bonus = 1.0f;

    if (auto* s = get_skill(skill_id::mining)) {
        bonus += s->level() * 0.01f;
    }

    if (auto* s = get_skill(skill_id::fishing)) {
        bonus += s->level() * 0.01f;
    }

    return bonus;
}

bool skills_system::can_use_skill(uint16_t skill_id, int32_t mp, int32_t sp) const {
    auto* s = get_skill(skill_id);
    if (!s || s->is_passive) return false;

    return mp >= s->mp_cost && sp >= s->sp_cost && is_skill_ready(skill_id);
}

void skills_system::update_cooldowns(float delta_time) {
    for (auto it = cooldowns_.begin(); it != cooldowns_.end(); ) {
        it->second -= delta_time;
        if (it->second <= 0) {
            it = cooldowns_.erase(it);
        } else {
            ++it;
        }
    }
}

bool skills_system::is_skill_ready(uint16_t skill_id) const {
    return cooldowns_.find(skill_id) == cooldowns_.end();
}

void skills_system::trigger_cooldown(uint16_t skill_id) {
    auto* s = get_skill(skill_id);
    if (s && s->cooldown > 0) {
        cooldowns_[skill_id] = s->cooldown;
    }
}

void skills_system::load_skill_definitions() {
    load_default_skills();
}

void skills_system::load_default_skills() {
    // Combat skills
    {
        skill s;
        s.id = skill_id::attack;
        s.name = "Attack";
        s.description = "Physical attack proficiency";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::defense;
        s.name = "Defense";
        s.description = "Physical defense proficiency";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::critical_hit;
        s.name = "Critical Hit";
        s.description = "Chance to deal critical damage";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::shield_block;
        s.name = "Shield Block";
        s.description = "Chance to block attacks";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::short_sword;
        s.name = "Short Sword";
        s.description = "Short sword mastery";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::long_sword;
        s.name = "Long Sword";
        s.description = "Long sword mastery";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::axe;
        s.name = "Axe";
        s.description = "Axe mastery";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::archery;
        s.name = "Archery";
        s.description = "Bow and arrow proficiency";
        s.category = skill_category::combat;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    // Magic skills
    {
        skill s;
        s.id = skill_id::magic_resistance;
        s.name = "Magic Resistance";
        s.description = "Resistance to magical attacks";
        s.category = skill_category::magic;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::meditation;
        s.name = "Meditation";
        s.description = "MP regeneration rate";
        s.category = skill_category::magic;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::fire_magic;
        s.name = "Fire Magic";
        s.description = "Fire spell proficiency";
        s.category = skill_category::magic;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::ice_magic;
        s.name = "Ice Magic";
        s.description = "Ice spell proficiency";
        s.category = skill_category::magic;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    // Crafting skills
    {
        skill s;
        s.id = skill_id::alchemy;
        s.name = "Alchemy";
        s.description = "Potion crafting";
        s.category = skill_category::crafting;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::blacksmithing;
        s.name = "Blacksmithing";
        s.description = "Weapon and armor crafting";
        s.category = skill_category::crafting;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::manufacturing;
        s.name = "Manufacturing";
        s.description = "General item crafting";
        s.category = skill_category::crafting;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    // Gathering skills
    {
        skill s;
        s.id = skill_id::mining;
        s.name = "Mining";
        s.description = "Mine ore and gems";
        s.category = skill_category::gathering;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::fishing;
        s.name = "Fishing";
        s.description = "Catch fish";
        s.category = skill_category::gathering;
        s.is_passive = true;
        skills_[s.id] = s;
    }

    // Misc skills
    {
        skill s;
        s.id = skill_id::pretend_corpse;
        s.name = "Pretend Corpse";
        s.description = "Play dead to avoid enemies";
        s.category = skill_category::misc;
        s.is_passive = false;
        s.sp_cost = 20;
        s.cooldown = 60.0f;
        skills_[s.id] = s;
    }

    {
        skill s;
        s.id = skill_id::poison_resistance;
        s.name = "Poison Resistance";
        s.description = "Resistance to poison";
        s.category = skill_category::misc;
        s.is_passive = true;
        skills_[s.id] = s;
    }
}

} // namespace hb
