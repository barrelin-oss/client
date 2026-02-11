#pragma once

#include "gameplay/effect.hpp"
#include "gameplay/effect_types.hpp"
#include "core/constants.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace hb {

class sprite_manager;
class sound_manager;
class world;
class renderer;

// Parameters for thunder bolt rendering
struct thunder_params
{
    float offset_pct = 0.10f;
    float offset_cap = 20.0f;
    float offset_min = 5.0f;
    float segment_size = 20.0f;
    int32_t jag_chance = 10;
    float jag_multiplier = 1.5f;
    int32_t thin_bolt_count = 4;
};

class effect_system
{
public:
    effect_system() = default;
    ~effect_system() = default;

    effect_system(const effect_system&) = delete;
    effect_system& operator=(const effect_system&) = delete;

    // Lifecycle
    bool initialize(sprite_manager& sprites, sound_manager& sounds, world& w);
    void shutdown();
    void update(float delta_time);
    void render(renderer& rend, int32_t camera_x, int32_t camera_y);

    // Primary API (tile coordinates, matching legacy bAddNewEffect signature)
    void add_effect(effect_type_id type_id,
                    int32_t src_x, int32_t src_y,
                    int32_t dest_x, int32_t dest_y,
                    int8_t start_frame = 0, int32_t value = 1);

    // Projectile/composite in world pixel coordinates (bypasses tile conversion + height_offset)
    void add_effect_world(effect_type_id type_id,
                          float src_x, float src_y,
                          float dest_x, float dest_y);

    // Convenience: single-position effect at tile coordinates
    void add_effect_at(effect_type_id type_id, int32_t tile_x, int32_t tile_y);

    // Convenience: effect at world pixel coordinates
    void add_effect_at_pixel(effect_type_id type_id, float world_x, float world_y);

    // Clear all active effects
    void clear();

    // Fish node management
    void add_fish_node(uint32_t fish_index, int32_t tile_x, int32_t tile_y);
    void remove_fish_node(uint32_t fish_index);
    void clear_fish_nodes();

    // Detail level: 0 = low (skip particles), 1 = medium, 2 = high
    void set_detail_level(uint8_t level) { detail_level_ = level; }
    uint8_t detail_level() const { return detail_level_; }

    // How many effects are currently active
    size_t active_count() const;

    // Render overrides for effect test tool
    struct render_override
    {
        bool active = false;
        bool force_additive = false;  // Use additive blending for all primary sprites
        float alpha_multiplier = 1.0f; // Multiply all alpha values by this
    };
    render_override render_override_;

private:
    int32_t find_free_slot() const;
    void init_effect(effect& eff, const effect_definition& def,
                     float src_x, float src_y,
                     float dest_x, float dest_y,
                     int8_t start_frame, int32_t value);
    void init_bresenham(effect& eff);

    void update_static(effect& eff, float delta_time);
    void update_projectile(effect& eff, float delta_time);
    void update_physics(effect& eff, float delta_time);
    void update_composite(effect& eff, float delta_time);

    bool step_bresenham(effect& eff);
    void spawn_children(const effect& parent, const effect_definition& def, int8_t trigger_frame);

    void play_effect_sound(const effect_definition& def, float world_x, float world_y);
    void trigger_shake(const effect_definition& def);
    const sprite* resolve_sprite(const effect_definition& def);
    void render_thunder(renderer& rend, float sx, float sy, float dx, float dy,
                        int8_t rx, int8_t ry);
    void render_overlay(renderer& rend, const effect& eff,
                        const effect_definition::sprite_overlay& ov,
                        int32_t screen_x, int32_t screen_y);

    std::array<effect, max_effects> effects_{};
    std::array<const sprite*, max_effect_sprites> effect_sprites_{};
    bool sprites_loaded_ = false;

    sprite_manager* sprites_ = nullptr;
    sound_manager* sounds_ = nullptr;
    world* world_ = nullptr;
    uint8_t detail_level_ = 2;
    thunder_params thunder_params_;

    // Fish node tracking: fish_index -> effect slot index
    std::unordered_map<uint32_t, int32_t> fish_nodes_;
};

} // namespace hb
