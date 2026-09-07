#include "ui/dialog_base.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include "input/input.hpp"
#include "core/config.hpp"
#include <algorithm>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#include "debug/position_store.hpp"
#endif

namespace hb
{

#ifdef HB_DEBUG_OVERLAY_ENABLED
namespace
{

// Convert dialog_type to string for position ID
const char* dialog_type_to_string(dialog_type type)
{
    switch (type)
    {
    case dialog_type::login:
        return "dialog.login";
    case dialog_type::character_select:
        return "dialog.character_select";
    case dialog_type::character_create:
        return "dialog.character_create";
    case dialog_type::inventory:
        return "dialog.inventory";
    case dialog_type::equipment:
        return "dialog.equipment";
    case dialog_type::spellbook:
        return "dialog.spellbook";
    case dialog_type::skills:
        return "dialog.skills";
    case dialog_type::quest:
        return "dialog.quest";
    case dialog_type::party:
        return "dialog.party";
    case dialog_type::guild:
        return "dialog.guild";
    case dialog_type::guild_menu:
        return "dialog.guild_menu";
    case dialog_type::guild_bank:
        return "dialog.guild_bank";
    case dialog_type::chat:
        return "dialog.chat";
    case dialog_type::whisper:
        return "dialog.whisper";
    case dialog_type::system_menu:
        return "dialog.system_menu";
    case dialog_type::options:
        return "dialog.options";
    case dialog_type::key_bindings:
        return "dialog.key_bindings";
    case dialog_type::shop:
        return "dialog.shop";
    case dialog_type::shop_sell:
        return "dialog.shop_sell";
    case dialog_type::bank:
        return "dialog.bank";
    case dialog_type::warehouse:
        return "dialog.warehouse";
    case dialog_type::trade:
        return "dialog.trade";
    case dialog_type::exchange:
        return "dialog.exchange";
    case dialog_type::craft:
        return "dialog.craft";
    case dialog_type::manufacture:
        return "dialog.manufacture";
    case dialog_type::repair:
        return "dialog.repair";
    case dialog_type::map:
        return "dialog.map";
    case dialog_type::minimap:
        return "dialog.minimap";
    case dialog_type::help:
        return "dialog.help";
    case dialog_type::character_info:
        return "dialog.character_info";
    case dialog_type::monster_info:
        return "dialog.monster_info";
    case dialog_type::item_info:
        return "dialog.item_info";
    case dialog_type::npc_dialog:
        return "dialog.npc_dialog";
    case dialog_type::confirm:
        return "dialog.confirm";
    case dialog_type::input_box:
        return "dialog.input_box";
    case dialog_type::message_box:
        return "dialog.message_box";
    case dialog_type::connection:
        return "dialog.connection";
    case dialog_type::icon_panel:
        return "dialog.icon_panel";
    case dialog_type::gauge_panel:
        return "dialog.gauge_panel";
    case dialog_type::levelup:
        return "dialog.levelup";
    default:
        return "dialog.unknown";
    }
}

} // namespace
#endif

// dialog implementation

dialog::dialog(dialog_type type) : type_(type)
{
    // Dialogs start hidden - must be explicitly opened
    visible_ = false;
    // Parchment palette, in line with the legacy dialog art
    bg_color_ = sf::Color(54, 42, 30, 236);
    border_color_ = sf::Color(150, 118, 66);
}

void dialog::update(float delta_time, const input& inp)
{
    if (!visible_)
        return;
    ui_panel::update(delta_time, inp);
}

void dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    if (has_art())
    {
        render_art(rend);
        render_children(rend);
        return;
    }
    ui_panel::render(rend);
    render_title_bar(rend);
}

void dialog::set_art(std::string pak, uint32_t sprite, uint32_t frame)
{
    art_ = art_ref{.pak = std::move(pak), .sprite = sprite, .frame = frame};
    art_sized_ = false;
}

void dialog::add_art_overlay(std::string pak, uint32_t sprite, uint32_t frame, int32_t dx, int32_t dy)
{
    overlays_.push_back(art_ref{.pak = std::move(pak), .sprite = sprite, .frame = frame, .dx = dx, .dy = dy});
}

void dialog::render_art(renderer& rend)
{
    if (!s_sprites_ || !art_)
        return;
    if (auto* spr = s_sprites_->get_sprite(art_->pak, art_->sprite))
    {
        if (!art_sized_ && art_->frame < spr->frame_count())
        {
            const auto& f = spr->get_frame(art_->frame);
            bounds_.width = f.source_rect.size.x;
            bounds_.height = f.source_rect.size.y;
            art_sized_ = true;
        }
        rend.draw_sprite(*spr, bounds_.x, bounds_.y, art_->frame);
    }
    for (const auto& o : overlays_)
    {
        if (auto* spr = s_sprites_->get_sprite(o.pak, o.sprite))
            rend.draw_sprite(*spr, bounds_.x + o.dx, bounds_.y + o.dy, o.frame);
    }
    if (closeable_)
        rend.draw_text("x", bounds_.x + bounds_.width - 15, bounds_.y + 2, sf::Color(170, 70, 60), 13);
}

bool dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (draggable_ && btn == sf::Mouse::Button::Left)
    {
        // Check title bar close button
        ui_rect title_rect{bounds_.x, bounds_.y, bounds_.width, title_bar_height};
        if (title_rect.contains(x, y) && closeable_)
        {
            ui_rect close_rect{bounds_.x + bounds_.width - 20, bounds_.y + 4, 16, 16};
            if (close_rect.contains(x, y))
            {
                close();
                return true;
            }
        }

        // Start drag from title bar, or anywhere on the dialog body if drag_anywhere_ is set
        bool in_drag_area = title_rect.contains(x, y)
                            || (drag_anywhere_ && bounds_.contains(x, y));
        if (in_drag_area)
        {
            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    return ui_panel::handle_mouse_down(x, y, btn);
}

bool dialog::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (btn == sf::Mouse::Button::Left)
    {
        dragging_ = false;
    }
    return ui_panel::handle_mouse_up(x, y, btn);
}

bool dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (dragging_)
    {
        bounds_.x = x - drag_offset_x_;
        bounds_.y = y - drag_offset_y_;
        clamp_to_screen();
        return true;
    }

    return ui_panel::handle_mouse_move(x, y);
}

void dialog::clamp_to_screen()
{
    const auto& video = config::instance().video();
    auto sw = static_cast<int32_t>(video.screen_width);
    auto sh = static_cast<int32_t>(video.screen_height);

    if (drag_clamp_ == drag_clamp::partial)
    {
        // Title bar must stay reachable: keep min_visible_width on-screen horizontally,
        // and title bar visible vertically
        bounds_.x = std::clamp(bounds_.x, -bounds_.width + min_visible_width, sw - min_visible_width);
        bounds_.y = std::clamp(bounds_.y, 0, sh - title_bar_height);
    }
    else
    {
        // Entire dialog stays on screen
        bounds_.x = std::clamp(bounds_.x, 0, std::max(0, sw - bounds_.width));
        bounds_.y = std::clamp(bounds_.y, 0, std::max(0, sh - bounds_.height));
    }
}

void dialog::open()
{
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

void dialog::close()
{
#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Unregister from debug overlay
    debug::debug_overlay::instance().unregister_element(this);
#endif

    set_visible(false);
    dragging_ = false;
    if (on_close_)
    {
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

void dialog::render_title_bar(renderer& rend)
{
    // Title bar background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, title_bar_height, sf::Color(84, 62, 36), true);
    rend.draw_line(bounds_.x,
                   bounds_.y + title_bar_height,
                   bounds_.x + bounds_.width,
                   bounds_.y + title_bar_height,
                   sf::Color(170, 136, 78));

    // Title text
    rend.draw_text(title_, bounds_.x + 8, bounds_.y + 4, sf::Color(242, 228, 196));

    // Close button
    if (closeable_)
    {
        int32_t close_x = bounds_.x + bounds_.width - 20;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 16, 16, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White);
    }
}

} // namespace hb
