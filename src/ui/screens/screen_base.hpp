#pragma once

#include "ui/mouse_interface.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace hb
{

class renderer;
class input;
class sprite_manager;

// Base class for sprite-based screens (login, main menu, character select, etc.)
// This is the modern equivalent of the original UpdateScreen_OnXXX pattern
class screen_base
{
public:
    using sound_callback = std::function<void()>;

    screen_base() = default;
    virtual ~screen_base() = default;

    screen_base(const screen_base&) = delete;
    screen_base& operator=(const screen_base&) = delete;
    screen_base(screen_base&&) noexcept = default;
    screen_base& operator=(screen_base&&) noexcept = default;

    // Called when entering this screen
    virtual void on_enter() = 0;

    // Called when leaving this screen
    virtual void on_exit() = 0;

    // Update logic (input handling, state changes)
    // Returns true if screen should remain active
    virtual bool update(float delta_time, const input& inp) = 0;

    // Update just the mouse position (for rendering cursor when input is blocked)
    void update_mouse_position(const input& inp);

    // Render the screen (sprite drawing)
    virtual void render(renderer& rend, sprite_manager& sprites) = 0;

    // Set sound callback for button clicks
    void set_on_button_sound(sound_callback cb) { on_button_sound_ = std::move(cb); }

protected:
    // Play button click sound (call when any button is clicked)
    void play_button_sound()
    {
        if (on_button_sound_)
            on_button_sound_();
    }
    // Helper to draw a sprite from the sprite manager (with color key transparency)
    void
    draw_sprite(renderer& rend, sprite_manager& sprites, uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame = 0);

    // Helper to draw a sprite with alpha transparency
    void draw_sprite_alpha(
        renderer& rend, sprite_manager& sprites, uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame, float alpha);

    // Helper to draw a sprite WITHOUT color key (for backgrounds)
    void draw_sprite_no_color_key(
        renderer& rend, sprite_manager& sprites, uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame = 0);

    // Helper to draw a sprite with alpha but WITHOUT color key
    void draw_sprite_alpha_no_color_key(
        renderer& rend, sprite_manager& sprites, uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame, float alpha);

    // Mouse interface for click detection
    mouse_interface mouse_interface_;

    // Game mode count (for animations/timing)
    int32_t mode_count_ = 0;

    // Focus tracking (like m_cCurFocus)
    int32_t current_focus_ = 0;
    int32_t max_focus_ = 0;

    // Arrow key tracking
    int8_t arrow_pressed_ = 0;

    // Enter/Escape key tracking
    bool enter_pressed_ = false;
    bool escape_pressed_ = false;

    // Mouse position (stored during update for render)
    int32_t mouse_x_ = 0;
    int32_t mouse_y_ = 0;

    // Screen centering (for 640x480 design resolution)
    static constexpr int32_t design_width_ = 640;
    static constexpr int32_t design_height_ = 480;
    int32_t screen_offset_x_ = 0;
    int32_t screen_offset_y_ = 0;

    // Calculate centering offset based on current window size
    void update_screen_offset(uint32_t window_width, uint32_t window_height);

    // Get adjusted mouse coordinates (subtracts offset for hit testing)
    std::pair<int32_t, int32_t> get_adjusted_mouse() const;

    // Sound callback for button clicks
    sound_callback on_button_sound_;
};

} // namespace hb
