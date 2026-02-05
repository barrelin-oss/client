#include "gameplay/effect_system.hpp"
#include "assets/sprite_manager.hpp"
#include "audio/sound_manager.hpp"
#include "world/world.hpp"
#include "world/tile.hpp"
#include "graphics/renderer.hpp"
#include "core/direction_utils.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <cstdlib>
#include <string>

namespace hb {

bool effect_system::initialize(sprite_manager& sprites, sound_manager& sounds, world& w)
{
    sprites_ = &sprites;
    sounds_ = &sounds;
    world_ = &w;

    // Try to load effect PAK files (graceful if missing)
    struct pak_entry
    {
        int index;
        const char* filename;
    };
    static constexpr pak_entry effect_paks[] = {
        {7,  "sprites/Effect7.pak"},
        {8,  "sprites/Effect8.pak"},
        {11, "sprites/Effect11.pak"},
        {12, "sprites/Effect12.pak"},
        {13, "sprites/Effect13.pak"},
    };

    int loaded = 0;
    for (const auto& pak : effect_paks)
    {
        std::string pak_name = "Effect" + std::to_string(pak.index);
        if (sprites_->load_pak(pak_name, pak.filename))
        {
            ++loaded;
            spdlog::debug("Loaded effect PAK: {}", pak.filename);
        }
        else
        {
            spdlog::debug("Effect PAK not found (will skip): {}", pak.filename);
        }
    }

    sprites_loaded_ = true;
    spdlog::info("Effect system initialized ({} PAK files loaded)", loaded);
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
    auto screen_w = static_cast<int32_t>(rend.width());
    auto screen_h = static_cast<int32_t>(rend.height());

    for (const auto& eff : effects_)
    {
        if (!eff.active || !eff.sprite_ptr)
        {
            continue;
        }

        // World to screen conversion
        auto screen_x = static_cast<int32_t>(eff.pos_x) - camera_x;
        auto screen_y = static_cast<int32_t>(eff.pos_y) - camera_y;

        // Visibility culling with generous margin
        constexpr int32_t margin = 128;
        if (screen_x < -margin || screen_x > screen_w + margin ||
            screen_y < -margin || screen_y > screen_h + margin)
        {
            continue;
        }

        // Calculate the frame to render
        uint32_t frame = static_cast<uint32_t>(eff.current_frame);

        // For directional effects, offset frame by direction
        if (eff.def->directional)
        {
            frame += static_cast<uint32_t>(eff.direction_index) * (eff.def->max_frames + 1);
        }

        // Render based on mode
        switch (eff.def->render_mode)
        {
            case effect_render_mode::normal:
                rend.draw_sprite(*eff.sprite_ptr, screen_x, screen_y, frame);
                break;

            case effect_render_mode::transparent:
                rend.draw_sprite(*eff.sprite_ptr, screen_x, screen_y, frame);
                break;

            case effect_render_mode::alpha_25:
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, 0.25f);
                break;

            case effect_render_mode::alpha_50:
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, 0.5f);
                break;

            case effect_render_mode::alpha_70:
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, 0.7f);
                break;

            case effect_render_mode::fade:
            {
                // Fade out over the effect lifetime
                float total_time = static_cast<float>(eff.def->max_frames) *
                                   static_cast<float>(eff.def->frame_time_ms) / 1000.0f;
                float alpha = 1.0f;
                if (total_time > 0.0f)
                {
                    alpha = 1.0f - (eff.elapsed / total_time);
                    if (alpha < 0.0f) alpha = 0.0f;
                }
                rend.draw_sprite_alpha(*eff.sprite_ptr, screen_x, screen_y, frame, alpha);
                break;
            }
        }
    }
}

void effect_system::add_effect(effect_type_id type_id,
                                int32_t src_x, int32_t src_y,
                                int32_t dest_x, int32_t dest_y,
                                int8_t start_frame, int32_t value)
{
    const auto* def = get_effect_definition(type_id);
    if (!def)
    {
        spdlog::debug("Unknown effect type: {}", static_cast<uint16_t>(type_id));
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

    // Convert tile coords to world pixels if needed
    float fx_src, fy_src, fx_dest, fy_dest;
    if (def->uses_tile_coords)
    {
        fx_src = static_cast<float>(src_x * tile_width);
        fy_src = static_cast<float>(src_y * tile_height);
        fx_dest = static_cast<float>(dest_x * tile_width);
        fy_dest = static_cast<float>(dest_y * tile_height);
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

void effect_system::init_effect(effect& eff, const effect_definition& def,
                                 float src_x, float src_y,
                                 float dest_x, float dest_y,
                                 int8_t start_frame, int32_t value)
{
    eff.clear();
    eff.def = &def;
    eff.active = true;
    eff.current_frame = start_frame;
    eff.max_frame = static_cast<int8_t>(def.max_frames);
    eff.value = value;

    eff.src_x = src_x;
    eff.src_y = src_y;
    eff.dest_x = dest_x;
    eff.dest_y = dest_y;

    // Position starts at source (with height offset for tile-based effects)
    eff.pos_x = src_x;
    eff.pos_y = src_y + static_cast<float>(def.height_offset);
    eff.prev_x = eff.pos_x;
    eff.prev_y = eff.pos_y;

    // Calculate direction for directional effects
    if (def.directional || def.behavior == effect_behavior::projectile)
    {
        auto dir = calculate_direction(
            static_cast<int32_t>(src_x), static_cast<int32_t>(src_y),
            static_cast<int32_t>(dest_x), static_cast<int32_t>(dest_y));
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
        eff.velocity_x = static_cast<float>((std::rand() % 11) - 5);
        eff.velocity_y = static_cast<float>(-(std::rand() % 8) - 2);
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
            eff.active = false;
            return;
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
            // Reached destination
            eff.active = false;
            return;
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

        if (eff.current_frame > eff.max_frame)
        {
            eff.active = false;
            return;
        }

        // Check if any children should spawn at this frame
        spawn_children(eff, *eff.def, old_frame);
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

    // Take multiple steps per frame for faster projectiles
    constexpr int steps_per_frame = 4;
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

        if (child_spec.trigger_frame != trigger_frame)
        {
            continue;
        }

        for (int8_t c = 0; c < child_spec.count; ++c)
        {
            float offset_x = static_cast<float>(child_spec.offset_x);
            float offset_y = static_cast<float>(child_spec.offset_y);

            if (child_spec.random_offset)
            {
                offset_x += static_cast<float>((std::rand() % (child_spec.random_range * 2 + 1)) - child_spec.random_range);
                offset_y += static_cast<float>((std::rand() % (child_spec.random_range * 2 + 1)) - child_spec.random_range);
            }

            float child_x = parent.pos_x + offset_x;
            float child_y = parent.pos_y + offset_y;

            add_effect_at_pixel(child_spec.type, child_x, child_y);
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
    if (!sprites_)
    {
        return nullptr;
    }

    // Check cache first
    uint8_t idx = def.sprite_pak_index;
    if (idx < max_effect_sprites && effect_sprites_[idx])
    {
        return effect_sprites_[idx];
    }

    // Try to load from the Effect PAK file
    std::string pak_name = "Effect" + std::to_string(idx);
    auto* spr = sprites_->get_sprite(pak_name, 0);

    if (spr && idx < max_effect_sprites)
    {
        effect_sprites_[idx] = spr;
    }

    if (!spr)
    {
        spdlog::debug("No sprite for effect PAK index {} ({})", idx, pak_name);
    }

    return spr;
}

} // namespace hb
