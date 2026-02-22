#include "ui/dialogs/level_up_settings_dialog.hpp"
#include "graphics/renderer.hpp"

namespace hb
{

level_up_settings_dialog::level_up_settings_dialog() : dialog(dialog_type::level_up_settings)
{
    set_title("Level Settings");
    set_bounds({100, 100, 260, 320});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void level_up_settings_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    rend.draw_text("Level-up settings", bounds_.x + 20, bounds_.y + 50, sf::Color(180, 180, 180));
    rend.draw_text("coming soon.", bounds_.x + 20, bounds_.y + 70, sf::Color(180, 180, 180));
}

} // namespace hb
