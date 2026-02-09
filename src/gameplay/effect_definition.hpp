#pragma once

#include "gameplay/effect_types.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace hb {

// Specification for a child effect spawned by a composite parent
struct child_effect_spec
{
    effect_type_id type = effect_type_id::none;
    int8_t trigger_frame = 0;       // Frame at which to spawn (-1 = every frame)
    int8_t count = 1;               // How many to spawn
    int16_t offset_x = 0;           // Pixel offset from parent
    int16_t offset_y = 0;
    bool random_offset = false;
    int16_t random_range = 0;
    bool as_projectile = false;     // Spawn as projectile from parent src to parent dest
};

// Static definition for each effect type (replaces the giant bAddNewEffect switch)
struct effect_definition
{
    effect_type_id type_id = effect_type_id::none;
    effect_behavior behavior = effect_behavior::static_anim;
    effect_render_mode render_mode = effect_render_mode::transparent;
    effect_detail detail_level = effect_detail::all;

    uint8_t sprite_pak_index = 0;   // Index into effect sprite array (EffectN.pak)
    uint8_t start_frame = 0;
    uint8_t max_frames = 0;
    uint16_t frame_time_ms = 30;

    int8_t sound_id = -1;           // 'E' sound num, -1 = none
    int8_t shake_intensity = 0;     // Camera shake on spawn (0 = none)
    int8_t shake_multiplier = 0;    // Shake multiplier for powerful effects

    bool emits_light = false;
    uint8_t light_radius = 0;

    int8_t gravity = 0;             // For physics particles (positive = downward)
    bool uses_tile_coords = true;   // Convert tile->pixel on creation
    int16_t height_offset = -40;    // Default head-height offset in pixels

    bool directional = false;       // Use direction for sprite frame offset

    // Projectile speed: pixels per Bresenham step (legacy GetPoint step param)
    // Arrow=70, spell projectiles=50, fire strike proj=50
    uint8_t projectile_speed = 4;

    // Projectile arrival: what to spawn when reaching destination
    effect_type_id impact_effect = effect_type_id::none;

    // Projectile trail: particles spawned each frame while traveling
    effect_type_id trail_effect = effect_type_id::none;
    uint8_t trail_count = 0;            // How many trail particles per frame
    int16_t trail_random_range = 10;    // Random offset range in pixels

    static constexpr size_t max_children = 8;
    std::array<child_effect_spec, max_children> children{};
    uint8_t child_count = 0;
};

// Look up a definition by effect type ID. Returns nullptr if not found.
const effect_definition* get_effect_definition(effect_type_id type);

// All registered effect definitions
std::span<const effect_definition> all_effect_definitions();

} // namespace hb
