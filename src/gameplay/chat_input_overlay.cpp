#include "gameplay/chat_input_overlay.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace hb {

void chat_input_overlay::activate()
{
    active_ = true;
    input_text_.clear();
    cursor_pos_ = 0;
    blink_timer_ = 0.0f;
    cursor_visible_ = true;

    // If whisper mode is persistent, keep it; otherwise default to say
    if (!whisper_active_)
        mode_ = chat_mode::say;
    else
        mode_ = chat_mode::whisper;
}

void chat_input_overlay::deactivate()
{
    active_ = false;
    input_text_.clear();
    cursor_pos_ = 0;
}

bool chat_input_overlay::update(float delta_time, const input& inp)
{
    if (!active_)
    {
        // Enter activates in default (say / whisper-persist) mode
        if (inp.is_key_pressed(sf::Keyboard::Key::Enter))
        {
            activate();
            return true;
        }

        // Check for prefix key activation via text_input
        auto text = inp.text_input();
        if (!text.empty())
        {
            char ch = text[0];
            switch (ch)
            {
                case '!': activate(); mode_ = chat_mode::shout; return true;
                case '~': activate(); mode_ = chat_mode::faction; return true;
                case '@': activate(); mode_ = chat_mode::guild; return true;
                case '$': activate(); mode_ = chat_mode::party; return true;
                case '%': activate(); mode_ = chat_mode::trade; return true;
                case '^': activate(); mode_ = chat_mode::gm; return true;
                case '#':
                    // # overrides whisper for one message -> force say mode
                    activate();
                    mode_ = chat_mode::say;
                    return true;
                case '/':
                    activate();
                    input_text_ = "/";
                    cursor_pos_ = 1;
                    return true;
                default:
                    // Type-to-chat: any printable character opens chat and seeds input
                    if (type_to_chat_ && ch >= 32 && ch != 127)
                    {
                        activate();
                        input_text_ = std::string(1, ch);
                        cursor_pos_ = 1;
                        return true;
                    }
                    break;
            }
        }

        return false;
    }

    // --- Active state ---

    // Cursor blink
    blink_timer_ += delta_time;
    if (blink_timer_ >= 0.5f)
    {
        blink_timer_ = 0.0f;
        cursor_visible_ = !cursor_visible_;
    }

    // Escape cancels
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape))
    {
        deactivate();
        return true;
    }

    // Enter sends
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter))
    {
        send_current();
        return true;
    }

    // Navigation keys
    if (inp.is_key_pressed(sf::Keyboard::Key::Backspace) && cursor_pos_ > 0)
    {
        input_text_.erase(cursor_pos_ - 1, 1);
        cursor_pos_--;
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::Delete) && cursor_pos_ < input_text_.size())
    {
        input_text_.erase(cursor_pos_, 1);
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::Left) && cursor_pos_ > 0)
    {
        cursor_pos_--;
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::Right) && cursor_pos_ < input_text_.size())
    {
        cursor_pos_++;
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::Home))
    {
        cursor_pos_ = 0;
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::End))
    {
        cursor_pos_ = input_text_.size();
    }

    // Text input
    auto text = inp.text_input();
    for (char ch : text)
    {
        if (ch < 32 || ch == 127) continue;
        if (input_text_.size() >= max_input_length) break;
        input_text_.insert(cursor_pos_, 1, ch);
        cursor_pos_++;
    }

    // Reset blink on any input
    if (!text.empty() || inp.is_key_pressed(sf::Keyboard::Key::Backspace) ||
        inp.is_key_pressed(sf::Keyboard::Key::Delete))
    {
        cursor_visible_ = true;
        blink_timer_ = 0.0f;
    }

    return true;  // Consuming all input while active
}

void chat_input_overlay::render(renderer& rend, int32_t screen_width, int32_t screen_height)
{
    if (!active_) return;

    // Position: above icon panel (icon panel is ~34px tall at bottom)
    constexpr int32_t bar_height = 24;
    constexpr int32_t icon_panel_height = 34;
    constexpr int32_t padding = 6;
    int32_t bar_y = screen_height - icon_panel_height - bar_height - 22;
    int32_t bar_x = 4;
    int32_t bar_width = screen_width - 8;

    // Semi-transparent background
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(0, 0, 0, 180), true);
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(60, 60, 80), false);

    // Mode label
    constexpr uint32_t font_size = 12;
    std::string label = label_for_mode();
    sf::Color label_color = color_for_mode();
    rend.draw_text(label, bar_x + padding, bar_y + 5, label_color, font_size);

    float label_w = rend.text().measure_width(label, font_size);
    int32_t text_x = bar_x + padding + static_cast<int32_t>(label_w) + 4;
    float avail_w = static_cast<float>(std::max(14, bar_width - (text_x - bar_x) - padding));

    // Input text (scrolled to show cursor)
    size_t start_char = 0;
    if (!input_text_.empty())
    {
        // Advance start_char until text from start_char..cursor fits
        while (start_char < cursor_pos_)
        {
            auto sub = std::string_view(input_text_).substr(start_char, cursor_pos_ - start_char);
            if (rend.text().measure_width(sub, font_size) <= avail_w)
                break;
            start_char++;
        }
    }

    // Build visible substring that fits
    size_t end_char = cursor_pos_;
    while (end_char < input_text_.size())
    {
        auto sub = std::string_view(input_text_).substr(start_char, end_char + 1 - start_char);
        if (rend.text().measure_width(sub, font_size) > avail_w)
            break;
        end_char++;
    }
    std::string visible_text = input_text_.substr(start_char, end_char - start_char);

    rend.draw_text(visible_text, text_x, bar_y + 5, label_color, font_size);

    // Blinking cursor
    if (cursor_visible_)
    {
        auto cursor_sub = std::string_view(input_text_).substr(start_char, cursor_pos_ - start_char);
        int32_t cursor_x = text_x + static_cast<int32_t>(rend.text().measure_width(cursor_sub, font_size));
        rend.draw_line(cursor_x, bar_y + 3, cursor_x, bar_y + bar_height - 3, sf::Color::White);
    }
}

void chat_input_overlay::send_current()
{
    // Trim whitespace
    std::string text = input_text_;
    while (!text.empty() && text.back() == ' ') text.pop_back();
    while (!text.empty() && text.front() == ' ') text.erase(text.begin());

    if (text.empty())
    {
        deactivate();
        return;
    }

    // Check for slash commands
    if (text[0] == '/')
    {
        parse_command(text);
        deactivate();
        return;
    }

    // Determine channel
    std::string_view channel = channel_for_mode();
    std::string_view recipient;

    // If whisper is active and mode is whisper/say (no prefix override), send as whisper
    if (whisper_active_ && (mode_ == chat_mode::whisper || mode_ == chat_mode::say))
    {
        channel = "whisper";
        recipient = whisper_target_;
    }

    if (on_send_)
    {
        on_send_(text, channel, recipient);
    }

    deactivate();
}

void chat_input_overlay::parse_command(std::string_view text)
{
    // /to PlayerName - activate whisper
    if (text.size() > 4 && text.substr(0, 4) == "/to ")
    {
        std::string target(text.substr(4));
        // Trim
        while (!target.empty() && target.back() == ' ') target.pop_back();
        if (!target.empty())
        {
            whisper_target_ = target;
            whisper_active_ = true;
            spdlog::info("Whisper mode activated: target='{}'", whisper_target_);
        }
        return;
    }

    // /tooff - deactivate whisper
    if (text == "/tooff")
    {
        whisper_active_ = false;
        whisper_target_.clear();
        spdlog::info("Whisper mode deactivated");
        return;
    }

    // Unknown command - send to server as command (no bubble, no chat echo)
    if (on_command_)
    {
        on_command_(text);
    }
}

std::string_view chat_input_overlay::channel_for_mode() const
{
    switch (mode_)
    {
        case chat_mode::say:     return "local";
        case chat_mode::shout:   return "shout";
        case chat_mode::faction: return "faction";
        case chat_mode::guild:   return "guild";
        case chat_mode::party:   return "party";
        case chat_mode::trade:   return "trade";
        case chat_mode::gm:      return "gm";
        case chat_mode::whisper: return "whisper";
    }
    return "local";
}

sf::Color chat_input_overlay::color_for_mode() const
{
    switch (mode_)
    {
        case chat_mode::say:     return get_chat_bubble_style("local").color;
        case chat_mode::shout:   return get_chat_bubble_style("shout").color;
        case chat_mode::faction: return get_chat_bubble_style("faction").color;
        case chat_mode::guild:   return get_chat_bubble_style("guild").color;
        case chat_mode::party:   return get_chat_bubble_style("party").color;
        case chat_mode::trade:   return get_chat_bubble_style("trade").color;
        case chat_mode::gm:      return get_chat_bubble_style("gm").color;
        case chat_mode::whisper: return get_chat_bubble_style("whisper").color;
    }
    return get_chat_bubble_style("local").color;
}

std::string chat_input_overlay::label_for_mode() const
{
    if (whisper_active_ && (mode_ == chat_mode::whisper || mode_ == chat_mode::say))
    {
        return "[To:" + whisper_target_ + "]";
    }

    switch (mode_)
    {
        case chat_mode::say:     return "[Say]";
        case chat_mode::shout:   return "[Shout]";
        case chat_mode::faction: return "[Faction]";
        case chat_mode::guild:   return "[Guild]";
        case chat_mode::party:   return "[Party]";
        case chat_mode::trade:   return "[Trade]";
        case chat_mode::gm:      return "[GM]";
        case chat_mode::whisper: return "[Whisper]";
    }
    return "[Say]";
}

} // namespace hb
