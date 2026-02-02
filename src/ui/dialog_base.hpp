#pragma once

#include "ui/ui_element.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/positionable.hpp"
#endif

namespace hb {

class renderer;
class input;

// Dialog type enumeration
enum class dialog_type {
    none,
    login,
    character_select,
    character_create,
    inventory,
    equipment,
    spellbook,
    skills,
    quest,
    party,
    guild,
    guild_menu,
    guild_bank,
    chat,
    whisper,
    system_menu,
    options,
    key_bindings,
    shop,
    shop_sell,
    bank,
    warehouse,
    trade,
    exchange,
    craft,
    manufacture,
    repair,
    map,
    minimap,
    help,
    character_info,
    monster_info,
    item_info,
    npc_dialog,
    confirm,
    input_box,
    message_box,
    connection,  // Classic Helbreath connection status dialog
    icon_panel,
    gauge_panel,
    levelup,
};

// Dialog base class
#ifdef HB_DEBUG_OVERLAY_ENABLED
class dialog : public ui_panel, public debug::positionable {
#else
class dialog : public ui_panel {
#endif
public:
    dialog(dialog_type type);
    ~dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    dialog_type type() const { return type_; }

    void open();
    void close();
    bool is_open() const { return visible(); }

    void set_title(std::string_view title) { title_ = title; }
    std::string_view title() const { return title_; }

    void set_draggable(bool draggable) { draggable_ = draggable; }
    bool draggable() const { return draggable_; }

    void set_closeable(bool closeable) { closeable_ = closeable; }
    bool closeable() const { return closeable_; }

    void set_modal(bool modal) { modal_ = modal; }
    bool modal() const { return modal_; }

    void set_always_on_top(bool always_on_top) { always_on_top_ = always_on_top; }
    bool always_on_top() const { return always_on_top_; }

    // Callbacks
    using close_callback = std::function<void()>;
    void set_on_close(close_callback callback) { on_close_ = std::move(callback); }

    // Set a custom position ID (for managed dialogs with YAML names)
    void set_position_id(std::string_view id) { position_id_ = id; }

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // positionable interface
    std::string position_id() const override;
    debug::bounds get_bounds() const override;
    void set_position(int32_t x, int32_t y) override;
#endif

protected:
    void render_title_bar(renderer& rend);

    dialog_type type_;
    std::string title_;
    std::string position_id_;  // Custom ID for debug overlay
    bool draggable_ = true;
    bool closeable_ = true;
    bool modal_ = false;
    bool always_on_top_ = false;
    bool dragging_ = false;
    int32_t drag_offset_x_ = 0;
    int32_t drag_offset_y_ = 0;

    close_callback on_close_;

    static constexpr int32_t title_bar_height = 24;
};

} // namespace hb
