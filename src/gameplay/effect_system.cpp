#include "gameplay/effect_system.hpp"
#include "assets/sprite_manager.hpp"
#include "audio/sound_manager.hpp"
#include "core/random.hpp"
#include "world/world.hpp"
#include "world/tile.hpp"
#include "graphics/renderer.hpp"
#include "core/direction_utils.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <string>

namespace hb
{

// Helper: draw an effect sprite with additive blending, using tinted variant when tint is set
static void
draw_additive(renderer& rend, const effect& eff, const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha)
{
    if (eff.def->rgb_tint_r != 0 || eff.def->rgb_tint_g != 0 || eff.def->rgb_tint_b != 0)
        rend.draw_sprite_additive_tinted(
            spr, x, y, frame, alpha, eff.def->rgb_tint_r, eff.def->rgb_tint_g, eff.def->rgb_tint_b);
    else
        rend.draw_sprite_additive_alpha(spr, x, y, frame, alpha);
}

// Forward declaration (defined below render_thunder)
static void draw_thunder_bolt(renderer& rend,
                              int32_t sx,
                              int32_t sy,
                              int32_t dx,
                              int32_t dy,
                              int32_t seed_offset,
                              bool thick,
                              const thunder_params& params);

bool effect_system::initialize(sprite_manager& sprites, sound_manager& sounds, world& w)
{
    sprites_ = &sprites;
    sounds_ = &sounds;
    world_ = &w;

    // Effect PAKs are already loaded in game_state init sequence.
    // Pre-resolve all effect sprites using the legacy global index mapping.
    // Matches Game.cpp MakeEffectSpr() calls (lines 4213-4227).
    struct pak_mapping
    {
        const char* pak_name;
        uint8_t global_start;
        uint8_t count;
        uint8_t local_start;
    };
    static constexpr pak_mapping mappings[] = {
        {"effect", 0, 10, 0},
        {"effect2", 10, 3, 0},
        {"effect3", 13, 6, 0},
        {"effect4", 19, 5, 0},
        {"effect5", 24, 7, 1}, // local starts at 1
        {"CruEffect1", 31, 9, 0},
        {"effect6", 40, 5, 0},
        {"effect7", 45, 12, 0},
        {"effect8", 57, 9, 0},
        {"effect9", 66, 21, 0},
        {"effect10", 87, 2, 0},
        {"effect11", 89, 14, 0},
        {"effect11s", 104, 1, 0},
        {"effect13", 105, 3, 0},
        {"effect12", 148, 4, 0},
    };

    int loaded = 0;
    for (const auto& m : mappings)
    {
        for (uint8_t i = 0; i < m.count; ++i)
        {
            uint8_t global_idx = m.global_start + i;
            if (global_idx >= max_effect_sprites)
                break;

            auto* spr = sprites_->get_sprite(m.pak_name, m.local_start + i);
            if (spr)
            {
                effect_sprites_[global_idx] = spr;
                ++loaded;
            }
        }
    }

    sprites_loaded_ = true;
    spdlog::info("Effect system initialized ({} effect sprites resolved)", loaded);
    return true;
}

void effect_system::shutdown()
{
    clear();
    sprites_ = nullptr;
    sounds_ = nullptr;
    world_ = nullptr;
}

void effect_system::update(float delta_time)
{
    for (auto& eff : effects_)
    {
        if (!eff.active || !eff.def)
        {
            continue;
        }

        eff.elapsed += delta_time;

        // Safety timeout: no effect should live longer than 10 seconds
        if (eff.elapsed > 10.0f)
        {
            eff.active = false;
            continue;
        }

        switch (eff.def->behavior)
        {
        case effect_behavior::static_anim:
            update_static(eff, delta_time);
            break;
        case effect_behavior::projectile:
            update_projectile(eff, delta_time);
            break;
        case effect_behavior::physics:
            update_physics(eff, delta_time);
            break;
        case effect_behavior::composite:
            update_composite(eff, delta_time);
            break;
        }
    }
}

void effect_system::render(renderer& rend, int32_t camera_x, int32_t camera_y)
{
    auto screen_w = static_cast<int32_t>(rend.scene_width());
    auto screen_h = static_cast<int32_t>(rend.scene_height());

    // Extended mode: effects outside fair zone are hidden
    bool extended_cull = rend.current_view_mode() == view_mode::extended;
    sf::IntRect fair;
    if (extended_cull)
        fair = rend.fair_bounds();

    for (const auto& eff : effects_)
    {
        if (!eff.active || !eff.def)
        {
            continue;
        }

        // Thunder effects don't need a sprite
        bool is_thunder = (eff.def->render_mode == effect_render_mode::thunder);
        if (!is_thunder && !eff.sprite_ptr)
        {
            continue;
        }

        // World to screen conversion (includes sub-frame interpolation)
        auto screen_x = static_cast<int32_t>(eff.pos_x + eff.interp_x) - camera_x;
        auto screen_y = static_cast<int32_t>(eff.pos_y + eff.interp_y) - camera_y;

        // Visibility culling with generous margin
        // Thunder needs large margin for vertical bolts; falling effects need margin for their offsets
        int32_t margin = is_thunder ? 900 : 128;
        if (eff.def->height_offset != 0 || eff.def->x_offset != 0)
        {
            margin = std::max(margin, std::abs(static_cast<int32_t>(eff.def->height_offset)) + 64);
            margin = std::max(margin, std::abs(static_cast<int32_t>(eff.def->x_offset)) + 64);
        }
        if (screen_x < -margin || screen_x > screen_w + margin || screen_y < -margin || screen_y > screen_h + margin)
        {
            continue;
        }

        // Extended mode: additionally check fair zone bounds
        if (extended_cull)
        {
            if (screen_x < fair.position.x - 64 || screen_x > fair.position.x + fair.size.x + 64 ||
                screen_y < fair.position.y - 64 || screen_y > fair.position.y + fair.size.y)
                continue;
        }

        // Thunder: procedural lightning bolt rendering
        if (is_thunder)
        {
            // Determine source and destination in screen space
            float thunder_sx, thunder_sy, thunder_dx, thunder_dy;
            if (std::abs(eff.src_x - eff.dest_x) < 1.0f && std::abs(eff.src_y - eff.dest_y) < 1.0f)
            {
                // Single-point effect (type 143): lightning from sky
                thunder_sx = static_cast<float>(screen_x);
                thunder_sy = static_cast<float>(screen_y) - 800.0f;
                thunder_dx = static_cast<float>(screen_x);
                thunder_dy = static_cast<float>(screen_y);
            }
            else
            {
                // Point-to-point (type 151): bolt from caster to target
                thunder_sx = eff.src_x - static_cast<float>(camera_x);
                thunder_sy = eff.src_y - static_cast<float>(camera_y);
                thunder_dx = eff.dest_x - static_cast<float>(camera_x);
                thunder_dy = eff.dest_y - static_cast<float>(camera_y);
            }
            render_thunder(rend, thunder_sx, thunder_sy, thunder_dx, thunder_dy, eff.rx, eff.ry);
            continue;
        }

        // Skip rendering if frame is negative (delayed start - still counting up)
        if (eff.current_frame < 0)
        {
            continue;
        }

        // Calculate the frame to render
        uint32_t frame = static_cast<uint32_t>(eff.current_frame) + eff.def->frame_offset;

        // For directional effects, offset frame by direction
        if (eff.def->directional)
        {
            uint32_t stride =
                eff.def->frames_per_direction > 0 ? eff.def->frames_per_direction : (eff.def->max_frames + 1);
            frame = static_cast<uint32_t>(eff.direction_index) * stride;
            if (eff.def->randomize_direction_frame && stride > 0)
            {
                frame += static_cast<uint32_t>(random_mod(stride));
            }
            // Add frame_offset on top of directional calc
            frame += eff.def->frame_offset;
        }

        // Render overlays first (behind primary sprite)
        for (uint8_t oi = 0; oi < eff.def->overlay_count; ++oi)
        {
            render_overlay(rend, eff, eff.def->overlays[oi], screen_x, screen_y);
        }

        // Render primary sprite on top
        // Default: additive blending for all effects (unless definition opts out)
        // Test tool override can force additive on/off and apply alpha multiplier
        bool use_additive;
        float alpha_mul;
        if (render_override_.active)
        {
            use_additive = render_override_.force_additive && !eff.def->no_additive;
            alpha_mul = render_override_.alpha_multiplier;
        }
        else
        {
            use_additive = !eff.def->no_additive;
            alpha_mul = 1.0f;
        }

        switch (eff.def->render_mode)
        {
        case effect_render_mode::normal:
        case effect_render_mode::transparent:
            if (use_additive)
                draw_additive(rend, eff, *eff.sprite_ptr, screen_x, screen_y, frame, alpha_mul);
            else if (alpha_mul < 1.0f)
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, alpha_mul);
            else
                rend.draw_sprite(*eff.sprite_ptr, screen_x, screen_y, frame);
            break;

        case effect_render_mode::alpha_25:
        {
            float a = 0.25f * alpha_mul;
            if (use_additive)
                draw_additive(rend, eff, *eff.sprite_ptr, screen_x, screen_y, frame, a);
            else
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, a);
            break;
        }

        case effect_render_mode::alpha_50:
        {
            float a = 0.5f * alpha_mul;
            if (use_additive)
                draw_additive(rend, eff, *eff.sprite_ptr, screen_x, screen_y, frame, a);
            else
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, a);
            break;
        }

        case effect_render_mode::alpha_70:
        {
            float a = 0.7f * alpha_mul;
            if (use_additive)
                draw_additive(rend, eff, *eff.sprite_ptr, screen_x, screen_y, frame, a);
            else
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, a);
            break;
        }

        case effect_render_mode::fade:
        {
            // Fade out over the effect lifetime
            float total_time =
                static_cast<float>(eff.def->max_frames) * static_cast<float>(eff.def->frame_time_ms) / 1000.0f;
            float alpha = 1.0f;
            if (total_time > 0.0f)
            {
                alpha = 1.0f - (eff.elapsed / total_time);
                if (alpha < 0.0f)
                    alpha = 0.0f;
            }
            alpha *= alpha_mul;
            if (use_additive)
                draw_additive(rend, eff, *eff.sprite_ptr, screen_x, screen_y, frame, alpha);
            else
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, alpha);
            break;
        }

        case effect_render_mode::arrow_trail:
        {
            // Multi-point fading trail behind projectile (legacy lightning arrow)
            // Compute backward direction from current pos toward source
            float bx = eff.src_x - eff.pos_x;
            float by = eff.src_y - eff.pos_y;
            float blen = std::sqrt(bx * bx + by * by);
            if (blen < 1.0f)
                blen = 1.0f;
            float nx = bx / blen;
            float ny = by / blen;

            uint32_t stride = eff.def->frames_per_direction > 0 ? eff.def->frames_per_direction : 1;

            // 5 trailing copies at distances 75, 60, 45, 30, 15 behind projectile
            // Alphas: 25%, 25%, 50%, 50%, 70% (tail to head)
            struct trail_point
            {
                float dist;
                float alpha;
            };
            static constexpr trail_point points[] = {
                {75.0f, 0.25f},
                {60.0f, 0.25f},
                {45.0f, 0.50f},
                {30.0f, 0.50f},
                {15.0f, 0.70f},
            };

            for (const auto& pt : points)
            {
                int32_t tx = screen_x + static_cast<int32_t>(nx * pt.dist);
                int32_t ty = screen_y + static_cast<int32_t>(ny * pt.dist);
                // Each trail copy gets independently randomized directional frame
                uint32_t tf = static_cast<uint32_t>(eff.direction_index) * stride +
                              static_cast<uint32_t>(random_mod(stride)) + eff.def->frame_offset;
                float a = pt.alpha * alpha_mul;
                if (use_additive)
                    draw_additive(rend, eff, *eff.sprite_ptr, tx, ty, tf, a);
                else
                    rend.draw_sprite_alpha(*eff.sprite_ptr, tx, ty, tf, a);
            }

            // Head at full opacity
            if (use_additive)
                draw_additive(rend, eff, *eff.sprite_ptr, screen_x, screen_y, frame, alpha_mul);
            else if (alpha_mul < 1.0f)
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, alpha_mul);
            else
                rend.draw_sprite(*eff.sprite_ptr, screen_x, screen_y, frame);
            break;
        }

        case effect_render_mode::thunder:
            break; // Handled above
        }
    }
}

void effect_system::add_effect(effect_type_id type_id,
                               int32_t src_x,
                               int32_t src_y,
                               int32_t dest_x,
                               int32_t dest_y,
                               int8_t start_frame,
                               int32_t value)
{
    const auto* def = get_effect_definition(type_id);
    if (!def)
    {
        spdlog::warn("Unknown effect type: {}", static_cast<uint16_t>(type_id));
        return;
    }

    // Detail level filtering
    if (detail_level_ == 0 && def->detail_level == effect_detail::high_only)
    {
        return;
    }

    int32_t slot = find_free_slot();
    if (slot < 0)
    {
        return; // Pool full
    }

    auto& eff = effects_[slot];

    // Convert tile coords to world pixels (tile center, matching entity positions)
    float fx_src, fy_src, fx_dest, fy_dest;
    if (def->uses_tile_coords)
    {
        fx_src = static_cast<float>(src_x * tile_width + tile_width / 2);
        fy_src = static_cast<float>(src_y * tile_height + tile_height / 2);
        fx_dest = static_cast<float>(dest_x * tile_width + tile_width / 2);
        fy_dest = static_cast<float>(dest_y * tile_height + tile_height / 2);
    }
    else
    {
        fx_src = static_cast<float>(src_x);
        fy_src = static_cast<float>(src_y);
        fx_dest = static_cast<float>(dest_x);
        fy_dest = static_cast<float>(dest_y);
    }

    init_effect(eff, *def, fx_src, fy_src, fx_dest, fy_dest, start_frame, value);

    // Play sound
    play_effect_sound(*def, eff.pos_x, eff.pos_y);

    // Camera shake
    trigger_shake(*def);
}

void effect_system::add_effect_world(effect_type_id type_id, float src_x, float src_y, float dest_x, float dest_y)
{
    const auto* def = get_effect_definition(type_id);
    if (!def)
    {
        spdlog::warn("Unknown effect type: {}", static_cast<uint16_t>(type_id));
        return;
    }

    if (detail_level_ == 0 && def->detail_level == effect_detail::high_only)
    {
        return;
    }

    int32_t slot = find_free_slot();
    if (slot < 0)
    {
        return;
    }

    auto& eff = effects_[slot];

    init_effect(eff, *def, src_x, src_y, dest_x, dest_y, 0, 1);

    play_effect_sound(*def, eff.pos_x, eff.pos_y);
    trigger_shake(*def);
}

void effect_system::add_effect_at(effect_type_id type_id, int32_t tile_x, int32_t tile_y)
{
    add_effect(type_id, tile_x, tile_y, tile_x, tile_y);
}

void effect_system::add_effect_at_pixel(effect_type_id type_id, float world_x, float world_y)
{
    const auto* def = get_effect_definition(type_id);
    if (!def)
    {
        spdlog::debug("Unknown effect type: {}", static_cast<uint16_t>(type_id));
        return;
    }

    if (detail_level_ == 0 && def->detail_level == effect_detail::high_only)
    {
        return;
    }

    int32_t slot = find_free_slot();
    if (slot < 0)
    {
        return;
    }

    auto& eff = effects_[slot];

    init_effect(eff, *def, world_x, world_y, world_x, world_y, 0, 1);
    play_effect_sound(*def, eff.pos_x, eff.pos_y);
    trigger_shake(*def);
}

void effect_system::clear()
{
    for (auto& eff : effects_)
    {
        eff.clear();
    }
    fish_nodes_.clear();
}

void effect_system::add_fish_node(uint32_t fish_index, int32_t tile_x, int32_t tile_y)
{
    // Remove existing node with same index
    remove_fish_node(fish_index);

    // Use effect type 8 (water ripple) as a looping fish node indicator
    auto type_id = static_cast<effect_type_id>(8);
    const auto* def = get_effect_definition(type_id);
    if (!def)
    {
        spdlog::warn("No effect definition for fish node visual");
        return;
    }

    int32_t slot = find_free_slot();
    if (slot < 0)
    {
        spdlog::warn("No free effect slot for fish node {}", fish_index);
        return;
    }

    // Convert tile coords to world pixels (tile center)
    float wx = static_cast<float>(tile_x * 32 + 16);
    float wy = static_cast<float>(tile_y * 32 + 16);

    auto& eff = effects_[slot];
    init_effect(eff, *def, wx, wy, wx, wy, 0, 1);
    eff.looping = true;
    fish_nodes_[fish_index] = slot;
    spdlog::debug("Fish node {} spawned at ({},{}) in slot {}", fish_index, tile_x, tile_y, slot);
}

void effect_system::remove_fish_node(uint32_t fish_index)
{
    auto it = fish_nodes_.find(fish_index);
    if (it != fish_nodes_.end())
    {
        int32_t slot = it->second;
        if (slot >= 0 && slot < static_cast<int32_t>(effects_.size()))
        {
            effects_[slot].clear();
        }
        fish_nodes_.erase(it);
        spdlog::debug("Fish node {} removed", fish_index);
    }
}

void effect_system::clear_fish_nodes()
{
    for (auto& [fish_index, slot] : fish_nodes_)
    {
        if (slot >= 0 && slot < static_cast<int32_t>(effects_.size()))
        {
            effects_[slot].clear();
        }
    }
    fish_nodes_.clear();
}

size_t effect_system::active_count() const
{
    size_t count = 0;
    for (const auto& eff : effects_)
    {
        if (eff.active)
        {
            ++count;
        }
    }
    return count;
}

int32_t effect_system::find_free_slot() const
{
    for (size_t i = 0; i < effects_.size(); ++i)
    {
        if (!effects_[i].active)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void effect_system::init_effect(effect& eff,
                                const effect_definition& def,
                                float src_x,
                                float src_y,
                                float dest_x,
                                float dest_y,
                                int8_t start_frame,
                                int32_t value)
{
    eff.clear();
    eff.def = &def;
    eff.active = true;
    eff.current_frame = start_frame;
    eff.max_frame = static_cast<int8_t>(def.max_frames);
    eff.value = value;

    // Apply position offsets to both endpoints
    float xo = static_cast<float>(def.x_offset);
    float h = static_cast<float>(def.height_offset);
    eff.src_x = src_x + xo;
    eff.src_y = src_y + h;
    eff.dest_x = dest_x + xo;
    eff.dest_y = dest_y + h;

    // Thunder effects position at dest (impact point) so children spawn at target
    if (def.render_mode == effect_render_mode::thunder)
    {
        eff.pos_x = eff.dest_x;
        eff.pos_y = eff.dest_y;
    }
    else
    {
        eff.pos_x = eff.src_x;
        eff.pos_y = eff.src_y;
    }
    eff.prev_x = eff.pos_x;
    eff.prev_y = eff.pos_y;

    // Calculate direction for directional effects
    if (def.directional || def.behavior == effect_behavior::projectile)
    {
        auto dir = calculate_direction(static_cast<int32_t>(src_x),
                                       static_cast<int32_t>(src_y),
                                       static_cast<int32_t>(dest_x),
                                       static_cast<int32_t>(dest_y));
        if (dir)
        {
            eff.direction_index = static_cast<uint8_t>(direction_to_sprite_index(*dir) - 1);
        }
    }

    // Initialize Bresenham for projectiles
    if (def.behavior == effect_behavior::projectile)
    {
        init_bresenham(eff);
    }

    // Initialize velocity for physics particles
    if (def.behavior == effect_behavior::physics)
    {
        // Random velocity for particles
        eff.velocity_x = static_cast<float>(random_int(-5, 5));
        eff.velocity_y = static_cast<float>(-random_int(2, 9));
    }

    // Initialize fall velocity for falling effects (composite/static with gravity)
    if (def.fall_initial_speed != 0)
    {
        eff.velocity_y = static_cast<float>(def.fall_initial_speed);
    }
    if (def.fall_initial_speed_x != 0)
    {
        eff.velocity_x = static_cast<float>(def.fall_initial_speed_x);
    }

    // Resolve sprite
    eff.sprite_ptr = resolve_sprite(def);
}

void effect_system::init_bresenham(effect& eff)
{
    int32_t x0 = static_cast<int32_t>(eff.src_x);
    int32_t y0 = static_cast<int32_t>(eff.src_y);
    int32_t x1 = static_cast<int32_t>(eff.dest_x);
    int32_t y1 = static_cast<int32_t>(eff.dest_y);

    eff.bresenham_dx = std::abs(x1 - x0);
    eff.bresenham_dy = std::abs(y1 - y0);
    eff.bresenham_sx = (x0 < x1) ? 1 : -1;
    eff.bresenham_sy = (y0 < y1) ? 1 : -1;
    eff.bresenham_err = eff.bresenham_dx - eff.bresenham_dy;
}

void effect_system::update_static(effect& eff, float delta_time)
{
    float frame_time = static_cast<float>(eff.def->frame_time_ms) / 1000.0f;
    eff.frame_accumulator += delta_time;

    while (eff.frame_accumulator >= frame_time)
    {
        eff.frame_accumulator -= frame_time;
        eff.current_frame++;

        if (eff.current_frame > eff.max_frame)
        {
            if (eff.looping)
            {
                eff.current_frame = 0;
            }
            else
            {
                eff.active = false;
                return;
            }
        }
    }
}

void effect_system::update_projectile(effect& eff, float delta_time)
{
    float frame_time = static_cast<float>(eff.def->frame_time_ms) / 1000.0f;
    eff.frame_accumulator += delta_time;

    while (eff.frame_accumulator >= frame_time)
    {
        eff.frame_accumulator -= frame_time;

        // Store previous position for trails
        eff.prev_x = eff.pos_x;
        eff.prev_y = eff.pos_y;

        // Step along the Bresenham line
        if (!step_bresenham(eff))
        {
            // Reached destination - spawn impact effect
            if (eff.def->impact_effect != effect_type_id::none)
            {
                add_effect_at_pixel(eff.def->impact_effect, eff.dest_x, eff.dest_y);
            }
            eff.active = false;
            return;
        }

        // Spawn trail particles while traveling
        if (eff.def->trail_effect != effect_type_id::none && eff.def->trail_count > 0)
        {
            int16_t range = eff.def->trail_random_range;
            for (uint8_t t = 0; t < eff.def->trail_count; ++t)
            {
                float tx = eff.pos_x + static_cast<float>(random_int(-range, range));
                float ty = eff.pos_y + static_cast<float>(random_int(-range, range));
                add_effect_at_pixel(eff.def->trail_effect, tx, ty);
            }
        }

        // For projectiles with max_frames > 0, advance frame
        if (eff.max_frame > 0)
        {
            eff.current_frame++;
            if (eff.current_frame > eff.max_frame)
            {
                eff.active = false;
                return;
            }
        }
    }
}

void effect_system::update_physics(effect& eff, float delta_time)
{
    float frame_time = static_cast<float>(eff.def->frame_time_ms) / 1000.0f;
    eff.frame_accumulator += delta_time;

    while (eff.frame_accumulator >= frame_time)
    {
        eff.frame_accumulator -= frame_time;

        // Store previous position
        eff.prev_x = eff.pos_x;
        eff.prev_y = eff.pos_y;

        // Apply velocity
        eff.pos_x += eff.velocity_x;
        eff.pos_y += eff.velocity_y;

        // Apply gravity
        eff.velocity_y += static_cast<float>(eff.def->gravity);

        // Advance frame
        eff.current_frame++;
        if (eff.current_frame > eff.max_frame)
        {
            eff.active = false;
            return;
        }
    }
}

void effect_system::update_composite(effect& eff, float delta_time)
{
    float frame_time = static_cast<float>(eff.def->frame_time_ms) / 1000.0f;
    eff.frame_accumulator += delta_time;

    while (eff.frame_accumulator >= frame_time)
    {
        eff.frame_accumulator -= frame_time;
        int8_t old_frame = eff.current_frame;
        eff.current_frame++;

        // Randomize thunder perturbation each frame (legacy bEffectFrameCounter behavior)
        if (eff.def->render_mode == effect_render_mode::thunder)
        {
            eff.rx = static_cast<int8_t>(random_int(-5, 5));
            eff.ry = static_cast<int8_t>(random_int(-5, 5));
        }

        // Velocity/gravity movement: apply after fall_start_frame, including death frame
        // (legacy applies physics before the death check, so the final position is correct)
        if ((eff.def->fall_initial_speed != 0 || eff.def->fall_initial_speed_x != 0 || eff.def->gravity != 0) &&
            eff.current_frame >= eff.def->fall_start_frame && eff.current_frame > 0)
        {
            eff.pos_x += eff.velocity_x;
            eff.pos_y += eff.velocity_y;
            eff.velocity_y += static_cast<float>(eff.def->gravity);
        }

        // Spawn children BEFORE death check so last-frame children can fire
        spawn_children(eff, *eff.def, old_frame);

        if (eff.current_frame > eff.max_frame)
        {
            eff.active = false;
            return;
        }
    }

    // Sub-frame interpolation for smooth movement between frame ticks
    bool has_velocity = eff.def->fall_initial_speed != 0 || eff.def->fall_initial_speed_x != 0 || eff.def->gravity != 0;
    if (has_velocity && eff.current_frame >= eff.def->fall_start_frame && eff.current_frame > 0 &&
        eff.current_frame <= eff.max_frame && frame_time > 0.0f)
    {
        float t = eff.frame_accumulator / frame_time;
        eff.interp_x = eff.velocity_x * t;
        eff.interp_y = eff.velocity_y * t;
    }
    else
    {
        eff.interp_x = 0.0f;
        eff.interp_y = 0.0f;
    }
}

bool effect_system::step_bresenham(effect& eff)
{
    int32_t x = static_cast<int32_t>(eff.pos_x);
    int32_t y = static_cast<int32_t>(eff.pos_y);
    int32_t dest_x = static_cast<int32_t>(eff.dest_x);
    int32_t dest_y = static_cast<int32_t>(eff.dest_y);

    // Check if we've reached the destination
    if (x == dest_x && y == dest_y)
    {
        return false;
    }

    // Take multiple steps per frame (matches legacy GetPoint step parameter)
    int steps_per_frame = eff.def ? eff.def->projectile_speed : 4;
    for (int step = 0; step < steps_per_frame; ++step)
    {
        x = static_cast<int32_t>(eff.pos_x);
        y = static_cast<int32_t>(eff.pos_y);

        if (x == dest_x && y == dest_y)
        {
            return false;
        }

        int32_t e2 = 2 * eff.bresenham_err;

        if (e2 > -eff.bresenham_dy)
        {
            eff.bresenham_err -= eff.bresenham_dy;
            eff.pos_x += static_cast<float>(eff.bresenham_sx);
        }

        if (e2 < eff.bresenham_dx)
        {
            eff.bresenham_err += eff.bresenham_dx;
            eff.pos_y += static_cast<float>(eff.bresenham_sy);
        }
    }

    return true;
}

void effect_system::spawn_children(const effect& parent, const effect_definition& def, int8_t trigger_frame)
{
    for (uint8_t i = 0; i < def.child_count; ++i)
    {
        const auto& child_spec = def.children[i];
        if (child_spec.type == effect_type_id::none)
        {
            continue;
        }

        // trigger_frame == -1 means "every frame"
        if (child_spec.trigger_frame != -1 && child_spec.trigger_frame != trigger_frame)
        {
            continue;
        }

        for (int8_t c = 0; c < child_spec.count; ++c)
        {
            // Staggered start frame: each copy gets a progressively offset start
            int8_t sf = static_cast<int8_t>(child_spec.start_frame + c * child_spec.start_frame_step);

            if (child_spec.as_projectile)
            {
                // Spawn as projectile from parent's source to parent's dest (+ fixed + random offset)
                float dest_x = parent.dest_x + static_cast<float>(child_spec.offset_x);
                float dest_y = parent.dest_y + static_cast<float>(child_spec.offset_y);
                if (child_spec.random_offset)
                {
                    int16_t range = child_spec.random_range;
                    dest_x += static_cast<float>(random_int(-range, range));
                    dest_y += static_cast<float>(random_int(-range, range));
                }

                // Look up the child effect definition
                const auto* child_def = get_effect_definition(child_spec.type);
                if (!child_def)
                    continue;

                int32_t slot = find_free_slot();
                if (slot < 0)
                    return;

                auto& eff = effects_[slot];
                // Parent's src/dest already have parent's offsets applied.
                // Subtract child's offsets so init_effect doesn't double-apply them.
                float child_xo = static_cast<float>(child_def->x_offset);
                float child_h = static_cast<float>(child_def->height_offset);
                init_effect(eff,
                            *child_def,
                            parent.src_x - child_xo,
                            parent.src_y - child_h,
                            dest_x - child_xo,
                            dest_y - child_h,
                            sf,
                            1);
                play_effect_sound(*child_def, eff.pos_x, eff.pos_y);
                trigger_shake(*child_def);
            }
            else
            {
                float offset_x = static_cast<float>(child_spec.offset_x);
                float offset_y = static_cast<float>(child_spec.offset_y);

                if (child_spec.random_offset)
                {
                    offset_x += static_cast<float>(random_int(-child_spec.random_range, child_spec.random_range));
                    offset_y += static_cast<float>(random_int(-child_spec.random_range, child_spec.random_range));
                }

                float child_x = parent.pos_x + offset_x;
                float child_y = parent.pos_y + offset_y;

                // Use inline spawning to support staggered start frames
                const auto* child_def = get_effect_definition(child_spec.type);
                if (!child_def)
                    continue;

                int32_t slot = find_free_slot();
                if (slot < 0)
                    return;

                auto& eff = effects_[slot];
                // Pass position directly; let init_effect apply the child's own offsets
                init_effect(eff, *child_def, child_x, child_y, child_x, child_y, sf, 1);
                play_effect_sound(*child_def, eff.pos_x, eff.pos_y);
                trigger_shake(*child_def);
            }
        }
    }
}

void effect_system::play_effect_sound(const effect_definition& def, float world_x, float world_y)
{
    if (def.sound_id < 0 || !sounds_)
    {
        return;
    }

    sounds_->play_sound_at('E', def.sound_id, static_cast<int32_t>(world_x), static_cast<int32_t>(world_y));
}

void effect_system::trigger_shake(const effect_definition& def)
{
    if (def.shake_intensity <= 0 || !world_)
    {
        return;
    }

    float intensity = static_cast<float>(def.shake_intensity);
    if (def.shake_multiplier > 0)
    {
        intensity *= static_cast<float>(def.shake_multiplier);
    }

    // Duration scales with intensity
    float duration = 0.2f + intensity * 0.05f;
    world_->add_camera_shake(intensity, duration);
}

const sprite* effect_system::resolve_sprite(const effect_definition& def)
{
    uint8_t idx = def.sprite_pak_index;
    if (idx < max_effect_sprites)
    {
        return effect_sprites_[idx];
    }
    return nullptr;
}

// Traces a single jagged lightning bolt from (sx,sy) to (dx,dy).
// Uses midpoint displacement for natural-looking arcs with perpendicular offsets.
// thick=true draws the main bolt (multi-line glow), thick=false draws a thin companion bolt.
static void draw_thunder_bolt(renderer& rend,
                              int32_t sx,
                              int32_t sy,
                              int32_t dx,
                              int32_t dy,
                              int32_t seed_offset,
                              bool thick,
                              const thunder_params& params)
{
    static const sf::Color col_core(255, 255, 255);
    static const sf::Color col_inner(220, 220, 255);
    static const sf::Color col_middle(160, 160, 240);
    static const sf::Color col_outer(100, 100, 200);
    static const sf::Color col_thin(200, 200, 255);

    // Compute the line from src to dest
    float line_dx = static_cast<float>(dx - sx);
    float line_dy = static_cast<float>(dy - sy);
    float line_len = std::sqrt(line_dx * line_dx + line_dy * line_dy);
    if (line_len < 1.0f)
        return;

    // Perpendicular direction for offsets
    float perp_x = -line_dy / line_len;
    float perp_y = line_dx / line_len;

    // Generate jagged points along the bolt using random perpendicular displacement
    int num_segments = std::clamp(static_cast<int>(line_len / params.segment_size), 6, 80);
    float max_offset = std::clamp(line_len * params.offset_pct, params.offset_min, params.offset_cap);

    int32_t prev_px = sx;
    int32_t prev_py = sy;

    for (int i = 1; i <= num_segments; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(num_segments);

        // Point on the straight line
        float base_x = static_cast<float>(sx) + line_dx * t;
        float base_y = static_cast<float>(sy) + line_dy * t;

        // Perpendicular offset: large in the middle, zero at endpoints
        float envelope = std::sin(t * 3.14159f);
        float offset = 0.0f;
        if (i < num_segments) // Last point snaps to destination
        {
            int32_t r = random_int(-100, 100);
            offset = max_offset * envelope * static_cast<float>(r + seed_offset * 15) / 120.0f;
            if (params.jag_chance > 0 && random_mod(params.jag_chance) == 0)
            {
                offset *= params.jag_multiplier;
            }
        }

        int32_t cur_px = static_cast<int32_t>(base_x + perp_x * offset);
        int32_t cur_py = static_cast<int32_t>(base_y + perp_y * offset);

        if (thick)
        {
            // Main bolt: bright core with glow layers
            rend.draw_line(prev_px, prev_py, cur_px, cur_py, col_core);
            rend.draw_line(prev_px - 1, prev_py, cur_px - 1, cur_py, col_inner);
            rend.draw_line(prev_px + 1, prev_py, cur_px + 1, cur_py, col_inner);
            rend.draw_line(prev_px, prev_py - 1, cur_px, cur_py - 1, col_inner);
            rend.draw_line(prev_px, prev_py + 1, cur_px, cur_py + 1, col_inner);
            rend.draw_line(prev_px - 2, prev_py, cur_px - 2, cur_py, col_middle);
            rend.draw_line(prev_px + 2, prev_py, cur_px + 2, cur_py, col_middle);
            rend.draw_line(prev_px, prev_py - 2, cur_px, cur_py - 2, col_middle);
            rend.draw_line(prev_px, prev_py + 2, cur_px, cur_py + 2, col_middle);
            rend.draw_line(prev_px - 2, prev_py - 1, cur_px - 2, cur_py - 1, col_outer);
            rend.draw_line(prev_px + 2, prev_py - 1, cur_px + 2, cur_py - 1, col_outer);
            rend.draw_line(prev_px + 2, prev_py + 1, cur_px + 2, cur_py + 1, col_outer);
            rend.draw_line(prev_px - 2, prev_py + 1, cur_px - 2, cur_py + 1, col_outer);
        }
        else
        {
            rend.draw_line(prev_px, prev_py, cur_px, cur_py, col_thin);
        }

        prev_px = cur_px;
        prev_py = cur_py;
    }
}

void effect_system::render_thunder(renderer& rend, float sx, float sy, float dx, float dy, int8_t rx, int8_t ry)
{
    int32_t x0 = static_cast<int32_t>(sx);
    int32_t y0 = static_cast<int32_t>(sy);
    int32_t x1 = static_cast<int32_t>(dx);
    int32_t y1 = static_cast<int32_t>(dy);

    draw_thunder_bolt(rend, x0, y0, x1, y1, static_cast<int32_t>(rx), true, thunder_params_);

    // Thin companion bolts
    static constexpr int32_t companion_seeds[] = {3, -2, 5, -4};
    for (int32_t i = 0; i < thunder_params_.thin_bolt_count && i < 4; ++i)
    {
        int32_t seed =
            (i == 0) ? static_cast<int32_t>(rx) + companion_seeds[i] : static_cast<int32_t>(ry) + companion_seeds[i];
        draw_thunder_bolt(rend, x0, y0, x1, y1, seed, false, thunder_params_);
    }
}

void effect_system::render_overlay(
    renderer& rend, const effect& eff, const effect_definition::sprite_overlay& ov, int32_t screen_x, int32_t screen_y)
{
    if (ov.pak_index >= max_effect_sprites)
        return;
    auto* spr = effect_sprites_[ov.pak_index];
    if (!spr)
        return;

    uint8_t fo = ov.frame_offset != 0 ? ov.frame_offset : eff.def->frame_offset;
    uint32_t frame = static_cast<uint32_t>(eff.current_frame) + fo;
    int32_t ox = screen_x + ov.offset_x;
    int32_t oy = screen_y + ov.offset_y;

    switch (ov.render_mode)
    {
    case effect_render_mode::normal:
    case effect_render_mode::transparent:
        rend.draw_sprite_additive(*spr, ox, oy, frame);
        break;
    case effect_render_mode::alpha_50:
        rend.draw_sprite_additive_alpha(*spr, ox, oy, frame, 0.5f);
        break;
    default:
        rend.draw_sprite_additive(*spr, ox, oy, frame);
        break;
    }
}

} // namespace hb
