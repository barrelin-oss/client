#include "application.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <thread>

namespace hb {

int application::run() {
    // Initialize logging with both console and file output
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("helbreath.log", true);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::warn);
    } catch (const spdlog::spdlog_ex&) {
        // Fall back to console only
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
    if (!renderer_.initialize(video_cfg.screen_width, video_cfg.screen_height, video_cfg.fullscreen)) {
        spdlog::error("Failed to initialize renderer");
        return false;
    }

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

    // Save configuration
    save_config();

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
    auto& video_cfg = config::instance().video();

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

        // Render
        render();

        // Reset per-frame input state
        input_.end_frame();

        // Update audio (clean up finished sounds)
        audio_.update();

        // Frame rate limiting (if vsync is off)
        if (!video_cfg.vsync && video_cfg.framerate_limit > 0) {
            float target_frame_time = 1.0f / static_cast<float>(video_cfg.framerate_limit);
            auto frame_end = clock::now();
            auto elapsed = std::chrono::duration<float>(frame_end - current_time).count();

            if (elapsed < target_frame_time) {
                auto sleep_time = std::chrono::duration<float>(target_frame_time - elapsed);
                std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time));
            }
        }
    }
}

void application::process_events() {
    while (auto event = renderer_.window().pollEvent()) {
        input_.process_event(*event);
    }

    // Handle global hotkeys
    if (input_.is_key_pressed(sf::Keyboard::Key::F11)) {
        // Toggle fullscreen
        auto& video_cfg = config::instance().video();
        video_cfg.fullscreen = !video_cfg.fullscreen;
        // Would need to recreate window for fullscreen toggle
        spdlog::info("Fullscreen toggle: {}", video_cfg.fullscreen);
    }

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

    // Render game state
    if (game_state_) {
        game_state_->render(renderer_);
    }

    // Show FPS if enabled
    if (config::instance().video().show_fps) {
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_));
        renderer_.draw_text(fps_text, 5, 5, sf::Color::Yellow);
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
    audio_.set_muted(audio_cfg.muted);

    // Apply language
    localization_.set_language(cfg.game().ui_language);

    // Notify of changes
    cfg.notify_changed();
}

} // namespace hb
