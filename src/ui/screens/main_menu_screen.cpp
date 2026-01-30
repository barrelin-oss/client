#include "ui/screens/main_menu_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

// For trace logging the draw calls
#define LOG_DRAW_CALLS 0

namespace hb {

void main_menu_screen::on_enter() {
    // Reset state
    mode_count_ = 0;
    current_focus_ = 1;
    max_focus_ = 3;
    enter_pressed_ = false;
    escape_pressed_ = false;
    arrow_pressed_ = 0;

    // Setup clickable rectangles (matching original coordinates)
    mouse_interface_.clear();
    mouse_interface_.add_rect(130, 160, 230, 185);  // Button 1: Start Game
    mouse_interface_.add_rect(90, 200, 265, 220);   // Button 2: Create Account / Website
    mouse_interface_.add_rect(140, 270, 230, 300);  // Button 3: Quit

    spdlog::info("Main menu screen entered");
}

void main_menu_screen::on_exit() {
    mouse_interface_.clear();
    spdlog::info("Main menu screen exited");
}

bool main_menu_screen::update(float delta_time, const input& inp) {
    (void)delta_time;

    // Update mode count (for animations)
    mode_count_++;
    if (mode_count_ > 100) mode_count_ = 100;

    // Log once per second
    if (mode_count_ == 1 || mode_count_ == 60) {
        spdlog::debug("Main menu update, mode_count={}", mode_count_);
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

    // Handle enter key
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter)) {
        switch (current_focus_) {
            case 1:  // Start Game
                if (on_start_) {
                    on_start_();
                }
                return true;
            case 2:  // Create Account / Website (skip for now)
                break;
            case 3:  // Quit
                if (on_quit_) {
                    on_quit_();
                }
                return true;
        }
    }

    // Handle escape key (quit)
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape)) {
        if (on_quit_) {
            on_quit_();
        }
        return true;
    }

    // Check mouse clicks
    mouse_result result;
    int32_t button_num = mouse_interface_.get_status(mouse_x_, mouse_y_, mouse_pressed, result);
    if (result == mouse_result::click) {
        switch (button_num) {
            case 1:  // Start Game
                if (on_start_) {
                    on_start_();
                }
                return true;
            case 2:  // Create Account / Website
                break;
            case 3:  // Quit
                if (on_quit_) {
                    on_quit_();
                }
                return true;
        }
    }

    // Update focus based on mouse hover position
    if (mouse_x_ >= 130 && mouse_y_ >= 160 && mouse_x_ <= 230 && mouse_y_ <= 185) current_focus_ = 1;
    if (mouse_x_ >= 90 && mouse_y_ >= 200 && mouse_x_ <= 265 && mouse_y_ <= 220) current_focus_ = 2;
    if (mouse_x_ >= 140 && mouse_y_ >= 270 && mouse_x_ <= 230 && mouse_y_ <= 300) current_focus_ = 3;

    return true;
}

void main_menu_screen::render(renderer& rend, sprite_manager& sprites) {
    draw(rend, sprites, mouse_x_, mouse_y_);
}

void main_menu_screen::draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y) {
    // Draw background (frame 0)
    // Original: DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_MAINMENU, -1, -1, 0, TRUE)
    // -1, -1 likely means centered or default position - using 0,0 for now
    spdlog::trace("Drawing main menu background at (0,0) frame 0");
    draw_sprite(rend, sprites, main_menu_sprites::background, 0, 0, 0);

    // Draw button highlight based on current focus
    // Sprite frames (from New-Dialog[1]):
    // - Frame 0: Background (640x480)
    // - Frame 1: rect(133,483 164x22) - Start Game highlight
    // - Frame 2: rect(300,483 164x22) - Create Account highlight
    // - Frame 3: rect(467,483 164x22) - Quit highlight
    switch (current_focus_) {
        case 1:  // Start Game highlight
            draw_sprite(rend, sprites, main_menu_sprites::background, 121, 161, 1);
            break;
        case 2:  // Create Account highlight
            draw_sprite(rend, sprites, main_menu_sprites::background, 96, 199, 2);
            break;
        case 3:  // Quit highlight (frame 3, not 4)
            draw_sprite(rend, sprites, main_menu_sprites::background, 121, 268, 3);
            break;
    }

    // Mouse cursor is drawn separately via render_cursor()
}

void main_menu_screen::render_cursor(renderer& rend, sprite_manager& sprites) {
    draw_sprite(rend, sprites, main_menu_sprites::mouse_cursor, mouse_x_, mouse_y_, 0);
}

} // namespace hb
