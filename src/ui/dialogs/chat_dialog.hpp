#pragma once

#include "ui/ui_system.hpp"
#include "chat/chat_message.hpp"
#include <functional>
#include <vector>
#include <string>
#include <deque>

namespace hb {

// Chat dialog - handles chat messages and input
class chat_dialog : public dialog {
public:
    static constexpr size_t max_messages = 200;
    static constexpr int32_t visible_lines = 6;
    static constexpr int32_t line_height = 16;

    chat_dialog();
    ~chat_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_key_press(sf::Keyboard::Key key) override;
    bool handle_text_input(char32_t unicode) override;

    // Add message
    void add_message(const chat_message& msg);
    void add_system_message(std::string_view text);
    void add_whisper(std::string_view sender, std::string_view text);

    // Clear messages
    void clear_messages();

    // Chat mode
    enum class chat_mode {
        normal,
        shout,
        whisper,
        party,
        guild,
        trade
    };

    void set_chat_mode(chat_mode mode);
    chat_mode current_chat_mode() const { return current_mode_; }

    // Whisper target
    void set_whisper_target(std::string_view name) { whisper_target_ = name; }
    std::string_view whisper_target() const { return whisper_target_; }

    // Input callbacks
    using send_callback = std::function<void(std::string_view message, chat_mode mode)>;
    void set_on_send(send_callback callback) { on_send_ = std::move(callback); }

    // Focus the input field
    void focus_input();
    bool is_input_focused() const { return input_focused_; }

private:
    sf::Color get_message_color(chat_type type) const;
    std::string get_mode_prefix() const;

    std::deque<chat_message> messages_;
    chat_mode current_mode_ = chat_mode::normal;
    std::string whisper_target_;

    // Input
    std::string input_text_;
    size_t cursor_pos_ = 0;
    bool input_focused_ = false;
    float cursor_blink_timer_ = 0.0f;
    bool cursor_visible_ = true;

    // Scroll
    int32_t scroll_offset_ = 0;

    // Callback
    send_callback on_send_;

    // Tab completion
    std::vector<std::string> recent_senders_;

    static constexpr size_t max_input_length = 200;
};

} // namespace hb
