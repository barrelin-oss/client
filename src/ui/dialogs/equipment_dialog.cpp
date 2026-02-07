#include "ui/dialogs/equipment_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <cstring>
#include <format>

namespace hb {

// Slot positions arranged like a character silhouette
const std::array<equipment_dialog::slot_position, static_cast<size_t>(equipment_dialog::max_equip_slots)>
equipment_dialog::slot_positions_ = {{
    {0, 0, "None"},           // none - unused
    {90, 30, "Head"},         // head
    {90, 90, "Body"},         // body
    {30, 90, "Arms"},         // arms
    {90, 150, "Legs"},        // pants
    {90, 210, "Feet"},        // boots
    {150, 60, "Neck"},        // neck
    {30, 150, "L.Hand"},      // left_hand
    {150, 150, "R.Hand"},     // right_hand
    {90, 120, "2H"},          // two_hand (overlaps body area)
    {150, 210, "R.Ring"},     // right_finger
    {30, 210, "L.Ring"},      // left_finger
    {150, 90, "Back"},        // back
    {90, 90, "Armor"},        // full_body (overlaps body area)
    {30, 30, "Arrows"},       // auxiliary (arrow slot)
}};

equipment_dialog::equipment_dialog()
    : dialog(dialog_type::equipment) {
    set_title("Equipment");
    set_bounds({320, 50, 220, 300});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void equipment_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void equipment_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t base_x = bounds_.x + 10;
    int32_t base_y = bounds_.y + 32;

    // Draw character silhouette background
    rend.draw_rect(base_x + 70, base_y + 40, 60, 180, sf::Color(35, 35, 45), true);
    rend.draw_rect(base_x + 70, base_y + 40, 60, 180, sf::Color(50, 50, 70), false);

    // Draw equipment slots
    // Skip slot 0 (none) and only draw valid equipment slots
    for (size_t i = 1; i < static_cast<size_t>(equipment_dialog::max_equip_slots); ++i) {
        equip_slot slot = static_cast<equip_slot>(i);

        // Skip two_hand and full_body for now (they overlap other slots)
        if (slot == equip_slot::two_hand || slot == equip_slot::full_body) {
            continue;
        }

        render_slot(rend, slot);
    }

    // Draw tooltip for hovered slot
    if (hovered_slot_.has_value()) {
        equip_slot slot = hovered_slot_.value();
        size_t idx = static_cast<size_t>(slot);

        int32_t tooltip_y = bounds_.y + bounds_.height - 45;
        rend.draw_line(base_x, tooltip_y - 5, base_x + 200, tooltip_y - 5, sf::Color(60, 60, 80));

        rend.draw_text(slot_positions_[idx].label, base_x, tooltip_y, sf::Color::Yellow);

        if (slots_[idx].item_ptr) {
            const item* itm = slots_[idx].item_ptr;
            rend.draw_text(itm->name, base_x, tooltip_y + 16, sf::Color::White, 12);
        } else {
            rend.draw_text("Empty", base_x, tooltip_y + 16, sf::Color(120, 120, 120), 12);
        }
    }
}

void equipment_dialog::render_slot(renderer& rend, equip_slot slot) {
    size_t idx = static_cast<size_t>(slot);
    if (idx == 0 || idx >= slot_positions_.size()) return;

    int32_t base_x = bounds_.x + 10;
    int32_t base_y = bounds_.y + 32;

    int32_t slot_x = base_x + slot_positions_[idx].x;
    int32_t slot_y = base_y + slot_positions_[idx].y;

    // Slot background
    sf::Color bg_color = sf::Color(40, 40, 55);
    if (hovered_slot_.has_value() && hovered_slot_.value() == slot) {
        bg_color = sf::Color(60, 60, 80);
    }

    rend.draw_rect(slot_x, slot_y, slot_size, slot_size, bg_color, true);
    rend.draw_rect(slot_x, slot_y, slot_size, slot_size, sf::Color(70, 70, 90), false);

    // Draw item if equipped
    if (slots_[idx].item_ptr) {
        const item* itm = slots_[idx].item_ptr;

        // Item color based on slot type
        sf::Color item_color;
        if (itm->is_weapon()) {
            item_color = sf::Color(200, 100, 100);
        } else if (itm->is_armor()) {
            item_color = sf::Color(100, 100, 200);
        } else if (itm->is_accessory()) {
            item_color = sf::Color(200, 180, 100);
        } else {
            item_color = sf::Color(150, 150, 150);
        }

        rend.draw_rect(slot_x + 4, slot_y + 4, slot_size - 8, slot_size - 8, item_color, true);

        // Draw durability if applicable
        if (itm->max_durability > 0) {
            float dur_pct = static_cast<float>(itm->durability) / itm->max_durability;
            int32_t dur_width = static_cast<int32_t>((slot_size - 4) * dur_pct);

            sf::Color dur_color = dur_pct > 0.5f ? sf::Color(100, 200, 100)
                                : dur_pct > 0.2f ? sf::Color(200, 200, 100)
                                : sf::Color(200, 100, 100);

            rend.draw_rect(slot_x + 2, slot_y + slot_size - 4, dur_width, 2, dur_color, true);
        }
    } else {
        // Draw slot type indicator
        const char* label = slot_positions_[idx].label;
        if (label && strlen(label) <= 4) {
            int32_t text_x = slot_x + slot_size / 2 - static_cast<int32_t>(strlen(label) * 2);
            rend.draw_text(label, text_x, slot_y + slot_size / 2 - 5, sf::Color(80, 80, 100), 10);
        }
    }
}

ui_rect equipment_dialog::get_slot_rect(equip_slot slot) const {
    size_t idx = static_cast<size_t>(slot);
    if (idx == 0 || idx >= slot_positions_.size()) {
        return {0, 0, 0, 0};
    }

    int32_t base_x = bounds_.x + 10;
    int32_t base_y = bounds_.y + 32;

    return {
        base_x + slot_positions_[idx].x,
        base_y + slot_positions_[idx].y,
        slot_size,
        slot_size
    };
}

std::optional<equip_slot> equipment_dialog::slot_at(int32_t x, int32_t y) const {
    // Check each valid slot
    for (size_t i = 1; i < static_cast<size_t>(equipment_dialog::max_equip_slots); ++i) {
        equip_slot slot = static_cast<equip_slot>(i);

        // Skip two_hand and full_body
        if (slot == equip_slot::two_hand || slot == equip_slot::full_body) {
            continue;
        }

        ui_rect rect = get_slot_rect(slot);
        if (rect.contains(x, y)) {
            return slot;
        }
    }
    return std::nullopt;
}

bool equipment_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    auto slot = slot_at(x, y);
    if (slot.has_value()) {
        if (btn == sf::Mouse::Button::Left) {
            if (on_slot_click_) {
                on_slot_click_(slot.value());
            }
            return true;
        } else if (btn == sf::Mouse::Button::Right) {
            if (on_slot_right_click_) {
                on_slot_right_click_(slot.value());
            }
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool equipment_dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_) return false;

    hovered_slot_ = slot_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void equipment_dialog::set_equipped(equip_slot slot, const item* itm) {
    size_t idx = static_cast<size_t>(slot);
    if (idx > 0 && idx < slots_.size()) {
        slots_[idx].item_ptr = itm;
    }
}

void equipment_dialog::clear_slot(equip_slot slot) {
    size_t idx = static_cast<size_t>(slot);
    if (idx > 0 && idx < slots_.size()) {
        slots_[idx].item_ptr = nullptr;
    }
}

void equipment_dialog::clear_all() {
    for (auto& slot : slots_) {
        slot.item_ptr = nullptr;
    }
}

const item* equipment_dialog::get_equipped(equip_slot slot) const {
    size_t idx = static_cast<size_t>(slot);
    if (idx > 0 && idx < slots_.size()) {
        return slots_[idx].item_ptr;
    }
    return nullptr;
}

} // namespace hb
