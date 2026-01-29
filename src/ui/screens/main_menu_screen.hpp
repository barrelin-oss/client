#pragma once

#include "ui/screens/screen_base.hpp"
#include <functional>

namespace hb {

// Sprite IDs for main menu (matching original DEF_SPRID_INTERFACE_ND_MAINMENU)
namespace main_menu_sprites {
    inline constexpr uint16_t background = 52;  // DEF_SPRID_INTERFACE_ND_MAINMENU
    inline constexpr uint16_t mouse_cursor = 0; // DEF_SPRID_MOUSECURSOR
}

// Main menu button IDs
enum class main_menu_button : int32_t {
    none = 0,
    start_game = 1,    // Start button
    quit_game = 2      // Quit button (if exists)
};

// Main menu screen - modern equivalent of UpdateScreen_OnMainMenu
class main_menu_screen : public screen_base {
public:
    using start_callback = std::function<void()>;
    using quit_callback = std::function<void()>;

    main_menu_screen() = default;
    ~main_menu_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;

    // Set callbacks
    void set_on_start(start_callback callback) { on_start_ = std::move(callback); }
    void set_on_quit(quit_callback callback) { on_quit_ = std::move(callback); }

private:
    // Draw the screen
    void draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y);

    start_callback on_start_;
    quit_callback on_quit_;
};

} // namespace hb
