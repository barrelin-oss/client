#include "ui/cursor.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"

namespace hb
{

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
    if (!sprites_)
        return;

    const sprite* spr = sprites_->get_sprite_by_id(cursor_sprite_id);
    if (!spr)
        return;

    rend.draw_sprite(*spr, mouse_x, mouse_y, frame_for(current_));
}

uint32_t cursor_manager::frame_for(cursor_type type)
{
    switch (type)
    {
    case cursor_type::ground_item_grab:
    {
        auto now = std::chrono::steady_clock::now();
        if (now - grab_anim_time_ > std::chrono::milliseconds(200))
        {
            grab_anim_time_ = now;
            grab_anim_frame_ = (grab_anim_frame_ == 1) ? 2 : 1;
        }
        return grab_anim_frame_;
    }
    case cursor_type::attack:
        return 3;
    case cursor_type::magic_target:
        return 4;
    case cursor_type::magic_attack:
        return 5;
    case cursor_type::friendly:
        return 6;
    case cursor_type::magic_unavailable:
        return 8;
    case cursor_type::item_use_target:
        return 10;
    case cursor_type::normal:
    default:
        return 0;
    }
}

} // namespace hb
