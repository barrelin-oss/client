#include "gameplay/effect_definition.hpp"
#include <algorithm>

namespace hb {

// Static table of all effect definitions, derived from docs/legacy/29_effects_system.md.
// Each entry maps directly from the legacy bAddNewEffect switch statement.
static const std::array<effect_definition, 93> s_definitions = {{
    // === Basic Effects (1-18) ===

    // Type 1: Sword Slash - melee weapon trail
    {
        .type_id = effect_type_id::sword_slash,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 8,
        .max_frames = 2,
        .frame_time_ms = 10,
        .uses_tile_coords = false,
    },

    // Type 2: Arrow - flying arrow projectile
    // Legacy: speed=70, impact=dust cloud (type 14)
    {
        .type_id = effect_type_id::arrow,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 7,
        .max_frames = 0,
        .frame_time_ms = 10,
        .directional = true,
        .projectile_speed = 70,
        .impact_effect = effect_type_id::dust_cloud,
    },

    // Type 4: Gold Drop - coin drop animation
    {
        .type_id = effect_type_id::gold_drop,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 1,
        .max_frames = 12,
        .frame_time_ms = 100,
        .sound_id = 12,
    },

    // Type 5: Fire Explosion - fireball impact (composite: spawns fire bursts)
    {
        .type_id = effect_type_id::fire_explosion,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 3,
        .max_frames = 11,
        .frame_time_ms = 10,
        .sound_id = 4,
        .emits_light = true,
        .light_radius = 20,
        .children = {{
            {effect_type_id::fire_burst, 1, 5, 0, 0, true, 50},
        }},
        .child_count = 1,
    },

    // Type 6: Energy Bolt Burst - lightning burst (composite: spawns physics bursts)
    {
        .type_id = effect_type_id::energy_bolt_burst,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 14,
        .frame_time_ms = 10,
        .sound_id = 2,
        .emits_light = true,
        .light_radius = 20,
        .children = {{
            {effect_type_id::burst_physics, 1, 5, 0, 0, true, 40},
        }},
        .child_count = 1,
    },

    // Type 7: Magic Missile Explosion
    {
        .type_id = effect_type_id::magic_missile_exp,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 5,
        .frame_time_ms = 50,
        .sound_id = 3,
        .emits_light = true,
        .light_radius = 10,
        .children = {{
            {effect_type_id::burst_stationary, 1, 3, 0, 0, true, 30},
        }},
        .child_count = 1,
    },

    // Type 8: Burst Stationary - particle burst (skipped at low detail)
    {
        .type_id = effect_type_id::burst_stationary,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 4,
        .frame_time_ms = 30,
    },

    // Type 9: Burst Physics - particle with gravity (skipped at low detail)
    {
        .type_id = effect_type_id::burst_physics,
        .behavior = effect_behavior::physics,
        .render_mode = effect_render_mode::transparent,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 14,
        .frame_time_ms = 30,
        .gravity = 1,
        .uses_tile_coords = false,
    },

    // Type 10: Lightning Arrow Explosion
    {
        .type_id = effect_type_id::lightning_arrow_exp,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 14,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 20,
        .children = {{
            {effect_type_id::burst_physics, 1, 5, 0, 0, true, 40},
        }},
        .child_count = 1,
    },

    // Type 11: Blood Burst (skipped at low detail)
    {
        .type_id = effect_type_id::blood_burst,
        .behavior = effect_behavior::physics,
        .render_mode = effect_render_mode::transparent,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 8,
        .frame_time_ms = 30,
        .gravity = 1,
        .uses_tile_coords = false,
    },

    // Type 12: Fire Burst (skipped at low detail)
    {
        .type_id = effect_type_id::fire_burst,
        .behavior = effect_behavior::physics,
        .render_mode = effect_render_mode::transparent,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 10,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 8,
        .gravity = 1,
        .uses_tile_coords = false,
    },

    // Type 13: Smoke Rising
    {
        .type_id = effect_type_id::smoke_rising,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 13,
        .max_frames = 18,
        .frame_time_ms = 20,
    },

    // Type 14: Dust Cloud (skipped at low detail)
    {
        .type_id = effect_type_id::dust_cloud,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 12,
        .max_frames = 4,
        .frame_time_ms = 100,
    },

    // Type 15: Fire Trail (skipped at low detail)
    {
        .type_id = effect_type_id::fire_trail,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 16,
        .frame_time_ms = 80,
        .emits_light = true,
        .light_radius = 6,
    },

    // Type 16: Energy Strike Projectile
    // Legacy: m_pEffectSpr[0] frame 0, speed=40, trail=1x type 8, impact=type 18 + 5x type 9
    {
        .type_id = effect_type_id::energy_strike_proj,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 10,
        .directional = false,
        .projectile_speed = 40,
        .impact_effect = effect_type_id::energy_strike_impact,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 1,
        .trail_random_range = 10,
    },

    // Type 17: Ice Storm Fragment
    {
        .type_id = effect_type_id::ice_storm_fragment,
        .behavior = effect_behavior::physics,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 11,
        .max_frames = 12,
        .frame_time_ms = 20,
        .gravity = 1,
        .uses_tile_coords = false,
    },

    // Type 18: Ground Shake
    // Legacy: m_pEffectSpr[18], PutTransSprite70_NoColorKey
    {
        .type_id = effect_type_id::ground_shake,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_70,
        .sprite_pak_index = 18,
        .max_frames = 10,
        .frame_time_ms = 50,
        .shake_intensity = 3,
    },

    // Type 19: Energy Strike Impact (ground shake + burst physics)
    // Legacy type 16 on-arrival: type 18 + 5x type 9 with ±20 random offset, sound 'E' 1
    // Render: m_pEffectSpr[18], PutTransSprite70_NoColorKey
    {
        .type_id = effect_type_id::energy_strike_impact,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_70,
        .sprite_pak_index = 18,
        .max_frames = 10,
        .frame_time_ms = 50,
        .sound_id = 1,
        .shake_intensity = 3,
        .children = {{
            {effect_type_id::burst_physics, 0, 5, 0, 0, true, 20},
        }},
        .child_count = 1,
    },

    // === Projectiles (20-27) ===
    // Legacy: speed=50, trail=2x type 8

    {
        .type_id = effect_type_id::magic_projectile_20,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_21,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_22,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_23,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_24,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_25,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_26,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },
    {
        .type_id = effect_type_id::magic_projectile_27,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 10,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },

    // === Spell Effects (30-77) ===

    // Type 30: Mass Fire Strike Main
    {
        .type_id = effect_type_id::mass_fire_strike_main,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 14,
        .max_frames = 9,
        .frame_time_ms = 30,
        .shake_intensity = 3,
        .shake_multiplier = 2,
        .emits_light = true,
        .light_radius = 30,
    },

    // Type 31: Mass Fire Strike Secondary
    {
        .type_id = effect_type_id::mass_fire_strike_secondary,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 15,
        .max_frames = 8,
        .frame_time_ms = 30,
        .shake_intensity = 2,
        .emits_light = true,
        .light_radius = 20,
    },

    // Type 32: Breaking Effect
    {
        .type_id = effect_type_id::breaking_effect,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 11,
        .max_frames = 4,
        .frame_time_ms = 30,
    },

    // Type 33: Mass Magic Attack
    {
        .type_id = effect_type_id::mass_magic_attack,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 16,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 20,
    },

    // Type 34: Moving Ice Bolt (projectile)
    // Legacy: speed=50
    {
        .type_id = effect_type_id::moving_ice_bolt,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 20,
        .max_frames = 0,
        .frame_time_ms = 10,
        .shake_intensity = 2,
        .directional = true,
        .projectile_speed = 50,
    },

    // Type 40: Chill Wind
    {
        .type_id = effect_type_id::chill_wind,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 20,
        .max_frames = 15,
        .frame_time_ms = 30,
        .sound_id = 45,
    },

    // Types 41-44: Meteor Large 1-4
    {
        .type_id = effect_type_id::meteor_large_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 14,
        .frame_time_ms = 30,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
    },
    {
        .type_id = effect_type_id::meteor_large_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 32,
        .max_frames = 14,
        .frame_time_ms = 30,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
    },
    {
        .type_id = effect_type_id::meteor_large_3,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 33,
        .max_frames = 14,
        .frame_time_ms = 30,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
    },
    {
        .type_id = effect_type_id::meteor_large_4,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 34,
        .max_frames = 14,
        .frame_time_ms = 30,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
    },

    // Types 45-46: Meteor Small 1-2
    {
        .type_id = effect_type_id::meteor_small_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 35,
        .max_frames = 14,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 15,
    },
    {
        .type_id = effect_type_id::meteor_small_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 35,
        .max_frames = 14,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 15,
    },

    // Types 47-49: Blizzard Ice 1-3
    {
        .type_id = effect_type_id::blizzard_ice_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 46,
        .max_frames = 12,
        .frame_time_ms = 30,
        .sound_id = 46,
    },
    {
        .type_id = effect_type_id::blizzard_ice_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 47,
        .max_frames = 12,
        .frame_time_ms = 30,
    },
    {
        .type_id = effect_type_id::blizzard_ice_3,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 48,
        .max_frames = 12,
        .frame_time_ms = 30,
    },

    // Type 50: Meteor Impact
    {
        .type_id = effect_type_id::meteor_impact,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 12,
        .frame_time_ms = 30,
        .shake_intensity = 4,
        .emits_light = true,
        .light_radius = 30,
    },

    // Type 51: Chill Wind Aftermath
    {
        .type_id = effect_type_id::chill_wind_aftermath,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 21,
        .max_frames = 9,
        .frame_time_ms = 30,
    },

    // Type 52: Protection Ring
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::protection_ring,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 24,
        .max_frames = 15,
        .frame_time_ms = 80,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 53: Hold Twist
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::hold_twist,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 25,
        .max_frames = 15,
        .frame_time_ms = 80,
    },

    // Types 54-55: Star Twinkle
    {
        .type_id = effect_type_id::star_twinkle_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 28,
        .max_frames = 10,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 8,
    },
    {
        .type_id = effect_type_id::star_twinkle_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 29,
        .max_frames = 10,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 8,
    },

    // Type 56: Mass Chill Wind
    {
        .type_id = effect_type_id::mass_chill_wind,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 22,
        .max_frames = 14,
        .frame_time_ms = 30,
        .sound_id = 45,
    },

    // Type 57: Casting Effect
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::casting_effect,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 16,
        .frame_time_ms = 80,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 12,
    },

    // Type 60: Meteor Strike Prepare
    {
        .type_id = effect_type_id::meteor_strike_prepare,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 10,
        .frame_time_ms = 50,
        .emits_light = true,
        .light_radius = 20,
    },

    // Type 61: Meteor Strike Explosion
    {
        .type_id = effect_type_id::meteor_strike_explosion,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 16,
        .frame_time_ms = 30,
        .shake_intensity = 5,
        .shake_multiplier = 2,
        .emits_light = true,
        .light_radius = 40,
        .children = {{
            {effect_type_id::fire_burst, 1, 8, 0, 0, true, 60},
        }},
        .child_count = 1,
    },

    // Type 62: Dark Cloud
    {
        .type_id = effect_type_id::dark_cloud,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_70,
        .sprite_pak_index = 38,
        .max_frames = 6,
        .frame_time_ms = 50,
    },

    // Type 63: Lightning Strike
    {
        .type_id = effect_type_id::lightning_strike,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 16,
        .frame_time_ms = 20,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 30,
    },

    // Type 64: Resurrection Effect
    {
        .type_id = effect_type_id::resurrection_effect,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 24,
        .max_frames = 15,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 20,
    },

    // Type 65: Moving Dark Cloud
    // Legacy: speed=50
    {
        .type_id = effect_type_id::moving_dark_cloud,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::alpha_70,
        .sprite_pak_index = 38,
        .max_frames = 30,
        .frame_time_ms = 30,
        .directional = false,
        .projectile_speed = 50,
    },

    // Type 66: Earthquake
    {
        .type_id = effect_type_id::earthquake,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 35,
        .max_frames = 14,
        .frame_time_ms = 30,
        .shake_intensity = 5,
        .shake_multiplier = 2,
    },

    // Type 67: Fire Pillar
    {
        .type_id = effect_type_id::fire_pillar,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 39,
        .max_frames = 27,
        .frame_time_ms = 30,
        .sound_id = 42,
        .emits_light = true,
        .light_radius = 25,
    },

    // Type 68: Worm Bite
    {
        .type_id = effect_type_id::worm_bite,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 40,
        .max_frames = 17,
        .frame_time_ms = 30,
    },

    // Types 69-70: Surface Fire
    {
        .type_id = effect_type_id::surface_fire_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 18,
        .max_frames = 11,
        .frame_time_ms = 30,
        .sound_id = 42,
        .emits_light = true,
        .light_radius = 15,
    },
    {
        .type_id = effect_type_id::surface_fire_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 19,
        .max_frames = 11,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 71: Moving Ice Bolt 2
    // Legacy: speed=50
    {
        .type_id = effect_type_id::moving_ice_bolt_2,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 20,
        .max_frames = 0,
        .frame_time_ms = 10,
        .directional = true,
        .projectile_speed = 50,
    },

    // Type 72: Blizzard Large Impact
    {
        .type_id = effect_type_id::blizzard_large_impact,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 49,
        .max_frames = 15,
        .frame_time_ms = 30,
        .sound_id = 47,
        .shake_intensity = 2,
    },

    // Types 73-74: Light Effects
    {
        .type_id = effect_type_id::light_effect_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 50,
        .max_frames = 15,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 20,
    },
    {
        .type_id = effect_type_id::light_effect_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 51,
        .max_frames = 19,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 20,
    },

    // Types 75-77: Directional Effects
    {
        .type_id = effect_type_id::directional_effect_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 74,
        .max_frames = 16,
        .frame_time_ms = 30,
        .directional = true,
    },
    {
        .type_id = effect_type_id::directional_effect_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 75,
        .max_frames = 16,
        .frame_time_ms = 30,
        .directional = true,
    },
    {
        .type_id = effect_type_id::directional_effect_3,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 76,
        .max_frames = 16,
        .frame_time_ms = 30,
        .directional = true,
    },

    // === Magic Spell Effects (100-200+) ===

    // Type 100: Magic Missile (projectile)
    // Legacy: speed=50, frame_time=20, trail=type 8 x1, impact=type 7
    {
        .type_id = effect_type_id::spell_magic_missile,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 20,
        .sound_id = 1,
        .emits_light = true,
        .light_radius = 10,
        .directional = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::magic_missile_exp,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 1,
        .trail_random_range = 10,
    },

    // Type 101: Heal
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::spell_heal,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 80,
        .emits_light = true,
        .light_radius = 12,
    },

    // Type 102: Create Food
    // Legacy: frame_time=120
    {
        .type_id = effect_type_id::spell_create_food,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
    },

    // Type 110: Energy Bolt (projectile)
    // Legacy: speed=50, frame_time=20, trail=type 8 x2, impact=type 6
    {
        .type_id = effect_type_id::spell_energy_bolt,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 0,
        .frame_time_ms = 20,
        .sound_id = 2,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::energy_bolt_burst,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },

    // Type 111: Stamina Drain
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::spell_stamina_drain,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 80,
    },

    // Type 112: Recall 1
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::spell_recall_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 12,
        .frame_time_ms = 80,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 113: Defense Shield
    // Legacy: frame_time=120
    {
        .type_id = effect_type_id::spell_defense_shield,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 24,
        .max_frames = 12,
        .frame_time_ms = 120,
        .emits_light = true,
        .light_radius = 12,
    },

    // Type 114: Celebrating Light (composite: spawns fire effects)
    {
        .type_id = effect_type_id::spell_celebrating_light,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 30,
        .emits_light = true,
        .light_radius = 20,
        .children = {{
            {effect_type_id::fire_burst, 2, 6, 0, 0, true, 80},
        }},
        .child_count = 1,
    },

    // Type 120: Fire Ball (directional projectile)
    // Legacy: speed=50, frame_time=20, no trail, impact=type 5
    {
        .type_id = effect_type_id::spell_fire_ball,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 3,
        .max_frames = 0,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::fire_explosion,
    },

    // Type 121: Great Heal
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::spell_great_heal,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 80,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 122: Recall 2
    // Legacy: frame_time=120
    {
        .type_id = effect_type_id::spell_recall_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 124: Protection from NM (triggers protection ring)
    {
        .type_id = effect_type_id::spell_protection_nm,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 30,
        .children = {{
            {effect_type_id::protection_ring, 0, 1, 0, 0, false, 0},
        }},
        .child_count = 1,
    },

    // Type 125: Hold Person (triggers hold twist)
    {
        .type_id = effect_type_id::spell_hold_person,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 30,
        .children = {{
            {effect_type_id::hold_twist, 0, 1, 0, 0, false, 0},
        }},
        .child_count = 1,
    },

    // Type 130: Fire Strike (projectile)
    // Legacy: speed=50, frame_time=20, no trail, impact=type 5
    // Legacy spawns 4 fire explosions with offsets; we spawn 1 and let composite handle children
    {
        .type_id = effect_type_id::spell_fire_strike,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 11,
        .max_frames = 0,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::fire_explosion,
    },

    // Type 131: Summon
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::spell_summon,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 12,
        .frame_time_ms = 80,
    },

    // Type 132: Invisibility
    // Legacy: frame_time=80
    {
        .type_id = effect_type_id::spell_invisibility,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::fade,
        .sprite_pak_index = 4,
        .max_frames = 12,
        .frame_time_ms = 80,
    },

    // Type 133: Protection Magic (triggers protection ring)
    {
        .type_id = effect_type_id::spell_protection_magic,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 30,
        .children = {{
            {effect_type_id::protection_ring, 0, 1, 0, 0, false, 0},
        }},
        .child_count = 1,
    },

    // Type 135: Paralyze (triggers hold twist)
    {
        .type_id = effect_type_id::spell_paralyze,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 14,
        .frame_time_ms = 30,
        .children = {{
            {effect_type_id::hold_twist, 0, 1, 0, 0, false, 0},
        }},
        .child_count = 1,
    },

    // Type 136: Cure
    // Legacy: frame_time=120
    {
        .type_id = effect_type_id::spell_cure,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .emits_light = true,
        .light_radius = 10,
    },

    // Type 137: Lightning Arrow (projectile)
    // Legacy: speed=50, frame_time=20, trail=type 8 x3, impact=type 10
    {
        .type_id = effect_type_id::spell_lightning_arrow,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 0,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::lightning_arrow_exp,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 3,
        .trail_random_range = 10,
    },

    // Type 138: Tremor (composite: spawns many dust clouds + camera shake)
    {
        .type_id = effect_type_id::spell_tremor,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 35,
        .max_frames = 14,
        .frame_time_ms = 30,
        .shake_intensity = 5,
        .shake_multiplier = 3,
        .children = {{
            {effect_type_id::dust_cloud, 1, 14, 0, 0, true, 100},
        }},
        .child_count = 1,
    },

    // Type 150: Berserk
    // Legacy: frame_time=100
    {
        .type_id = effect_type_id::spell_berserk,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 11,
        .frame_time_ms = 100,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 160: Energy Strike (invisible spawner - fires projectiles from caster to target)
    // Legacy: max_frames=7, frame_time=80, spawns type 16 each frame from src to dest±50
    {
        .type_id = effect_type_id::spell_energy_strike,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // No visible sprite
        .max_frames = 7,
        .frame_time_ms = 80,
        .sound_id = 1,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::energy_strike_proj, -1, 1, 0, 0, true, 50, true},
        }},
        .child_count = 1,
    },

    // Type 180: Illusion
    // Legacy: frame_time=100
    {
        .type_id = effect_type_id::spell_illusion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::fade,
        .sprite_pak_index = 60,
        .max_frames = 11,
        .frame_time_ms = 100,
    },

    // Type 181: Special Meteor (lightning variant)
    {
        .type_id = effect_type_id::spell_special_meteor,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 16,
        .frame_time_ms = 20,
        .shake_intensity = 4,
        .emits_light = true,
        .light_radius = 30,
    },

    // Type 190: Mass Illusion
    // Legacy: frame_time=100
    {
        .type_id = effect_type_id::spell_mass_illusion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::fade,
        .sprite_pak_index = 61,
        .max_frames = 11,
        .frame_time_ms = 100,
    },
}};

const effect_definition* get_effect_definition(effect_type_id type)
{
    auto it = std::find_if(s_definitions.begin(), s_definitions.end(),
        [type](const effect_definition& def) { return def.type_id == type; });

    if (it != s_definitions.end())
    {
        return &(*it);
    }
    return nullptr;
}

std::span<const effect_definition> all_effect_definitions()
{
    return s_definitions;
}

} // namespace hb
