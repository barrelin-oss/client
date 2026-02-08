#include "ui/screens/screen_base.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void screen_base::update_mouse_position(const input& inp) {
    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();
}

void screen_base::draw_sprite(renderer& rend, sprite_manager& sprites,
                              uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite(*spr, x + screen_offset_x_, y + screen_offset_y_, frame);
    }
}

void screen_base::draw_sprite_alpha(renderer& rend, sprite_manager& sprites,
                                    uint16_t sprite_id, int32_t x, int32_t y,
                                    uint32_t frame, float alpha) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite_alpha(*spr, x + screen_offset_x_, y + screen_offset_y_, frame, alpha);
    }
}

void screen_base::draw_sprite_no_color_key(renderer& rend, sprite_manager& sprites,
                                           uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite_no_color_key(*spr, x + screen_offset_x_, y + screen_offset_y_, frame);
    }
}

void screen_base::draw_sprite_alpha_no_color_key(renderer& rend, sprite_manager& sprites,
                                                 uint16_t sprite_id, int32_t x, int32_t y,
                                                 uint32_t frame, float alpha) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite_alpha_no_color_key(*spr, x + screen_offset_x_, y + screen_offset_y_, frame, alpha);
    }
}

void screen_base::update_screen_offset(uint32_t window_width, uint32_t window_height)
{
    screen_offset_x_ = (static_cast<int32_t>(window_width) - design_width_) / 2;
    screen_offset_y_ = (static_cast<int32_t>(window_height) - design_height_) / 2;

    if (screen_offset_x_ < 0) screen_offset_x_ = 0;
    if (screen_offset_y_ < 0) screen_offset_y_ = 0;
}

std::pair<int32_t, int32_t> screen_base::get_adjusted_mouse() const
{
    return {mouse_x_ - screen_offset_x_, mouse_y_ - screen_offset_y_};
}

} // namespace hb
