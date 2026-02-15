#include "ui/screens/character_select_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void character_select_screen::on_enter()
{
    // Reset state
    mode_count_ = 0;
    current_focus_ = 1; // Start on first character slot
    max_focus_ = 4;     // 4 character slots
    enter_pressed_ = false;
    escape_pressed_ = false;
    arrow_pressed_ = 0;

    // Animation state
    menu_frame_ = 0;
    menu_dir_ = 1;
    menu_dir_count_ = 0;
    frame_timer_ = 0.0f;

    // Setup clickable rectangles (matching original coordinates from UpdateScreen_OnSelectCharacter)
    // Character slots 1-4
    mouse_interface_.clear();
    mouse_interface_.add_rect(100, 50, 210, 250); // Button 1: Character slot 1
    mouse_interface_.add_rect(211, 50, 321, 250); // Button 2: Character slot 2
    mouse_interface_.add_rect(322, 50, 431, 250); // Button 3: Character slot 3
    mouse_interface_.add_rect(432, 50, 542, 250); // Button 4: Character slot 4

    // Action buttons (5 buttons total)
    mouse_interface_.add_rect(360, 283, 545, 315); // Button 5: Enter Game (Start)
    mouse_interface_.add_rect(360, 316, 545, 345); // Button 6: New Character
    mouse_interface_.add_rect(360, 346, 545, 375); // Button 7: Delete Character
    mouse_interface_.add_rect(360, 376, 545, 405); // Button 8: Change Password (no action)
    mouse_interface_.add_rect(360, 406, 545, 435); // Button 9: Log Out

    spdlog::info("Character select screen entered");
}

void character_select_screen::on_exit()
{
    mouse_interface_.clear();
    spdlog::info("Character select screen exited");
}

void character_select_screen::set_characters(const std::vector<char_slot_info>& characters)
{
    total_characters_ = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        if (i < characters.size())
        {
            characters_[i] = characters[i];
            if (characters_[i].has_character)
            {
                total_characters_++;
            }
        }
        else
        {
            characters_[i] = char_slot_info{};
        }
    }
    spdlog::info("Set {} characters for selection", total_characters_);
}

bool character_select_screen::update(float delta_time, const input& inp)
{
    // Update mode count
    mode_count_++;
    if (mode_count_ > 100)
        mode_count_ = 100;

    // Store mouse position for render
    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();
    bool mouse_pressed = inp.is_mouse_pressed(sf::Mouse::Button::Left);

    // Get adjusted mouse coordinates for hit testing within 640x480 design space
    auto [adj_x, adj_y] = get_adjusted_mouse();

    // Update animation timer
    frame_timer_ += delta_time;
    if (frame_timer_ >= 0.1f)
    { // 100ms per frame
        frame_timer_ = 0.0f;
        menu_frame_++;
        if (menu_frame_ >= 8)
        {
            menu_frame_ = 0;
            menu_dir_count_++;
            if (menu_dir_count_ > 8)
            {
                menu_dir_++;
                menu_dir_count_ = 1;
            }
        }
        if (menu_dir_ > 8)
            menu_dir_ = 1;
    }

    // Handle arrow key navigation
    if (inp.is_key_pressed(sf::Keyboard::Key::Left))
    {
        current_focus_--;
        if (current_focus_ <= 0)
            current_focus_ = max_focus_;
    }
    if (inp.is_key_pressed(sf::Keyboard::Key::Right))
    {
        current_focus_++;
        if (current_focus_ > max_focus_)
            current_focus_ = 1;
    }

    // Handle Escape key (logout)
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape))
    {
        play_button_sound();
        logout();
        return true;
    }

    // Handle Enter key
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter))
    {
        int32_t slot_index = current_focus_ - 1;
        if (slot_index >= 0 && slot_index < 4)
        {
            play_button_sound();
            if (characters_[slot_index].has_character)
            {
                // Enter game with selected character
                enter_game();
            }
            else
            {
                // Create new character in empty slot
                create_character();
            }
        }
        return true;
    }

    // Check settings button hover/click (bottom-right, resolution-independent)
    if (window_width_ > 0 && window_height_ > 0)
    {
        int32_t sbx = static_cast<int32_t>(window_width_) - settings_btn_width_ - settings_btn_margin_;
        int32_t sby = static_cast<int32_t>(window_height_) - settings_btn_height_ - settings_btn_margin_;
        settings_hovered_ = mouse_x_ >= sbx && mouse_x_ <= sbx + settings_btn_width_ && mouse_y_ >= sby &&
                            mouse_y_ <= sby + settings_btn_height_;

        if (settings_hovered_ && mouse_pressed)
        {
            play_button_sound();
            if (on_settings_)
            {
                on_settings_();
            }
            return true;
        }
    }

    // Check mouse clicks (use adjusted coords for 640x480 design space)
    mouse_result result;
    int32_t button_num = mouse_interface_.get_status(adj_x, adj_y, mouse_pressed, result);

    if (result == mouse_result::click)
    {
        play_button_sound();
        switch (button_num)
        {
        case btn_char1:
        case btn_char2:
        case btn_char3:
        case btn_char4:
            // Select character slot or double-click to enter
            if (current_focus_ != button_num)
            {
                current_focus_ = button_num;
            }
            else
            {
                // Double-click: enter game or create new
                int32_t slot_index = button_num - 1;
                if (characters_[slot_index].has_character)
                {
                    enter_game();
                }
                else
                {
                    create_character();
                }
            }
            break;

        case btn_enter_game:
            enter_game();
            break;

        case btn_new_char:
            create_character();
            break;

        case btn_delete_char:
            delete_character();
            break;

        case btn_change_password:
            // No action implemented for Change Password
            break;

        case btn_logout:
            logout();
            return true;
        }
    }

    return true;
}

void character_select_screen::render(renderer& rend, sprite_manager& sprites)
{
    window_width_ = rend.width();
    window_height_ = rend.height();
    update_screen_offset(window_width_, window_height_);
    draw(rend, sprites, mouse_x_, mouse_y_);
}

void character_select_screen::draw(renderer& rend,
                                   sprite_manager& sprites,
                                   [[maybe_unused]] int32_t mouse_x,
                                   [[maybe_unused]] int32_t mouse_y)
{
    // Adjusted mouse for hover detection in 640x480 design space
    auto [adj_mx, adj_my] = get_adjusted_mouse();

    // Draw background (GameDialog sprite 8, frame 0) - NO color key for full-screen backgrounds
    draw_sprite_no_color_key(rend, sprites, charselect_sprites::select_char, 0, 0, 0);

    // Draw button bar (DialogText sprite 1, frame 50) - WITH color key for UI overlays
    draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 50);

    // Draw character slot selection highlights
    for (int i = 0; i < 4; ++i)
    {
        int32_t slot_x = 110 + i * 109 - 7;
        int32_t slot_y = 63 - 9;

        if (current_focus_ - 1 == i)
        {
            // Selected slot - use frame 62
            draw_sprite(rend, sprites, charselect_sprites::button, slot_x, slot_y, 62);
        }
        else
        {
            // Unselected slot - use frame 61
            draw_sprite(rend, sprites, charselect_sprites::button, slot_x, slot_y, 61);
        }

        // Draw character if present
        if (characters_[i].has_character)
        {
            // Draw character sprite preview
            if (char_renderer_)
            {
                // Character position in slot (centered), offset for screen centering
                int32_t char_x = 160 + i * 109 + screen_offset_x_;
                int32_t char_y = 130 + screen_offset_y_;

                character_appearance appearance;
                appearance.gender = characters_[i].gender;
                appearance.skin_color = characters_[i].skin_color;
                appearance.hair_style = characters_[i].hair_style;
                appearance.hair_color = characters_[i].hair_color;
                appearance.underwear_color = characters_[i].underwear_color;
                // Equipment
                appearance.body_armor = characters_[i].body_armor;
                appearance.arm_armor = characters_[i].arm_armor;
                appearance.pants = characters_[i].pants;
                appearance.boots = characters_[i].boots;
                appearance.helmet = characters_[i].helmet;
                appearance.mantle = characters_[i].mantle;
                appearance.weapon = characters_[i].weapon;
                appearance.shield = characters_[i].shield;

                char_renderer_->draw(rend, sprites, char_x, char_y, appearance, menu_dir_, menu_frame_);
            }

            // Draw character info text
            int32_t text_x = 112 + i * 109 + screen_offset_x_;
            int32_t text_y = 179 - 9 + 10 + screen_offset_y_; // sY = 10

            // Character name
            rend.draw_text(characters_[i].name, text_x, text_y, sf::Color(200, 200, 200));

            // Level
            std::string level_str = std::to_string(characters_[i].level);
            rend.draw_text(level_str, text_x + 26, text_y + 17, sf::Color(200, 200, 200));

            // Exp
            std::string exp_str = std::to_string(characters_[i].exp);
            rend.draw_text(exp_str, text_x + 26, text_y + 32, sf::Color(200, 200, 200));
        }
    }

    // Draw action buttons (frames 51-55)
    draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 51); // Enter Game (Start)
    draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 52); // New Character
    draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 53); // Delete Character
    draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 54); // Change Password
    draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 55); // Log Out

    // Draw button highlights on hover (use adjusted mouse for design space)
    if (adj_mx > 360 && adj_my >= 283 && adj_mx < 545 && adj_my <= 315)
    {
        draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 56); // Enter Game highlight
    }
    else if (adj_mx > 360 && adj_my >= 316 && adj_mx < 545 && adj_my <= 345)
    {
        draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 57); // New Character highlight
    }
    else if (adj_mx > 360 && adj_my >= 346 && adj_mx < 545 && adj_my <= 375)
    {
        draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 58); // Delete Character highlight
    }
    else if (adj_mx > 360 && adj_my >= 376 && adj_mx < 545 && adj_my <= 405)
    {
        draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 59); // Change Password highlight
    }
    else if (adj_mx > 360 && adj_my >= 406 && adj_mx < 545 && adj_my <= 435)
    {
        draw_sprite(rend, sprites, charselect_sprites::button, 0, 0, 60); // Log Out highlight
    }

    // Draw help text for empty slots
    if (total_characters_ == 0)
    {
        rend.draw_text(
            "No characters found.", 150 + screen_offset_x_, 290 + screen_offset_y_, sf::Color(200, 200, 200));
        rend.draw_text("Click 'New Character' to create one.",
                       120 + screen_offset_x_,
                       310 + screen_offset_y_,
                       sf::Color(180, 180, 180));
    }

    // Settings button (bottom-right corner)
    if (window_width_ > 0 && window_height_ > 0)
    {
        int32_t sbx = static_cast<int32_t>(window_width_) - settings_btn_width_ - settings_btn_margin_;
        int32_t sby = static_cast<int32_t>(window_height_) - settings_btn_height_ - settings_btn_margin_;

        auto bg_color = settings_hovered_ ? sf::Color(80, 80, 80, 200) : sf::Color(40, 40, 40, 180);
        auto border_color = settings_hovered_ ? sf::Color(200, 200, 200, 220) : sf::Color(140, 140, 140, 180);

        rend.draw_rect(sbx, sby, settings_btn_width_, settings_btn_height_, bg_color, true);
        rend.draw_rect(sbx, sby, settings_btn_width_, settings_btn_height_, border_color, false);

        int32_t text_x = sbx + (settings_btn_width_ / 2) - 22;
        int32_t text_y = sby + (settings_btn_height_ / 2) - 6;
        rend.draw_text("Settings", text_x, text_y, sf::Color::White, 12);
    }
}

void character_select_screen::enter_game()
{
    int32_t slot_index = current_focus_ - 1;
    if (slot_index >= 0 && slot_index < 4 && characters_[slot_index].has_character)
    {
        spdlog::info("Entering game with character: {}", characters_[slot_index].name);
        if (on_select_)
        {
            on_select_(slot_index);
        }
    }
}

void character_select_screen::create_character()
{
    if (total_characters_ < 4)
    {
        spdlog::info("Creating new character");
        if (on_create_)
        {
            on_create_();
        }
    }
    else
    {
        spdlog::warn("Cannot create character: maximum 4 characters reached");
    }
}

void character_select_screen::delete_character()
{
    int32_t slot_index = current_focus_ - 1;
    if (slot_index >= 0 && slot_index < 4 && characters_[slot_index].has_character)
    {
        spdlog::info("Delete character requested: {}", characters_[slot_index].name);
        if (on_delete_)
        {
            on_delete_(slot_index);
        }
    }
}

void character_select_screen::logout()
{
    spdlog::info("Logging out");
    if (on_logout_)
    {
        on_logout_();
    }
}

} // namespace hb
