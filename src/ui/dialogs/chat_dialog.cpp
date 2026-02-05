#include "ui/dialogs/chat_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <algorithm>

namespace hb {

chat_dialog::chat_dialog()
    : dialog(dialog_type::chat) {
    set_title("");  // No title bar for chat
    set_bounds({0, static_cast<int32_t>(screen_height) - 140,
                static_cast<int32_t>(screen_width), 140});
    set_draggable(false);
    set_closeable(false);
    set_visible(true);
    set_has_border(false);
    set_background_color(sf::Color(0, 0, 0, 150));
    set_drag_clamp(drag_clamp::partial);
}

void chat_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);

    if (input_focused_) {
        cursor_blink_timer_ += delta_time;
        if (cursor_blink_timer_ >= 0.5f) {
            cursor_blink_timer_ = 0.0f;
            cursor_visible_ = !cursor_visible_;
        }
    }
}

void chat_dialog::render(renderer& rend) {
    if (!visible_) return;

    // Semi-transparent background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(0, 0, 0, 150), true);

    int32_t x = bounds_.x + 8;
    int32_t y = bounds_.y + 8;

    // Render messages from bottom up
    int32_t msg_area_height = bounds_.height - 35;
    int32_t max_lines = msg_area_height / line_height;

    int32_t start_idx = std::max(0, static_cast<int32_t>(messages_.size()) - max_lines - scroll_offset_);
    int32_t end_idx = std::max(0, static_cast<int32_t>(messages_.size()) - scroll_offset_);

    int32_t line_y = y;
    for (int32_t i = start_idx; i < end_idx && line_y < y + msg_area_height; ++i) {
        const auto& msg = messages_[i];

        sf::Color color = get_message_color(msg.type);

        std::string display_text;
        if (!msg.sender.empty()) {
            display_text = std::format("[{}] {}", msg.sender, msg.content);
        } else {
            display_text = msg.content;
        }

        // Truncate if too long
        size_t max_chars = static_cast<size_t>((bounds_.width - 16) / 6);
        if (display_text.length() > max_chars) {
            display_text = display_text.substr(0, max_chars - 3) + "...";
        }

        rend.draw_text(display_text, x, line_y, color, 12);
        line_y += line_height;
    }

    // Input area
    int32_t input_y = bounds_.y + bounds_.height - 28;

    // Mode indicator
    std::string prefix = get_mode_prefix();
    sf::Color prefix_color;
    switch (current_mode_) {
        case chat_mode::shout: prefix_color = sf::Color(255, 150, 150); break;
        case chat_mode::whisper: prefix_color = sf::Color(255, 200, 255); break;
        case chat_mode::party: prefix_color = sf::Color(150, 200, 255); break;
        case chat_mode::guild: prefix_color = sf::Color(150, 255, 150); break;
        case chat_mode::trade: prefix_color = sf::Color(255, 220, 100); break;
        default: prefix_color = sf::Color(180, 180, 180); break;
    }
    rend.draw_text(prefix, x, input_y + 4, prefix_color, 12);

    int32_t input_x = x + static_cast<int32_t>(prefix.length() * 6) + 4;
    int32_t input_width = bounds_.width - input_x - 80;

    // Input background
    sf::Color input_bg = input_focused_ ? sf::Color(30, 30, 40, 200) : sf::Color(20, 20, 30, 150);
    rend.draw_rect(input_x, input_y, input_width, 22, input_bg, true);
    rend.draw_rect(input_x, input_y, input_width, 22,
                   input_focused_ ? sf::Color(80, 80, 120) : sf::Color(50, 50, 70), false);

    // Input text
    std::string display_input = input_text_;
    size_t max_visible = static_cast<size_t>((input_width - 8) / 6);
    size_t start_char = 0;
    if (cursor_pos_ > max_visible - 1) {
        start_char = cursor_pos_ - max_visible + 1;
    }
    display_input = display_input.substr(start_char,
        std::min(max_visible, display_input.length() - start_char));

    rend.draw_text(display_input, input_x + 4, input_y + 4, sf::Color::White, 12);

    // Cursor
    if (input_focused_ && cursor_visible_) {
        int32_t cursor_x = input_x + 4 + static_cast<int32_t>((cursor_pos_ - start_char) * 6);
        rend.draw_line(cursor_x, input_y + 3, cursor_x, input_y + 19, sf::Color::White);
    }

    // Send button
    int32_t send_btn_x = bounds_.x + bounds_.width - 70;
    rend.draw_rect(send_btn_x, input_y, 60, 22, sf::Color(50, 80, 50), true);
    rend.draw_text("Send", send_btn_x + 15, input_y + 4, sf::Color::White, 12);

    // Scroll indicator
    if (messages_.size() > static_cast<size_t>(max_lines)) {
        int32_t scroll_x = bounds_.x + bounds_.width - 12;
        int32_t scroll_y = bounds_.y + 8;
        int32_t scroll_height = msg_area_height;

        rend.draw_rect(scroll_x, scroll_y, 6, scroll_height, sf::Color(30, 30, 40), true);

        float visible_ratio = static_cast<float>(max_lines) / messages_.size();
        int32_t thumb_height = std::max(20, static_cast<int32_t>(scroll_height * visible_ratio));

        float scroll_ratio = static_cast<float>(scroll_offset_) / (messages_.size() - max_lines);
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) * (1.0f - scroll_ratio));

        rend.draw_rect(scroll_x, thumb_y, 6, thumb_height, sf::Color(80, 80, 100), true);
    }
}

bool chat_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    int32_t input_y = bounds_.y + bounds_.height - 28;
    int32_t send_btn_x = bounds_.x + bounds_.width - 70;

    // Check send button
    ui_rect send_btn{send_btn_x, input_y, 60, 22};
    if (send_btn.contains(x, y)) {
        if (!input_text_.empty() && on_send_) {
            on_send_(input_text_, current_mode_);
            input_text_.clear();
            cursor_pos_ = 0;
        }
        return true;
    }

    // Check input area
    int32_t input_x = bounds_.x + 8 + static_cast<int32_t>(get_mode_prefix().length() * 6) + 4;
    int32_t input_width = bounds_.width - input_x - 80;
    ui_rect input_area{input_x, input_y, input_width, 22};

    if (input_area.contains(x, y)) {
        input_focused_ = true;
        cursor_visible_ = true;
        cursor_blink_timer_ = 0.0f;
        return true;
    }

    // Click elsewhere unfocuses
    if (!bounds_.contains(x, y)) {
        input_focused_ = false;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool chat_dialog::handle_key_press(sf::Keyboard::Key key) {
    if (!input_focused_) return false;

    if (key == sf::Keyboard::Key::Enter) {
        if (!input_text_.empty() && on_send_) {
            on_send_(input_text_, current_mode_);
            input_text_.clear();
            cursor_pos_ = 0;
        }
        return true;
    }

    if (key == sf::Keyboard::Key::Escape) {
        input_focused_ = false;
        return true;
    }

    if (key == sf::Keyboard::Key::Backspace && cursor_pos_ > 0) {
        input_text_.erase(cursor_pos_ - 1, 1);
        cursor_pos_--;
        return true;
    }

    if (key == sf::Keyboard::Key::Delete && cursor_pos_ < input_text_.length()) {
        input_text_.erase(cursor_pos_, 1);
        return true;
    }

    if (key == sf::Keyboard::Key::Left && cursor_pos_ > 0) {
        cursor_pos_--;
        return true;
    }

    if (key == sf::Keyboard::Key::Right && cursor_pos_ < input_text_.length()) {
        cursor_pos_++;
        return true;
    }

    if (key == sf::Keyboard::Key::Home) {
        cursor_pos_ = 0;
        return true;
    }

    if (key == sf::Keyboard::Key::End) {
        cursor_pos_ = input_text_.length();
        return true;
    }

    if (key == sf::Keyboard::Key::PageUp) {
        int32_t max_scroll = std::max(0, static_cast<int32_t>(messages_.size()) - visible_lines);
        scroll_offset_ = std::min(scroll_offset_ + visible_lines, max_scroll);
        return true;
    }

    if (key == sf::Keyboard::Key::PageDown) {
        scroll_offset_ = std::max(0, scroll_offset_ - visible_lines);
        return true;
    }

    return false;
}

bool chat_dialog::handle_text_input(char32_t unicode) {
    if (!input_focused_) return false;

    // Filter control characters
    if (unicode < 32 || unicode == 127) {
        return false;
    }

    if (input_text_.length() >= max_input_length) {
        return false;
    }

    // Check for chat mode prefixes
    if (input_text_.empty() && cursor_pos_ == 0) {
        switch (unicode) {
            case '!':
                set_chat_mode(chat_mode::shout);
                return true;
            case '@':
                set_chat_mode(chat_mode::whisper);
                return true;
            case '#':
                set_chat_mode(chat_mode::party);
                return true;
            case '$':
                set_chat_mode(chat_mode::guild);
                return true;
            case '%':
                set_chat_mode(chat_mode::trade);
                return true;
        }
    }

    // Add character
    if (unicode < 128) {
        input_text_.insert(cursor_pos_, 1, static_cast<char>(unicode));
        cursor_pos_++;
        return true;
    }

    return false;
}

void chat_dialog::add_message(const chat_message& msg) {
    messages_.push_back(msg);

    // Keep track of senders for tab completion
    if (!msg.sender.empty()) {
        auto it = std::find(recent_senders_.begin(), recent_senders_.end(), msg.sender);
        if (it != recent_senders_.end()) {
            recent_senders_.erase(it);
        }
        recent_senders_.insert(recent_senders_.begin(), msg.sender);
        if (recent_senders_.size() > 20) {
            recent_senders_.pop_back();
        }
    }

    // Limit message history
    while (messages_.size() > max_messages) {
        messages_.pop_front();
    }

    // Auto-scroll to bottom
    scroll_offset_ = 0;
}

void chat_dialog::add_system_message(std::string_view text) {
    chat_message msg;
    msg.type = chat_type::system;
    msg.content = std::string(text);
    add_message(msg);
}

void chat_dialog::add_whisper(std::string_view sender, std::string_view text) {
    chat_message msg;
    msg.type = chat_type::whisper;
    msg.sender = std::string(sender);
    msg.content = std::string(text);
    add_message(msg);

    // Set as whisper target
    whisper_target_ = sender;
}

void chat_dialog::clear_messages() {
    messages_.clear();
    scroll_offset_ = 0;
}

void chat_dialog::set_chat_mode(chat_mode mode) {
    current_mode_ = mode;
}

void chat_dialog::focus_input() {
    input_focused_ = true;
    cursor_visible_ = true;
    cursor_blink_timer_ = 0.0f;
}

sf::Color chat_dialog::get_message_color(chat_type type) const {
    switch (type) {
        case chat_type::shout:
            return sf::Color(255, 150, 150);
        case chat_type::whisper:
            return sf::Color(255, 200, 255);
        case chat_type::party:
            return sf::Color(150, 200, 255);
        case chat_type::guild:
            return sf::Color(150, 255, 150);
        case chat_type::system:
            return sf::Color::Yellow;
        case chat_type::gm:
            return sf::Color(255, 100, 100);
        case chat_type::trade:
            return sf::Color(255, 220, 100);
        default:
            return sf::Color::White;
    }
}

std::string chat_dialog::get_mode_prefix() const {
    switch (current_mode_) {
        case chat_mode::shout:
            return "[Shout]";
        case chat_mode::whisper:
            return std::format("[To:{}]", whisper_target_.empty() ? "?" : whisper_target_);
        case chat_mode::party:
            return "[Party]";
        case chat_mode::guild:
            return "[Guild]";
        case chat_mode::trade:
            return "[Trade]";
        default:
            return "[Say]";
    }
}

} // namespace hb
