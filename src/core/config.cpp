#include "core/config.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

namespace hb {

config& config::instance() {
    static config inst;
    return inst;
}

void config::initialize() {
    reset_all();
}

void config::reset_video() {
    video_ = video_config{};
}

void config::reset_audio() {
    audio_ = audio_config{};
}

void config::reset_network() {
    network_ = network_config{};
}

void config::reset_chat() {
    chat_ = chat_config_settings{};
}

void config::reset_game() {
    game_ = game_config{};
}

void config::reset_controls() {
    controls_ = control_config{};
}

void config::reset_all() {
    reset_video();
    reset_audio();
    reset_network();
    reset_chat();
    reset_game();
    reset_controls();
}

bool config::load(std::string_view path) {
    std::string path_str{path};
    std::ifstream file{path_str};
    if (!file) {
        spdlog::warn("Config file not found: {}, using defaults", path);
        return false;
    }

    try {
        nlohmann::json json;
        file >> json;

        // Video settings
        if (json.contains("video")) {
            auto& v = json["video"];
            if (v.contains("screen_width")) video_.screen_width = v["screen_width"].get<uint32_t>();
            if (v.contains("screen_height")) video_.screen_height = v["screen_height"].get<uint32_t>();
            if (v.contains("fullscreen")) video_.fullscreen = v["fullscreen"].get<bool>();
            if (v.contains("vsync")) video_.vsync = v["vsync"].get<bool>();
            if (v.contains("framerate_limit")) video_.framerate_limit = v["framerate_limit"].get<uint32_t>();
            if (v.contains("show_fps")) video_.show_fps = v["show_fps"].get<bool>();
            if (v.contains("remember_position")) video_.remember_position = v["remember_position"].get<bool>();
            if (v.contains("window_x")) video_.window_x = v["window_x"].get<int32_t>();
            if (v.contains("window_y")) video_.window_y = v["window_y"].get<int32_t>();
        }

        // Audio settings
        if (json.contains("audio")) {
            auto& a = json["audio"];
            if (a.contains("master_volume")) audio_.master_volume = a["master_volume"].get<float>();
            if (a.contains("music_volume")) audio_.music_volume = a["music_volume"].get<float>();
            if (a.contains("sfx_volume")) audio_.sfx_volume = a["sfx_volume"].get<float>();
            if (a.contains("muted")) audio_.muted = a["muted"].get<bool>();
            if (a.contains("music_enabled")) audio_.music_enabled = a["music_enabled"].get<bool>();
            if (a.contains("sfx_enabled")) audio_.sfx_enabled = a["sfx_enabled"].get<bool>();
        }

        // Network settings
        if (json.contains("network")) {
            auto& n = json["network"];
            if (n.contains("login_server_host")) network_.login_server_host = n["login_server_host"].get<std::string>();
            if (n.contains("login_server_port")) network_.login_server_port = n["login_server_port"].get<uint16_t>();
            if (n.contains("connection_timeout_ms")) network_.connection_timeout_ms = n["connection_timeout_ms"].get<uint32_t>();
            if (n.contains("reconnect_attempts")) network_.reconnect_attempts = n["reconnect_attempts"].get<uint32_t>();
            if (n.contains("reconnect_delay_ms")) network_.reconnect_delay_ms = n["reconnect_delay_ms"].get<uint32_t>();
        }

        // Chat settings
        if (json.contains("chat")) {
            auto& c = json["chat"];
            if (c.contains("max_history")) chat_.max_history = c["max_history"].get<size_t>();
            if (c.contains("max_message_length")) chat_.max_message_length = c["max_message_length"].get<size_t>();
            if (c.contains("show_timestamps")) chat_.show_timestamps = c["show_timestamps"].get<bool>();
            if (c.contains("filter_profanity")) chat_.filter_profanity = c["filter_profanity"].get<bool>();
            if (c.contains("block_spam")) chat_.block_spam = c["block_spam"].get<bool>();
            if (c.contains("spam_delay")) chat_.spam_delay = c["spam_delay"].get<float>();
            if (c.contains("show_normal")) chat_.show_normal = c["show_normal"].get<bool>();
            if (c.contains("show_shout")) chat_.show_shout = c["show_shout"].get<bool>();
            if (c.contains("show_whisper")) chat_.show_whisper = c["show_whisper"].get<bool>();
            if (c.contains("show_guild")) chat_.show_guild = c["show_guild"].get<bool>();
            if (c.contains("show_party")) chat_.show_party = c["show_party"].get<bool>();
            if (c.contains("show_system")) chat_.show_system = c["show_system"].get<bool>();
            if (c.contains("show_trade")) chat_.show_trade = c["show_trade"].get<bool>();
            if (c.contains("show_global")) chat_.show_global = c["show_global"].get<bool>();
        }

        // Game settings
        if (json.contains("game")) {
            auto& g = json["game"];
            if (g.contains("ui_language")) {
                std::string lang_str = g["ui_language"].get<std::string>();
                if (lang_str == "en") game_.ui_language = language::english;
                else if (lang_str == "ko") game_.ui_language = language::korean;
                else if (lang_str == "ja") game_.ui_language = language::japanese;
                else if (lang_str == "zh_cn") game_.ui_language = language::chinese_simplified;
                else if (lang_str == "zh_tw") game_.ui_language = language::chinese_traditional;
            }
            if (g.contains("auto_attack")) game_.auto_attack = g["auto_attack"].get<bool>();
            if (g.contains("show_damage_numbers")) game_.show_damage_numbers = g["show_damage_numbers"].get<bool>();
            if (g.contains("show_names")) game_.show_names = g["show_names"].get<bool>();
            if (g.contains("show_guild_names")) game_.show_guild_names = g["show_guild_names"].get<bool>();
            if (g.contains("show_hp_bars")) game_.show_hp_bars = g["show_hp_bars"].get<bool>();
            if (g.contains("camera_shake")) game_.camera_shake = g["camera_shake"].get<bool>();
            if (g.contains("camera_speed")) game_.camera_speed = g["camera_speed"].get<float>();
        }

        // Control settings
        if (json.contains("controls")) {
            auto& ctrl = json["controls"];
            if (ctrl.contains("move_up_key")) controls_.move_up_key = ctrl["move_up_key"].get<int32_t>();
            if (ctrl.contains("move_down_key")) controls_.move_down_key = ctrl["move_down_key"].get<int32_t>();
            if (ctrl.contains("move_left_key")) controls_.move_left_key = ctrl["move_left_key"].get<int32_t>();
            if (ctrl.contains("move_right_key")) controls_.move_right_key = ctrl["move_right_key"].get<int32_t>();
            if (ctrl.contains("attack_key")) controls_.attack_key = ctrl["attack_key"].get<int32_t>();
            if (ctrl.contains("skill_key")) controls_.skill_key = ctrl["skill_key"].get<int32_t>();
            if (ctrl.contains("inventory_key")) controls_.inventory_key = ctrl["inventory_key"].get<int32_t>();
            if (ctrl.contains("skills_key")) controls_.skills_key = ctrl["skills_key"].get<int32_t>();
            if (ctrl.contains("spells_key")) controls_.spells_key = ctrl["spells_key"].get<int32_t>();
            if (ctrl.contains("chat_key")) controls_.chat_key = ctrl["chat_key"].get<int32_t>();
            if (ctrl.contains("screenshot_key")) controls_.screenshot_key = ctrl["screenshot_key"].get<int32_t>();
            if (ctrl.contains("mouse_sensitivity")) controls_.mouse_sensitivity = ctrl["mouse_sensitivity"].get<float>();
            if (ctrl.contains("invert_mouse_y")) controls_.invert_mouse_y = ctrl["invert_mouse_y"].get<bool>();
        }

        config_path_ = path;
        spdlog::info("Loaded configuration from: {}", path);
        return true;

    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse config file: {}", e.what());
        return false;
    }
}

bool config::save(std::string_view path) const {
    nlohmann::json json;

    // Video settings
    json["video"] = {
        {"screen_width", video_.screen_width},
        {"screen_height", video_.screen_height},
        {"fullscreen", video_.fullscreen},
        {"vsync", video_.vsync},
        {"framerate_limit", video_.framerate_limit},
        {"show_fps", video_.show_fps},
        {"remember_position", video_.remember_position},
        {"window_x", video_.window_x},
        {"window_y", video_.window_y}
    };

    // Audio settings
    json["audio"] = {
        {"master_volume", audio_.master_volume},
        {"music_volume", audio_.music_volume},
        {"sfx_volume", audio_.sfx_volume},
        {"muted", audio_.muted},
        {"music_enabled", audio_.music_enabled},
        {"sfx_enabled", audio_.sfx_enabled}
    };

    // Network settings
    json["network"] = {
        {"login_server_host", network_.login_server_host},
        {"login_server_port", network_.login_server_port},
        {"connection_timeout_ms", network_.connection_timeout_ms},
        {"reconnect_attempts", network_.reconnect_attempts},
        {"reconnect_delay_ms", network_.reconnect_delay_ms}
    };

    // Chat settings
    json["chat"] = {
        {"max_history", chat_.max_history},
        {"max_message_length", chat_.max_message_length},
        {"show_timestamps", chat_.show_timestamps},
        {"filter_profanity", chat_.filter_profanity},
        {"block_spam", chat_.block_spam},
        {"spam_delay", chat_.spam_delay},
        {"show_normal", chat_.show_normal},
        {"show_shout", chat_.show_shout},
        {"show_whisper", chat_.show_whisper},
        {"show_guild", chat_.show_guild},
        {"show_party", chat_.show_party},
        {"show_system", chat_.show_system},
        {"show_trade", chat_.show_trade},
        {"show_global", chat_.show_global}
    };

    // Game settings
    std::string lang_str;
    switch (game_.ui_language) {
        case language::english: lang_str = "en"; break;
        case language::korean: lang_str = "ko"; break;
        case language::japanese: lang_str = "ja"; break;
        case language::chinese_simplified: lang_str = "zh_cn"; break;
        case language::chinese_traditional: lang_str = "zh_tw"; break;
        default: lang_str = "en"; break;
    }

    json["game"] = {
        {"ui_language", lang_str},
        {"auto_attack", game_.auto_attack},
        {"show_damage_numbers", game_.show_damage_numbers},
        {"show_names", game_.show_names},
        {"show_guild_names", game_.show_guild_names},
        {"show_hp_bars", game_.show_hp_bars},
        {"camera_shake", game_.camera_shake},
        {"camera_speed", game_.camera_speed}
    };

    // Control settings
    json["controls"] = {
        {"move_up_key", controls_.move_up_key},
        {"move_down_key", controls_.move_down_key},
        {"move_left_key", controls_.move_left_key},
        {"move_right_key", controls_.move_right_key},
        {"attack_key", controls_.attack_key},
        {"skill_key", controls_.skill_key},
        {"inventory_key", controls_.inventory_key},
        {"skills_key", controls_.skills_key},
        {"spells_key", controls_.spells_key},
        {"chat_key", controls_.chat_key},
        {"screenshot_key", controls_.screenshot_key},
        {"mouse_sensitivity", controls_.mouse_sensitivity},
        {"invert_mouse_y", controls_.invert_mouse_y}
    };

    // Write to file
    std::string out_path{path};
    std::ofstream file{out_path};
    if (!file) {
        spdlog::error("Failed to open config file for writing: {}", path);
        return false;
    }

    file << json.dump(4);  // Pretty print with 4 spaces
    spdlog::info("Saved configuration to: {}", path);
    return true;
}

bool config::save() const {
    if (config_path_.empty()) {
        spdlog::error("No config file path set, cannot save");
        return false;
    }
    return save(config_path_);
}

void config::on_changed(config_change_callback callback) {
    if (callback) {
        callbacks_.push_back(std::move(callback));
    }
}

void config::notify_changed() {
    for (auto& callback : callbacks_) {
        if (callback) {
            callback();
        }
    }
}

} // namespace hb
