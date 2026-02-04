#include "ui/dialog_base.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#include "debug/position_store.hpp"
#endif

namespace hb {

#ifdef HB_DEBUG_OVERLAY_ENABLED
namespace {

// Convert dialog_type to string for position ID
const char* dialog_type_to_string(dialog_type type)
{
    switch (type)
    {
        case dialog_type::login: return "dialog.login";
        case dialog_type::character_select: return "dialog.character_select";
        case dialog_type::character_create: return "dialog.character_create";
        case dialog_type::inventory: return "dialog.inventory";
        case dialog_type::equipment: return "dialog.equipment";
        case dialog_type::spellbook: return "dialog.spellbook";
        case dialog_type::skills: return "dialog.skills";
        case dialog_type::quest: return "dialog.quest";
        case dialog_type::party: return "dialog.party";
        case dialog_type::guild: return "dialog.guild";
        case dialog_type::guild_menu: return "dialog.guild_menu";
        case dialog_type::guild_bank: return "dialog.guild_bank";
        case dialog_type::chat: return "dialog.chat";
        case dialog_type::whisper: return "dialog.whisper";
        case dialog_type::system_menu: return "dialog.system_menu";
        case dialog_type::options: return "dialog.options";
        case dialog_type::key_bindings: return "dialog.key_bindings";
        case dialog_type::shop: return "dialog.shop";
        case dialog_type::shop_sell: return "dialog.shop_sell";
        case dialog_type::bank: return "dialog.bank";
        case dialog_type::warehouse: return "dialog.warehouse";
        case dialog_type::trade: return "dialog.trade";
        case dialog_type::exchange: return "dialog.exchange";
        case dialog_type::craft: return "dialog.craft";
        case dialog_type::manufacture: return "dialog.manufacture";
        case dialog_type::repair: return "dialog.repair";
        case dialog_type::map: return "dialog.map";
        case dialog_type::minimap: return "dialog.minimap";
        case dialog_type::help: return "dialog.help";
        case dialog_type::character_info: return "dialog.character_info";
        case dialog_type::monster_info: return "dialog.monster_info";
        case dialog_type::item_info: return "dialog.item_info";
        case dialog_type::npc_dialog: return "dialog.npc_dialog";
        case dialog_type::confirm: return "dialog.confirm";
        case dialog_type::input_box: return "dialog.input_box";
        case dialog_type::message_box: return "dialog.message_box";
        case dialog_type::connection: return "dialog.connection";
        case dialog_type::icon_panel: return "dialog.icon_panel";
        case dialog_type::gauge_panel: return "dialog.gauge_panel";
        case dialog_type::levelup: return "dialog.levelup";
        default: return "dialog.unknown";
    }
}

}  // namespace
#endif

// dialog implementation

dialog::dialog(dialog_type type)
    : type_(type) {
    // Dialogs start hidden - must be explicitly opened
    visible_ = false;
}

void dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;
    ui_panel::update(delta_time, inp);
}

void dialog::render(renderer& rend) {
    if (!visible_) return;

    ui_panel::render(rend);
    render_title_bar(rend);
}

bool dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check title bar for dragging
    if (draggable_ && btn == sf::Mouse::Button::Left) {
        ui_rect title_rect{bounds_.x, bounds_.y, bounds_.width, title_bar_height};
        if (title_rect.contains(x, y)) {
            // Check close button
            if (closeable_) {
                ui_rect close_rect{bounds_.x + bounds_.width - 20, bounds_.y + 4, 16, 16};
                if (close_rect.contains(x, y)) {
                    close();
                    return true;
                }
            }

            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    return ui_panel::handle_mouse_down(x, y, btn);
}

bool dialog::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (btn == sf::Mouse::Button::Left) {
        dragging_ = false;
    }
    return ui_panel::handle_mouse_up(x, y, btn);
}

bool dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (dragging_) {
        bounds_.x = x - drag_offset_x_;
        bounds_.y = y - drag_offset_y_;

        // Clamp to screen
        bounds_.x = std::clamp(bounds_.x, 0, static_cast<int32_t>(screen_width) - bounds_.width);
        bounds_.y = std::clamp(bounds_.y, 0, static_cast<int32_t>(screen_height) - bounds_.height);

        return true;
    }

    return ui_panel::handle_mouse_move(x, y);
}

void dialog::open() {
    set_visible(true);

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Register with debug overlay and apply stored position
    debug::debug_overlay::instance().register_element(this);
    if (auto pos = debug::position_store::instance().get(position_id()))
    {
        bounds_.x = pos->x;
        bounds_.y = pos->y;
    }
#endif
}

void dialog::close() {
#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Unregister from debug overlay
    debug::debug_overlay::instance().unregister_element(this);
#endif

    set_visible(false);
    dragging_ = false;
    if (on_close_) {
        on_close_();
    }
}

#ifdef HB_DEBUG_OVERLAY_ENABLED
std::string dialog::position_id() const
{
    // Use custom ID if set (for YAML dialogs), otherwise use dialog_type
    if (!position_id_.empty())
    {
        return position_id_;
    }
    return dialog_type_to_string(type_);
}

debug::bounds dialog::get_bounds() const
{
    return {bounds_.x, bounds_.y, bounds_.width, bounds_.height};
}

void dialog::set_position(int32_t x, int32_t y)
{
    bounds_.x = x;
    bounds_.y = y;
}
#endif

void dialog::render_title_bar(renderer& rend) {
    // Title bar background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, title_bar_height,
                   sf::Color(60, 60, 80), true);
    rend.draw_line(bounds_.x, bounds_.y + title_bar_height,
                   bounds_.x + bounds_.width, bounds_.y + title_bar_height,
                   sf::Color(100, 100, 140));

    // Title text
    rend.draw_text(title_, bounds_.x + 8, bounds_.y + 4, sf::Color::White);

    // Close button
    if (closeable_) {
        int32_t close_x = bounds_.x + bounds_.width - 20;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 16, 16, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White);
    }
}

} // namespace hb
