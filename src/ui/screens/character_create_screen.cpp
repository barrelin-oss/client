#include "ui/screens/character_create_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void character_create_screen::on_enter() {
    // Reset state
    mode_count_ = 0;
    current_focus_ = 1;  // Start on name field
    max_focus_ = 6;      // 6 focusable areas (name + 5 preset buttons)
    enter_pressed_ = false;
    escape_pressed_ = false;
    arrow_pressed_ = 0;
    cursor_timer_ = 0.0f;
    cursor_visible_ = true;

    // Animation state
    menu_frame_ = 0;
    menu_dir_ = 1;
    menu_dir_count_ = 0;
    frame_timer_ = 0.0f;

    // Reset character data
    char_name_.clear();
    gender_ = 1;
    skin_color_ = 1;
    hair_style_ = 0;
    hair_color_ = 0;
    underwear_color_ = 0;

    // Reset stats to default
    str_ = 10;
    vit_ = 10;
    dex_ = 10;
    int_ = 10;
    mag_ = 10;
    chr_ = 10;

    // Setup clickable rectangles (from legacy UpdateScreen_OnCreateNewCharacter)
    mouse_interface_.clear();

    // Button 1: Name input field
    mouse_interface_.add_rect(69, 110, 279, 127);

    // Appearance buttons (left/right arrows)
    // Gender
    mouse_interface_.add_rect(236, 156, 257, 169);   // Button 2: Gender left
    mouse_interface_.add_rect(259, 156, 276, 169);   // Button 3: Gender right
    // Skin color
    mouse_interface_.add_rect(236, 171, 257, 184);   // Button 4: Skin left
    mouse_interface_.add_rect(259, 171, 276, 184);   // Button 5: Skin right
    // Hair style
    mouse_interface_.add_rect(236, 186, 257, 199);   // Button 6: Hair style left
    mouse_interface_.add_rect(259, 186, 276, 199);   // Button 7: Hair style right
    // Hair color
    mouse_interface_.add_rect(236, 201, 257, 214);   // Button 8: Hair color left
    mouse_interface_.add_rect(259, 201, 276, 214);   // Button 9: Hair color right
    // Underwear color
    mouse_interface_.add_rect(236, 216, 257, 229);   // Button 10: Underwear left
    mouse_interface_.add_rect(259, 216, 276, 229);   // Button 11: Underwear right

    // Stats buttons (up/down)
    // Strength
    mouse_interface_.add_rect(236, 276, 257, 289);   // Button 12: Str up
    mouse_interface_.add_rect(259, 276, 280, 289);   // Button 13: Str down
    // Vitality
    mouse_interface_.add_rect(236, 291, 257, 304);   // Button 14: Vit up
    mouse_interface_.add_rect(259, 291, 280, 304);   // Button 15: Vit down
    // Dexterity
    mouse_interface_.add_rect(236, 306, 257, 319);   // Button 16: Dex up
    mouse_interface_.add_rect(259, 306, 280, 319);   // Button 17: Dex down
    // Intelligence
    mouse_interface_.add_rect(236, 321, 257, 334);   // Button 18: Int up
    mouse_interface_.add_rect(259, 321, 280, 334);   // Button 19: Int down
    // Magic
    mouse_interface_.add_rect(236, 336, 257, 349);   // Button 20: Mag up
    mouse_interface_.add_rect(259, 336, 280, 349);   // Button 21: Mag down
    // Charisma
    mouse_interface_.add_rect(236, 351, 257, 364);   // Button 22: Chr up
    mouse_interface_.add_rect(259, 351, 280, 364);   // Button 23: Chr down

    // Bottom action buttons
    mouse_interface_.add_rect(384, 445, 456, 460);   // Button 24: Create
    mouse_interface_.add_rect(500, 445, 572, 460);   // Button 25: Cancel

    // Preset buttons
    mouse_interface_.add_rect(60, 445, 132, 460);    // Button 26: Warrior
    mouse_interface_.add_rect(145, 445, 217, 460);   // Button 27: Mage
    mouse_interface_.add_rect(230, 445, 302, 460);   // Button 28: Merchant

    spdlog::info("Character create screen entered");
}

void character_create_screen::on_exit() {
    mouse_interface_.clear();
    spdlog::info("Character create screen exited");
}

bool character_create_screen::update(float delta_time, const input& inp) {
    // Update mode count
    mode_count_++;
    if (mode_count_ > 100) mode_count_ = 100;

    // Update cursor blink
    cursor_timer_ += delta_time;
    if (cursor_timer_ >= 0.5f) {
        cursor_timer_ = 0.0f;
        cursor_visible_ = !cursor_visible_;
    }

    // Update animation timer
    frame_timer_ += delta_time;
    if (frame_timer_ >= 0.1f) {
        frame_timer_ = 0.0f;
        menu_frame_++;
        if (menu_frame_ >= 8) {
            menu_frame_ = 0;
            menu_dir_count_++;
            if (menu_dir_count_ > 8) {
                menu_dir_++;
                menu_dir_count_ = 1;
            }
        }
        if (menu_dir_ > 8) menu_dir_ = 1;
    }

    // Store mouse position for render
    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();
    bool mouse_pressed = inp.is_mouse_pressed(sf::Mouse::Button::Left);

    // Handle arrow key navigation
    if (inp.is_key_pressed(sf::Keyboard::Key::Up)) {
        current_focus_--;
        if (current_focus_ <= 0) current_focus_ = max_focus_;
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::Down)) {
        current_focus_++;
        if (current_focus_ > max_focus_) current_focus_ = 1;
    }

    // Handle text input when name field is focused
    if (current_focus_ == 1) {
        handle_text_input(inp);
    }

    // Handle Escape key (cancel)
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape)) {
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    }

    // Handle Enter key
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter)) {
        if (current_focus_ == 2 && is_valid_character()) {
            try_create();
            return true;
        }
    }

    // Check mouse clicks
    mouse_result result;
    int32_t button_num = mouse_interface_.get_status(mouse_x_, mouse_y_, mouse_pressed, result);

    if (result == mouse_result::click) {
        switch (button_num) {
            case btn_name_field:
                current_focus_ = 1;
                break;

            // Gender
            case btn_gender_left:
                gender_--;
                if (gender_ < 1) gender_ = 2;
                break;
            case btn_gender_right:
                gender_++;
                if (gender_ > 2) gender_ = 1;
                break;

            // Skin color
            case btn_skin_left:
                skin_color_--;
                if (skin_color_ < 1) skin_color_ = 3;
                break;
            case btn_skin_right:
                skin_color_++;
                if (skin_color_ > 3) skin_color_ = 1;
                break;

            // Hair style
            case btn_hair_style_left:
                if (hair_style_ == 0) hair_style_ = 7;
                else hair_style_--;
                break;
            case btn_hair_style_right:
                hair_style_++;
                if (hair_style_ > 7) hair_style_ = 0;
                break;

            // Hair color
            case btn_hair_color_left:
                if (hair_color_ == 0) hair_color_ = 15;
                else hair_color_--;
                break;
            case btn_hair_color_right:
                hair_color_++;
                if (hair_color_ > 15) hair_color_ = 0;
                break;

            // Underwear color
            case btn_underwear_left:
                if (underwear_color_ == 0) underwear_color_ = 7;
                else underwear_color_--;
                break;
            case btn_underwear_right:
                underwear_color_++;
                if (underwear_color_ > 7) underwear_color_ = 0;
                break;

            // Stats
            case btn_str_up:
                if (remaining_points() > 0 && str_ < max_stat) str_++;
                break;
            case btn_str_down:
                if (str_ > min_stat) str_--;
                break;
            case btn_vit_up:
                if (remaining_points() > 0 && vit_ < max_stat) vit_++;
                break;
            case btn_vit_down:
                if (vit_ > min_stat) vit_--;
                break;
            case btn_dex_up:
                if (remaining_points() > 0 && dex_ < max_stat) dex_++;
                break;
            case btn_dex_down:
                if (dex_ > min_stat) dex_--;
                break;
            case btn_int_up:
                if (remaining_points() > 0 && int_ < max_stat) int_++;
                break;
            case btn_int_down:
                if (int_ > min_stat) int_--;
                break;
            case btn_mag_up:
                if (remaining_points() > 0 && mag_ < max_stat) mag_++;
                break;
            case btn_mag_down:
                if (mag_ > min_stat) mag_--;
                break;
            case btn_chr_up:
                if (remaining_points() > 0 && chr_ < max_stat) chr_++;
                break;
            case btn_chr_down:
                if (chr_ > min_stat) chr_--;
                break;

            // Action buttons
            case btn_create:
                current_focus_ = 2;
                if (is_valid_character()) {
                    try_create();
                    return true;
                }
                break;
            case btn_cancel:
                current_focus_ = 3;
                if (on_cancel_) {
                    on_cancel_();
                }
                return true;

            // Preset buttons
            case btn_warrior:
                current_focus_ = 4;
                apply_warrior_preset();
                break;
            case btn_mage:
                current_focus_ = 5;
                apply_mage_preset();
                break;
            case btn_merchant:
                current_focus_ = 6;
                apply_merchant_preset();
                break;
        }
    }

    return true;
}

void character_create_screen::render(renderer& rend, sprite_manager& sprites) {
    draw(rend, sprites, mouse_x_, mouse_y_);
}

void character_create_screen::handle_text_input(const input& inp) {
    // Handle backspace
    if (inp.is_key_pressed(sf::Keyboard::Key::Backspace)) {
        if (!char_name_.empty()) {
            char_name_.pop_back();
        }
    }

    // Handle text input
    std::string_view text_input = inp.text_input();
    for (char c : text_input) {
        // Only allow alphanumeric characters
        if (((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            && char_name_.size() < name_max_length) {
            char_name_.push_back(c);
        }
    }
}

void character_create_screen::try_create() {
    if (!is_valid_character()) {
        spdlog::warn("Cannot create character: invalid data");
        return;
    }

    spdlog::info("Creating character: name='{}' gender={} stats={}/{}/{}/{}/{}/{}",
                 char_name_, gender_, str_, vit_, dex_, int_, mag_, chr_);

    if (on_create_) {
        character_create_data data;
        data.name = char_name_;
        data.gender = gender_;
        data.skin_color = skin_color_;
        data.hair_style = hair_style_;
        data.hair_color = hair_color_;
        data.underwear_color = underwear_color_;
        data.strength = str_;
        data.vitality = vit_;
        data.dexterity = dex_;
        data.intelligence = int_;
        data.magic = mag_;
        data.charisma = chr_;
        on_create_(data);
    }
}

void character_create_screen::apply_warrior_preset() {
    str_ = 14;
    vit_ = 12;
    dex_ = 14;
    int_ = 10;
    mag_ = 10;
    chr_ = 10;
    spdlog::debug("Applied warrior preset");
}

void character_create_screen::apply_mage_preset() {
    str_ = 10;
    vit_ = 12;
    dex_ = 10;
    int_ = 14;
    mag_ = 14;
    chr_ = 10;
    spdlog::debug("Applied mage preset");
}

void character_create_screen::apply_merchant_preset() {
    str_ = 14;
    vit_ = 10;
    dex_ = 12;
    int_ = 10;
    mag_ = 10;
    chr_ = 14;
    spdlog::debug("Applied merchant preset");
}

int32_t character_create_screen::remaining_points() const {
    int32_t used = str_ + vit_ + dex_ + int_ + mag_ + chr_;
    return total_stat_points - used;
}

bool character_create_screen::is_valid_character() const {
    // Name must not be empty
    if (char_name_.empty()) return false;

    // All stat points must be allocated
    if (remaining_points() != 0) return false;

    return true;
}

void character_create_screen::draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y) {
    // Draw background (frame 0) - NO color key for full-screen background
    draw_sprite_no_color_key(rend, sprites, charcreate_sprites::new_char, 0, 0, 0);

    // Draw button bar overlay (frame 69)
    draw_sprite(rend, sprites, charcreate_sprites::button, 0, 0, 69);

    // Dark text color for labels
    sf::Color label_color(5, 5, 5);
    sf::Color value_color(25, 35, 25);

    // === Section 1: Character Name ===
    // "Enter a character name." - centered header
    rend.draw_text("Enter a character name.", 100, 90, label_color);
    // "Character Name" label
    rend.draw_text("Character Name", 57, 110, label_color);
    // Name input field
    if (current_focus_ != 1) {
        rend.draw_text(char_name_, 197, 112, value_color);
    } else {
        std::string display_name = char_name_;
        if (cursor_visible_) display_name += "_";
        rend.draw_text(display_name, 197, 112, sf::Color::White);
    }

    // === Section 2: Appearance ===
    // "Select character's gender." - section header
    rend.draw_text("Select character's appearance.", 80, 140, label_color);

    // Appearance labels (left column)
    rend.draw_text("Gender", 100, 160, label_color);
    rend.draw_text("Skin Color", 100, 175, label_color);
    rend.draw_text("Hair Style", 100, 190, label_color);
    rend.draw_text("Hair Color", 100, 205, label_color);
    rend.draw_text("Underwear Color", 100, 220, label_color);

    // Appearance values (right side, between arrows)
    const char* gender_str = (gender_ == 1) ? "Male" : "Female";
    rend.draw_text(gender_str, 200, 160, value_color);
    rend.draw_text(std::to_string(skin_color_), 200, 175, value_color);
    rend.draw_text(std::to_string(hair_style_), 200, 190, value_color);
    rend.draw_text(std::to_string(hair_color_), 200, 205, value_color);
    rend.draw_text(std::to_string(underwear_color_), 200, 220, value_color);

    // === Section 3: Stats ===
    // "Select character's stats." - section header
    rend.draw_text("Allocate stat points.", 100, 245, label_color);

    // Remaining points
    std::string points_str = "Stat points left: " + std::to_string(remaining_points()) + " points";
    rend.draw_text(points_str, 100, 260, sf::Color(15, 10, 10));

    // Stat labels (left column)
    rend.draw_text("Strength", 100, 275, label_color);
    rend.draw_text("Vitality", 100, 292, label_color);
    rend.draw_text("Dexterity", 100, 309, label_color);
    rend.draw_text("Intelligence", 100, 326, label_color);
    rend.draw_text("Magic", 100, 343, label_color);
    rend.draw_text("Charisma", 100, 360, label_color);

    // Stat values (between +/- buttons)
    rend.draw_text(std::to_string(str_), 204, 277, value_color);
    rend.draw_text(std::to_string(vit_), 204, 293, value_color);
    rend.draw_text(std::to_string(dex_), 204, 309, value_color);
    rend.draw_text(std::to_string(int_), 204, 325, value_color);
    rend.draw_text(std::to_string(mag_), 204, 341, value_color);
    rend.draw_text(std::to_string(chr_), 204, 357, value_color);

    // Stat range note
    rend.draw_text("You can have each value between 10~14", 64, 380, label_color);

    // === Right side: Calculated HP/MP/SP ===
    rend.draw_text("Hit Point", 445, 192, label_color);
    rend.draw_text("Mana Point", 445, 208, label_color);
    rend.draw_text("Stamina Point", 445, 224, label_color);

    // Calculate and display HP/MP/SP
    int32_t hp = vit_ * 3 + 2 + str_ / 2;
    int32_t mp = mag_ * 2 + 2 + int_ / 2;
    int32_t sp = str_ * 2 + 2;
    rend.draw_text(std::to_string(hp), 550, 192, value_color);
    rend.draw_text(std::to_string(mp), 550, 208, value_color);
    rend.draw_text(std::to_string(sp), 550, 224, value_color);

    // === Character Preview ===
    if (char_renderer_) {
        // Draw character preview in the right side panel
        int32_t char_x = 500;  // Center of preview area
        int32_t char_y = 130;  // Vertical position

        character_appearance appearance;
        appearance.gender = gender_;
        appearance.skin_color = skin_color_;
        appearance.hair_style = hair_style_;
        appearance.hair_color = hair_color_;
        appearance.underwear_color = underwear_color_;

        char_renderer_->draw(rend, sprites, char_x, char_y, appearance, menu_dir_, menu_frame_);
    }

    // === Bottom buttons ===
    bool can_create = is_valid_character();

    // Draw Create button (frames 24/25)
    if (can_create && current_focus_ == 2) {
        draw_sprite(rend, sprites, charcreate_sprites::button, 384, 445, 25);  // Highlighted
    } else {
        draw_sprite(rend, sprites, charcreate_sprites::button, 384, 445, 24);  // Normal
    }

    // Draw Cancel button (frames 16/17)
    if (current_focus_ == 3) {
        draw_sprite(rend, sprites, charcreate_sprites::button, 500, 445, 17);  // Highlighted
    } else {
        draw_sprite(rend, sprites, charcreate_sprites::button, 500, 445, 16);  // Normal
    }

    // Draw Warrior preset button (frames 67/68)
    if (current_focus_ == 4) {
        draw_sprite(rend, sprites, charcreate_sprites::button, 60, 445, 68);   // Highlighted
    } else {
        draw_sprite(rend, sprites, charcreate_sprites::button, 60, 445, 67);   // Normal
    }

    // Draw Mage preset button (frames 65/66)
    if (current_focus_ == 5) {
        draw_sprite(rend, sprites, charcreate_sprites::button, 145, 445, 66);  // Highlighted
    } else {
        draw_sprite(rend, sprites, charcreate_sprites::button, 145, 445, 65);  // Normal
    }

    // Draw Merchant preset button (frames 63/64)
    if (current_focus_ == 6) {
        draw_sprite(rend, sprites, charcreate_sprites::button, 230, 445, 64);  // Highlighted
    } else {
        draw_sprite(rend, sprites, charcreate_sprites::button, 230, 445, 63);  // Normal
    }

    // Draw mouse cursor
    draw_sprite(rend, sprites, charcreate_sprites::mouse_cursor, mouse_x, mouse_y, 0);
}

} // namespace hb
