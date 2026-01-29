#include "ui/mouse_interface.hpp"

namespace hb {

int32_t mouse_interface::add_rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    rects_.push_back({x1, y1, x2, y2});
    return static_cast<int32_t>(rects_.size());  // 1-based button number
}

void mouse_interface::clear() {
    rects_.clear();
    was_pressed_ = false;
}

int32_t mouse_interface::get_status(int32_t mouse_x, int32_t mouse_y, bool mouse_pressed, mouse_result& result) {
    result = mouse_result::none;

    // Find which button we're hovering over
    int32_t button_num = hit_test(mouse_x, mouse_y);

    if (button_num > 0) {
        // We're over a button
        if (mouse_pressed && !was_pressed_) {
            // Button was just pressed
            result = mouse_result::click;
        }
    }

    was_pressed_ = mouse_pressed;
    return button_num;
}

int32_t mouse_interface::hit_test(int32_t x, int32_t y) const {
    for (size_t i = 0; i < rects_.size(); ++i) {
        const auto& rect = rects_[i];
        if (x >= rect.x1 && x <= rect.x2 && y >= rect.y1 && y <= rect.y2) {
            return static_cast<int32_t>(i + 1);  // 1-based button number
        }
    }
    return 0;
}

} // namespace hb
