#include "ui/screens/login_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void login_screen::on_enter() {
    // Reset state
    mode_count_ = 0;
    current_focus_ = 1;  // Start on account name field
    max_focus_ = 4;
    enter_pressed_ = false;
    escape_pressed_ = false;
    arrow_pressed_ = 0;
    cursor_timer_ = 0.0f;
    cursor_visible_ = true;

    // Clear input
    account_name_.clear();
    password_.clear();

    // Setup clickable rectangles (matching original coordinates from UpdateScreen_OnLogin)
    mouse_interface_.clear();
    mouse_interface_.add_rect(80, 151, 337, 179);   // Button 1: Account name field
    mouse_interface_.add_rect(80, 180, 337, 205);   // Button 2: Password field
    mouse_interface_.add_rect(80, 280, 163, 302);   // Button 3: Connect button
    mouse_interface_.add_rect(258, 280, 327, 302);  // Button 4: Cancel button

    spdlog::info("Login screen entered");
}

void login_screen::on_exit() {
    mouse_interface_.clear();
    spdlog::info("Login screen exited");
}

bool login_screen::update(float delta_time, const input& inp) {
    // Update mode count
    mode_count_++;
    if (mode_count_ > 100) mode_count_ = 100;

    // Update cursor blink
    cursor_timer_ += delta_time;
    if (cursor_timer_ >= 0.5f) {
        cursor_timer_ = 0.0f;
        cursor_visible_ = !cursor_visible_;
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
    if (inp.is_key_pressed(sf::Keyboard::Key::Left) || inp.is_key_pressed(sf::Keyboard::Key::Right)) {
        // Swap between connect and cancel when on those buttons
        if (current_focus_ == 3) current_focus_ = 4;
        else if (current_focus_ == 4) current_focus_ = 3;
    }

    // Handle text input
    handle_text_input(inp);

    // Handle Tab key to move between fields
    if (inp.is_key_pressed(sf::Keyboard::Key::Tab)) {
        current_focus_++;
        if (current_focus_ > max_focus_) current_focus_ = 1;
    }

    // Handle Enter key
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter)) {
        switch (current_focus_) {
            case 1:  // On account name, move to password
                current_focus_ = 2;
                break;
            case 2:  // On password, try to login
            case 3:  // On connect button
                try_login();
                return true;
            case 4:  // On cancel button
                if (on_cancel_) {
                    on_cancel_();
                }
                return true;
        }
    }

    // Handle Escape key (cancel)
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape)) {
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    }

    // Check mouse clicks
    mouse_result result;
    int32_t button_num = mouse_interface_.get_status(mouse_x_, mouse_y_, mouse_pressed, result);
    if (result == mouse_result::click) {
        switch (button_num) {
            case 1:  // Account name field
                current_focus_ = 1;
                break;
            case 2:  // Password field
                current_focus_ = 2;
                break;
            case 3:  // Connect button
                try_login();
                return true;
            case 4:  // Cancel button
                if (on_cancel_) {
                    on_cancel_();
                }
                return true;
        }
    }

    // Update focus based on mouse hover for buttons
    if (mouse_x_ >= 80 && mouse_x_ <= 163 && mouse_y_ >= 280 && mouse_y_ <= 302) current_focus_ = 3;
    if (mouse_x_ >= 258 && mouse_x_ <= 327 && mouse_y_ >= 280 && mouse_y_ <= 302) current_focus_ = 4;

    return true;
}

void login_screen::render(renderer& rend, sprite_manager& sprites) {
    draw(rend, sprites, mouse_x_, mouse_y_);
}

void login_screen::handle_text_input(const input& inp) {
    // Get the current text buffer being edited
    std::string* current_buffer = nullptr;
    size_t max_len = 0;

    if (current_focus_ == 1) {
        current_buffer = &account_name_;
        max_len = account_max_length;
    } else if (current_focus_ == 2) {
        current_buffer = &password_;
        max_len = password_max_length;
    }

    if (!current_buffer) return;

    // Handle backspace
    if (inp.is_key_pressed(sf::Keyboard::Key::Backspace)) {
        if (!current_buffer->empty()) {
            current_buffer->pop_back();
        }
    }

    // Handle text input from the input system
    std::string_view text_input = inp.text_input();
    for (char c : text_input) {
        // Only allow printable ASCII characters
        if (c >= 32 && c < 127 && current_buffer->size() < max_len) {
            current_buffer->push_back(c);
        }
    }
}

void login_screen::try_login() {
    spdlog::info("try_login called: account='{}' password_len={}", account_name_, password_.length());

    if (account_name_.empty() || password_.empty()) {
        spdlog::warn("Login failed: account name or password is empty");
        return;
    }

    if (on_login_) {
        spdlog::info("Calling on_login_ callback with account='{}' password_len={}", account_name_, password_.length());
        on_login_(account_name_, password_);
    }
}

void login_screen::draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y) {
    // Draw background (frame 0)
    draw_sprite(rend, sprites, login_sprites::background, 0, 0, 0);

    // LoginDialog[0] sprite frames:
    // - Frame 0: Background (640x480)
    // - Frame 1: Server select panel (332x184) - SKIPPED
    // - Frame 2: Login details panel (332x184) - account/password input
    // - Frame 3: Connect button highlight (84x20)
    // - Frame 4: Cancel button highlight (76x20)
    // - Frame 5-7: Text elements

    // Draw the login details panel (frame 2) after fade-in animation
    if (mode_count_ >= 15 && mode_count_ <= 20) {
        draw_sprite_alpha(rend, sprites, login_sprites::background, 154, 148, 2, 0.25f);
    } else if (mode_count_ > 20) {
        draw_sprite(rend, sprites, login_sprites::background, 154, 148, 2);
    }

    // Draw button highlights based on focus
    bool can_connect = !account_name_.empty() && !password_.empty();

    // Connect button highlight (frame 3)
    if (can_connect && current_focus_ == 3) {
        draw_sprite(rend, sprites, login_sprites::background, 80, 282, 3);
    }
    // Cancel button highlight (frame 4)
    if (current_focus_ == 4) {
        draw_sprite(rend, sprites, login_sprites::background, 256, 282, 4);
    }

    // DEBUG: Draw outlines around click areas
    // Account name field (click area 1): (80, 151) to (337, 179)
    sf::Color account_outline = (current_focus_ == 1) ? sf::Color::Yellow : sf::Color(100, 100, 100);
    rend.draw_rect(80, 151, 257, 28, account_outline, false);

    // Password field (click area 2): (80, 180) to (337, 205)
    sf::Color password_outline = (current_focus_ == 2) ? sf::Color::Yellow : sf::Color(100, 100, 100);
    rend.draw_rect(80, 180, 257, 25, password_outline, false);

    // Connect button (click area 3): (80, 280) to (163, 302)
    sf::Color connect_outline = (current_focus_ == 3) ? sf::Color::Yellow : sf::Color(100, 100, 100);
    rend.draw_rect(80, 280, 83, 22, connect_outline, false);

    // Cancel button (click area 4): (258, 280) to (327, 302)
    sf::Color cancel_outline = (current_focus_ == 4) ? sf::Color::Yellow : sf::Color(100, 100, 100);
    rend.draw_rect(258, 280, 69, 22, cancel_outline, false);

    // Draw account name text inside its click area
    if (current_focus_ != 1) {
        sf::Color text_color = sf::Color(200, 200, 200);
        rend.draw_text(account_name_, 85, 155, text_color);
    } else {
        // Draw with cursor when editing
        std::string display_text = account_name_;
        if (cursor_visible_) {
            display_text += "_";
        }
        rend.draw_text(display_text, 85, 155, sf::Color::White);
    }

    // Draw password text (masked with asterisks) inside its click area
    std::string masked_password(password_.size(), '*');
    if (current_focus_ != 2) {
        sf::Color text_color = sf::Color(200, 200, 200);
        rend.draw_text(masked_password, 85, 184, text_color);
    } else {
        // Draw with cursor when editing
        if (cursor_visible_) {
            masked_password += "_";
        }
        rend.draw_text(masked_password, 85, 184, sf::Color::White);
    }

    // Mouse cursor is drawn separately via render_cursor()
}

void login_screen::render_cursor(renderer& rend, sprite_manager& sprites) {
    draw_sprite(rend, sprites, login_sprites::mouse_cursor, mouse_x_, mouse_y_, 0);
}

} // namespace hb
