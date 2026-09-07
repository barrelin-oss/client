#pragma once

#include "ui/ui_element.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>
#include <optional>
#include <vector>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/positionable.hpp"
#endif

namespace hb
{
class sprite_manager;

class renderer;
class input;

// Dialog type enumeration
enum class dialog_type
{
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
    connection, // Classic Helbreath connection status dialog
    icon_panel,
    gauge_panel,
    levelup,
    fishing,
    death,
    level_up_settings,
};

// Controls how a dialog is clamped to the screen during dragging
enum class drag_clamp : uint8_t
{
    on_screen, // Entire dialog must stay within screen bounds (default)
    partial    // Dialog can be dragged partially off-screen; title bar stays reachable
};

// Dialog base class
#ifdef HB_DEBUG_OVERLAY_ENABLED
class dialog
    : public ui_panel
    , public debug::positionable
{
#else
class dialog : public ui_panel
{
#endif
public:
    dialog(dialog_type type);
    ~dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    dialog_type type() const { return type_; }

    virtual void open();
    void close();
    bool is_open() const { return visible(); }

    void set_title(std::string_view title) { title_ = title; }
    std::string_view title() const { return title_; }

    void set_draggable(bool draggable) { draggable_ = draggable; }
    bool draggable() const { return draggable_; }

    void set_drag_anywhere(bool anywhere) { drag_anywhere_ = anywhere; }
    bool drag_anywhere() const { return drag_anywhere_; }

    void set_closeable(bool closeable) { closeable_ = closeable; }
    bool closeable() const { return closeable_; }

    void set_modal(bool modal) { modal_ = modal; }
    bool modal() const { return modal_; }

    void set_always_on_top(bool always_on_top) { always_on_top_ = always_on_top; }
    bool always_on_top() const { return always_on_top_; }

    void set_right_click_closeable(bool v) { right_click_closeable_ = v; }
    bool right_click_closeable() const { return right_click_closeable_; }

    void set_drag_clamp(drag_clamp mode) { drag_clamp_ = mode; }
    drag_clamp get_drag_clamp() const { return drag_clamp_; }
    bool is_window_dragging() const { return dragging_; }

    // Callbacks
    using close_callback = std::function<void()>;
    void set_on_close(close_callback callback) { on_close_ = std::move(callback); }

    // Set a custom position ID (for managed dialogs with YAML names)
    void set_position_id(std::string_view id) { position_id_ = id; }

    // Legacy dialog art: a background frame from a pak (the dialog takes the frame's size) and
    // optional overlays (the title banners of DialogText) drawn at an offset from the origin.
    void set_art(std::string pak, uint32_t sprite, uint32_t frame);
    void add_art_overlay(std::string pak, uint32_t sprite, uint32_t frame, int32_t dx = 0, int32_t dy = 0);
    bool has_art() const { return art_.has_value(); }
    static void set_sprite_manager(sprite_manager* sprites) { s_sprites_ = sprites; }

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // positionable interface
    std::string position_id() const override;
    debug::bounds get_bounds() const override;
    void set_position(int32_t x, int32_t y) override;
#endif

protected:
    void render_title_bar(renderer& rend);
    void render_art(renderer& rend);

    struct art_ref
    {
        std::string pak;
        uint32_t sprite{0};
        uint32_t frame{0};
        int32_t dx{0};
        int32_t dy{0};
    };
    std::optional<art_ref> art_;
    std::vector<art_ref> overlays_;
    bool art_sized_ = false;
    static inline sprite_manager* s_sprites_ = nullptr;
    void clamp_to_screen();

    dialog_type type_;
    std::string title_;
    std::string position_id_; // Custom ID for debug overlay
    bool draggable_ = true;
    bool drag_anywhere_ = false;
    bool closeable_ = true;
    bool modal_ = false;
    bool always_on_top_ = false;
    bool right_click_closeable_ = true;
    bool dragging_ = false;
    drag_clamp drag_clamp_ = drag_clamp::on_screen;
    int32_t drag_offset_x_ = 0;
    int32_t drag_offset_y_ = 0;

    close_callback on_close_;

    static constexpr int32_t title_bar_height = 24;
    // Minimum visible pixels when using partial drag clamp
    static constexpr int32_t min_visible_width = 60;
};

} // namespace hb
