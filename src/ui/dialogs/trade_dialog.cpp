#include "ui/dialogs/trade_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>

namespace hb {

trade_dialog::trade_dialog()
    : dialog(dialog_type::trade) {
    set_title("Trade");
    set_bounds({180, 100, 280, 340});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void trade_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void trade_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Partner name
    rend.draw_text(std::format("Trading with: {}", partner_name_), x, y, sf::Color::Yellow);
    y += 24;

    // Separator
    rend.draw_line(x, y, x + 260, y, sf::Color(80, 80, 100));
    y += 8;

    // My offer section
    rend.draw_text("Your Offer:", x, y, sf::Color::White);
    y += 18;

    // My item slots (2 rows of 4)
    for (int32_t i = 0; i < trade_slots; ++i) {
        int32_t row = i / 4;
        int32_t col = i % 4;
        int32_t slot_x = x + col * (slot_size + slot_padding);
        int32_t slot_y = y + row * (slot_size + slot_padding);

        bool hovered = hovered_slot_.has_value() && hovering_my_side_ && hovered_slot_.value() == i;
        render_slot(rend, slot_x, slot_y, my_items_[i], hovered);
    }
    y += 2 * (slot_size + slot_padding) + 8;

    // My gold
    rend.draw_text(std::format("Gold: {}", my_gold_), x, y, sf::Color(255, 200, 100));
    y += 20;

    // My confirm button
    sf::Color my_btn_color = my_confirmed_ ? sf::Color(60, 120, 60) : sf::Color(60, 60, 80);
    rend.draw_rect(x, y, 80, 24, my_btn_color, true);
    rend.draw_rect(x, y, 80, 24, sf::Color(100, 100, 120), false);
    rend.draw_text(my_confirmed_ ? "Confirmed" : "Confirm", x + 10, y + 4, sf::Color::White);
    y += 34;

    // Separator
    rend.draw_line(x, y, x + 260, y, sf::Color(80, 80, 100));
    y += 8;

    // Partner offer section
    rend.draw_text("Their Offer:", x, y, sf::Color::White);
    y += 18;

    // Partner item slots
    for (int32_t i = 0; i < trade_slots; ++i) {
        int32_t row = i / 4;
        int32_t col = i % 4;
        int32_t slot_x = x + col * (slot_size + slot_padding);
        int32_t slot_y = y + row * (slot_size + slot_padding);

        bool hovered = hovered_slot_.has_value() && !hovering_my_side_ && hovered_slot_.value() == i;
        render_slot(rend, slot_x, slot_y, partner_items_[i], hovered);
    }
    y += 2 * (slot_size + slot_padding) + 8;

    // Partner gold
    rend.draw_text(std::format("Gold: {}", partner_gold_), x, y, sf::Color(255, 200, 100));
    y += 20;

    // Partner confirm status
    sf::Color partner_status_color = partner_confirmed_ ? sf::Color(100, 200, 100) : sf::Color(150, 150, 150);
    rend.draw_text(partner_confirmed_ ? "Partner Confirmed" : "Waiting...", x, y, partner_status_color);
    y += 24;

    // Cancel button
    int32_t cancel_x = bounds_.x + bounds_.width - 90;
    int32_t cancel_y = bounds_.y + bounds_.height - 35;
    rend.draw_rect(cancel_x, cancel_y, 80, 24, sf::Color(120, 60, 60), true);
    rend.draw_rect(cancel_x, cancel_y, 80, 24, sf::Color(150, 100, 100), false);
    rend.draw_text("Cancel", cancel_x + 18, cancel_y + 4, sf::Color::White);
}

void trade_dialog::render_slot(renderer& rend, int32_t x, int32_t y, const item* itm, bool hovered) {
    sf::Color bg = hovered ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);
    rend.draw_rect(x, y, slot_size, slot_size, bg, true);
    rend.draw_rect(x, y, slot_size, slot_size, sf::Color(70, 70, 90), false);

    if (itm) {
        // Draw item color indicator
        sf::Color item_color;
        if (itm->is_weapon()) {
            item_color = sf::Color(200, 100, 100);
        } else if (itm->is_armor()) {
            item_color = sf::Color(100, 100, 200);
        } else {
            item_color = sf::Color(150, 150, 150);
        }

        rend.draw_rect(x + 4, y + 4, slot_size - 8, slot_size - 8, item_color, true);

        // Item amount if stackable
        if (itm->amount > 1) {
            rend.draw_text(std::to_string(itm->amount), x + 2, y + slot_size - 14, sf::Color::White, 10);
        }
    }
}

std::optional<int32_t> trade_dialog::slot_at(int32_t mx, int32_t my) const {
    int32_t x = bounds_.x + 10;
    int32_t my_slots_y = bounds_.y + 32 + 24 + 8 + 18;
    int32_t partner_slots_y = my_slots_y + 2 * (slot_size + slot_padding) + 8 + 20 + 24 + 34 + 8 + 18;

    // Check my slots
    for (int32_t i = 0; i < trade_slots; ++i) {
        int32_t row = i / 4;
        int32_t col = i % 4;
        int32_t slot_x = x + col * (slot_size + slot_padding);
        int32_t slot_y = my_slots_y + row * (slot_size + slot_padding);

        ui_rect rect{slot_x, slot_y, slot_size, slot_size};
        if (rect.contains(mx, my)) {
            return i;
        }
    }

    // Check partner slots (just for highlighting, no interaction)
    for (int32_t i = 0; i < trade_slots; ++i) {
        int32_t row = i / 4;
        int32_t col = i % 4;
        int32_t slot_x = x + col * (slot_size + slot_padding);
        int32_t slot_y = partner_slots_y + row * (slot_size + slot_padding);

        ui_rect rect{slot_x, slot_y, slot_size, slot_size};
        if (rect.contains(mx, my)) {
            return i + trade_slots;  // Offset to distinguish
        }
    }

    return std::nullopt;
}

bool trade_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    int32_t base_x = bounds_.x + 10;
    int32_t my_slots_y = bounds_.y + 32 + 24 + 8 + 18;

    // Check confirm button
    int32_t confirm_y = my_slots_y + 2 * (slot_size + slot_padding) + 8 + 20;
    ui_rect confirm_rect{base_x, confirm_y, 80, 24};
    if (confirm_rect.contains(x, y) && btn == sf::Mouse::Button::Left) {
        if (!my_confirmed_ && on_confirm_) {
            on_confirm_();
        }
        return true;
    }

    // Check cancel button
    int32_t cancel_x = bounds_.x + bounds_.width - 90;
    int32_t cancel_y = bounds_.y + bounds_.height - 35;
    ui_rect cancel_rect{cancel_x, cancel_y, 80, 24};
    if (cancel_rect.contains(x, y) && btn == sf::Mouse::Button::Left) {
        if (on_cancel_) {
            on_cancel_();
        }
        close();
        return true;
    }

    // Check my slots
    auto slot = slot_at(x, y);
    if (slot.has_value() && slot.value() < trade_slots) {
        if (btn == sf::Mouse::Button::Left) {
            if (on_add_item_) {
                on_add_item_(slot.value());
            }
        } else if (btn == sf::Mouse::Button::Right) {
            if (on_remove_item_ && my_items_[slot.value()]) {
                on_remove_item_(slot.value());
            }
        }
        return true;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool trade_dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_) return false;

    auto slot = slot_at(x, y);
    if (slot.has_value()) {
        hovered_slot_ = slot.value() % trade_slots;
        hovering_my_side_ = slot.value() < trade_slots;
    } else {
        hovered_slot_.reset();
    }

    return dialog::handle_mouse_move(x, y);
}

void trade_dialog::set_my_item(int32_t slot, const item* itm) {
    if (slot >= 0 && slot < trade_slots) {
        my_items_[slot] = itm;
        my_confirmed_ = false;  // Reset confirmation when items change
    }
}

void trade_dialog::clear_my_slot(int32_t slot) {
    if (slot >= 0 && slot < trade_slots) {
        my_items_[slot] = nullptr;
        my_confirmed_ = false;
    }
}

void trade_dialog::clear_my_items() {
    my_items_.fill(nullptr);
    my_confirmed_ = false;
}

void trade_dialog::set_partner_item(int32_t slot, const item* itm) {
    if (slot >= 0 && slot < trade_slots) {
        partner_items_[slot] = itm;
        partner_confirmed_ = false;
    }
}

void trade_dialog::clear_partner_slot(int32_t slot) {
    if (slot >= 0 && slot < trade_slots) {
        partner_items_[slot] = nullptr;
        partner_confirmed_ = false;
    }
}

void trade_dialog::clear_partner_items() {
    partner_items_.fill(nullptr);
    partner_confirmed_ = false;
}

} // namespace hb
