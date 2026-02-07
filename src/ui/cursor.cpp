#include "ui/cursor.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"

namespace hb {

void cursor_manager::begin_frame()
{
    current_ = cursor_type::normal;
}

void cursor_manager::set_cursor(cursor_type type)
{
    current_ = type;
}

void cursor_manager::render(renderer& rend, int32_t mouse_x, int32_t mouse_y)
{
    if (!sprites_) return;

    const sprite* spr = sprites_->get_sprite_by_id(cursor_sprite_id);
    if (!spr) return;

    rend.draw_sprite(*spr, mouse_x, mouse_y, frame_for(current_));
}

uint32_t cursor_manager::frame_for(cursor_type type) const
{
    switch (type)
    {
        case cursor_type::normal:
        default:
            return 0;
    }
}

} // namespace hb
