#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>

namespace hb {

class renderer;
class input;

// UI element rectangle
struct ui_rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    bool contains(int32_t px, int32_t py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

// Base UI element
class ui_element {
public:
    ui_element() = default;
    virtual ~ui_element() = default;

    ui_element(const ui_element&) = delete;
    ui_element& operator=(const ui_element&) = delete;
    ui_element(ui_element&&) noexcept = default;
    ui_element& operator=(ui_element&&) noexcept = default;

    // Lifecycle
    virtual void update(float delta_time, const input& inp);
    virtual void render(renderer& rend);

    // Input handling - return true if handled
    virtual bool handle_mouse_move(int32_t x, int32_t y);
    virtual bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn);
    virtual bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn);
    virtual bool handle_key_press(sf::Keyboard::Key key);
    virtual bool handle_text_input(char32_t unicode);

    // Position and size
    void set_position(int32_t x, int32_t y);
    void set_size(int32_t width, int32_t height);
    void set_bounds(const ui_rect& rect);
    const ui_rect& bounds() const { return bounds_; }

    // State
    void set_visible(bool visible) { visible_ = visible; }
    bool visible() const { return visible_; }

    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }

    bool hovered() const { return hovered_; }
    bool focused() const { return focused_; }
    void set_focused(bool focused) { focused_ = focused; }

    // ID
    void set_id(std::string_view id) { id_ = id; }
    std::string_view id() const { return id_; }

    // Hierarchy
    void add_child(std::unique_ptr<ui_element> child);
    void remove_child(ui_element* child);
    void clear_children();
    const std::vector<std::unique_ptr<ui_element>>& children() const { return children_; }
    ui_element* parent() const { return parent_; }

    // Find child by ID
    ui_element* find_child(std::string_view id);

protected:
    bool contains_point(int32_t x, int32_t y) const;
    void update_children(float delta_time, const input& inp);
    void render_children(renderer& rend);

    // Get absolute screen position by traversing parent hierarchy
    void get_absolute_position(int32_t& abs_x, int32_t& abs_y) const;

    // Check if screen-space point is within this element's absolute bounds
    bool contains_point_absolute(int32_t screen_x, int32_t screen_y) const;

    ui_rect bounds_;
    bool visible_ = true;
    bool enabled_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    std::string id_;

    ui_element* parent_ = nullptr;
    std::vector<std::unique_ptr<ui_element>> children_;
};

// Panel - container with optional background
class ui_panel : public ui_element {
public:
    void render(renderer& rend) override;

    void set_background_color(sf::Color color) { bg_color_ = color; }
    void set_border_color(sf::Color color) { border_color_ = color; }
    void set_has_border(bool border) { has_border_ = border; }

protected:
    sf::Color bg_color_ = sf::Color(40, 40, 60, 220);
    sf::Color border_color_ = sf::Color(100, 100, 140);
    bool has_border_ = true;
};

// Label - text display
class ui_label : public ui_element {
public:
    void render(renderer& rend) override;

    void set_text(std::string_view text) { text_ = text; }
    std::string_view text() const { return text_; }

    void set_text_color(sf::Color color) { text_color_ = color; }
    void set_font_size(uint32_t size) { font_size_ = size; }

    enum class alignment { left, center, right };
    void set_alignment(alignment align) { alignment_ = align; }

protected:
    std::string text_;
    sf::Color text_color_ = sf::Color::White;
    uint32_t font_size_ = 14;
    alignment alignment_ = alignment::left;
};

// Button - clickable element
class ui_button : public ui_element {
public:
    using click_callback = std::function<void()>;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    void set_text(std::string_view text) { text_ = text; }
    void set_on_click(click_callback callback) { on_click_ = std::move(callback); }

    void set_normal_color(sf::Color color) { normal_color_ = color; }
    void set_hover_color(sf::Color color) { hover_color_ = color; }
    void set_pressed_color(sf::Color color) { pressed_color_ = color; }
    void set_disabled_color(sf::Color color) { disabled_color_ = color; }

protected:
    std::string text_;
    click_callback on_click_;
    bool pressed_ = false;

    sf::Color normal_color_ = sf::Color(60, 60, 80);
    sf::Color hover_color_ = sf::Color(80, 80, 100);
    sf::Color pressed_color_ = sf::Color(40, 40, 60);
    sf::Color disabled_color_ = sf::Color(40, 40, 40);
    sf::Color text_color_ = sf::Color::White;
};

// Text input field
class ui_text_input : public ui_element {
public:
    using change_callback = std::function<void(std::string_view)>;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_key_press(sf::Keyboard::Key key) override;
    bool handle_text_input(char32_t unicode) override;

    void set_text(std::string_view text);
    std::string_view text() const { return text_; }

    void set_placeholder(std::string_view placeholder) { placeholder_ = placeholder; }
    void set_max_length(size_t len) { max_length_ = len; }
    void set_password_mode(bool password) { password_mode_ = password; }
    void set_on_change(change_callback callback) { on_change_ = std::move(callback); }

protected:
    std::string text_;
    std::string placeholder_;
    change_callback on_change_;
    size_t max_length_ = 256;
    size_t cursor_pos_ = 0;
    bool password_mode_ = false;
    float cursor_blink_timer_ = 0.0f;
    bool cursor_visible_ = true;

    sf::Color bg_color_ = sf::Color(30, 30, 40);
    sf::Color border_color_ = sf::Color(80, 80, 100);
    sf::Color text_color_ = sf::Color::White;
    sf::Color placeholder_color_ = sf::Color(100, 100, 100);
};

// Progress bar
class ui_progress_bar : public ui_element {
public:
    void render(renderer& rend) override;

    void set_value(float value);  // 0.0 to 1.0
    float value() const { return value_; }

    void set_min_max(float min_val, float max_val);
    void set_current(float current);  // Will be clamped to min/max

    void set_fill_color(sf::Color color) { fill_color_ = color; }
    void set_background_color(sf::Color color) { bg_color_ = color; }
    void set_show_text(bool show) { show_text_ = show; }

protected:
    float value_ = 0.0f;
    float min_value_ = 0.0f;
    float max_value_ = 1.0f;

    sf::Color fill_color_ = sf::Color(100, 200, 100);
    sf::Color bg_color_ = sf::Color(40, 40, 40);
    bool show_text_ = false;
};

// Scrollbar
class ui_scrollbar : public ui_element {
public:
    using scroll_callback = std::function<void(float)>;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    void set_value(float value);  // 0.0 to 1.0
    float value() const { return value_; }

    void set_thumb_size(float size) { thumb_size_ = size; }
    void set_on_scroll(scroll_callback callback) { on_scroll_ = std::move(callback); }
    void set_vertical(bool vertical) { vertical_ = vertical; }

protected:
    float value_ = 0.0f;
    float thumb_size_ = 0.2f;
    bool vertical_ = true;
    bool dragging_ = false;
    int32_t drag_offset_ = 0;

    scroll_callback on_scroll_;

    sf::Color track_color_ = sf::Color(40, 40, 50);
    sf::Color thumb_color_ = sf::Color(80, 80, 100);
    sf::Color thumb_hover_color_ = sf::Color(100, 100, 120);
};

// List box
class ui_list_box : public ui_element {
public:
    using select_callback = std::function<void(int32_t)>;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    void add_item(std::string_view text);
    void remove_item(int32_t index);
    void clear_items();

    int32_t selected_index() const { return selected_index_; }
    void set_selected_index(int32_t index);
    std::string_view selected_text() const;

    void set_on_select(select_callback callback) { on_select_ = std::move(callback); }
    void set_item_height(int32_t height) { item_height_ = height; }

protected:
    std::vector<std::string> items_;
    int32_t selected_index_ = -1;
    int32_t scroll_offset_ = 0;
    int32_t item_height_ = 20;

    select_callback on_select_;

    sf::Color bg_color_ = sf::Color(30, 30, 40);
    sf::Color item_color_ = sf::Color::White;
    sf::Color selected_color_ = sf::Color(60, 100, 160);
    sf::Color hover_color_ = sf::Color(50, 50, 70);
};

// Dropdown - collapsible selection widget
class ui_dropdown : public ui_element {
public:
    using select_callback = std::function<void(int32_t)>;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;
    bool handle_key_press(sf::Keyboard::Key key) override;

    void add_item(std::string_view text);
    void remove_item(int32_t index);
    void clear_items();

    int32_t selected_index() const { return selected_index_; }
    void set_selected_index(int32_t index);
    std::string_view selected_text() const;
    const std::vector<std::string>& items() const { return items_; }

    void set_on_select(select_callback callback) { on_select_ = std::move(callback); }
    void set_item_height(int32_t height) { item_height_ = height; }
    void set_max_visible_items(int32_t count) { max_visible_items_ = count; }
    void set_placeholder(std::string_view placeholder) { placeholder_ = placeholder; }

    bool is_expanded() const { return expanded_; }
    bool is_animating() const { return animating_; }
    void expand();
    void collapse();

    // Animation settings
    void set_animation_speed(float speed) { animation_speed_ = speed; }
    void set_animation_enabled(bool enabled) { animation_enabled_ = enabled; }

    // Get the expanded bounds (header + list) for hit testing
    ui_rect expanded_bounds() const;

protected:
    // Check if point is in header (using local coordinates relative to parent)
    bool point_in_header(int32_t x, int32_t y) const;

    // Check if point is in list area (using local coordinates relative to parent)
    bool point_in_list(int32_t x, int32_t y) const;

    // Get item index at position (using local coordinates), returns -1 if not in list
    int32_t item_at_position(int32_t x, int32_t y) const;

    std::vector<std::string> items_;
    int32_t selected_index_ = -1;
    int32_t hovered_index_ = -1;
    int32_t scroll_offset_ = 0;
    int32_t item_height_ = 24;
    int32_t max_visible_items_ = 5;
    bool expanded_ = false;
    std::string placeholder_ = "Select...";

    // Animation state
    bool animation_enabled_ = true;
    bool animating_ = false;
    float animation_progress_ = 0.0f;  // 0.0 = closed, 1.0 = fully open
    float animation_speed_ = 8.0f;     // Speed multiplier

    select_callback on_select_;

    // Colors
    sf::Color header_color_ = sf::Color(50, 50, 70);
    sf::Color header_hover_color_ = sf::Color(60, 60, 80);
    sf::Color list_bg_color_ = sf::Color(35, 35, 50);
    sf::Color item_color_ = sf::Color::White;
    sf::Color item_hover_color_ = sf::Color(60, 80, 120);
    sf::Color item_selected_color_ = sf::Color(50, 100, 160);
    sf::Color border_color_ = sf::Color(80, 80, 100);
    sf::Color arrow_color_ = sf::Color(180, 180, 200);
};

} // namespace hb
