#pragma once

#include "chat/chat_message.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <functional>
#include <unordered_set>

namespace hb {

// Chat system configuration
struct chat_config {
    size_t max_history = 500;
    size_t max_message_length = 200;
    bool show_timestamps = false;
    bool filter_profanity = true;
    bool block_spam = true;
    float spam_delay = 0.5f;  // Minimum seconds between messages

    // Channel visibility
    bool show_normal = true;
    bool show_shout = true;
    bool show_whisper = true;
    bool show_guild = true;
    bool show_party = true;
    bool show_system = true;
    bool show_trade = true;
    bool show_global = true;
};

// Chat callbacks
struct chat_callbacks {
    std::function<void(const chat_message&)> on_message_received;
    std::function<void(chat_type, std::string_view)> on_send_message;
    std::function<void(std::string_view, std::string_view)> on_send_whisper;
};

class chat_system {
public:
    chat_system() = default;
    ~chat_system() = default;

    // Initialize/shutdown
    void initialize();
    void shutdown();

    // Update (for spam protection timing)
    void update(float delta_time);

    // Receive message (from network)
    void receive_message(const chat_message& msg);
    void receive_message(chat_type type, std::string_view sender, std::string_view content);

    // Send message (to network)
    bool send_message(std::string_view content);  // Auto-detect type from prefix
    bool send_normal(std::string_view content);
    bool send_shout(std::string_view content);
    bool send_whisper(std::string_view target, std::string_view content);
    bool send_guild(std::string_view content);
    bool send_party(std::string_view content);
    bool send_trade(std::string_view content);

    // Message history
    const std::deque<chat_message>& history() const { return history_; }
    std::vector<const chat_message*> get_filtered_history(size_t count = 50) const;
    std::vector<const chat_message*> get_messages_by_type(chat_type type, size_t count = 50) const;
    void clear_history();

    // Ignore list
    void add_ignore(std::string_view name);
    void remove_ignore(std::string_view name);
    bool is_ignored(std::string_view name) const;
    const std::unordered_set<std::string>& ignore_list() const { return ignore_list_; }
    void clear_ignore_list() { ignore_list_.clear(); }

    // Profanity filter
    void add_filter_word(std::string_view word);
    void remove_filter_word(std::string_view word);
    std::string filter_message(std::string_view message) const;
    void load_filter_list(std::string_view path);

    // Configuration
    void set_config(const chat_config& config) { config_ = config; }
    const chat_config& config() const { return config_; }

    // Callbacks
    void set_callbacks(const chat_callbacks& callbacks) { callbacks_ = callbacks; }

    // Last whisper target (for quick reply)
    void set_last_whisper_target(std::string_view name) { last_whisper_target_ = name; }
    std::string_view last_whisper_target() const { return last_whisper_target_; }

    // System messages
    void add_system_message(std::string_view content);
    void add_error_message(std::string_view content);

private:
    bool can_send_message() const;
    void add_to_history(const chat_message& msg);
    bool should_show_message(const chat_message& msg) const;
    chat_type detect_message_type(std::string_view& content) const;

    std::deque<chat_message> history_;
    std::unordered_set<std::string> ignore_list_;
    std::unordered_set<std::string> filter_words_;

    chat_config config_;
    chat_callbacks callbacks_;

    std::string last_whisper_target_;
    float spam_timer_ = 0.0f;
};

} // namespace hb
