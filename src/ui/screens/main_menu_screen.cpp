#include "ui/screens/main_menu_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#endif

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

    // Setup clickable rectangles (button highlight is 164x22 pixels)
    mouse_interface_.clear();
    mouse_interface_.add_rect(btn1_x_, btn1_y_, btn1_x_ + btn_width_, btn1_y_ + btn_height_);  // Button 1: Start Game
    mouse_interface_.add_rect(btn2_x_, btn2_y_, btn2_x_ + btn_width_, btn2_y_ + btn_height_);  // Button 2: Create Account
    mouse_interface_.add_rect(btn3_x_, btn3_y_, btn3_x_ + btn_width_, btn3_y_ + btn_height_);  // Button 3: Quit

#ifdef HB_DEBUG_OVERLAY_ENABLED
    register_debug_adapters();
#endif

    spdlog::info("Main menu screen entered");
}

void main_menu_screen::on_exit() {
#ifdef HB_DEBUG_OVERLAY_ENABLED
    unregister_debug_adapters();
#endif
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
                play_button_sound();
                if (on_start_) {
                    on_start_();
                }
                return true;
            case 2:  // Create Account / Website (skip for now)
                play_button_sound();
                break;
            case 3:  // Quit
                play_button_sound();
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
        play_button_sound();
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
    if (mouse_x_ >= btn1_x_ && mouse_x_ <= btn1_x_ + btn_width_ && mouse_y_ >= btn1_y_ && mouse_y_ <= btn1_y_ + btn_height_) current_focus_ = 1;
    if (mouse_x_ >= btn2_x_ && mouse_x_ <= btn2_x_ + btn_width_ && mouse_y_ >= btn2_y_ && mouse_y_ <= btn2_y_ + btn_height_) current_focus_ = 2;
    if (mouse_x_ >= btn3_x_ && mouse_x_ <= btn3_x_ + btn_width_ && mouse_y_ >= btn3_y_ && mouse_y_ <= btn3_y_ + btn_height_) current_focus_ = 3;

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

    // Draw button highlight based on current focus (use adjustable positions)
    // Sprite frames (from New-Dialog[1]):
    // - Frame 0: Background (640x480)
    // - Frame 1: Start Game highlight
    // - Frame 2: Create Account highlight
    // - Frame 3: Quit highlight
    switch (current_focus_) {
        case 1:  // Start Game highlight
            draw_sprite(rend, sprites, main_menu_sprites::background, btn1_x_, btn1_y_, 1);
            break;
        case 2:  // Create Account highlight
            draw_sprite(rend, sprites, main_menu_sprites::background, btn2_x_, btn2_y_, 2);
            break;
        case 3:  // Quit highlight
            draw_sprite(rend, sprites, main_menu_sprites::background, btn3_x_, btn3_y_, 3);
            break;
    }

    // Mouse cursor is drawn separately via render_cursor()
}

void main_menu_screen::render_cursor(renderer& rend, sprite_manager& sprites) {
    draw_sprite(rend, sprites, main_menu_sprites::mouse_cursor, mouse_x_, mouse_y_, 0);
}

#ifdef HB_DEBUG_OVERLAY_ENABLED
void main_menu_screen::register_debug_adapters() {
    debug_adapters_.clear();

    // Button adapters
    debug_adapters_.push_back(std::make_unique<debug::screen_point_adapter>(
        "main_menu.btn_start", btn1_x_, btn1_y_, btn_width_, btn_height_));

    debug_adapters_.push_back(std::make_unique<debug::screen_point_adapter>(
        "main_menu.btn_create", btn2_x_, btn2_y_, btn_width_, btn_height_));

    debug_adapters_.push_back(std::make_unique<debug::screen_point_adapter>(
        "main_menu.btn_quit", btn3_x_, btn3_y_, btn_width_, btn_height_));

    // Register all adapters with debug overlay
    auto& overlay = debug::debug_overlay::instance();
    for (auto& adapter : debug_adapters_) {
        overlay.register_element(adapter.get());
    }
}

void main_menu_screen::unregister_debug_adapters() {
    auto& overlay = debug::debug_overlay::instance();
    for (auto& adapter : debug_adapters_) {
        overlay.unregister_element(adapter.get());
    }
    debug_adapters_.clear();
}
#endif

} // namespace hb
