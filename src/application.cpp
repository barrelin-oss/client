#include "application.hpp"
#include "platform/monitor.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <thread>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#endif

namespace hb {

int application::run() {
    // Initialize async logging - log I/O runs on a background thread so it
    // never blocks the main thread (important during loading screen rendering)
    try {
        spdlog::init_thread_pool(8192, 1);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("helbreath.log", true);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::async_logger>(
            "main", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::warn);
    } catch (const spdlog::spdlog_ex&) {
        // Fall back to sync console only
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
    }

    spdlog::info("========================================");
    spdlog::info("Helbreath Client v{}.{}", version_major, version_minor);
    spdlog::info("========================================");

    if (!initialize()) {
        spdlog::error("Failed to initialize application");
        return 1;
    }

    main_loop();
    shutdown();

    spdlog::info("Application exited normally");
    spdlog::shutdown();
    return 0;
}

bool application::initialize() {
    spdlog::info("Initializing subsystems...");

    // Load configuration first
    load_config();

    // Initialize localization
    if (!localization_.initialize(config::instance().game().ui_language)) {
        spdlog::error("Failed to initialize localization");
        return false;
    }

    // Load additional language files if they exist
    localization_.load_language(language::korean, "assets/strings/ko.json");
    localization_.load_language(language::japanese, "assets/strings/ja.json");
    localization_.load_language(language::chinese_simplified, "assets/strings/zh_cn.json");
    localization_.load_language(language::chinese_traditional, "assets/strings/zh_tw.json");

    // Initialize renderer
    auto& video_cfg = config::instance().video();
    int32_t monitor_x = 0, monitor_y = 0;
    if (video_cfg.borderless) {
        // Find the target monitor's position for borderless mode
        auto monitors = hb::enumerate_monitors();
        for (const auto& m : monitors) {
            if (m.index == video_cfg.monitor_index) {
                monitor_x = m.x;
                monitor_y = m.y;
                // Override resolution to match monitor native res
                video_cfg.screen_width = static_cast<uint32_t>(m.width);
                video_cfg.screen_height = static_cast<uint32_t>(m.height);
                break;
            }
        }
    }
    if (!renderer_.initialize(video_cfg.screen_width, video_cfg.screen_height,
                              video_cfg.fullscreen, video_cfg.borderless, monitor_x, monitor_y)) {
        spdlog::error("Failed to initialize renderer");
        return false;
    }

    // Apply view mode preferences from config
    renderer_.set_aspect_mode(static_cast<aspect_mode>(video_cfg.aspect_mode));
    renderer_.set_scale_filter(static_cast<scale_filter>(video_cfg.scale_filter));
    renderer_.set_ui_scale(video_cfg.ui_scale);

    // Load font for text rendering
    if (!renderer_.load_font("assets/fonts/OpenSans-Regular.ttf")) {
        spdlog::warn("Failed to load font - text rendering will be disabled");
    }

    // Initialize audio
    auto& audio_cfg = config::instance().audio();
    if (!audio_.initialize()) {
        spdlog::warn("Failed to initialize audio - continuing without sound");
    } else {
        audio_.set_master_volume(audio_cfg.master_volume);
        audio_.set_music_volume(audio_cfg.music_volume);
        audio_.set_sound_volume(audio_cfg.sfx_volume);
        audio_.set_muted(audio_cfg.muted);
    }

    // Initialize chat system
    chat_.initialize();

    // Apply chat config
    auto& chat_cfg = config::instance().chat();
    chat_config cfg;
    cfg.max_history = chat_cfg.max_history;
    cfg.max_message_length = chat_cfg.max_message_length;
    cfg.show_timestamps = chat_cfg.show_timestamps;
    cfg.filter_profanity = chat_cfg.filter_profanity;
    cfg.block_spam = chat_cfg.block_spam;
    cfg.spam_delay = chat_cfg.spam_delay;
    cfg.show_normal = chat_cfg.show_normal;
    cfg.show_shout = chat_cfg.show_shout;
    cfg.show_whisper = chat_cfg.show_whisper;
    cfg.show_guild = chat_cfg.show_guild;
    cfg.show_party = chat_cfg.show_party;
    cfg.show_system = chat_cfg.show_system;
    cfg.show_trade = chat_cfg.show_trade;
    cfg.show_global = chat_cfg.show_global;
    chat_.set_config(cfg);

    // Initialize game state manager
    game_state_ = std::make_unique<game_state_manager>();
    if (!game_state_->initialize(renderer_, audio_)) {
        spdlog::error("Failed to initialize game state manager");
        return false;
    }

    // Preload a random character body sprite for loading screen animation
    {
        auto& sprites = game_state_->sprites();
        bool use_female = (std::rand() % 2) != 0;
        const char* pak_name = use_female ? "Yw" : "Ym";
        // Ym = body_base(500) + (3-1)*120 = 740, Yw = 500 + (6-1)*120 = 1100
        uint16_t sprite_base = use_female ? 1100 : 740;

        std::string pak_path = std::string("sprites/") + pak_name + ".pak";
        if (sprites.load_pak(pak_name, pak_path)) {
            for (uint16_t i = 0; i < 120; ++i) {
                sprites.store_sprite_at_id(static_cast<uint16_t>(sprite_base + i), pak_name, i);
            }
            // Run animation facing right (east=dir 3): action(4)*8 + (dir-1)=2 = index 34
            loading_char_sprite_ = sprites.get_sprite_by_id(static_cast<uint16_t>(sprite_base + 34));
            if (loading_char_sprite_) {
                spdlog::info("Loading screen character: {} (sprite {})", pak_name, sprite_base + 18);
            }
        }
    }

    // Run loading screen while processing initialization steps (one step per frame)
    {
        auto load_start = clock::now();
        float displayed_progress = 0.0f;
        float target_progress = 0.0f;
        std::string current_message = "Initializing...";

        while (game_state_->has_pending_init_steps() || displayed_progress < 0.999f) {
            while (auto event = renderer_.window().pollEvent())
                input_.process_event(*event);

            if (game_state_->has_pending_init_steps()) {
                auto [p, msg] = game_state_->run_next_init_step();
                target_progress = p;
                current_message = std::move(msg);
            }

            displayed_progress += (target_progress - displayed_progress) * 0.15f;
            if (target_progress - displayed_progress < 0.003f)
                displayed_progress = target_progress;

            float elapsed = std::chrono::duration<float>(clock::now() - load_start).count();
            render_loading_frame(displayed_progress, current_message, elapsed);
        }
    }

    // Wire up software cursor to game sprites
    cursor_.set_sprite_manager(&game_state_->sprites());

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Initialize debug overlay for UI positioning
    debug::debug_overlay::instance().initialize("config/ui_positions.json");
    spdlog::info("Debug overlay available (F11 to toggle)");
#endif

    // Setup chat callbacks
    chat_callbacks chat_cb;
    chat_cb.on_message_received = [](const chat_message& msg) {
        spdlog::debug("Chat [{}]: {}", msg.sender, msg.content);
    };
    chat_.set_callbacks(chat_cb);

    // Start running
    running_ = true;
    last_frame_time_ = clock::now();

    spdlog::info("Initialization complete");
    return true;
}

void application::shutdown() {
    spdlog::info("Shutting down...");

    // Save window position before closing
    auto& video = config::instance().video();
    if (video.remember_position && renderer_.is_open() && !video.fullscreen) {
        auto pos = renderer_.window().getPosition();
        video.window_x = pos.x;
        video.window_y = pos.y;
    }

    // Save configuration
    save_config();

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Shutdown debug overlay (saves positions if modified)
    debug::debug_overlay::instance().shutdown();
#endif

    // Shutdown in reverse order
    if (game_state_) {
        game_state_->shutdown();
        game_state_.reset();
    }

    chat_.shutdown();
    audio_.shutdown();
    renderer_.shutdown();
    localization_.shutdown();

    spdlog::info("Shutdown complete");
}

void application::main_loop() {
    while (running_ && renderer_.is_open()) {
        // Calculate delta time
        auto current_time = clock::now();
        auto delta = std::chrono::duration<float>(current_time - last_frame_time_);
        float delta_time = delta.count();
        last_frame_time_ = current_time;

        // FPS calculation
        frame_count_++;
        fps_timer_ += delta_time;
        if (fps_timer_ >= 1.0f) {
            fps_ = static_cast<float>(frame_count_) / fps_timer_;
            frame_count_ = 0;
            fps_timer_ = 0.0f;
        }

        // Cap delta time to prevent spiral of death
        if (delta_time > 0.25f) {
            delta_time = 0.25f;
        }

        // Process input events
        process_events();

        // Check for close request
        if (input_.should_close()) {
            running_ = false;
            continue;
        }

        // Update game logic
        update(delta_time);

        // Check if game wants to quit
        if (game_state_ && game_state_->current_state() == game_state::quit) {
            running_ = false;
            continue;
        }

        // Render
        render();

        // Reset per-frame input state
        input_.end_frame();

        // Update audio (clean up finished sounds, process fades)
        audio_.update(delta_time);
    }
}

void application::process_events() {
    while (auto event = renderer_.window().pollEvent()) {
        input_.process_event(*event);
    }

    // Track focus changes for borderless topmost management
    bool has_focus = input_.has_focus();
    if (has_focus != had_focus_) {
        had_focus_ = has_focus;
        renderer_.on_focus_changed(has_focus);
    }

    // Handle global hotkeys
    if (input_.is_key_pressed(sf::Keyboard::Key::F12)) {
        // Screenshot
        spdlog::info("Screenshot requested");
        // TODO: Implement screenshot
    }
}

void application::update(float delta_time) {
    // Update chat system
    chat_.update(delta_time);

    // Update game state
    if (game_state_) {
        game_state_->update(delta_time, input_);
    }
}

void application::render() {
    renderer_.begin_frame();
    cursor_.begin_frame();

    // Render game state
    if (game_state_) {
        game_state_->render(renderer_);
    }

    // Show FPS if enabled
    if (config::instance().video().show_fps) {
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_));
        renderer_.draw_text(fps_text, 5, 5, sf::Color::Yellow);
    }

    // Software cursor - always drawn last to guarantee visibility on top of everything
    cursor_.render(renderer_, input_.mouse_x(), input_.mouse_y());

    renderer_.end_frame();
}

void application::render_loading_frame(float progress, std::string_view message, float elapsed_time) {
    const auto sw = static_cast<int32_t>(renderer_.width());
    const auto sh = static_cast<int32_t>(renderer_.height());

    renderer_.begin_frame();

    // Background - dark navy
    renderer_.draw_rect(0, 0, sw, sh, sf::Color(8, 8, 18));

    // Stars - precomputed positions with twinkling alpha
    struct star { float x; float y; float offset; float size; };
    static const std::array<star, 40> stars = []() {
        std::array<star, 40> s{};
        // Pseudo-random but deterministic positions using a simple hash
        for (size_t i = 0; i < 40; ++i)
        {
            uint32_t h = static_cast<uint32_t>(i) * 2654435761u;
            s[i].x = static_cast<float>(h % 1000u) / 1000.0f;
            h = h * 2246822519u + 3266489917u;
            s[i].y = static_cast<float>(h % 1000u) / 1000.0f;
            h = h * 374761393u;
            s[i].offset = static_cast<float>(h % 628u) / 100.0f; // 0-6.28
            s[i].size = (h % 2u == 0) ? 1.0f : 2.0f;
        }
        return s;
    }();

    for (const auto& s : stars)
    {
        float alpha_f = 80.0f + 60.0f * std::sin(elapsed_time * 1.8f + s.offset);
        auto alpha = static_cast<uint8_t>(std::clamp(alpha_f, 0.0f, 255.0f));
        int32_t sx = static_cast<int32_t>(s.x * static_cast<float>(sw));
        int32_t sy = static_cast<int32_t>(s.y * static_cast<float>(sh));
        int32_t sz = static_cast<int32_t>(s.size);
        renderer_.draw_rect(sx, sy, sz, sz, sf::Color(255, 255, 255, alpha));
    }

    // Title - "HELBREATH" above, "XTREME" below with fire effect
    {
        float pulse = 200.0f + 55.0f * std::sin(elapsed_time * 1.5f);
        auto title_alpha = static_cast<uint8_t>(std::clamp(pulse, 0.0f, 255.0f));

        // "HELBREATH" - upper line, white with gentle pulse
        constexpr int32_t hb_size = 26;
        float hb_w = renderer_.text().measure_width("HELBREATH", hb_size);
        int32_t hb_x = (sw - static_cast<int32_t>(hb_w)) / 2;
        int32_t hb_y = sh * 28 / 100;
        renderer_.draw_text_outlined("HELBREATH", hb_x, hb_y,
                                     sf::Color(255, 255, 255, title_alpha),
                                     sf::Color(0, 0, 0, title_alpha), hb_size, 2.0f);

        // "XTREME" - below, on fire
        constexpr int32_t xt_size = 38;
        float xt_w = renderer_.text().measure_width("XTREME", xt_size);
        int32_t xt_x = (sw - static_cast<int32_t>(xt_w)) / 2;
        int32_t xt_y = hb_y + hb_size + 8;

        // Warm glow halo - text-shaped outlines so glow follows the letters
        {
            float glow_pulse = 0.7f + 0.3f * std::sin(elapsed_time * 2.0f);
            struct glow_ring { float thickness; uint8_t r, g, b; float base_alpha; };
            static constexpr std::array<glow_ring, 5> rings = {{
                {18.0f, 120,  10,   0,  10.0f},
                {14.0f, 170,  30,   0,  16.0f},
                {10.0f, 220,  70,   0,  22.0f},
                { 6.0f, 245, 130,  10,  30.0f},
                { 3.0f, 255, 180,  40,  40.0f},
            }};
            for (const auto& g : rings) {
                auto a = static_cast<uint8_t>(g.base_alpha * glow_pulse);
                renderer_.draw_text_outlined("XTREME", xt_x, xt_y,
                                             sf::Color(g.r, g.g, g.b, a),
                                             sf::Color(g.r, g.g, g.b, static_cast<uint8_t>(a / 2)),
                                             xt_size, g.thickness);
            }
        }

        // Ember particles rising from text
        struct ember { float x; float speed; float phase; float size; };
        static const std::array<ember, 48> embers = []() {
            std::array<ember, 48> e{};
            for (size_t i = 0; i < 48; ++i) {
                uint32_t h = static_cast<uint32_t>(i) * 2654435761u;
                e[i].x = static_cast<float>(h % 1000u) / 1000.0f;
                h = h * 2246822519u;
                e[i].speed = 20.0f + static_cast<float>(h % 40u);
                h = h * 374761393u;
                e[i].phase = static_cast<float>(h % 628u) / 100.0f;
                e[i].size = 1.0f + static_cast<float>(h % 3u);
            }
            return e;
        }();

        for (const auto& e : embers) {
            float cycle = std::fmod(elapsed_time * e.speed / 50.0f + e.phase, 1.0f);
            float rise_height = 80.0f + e.size * 15.0f;
            float ey = static_cast<float>(xt_y + xt_size / 2) - cycle * rise_height;
            float ex = static_cast<float>(xt_x) + e.x * xt_w
                       + 5.0f * std::sin(elapsed_time * 3.0f + e.phase);
            float ea = (1.0f - cycle) * 220.0f;
            uint8_t er = 255;
            uint8_t eg = static_cast<uint8_t>(std::clamp(240.0f * (1.0f - cycle * 0.7f), 0.0f, 255.0f));
            uint8_t eb = static_cast<uint8_t>(std::clamp(100.0f * (1.0f - cycle), 0.0f, 255.0f));
            int32_t esz = static_cast<int32_t>(e.size * (1.0f - cycle * 0.5f));
            if (esz < 1) esz = 1;
            renderer_.draw_rect(static_cast<int32_t>(ex), static_cast<int32_t>(ey),
                                esz, esz,
                                sf::Color(er, eg, eb, static_cast<uint8_t>(std::clamp(ea, 0.0f, 255.0f))));
        }

        // Fire glow layers behind text (back to front, rising upward)
        struct fire_layer { float y_off; float speed; float amp; uint8_t r, g, b, a; };
        static constexpr std::array<fire_layer, 9> flames = {{
            {-16.0f, 2.2f, 7.0f, 100,   5,   0,  25},
            {-12.0f, 2.8f, 6.0f, 140,  15,   0,  40},
            { -9.0f, 3.5f, 5.0f, 180,  35,   0,  55},
            { -6.0f, 4.2f, 4.0f, 210,  60,   0,  75},
            { -4.0f, 5.0f, 3.2f, 235, 100,   0, 100},
            { -2.5f, 3.8f, 2.5f, 250, 145,  10, 130},
            { -1.0f, 4.5f, 1.8f, 255, 190,  30, 160},
            {  0.0f, 2.5f, 1.2f, 255, 220,  60, 190},
            {  1.0f, 3.2f, 0.8f, 255, 240, 100, 140},
        }};

        for (const auto& f : flames) {
            float wobble = f.amp * std::sin(elapsed_time * f.speed + f.y_off * 0.7f);
            float x_wobble = (f.amp * 0.3f) * std::cos(elapsed_time * f.speed * 0.7f + f.y_off);
            int32_t fy = xt_y + static_cast<int32_t>(f.y_off + wobble);
            int32_t fx = xt_x + static_cast<int32_t>(x_wobble);
            renderer_.draw_text_outlined("XTREME", fx, fy,
                                         sf::Color(f.r, f.g, f.b, f.a),
                                         sf::Color(0, 0, 0, 0), xt_size, 0.0f);
        }

        // Main XTREME text - bright fire core with dark outline
        renderer_.draw_text_outlined("XTREME", xt_x, xt_y,
                                     sf::Color(255, 230, 80),
                                     sf::Color(160, 30, 0), xt_size, 2.5f);

        // Hot white-yellow pulsing overlay
        float hot = 140.0f + 100.0f * std::sin(elapsed_time * 3.5f);
        renderer_.draw_text_outlined("XTREME", xt_x, xt_y,
                                     sf::Color(255, 255, 200, static_cast<uint8_t>(std::clamp(hot, 0.0f, 240.0f))),
                                     sf::Color(0, 0, 0, 0), xt_size, 0.0f);

        // Secondary hot-white flash (faster, out of phase)
        float hot2 = 60.0f + 60.0f * std::sin(elapsed_time * 5.5f + 1.5f);
        renderer_.draw_text_outlined("XTREME", xt_x, xt_y,
                                     sf::Color(255, 255, 255, static_cast<uint8_t>(std::clamp(hot2, 0.0f, 120.0f))),
                                     sf::Color(0, 0, 0, 0), xt_size, 0.0f);
    }

    // Progress bar area - centered at ~55% height
    constexpr int32_t bar_w = 420;
    constexpr int32_t bar_h = 16;
    int32_t bar_x = (sw - bar_w) / 2;
    int32_t bar_y = sh * 55 / 100;

    // Decorative lines above and below bar
    sf::Color line_color(180, 150, 80, 100);
    int32_t line_margin = 20;
    renderer_.draw_line(bar_x - line_margin, bar_y - 10,
                        bar_x + bar_w + line_margin, bar_y - 10, line_color);
    renderer_.draw_line(bar_x - line_margin, bar_y + bar_h + 10,
                        bar_x + bar_w + line_margin, bar_y + bar_h + 10, line_color);

    // Bar background
    renderer_.draw_rect(bar_x, bar_y, bar_w, bar_h, sf::Color(25, 25, 40));

    // Bar fill
    float clamped = std::clamp(progress, 0.0f, 1.0f);
    int32_t fill_w = static_cast<int32_t>(static_cast<float>(bar_w) * clamped);
    if (fill_w > 0)
    {
        // Main fill
        renderer_.draw_rect(bar_x, bar_y, fill_w, bar_h, sf::Color(70, 120, 180));
        // Brighter top half overlay for depth
        renderer_.draw_rect(bar_x, bar_y, fill_w, bar_h / 2, sf::Color(90, 140, 200, 100));

        // Shimmer stripe sliding across fill
        float shimmer_speed = 120.0f;
        float shimmer_w = 50.0f;
        float shimmer_pos = std::fmod(elapsed_time * shimmer_speed, static_cast<float>(fill_w) + shimmer_w) - shimmer_w;
        int32_t sx1 = bar_x + static_cast<int32_t>(std::max(shimmer_pos, 0.0f));
        int32_t sx2 = bar_x + static_cast<int32_t>(std::min(shimmer_pos + shimmer_w, static_cast<float>(fill_w)));
        if (sx2 > sx1)
        {
            renderer_.draw_rect(sx1, bar_y, sx2 - sx1, bar_h, sf::Color(180, 210, 255, 80));
        }
    }

    // Running character following the progress bar fill edge
    if (loading_char_sprite_ && fill_w > 0)
    {
        // 8-frame run cycle, ~42ms per frame (336ms run duration / 8 frames)
        uint32_t anim_frame = static_cast<uint32_t>(elapsed_time / 0.042f) % 8;
        // Character feet at the top of the bar, positioned at the fill edge
        int32_t char_x = bar_x + fill_w;
        int32_t char_y = bar_y;
        renderer_.draw_sprite(*loading_char_sprite_, char_x, char_y, anim_frame);
    }

    // Bar border
    renderer_.draw_rect(bar_x - 1, bar_y - 1, bar_w + 2, 1, sf::Color(60, 60, 90));       // top
    renderer_.draw_rect(bar_x - 1, bar_y + bar_h, bar_w + 2, 1, sf::Color(60, 60, 90));    // bottom
    renderer_.draw_rect(bar_x - 1, bar_y, 1, bar_h, sf::Color(60, 60, 90));                 // left
    renderer_.draw_rect(bar_x + bar_w, bar_y, 1, bar_h, sf::Color(60, 60, 90));             // right

    // Percentage text centered on bar
    int pct = static_cast<int>(clamped * 100.0f);
    std::string pct_text = std::to_string(pct) + "%";
    float pct_w = renderer_.text().measure_width(pct_text, 11);
    int32_t pct_x = bar_x + (bar_w - static_cast<int32_t>(pct_w)) / 2;
    int32_t pct_y = bar_y + (bar_h - 11) / 2;
    renderer_.draw_text_outlined(pct_text, pct_x, pct_y,
                                 sf::Color::White, sf::Color::Black, 11, 1.0f);

    // Status message below bar
    {
        float msg_w = renderer_.text().measure_width(message, 12);
        int32_t msg_x = (sw - static_cast<int32_t>(msg_w)) / 2;
        int32_t msg_y = bar_y + bar_h + 20;
        renderer_.draw_text_outlined(message, msg_x, msg_y,
                                     sf::Color(160, 160, 170), sf::Color::Black, 12, 1.0f);
    }

    // Loading dots at bottom
    {
        int dot_count = static_cast<int>(elapsed_time / 0.5f) % 3 + 1;
        std::string loading_text = "Loading";
        for (int i = 0; i < dot_count; ++i) loading_text += '.';

        float load_w = renderer_.text().measure_width(loading_text, 11);
        int32_t load_x = (sw - static_cast<int32_t>(load_w)) / 2;
        int32_t load_y = sh - 40;
        renderer_.draw_text_outlined(loading_text, load_x, load_y,
                                     sf::Color(120, 120, 130), sf::Color::Black, 11, 1.0f);
    }

    renderer_.end_frame();
}

void application::load_config() {
    auto& cfg = config::instance();
    cfg.initialize();

    if (!cfg.load("config.json")) {
        spdlog::info("No config file found, using defaults");
        cfg.save("config.json");
    }
}

void application::save_config() {
    config::instance().save();
}

void application::apply_config() {
    auto& cfg = config::instance();

    // Apply audio settings
    auto& audio_cfg = cfg.audio();
    audio_.set_master_volume(audio_cfg.master_volume);
    audio_.set_music_volume(audio_cfg.music_volume);
    audio_.set_sound_volume(audio_cfg.sfx_volume);
    audio_.set_muted(audio_cfg.muted);

    // Apply language
    localization_.set_language(cfg.game().ui_language);

    // Notify of changes
    cfg.notify_changed();
}

} // namespace hb
