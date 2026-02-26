#include "gameplay/combat.hpp"
#include "audio/sound_manager.hpp"
#include "core/random.hpp"
#include "entity/entity_manager.hpp"
#include "gameplay/inventory.hpp"
#include "gameplay/magic.hpp"
#include "gameplay/skills.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hb
{

namespace
{

bool roll_percent(int32_t chance)
{
    return random_int(1, 100) <= chance;
}
} // namespace

void combat_system::initialize(entity_manager* entities,
                               magic_system* magic,
                               skills_system* skills,
                               sound_manager* sounds,
                               inventory_system* inventory)
{
    entities_ = entities;
    magic_ = magic;
    skills_ = skills;
    sounds_ = sounds;
    inventory_ = inventory;
    spdlog::info("Combat system initialized");
}

void combat_system::update(float delta_time)
{
    (void)delta_time;
    // Combat cooldowns and effects are updated in entity_manager
}

damage_result combat_system::calculate_damage(const attack_params& params)
{
    damage_result result;

    // Check for dodge
    if (roll_hit(params.hit_ratio, params.target_dodge))
    {
        result.is_dodged = true;
        return result;
    }

    // Calculate base damage
    int32_t base_damage = calculate_base_damage(params.attack_power, params.target_defense);

    // Check for critical hit
    if (roll_critical(params.critical_ratio))
    {
        result.is_critical = true;
        base_damage = apply_critical_bonus(base_damage);
    }

    // Apply weapon skill bonus
    weapon_skill skill = get_weapon_skill(params.weapon);
    uint8_t mastery = 0;
    if (skills_)
    {
        mastery = skills_->get_skill_level(static_cast<uint16_t>(skill));
    }
    base_damage = apply_skill_bonus(base_damage, skill, mastery);

    // Apply super attack multiplier
    if (params.type >= attack_type::super_1 && params.type <= attack_type::super_8)
    {
        int super_level = static_cast<int>(params.type) - static_cast<int>(attack_type::super_1) + 1;
        float multiplier = 1.0f + super_level * 0.3f;
        float scaled_damage = static_cast<float>(base_damage) * multiplier;
        // Saturate to prevent overflow (max reasonable damage is INT32_MAX)
        base_damage = static_cast<int32_t>(std::min(scaled_damage, static_cast<float>(INT32_MAX)));
    }

    // Minimum damage of 1
    result.damage = std::max(1, base_damage);

    return result;
}

bool combat_system::can_attack(entity_id attacker, entity_id target) const
{
    if (!entities_)
        return false;

    entity* att = entities_->get_entity(attacker);
    entity* tgt = entities_->get_entity(target);

    if (!att || !tgt)
        return false;
    if (att->should_remove() || tgt->should_remove())
        return false;

    // Check if attacker can fight
    if (!att->has_combat())
        return false;
    if (att->combat().attack_cooldown > 0)
        return false;
    if (att->combat().paralyzed || att->combat().frozen)
        return false;

    // Check if target is valid
    if (!tgt->has_stats())
        return false;
    if (tgt->stats().hp <= 0)
        return false;

    // Check range
    if (!is_in_melee_range(attacker, target) && !is_in_ranged_range(attacker, target))
    {
        return false;
    }

    // Combat mode check
    const auto& combat = att->combat();
    switch (combat.mode)
    {
    case combat_component::combat_mode::peaceful:
        return tgt->type() == entity_type::monster;

    case combat_component::combat_mode::attack_player:
        return tgt->type() == entity_type::character || tgt->type() == entity_type::player;

    case combat_component::combat_mode::attack_monster:
        return tgt->type() == entity_type::monster;

    case combat_component::combat_mode::full_attack:
        return true;
    }

    return false;
}

void combat_system::start_attack(entity_id attacker, entity_id target, attack_type type)
{
    if (!can_attack(attacker, target))
        return;

    entity* att = entities_->get_entity(attacker);
    if (!att)
        return;

    // Set attack state
    att->combat().is_attacking = true;
    att->combat().target_id = target;
    att->combat().attack_type = static_cast<uint8_t>(type);

    // Get equipped weapon type for animation and sound selection
    weapon_type wt = weapon_type::none;
    if (inventory_)
    {
        if (const auto* weapon = inventory_->get_equipped_item(equip_pos::weapon))
        {
            wt = weapon->weapon;
        }
    }

    // Set animation - use bow animation for ranged weapons
    if (type == attack_type::ranged || is_bow_weapon(wt))
    {
        att->set_action(object_action::attack_combat_bow);
    }
    else
    {
        att->set_action(object_action::attack_peace);
    }

    play_attack_sound(attacker, wt);

    // Play critical sound for super attacks
    if (type >= attack_type::super_1 && type <= attack_type::super_8)
    {
        play_critical_sound(attacker);
    }

    // Trigger attack start callback
    if (callbacks_.on_attack_start)
    {
        callbacks_.on_attack_start(attacker, target, type);
    }

    spdlog::debug("Entity {} starts attack on {} with type {}", attacker, target, static_cast<int>(type));
}

void combat_system::process_attack(entity_id attacker, entity_id target, int32_t damage)
{
    entity* att = entities_->get_entity(attacker);
    entity* tgt = entities_->get_entity(target);

    if (!att || !tgt)
        return;

    // Apply damage
    apply_damage(target, damage);

    // Check if target died
    bool killed = tgt->stats().hp <= 0;

    // Create damage result for callback
    damage_result result;
    result.damage = damage;
    result.target_killed = killed;

    // Award experience if monster killed
    if (killed && tgt->type() == entity_type::monster && tgt->has_monster())
    {
        result.experience_gained = tgt->monster().exp_reward;
        award_experience(attacker, result.experience_gained);
    }

    // Set attack cooldown
    float cooldown = 1.0f;
    if (att->has_combat())
    {
        att->combat().attack_cooldown = cooldown;
        att->combat().is_attacking = false;
    }

    // Trigger damage callback
    if (callbacks_.on_damage)
    {
        callbacks_.on_damage(attacker, target, result);
    }

    // Handle death
    if (killed)
    {
        kill_entity(target);

        // PK handling
        if (tgt->type() == entity_type::character || tgt->type() == entity_type::player)
        {
            add_pk_count(attacker);
        }
    }
}

attack_type combat_system::get_attack_type(weapon_type wt, uint8_t skill_mastery, bool use_super) const
{
    // Bow weapons use ranged attack
    if (is_bow_weapon(wt))
    {
        return attack_type::ranged;
    }

    // Super attacks require 100% mastery
    if (use_super && skill_mastery >= 100)
    {
        // Determine which super attack based on weapon type
        weapon_skill skill = get_weapon_skill(wt);
        switch (skill)
        {
        case weapon_skill::sword:
            return attack_type::super_1;
        case weapon_skill::axe:
            return attack_type::super_2;
        case weapon_skill::hammer:
            return attack_type::super_3;
        case weapon_skill::two_hand:
            return attack_type::super_4;
        case weapon_skill::spear:
            return attack_type::super_5;
        case weapon_skill::archery:
            return attack_type::super_6;
        case weapon_skill::staff:
            return attack_type::super_7;
        case weapon_skill::fist:
            return attack_type::super_8;
        default:
            break;
        }
    }

    return attack_type::normal;
}

weapon_skill combat_system::get_weapon_skill(weapon_type wt) const
{
    switch (wt)
    {
    case weapon_type::fist:
    case weapon_type::none:
        return weapon_skill::fist;
    case weapon_type::sword:
    case weapon_type::dagger:
        return weapon_skill::sword;
    case weapon_type::axe:
        return weapon_skill::axe;
    case weapon_type::hammer:
        return weapon_skill::hammer;
    case weapon_type::staff:
    case weapon_type::wand:
        return weapon_skill::staff;
    case weapon_type::bow:
        return weapon_skill::archery;
    }
    return weapon_skill::fist;
}

bool combat_system::is_in_melee_range(entity_id attacker, entity_id target) const
{
    if (!entities_)
        return false;

    entity* att = entities_->get_entity(attacker);
    entity* tgt = entities_->get_entity(target);

    if (!att || !tgt)
        return false;

    const auto& att_t = att->transform();
    const auto& tgt_t = tgt->transform();

    // Check if adjacent tile (including diagonals)
    int32_t dx = std::abs(att_t.tile_x - tgt_t.tile_x);
    int32_t dy = std::abs(att_t.tile_y - tgt_t.tile_y);

    return dx <= 1 && dy <= 1;
}

bool combat_system::is_in_dash_range(entity_id attacker, entity_id target) const
{
    if (!entities_)
        return false;

    entity* att = entities_->get_entity(attacker);
    entity* tgt = entities_->get_entity(target);

    if (!att || !tgt)
        return false;

    const auto& att_t = att->transform();
    const auto& tgt_t = tgt->transform();

    int32_t dx = att_t.tile_x - tgt_t.tile_x;
    int32_t dy = att_t.tile_y - tgt_t.tile_y;
    int32_t adx = std::abs(dx);
    int32_t ady = std::abs(dy);

    // Chebyshev distance must be exactly 2
    if (std::max(adx, ady) != 2)
        return false;

    // Must be cardinal or diagonal (not L-shaped)
    // Cardinal: one axis is 0, other is 2
    // Diagonal: both axes are 2
    return adx == 0 || ady == 0 || adx == ady;
}

bool combat_system::can_dash_attack(entity_id attacker) const
{
    if (!entities_ || !skills_ || !inventory_)
        return false;

    entity* att = entities_->get_entity(attacker);
    if (!att)
        return false;

    // Get equipped weapon type
    const auto* weapon = inventory_->get_equipped_item(equip_pos::weapon);
    weapon_type wt = weapon ? weapon->weapon : weapon_type::none;

    // Bows cannot dash
    if (is_bow_weapon(wt))
        return false;

    // Check if weapon skill is mastered (level 100)
    weapon_skill skill = get_weapon_skill(wt);
    return skills_->is_skill_mastered(static_cast<uint16_t>(skill));
}

bool combat_system::is_in_ranged_range(entity_id attacker, entity_id target) const
{
    if (!entities_)
        return false;

    entity* att = entities_->get_entity(attacker);
    entity* tgt = entities_->get_entity(target);

    if (!att || !tgt)
        return false;

    const auto& att_t = att->transform();
    const auto& tgt_t = tgt->transform();

    int32_t dx = std::abs(att_t.tile_x - tgt_t.tile_x);
    int32_t dy = std::abs(att_t.tile_y - tgt_t.tile_y);
    int32_t distance = std::max(dx, dy);

    // Ranged attack range is 10 tiles
    return distance <= 10;
}

int32_t combat_system::get_attack_range(attack_type type, weapon_type /*wt*/) const
{

    switch (type)
    {
    case attack_type::ranged:
        return 10;
    case attack_type::normal:
    case attack_type::super_1:
    case attack_type::super_2:
    case attack_type::super_3:
    case attack_type::super_4:
    case attack_type::super_5:
    case attack_type::super_6:
    case attack_type::super_7:
    case attack_type::super_8:
    default:
        return 1;
    }
}

bool combat_system::can_use_super_attack(entity_id attacker, attack_type super_type) const
{
    if (!entities_ || !skills_)
        return false;
    if (super_type < attack_type::super_1 || super_type > attack_type::super_8)
        return false;

    entity* att = entities_->get_entity(attacker);
    if (!att)
        return false;

    // Get weapon skill type
    // For now, we need to know the equipped weapon
    // This would come from the inventory system
    // For simplicity, check if any weapon skill is at 100%

    // Map super type to weapon skill
    weapon_skill skill = weapon_skill::fist;
    switch (super_type)
    {
    case attack_type::super_1:
        skill = weapon_skill::sword;
        break;
    case attack_type::super_2:
        skill = weapon_skill::axe;
        break;
    case attack_type::super_3:
        skill = weapon_skill::hammer;
        break;
    case attack_type::super_4:
        skill = weapon_skill::two_hand;
        break;
    case attack_type::super_5:
        skill = weapon_skill::spear;
        break;
    case attack_type::super_6:
        skill = weapon_skill::archery;
        break;
    case attack_type::super_7:
        skill = weapon_skill::staff;
        break;
    case attack_type::super_8:
        skill = weapon_skill::fist;
        break;
    default:
        return false;
    }

    // Check if skill mastery is 100%
    uint8_t mastery = skills_->get_skill_level(static_cast<uint16_t>(skill));
    return mastery >= 100;
}

void combat_system::apply_damage(entity_id target, int32_t damage)
{
    if (!entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt || !tgt->has_stats())
        return;

    auto& stats = tgt->stats();
    stats.hp = std::max(0, stats.hp - damage);

    // Set damage animation (hurt sound plays from update_animation at frame 5)
    tgt->set_action(object_action::damage);

    spdlog::debug("Entity {} took {} damage, HP: {}/{}", target, damage, stats.hp, stats.max_hp);
}

void combat_system::heal_entity(entity_id target, int32_t amount)
{
    if (!entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt || !tgt->has_stats())
        return;

    auto& stats = tgt->stats();
    stats.hp = std::min(stats.max_hp, stats.hp + amount);

    spdlog::debug("Entity {} healed {} HP, HP: {}/{}", target, amount, stats.hp, stats.max_hp);
}

void combat_system::restore_mp(entity_id target, int32_t amount)
{
    if (!entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt || !tgt->has_stats())
        return;

    auto& stats = tgt->stats();
    stats.mp = std::min(stats.max_mp, stats.mp + amount);
}

void combat_system::restore_sp(entity_id target, int32_t amount)
{
    if (!entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt || !tgt->has_stats())
        return;

    auto& stats = tgt->stats();
    stats.sp = std::min(stats.max_sp, stats.sp + amount);
}

void combat_system::kill_entity(entity_id entity)
{
    if (!entities_)
        return;

    class entity* ent = entities_->get_entity(entity);
    if (!ent)
        return;

    // Set death state (death sound plays from update_animation at frame 7)
    ent->set_action(object_action::dying);
    if (ent->has_stats())
    {
        ent->stats().hp = 0;
    }

    // Trigger death callback
    if (callbacks_.on_death)
    {
        callbacks_.on_death(entity);
    }

    spdlog::debug("Entity {} died", entity);
}

bool combat_system::is_dead(entity_id entity) const
{
    if (!entities_)
        return true;

    class entity* ent = entities_->get_entity(entity);
    if (!ent)
        return true;

    if (!ent->has_stats())
        return false;
    return ent->stats().hp <= 0;
}

void combat_system::award_experience(entity_id entity, int32_t experience)
{
    if (!entities_)
        return;

    class entity* ent = entities_->get_entity(entity);
    if (!ent || !ent->has_stats())
        return;

    auto& stats = ent->stats();
    stats.experience += experience;

    // Trigger experience callback
    if (callbacks_.on_experience_gained)
    {
        callbacks_.on_experience_gained(entity, experience);
    }

    // Check for level up
    if (check_level_up(entity))
    {
        if (callbacks_.on_level_up)
        {
            callbacks_.on_level_up(entity);
        }
    }
}

bool combat_system::check_level_up(entity_id entity)
{
    if (!entities_)
        return false;

    class entity* ent = entities_->get_entity(entity);
    if (!ent || !ent->has_stats())
        return false;

    auto& stats = ent->stats();

    // Simple level up formula: need level * 100 exp
    int64_t exp_needed = static_cast<int64_t>(stats.level) * 100;
    if (stats.experience >= exp_needed)
    {
        stats.experience -= exp_needed;
        stats.level++;

        // Recalculate max stats
        stats.max_hp = stat_formulas::calculate_max_hp(stats.vitality, stats.level, stats.strength);
        stats.max_mp = stat_formulas::calculate_max_mp(stats.magic, stats.level, stats.intelligence);
        stats.max_sp = stat_formulas::calculate_max_sp(stats.strength, stats.level);

        // Full restore on level up
        stats.hp = stats.max_hp;
        stats.mp = stats.max_mp;
        stats.sp = stats.max_sp;

        // Play level up sound
        play_level_up_sound(entity);

        spdlog::info("Entity {} leveled up to level {}", entity, stats.level);
        return true;
    }

    return false;
}

void combat_system::add_pk_count(entity_id killer)
{
    if (!entities_)
        return;

    entity* ent = entities_->get_entity(killer);
    if (!ent || !ent->has_combat())
        return;

    ent->combat().pk_count++;
    spdlog::info("Entity {} PK count increased to {}", killer, ent->combat().pk_count);
}

void combat_system::reduce_pk_count(entity_id entity, int32_t amount)
{
    if (!entities_)
        return;

    class entity* ent = entities_->get_entity(entity);
    if (!ent || !ent->has_combat())
        return;

    ent->combat().pk_count = std::max(0, ent->combat().pk_count - amount);
}

bool combat_system::roll_hit(int32_t hit_ratio, int32_t dodge_ratio) const
{
    // Hit chance = hit_ratio - dodge_ratio, clamped to 5-95%
    int32_t hit_chance = std::clamp(hit_ratio - dodge_ratio, 5, 95);
    return !roll_percent(hit_chance); // Returns true if attack was DODGED
}

bool combat_system::roll_critical(int32_t critical_ratio) const
{
    return roll_percent(std::clamp(critical_ratio, 0, 50));
}

int32_t combat_system::calculate_base_damage(int32_t attack_power, int32_t defense) const
{
    // Base damage = attack_power - (defense / 2), with some randomness
    int32_t base = attack_power - (defense / 2);
    int32_t variance = std::max(1, attack_power / 10);
    return std::max(1, base + random_int(-variance, variance));
}

int32_t combat_system::apply_skill_bonus(int32_t damage, weapon_skill skill, uint8_t mastery) const
{
    (void)skill; // All skills apply similar bonus
    // Each 10% mastery adds 5% damage
    float bonus = 1.0f + (mastery / 20.0f);
    float result = static_cast<float>(damage) * bonus;
    // Saturate to prevent overflow
    return static_cast<int32_t>(std::min(result, static_cast<float>(INT32_MAX)));
}

int32_t combat_system::apply_critical_bonus(int32_t damage) const
{
    // Critical hits deal 150% damage
    float result = static_cast<float>(damage) * 1.5f;
    // Saturate to prevent overflow
    return static_cast<int32_t>(std::min(result, static_cast<float>(INT32_MAX)));
}

int combat_system::get_player_type(entity_id id) const
{
    if (!entities_)
        return 1;

    entity* ent = entities_->get_entity(id);
    if (!ent)
        return 1;

    // For players/characters, use sprite.gender (1=male, 2=female)
    // Convert to player_type: 1-3 = male, 4-6 = female
    if (ent->type() == entity_type::player || ent->type() == entity_type::character)
    {
        return ent->sprite().gender <= 1 ? 1 : 4; // 1 for male, 4 for female
    }

    // Default to male for other entity types
    return 1;
}

void combat_system::play_attack_sound(entity_id attacker, weapon_type wt)
{
    if (!sounds_ || !entities_)
        return;

    entity* att = entities_->get_entity(attacker);
    if (!att)
        return;

    // Get sound based on weapon type
    character_sound sound = get_weapon_swing_sound(wt);

    // Play at entity position
    const auto& t = att->transform();
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

void combat_system::play_hurt_sound(entity_id target)
{
    if (!sounds_ || !entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt)
        return;

    // Only play for players/characters, not monsters
    if (tgt->type() != entity_type::player && tgt->type() != entity_type::character)
    {
        return;
    }

    int player_type = get_player_type(target);
    character_sound sound = get_hurt_sound(player_type);

    const auto& t = tgt->transform();
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

void combat_system::play_death_sound(entity_id target)
{
    if (!sounds_ || !entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt)
        return;

    // Only play for players/characters, not monsters
    if (tgt->type() != entity_type::player && tgt->type() != entity_type::character)
    {
        return;
    }

    int player_type = get_player_type(target);
    character_sound sound = get_death_sound(player_type);

    const auto& t = tgt->transform();
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

void combat_system::play_critical_sound(entity_id attacker)
{
    if (!sounds_ || !entities_)
        return;

    entity* att = entities_->get_entity(attacker);
    if (!att)
        return;

    // Only play for players/characters
    if (att->type() != entity_type::player && att->type() != entity_type::character)
    {
        return;
    }

    int player_type = get_player_type(attacker);
    character_sound sound = get_critical_sound(player_type);

    const auto& t = att->transform();
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

void combat_system::play_level_up_sound(entity_id target)
{
    if (!sounds_ || !entities_)
        return;

    entity* tgt = entities_->get_entity(target);
    if (!tgt)
        return;

    int player_type = get_player_type(target);
    character_sound sound = get_level_up_sound(player_type);

    const auto& t = tgt->transform();
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

} // namespace hb
