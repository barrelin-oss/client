#include "ui/screens/screen_manager.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void screen_manager::initialize() {
    spdlog::info("Screen manager initialized");
}

void screen_manager::shutdown() {
    if (current_screen_) {
        current_screen_->on_exit();
        current_screen_ = nullptr;
    }
    current_type_ = screen_type::none;
    spdlog::info("Screen manager shutdown");
}

void screen_manager::change_screen(screen_type type) {
    if (type == current_type_) {
        return;
    }

    // Exit current screen
    if (current_screen_) {
        current_screen_->on_exit();
    }

    // Switch to new screen
    current_type_ = type;
    current_screen_ = get_screen(type);

    // Enter new screen
    if (current_screen_) {
        current_screen_->on_enter();
    }

    // Notify listeners
    if (on_state_change_) {
        on_state_change_(type);
    }

    spdlog::info("Screen changed to: {}", static_cast<int>(type));
}

void screen_manager::update(float delta_time, const input& inp) {
    if (current_screen_) {
        current_screen_->update(delta_time, inp);
    }
}

void screen_manager::update_mouse_position(const input& inp) {
    if (current_screen_) {
        current_screen_->update_mouse_position(inp);
    }
}

void screen_manager::render(renderer& rend, sprite_manager& sprites) {
    if (current_screen_) {
        current_screen_->render(rend, sprites);
    }
}

void screen_manager::render_cursor(renderer& rend, sprite_manager& sprites) {
    if (current_screen_) {
        current_screen_->render_cursor(rend, sprites);
    }
}

screen_base* screen_manager::get_screen(screen_type type) {
    switch (type) {
        case screen_type::main_menu:
            return &main_menu_;
        case screen_type::login:
            return &login_;
        case screen_type::character_select:
            return &character_select_;
        case screen_type::character_create:
            return &character_create_;
        default:
            return nullptr;
    }
}

} // namespace hb
