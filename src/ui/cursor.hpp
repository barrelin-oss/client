#pragma once

#include <cstdint>

namespace hb {

class renderer;
class sprite_manager;

// Cursor types - game code sets this during update to control which cursor sprite is shown.
// Each frame resets to normal, so contextual cursors must be set every frame.
enum class cursor_type : uint8_t
{
    normal = 0,
    magic_target = 1,   // Magic targeting crosshair (frame 4)
    magic_arrow = 2,    // Magic targeting on enemy (frame 5)
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

    uint32_t frame_for(cursor_type type) const;
};

} // namespace hb
