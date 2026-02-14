#include "gameplay/chat_input_overlay.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace hb {

// ============================================================
// Command autocomplete helpers
// ============================================================

void chat_input_overlay::set_commands(std::vector<command_entry> commands)
{
    // Add client-side commands that never go to the server
    commands.push_back({"to", "Whisper to a player", "/to <player>", "chat", true});
    commands.push_back({"tooff", "Stop whispering", "/tooff", "chat", true});

    // Sort by category display order, then alphabetically within each category
    std::sort(commands.begin(), commands.end(), [](const command_entry& a, const command_entry& b)
    {
        int oa = category_sort_order(a.category);
        int ob = category_sort_order(b.category);
        if (oa != ob) return oa < ob;
        return a.name < b.name;
    });

    commands_ = std::move(commands);
    spdlog::info("Chat autocomplete: received {} commands", commands_.size());
}

void chat_input_overlay::update_command_availability(const std::string& name, bool enabled)
{
    for (auto& cmd : commands_)
    {
        if (cmd.name == name)
        {
            cmd.enabled = enabled;
            return;
        }
    }
}

int32_t chat_input_overlay::category_sort_order(const std::string& cat)
{
    if (cat == "general") return 0;
    if (cat == "chat") return 1;
    if (cat == "guild") return 2;
    if (cat == "gm") return 3;
    if (cat == "admin") return 4;
    return 5;
}

sf::Color chat_input_overlay::category_stripe_color(const std::string& cat)
{
    if (cat == "general") return sf::Color(200, 200, 200);
    if (cat == "guild") return sf::Color(100, 200, 100);
    if (cat == "chat") return sf::Color(120, 180, 255);
    if (cat == "gm") return sf::Color(0, 220, 220);
    if (cat == "admin") return sf::Color(220, 100, 100);
    return sf::Color(140, 140, 140);
}

sf::Color chat_input_overlay::category_tint_color(const std::string& cat)
{
    auto c = category_stripe_color(cat);
    return sf::Color(c.r, c.g, c.b, 18);
}

void chat_input_overlay::update_popup_filter()
{
    popup_filtered_.clear();

    // Popup only when input starts with "/" and has no space yet
    if (input_text_.empty() || input_text_[0] != '/')
    {
        popup_visible_ = false;
        return;
    }
    if (input_text_.find(' ') != std::string::npos)
    {
        popup_visible_ = false;
        return;
    }

    // Extract the prefix after "/"
    std::string prefix = input_text_.substr(1);
    // Convert to lowercase for case-insensitive matching
    for (char& c : prefix)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    for (const auto& cmd : commands_)
    {
        if (!cmd.enabled) continue;

        // Match if command name starts with prefix
        if (prefix.empty())
        {
            popup_filtered_.push_back(&cmd);
        }
        else
        {
            std::string lower_name = cmd.name;
            for (char& c : lower_name)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower_name.starts_with(prefix))
                popup_filtered_.push_back(&cmd);
        }
    }

    popup_visible_ = !popup_filtered_.empty();

    // Clamp selection
    if (popup_selected_ >= static_cast<int32_t>(popup_filtered_.size()))
        popup_selected_ = std::max(0, static_cast<int32_t>(popup_filtered_.size()) - 1);

    // Clamp scroll
    int32_t max_scroll = std::max(0, static_cast<int32_t>(popup_filtered_.size()) - popup_max_visible);
    if (popup_scroll_offset_ > max_scroll)
        popup_scroll_offset_ = max_scroll;
}

void chat_input_overlay::select_popup_command()
{
    if (popup_selected_ < 0 || popup_selected_ >= static_cast<int32_t>(popup_filtered_.size()))
        return;

    const auto* cmd = popup_filtered_[popup_selected_];
    input_text_ = "/" + cmd->name + " ";
    cursor_pos_ = input_text_.size();
    popup_visible_ = false;
    popup_selected_ = 0;
    popup_scroll_offset_ = 0;
}

// ============================================================
// Activation / deactivation
// ============================================================

void chat_input_overlay::activate()
{
    active_ = true;
    input_text_.clear();
    cursor_pos_ = 0;
    blink_timer_ = 0.0f;
    cursor_visible_ = true;
    popup_visible_ = false;
    popup_selected_ = 0;
    popup_scroll_offset_ = 0;
    popup_hovered_ = -1;

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
    popup_visible_ = false;
}

// ============================================================
// Update
// ============================================================

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
                    update_popup_filter();
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

    // --- Popup input interception ---
    if (popup_visible_)
    {
        // Mouse hover detection
        int32_t mx = inp.mouse_x();
        int32_t my = inp.mouse_y();
        if (mx >= popup_x_ && mx < popup_x_ + popup_width_ &&
            my >= popup_y_ && my < popup_y_ + popup_height_)
        {
            int32_t row = (my - popup_y_) / popup_row_height + popup_scroll_offset_;
            if (row >= 0 && row < static_cast<int32_t>(popup_filtered_.size()))
                popup_hovered_ = row;
            else
                popup_hovered_ = -1;

            // Mouse click selects
            if (inp.is_mouse_pressed(sf::Mouse::Button::Left) && popup_hovered_ >= 0)
            {
                popup_selected_ = popup_hovered_;
                select_popup_command();
                update_popup_filter();
                return true;
            }
        }
        else
        {
            popup_hovered_ = -1;
        }

        // Mouse wheel scrolls popup
        int32_t wheel = inp.wheel_delta();
        if (wheel != 0)
        {
            popup_scroll_offset_ -= wheel;
            int32_t max_scroll = std::max(0, static_cast<int32_t>(popup_filtered_.size()) - popup_max_visible);
            popup_scroll_offset_ = std::clamp(popup_scroll_offset_, 0, max_scroll);
        }

        // Up/Down navigate selection
        if (inp.is_key_pressed(sf::Keyboard::Key::Up))
        {
            popup_selected_--;
            if (popup_selected_ < 0)
                popup_selected_ = static_cast<int32_t>(popup_filtered_.size()) - 1;
            // Adjust scroll to keep selection visible
            if (popup_selected_ < popup_scroll_offset_)
                popup_scroll_offset_ = popup_selected_;
            if (popup_selected_ >= popup_scroll_offset_ + popup_max_visible)
                popup_scroll_offset_ = popup_selected_ - popup_max_visible + 1;
            return true;
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Down))
        {
            popup_selected_++;
            if (popup_selected_ >= static_cast<int32_t>(popup_filtered_.size()))
                popup_selected_ = 0;
            // Adjust scroll to keep selection visible
            if (popup_selected_ < popup_scroll_offset_)
                popup_scroll_offset_ = popup_selected_;
            if (popup_selected_ >= popup_scroll_offset_ + popup_max_visible)
                popup_scroll_offset_ = popup_selected_ - popup_max_visible + 1;
            return true;
        }

        // Enter or Tab selects highlighted command
        if (inp.is_key_pressed(sf::Keyboard::Key::Enter) ||
            inp.is_key_pressed(sf::Keyboard::Key::Tab))
        {
            select_popup_command();
            update_popup_filter();
            return true;
        }

        // Escape closes popup (not the chat bar)
        if (inp.is_key_pressed(sf::Keyboard::Key::Escape))
        {
            popup_visible_ = false;
            return true;
        }
    }
    else
    {
        // Popup not visible — normal Escape cancels chat
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
    }

    // Navigation keys (always active)
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

    // Update popup filter after any text change
    update_popup_filter();

    return true;  // Consuming all input while active
}

// ============================================================
// Render
// ============================================================

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

    // --- Command autocomplete popup ---
    if (popup_visible_ && !popup_filtered_.empty())
    {
        int32_t visible_count = std::min(static_cast<int32_t>(popup_filtered_.size()), popup_max_visible);
        int32_t ph = visible_count * popup_row_height;
        int32_t px = bar_x;
        int32_t py = bar_y - ph - 2;
        int32_t pw = bar_width;

        // Cache geometry for mouse hit testing
        popup_x_ = px;
        popup_y_ = py;
        popup_width_ = pw;
        popup_height_ = ph;

        // Background
        rend.draw_rect(px, py, pw, ph, sf::Color(20, 20, 30, 230), true);
        rend.draw_rect(px, py, pw, ph, sf::Color(60, 60, 80), false);

        constexpr int32_t stripe_width = 3;
        constexpr int32_t name_offset_x = stripe_width + 6;

        for (int32_t i = 0; i < visible_count; ++i)
        {
            int32_t idx = i + popup_scroll_offset_;
            if (idx < 0 || idx >= static_cast<int32_t>(popup_filtered_.size())) break;

            const auto* cmd = popup_filtered_[idx];
            int32_t row_y = py + i * popup_row_height;

            // Category background tint
            rend.draw_rect(px + 1, row_y, pw - 2, popup_row_height,
                category_tint_color(cmd->category), true);

            // Selection or hover highlight
            if (idx == popup_selected_)
            {
                rend.draw_rect(px + 1, row_y, pw - 2, popup_row_height,
                    sf::Color(60, 60, 120, 200), true);
            }
            else if (idx == popup_hovered_)
            {
                rend.draw_rect(px + 1, row_y, pw - 2, popup_row_height,
                    sf::Color(50, 50, 90, 150), true);
            }

            // Category color stripe (left edge)
            rend.draw_rect(px + 1, row_y, stripe_width, popup_row_height,
                category_stripe_color(cmd->category), true);

            int32_t text_y = row_y + 3;

            // Command name
            std::string name_str = "/" + cmd->name;
            rend.draw_text(name_str, px + name_offset_x, text_y,
                sf::Color::White, font_size);

            // Description (after command name, with gap)
            float name_w = rend.text().measure_width(name_str, font_size);
            int32_t desc_x = px + name_offset_x + static_cast<int32_t>(name_w) + 12;
            rend.draw_text(cmd->description, desc_x, text_y,
                sf::Color(160, 160, 160), font_size);

            // Usage (right-aligned)
            float usage_w = rend.text().measure_width(cmd->usage, font_size);
            int32_t usage_x = px + pw - static_cast<int32_t>(usage_w) - 8;
            // Only draw if it wouldn't overlap description
            if (usage_x > desc_x + 40)
            {
                rend.draw_text(cmd->usage, usage_x, text_y,
                    sf::Color(100, 100, 100), font_size);
            }
        }

        // Scroll indicator: draw a subtle line if there are more items above/below
        if (popup_scroll_offset_ > 0)
        {
            rend.draw_line(px + 4, py + 1, px + pw - 4, py + 1, sf::Color(100, 100, 140));
        }
        int32_t total = static_cast<int32_t>(popup_filtered_.size());
        if (popup_scroll_offset_ + popup_max_visible < total)
        {
            rend.draw_line(px + 4, py + ph - 1, px + pw - 4, py + ph - 1, sf::Color(100, 100, 140));
        }
    }
}

// ============================================================
// Chat logic (unchanged)
// ============================================================

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
