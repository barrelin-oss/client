#pragma once

#include "assets/sprite_manager.hpp"
#include "assets/tile_sprite_registry.hpp"
#include "audio/audio.hpp"
#include "audio/sound_manager.hpp"
#include "gameplay/effect_system.hpp"
#include "gameplay/magic.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "world/world.hpp"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hb {

class effect_test_app
{
public:
    effect_test_app() = default;
    ~effect_test_app() = default;

    effect_test_app(const effect_test_app&) = delete;
    effect_test_app& operator=(const effect_test_app&) = delete;

    bool initialize();
    void run();
    void shutdown();

private:
    void process_events();
    void update(float delta_time);
    void render();
    void render_tile_markers();
    void render_controls();
    void render_info_overlay();

    void trigger_effect();
    void cycle_spell(int direction);

    // Systems
    renderer renderer_;
    input input_;
    audio audio_;
    sound_manager sounds_;
    sprite_manager sprites_;
    tile_sprite_registry tile_registry_;
    world world_;
    effect_system effects_;
    magic_system magic_;

    bool running_ = false;
    bool effect_in_progress_ = false;

    // Tile selection
    std::optional<std::pair<int32_t, int32_t>> source_tile_;
    std::optional<std::pair<int32_t, int32_t>> dest_tile_;

    // Spell selection
    std::vector<const spell*> all_spells_;
    int32_t spell_index_ = 0;

    // Settings
    bool use_additive_blend_ = true;
    float additive_intensity_ = 1.0f;
    float alpha_override_ = 1.0f;
    bool loop_mode_ = false;

    // Timing
    std::chrono::steady_clock::time_point last_frame_time_;
    float loop_delay_timer_ = 0.0f;
    static constexpr float loop_delay_ = 0.5f;
};

} // namespace hb
