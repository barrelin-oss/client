#include "gameplay/effect_definition.hpp"
#include <algorithm>

namespace hb {

// Static table of all effect definitions, derived from docs/legacy/29_effects_system.md.
// Each entry maps directly from the legacy bAddNewEffect switch statement.
static const std::array<effect_definition, 150> s_definitions = {{
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
    // Legacy: sprite [6] (effect.pak #6), 5 frames, 50ms, sound E3
    // Legacy: at frame 1 spawns 3x BURST_MEDIUM (type 9) at ±5 random offset
    // Legacy render: alpha 50% for frames 0-3, fade-to-dark for frames 4-5
    {
        .type_id = effect_type_id::magic_missile_exp,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 6,
        .max_frames = 5,
        .frame_time_ms = 50,
        .sound_id = 3,
        .emits_light = true,
        .light_radius = 10,
        .children = {{
            {effect_type_id::burst_physics, 1, 3, 0, 0, true, 5},
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
    // Legacy: sprite 6, maxFrame=14, frameTime=10, sound E2, camera shake
    // Frame 1: spawns 11x type 9 (burst_physics) at ±20px random
    {
        .type_id = effect_type_id::lightning_arrow_exp,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 6,
        .max_frames = 14,
        .frame_time_ms = 10,
        .sound_id = 2,
        .shake_intensity = 2,
        .emits_light = true,
        .light_radius = 20,
        .children = {{
            {effect_type_id::burst_physics, 1, 11, 0, 0, true, 20},
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
    // Legacy DrawEffects: m_pEffectSpr[11]->PutTransSprite50_NoColorKey(dX, dY, 28 + cFrame)
    {
        .type_id = effect_type_id::dust_cloud,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 4,
        .frame_time_ms = 100,
        .frame_offset = 28,
    },

    // Type 15: Fire Trail / Red Cloud Particles (skipped at low detail)
    // Legacy: sprite [11], frame 33+currentFrame, maxFrame=16, 80ms, alpha 50%
    {
        .type_id = effect_type_id::fire_trail,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .detail_level = effect_detail::high_only,
        .sprite_pak_index = 11,
        .max_frames = 16,
        .frame_time_ms = 80,
        .emits_light = true,
        .light_radius = 6,
        .uses_tile_coords = false,
        .frame_offset = 33,
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
    // Two visual layers:
    //   1. Tornado vortex: sprite 13 (effect3 #0), 10 frames
    //   2. Ice projectiles orbiting: sprite 11 (effect2 #1), frames 39-47 (swirl behavior)
    // Legacy update: complex steering/orbit around origin that drifts upward
    // Ground glow: sprite 40, frame 11, 25% alpha
    // TODO: implement swirling ice fragment orbit behavior + dual-layer rendering
    {
        .type_id = effect_type_id::ice_storm_fragment,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 13,
        .max_frames = 9,
        .frame_time_ms = 80,
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
        .projectile_speed = 50,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },

    // === Spell Effects (30-77) ===

    // Type 30: Mass Fire Strike Main (primary impact explosion)
    // Legacy: sprite [14] (effect3.pak #1), maxFrame=9 (10 frames), 40ms, alpha 50%
    // Legacy: sound E4, camera shake sDist*2
    // Legacy: frame 1 spawns 5x BURST_LARGE, frame 7 spawns 3x RED_CLOUD_PARTICLES
    // Legacy: projectile impact also spawns 3x type 31 at pixel offsets with staggered starts
    {
        .type_id = effect_type_id::mass_fire_strike_main,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 14,
        .max_frames = 9,
        .frame_time_ms = 40,
        .sound_id = 4,
        .shake_intensity = 3,
        .shake_multiplier = 2,
        .emits_light = true,
        .light_radius = 30,
        .uses_tile_coords = false,
        .children = {{
            {effect_type_id::mass_fire_strike_secondary, 0, 1, -30, -15, false, 0, false, -7, 0},
            {effect_type_id::mass_fire_strike_secondary, 0, 1, 35, -30, false, 0, false, -5, 0},
            {effect_type_id::mass_fire_strike_secondary, 0, 1, 20, 30, false, 0, false, -3, 0},
            {effect_type_id::fire_burst, 1, 5, 0, 0, true, 5},
            {effect_type_id::fire_trail, 7, 3, 0, 0, true, 5},
        }},
        .child_count = 5,
    },

    // Type 31: Mass Fire Strike Secondary (3 offset explosions spawned by type 30)
    // Legacy: sprite [15] (effect3.pak #2), maxFrame=8 (9 frames), 40ms, alpha 50%
    // Legacy: sound E4, camera shake sDist
    // Legacy: frame 1 spawns 5x BURST_LARGE, frame 7 spawns 3x RED_CLOUD_PARTICLES
    {
        .type_id = effect_type_id::mass_fire_strike_secondary,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 15,
        .max_frames = 8,
        .frame_time_ms = 40,
        .sound_id = 4,
        .shake_intensity = 2,
        .emits_light = true,
        .light_radius = 20,
        .uses_tile_coords = false,
        .children = {{
            {effect_type_id::fire_burst, 1, 5, 0, 0, true, 5},
            {effect_type_id::fire_trail, 7, 3, 0, 0, true, 5},
        }},
        .child_count = 2,
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

    // Type 33: Blood Particle (trail from bloody shock wave projectiles)
    // Legacy bAddNewEffect: m_mX=sX, m_mY=sY, maxFrame=16, frameTime=10
    // Legacy DrawEffects: m_pEffectSpr[19]->PutTransSpriteRGB (effect4 sprite #0)
    {
        .type_id = effect_type_id::blood_particle,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 19,
        .max_frames = 16,
        .frame_time_ms = 10,
        .uses_tile_coords = false,
        .x_offset = 0,
        .height_offset = 0,
    },

    // Type 34: Blood Projectile (invisible, trails blood particles)
    // Legacy bAddNewEffect: m_mX=sX*32, m_mY=sY*32-40, speed=50, maxFrame=0
    // Legacy bEffectFrameCounter: GetPoint + spawn type 33 trail, on arrival spawn type 33
    // Legacy DrawEffects: case 34 = break (invisible)
    {
        .type_id = effect_type_id::blood_projectile,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,
        .max_frames = 0,
        .frame_time_ms = 20,
        .shake_intensity = 2,
        .uses_tile_coords = false,
        .x_offset = 0,
        .height_offset = 0,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::blood_particle,
        .trail_effect = effect_type_id::blood_particle,
        .trail_count = 1,
        .trail_random_range = 15,
    },

    // Type 35: Mass Magic Missile Aura1 (primary impact explosion, spawns 3x AURA2 children)
    // Legacy: sprite [6] (effect.pak #6), 18 frames, 40ms, sound E4, shake 2x
    // Legacy position: dest + (22, -15), render offset (-30, -18) -> combined offset from dest center
    // Legacy render: DrawParams{0.5f, ...} = alpha 50%
    {
        .type_id = effect_type_id::mass_magic_missile_aura1,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 6,
        .max_frames = 18,
        .frame_time_ms = 40,
        .sound_id = 4,
        .shake_intensity = 2,
        .uses_tile_coords = false,
        .x_offset = -24,
        .height_offset = -49,
        .children = {{
            // 3x type 36 at different offsets from parent, staggered trigger frames
            // Legacy offsets from dest tile origin: (-22,-7), (30,-22), (12,22)
            // Adjusted to parent position: parent at dest + (-24, -49)
            {effect_type_id::mass_magic_missile_explosion, 0, 1, -14, 26, false, 0, false, 0, 0},
            {effect_type_id::mass_magic_missile_explosion, 2, 1, 38, 11, false, 0, false, 0, 0},
            {effect_type_id::mass_magic_missile_explosion, 4, 1, 20, 55, false, 0, false, 0, 0},
        }},
        .child_count = 3,
    },

    // Type 36: Mass Magic Missile Aura2 (secondary impact explosions)
    // Legacy bAddNewEffect: m_mX=sX, m_mY=sY, maxFrame=15, frameTime=40, sound E4, camera shake
    // Legacy DrawEffects: m_pEffectSpr[97]->DrawParams{0.5f, ...} (effect11 #8, alpha 50%)
    {
        .type_id = effect_type_id::mass_magic_missile_explosion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 97,
        .max_frames = 15,
        .frame_time_ms = 40,
        .sound_id = 4,
        .shake_intensity = 1,
        .uses_tile_coords = false,
        .x_offset = 0,
        .height_offset = 0,
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
    // Legacy: sprite 21, start 220px above, 7 frames stationary then fall with gravity
    // On death: spawn impact (type 50) + 3x dust (type 14) + 2x chill aftermath (type 51)
    // Sound E46, maxFrame=14, frameTime=20, fall_velocity starts at 20, gravity=1
    {
        .type_id = effect_type_id::meteor_large_1,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 21,
        .max_frames = 14,
        .frame_time_ms = 20,
        .sound_id = 46,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
        .gravity = 1,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .frame_offset = 0,
        .children = {{
            {effect_type_id::meteor_impact, 14, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 14, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 14, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },
    {
        .type_id = effect_type_id::meteor_large_2,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 21,
        .max_frames = 14,
        .frame_time_ms = 20,
        .sound_id = 46,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
        .gravity = 1,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .frame_offset = 8,
        .children = {{
            {effect_type_id::meteor_impact, 14, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 14, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 14, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },
    {
        .type_id = effect_type_id::meteor_large_3,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 21,
        .max_frames = 14,
        .frame_time_ms = 20,
        .sound_id = 46,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
        .gravity = 1,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .frame_offset = 16,
        .children = {{
            {effect_type_id::meteor_impact, 14, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 14, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 14, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },
    {
        .type_id = effect_type_id::meteor_large_4,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 21,
        .max_frames = 14,
        .frame_time_ms = 20,
        .sound_id = 46,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 25,
        .gravity = 1,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .frame_offset = 24,
        .children = {{
            {effect_type_id::meteor_impact, 14, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 14, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 14, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },

    // Types 45-46: Meteor Small 1-2
    // Legacy: same falling behavior but no impact effects on death
    {
        .type_id = effect_type_id::meteor_small_1,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 21,
        .max_frames = 14,
        .frame_time_ms = 20,
        .sound_id = 46,
        .emits_light = true,
        .light_radius = 15,
        .gravity = 1,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .frame_offset = 32,
    },
    {
        .type_id = effect_type_id::meteor_small_2,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 21,
        .max_frames = 14,
        .frame_time_ms = 20,
        .sound_id = 46,
        .emits_light = true,
        .light_radius = 15,
        .gravity = 1,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .frame_offset = 40,
    },

    // Types 47-49: Blizzard Ice 1-3
    // Legacy: spawned 220px above target, animate 7 frames, then fall with gravity
    // On death: type 49 spawns large impact (72), types 47/48 spawn small impact (50)
    // Plus dust clouds and chill wind aftermath
    {
        .type_id = effect_type_id::blizzard_ice_1,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 46,
        .max_frames = 12,
        .frame_time_ms = 20,
        .sound_id = 46,
        .gravity = 4,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .children = {{
            {effect_type_id::meteor_impact, 12, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 12, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 12, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },
    {
        .type_id = effect_type_id::blizzard_ice_2,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 47,
        .max_frames = 12,
        .frame_time_ms = 20,
        .gravity = 4,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .children = {{
            {effect_type_id::meteor_impact, 12, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 12, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 12, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },
    {
        .type_id = effect_type_id::blizzard_ice_3,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 48,
        .max_frames = 12,
        .frame_time_ms = 20,
        .gravity = 4,
        .fall_start_frame = 7,
        .fall_initial_speed = 20,
        .fall_initial_speed_x = -1,
        .uses_tile_coords = false,
        .height_offset = -220,
        .children = {{
            {effect_type_id::blizzard_large_impact, 12, 1, 0, 0, false, 0},
            {effect_type_id::dust_cloud, 12, 3, 0, 0, true, 10},
            {effect_type_id::chill_wind_aftermath, 12, 2, 0, 0, true, 10},
        }},
        .child_count = 3,
    },

    // Type 50: Smoke/Dust Burst
    // Legacy: m_pEffectSpr[22], frame_time=50, PutTransSpriteRGB with darken after frame 6
    // Sound E47 + 25% shake in legacy, but sound omitted here (too many spawns in blizzard)
    {
        .type_id = effect_type_id::meteor_impact,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 22,
        .max_frames = 12,
        .frame_time_ms = 50,
    },

    // Type 51: Chill Wind Aftermath
    // Legacy DrawEffects: m_pEffectSpr[28]->PutTransSprite25(dX, dY, cTempFrame + 11)
    {
        .type_id = effect_type_id::chill_wind_aftermath,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_25,
        .sprite_pak_index = 28,
        .max_frames = 9,
        .frame_time_ms = 80,
        .frame_offset = 11,
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
    // Legacy DrawEffects: m_pEffectSpr[29]->PutTransSprite50_NoColorKey
    {
        .type_id = effect_type_id::mass_chill_wind,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 29,
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

    // Type 60: Meteor Strike (descending fireball)
    // Legacy: starts at sX+300, sY-460, moves -30x/+46y per frame for 10 frames
    // Dual-layer: sprite 31 frame 15+f (opaque) + frame f (transparent overlay)
    // Each frame spawns type 62 trail; on death spawns type 61 + 63 + 5x 12
    {
        .type_id = effect_type_id::meteor_strike_prepare,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 10,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 20,
        .fall_start_frame = 0,
        .fall_initial_speed = 46,
        .fall_initial_speed_x = -30,
        .uses_tile_coords = false,
        .x_offset = 300,
        .height_offset = -460,
        .overlays = {{
            {31, 0, 0, effect_render_mode::normal, 15},
        }},
        .overlay_count = 1,
        .children = {{
            {effect_type_id::dark_cloud, -1, 1, 0, 0, false, 0},
            {effect_type_id::meteor_strike_explosion, 10, 1, 0, 0, false, 0},
            {effect_type_id::lightning_strike, 10, 1, 0, 0, false, 0},
            {effect_type_id::fire_burst, 10, 5, 0, 0, true, 10},
        }},
        .child_count = 4,
    },

    // Type 61: Meteor Strike Fire Aura (ground impact)
    // Legacy: m_pEffectSpr[32] (CruEffect1 sprite 1), maxFrame=16, frameTime=10
    // Sound E4, camera shake distance-based
    {
        .type_id = effect_type_id::meteor_strike_explosion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 32,
        .max_frames = 16,
        .frame_time_ms = 10,
        .sound_id = 4,
        .shake_intensity = 3,
        .shake_multiplier = 2,
        .emits_light = true,
        .light_radius = 40,
    },

    // Type 62: Dark Cloud (meteor strike trail)
    // Legacy bAddNewEffect: maxFrame=6, frameTime=100
    // Legacy DrawEffects: m_pEffectSpr[31]->PutRevTransSprite(dX, dY, 20+cTempFrame, cTempFrame/3)
    // Legacy bEffectFrameCounter: position jiggles ±1 per frame
    {
        .type_id = effect_type_id::dark_cloud,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 6,
        .frame_time_ms = 100,
        .uses_tile_coords = false,
        .frame_offset = 20,
    },

    // Type 63: Lightning Strike
    // Legacy DrawEffects: m_pEffectSpr[33]->PutTransSprite_NoColorKey
    {
        .type_id = effect_type_id::lightning_strike,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 33,
        .max_frames = 16,
        .frame_time_ms = 20,
        .shake_intensity = 3,
        .emits_light = true,
        .light_radius = 30,
    },

    // Type 64: Resurrection Effect
    // Legacy: sprite [99] (effect11.pak #10), 31 frames (maxFrame=30), 40ms
    // Legacy render: Alpha(0.5f) — 50% alpha blend
    {
        .type_id = effect_type_id::resurrection_effect,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 99,
        .max_frames = 31,
        .frame_time_ms = 40,
        .emits_light = true,
        .light_radius = 20,
    },

    // Type 65: Moving Dark Cloud
    // Legacy DrawEffects: m_pEffectSpr[31]->PutRevTransSprite(dX, dY, 20+cTempFrame/6)
    // Legacy bAddNewEffect: maxFrame=30, frameTime=80, speed=50
    {
        .type_id = effect_type_id::moving_dark_cloud,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 31,
        .max_frames = 30,
        .frame_time_ms = 80,
        .directional = false,
        .projectile_speed = 50,
    },

    // Type 66: Earthquake
    // Legacy DrawEffects: m_pEffectSpr[39]->PutRevTransSprite + PutTransSprite_NoColorKey
    {
        .type_id = effect_type_id::earthquake,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 39,
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
    // Legacy DrawEffects: m_pEffectSpr[40] (opaque) + m_pEffectSpr[41] (50% alpha overlay)
    //   + m_pEffectSpr[44] x2 (debris), shake at frame 11
    {
        .type_id = effect_type_id::worm_bite,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::normal,
        .sprite_pak_index = 40,
        .max_frames = 17,
        .frame_time_ms = 30,
        .shake_intensity = 2,
        .uses_tile_coords = false,
        .no_additive = true,
        .overlays = {{
            {41, 0, 0, effect_render_mode::alpha_50},
            {44, -2, -3, effect_render_mode::transparent},
            {44, -4, -3, effect_render_mode::transparent},
        }},
        .overlay_count = 3,
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

    // Type 71: Moving Ice Bolt 2 (Blizzard invisible projectile)
    // Legacy: speed=50, invisible (no rendering), spawns type 48 + type 51 trail, type 49 on impact
    {
        .type_id = effect_type_id::moving_ice_bolt_2,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,
        .max_frames = 0,
        .frame_time_ms = 20,
        .directional = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::blizzard_ice_3,
        .trail_effect = effect_type_id::blizzard_ice_2,
        .trail_count = 1,
        .trail_random_range = 15,
    },

    // Type 72: Blizzard Large Impact
    // Legacy: m_pEffectSpr[51], frame_time=20, sound E47, shake
    {
        .type_id = effect_type_id::blizzard_large_impact,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 51,
        .max_frames = 15,
        .frame_time_ms = 20,
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
    // Legacy renders sprite [0] at alpha 50%, frame 0 always
    {
        .type_id = effect_type_id::spell_magic_missile,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 0,
        .max_frames = 0,
        .frame_time_ms = 20,
        .sound_id = 1,
        .emits_light = true,
        .light_radius = 10,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::magic_missile_exp,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 1,
        .trail_random_range = 10,
    },

    // Type 101: Heal
    // Legacy DrawEffects: m_pEffectSpr[50]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=14, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_heal,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 50,
        .max_frames = 14,
        .frame_time_ms = 80,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 12,
    },

    // Type 102: Create Food
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_create_food,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 110: Energy Bolt (projectile)
    // Legacy: speed=50, frame_time=20, trail=type 8 x2, impact=type 6
    // Legacy renders m_cFrame (=0), not direction
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
        .projectile_speed = 50,
        .impact_effect = effect_type_id::energy_bolt_burst,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 2,
        .trail_random_range = 10,
    },

    // Type 111: Stamina Drain
    // Legacy DrawEffects: m_pEffectSpr[49]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=14, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_stamina_drain,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 49,
        .max_frames = 14,
        .frame_time_ms = 80,
        .sound_id = 5,
    },

    // Type 112: Recall 1
    // Legacy DrawEffects: m_pEffectSpr[52]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=12, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_recall_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 52,
        .max_frames = 12,
        .frame_time_ms = 80,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 113: Defense Shield
    // Legacy DrawEffects: m_pEffectSpr[62]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=12, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_defense_shield,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 62,
        .max_frames = 12,
        .frame_time_ms = 120,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 12,
    },

    // Type 114: Celebrating Light (instant spawner: creates surface fires)
    // Legacy bAddNewEffect: spawns 5x type69/70 (surface_fire_1/2) at random offsets, deletes self
    {
        .type_id = effect_type_id::spell_celebrating_light,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::surface_fire_1, 0, 3, 0, 0, true, 20},
            {effect_type_id::surface_fire_2, 0, 2, 0, 0, true, 20},
        }},
        .child_count = 2,
    },

    // Type 120: Fire Ball (projectile)
    // Legacy: speed=50, frame_time=20, m_pEffectSpr[5] with (dir-1)*4+rand()%4, impact=type 5
    {
        .type_id = effect_type_id::spell_fire_ball,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 5,
        .max_frames = 0,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .frames_per_direction = 4,
        .randomize_direction_frame = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::fire_explosion,
    },

    // Type 121: Great Heal
    // Legacy DrawEffects: m_pEffectSpr[50]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=14, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_great_heal,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 50,
        .max_frames = 14,
        .frame_time_ms = 80,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 122: Recall 2
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_recall_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 15,
    },

    // Type 123: Stamina Recovery
    // Legacy DrawEffects: m_pEffectSpr[56]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=14, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_stamina_recovery_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 56,
        .max_frames = 14,
        .frame_time_ms = 80,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 12,
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

    // Type 126: Possession
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_possession,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 127: Poison
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_poison,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 128: Great Stamina Recovery
    // Legacy DrawEffects: m_pEffectSpr[56]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=14, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_great_stamina_recovery,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 56,
        .max_frames = 14,
        .frame_time_ms = 80,
        .sound_id = 5,
    },

    // Type 130: Fire Strike (projectile)
    // Legacy: speed=50, frame_time=20, m_pEffectSpr[5] with (dir-1)*4+rand()%4, impact=type 5
    {
        .type_id = effect_type_id::spell_fire_strike,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 5,
        .max_frames = 0,
        .frame_time_ms = 20,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .frames_per_direction = 4,
        .randomize_direction_frame = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::fire_explosion,
    },

    // Type 131: Summon
    // Legacy DrawEffects: m_pEffectSpr[52]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=12, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_summon,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 52,
        .max_frames = 12,
        .frame_time_ms = 80,
        .sound_id = 5,
    },

    // Type 132: Invisibility
    // Legacy DrawEffects: m_pEffectSpr[52]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=12, frameTime=80, sound='E' 5
    {
        .type_id = effect_type_id::spell_invisibility,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 52,
        .max_frames = 12,
        .frame_time_ms = 80,
        .sound_id = 5,
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

    // Type 134: Detect Invisibility
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_detect_invis,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 15,
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
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_cure,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 10,
    },

    // Type 137: Lightning Arrow (projectile)
    // Legacy: sprite 10 (effect2 #0), directional 8dirs x 4 random frames
    // Renders 6 copies with fading trail (25%, 25%, 50%, 50%, 70%, 100%)
    // speed=50, frame_time=20, trail=type 8 x3 per step, impact=type 10, sound E1
    {
        .type_id = effect_type_id::spell_lightning_arrow,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::arrow_trail,
        .sprite_pak_index = 10,
        .max_frames = 0,
        .frame_time_ms = 20,
        .sound_id = 1,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .frames_per_direction = 4,
        .randomize_direction_frame = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::lightning_arrow_exp,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 3,
        .trail_random_range = 10,
    },

    // Type 138: Tremor (instant spawner: creates dust clouds + camera shake)
    // Legacy bAddNewEffect: spawns 10x type 14 (dust_cloud) at random offsets, sound 'E' 4, camera shake
    {
        .type_id = effect_type_id::spell_tremor,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .sound_id = 4,
        .shake_intensity = 5,
        .shake_multiplier = 2,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::dust_cloud, 0, 10, 0, 0, true, 60},
        }},
        .child_count = 1,
    },

    // Type 142: Confuse Language
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_confuse_lang,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 143: Lightning (Thunder bolt from sky to target)
    // Legacy DrawEffects: _DrawThunderEffect from dY*32-800 to dY*32 (procedural lightning)
    // Legacy bAddNewEffect: maxFrame=7, frameTime=10, sound='E' 40
    // Legacy bEffectFrameCounter: on expire spawns type 10 (lightning_arrow_exp), randomizes rX/rY each frame
    {
        .type_id = effect_type_id::spell_lightning_thunder,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::thunder,
        .sprite_pak_index = 255,    // No sprite - procedural rendering
        .max_frames = 7,
        .frame_time_ms = 10,
        .sound_id = 40,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::lightning_arrow_exp, 6, 1, 0, 0, false, 0},
        }},
        .child_count = 1,
    },

    // Type 144: Great Defense Shield
    // Legacy DrawEffects: m_pEffectSpr[63]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=12, frameTime=120, sound='E' 5
    // NOTE: Legacy does NOT spawn protection_ring - standalone anim only
    {
        .type_id = effect_type_id::spell_great_def_shield,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 63,
        .max_frames = 12,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 145: Chill Wind Spawner (spawns 4x chill_wind at offsets, then dies)
    // Legacy bAddNewEffect: maxFrame=2, frameTime=10
    // Legacy bEffectFrameCounter: immediately spawns 4x type 40 at dX*32 offsets, deletes self
    {
        .type_id = effect_type_id::spell_chill_wind_spawner,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::chill_wind, 0, 1, 0, 0, false, 0, false, 0},
            {effect_type_id::chill_wind, 0, 1, -30, -15, false, 0, false, -10},
            {effect_type_id::chill_wind, 0, 1, 35, -30, false, 0, false, -6},
            {effect_type_id::chill_wind, 0, 1, 20, 30, false, 0, false, -3},
        }},
        .child_count = 4,
    },

    // Type 147: Triple Energy Bolt Spawner (spawns 3 energy bolt projectiles + impact)
    // Legacy bAddNewEffect: maxFrame=0, frameTime=20
    // Legacy bEffectFrameCounter: spawns 3x type 110 from sX,sY to dX±1,dY±1 + burst + impact
    {
        .type_id = effect_type_id::spell_triple_energy_bolt_spawner,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 1,
        .frame_time_ms = 20,
        .sound_id = 1,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::spell_energy_bolt, 0, 1, -32, -32, false, 0, true},
            {effect_type_id::spell_energy_bolt, 0, 1, 32, -32, false, 0, true},
            {effect_type_id::spell_energy_bolt, 0, 1, 32, 32, false, 0, true},
            {effect_type_id::magic_missile_exp, 0, 1, 0, 0, false, 0},
        }},
        .child_count = 4,
    },

    // Type 150: Berserk
    // Legacy bAddNewEffect: maxFrame=11, frameTime=100, sound='E' 5
    // Legacy DrawEffectLights: m_pEffectSpr[58]->PutTransSprite_NoColorKey
    {
        .type_id = effect_type_id::spell_berserk,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 58,
        .max_frames = 11,
        .frame_time_ms = 100,
        .sound_id = 5,
        .emits_light = true,
        .light_radius = 15,
        .x_offset = 0,
        .height_offset = 0,
    },

    // Type 151: Lightning Bolt (Thunder bolt from caster to target)
    // Legacy DrawEffects: _DrawThunderEffect from m_mX,m_mY to dX*32,dY*32
    // Legacy bAddNewEffect: maxFrame=10, frameTime=10, sound='E' 40
    // Legacy bEffectFrameCounter: on expire spawns type 10, randomizes rX/rY each frame
    {
        .type_id = effect_type_id::spell_lightning_bolt_thunder,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::thunder,
        .sprite_pak_index = 255,    // No sprite - procedural rendering
        .max_frames = 10,
        .frame_time_ms = 10,
        .sound_id = 40,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::lightning_arrow_exp, 9, 1, 0, 0, false, 0},
        }},
        .child_count = 1,
    },

    // Type 152: Mass Poison
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_mass_poison_1,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 153: Mass Poison 2
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_mass_poison_2,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 156: Mass Lightning Arrow Timer (spawns lightning arrows each frame)
    // Legacy bAddNewEffect: maxFrame=3, frameTime=130, no sound on timer itself
    // Legacy bEffectFrameCounter: each frame spawns type 137 (which plays E1) from sX,sY to dX,dY
    {
        .type_id = effect_type_id::spell_mass_lightning_arrow_timer,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible timer
        .max_frames = 3,
        .frame_time_ms = 130,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::spell_lightning_arrow, -1, 1, 0, 0, false, 0, true},
        }},
        .child_count = 1,
    },

    // Type 157: Ice Strike Spawner (spawns meteors + small ice fragments)
    // Legacy: maxFrame=2, frameTime=10 (instant spawner)
    // Spawns 1 centered large meteor + 14 random large (types 41-43) staggered -1 to -14
    //       + 6 random small (types 45-46) staggered -11 to -16
    {
        .type_id = effect_type_id::spell_ice_strike_spawner,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::meteor_large_1, 0, 1, 0, 0, false, 0, false, 0, 0},
            {effect_type_id::meteor_large_1, 0, 5, 10, 0, true, 50, false, -1, -1},
            {effect_type_id::meteor_large_2, 0, 5, 10, 0, true, 50, false, -6, -1},
            {effect_type_id::meteor_large_3, 0, 4, 10, 0, true, 50, false, -11, -1},
            {effect_type_id::meteor_small_1, 0, 3, 10, 0, true, 50, false, -11, -1},
            {effect_type_id::meteor_small_2, 0, 3, 10, 0, true, 50, false, -14, -1},
        }},
        .child_count = 6,
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

    // Type 161: Mass Fire Strike (projectile)
    // Legacy: sprite [5], (dir-1)*4+rand()%4, alpha 50%, speed=50
    // Legacy: on impact spawns 1x type 30 + 3x type 31 (handled by type 30 composite)
    {
        .type_id = effect_type_id::spell_mass_fire_strike_proj,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 5,
        .max_frames = 0,
        .frame_time_ms = 20,
        .sound_id = 1,
        .emits_light = true,
        .light_radius = 15,
        .directional = true,
        .frames_per_direction = 4,
        .randomize_direction_frame = true,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::mass_fire_strike_main,
    },

    // Type 162: Confusion
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_confusion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 163: Mass Chill Wind Spawner (spawns 4 fixed + 4 random mass_chill_wind)
    // Legacy bAddNewEffect: maxFrame=2, frameTime=10
    // Legacy bEffectFrameCounter: spawns 4x fixed + 4x random type 56, then deletes
    {
        .type_id = effect_type_id::spell_mass_chill_wind_spawner,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::mass_chill_wind, 0, 1, 0, 0, false, 0, false, 0},
            {effect_type_id::mass_chill_wind, 0, 1, -30, -15, false, 0, false, -10},
            {effect_type_id::mass_chill_wind, 0, 1, 35, -30, false, 0, false, -6},
            {effect_type_id::mass_chill_wind, 0, 1, 20, 30, false, 0, false, -3},
            {effect_type_id::mass_chill_wind, 0, 4, 0, 0, true, 50, false, -2, -2},
        }},
        .child_count = 5,
    },

    // Type 164: Earthworm Strike Spawner (spawns dust + worm_bite on expire)
    // Legacy bAddNewEffect: maxFrame=1, frameTime=10, sound='E' 4 + spawns 14x type14
    // Legacy bEffectFrameCounter: on expire spawns type 68 (worm_bite)
    {
        .type_id = effect_type_id::spell_earthworm_strike_spawner,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .sound_id = 4,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::dust_cloud, 0, 14, 0, 0, true, 60},
            {effect_type_id::worm_bite, 1, 1, 0, 0, false, 0},
        }},
        .child_count = 2,
    },

    // Type 165: Absolute Magic Protection
    // Legacy DrawEffects: m_pEffectSpr[53]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=21, frameTime=70, sound='E' 5
    {
        .type_id = effect_type_id::spell_absolute_magic_protection,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 53,
        .max_frames = 21,
        .frame_time_ms = 70,
        .sound_id = 5,
    },

    // Type 166: Armor Break
    // Legacy DrawEffects: m_pEffectSpr[55]->PutRevTransSprite(dX, dY+35) + m_pEffectSpr[54]->PutTransSprite50
    // Legacy bAddNewEffect: maxFrame=13, frameTime=80, sound='E' 5
    // Primary: PAK 54 at 50% alpha. Secondary: PAK 55 at y+35.
    {
        .type_id = effect_type_id::spell_armor_break,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 54,
        .max_frames = 13,
        .frame_time_ms = 80,
        .sound_id = 5,
        .overlays = {{
            {55, 0, 35, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
    },

    // Type 170: Bloody Shock Wave Timer (spawns blood projectiles from caster to target area)
    // Legacy bAddNewEffect: maxFrame=7, frameTime=80
    // Legacy bEffectFrameCounter: every even frame (0,2,4,6) spawns type 34 from sX,sY to dX*32±30
    {
        .type_id = effect_type_id::spell_bloody_shock_wave_timer,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible timer
        .max_frames = 7,
        .frame_time_ms = 80,
        .sound_id = 1,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::blood_projectile, 0, 1, 0, 0, true, 30, true},
            {effect_type_id::blood_projectile, 2, 1, 0, 0, true, 30, true},
            {effect_type_id::blood_projectile, 4, 1, 0, 0, true, 30, true},
            {effect_type_id::blood_projectile, 6, 1, 0, 0, true, 30, true},
        }},
        .child_count = 4,
    },

    // Type 171: Mass Confusion
    // Legacy DrawEffects: m_pEffectSpr[4]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=13, frameTime=120, sound='E' 5
    {
        .type_id = effect_type_id::spell_mass_confusion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 4,
        .max_frames = 13,
        .frame_time_ms = 120,
        .sound_id = 5,
    },

    // Type 172: Mass Ice Strike Spawner (spawns many large + small meteor fragments)
    // Legacy: maxFrame=2, frameTime=10 (instant spawner)
    // Spawns 1 centered type44 + 8 immediate type44 + 16 staggered type44 (-1 to -16)
    //       + 4 staggered type45 (-11 to -14) + 4 staggered type46 (-15 to -18)
    {
        .type_id = effect_type_id::spell_mass_ice_strike_spawner,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible spawner
        .max_frames = 2,
        .frame_time_ms = 10,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::meteor_large_4, 0, 1, 0, 0, false, 0, false, 0, 0},
            {effect_type_id::meteor_large_4, 0, 8, 10, 0, true, 55, false, 0, 0},
            {effect_type_id::meteor_large_4, 0, 16, 10, 0, true, 55, false, -1, -1},
            {effect_type_id::meteor_small_1, 0, 4, 10, 0, true, 50, false, -11, -1},
            {effect_type_id::meteor_small_2, 0, 4, 10, 0, true, 50, false, -15, -1},
        }},
        .child_count = 5,
    },

    // Type 174: Lightning Strike Timer (spawns lightning bolts each frame)
    // Legacy bAddNewEffect: maxFrame=5, frameTime=120
    // Legacy bEffectFrameCounter: each frame spawns type 151 from sX,sY to dX±rand,dY±rand + sound
    {
        .type_id = effect_type_id::spell_lightning_strike_timer,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible timer
        .max_frames = 5,
        .frame_time_ms = 120,
        .sound_id = 1,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::spell_lightning_bolt_thunder, -1, 1, 0, 0, true, 32, true},
        }},
        .child_count = 1,
    },

    // Type 180: Illusion
    // Legacy DrawEffects: m_pEffectSpr[60]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=11, frameTime=100, sound='E' 5
    {
        .type_id = effect_type_id::spell_illusion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 60,
        .max_frames = 11,
        .frame_time_ms = 100,
        .sound_id = 5,
        .x_offset = 0,
        .height_offset = 0,
        .overlays = {{
            {59, 0, 0, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
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

    // Type 182: Mass Magic Missile Flying (projectile from caster to target)
    // Legacy: sX*32, sY*32-40, speed=50, frameTime=20, sprite[98] at alpha 50%
    // Legacy: spawns type 8 trail, on arrival spawns AURA1 + 3x AURA2
    // Sprite 98 = effect11.pak sprite #9
    {
        .type_id = effect_type_id::spell_mass_magic_missile_proj,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::alpha_50,
        .sprite_pak_index = 98,
        .max_frames = 0,
        .frame_time_ms = 20,
        .sound_id = 1,
        .uses_tile_coords = false,
        .projectile_speed = 50,
        .impact_effect = effect_type_id::mass_magic_missile_aura1,
        .trail_effect = effect_type_id::burst_stationary,
        .trail_count = 1,
        .trail_random_range = 10,
    },

    // Type 190: Mass Illusion
    // Legacy DrawEffects: m_pEffectSpr[61]->PutTransSprite_NoColorKey at m_dX*32
    // Legacy bAddNewEffect: maxFrame=11, frameTime=100, sound='E' 5
    {
        .type_id = effect_type_id::spell_mass_illusion,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 61,
        .max_frames = 11,
        .frame_time_ms = 100,
        .sound_id = 5,
        .x_offset = 0,
        .height_offset = 0,
        .overlays = {{
            {59, 0, 0, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
    },

    // Type 176: Cancellation
    // Legacy bAddNewEffect: maxFrame=23, frameTime=60, sound='E' 5
    // Legacy DrawEffects: m_pEffectSpr[90]->PutTransSprite_NoColorKey at dX*32+50, dY*32+85
    {
        .type_id = effect_type_id::spell_cancellation,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 90,
        .max_frames = 23,
        .frame_time_ms = 60,
        .sound_id = 5,
        .x_offset = 50,
        .height_offset = 85,
    },

    // Type 177: Illusion Movement
    // Legacy bAddNewEffect: maxFrame=11, frameTime=100, sound='E' 5 (shared with type 150)
    // Legacy DrawEffects: m_pEffectSpr[60]->PutTransSprite_NoColorKey, fade after frame 9
    {
        .type_id = effect_type_id::spell_illusion_movement,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 60,
        .max_frames = 11,
        .frame_time_ms = 100,
        .sound_id = 5,
        .x_offset = 0,
        .height_offset = 0,
        .overlays = {{
            {102, 0, 30, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
    },

    // Type 183: Inhibition Casting
    // Legacy bAddNewEffect: maxFrame=11, frameTime=100, sound='E' 5 (shared with type 150)
    // Legacy DrawEffects: m_pEffectSpr[94]->PutTransSprite_NoColorKey at dX, dY+40
    {
        .type_id = effect_type_id::spell_inhibition_casting,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 94,
        .max_frames = 11,
        .frame_time_ms = 100,
        .sound_id = 5,
        .x_offset = 0,
        .height_offset = 40,
        .overlays = {{
            {95, 0, 0, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
    },

    // Type 191: Blizzard Timer (spawns moving ice bolts each frame)
    // Legacy bAddNewEffect: maxFrame=7, frameTime=80
    // Legacy bEffectFrameCounter: each frame spawns type 71 (moving_ice_bolt_2) from sX,sY to dX*32±60 + sound
    {
        .type_id = effect_type_id::spell_blizzard_timer,
        .behavior = effect_behavior::composite,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 255,    // Invisible timer
        .max_frames = 7,
        .frame_time_ms = 80,
        .sound_id = 46,
        .uses_tile_coords = true,
        .children = {{
            {effect_type_id::moving_ice_bolt_2, -1, 1, 0, 0, true, 60, true},
        }},
        .child_count = 1,
    },
    // Type 195: Mass Illusion Movement
    // Legacy bAddNewEffect: maxFrame=11, frameTime=100, sound='E' 5 (shared with type 150)
    // Legacy DrawEffects: m_pEffectSpr[61]->PutTransSprite_NoColorKey, fade after frame 9
    {
        .type_id = effect_type_id::spell_mass_illusion_movement,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::transparent,
        .sprite_pak_index = 61,
        .max_frames = 11,
        .frame_time_ms = 100,
        .sound_id = 5,
        .x_offset = 0,
        .height_offset = 0,
        .overlays = {{
            {102, 0, 30, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
    },

    // Type 196: Earth Shock Wave (projectile from caster to target, spawns particles)
    // Legacy bAddNewEffect: mX=sX*32, mY=sY*32, maxFrame=30, frameTime=25, camera shake
    // Legacy bEffectFrameCounter: moves toward dest at speed 40, spawns 2x type 80 each frame
    // Legacy DrawEffects: m_pEffectSpr[91]->PutSpriteFast + m_pEffectSpr[92]->PutTransSprite
    {
        .type_id = effect_type_id::spell_earth_shock_wave,
        .behavior = effect_behavior::projectile,
        .render_mode = effect_render_mode::normal,
        .sprite_pak_index = 91,
        .max_frames = 30,
        .frame_time_ms = 25,
        .shake_intensity = 3,
        .no_additive = true,
        .overlays = {{
            {92, 0, 0, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
        .projectile_speed = 40,
        .trail_effect = effect_type_id::earth_shock_wave_particle,
        .trail_count = 2,
        .trail_random_range = 15,
    },

    // Type 80: Earth Shock Wave Particle (spawned by type 196)
    // Legacy bAddNewEffect: mX=sX, mY=sY, maxFrame=30, frameTime=25, camera shake
    // Legacy DrawEffects: same as type 196 (m_pEffectSpr[91] + m_pEffectSpr[92])
    {
        .type_id = effect_type_id::earth_shock_wave_particle,
        .behavior = effect_behavior::static_anim,
        .render_mode = effect_render_mode::normal,
        .sprite_pak_index = 91,
        .max_frames = 30,
        .frame_time_ms = 25,
        .shake_intensity = 3,
        .uses_tile_coords = false,
        .no_additive = true,
        .overlays = {{
            {92, 0, 0, effect_render_mode::transparent},
        }},
        .overlay_count = 1,
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
