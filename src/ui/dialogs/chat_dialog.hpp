#pragma once

#include "ui/dialog_base.hpp"
#include "chat/chat_message.hpp"
#include <deque>
#include <string>
#include <vector>

namespace hb {

class chat_dialog : public dialog
{
public:
    static constexpr size_t max_messages = 500;
    static constexpr int32_t line_height = 16;

    enum class chat_tab : uint8_t
    {
        all,
        global,
        trade,
        town,
        nearby,
        guild,
        party,
        drops,
        misc
    };
    static constexpr int tab_count = 9;

    chat_dialog();
    ~chat_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;
    bool handle_key_press(sf::Keyboard::Key key) override;
    bool handle_text_input(char32_t unicode) override;

    void add_message(const chat_message& msg);
    void add_system_message(std::string_view text);
    void add_whisper(std::string_view sender, std::string_view text);
    void clear_messages();

    void set_active_tab(chat_tab tab);
    chat_tab active_tab() const { return active_tab_; }

    bool is_search_focused() const { return search_focused_; }

private:
    bool should_show_message(const chat_message& msg) const;
    bool matches_search(const chat_message& msg) const;
    bool search_bar_visible() const { return search_open_ || !search_text_.empty(); }

    std::deque<chat_message> messages_;
    int32_t scroll_offset_ = 0;
    float elapsed_time_ = 0.0f;

    // Tabs
    chat_tab active_tab_ = chat_tab::all;
    int32_t hovered_tab_ = -1;
    int32_t tabs_y_ = 0;

    // Resize
    bool resizing_ = false;
    int32_t resize_start_x_ = 0, resize_start_y_ = 0;
    int32_t resize_start_w_ = 0, resize_start_h_ = 0;
    static constexpr int32_t resize_handle_size = 12;
    // 4px pad + 9 tabs * 42px + 8 gaps * 2px + 4px pad
    static constexpr int32_t min_width = 402;
    static constexpr int32_t min_height = 120;
    static constexpr int32_t max_width = 800;
    static constexpr int32_t max_height = 500;

    // Search: open = bar visible, focused = accepting input
    bool search_open_ = false;
    bool search_focused_ = false;
    bool search_name_only_ = false;  // When true, search matches sender name only
    std::string search_text_;
    size_t search_cursor_ = 0;
    float search_blink_timer_ = 0.0f;
    bool search_cursor_visible_ = true;

    // Tab completion
    std::vector<std::string> recent_senders_;
};

} // namespace hb
