#include "ui/dialogs/chat_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include "core/config.hpp"
#include <algorithm>
#include <cctype>
#include <ctime>

namespace hb {

namespace {

constexpr std::string_view tab_labels[] = {
    "All", "Global", "Trade", "Town", "Nearby", "Guild", "Party", "Drops", "Misc"
};

constexpr int32_t tab_width = 42;
constexpr int32_t tab_height = 18;
constexpr int32_t tab_gap = 2;
constexpr int32_t search_bar_height = 24;
constexpr int32_t search_icon_size = 16;

bool ci_contains(std::string_view haystack, std::string_view needle)
{
    if (needle.empty()) return true;
    if (haystack.size() < needle.size()) return false;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) ==
                                     std::tolower(static_cast<unsigned char>(b)); });
    return it != haystack.end();
}

std::string format_timestamp(std::chrono::system_clock::time_point tp)
{
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val{};
#ifdef _WIN32
    localtime_s(&tm_val, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_val);
#endif
    char buf[8];
    std::snprintf(buf, sizeof(buf), "[%02d:%02d]", tm_val.tm_hour, tm_val.tm_min);
    return buf;
}

} // anonymous namespace

chat_dialog::chat_dialog()
    : dialog(dialog_type::chat)
{
    set_title("Chat");
    set_bounds({0, static_cast<int32_t>(screen_height) - 200, min_width, 200});
    set_draggable(true);
    set_closeable(true);
    set_visible(true);
    set_has_border(false);
    set_drag_clamp(drag_clamp::partial);
}

void chat_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
    elapsed_time_ += delta_time;

    if (search_focused_)
    {
        search_blink_timer_ += delta_time;
        if (search_blink_timer_ >= 0.5f)
        {
            search_blink_timer_ = 0.0f;
            search_cursor_visible_ = !search_cursor_visible_;
        }
    }

    // Mouse wheel scrolling when hovered
    if (bounds_.contains(inp.mouse_x(), inp.mouse_y()))
    {
        int32_t wheel = inp.wheel_delta();
        if (wheel != 0)
        {
            scroll_offset_ += wheel * 3;
            if (scroll_offset_ < 0) scroll_offset_ = 0;

            // Clamp to max
            int32_t total = 0;
            for (auto& m : messages_)
                if (should_show_message(m)) total++;
            int32_t msg_area_top = tabs_y_ + tab_height + 2;
            int32_t msg_area_bottom = bounds_.y + bounds_.height
                - (search_bar_visible() ? search_bar_height : 0);
            int32_t visible_lines = std::max(1, (msg_area_bottom - msg_area_top) / line_height);
            int32_t max_scroll = std::max(0, total - visible_lines);
            if (scroll_offset_ > max_scroll) scroll_offset_ = max_scroll;
        }
    }
}

void chat_dialog::render(renderer& rend)
{
    if (!visible_) return;

    bool bar_visible = search_bar_visible();

    // Background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(0, 0, 0, 120), true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(60, 60, 80, 160), false);

    // Title bar
    render_title_bar(rend);

    // Tab bar
    tabs_y_ = bounds_.y + title_bar_height;
    for (int i = 0; i < tab_count; ++i)
    {
        int32_t tx = bounds_.x + 4 + i * (tab_width + tab_gap);
        int32_t ty = tabs_y_;

        sf::Color bg;
        sf::Color text_color;
        if (i == static_cast<int>(active_tab_))
        {
            bg = sf::Color(80, 80, 120);
            text_color = sf::Color(255, 255, 100);
        }
        else if (i == hovered_tab_)
        {
            bg = sf::Color(60, 60, 85);
            text_color = sf::Color::White;
        }
        else
        {
            bg = sf::Color(50, 50, 70);
            text_color = sf::Color::White;
        }

        rend.draw_rect(tx, ty, tab_width, tab_height, bg, true);
        // Center text in tab
        auto label = tab_labels[i];
        int32_t text_w = static_cast<int32_t>(label.size()) * 6;
        int32_t text_x = tx + (tab_width - text_w) / 2;
        rend.draw_text(std::string(label), text_x, ty + 3, text_color, 11);
    }

    // Message area
    int32_t msg_area_top = tabs_y_ + tab_height + 2;
    int32_t msg_area_bottom = bounds_.y + bounds_.height
        - (bar_visible ? search_bar_height : 0) - resize_handle_size;
    int32_t msg_area_height = msg_area_bottom - msg_area_top;
    int32_t visible_lines = std::max(1, msg_area_height / line_height);

    // Drops/Misc placeholder
    if (active_tab_ == chat_tab::drops || active_tab_ == chat_tab::misc)
    {
        std::string placeholder = "Coming soon";
        int32_t text_w = static_cast<int32_t>(placeholder.size()) * 7;
        int32_t cx = bounds_.x + (bounds_.width - text_w) / 2;
        int32_t cy = msg_area_top + msg_area_height / 2 - 6;
        rend.draw_text(placeholder, cx, cy, sf::Color(120, 120, 140), 12);
    }
    else
    {
        // Collect filtered messages
        std::vector<size_t> filtered;
        filtered.reserve(messages_.size());
        for (size_t i = 0; i < messages_.size(); ++i)
        {
            if (should_show_message(messages_[i]))
                filtered.push_back(i);
        }

        int32_t total = static_cast<int32_t>(filtered.size());
        int32_t start_idx = std::max(0, total - visible_lines - scroll_offset_);
        int32_t end_idx = std::max(0, total - scroll_offset_);

        int32_t line_y = msg_area_top;
        for (int32_t fi = start_idx; fi < end_idx && line_y + line_height <= msg_area_bottom; ++fi)
        {
            const auto& msg = messages_[filtered[fi]];

            std::string display_text;
            if (config::instance().chat().show_timestamps
                && msg.timestamp.time_since_epoch().count() > 0)
            {
                display_text = format_timestamp(msg.timestamp) + " " + msg.formatted();
            }
            else
            {
                display_text = msg.formatted();
            }

            // Truncate if too long
            size_t max_chars = static_cast<size_t>((bounds_.width - 16) / 6);
            if (display_text.length() > max_chars)
            {
                display_text = display_text.substr(0, max_chars - 3) + "...";
            }

            auto style = get_chat_bubble_style(msg.type);
            style.size = 12;
            if (style.effect != text_effect::none)
            {
                rend.text().draw(display_text, bounds_.x + 8, line_y, style, elapsed_time_);
            }
            else
            {
                rend.draw_text(display_text, bounds_.x + 8, line_y, style.color, 12);
            }
            line_y += line_height;
        }

        // Scroll indicator
        if (total > visible_lines)
        {
            int32_t scroll_x = bounds_.x + bounds_.width - 10;
            int32_t scroll_y = msg_area_top;
            int32_t scroll_h = msg_area_height;

            rend.draw_rect(scroll_x, scroll_y, 4, scroll_h, sf::Color(30, 30, 40), true);

            float visible_ratio = static_cast<float>(visible_lines) / total;
            int32_t thumb_height = std::max(12, static_cast<int32_t>(scroll_h * visible_ratio));

            int32_t max_scroll = total - visible_lines;
            float scroll_ratio = max_scroll > 0 ? static_cast<float>(scroll_offset_) / max_scroll : 0.0f;
            int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_h - thumb_height) * (1.0f - scroll_ratio));

            rend.draw_rect(scroll_x, thumb_y, 4, thumb_height, sf::Color(80, 80, 100), true);
        }
    }

    // Search icon (bottom-left, only when bar is hidden)
    if (!bar_visible)
    {
        int32_t icon_x = bounds_.x + 6;
        int32_t icon_y = bounds_.y + bounds_.height - resize_handle_size - search_icon_size - 4;
        // Draw magnifying glass: square + line
        int32_t cx = icon_x + 6;
        int32_t cy = icon_y + 6;
        rend.draw_rect(cx - 4, cy - 4, 9, 9, sf::Color(150, 150, 170), false);
        rend.draw_line(cx + 3, cy + 3, cx + 6, cy + 6, sf::Color(150, 150, 170));
    }

    // Search bar (when visible)
    if (bar_visible)
    {
        int32_t sb_y = bounds_.y + bounds_.height - resize_handle_size - search_bar_height;
        int32_t sb_x = bounds_.x + 4;
        int32_t sb_w = bounds_.width - 8;

        // Focused vs unfocused appearance
        sf::Color bar_bg = search_focused_ ? sf::Color(20, 20, 30, 220) : sf::Color(15, 15, 22, 180);
        sf::Color bar_border = search_focused_ ? sf::Color(80, 80, 120) : sf::Color(50, 50, 70);
        sf::Color text_color = search_focused_ ? sf::Color::White : sf::Color(160, 160, 180);

        rend.draw_rect(sb_x, sb_y, sb_w, search_bar_height, bar_bg, true);
        rend.draw_rect(sb_x, sb_y, sb_w, search_bar_height, bar_border, false);

        // Search text
        constexpr uint32_t search_font_size = 12;
        int32_t text_x = sb_x + 6;
        float avail_w = static_cast<float>(sb_w - 62);

        // Compute start_char so cursor stays visible
        size_t start_char = 0;
        if (search_focused_ && !search_text_.empty())
        {
            // Measure text from 0 to cursor; if it exceeds avail_w, advance start_char
            while (start_char < search_cursor_)
            {
                auto sub = std::string_view(search_text_).substr(start_char, search_cursor_ - start_char);
                if (rend.text().measure_width(sub, search_font_size) <= avail_w)
                    break;
                start_char++;
            }
        }

        // Build visible substring that fits
        size_t end_char = search_cursor_;
        while (end_char < search_text_.size())
        {
            auto sub = std::string_view(search_text_).substr(start_char, end_char + 1 - start_char);
            if (rend.text().measure_width(sub, search_font_size) > avail_w)
                break;
            end_char++;
        }
        std::string vis_text = search_text_.substr(start_char, end_char - start_char);

        if (search_text_.empty())
        {
            rend.draw_text("Search...", text_x, sb_y + 5, sf::Color(100, 100, 120), search_font_size);
        }
        else
        {
            rend.draw_text(vis_text, text_x, sb_y + 5, text_color, search_font_size);
        }

        // Cursor (only when focused)
        if (search_focused_ && search_cursor_visible_)
        {
            auto cursor_sub = std::string_view(search_text_).substr(start_char, search_cursor_ - start_char);
            int32_t cursor_x = text_x + static_cast<int32_t>(rend.text().measure_width(cursor_sub, search_font_size));
            rend.draw_line(cursor_x, sb_y + 4, cursor_x, sb_y + search_bar_height - 4, sf::Color::White);
        }

        // Name-only toggle button (right of text, before X)
        int32_t toggle_x = sb_x + sb_w - 50;
        int32_t toggle_y = sb_y + 3;
        sf::Color toggle_bg = search_name_only_ ? sf::Color(70, 70, 110) : sf::Color(35, 35, 50);
        sf::Color toggle_text = search_name_only_ ? sf::Color(255, 255, 100) : sf::Color(120, 120, 140);
        rend.draw_rect(toggle_x, toggle_y, 28, search_bar_height - 6, toggle_bg, true);
        rend.draw_text("@", toggle_x + 10, toggle_y + 2, toggle_text, 11);

        // X close button (clears filter and hides bar)
        int32_t close_x = sb_x + sb_w - 18;
        int32_t close_y = sb_y + 4;
        rend.draw_text("x", close_x, close_y, sf::Color(180, 180, 180), 12);
    }

    // Resize handle (bottom-right diagonal grip lines)
    int32_t rx = bounds_.x + bounds_.width - resize_handle_size;
    int32_t ry = bounds_.y + bounds_.height - resize_handle_size;
    sf::Color grip_color(100, 100, 120);
    rend.draw_line(rx + 3, ry + resize_handle_size - 1, rx + resize_handle_size - 1, ry + 3, grip_color);
    rend.draw_line(rx + 6, ry + resize_handle_size - 1, rx + resize_handle_size - 1, ry + 6, grip_color);
    rend.draw_line(rx + 9, ry + resize_handle_size - 1, rx + resize_handle_size - 1, ry + 9, grip_color);
}

bool chat_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_) return false;
    if (!bounds_.contains(x, y)) return false;

    // Resize handle
    int32_t rx = bounds_.x + bounds_.width - resize_handle_size;
    int32_t ry = bounds_.y + bounds_.height - resize_handle_size;
    if (x >= rx && y >= ry)
    {
        resizing_ = true;
        resize_start_x_ = x;
        resize_start_y_ = y;
        resize_start_w_ = bounds_.width;
        resize_start_h_ = bounds_.height;
        return true;
    }

    // Tab bar
    if (y >= tabs_y_ && y < tabs_y_ + tab_height)
    {
        for (int i = 0; i < tab_count; ++i)
        {
            int32_t tx = bounds_.x + 4 + i * (tab_width + tab_gap);
            if (x >= tx && x < tx + tab_width)
            {
                set_active_tab(static_cast<chat_tab>(i));
                return true;
            }
        }
    }

    bool bar_visible = search_bar_visible();

    // Search bar area
    if (bar_visible)
    {
        int32_t sb_y = bounds_.y + bounds_.height - resize_handle_size - search_bar_height;
        int32_t sb_x = bounds_.x + 4;
        int32_t sb_w = bounds_.width - 8;

        if (y >= sb_y && y < sb_y + search_bar_height && x >= sb_x && x < sb_x + sb_w)
        {
            // X close button — clears text and closes bar
            int32_t close_x = sb_x + sb_w - 18;
            if (x >= close_x && x < close_x + 14)
            {
                search_open_ = false;
                search_focused_ = false;
                search_text_.clear();
                search_name_only_ = false;
                return true;
            }

            // Name-only toggle button
            int32_t toggle_x = sb_x + sb_w - 50;
            if (x >= toggle_x && x < toggle_x + 28)
            {
                search_name_only_ = !search_name_only_;
                return true;
            }

            // Click on the bar itself — focus it
            search_focused_ = true;
            search_blink_timer_ = 0.0f;
            search_cursor_visible_ = true;
            return true;
        }
    }

    // Search icon click (when bar is hidden)
    if (!bar_visible)
    {
        int32_t icon_x = bounds_.x + 6;
        int32_t icon_y = bounds_.y + bounds_.height - resize_handle_size - search_icon_size - 4;
        if (x >= icon_x && x < icon_x + search_icon_size && y >= icon_y && y < icon_y + search_icon_size)
        {
            search_open_ = true;
            search_focused_ = true;
            search_text_.clear();
            search_cursor_ = 0;
            search_blink_timer_ = 0.0f;
            search_cursor_visible_ = true;
            return true;
        }
    }

    // Click elsewhere in dialog unfocuses search
    search_focused_ = false;

    return dialog::handle_mouse_down(x, y, btn);
}

bool chat_dialog::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (resizing_)
    {
        resizing_ = false;
        return true;
    }
    return dialog::handle_mouse_up(x, y, btn);
}

bool chat_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (resizing_)
    {
        int32_t dx = x - resize_start_x_;
        int32_t dy = y - resize_start_y_;
        int32_t new_w = std::clamp(resize_start_w_ + dx, min_width, max_width);
        int32_t new_h = std::clamp(resize_start_h_ + dy, min_height, max_height);
        set_bounds({bounds_.x, bounds_.y, new_w, new_h});
        return true;
    }

    // Tab hover
    hovered_tab_ = -1;
    if (y >= tabs_y_ && y < tabs_y_ + tab_height)
    {
        for (int i = 0; i < tab_count; ++i)
        {
            int32_t tx = bounds_.x + 4 + i * (tab_width + tab_gap);
            if (x >= tx && x < tx + tab_width)
            {
                hovered_tab_ = i;
                break;
            }
        }
    }

    return dialog::handle_mouse_move(x, y);
}

bool chat_dialog::handle_key_press(sf::Keyboard::Key key)
{
    if (!search_focused_)
    {
        // PageUp/PageDown for scrolling when not focused on search
        if (key == sf::Keyboard::Key::PageUp)
        {
            scroll_offset_ += 5;
            return true;
        }
        if (key == sf::Keyboard::Key::PageDown)
        {
            scroll_offset_ = std::max(0, scroll_offset_ - 5);
            return true;
        }
        return false;
    }

    // Enter/Escape unfocus but keep text and bar visible
    if (key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Enter)
    {
        search_focused_ = false;
        return true;
    }

    if (key == sf::Keyboard::Key::Backspace && search_cursor_ > 0)
    {
        search_text_.erase(search_cursor_ - 1, 1);
        search_cursor_--;
        search_cursor_visible_ = true;
        search_blink_timer_ = 0.0f;
        return true;
    }

    if (key == sf::Keyboard::Key::Delete && search_cursor_ < search_text_.size())
    {
        search_text_.erase(search_cursor_, 1);
        return true;
    }

    if (key == sf::Keyboard::Key::Left && search_cursor_ > 0)
    {
        search_cursor_--;
        return true;
    }

    if (key == sf::Keyboard::Key::Right && search_cursor_ < search_text_.size())
    {
        search_cursor_++;
        return true;
    }

    if (key == sf::Keyboard::Key::Home)
    {
        search_cursor_ = 0;
        return true;
    }

    if (key == sf::Keyboard::Key::End)
    {
        search_cursor_ = search_text_.size();
        return true;
    }

    return false;
}

bool chat_dialog::handle_text_input(char32_t unicode)
{
    if (!search_focused_) return false;

    if (unicode < 32 || unicode == 127) return false;

    if (unicode < 128 && search_text_.size() < 64)
    {
        search_text_.insert(search_cursor_, 1, static_cast<char>(unicode));
        search_cursor_++;
        search_cursor_visible_ = true;
        search_blink_timer_ = 0.0f;
        return true;
    }

    return false;
}

void chat_dialog::add_message(const chat_message& msg)
{
    messages_.push_back(msg);

    // Track senders for future tab completion
    if (!msg.sender.empty())
    {
        auto it = std::find(recent_senders_.begin(), recent_senders_.end(), msg.sender);
        if (it != recent_senders_.end())
            recent_senders_.erase(it);
        recent_senders_.insert(recent_senders_.begin(), msg.sender);
        if (recent_senders_.size() > 20)
            recent_senders_.pop_back();
    }

    while (messages_.size() > max_messages)
        messages_.pop_front();

    // Auto-scroll to bottom
    scroll_offset_ = 0;
}

void chat_dialog::add_system_message(std::string_view text)
{
    chat_message msg;
    msg.type = chat_type::system;
    msg.content = std::string(text);
    msg.timestamp = std::chrono::system_clock::now();
    add_message(msg);
}

void chat_dialog::add_whisper(std::string_view sender, std::string_view text)
{
    chat_message msg;
    msg.type = chat_type::whisper;
    msg.sender = std::string(sender);
    msg.content = std::string(text);
    msg.timestamp = std::chrono::system_clock::now();
    add_message(msg);
}

void chat_dialog::clear_messages()
{
    messages_.clear();
    scroll_offset_ = 0;
}

void chat_dialog::set_active_tab(chat_tab tab)
{
    active_tab_ = tab;
    scroll_offset_ = 0;
}

bool chat_dialog::should_show_message(const chat_message& msg) const
{
    bool type_ok = false;
    switch (active_tab_)
    {
        case chat_tab::all:     type_ok = true; break;
        case chat_tab::global:  type_ok = (msg.type == chat_type::shout ||
                                           msg.type == chat_type::gm ||
                                           msg.type == chat_type::system); break;
        case chat_tab::trade:   type_ok = (msg.type == chat_type::trade); break;
        case chat_tab::town:    type_ok = (msg.type == chat_type::faction); break;
        case chat_tab::nearby:  type_ok = (msg.type == chat_type::normal ||
                                           msg.type == chat_type::emote); break;
        case chat_tab::guild:   type_ok = (msg.type == chat_type::guild); break;
        case chat_tab::party:   type_ok = (msg.type == chat_type::party); break;
        case chat_tab::drops:   return false;
        case chat_tab::misc:    return false;
    }
    if (!type_ok) return false;

    if (!search_text_.empty())
        return matches_search(msg);

    return true;
}

bool chat_dialog::matches_search(const chat_message& msg) const
{
    if (search_name_only_)
        return ci_contains(msg.sender, search_text_);
    return ci_contains(msg.sender, search_text_) || ci_contains(msg.content, search_text_);
}

} // namespace hb
