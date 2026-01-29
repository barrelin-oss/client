#include "ui/ui_element.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include <algorithm>

namespace hb {

// ui_element implementation

void ui_element::update(float delta_time, const input& inp) {
    if (!visible_ || !enabled_) return;
    update_children(delta_time, inp);
}

void ui_element::render(renderer& rend) {
    if (!visible_) return;
    render_children(rend);
}

bool ui_element::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_ || !enabled_) return false;

    hovered_ = contains_point(x, y);

    // Check children in reverse order (top to bottom)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->handle_mouse_move(x, y)) {
            return true;
        }
    }

    return hovered_;
}

bool ui_element::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->handle_mouse_down(x, y, btn)) {
            return true;
        }
    }

    return contains_point(x, y);
}

bool ui_element::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->handle_mouse_up(x, y, btn)) {
            return true;
        }
    }

    return contains_point(x, y);
}

bool ui_element::handle_key_press(sf::Keyboard::Key key) {
    if (!visible_ || !enabled_ || !focused_) return false;

    for (auto& child : children_) {
        if (child->handle_key_press(key)) {
            return true;
        }
    }

    return false;
}

bool ui_element::handle_text_input(char32_t unicode) {
    if (!visible_ || !enabled_ || !focused_) return false;

    for (auto& child : children_) {
        if (child->handle_text_input(unicode)) {
            return true;
        }
    }

    return false;
}

void ui_element::set_position(int32_t x, int32_t y) {
    bounds_.x = x;
    bounds_.y = y;
}

void ui_element::set_size(int32_t width, int32_t height) {
    bounds_.width = width;
    bounds_.height = height;
}

void ui_element::set_bounds(const ui_rect& rect) {
    bounds_ = rect;
}

void ui_element::add_child(std::unique_ptr<ui_element> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

void ui_element::remove_child(ui_element* child) {
    children_.erase(
        std::remove_if(children_.begin(), children_.end(),
            [child](const std::unique_ptr<ui_element>& e) { return e.get() == child; }),
        children_.end()
    );
}

void ui_element::clear_children() {
    children_.clear();
}

ui_element* ui_element::find_child(std::string_view id) {
    for (auto& child : children_) {
        if (child->id() == id) {
            return child.get();
        }
        if (auto* found = child->find_child(id)) {
            return found;
        }
    }
    return nullptr;
}

bool ui_element::contains_point(int32_t x, int32_t y) const {
    return bounds_.contains(x, y);
}

void ui_element::update_children(float delta_time, const input& inp) {
    for (auto& child : children_) {
        child->update(delta_time, inp);
    }
}

void ui_element::render_children(renderer& rend) {
    for (auto& child : children_) {
        child->render(rend);
    }
}

// ui_panel implementation

void ui_panel::render(renderer& rend) {
    if (!visible_) return;

    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, bg_color_, true);
    if (has_border_) {
        rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, border_color_, false);
    }

    render_children(rend);
}

// ui_label implementation

void ui_label::render(renderer& rend) {
    if (!visible_) return;

    int32_t text_x = bounds_.x;
    if (alignment_ == alignment::center) {
        text_x = bounds_.x + bounds_.width / 2 - static_cast<int32_t>(text_.length() * font_size_ / 4);
    } else if (alignment_ == alignment::right) {
        text_x = bounds_.x + bounds_.width - static_cast<int32_t>(text_.length() * font_size_ / 2);
    }

    rend.draw_text(text_, text_x, bounds_.y, text_color_, font_size_);
    render_children(rend);
}

// ui_button implementation

void ui_button::update(float delta_time, const input& inp) {
    if (!visible_ || !enabled_) return;
    ui_element::update(delta_time, inp);
}

void ui_button::render(renderer& rend) {
    if (!visible_) return;

    sf::Color bg_color = normal_color_;
    if (!enabled_) {
        bg_color = disabled_color_;
    } else if (pressed_) {
        bg_color = pressed_color_;
    } else if (hovered_) {
        bg_color = hover_color_;
    }

    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, bg_color, true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, sf::Color(100, 100, 140), false);

    // Center text
    int32_t text_x = bounds_.x + bounds_.width / 2 - static_cast<int32_t>(text_.length() * 3);
    int32_t text_y = bounds_.y + bounds_.height / 2 - 7;
    rend.draw_text(text_, text_x, text_y, enabled_ ? text_color_ : sf::Color(100, 100, 100));

    render_children(rend);
}

bool ui_button::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    if (btn == sf::Mouse::Button::Left && contains_point(x, y)) {
        pressed_ = true;
        return true;
    }

    return ui_element::handle_mouse_down(x, y, btn);
}

bool ui_button::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    if (btn == sf::Mouse::Button::Left && pressed_) {
        pressed_ = false;
        if (contains_point(x, y) && on_click_) {
            on_click_();
        }
        return true;
    }

    return ui_element::handle_mouse_up(x, y, btn);
}

// ui_text_input implementation

void ui_text_input::update(float delta_time, const input& inp) {
    if (!visible_ || !enabled_) return;

    if (focused_) {
        cursor_blink_timer_ += delta_time;
        if (cursor_blink_timer_ >= 0.5f) {
            cursor_blink_timer_ = 0.0f;
            cursor_visible_ = !cursor_visible_;
        }
    }

    ui_element::update(delta_time, inp);
}

void ui_text_input::render(renderer& rend) {
    if (!visible_) return;

    // Background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, bg_color_, true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   focused_ ? sf::Color(120, 120, 160) : border_color_, false);

    // Text
    std::string display_text;
    if (text_.empty()) {
        display_text = std::string(placeholder_);
        rend.draw_text(display_text, bounds_.x + 4, bounds_.y + 4, placeholder_color_);
    } else {
        if (password_mode_) {
            display_text = std::string(text_.length(), '*');
        } else {
            display_text = text_;
        }
        rend.draw_text(display_text, bounds_.x + 4, bounds_.y + 4, text_color_);
    }

    // Cursor
    if (focused_ && cursor_visible_) {
        int32_t cursor_x = bounds_.x + 4 + static_cast<int32_t>(cursor_pos_ * 7);
        rend.draw_line(cursor_x, bounds_.y + 4, cursor_x, bounds_.y + bounds_.height - 4, text_color_);
    }

    render_children(rend);
}

bool ui_text_input::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    if (btn == sf::Mouse::Button::Left) {
        bool was_focused = focused_;
        focused_ = contains_point(x, y);

        if (focused_ && !was_focused) {
            cursor_pos_ = text_.length();
            cursor_visible_ = true;
            cursor_blink_timer_ = 0.0f;
        }

        if (focused_) {
            return true;
        }
    }

    return ui_element::handle_mouse_down(x, y, btn);
}

bool ui_text_input::handle_key_press(sf::Keyboard::Key key) {
    if (!visible_ || !enabled_ || !focused_) return false;

    if (key == sf::Keyboard::Key::Backspace && cursor_pos_ > 0) {
        text_.erase(cursor_pos_ - 1, 1);
        cursor_pos_--;
        if (on_change_) on_change_(text_);
        return true;
    }

    if (key == sf::Keyboard::Key::Delete && cursor_pos_ < text_.length()) {
        text_.erase(cursor_pos_, 1);
        if (on_change_) on_change_(text_);
        return true;
    }

    if (key == sf::Keyboard::Key::Left && cursor_pos_ > 0) {
        cursor_pos_--;
        return true;
    }

    if (key == sf::Keyboard::Key::Right && cursor_pos_ < text_.length()) {
        cursor_pos_++;
        return true;
    }

    if (key == sf::Keyboard::Key::Home) {
        cursor_pos_ = 0;
        return true;
    }

    if (key == sf::Keyboard::Key::End) {
        cursor_pos_ = text_.length();
        return true;
    }

    return false;
}

bool ui_text_input::handle_text_input(char32_t unicode) {
    if (!visible_ || !enabled_ || !focused_) return false;

    // Filter control characters
    if (unicode < 32 || unicode == 127) {
        return false;
    }

    if (text_.length() >= max_length_) {
        return false;
    }

    // Convert UTF-32 to UTF-8 (simplified - only handles basic characters)
    if (unicode < 128) {
        text_.insert(cursor_pos_, 1, static_cast<char>(unicode));
        cursor_pos_++;
        if (on_change_) on_change_(text_);
        return true;
    }

    return false;
}

void ui_text_input::set_text(std::string_view text) {
    text_ = text;
    cursor_pos_ = text_.length();
}

// ui_progress_bar implementation

void ui_progress_bar::render(renderer& rend) {
    if (!visible_) return;

    // Background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, bg_color_, true);

    // Fill
    int32_t fill_width = static_cast<int32_t>(bounds_.width * value_);
    if (fill_width > 0) {
        rend.draw_rect(bounds_.x, bounds_.y, fill_width, bounds_.height, fill_color_, true);
    }

    // Border
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, sf::Color(80, 80, 100), false);

    // Text
    if (show_text_) {
        std::string text = std::to_string(static_cast<int>(value_ * 100)) + "%";
        int32_t text_x = bounds_.x + bounds_.width / 2 - static_cast<int32_t>(text.length() * 3);
        int32_t text_y = bounds_.y + bounds_.height / 2 - 7;
        rend.draw_text(text, text_x, text_y, sf::Color::White);
    }

    render_children(rend);
}

void ui_progress_bar::set_value(float value) {
    value_ = std::clamp(value, 0.0f, 1.0f);
}

void ui_progress_bar::set_min_max(float min_val, float max_val) {
    min_value_ = min_val;
    max_value_ = max_val;
}

void ui_progress_bar::set_current(float current) {
    if (max_value_ > min_value_) {
        value_ = std::clamp((current - min_value_) / (max_value_ - min_value_), 0.0f, 1.0f);
    }
}

// ui_scrollbar implementation

void ui_scrollbar::update(float delta_time, const input& inp) {
    ui_element::update(delta_time, inp);
}

void ui_scrollbar::render(renderer& rend) {
    if (!visible_) return;

    // Track
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, track_color_, true);

    // Thumb
    int32_t thumb_pos, thumb_size;
    if (vertical_) {
        thumb_size = static_cast<int32_t>(bounds_.height * thumb_size_);
        thumb_pos = bounds_.y + static_cast<int32_t>((bounds_.height - thumb_size) * value_);
        rend.draw_rect(bounds_.x, thumb_pos, bounds_.width, thumb_size,
                       dragging_ || hovered_ ? thumb_hover_color_ : thumb_color_, true);
    } else {
        thumb_size = static_cast<int32_t>(bounds_.width * thumb_size_);
        thumb_pos = bounds_.x + static_cast<int32_t>((bounds_.width - thumb_size) * value_);
        rend.draw_rect(thumb_pos, bounds_.y, thumb_size, bounds_.height,
                       dragging_ || hovered_ ? thumb_hover_color_ : thumb_color_, true);
    }

    render_children(rend);
}

bool ui_scrollbar::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    if (btn == sf::Mouse::Button::Left && contains_point(x, y)) {
        dragging_ = true;

        // Calculate thumb position
        if (vertical_) {
            int32_t thumb_size = static_cast<int32_t>(bounds_.height * thumb_size_);
            int32_t thumb_pos = bounds_.y + static_cast<int32_t>((bounds_.height - thumb_size) * value_);
            drag_offset_ = y - thumb_pos;
        } else {
            int32_t thumb_size = static_cast<int32_t>(bounds_.width * thumb_size_);
            int32_t thumb_pos = bounds_.x + static_cast<int32_t>((bounds_.width - thumb_size) * value_);
            drag_offset_ = x - thumb_pos;
        }

        return true;
    }

    return ui_element::handle_mouse_down(x, y, btn);
}

bool ui_scrollbar::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (btn == sf::Mouse::Button::Left) {
        dragging_ = false;
    }
    return ui_element::handle_mouse_up(x, y, btn);
}

bool ui_scrollbar::handle_mouse_move(int32_t x, int32_t y) {
    ui_element::handle_mouse_move(x, y);

    if (dragging_) {
        float new_value;
        if (vertical_) {
            int32_t thumb_size = static_cast<int32_t>(bounds_.height * thumb_size_);
            int32_t track_size = bounds_.height - thumb_size;
            if (track_size > 0) {
                new_value = static_cast<float>(y - bounds_.y - drag_offset_) / track_size;
            } else {
                new_value = 0.0f;
            }
        } else {
            int32_t thumb_size = static_cast<int32_t>(bounds_.width * thumb_size_);
            int32_t track_size = bounds_.width - thumb_size;
            if (track_size > 0) {
                new_value = static_cast<float>(x - bounds_.x - drag_offset_) / track_size;
            } else {
                new_value = 0.0f;
            }
        }

        set_value(new_value);
        return true;
    }

    return hovered_;
}

void ui_scrollbar::set_value(float value) {
    float old_value = value_;
    value_ = std::clamp(value, 0.0f, 1.0f);
    if (value_ != old_value && on_scroll_) {
        on_scroll_(value_);
    }
}

// ui_list_box implementation

void ui_list_box::update(float delta_time, const input& inp) {
    ui_element::update(delta_time, inp);
}

void ui_list_box::render(renderer& rend) {
    if (!visible_) return;

    // Background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, bg_color_, true);

    // Items
    int32_t visible_items = bounds_.height / item_height_;
    for (int32_t i = 0; i < visible_items && i + scroll_offset_ < static_cast<int32_t>(items_.size()); ++i) {
        int32_t index = i + scroll_offset_;
        int32_t item_y = bounds_.y + i * item_height_;

        // Selection/hover highlight
        if (index == selected_index_) {
            rend.draw_rect(bounds_.x, item_y, bounds_.width, item_height_, selected_color_, true);
        } else if (hovered_) {
            // Check if mouse is over this item
            // (simplified - would need actual mouse position)
        }

        // Text
        rend.draw_text(items_[index], bounds_.x + 4, item_y + 2, item_color_);
    }

    // Border
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, sf::Color(80, 80, 100), false);

    render_children(rend);
}

bool ui_list_box::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_ || !enabled_) return false;

    if (btn == sf::Mouse::Button::Left && contains_point(x, y)) {
        int32_t clicked_item = (y - bounds_.y) / item_height_ + scroll_offset_;
        if (clicked_item >= 0 && clicked_item < static_cast<int32_t>(items_.size())) {
            set_selected_index(clicked_item);
        }
        return true;
    }

    return ui_element::handle_mouse_down(x, y, btn);
}

void ui_list_box::add_item(std::string_view text) {
    items_.emplace_back(text);
}

void ui_list_box::remove_item(int32_t index) {
    if (index >= 0 && index < static_cast<int32_t>(items_.size())) {
        items_.erase(items_.begin() + index);
        if (selected_index_ >= static_cast<int32_t>(items_.size())) {
            selected_index_ = static_cast<int32_t>(items_.size()) - 1;
        }
    }
}

void ui_list_box::clear_items() {
    items_.clear();
    selected_index_ = -1;
    scroll_offset_ = 0;
}

void ui_list_box::set_selected_index(int32_t index) {
    if (index >= -1 && index < static_cast<int32_t>(items_.size())) {
        selected_index_ = index;
        if (on_select_) {
            on_select_(index);
        }
    }
}

std::string_view ui_list_box::selected_text() const {
    if (selected_index_ >= 0 && selected_index_ < static_cast<int32_t>(items_.size())) {
        return items_[selected_index_];
    }
    return {};
}

} // namespace hb
