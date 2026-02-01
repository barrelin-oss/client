#include "ui/dialog_base.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>

namespace hb {

// dialog implementation

dialog::dialog(dialog_type type)
    : type_(type) {
    // Dialogs start hidden - must be explicitly opened
    visible_ = false;
}

void dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;
    ui_panel::update(delta_time, inp);
}

void dialog::render(renderer& rend) {
    if (!visible_) return;

    ui_panel::render(rend);
    render_title_bar(rend);
}

bool dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check title bar for dragging
    if (draggable_ && btn == sf::Mouse::Button::Left) {
        ui_rect title_rect{bounds_.x, bounds_.y, bounds_.width, title_bar_height};
        if (title_rect.contains(x, y)) {
            // Check close button
            if (closeable_) {
                ui_rect close_rect{bounds_.x + bounds_.width - 20, bounds_.y + 4, 16, 16};
                if (close_rect.contains(x, y)) {
                    close();
                    return true;
                }
            }

            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    return ui_panel::handle_mouse_down(x, y, btn);
}

bool dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (dragging_) {
        bounds_.x = x - drag_offset_x_;
        bounds_.y = y - drag_offset_y_;

        // Clamp to screen
        bounds_.x = std::clamp(bounds_.x, 0, static_cast<int32_t>(screen_width) - bounds_.width);
        bounds_.y = std::clamp(bounds_.y, 0, static_cast<int32_t>(screen_height) - bounds_.height);

        return true;
    }

    return ui_panel::handle_mouse_move(x, y);
}

void dialog::open() {
    set_visible(true);
}

void dialog::close() {
    set_visible(false);
    dragging_ = false;
    if (on_close_) {
        on_close_();
    }
}

void dialog::render_title_bar(renderer& rend) {
    // Title bar background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, title_bar_height,
                   sf::Color(60, 60, 80), true);
    rend.draw_line(bounds_.x, bounds_.y + title_bar_height,
                   bounds_.x + bounds_.width, bounds_.y + title_bar_height,
                   sf::Color(100, 100, 140));

    // Title text
    rend.draw_text(title_, bounds_.x + 8, bounds_.y + 4, sf::Color::White);

    // Close button
    if (closeable_) {
        int32_t close_x = bounds_.x + bounds_.width - 20;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 16, 16, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White);
    }
}

} // namespace hb
