#include "ui/dialogs/spellbook_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <format>

namespace hb {

spellbook_dialog::spellbook_dialog()
    : dialog(dialog_type::spellbook) {
    set_title("Spellbook");
    set_bounds({100, 80, 280, 380});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void spellbook_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
    click_timer_ += delta_time;
}

void spellbook_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Circle tabs
    rend.draw_text("Circle:", x, y, sf::Color::White);
    y += 18;

    for (uint8_t circle = 1; circle <= max_circles; ++circle) {
        int32_t tab_x = x + (circle - 1) * 25;
        bool is_current = (circle == current_circle_);

        sf::Color bg = is_current ? sf::Color(80, 80, 120) : sf::Color(50, 50, 70);
        rend.draw_rect(tab_x, y, 22, 18, bg, true);
        rend.draw_rect(tab_x, y, 22, 18, sf::Color(80, 80, 100), false);

        sf::Color text_color = is_current ? sf::Color::Yellow : sf::Color::White;
        rend.draw_text(std::to_string(circle), tab_x + 7, y + 2, text_color, 12);
    }
    y += 26;

    // Separator
    rend.draw_line(x, y, x + 260, y, sf::Color(80, 80, 100));
    y += 8;

    content_start_y_ = y;

    // Filter spells by current circle
    current_circle_spells_.clear();
    for (const auto& sp : spells_) {
        if (sp.circle == current_circle_) {
            current_circle_spells_.push_back(sp);
        }
    }

    if (current_circle_spells_.empty()) {
        rend.draw_text("No spells learned in this circle", x, y, sf::Color(150, 150, 150));
        return;
    }

    // Draw spell grid
    int32_t col = 0;
    int32_t row = 0;
    for (size_t i = 0; i < current_circle_spells_.size(); ++i) {
        int32_t slot_x = x + col * (spell_slot_size + slot_padding);
        int32_t slot_y = y + row * (spell_slot_size + slot_padding + 14);

        bool is_selected = selected_spell_.has_value() &&
                          selected_spell_.value() == current_circle_spells_[i].id;

        render_spell_slot(rend, current_circle_spells_[i], slot_x, slot_y, is_selected);

        col++;
        if (col >= spells_per_row) {
            col = 0;
            row++;
        }
    }

    // Spell info at bottom if hovered
    if (hovered_index_.has_value() && hovered_index_.value() < current_circle_spells_.size()) {
        const auto& sp = current_circle_spells_[hovered_index_.value()];
        int32_t info_y = bounds_.y + bounds_.height - 60;

        rend.draw_line(x, info_y - 8, x + 260, info_y - 8, sf::Color(80, 80, 100));

        rend.draw_text(sp.name, x, info_y, sf::Color::Yellow);
        info_y += 16;

        rend.draw_text(std::format("MP: {}  Cast: {:.1f}s", sp.mp_cost, sp.cast_time),
                      x, info_y, sf::Color::White, 12);
        info_y += 14;

        rend.draw_text(std::format("Mastery: {}%", sp.mastery_level), x, info_y, sf::Color(150, 200, 255), 12);
    }
}

void spellbook_dialog::render_spell_slot(renderer& rend, const spell& sp,
                                         int32_t x, int32_t y, bool selected) {
    // Slot background
    sf::Color bg = selected ? sf::Color(60, 80, 120) : sf::Color(40, 40, 55);
    if (hovered_index_.has_value()) {
        size_t idx = hovered_index_.value();
        if (idx < current_circle_spells_.size() && current_circle_spells_[idx].id == sp.id) {
            bg = sf::Color(60, 60, 80);
        }
    }

    rend.draw_rect(x, y, spell_slot_size, spell_slot_size, bg, true);
    rend.draw_rect(x, y, spell_slot_size, spell_slot_size,
                  selected ? sf::Color(100, 120, 180) : sf::Color(60, 60, 80), false);

    // Spell icon (colored based on magic type)
    sf::Color spell_color;
    switch (sp.type) {
        case magic_type::damage_spot:
        case magic_type::damage_area:
        case magic_type::damage_area_no_spot:
            spell_color = sf::Color(200, 100, 100);  // Red for damage
            break;
        case magic_type::hp_up_spot:
        case magic_type::sp_up_spot:
        case magic_type::sp_up_area:
            spell_color = sf::Color(100, 200, 100);  // Green for healing/recovery
            break;
        case magic_type::protect:
        case magic_type::hold_object:
            spell_color = sf::Color(100, 150, 200);  // Blue for support
            break;
        default:
            spell_color = sf::Color(180, 150, 200);  // Purple for others
            break;
    }

    rend.draw_rect(x + 6, y + 6, spell_slot_size - 12, spell_slot_size - 12, spell_color, true);

    // Spell name below
    std::string short_name = sp.name.length() > 8 ? sp.name.substr(0, 7) + "." : sp.name;
    int32_t text_x = x + spell_slot_size / 2 - static_cast<int32_t>(short_name.length() * 2);
    rend.draw_text(short_name, text_x, y + spell_slot_size + 2, sf::Color::White, 10);
}

std::optional<size_t> spellbook_dialog::spell_index_at(int32_t x, int32_t y) const {
    int32_t base_x = bounds_.x + 10;
    int32_t base_y = content_start_y_;

    for (size_t i = 0; i < current_circle_spells_.size(); ++i) {
        int32_t col = static_cast<int32_t>(i % spells_per_row);
        int32_t row = static_cast<int32_t>(i / spells_per_row);

        int32_t slot_x = base_x + col * (spell_slot_size + slot_padding);
        int32_t slot_y = base_y + row * (spell_slot_size + slot_padding + 14);

        ui_rect slot_rect{slot_x, slot_y, spell_slot_size, spell_slot_size + 14};
        if (slot_rect.contains(x, y)) {
            return i;
        }
    }

    return std::nullopt;
}

bool spellbook_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check circle tabs
    int32_t tab_y = bounds_.y + 50;
    for (uint8_t circle = 1; circle <= max_circles; ++circle) {
        int32_t tab_x = bounds_.x + 10 + (circle - 1) * 25;
        ui_rect tab_rect{tab_x, tab_y, 22, 18};
        if (tab_rect.contains(x, y)) {
            set_current_circle(circle);
            return true;
        }
    }

    // Check spell slots
    if (btn == sf::Mouse::Button::Left) {
        auto idx = spell_index_at(x, y);
        if (idx.has_value() && idx.value() < current_circle_spells_.size()) {
            uint16_t clicked_id = current_circle_spells_[idx.value()].id;

            // Double-click check
            if (last_click_index_ == idx.value() && click_timer_ < double_click_time) {
                if (on_spell_double_click_) {
                    on_spell_double_click_(clicked_id);
                }
                last_click_index_ = SIZE_MAX;
            } else {
                last_click_index_ = idx.value();
                click_timer_ = 0.0f;

                selected_spell_ = clicked_id;
                if (on_spell_click_) {
                    on_spell_click_(clicked_id);
                }
            }
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool spellbook_dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_) return false;

    hovered_index_ = spell_index_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void spellbook_dialog::set_spells(const std::vector<spell>& spells) {
    spells_ = spells;
}

void spellbook_dialog::add_spell(const spell& sp) {
    // Check if already exists
    auto it = std::find_if(spells_.begin(), spells_.end(),
        [&sp](const spell& existing) { return existing.id == sp.id; });

    if (it != spells_.end()) {
        *it = sp;  // Update
    } else {
        spells_.push_back(sp);
    }
}

void spellbook_dialog::clear_spells() {
    spells_.clear();
    current_circle_spells_.clear();
    selected_spell_.reset();
}

void spellbook_dialog::set_current_circle(uint8_t circle) {
    if (circle >= 1 && circle <= max_circles) {
        current_circle_ = circle;
        hovered_index_.reset();
    }
}

} // namespace hb
