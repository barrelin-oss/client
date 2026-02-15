#pragma once

#include "ui/screens/screen_base.hpp"
#include <functional>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/screen_adapters.hpp"
#include <memory>
#include <vector>
#endif

namespace hb
{

// Sprite IDs for main menu (matching original DEF_SPRID_INTERFACE_ND_MAINMENU)
namespace main_menu_sprites
{
inline constexpr uint16_t background = 52;  // DEF_SPRID_INTERFACE_ND_MAINMENU
inline constexpr uint16_t mouse_cursor = 0; // DEF_SPRID_MOUSECURSOR
} // namespace main_menu_sprites

// Main menu button IDs
enum class main_menu_button : int32_t
{
    none = 0,
    start_game = 1, // Start button
    quit_game = 2   // Quit button (if exists)
};

// Main menu screen - modern equivalent of UpdateScreen_OnMainMenu
class main_menu_screen : public screen_base
{
public:
    using start_callback = std::function<void()>;
    using quit_callback = std::function<void()>;
    using settings_callback = std::function<void()>;

    main_menu_screen() = default;
    ~main_menu_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;

    // Set callbacks
    void set_on_start(start_callback callback) { on_start_ = std::move(callback); }
    void set_on_quit(quit_callback callback) { on_quit_ = std::move(callback); }
    void set_on_settings(settings_callback callback) { on_settings_ = std::move(callback); }

private:
    // Draw the screen
    void draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y);

    start_callback on_start_;
    quit_callback on_quit_;
    settings_callback on_settings_;

    // Button highlight positions (mutable for debug overlay positioning)
    int32_t btn1_x_ = 385; // Start Game
    int32_t btn1_y_ = 178;
    int32_t btn2_x_ = 385; // Create Account
    int32_t btn2_y_ = 217;
    int32_t btn3_x_ = 385; // Quit
    int32_t btn3_y_ = 255;

    // Button dimensions (from sprite)
    static constexpr int32_t btn_width_ = 164;
    static constexpr int32_t btn_height_ = 22;

    // Settings button (bottom-right corner, resolution-independent)
    static constexpr int32_t settings_btn_width_ = 80;
    static constexpr int32_t settings_btn_height_ = 28;
    static constexpr int32_t settings_btn_margin_ = 12;
    bool settings_hovered_ = false;

    // Cached window dimensions (updated each render frame)
    uint32_t window_width_ = 0;
    uint32_t window_height_ = 0;

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Debug overlay adapters
    void register_debug_adapters();
    void unregister_debug_adapters();
    std::vector<std::unique_ptr<debug::screen_point_adapter>> debug_adapters_;
#endif
};

} // namespace hb
