#pragma once

#include <cstdint>

namespace hb {

// Effect type IDs matching legacy m_sType values
enum class effect_type_id : uint16_t
{
    none = 0,

    // Basic effects (1-18)
    sword_slash = 1,
    arrow = 2,
    gold_drop = 4,
    fire_explosion = 5,
    energy_bolt_burst = 6,
    magic_missile_exp = 7,
    burst_stationary = 8,
    burst_physics = 9,
    lightning_arrow_exp = 10,
    blood_burst = 11,
    fire_burst = 12,
    smoke_rising = 13,
    dust_cloud = 14,
    fire_trail = 15,
    energy_strike_proj = 16,
    ice_storm_fragment = 17,
    ground_shake = 18,
    energy_strike_impact = 19,  // Composite: ground shake + burst physics on proj arrival

    // Projectiles (20-27)
    magic_projectile_20 = 20,
    magic_projectile_21 = 21,
    magic_projectile_22 = 22,
    magic_projectile_23 = 23,
    magic_projectile_24 = 24,
    magic_projectile_25 = 25,
    magic_projectile_26 = 26,
    magic_projectile_27 = 27,

    // Spell effects (30-77)
    mass_fire_strike_main = 30,
    mass_fire_strike_secondary = 31,
    breaking_effect = 32,
    mass_magic_attack = 33,
    moving_ice_bolt = 34,
    chill_wind = 40,
    meteor_large_1 = 41,
    meteor_large_2 = 42,
    meteor_large_3 = 43,
    meteor_large_4 = 44,
    meteor_small_1 = 45,
    meteor_small_2 = 46,
    blizzard_ice_1 = 47,
    blizzard_ice_2 = 48,
    blizzard_ice_3 = 49,
    meteor_impact = 50,
    chill_wind_aftermath = 51,
    protection_ring = 52,
    hold_twist = 53,
    star_twinkle_1 = 54,
    star_twinkle_2 = 55,
    mass_chill_wind = 56,
    casting_effect = 57,
    meteor_strike_prepare = 60,
    meteor_strike_explosion = 61,
    dark_cloud = 62,
    lightning_strike = 63,
    resurrection_effect = 64,
    moving_dark_cloud = 65,
    earthquake = 66,
    fire_pillar = 67,
    worm_bite = 68,
    surface_fire_1 = 69,
    surface_fire_2 = 70,
    moving_ice_bolt_2 = 71,
    blizzard_large_impact = 72,
    light_effect_1 = 73,
    light_effect_2 = 74,
    directional_effect_1 = 75,
    directional_effect_2 = 76,
    directional_effect_3 = 77,

    // Magic spell effects (100-200+)
    spell_magic_missile = 100,
    spell_heal = 101,
    spell_create_food = 102,
    spell_energy_bolt = 110,
    spell_stamina_drain = 111,
    spell_recall_1 = 112,
    spell_defense_shield = 113,
    spell_celebrating_light = 114,
    spell_fire_ball = 120,
    spell_great_heal = 121,
    spell_recall_2 = 122,
    spell_stamina_recovery_1 = 123,
    spell_protection_nm = 124,
    spell_hold_person = 125,
    spell_fire_strike = 130,
    spell_summon = 131,
    spell_invisibility = 132,
    spell_protection_magic = 133,
    spell_detect_invis = 134,
    spell_paralyze = 135,
    spell_cure = 136,
    spell_lightning_arrow = 137,
    spell_tremor = 138,
    spell_confuse_lang = 142,
    spell_great_def_shield = 144,
    spell_energy_strike = 160,
    spell_berserk = 150,
    spell_mass_poison_1 = 152,
    spell_mass_poison_2 = 153,
    spell_confusion = 162,
    spell_special_165 = 165,
    spell_mass_confusion = 171,
    spell_illusion = 180,
    spell_special_meteor = 181,
    spell_mass_illusion = 190,
};

// Effect behavior categories
enum class effect_behavior : uint8_t
{
    static_anim,    // Simple frame animation at a position
    projectile,     // Moves from source to destination via Bresenham
    physics,        // Velocity + gravity (particles)
    composite,      // Static anim that spawns child effects at specific frames
};

// Render mode for the effect sprite
enum class effect_render_mode : uint8_t
{
    normal,         // Standard opaque render
    transparent,    // Color-key transparency
    alpha_25,       // 25% alpha
    alpha_50,       // 50% alpha
    alpha_70,       // 70% alpha
    fade,           // Fade out over lifetime
};

// Detail level filter
enum class effect_detail : uint8_t
{
    all,            // Render at all detail levels
    high_only,      // Skip at low detail (particles)
};

} // namespace hb
