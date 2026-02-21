#pragma once

#include "gameplay/effect_types.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace hb
{

// Specification for a child effect spawned by a composite parent
struct child_effect_spec
{
    effect_type_id type = effect_type_id::none;
    int8_t trigger_frame = 0; // Frame at which to spawn (-1 = every frame)
    int8_t count = 1;         // How many to spawn
    int16_t offset_x = 0;     // Pixel offset from parent
    int16_t offset_y = 0;
    bool random_offset = false;
    int16_t random_range = 0;
    bool as_projectile = false;  // Spawn as projectile from parent src to parent dest
    int8_t start_frame = 0;      // Start frame for first copy (negative = delay)
    int8_t start_frame_step = 0; // Increment per copy (e.g. -1 for cascading delays)
};

// Static definition for each effect type (replaces the giant bAddNewEffect switch)
struct effect_definition
{
    effect_type_id type_id = effect_type_id::none;
    effect_behavior behavior = effect_behavior::static_anim;
    effect_render_mode render_mode = effect_render_mode::transparent;
    effect_detail detail_level = effect_detail::all;

    uint8_t sprite_pak_index = 0; // Index into effect sprite array (EffectN.pak)
    uint8_t start_frame = 0;
    uint8_t max_frames = 0;
    uint16_t frame_time_ms = 30;

    int8_t sound_id = -1;        // 'E' sound num, -1 = none
    int8_t shake_intensity = 0;  // Camera shake on spawn (0 = none)
    int8_t shake_multiplier = 0; // Shake multiplier for powerful effects

    bool emits_light = false;
    uint8_t light_radius = 0;

    int8_t gravity = 0;               // For physics/falling effects (positive = downward)
    uint8_t fall_start_frame = 0;     // Frame at which gravity falling begins (composite/static)
    int16_t fall_initial_speed = 0;   // Initial vertical speed when falling starts
    int16_t fall_initial_speed_x = 0; // Initial horizontal speed when falling starts
    bool uses_tile_coords = true;     // Convert tile->pixel on creation
    int16_t x_offset = 0;             // Horizontal pixel offset from tile origin
    int16_t height_offset = 0;        // Vertical pixel offset from tile origin

    bool directional = false;               // Use direction for sprite frame offset
    uint8_t frame_offset = 0;               // Base frame offset (for multi-variant PAK sprites)
    uint8_t frames_per_direction = 0;       // Directional frame stride override (0 = max_frames+1)
    bool randomize_direction_frame = false; // Randomize within direction frame range
    bool no_additive = false;               // Skip additive blending (earth effects etc.)

    // Random render frame: when max > min, renderer picks a random frame from [min,max]
    // each render call instead of using current_frame (legacy blood splatter etc.)
    uint8_t random_frame_min = 0;
    uint8_t random_frame_max = 0;

    // Per-channel color offset for PutTransSpriteRGB effects (fire=red, ice=blue, etc.)
    // Values in 0-255 range, converted to 0.0-1.0 in shader
    int16_t rgb_tint_r = 0;
    int16_t rgb_tint_g = 0;
    int16_t rgb_tint_b = 0;

    // Sprite overlays rendered on top of the primary sprite
    struct sprite_overlay
    {
        uint8_t pak_index = 255; // 255 = unused
        int16_t offset_x = 0;
        int16_t offset_y = 0;
        effect_render_mode render_mode = effect_render_mode::transparent;
        uint8_t frame_offset = 0; // Override frame offset (0 = use parent's frame_offset)
    };
    static constexpr size_t max_overlays = 4;
    std::array<sprite_overlay, max_overlays> overlays{};
    uint8_t overlay_count = 0;

    // Projectile speed: pixels per Bresenham step (legacy GetPoint step param)
    // Arrow=70, spell projectiles=50, fire strike proj=50
    uint8_t projectile_speed = 4;

    // Projectile arrival: what to spawn when reaching destination
    effect_type_id impact_effect = effect_type_id::none;

    // Projectile trail: particles spawned each frame while traveling
    effect_type_id trail_effect = effect_type_id::none;
    uint8_t trail_count = 0;         // How many trail particles per frame
    int16_t trail_random_range = 10; // Random offset range in pixels

    static constexpr size_t max_children = 8;
    std::array<child_effect_spec, max_children> children{};
    uint8_t child_count = 0;
};

// Look up a definition by effect type ID. Returns nullptr if not found.
const effect_definition* get_effect_definition(effect_type_id type);

// All registered effect definitions
std::span<const effect_definition> all_effect_definitions();

} // namespace hb
