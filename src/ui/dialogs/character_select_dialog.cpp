#include "ui/dialogs/character_select_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>

namespace hb {

character_select_dialog::character_select_dialog()
    : dialog(dialog_type::character_select) {
    set_title("Select Character");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - 350,
        static_cast<int32_t>(screen_height) / 2 - 170,
        700, 340
    });
    set_closeable(true);
    set_modal(true);

    create_ui();
}

void character_select_dialog::create_ui() {
    // Create buttons at bottom
    int32_t btn_row_y = 290;

    // Enter game button
    auto enter_btn = std::make_unique<ui_button>();
    enter_btn->set_id("enter_button");
    enter_btn->set_bounds({160, btn_row_y, 100, 32});
    enter_btn->set_text("Enter Game");
    enter_btn->set_on_click([this]() {
        if (selected_index_ >= 0 && on_enter_) {
            on_enter_(selected_index_);
        } else {
            set_status("Please select a character first.", true);
        }
    });
    add_child(std::move(enter_btn));

    // Create new character button
    auto create_btn = std::make_unique<ui_button>();
    create_btn->set_id("create_button");
    create_btn->set_bounds({300, btn_row_y, 100, 32});
    create_btn->set_text("Create New");
    create_btn->set_on_click([this]() {
        if (characters_.size() >= max_characters) {
            set_status("Maximum characters reached.", true);
            return;
        }
        if (on_create_) {
            on_create_();
        }
    });
    add_child(std::move(create_btn));

    // Delete character button
    auto delete_btn = std::make_unique<ui_button>();
    delete_btn->set_id("delete_button");
    delete_btn->set_bounds({440, btn_row_y, 100, 32});
    delete_btn->set_text("Delete");
    delete_btn->set_on_click([this]() {
        if (selected_index_ >= 0 && on_delete_) {
            on_delete_(selected_index_);
        } else {
            set_status("Please select a character to delete.", true);
        }
    });
    add_child(std::move(delete_btn));
}

void character_select_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;
    dialog::update(delta_time, inp);

    // Handle click on character slots
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left)) {
        int32_t mx = inp.mouse_x();
        int32_t my = inp.mouse_y();

        // Check if clicking within character slots area
        int32_t slots_start_x = bounds_.x + 20;
        int32_t slots_start_y = bounds_.y + 50;

        for (int32_t i = 0; i < max_characters; ++i) {
            int32_t slot_x = slots_start_x + i * (slot_width + slot_padding);
            int32_t slot_y = slots_start_y;

            if (mx >= slot_x && mx < slot_x + slot_width &&
                my >= slot_y && my < slot_y + slot_height) {
                if (i < static_cast<int32_t>(characters_.size())) {
                    set_selected_index(i);
                }
                break;
            }
        }
    }

    // Double click to enter game
    // Note: simplified - would need proper double-click detection in a real implementation
}

void character_select_dialog::render(renderer& rend) {
    if (!visible_) return;
    dialog::render(rend);

    // Render character slots
    int32_t slots_start_x = bounds_.x + 20;
    int32_t slots_start_y = bounds_.y + 50;

    for (int32_t i = 0; i < max_characters; ++i) {
        int32_t slot_x = slots_start_x + i * (slot_width + slot_padding);
        int32_t slot_y = slots_start_y;

        render_character_slot(rend, i, slot_x, slot_y);
    }

    // Render status message
    if (!status_message_.empty()) {
        sf::Color status_color = status_is_error_ ? sf::Color(255, 100, 100) : sf::Color(100, 255, 100);
        rend.draw_text(status_message_,
            bounds_.x + 20, bounds_.y + 260,
            status_color);
    }
}

void character_select_dialog::render_character_slot(renderer& rend, int32_t slot, int32_t x, int32_t y) {
    // Slot background
    bool selected = (slot == selected_index_);
    bool has_character = (slot < static_cast<int32_t>(characters_.size()));

    sf::Color bg_color = selected ? sf::Color(60, 80, 120) : sf::Color(40, 40, 60);
    sf::Color border_color = selected ? sf::Color(100, 140, 200) : sf::Color(80, 80, 100);

    rend.draw_rect(x, y, slot_width, slot_height, bg_color, true);
    rend.draw_rect(x, y, slot_width, slot_height, border_color, false);

    if (has_character) {
        const auto& ch = characters_[slot];

        // Character portrait area (placeholder)
        rend.draw_rect(x + 30, y + 10, 100, 80, sf::Color(30, 30, 40), true);

        // Character portrait placeholder text
        std::string gender_str = ch.gender == 0 ? "M" : "F";
        std::string class_str = ch.warrior ? "W" : "M";
        rend.draw_text(gender_str + "/" + class_str,
            x + 60, y + 40, sf::Color(120, 120, 140));

        // Character name
        rend.draw_text(ch.name, x + 10, y + 95, sf::Color::White);

        // Level and class
        std::string level_text = "Lv." + std::to_string(ch.level) + " " + ch.class_name;
        rend.draw_text(level_text, x + 10, y + 115, sf::Color(200, 200, 200));

        // Map name
        rend.draw_text(ch.map_name, x + 10, y + 135, sf::Color(150, 150, 150));

        // Stats
        std::string stats1 = "STR:" + std::to_string(ch.strength) +
                            " VIT:" + std::to_string(ch.vitality) +
                            " DEX:" + std::to_string(ch.dexterity);
        rend.draw_text(stats1, x + 10, y + 155, sf::Color(100, 150, 200), 10);
    } else {
        // Empty slot
        rend.draw_text("Empty Slot", x + 40, y + 80, sf::Color(80, 80, 100));
        rend.draw_text("Click Create", x + 35, y + 100, sf::Color(60, 60, 80));
    }
}

void character_select_dialog::set_characters(const std::vector<character_display_info>& characters) {
    characters_ = characters;
    if (!characters_.empty()) {
        selected_index_ = 0;
    } else {
        selected_index_ = -1;
    }
    update_character_display();
}

void character_select_dialog::clear_characters() {
    characters_.clear();
    selected_index_ = -1;
}

void character_select_dialog::set_selected_index(int32_t index) {
    if (index >= 0 && index < static_cast<int32_t>(characters_.size())) {
        selected_index_ = index;
        clear_status();
        if (on_select_) {
            on_select_(index);
        }
    }
}

void character_select_dialog::update_character_display() {
    // Update any dynamic UI elements based on character data
}

void character_select_dialog::set_status(std::string_view message, bool error) {
    status_message_ = message;
    status_is_error_ = error;
}

void character_select_dialog::clear_status() {
    status_message_.clear();
    status_is_error_ = false;
}

} // namespace hb
