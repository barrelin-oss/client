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
        rend.draw_sprite(*spr, x, y, frame);
    }
}

void screen_base::draw_sprite_alpha(renderer& rend, sprite_manager& sprites,
                                    uint16_t sprite_id, int32_t x, int32_t y,
                                    uint32_t frame, float alpha) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite_alpha(*spr, x, y, frame, alpha);
    }
}

void screen_base::draw_sprite_no_color_key(renderer& rend, sprite_manager& sprites,
                                           uint16_t sprite_id, int32_t x, int32_t y, uint32_t frame) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite_no_color_key(*spr, x, y, frame);
    }
}

void screen_base::draw_sprite_alpha_no_color_key(renderer& rend, sprite_manager& sprites,
                                                 uint16_t sprite_id, int32_t x, int32_t y,
                                                 uint32_t frame, float alpha) {
    const sprite* spr = sprites.get_sprite_by_id(sprite_id);
    if (spr) {
        rend.draw_sprite_alpha_no_color_key(*spr, x, y, frame, alpha);
    }
}

} // namespace hb
