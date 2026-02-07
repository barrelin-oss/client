#include "chat/chat_system.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <cctype>

namespace hb {

void chat_system::initialize() {
    history_.clear();
    ignore_list_.clear();
    filter_words_.clear();
    spam_timer_ = 0.0f;
    sender_message_times_.clear();

    spdlog::info("Chat system initialized");
}

void chat_system::shutdown() {
    history_.clear();
    ignore_list_.clear();
    filter_words_.clear();
    sender_message_times_.clear();
    spdlog::info("Chat system shutdown");
}

void chat_system::update(float delta_time) {
    if (spam_timer_ > 0) {
        spam_timer_ -= delta_time;
    }
}

void chat_system::receive_message(const chat_message& msg) {
    // Check if sender is ignored
    if (!msg.sender.empty() && is_ignored(msg.sender)) {
        return;
    }

    // Check for incoming spam (skip system messages - they're from the server)
    if (config_.block_spam && !msg.sender.empty() && msg.type != chat_type::system) {
        if (is_incoming_spam(msg.sender)) {
            return;
        }
    }

    // Profanity filtering is handled server-side; the client sends its
    // filter_profanity preference via set_chat_preferences and the server
    // replaces words before broadcasting.

    chat_message filtered_msg = msg;

    // Set timestamp
    filtered_msg.timestamp = std::chrono::system_clock::now();

    // Add to history
    add_to_history(filtered_msg);

    // Callback
    if (callbacks_.on_message_received) {
        callbacks_.on_message_received(filtered_msg);
    }

    // Track whisper sender for quick reply
    if (msg.type == chat_type::whisper && !msg.sender.empty()) {
        last_whisper_target_ = msg.sender;
    }
}

void chat_system::receive_message(chat_type type, std::string_view sender, std::string_view content) {
    chat_message msg;
    msg.type = type;
    msg.sender = std::string(sender);
    msg.content = std::string(content);
    receive_message(msg);
}

bool chat_system::send_message(std::string_view content) {
    if (content.empty()) {
        return false;
    }

    // Make a mutable copy for type detection
    std::string content_str(content);
    std::string_view content_view = content_str;

    // Detect message type from prefix
    chat_type type = detect_message_type(content_view);

    // Update content to remove prefix
    content_str = std::string(content_view);

    switch (type) {
        case chat_type::shout:
            return send_shout(content_str);
        case chat_type::guild:
            return send_guild(content_str);
        case chat_type::party:
            return send_party(content_str);
        case chat_type::trade:
            return send_trade(content_str);
        case chat_type::whisper:
            // Extract target from content "/w target message"
            {
                size_t space_pos = content_str.find(' ');
                if (space_pos != std::string::npos) {
                    std::string target = content_str.substr(0, space_pos);
                    std::string message = content_str.substr(space_pos + 1);
                    return send_whisper(target, message);
                }
            }
            return false;
        default:
            return send_normal(content_str);
    }
}

bool chat_system::send_normal(std::string_view content) {
    if (!can_send_message() || content.empty()) {
        return false;
    }

    if (content.length() > config_.max_message_length) {
        add_error_message("Message too long");
        return false;
    }

    spam_timer_ = config_.spam_delay;

    if (callbacks_.on_send_message) {
        callbacks_.on_send_message(chat_type::normal, content);
    }

    return true;
}

bool chat_system::send_shout(std::string_view content) {
    if (!can_send_message() || content.empty()) {
        return false;
    }

    spam_timer_ = config_.spam_delay * 2;  // Longer delay for shouts

    if (callbacks_.on_send_message) {
        callbacks_.on_send_message(chat_type::shout, content);
    }

    return true;
}

bool chat_system::send_whisper(std::string_view target, std::string_view content) {
    if (!can_send_message() || content.empty() || target.empty()) {
        return false;
    }

    spam_timer_ = config_.spam_delay;
    last_whisper_target_ = target;

    if (callbacks_.on_send_whisper) {
        callbacks_.on_send_whisper(target, content);
    }

    return true;
}

bool chat_system::send_guild(std::string_view content) {
    if (!can_send_message() || content.empty()) {
        return false;
    }

    spam_timer_ = config_.spam_delay;

    if (callbacks_.on_send_message) {
        callbacks_.on_send_message(chat_type::guild, content);
    }

    return true;
}

bool chat_system::send_party(std::string_view content) {
    if (!can_send_message() || content.empty()) {
        return false;
    }

    spam_timer_ = config_.spam_delay;

    if (callbacks_.on_send_message) {
        callbacks_.on_send_message(chat_type::party, content);
    }

    return true;
}

bool chat_system::send_trade(std::string_view content) {
    if (!can_send_message() || content.empty()) {
        return false;
    }

    spam_timer_ = config_.spam_delay * 3;  // Longer delay for trade spam

    if (callbacks_.on_send_message) {
        callbacks_.on_send_message(chat_type::trade, content);
    }

    return true;
}

std::vector<const chat_message*> chat_system::get_filtered_history(size_t count) const {
    std::vector<const chat_message*> result;
    result.reserve(count);

    for (auto it = history_.rbegin(); it != history_.rend() && result.size() < count; ++it) {
        if (should_show_message(*it)) {
            result.push_back(&(*it));
        }
    }

    // Reverse to get chronological order
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<const chat_message*> chat_system::get_messages_by_type(chat_type type, size_t count) const {
    std::vector<const chat_message*> result;
    result.reserve(count);

    for (auto it = history_.rbegin(); it != history_.rend() && result.size() < count; ++it) {
        if (it->type == type) {
            result.push_back(&(*it));
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

void chat_system::clear_history() {
    history_.clear();
}

void chat_system::add_ignore(std::string_view name) {
    std::string lower_name;
    lower_name.reserve(name.size());
    for (char c : name) {
        lower_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    ignore_list_.insert(lower_name);
    add_system_message("Added " + std::string(name) + " to ignore list");
}

void chat_system::remove_ignore(std::string_view name) {
    std::string lower_name;
    lower_name.reserve(name.size());
    for (char c : name) {
        lower_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    ignore_list_.erase(lower_name);
    add_system_message("Removed " + std::string(name) + " from ignore list");
}

bool chat_system::is_ignored(std::string_view name) const {
    std::string lower_name;
    lower_name.reserve(name.size());
    for (char c : name) {
        lower_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ignore_list_.count(lower_name) > 0;
}

void chat_system::add_filter_word(std::string_view word) {
    std::string lower_word;
    lower_word.reserve(word.size());
    for (char c : word) {
        lower_word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    filter_words_.insert(lower_word);
}

void chat_system::remove_filter_word(std::string_view word) {
    std::string lower_word;
    lower_word.reserve(word.size());
    for (char c : word) {
        lower_word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    filter_words_.erase(lower_word);
}

std::string chat_system::filter_message(std::string_view message) const {
    if (filter_words_.empty()) {
        return std::string(message);
    }

    std::string result(message);
    std::string lower_result;
    lower_result.reserve(result.size());
    for (char c : result) {
        lower_result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    for (const auto& word : filter_words_) {
        size_t pos = 0;
        while ((pos = lower_result.find(word, pos)) != std::string::npos) {
            // Replace with asterisks
            for (size_t i = 0; i < word.length(); ++i) {
                result[pos + i] = '*';
                lower_result[pos + i] = '*';
            }
            pos += word.length();
        }
    }

    return result;
}

void chat_system::load_filter_list(std::string_view path) {
    std::string path_str{path};
    std::ifstream file{path_str};
    if (!file) {
        spdlog::warn("Could not load filter list: {}", path);
        return;
    }

    filter_words_.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') {  // Skip comments
            add_filter_word(line);
        }
    }

    spdlog::info("Loaded {} filter words", filter_words_.size());
}

void chat_system::add_system_message(std::string_view content) {
    chat_message msg;
    msg.type = chat_type::system;
    msg.content = std::string(content);
    msg.timestamp = std::chrono::system_clock::now();
    add_to_history(msg);
}

void chat_system::add_error_message(std::string_view content) {
    chat_message msg;
    msg.type = chat_type::system;
    msg.content = "[Error] " + std::string(content);
    msg.timestamp = std::chrono::system_clock::now();
    add_to_history(msg);
}

bool chat_system::is_incoming_spam(std::string_view sender) {
    // Build lowercase sender key
    std::string key;
    key.reserve(sender.size());
    for (char c : sender) {
        key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    auto now = std::chrono::steady_clock::now();
    auto window = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(spam_window_seconds));

    auto& times = sender_message_times_[key];

    // Remove entries outside the time window
    while (!times.empty() && (now - times.front()) > window) {
        times.pop_front();
    }

    // Check if over threshold
    if (times.size() >= spam_message_threshold) {
        return true;
    }

    // Record this message
    times.push_back(now);
    return false;
}

bool chat_system::can_send_message() const {
    if (config_.block_spam && spam_timer_ > 0) {
        return false;
    }
    return true;
}

void chat_system::add_to_history(const chat_message& msg) {
    history_.push_back(msg);

    // Trim history if too large
    while (history_.size() > config_.max_history) {
        history_.pop_front();
    }
}

bool chat_system::should_show_message(const chat_message& msg) const {
    switch (msg.type) {
        case chat_type::normal:  return config_.show_normal;
        case chat_type::shout:   return config_.show_shout;
        case chat_type::whisper: return config_.show_whisper;
        case chat_type::guild:   return config_.show_guild;
        case chat_type::party:   return config_.show_party;
        case chat_type::system:  return config_.show_system;
        case chat_type::trade:   return config_.show_trade;
        case chat_type::global:  return config_.show_global;
        default:                 return true;
    }
}

chat_type chat_system::detect_message_type(std::string_view& content) const {
    if (content.empty()) {
        return chat_type::normal;
    }

    // Check for command prefixes
    if (content[0] == '!') {
        content.remove_prefix(1);
        return chat_type::shout;
    }

    if (content[0] == '@') {
        content.remove_prefix(1);
        return chat_type::guild;
    }

    if (content[0] == '#') {
        content.remove_prefix(1);
        return chat_type::party;
    }

    if (content[0] == '$') {
        content.remove_prefix(1);
        return chat_type::trade;
    }

    // Check for /commands
    if (content.size() > 2 && content[0] == '/') {
        if (content.substr(0, 2) == "/w" || content.substr(0, 8) == "/whisper") {
            size_t start = content[2] == ' ' ? 3 : 9;
            if (start < content.size()) {
                content.remove_prefix(start);
                return chat_type::whisper;
            }
        }

        if (content.substr(0, 2) == "/g" || content.substr(0, 6) == "/guild") {
            size_t start = content[2] == ' ' ? 3 : 7;
            if (start < content.size()) {
                content.remove_prefix(start);
                return chat_type::guild;
            }
        }

        if (content.substr(0, 2) == "/p" || content.substr(0, 6) == "/party") {
            size_t start = content[2] == ' ' ? 3 : 7;
            if (start < content.size()) {
                content.remove_prefix(start);
                return chat_type::party;
            }
        }

        if (content.substr(0, 6) == "/shout") {
            content.remove_prefix(7);
            return chat_type::shout;
        }

        if (content.substr(0, 3) == "/me") {
            content.remove_prefix(4);
            return chat_type::emote;
        }
    }

    return chat_type::normal;
}

} // namespace hb
