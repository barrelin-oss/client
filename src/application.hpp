#pragma once

#include "audio/audio.hpp"
#include "core/constants.hpp"
#include "core/config.hpp"
#include "core/game_enums.hpp"
#include "core/launch_options.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "gameplay/game_state.hpp"
#include "localization/localization.hpp"
#include "chat/chat_system.hpp"
#include "ui/cursor.hpp"

#include <chrono>
#include <memory>
#include <string_view>

namespace hb
{
class sprite;
}

namespace hb
{

class application
{
public:
    application() = default;
    ~application() = default;

    application(const application&) = delete;
    application& operator=(const application&) = delete;

    // Main entry point - returns exit code
    int run(const launch_options& opts = {});

private:
    bool initialize(const launch_options& opts);
    void shutdown();
    void main_loop();

    void process_events();
    void update(float delta_time);
    void render();

    // Loading screen
    void render_loading_frame(float progress, std::string_view message, float elapsed_time);

    // Configuration
    void load_config();
    void save_config();
    void apply_config();

    // Subsystems (order matters for initialization/shutdown)
    renderer renderer_;
    input input_;
    audio audio_;
    localization localization_;
    chat_system chat_;
    cursor_manager cursor_;
    std::unique_ptr<game_state_manager> game_state_;

    // State
    bool running_ = false;
    bool had_focus_ = true; // Track focus changes for borderless topmost

    // Loading screen character
    const sprite* loading_char_sprite_ = nullptr;

    // Timing
    using clock = std::chrono::high_resolution_clock;
    clock::time_point last_frame_time_;
    float fps_ = 0.0f;
    float fps_timer_ = 0.0f;
    int frame_count_ = 0;
};

} // namespace hb
