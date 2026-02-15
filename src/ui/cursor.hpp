#pragma once

#include <chrono>
#include <cstdint>

namespace hb
{

class renderer;
class sprite_manager;

// Cursor types matching legacy interface.pak sprite 0 frames.
// Each frame resets to normal, so contextual cursors must be set every frame.
enum class cursor_type : uint8_t
{
    normal = 0,            // Default arrow (frame 0)
    ground_item_grab = 1,  // Animated grab hand over ground items (frames 1/2, 200ms toggle)
    attack = 2,            // Attack sword over hostile entity (frame 3)
    magic_target = 3,      // Magic targeting crosshair on friendly/ground (frame 4)
    magic_attack = 4,      // Magic targeting on hostile entity (frame 5)
    friendly = 5,          // Hovering over a friendly entity (frame 6)
    magic_unavailable = 6, // Spell not available / out of range (frame 8)
    item_use_target = 7,   // Item use targeting mode (frame 10)
};

// Centralized cursor manager. Renders the software cursor as the very last draw call
// each frame, ensuring it's always visible on top of all game content.
//
// Usage:
//   1. Call begin_frame() at the start of each render pass (resets to normal cursor)
//   2. Game code calls set_cursor() during update/render to change cursor type
//   3. Call render() as the absolute last draw call before end_frame()
class cursor_manager
{
public:
    void set_sprite_manager(sprite_manager* sprites) { sprites_ = sprites; }

    // Reset cursor to default at the start of each frame
    void begin_frame();

    // Set cursor type for this frame (call during update or render)
    void set_cursor(cursor_type type);

    // Draw the cursor at the given mouse position - call LAST
    void render(renderer& rend, int32_t mouse_x, int32_t mouse_y);

private:
    // Sprite ID for the cursor (sprite 0 = DEF_SPRID_MOUSECURSOR from interface.pak)
    static constexpr uint16_t cursor_sprite_id = 0;

    sprite_manager* sprites_ = nullptr;
    cursor_type current_ = cursor_type::normal;

    // Animated grab cursor state (toggles between frames 1 and 2 every 200ms)
    std::chrono::steady_clock::time_point grab_anim_time_{};
    uint8_t grab_anim_frame_ = 1;

    uint32_t frame_for(cursor_type type);
};

} // namespace hb
