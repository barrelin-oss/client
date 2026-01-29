#pragma once

#include "ui/ui_element.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include <functional>

namespace hb {

class renderer;
class input;
class sprite_manager;

// UI visual style
enum class ui_style {
    modern,     // Programmatic rendering with modern look
    classic     // Sprite-based rendering matching original Helbreath
};

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
    icon_panel,
    gauge_panel,
    levelup,
};

// Dialog base class
class dialog : public ui_panel {
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

    // Callbacks
    using close_callback = std::function<void()>;
    void set_on_close(close_callback callback) { on_close_ = std::move(callback); }

protected:
    void render_title_bar(renderer& rend);

    dialog_type type_;
    std::string title_;
    bool draggable_ = true;
    bool closeable_ = true;
    bool modal_ = false;
    bool dragging_ = false;
    int32_t drag_offset_x_ = 0;
    int32_t drag_offset_y_ = 0;

    close_callback on_close_;

    static constexpr int32_t title_bar_height = 24;
};

// UI system manager
class ui_system {
public:
    ui_system() = default;
    ~ui_system() = default;

    ui_system(const ui_system&) = delete;
    ui_system& operator=(const ui_system&) = delete;

    // Initialize/shutdown
    void initialize();
    void shutdown();

    // UI style management
    void set_style(ui_style style) { style_ = style; }
    ui_style style() const { return style_; }

    // Sprite manager for classic UI rendering
    void set_sprite_manager(sprite_manager* sprites) { sprites_ = sprites; }
    sprite_manager* sprites() const { return sprites_; }

    // Update and render
    void update(float delta_time, const input& inp);
    void render(renderer& rend);

    // Input handling (returns true if UI consumed input)
    bool handle_mouse_move(int32_t x, int32_t y);
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn);
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn);
    bool handle_key_press(sf::Keyboard::Key key);
    bool handle_text_input(char32_t unicode);

    // Dialog management
    dialog* get_dialog(dialog_type type);
    void open_dialog(dialog_type type);
    void close_dialog(dialog_type type);
    void toggle_dialog(dialog_type type);
    bool is_dialog_open(dialog_type type) const;
    void close_all_dialogs();

    // Create standard dialogs
    void create_login_dialog();
    void create_character_select_dialog();
    void create_character_dialog();
    void create_inventory_dialog();
    void create_equipment_dialog();
    void create_spellbook_dialog();
    void create_skills_dialog();
    void create_chat_dialog();
    void create_shop_dialog();
    void create_bank_dialog();
    void create_party_dialog();
    void create_guild_dialog();
    void create_npc_dialog();
    void create_system_menu_dialog();
    void create_options_dialog();
    void create_trade_dialog();
    void create_craft_dialog();
    void create_map_dialog();
    void create_repair_dialog();
    void create_help_dialog();
    void create_message_box(std::string_view title, std::string_view message,
                            std::function<void()> on_ok = nullptr);
    void create_confirm_box(std::string_view title, std::string_view message,
                            std::function<void(bool)> on_result);
    void create_input_box(std::string_view title, std::string_view prompt,
                          std::function<void(std::string_view)> on_submit);

    // Phase 3 dialogs
    void create_character_create_dialog();
    void create_icon_panel_dialog();
    void create_gauge_panel_dialog();
    void create_levelup_dialog();

    // Tooltip
    void show_tooltip(std::string_view text, int32_t x, int32_t y);
    void hide_tooltip();

    // Focus
    void set_focus(ui_element* element);
    ui_element* focused_element() const { return focused_; }

    // Check if any dialog is blocking input
    bool is_modal_open() const;

private:
    void bring_to_front(dialog* dlg);

    std::unordered_map<dialog_type, std::unique_ptr<dialog>> dialogs_;
    std::vector<dialog*> dialog_order_;  // For z-ordering
    ui_element* focused_ = nullptr;

    // UI style (default to classic for authentic Helbreath experience)
    ui_style style_ = ui_style::classic;
    sprite_manager* sprites_ = nullptr;

    // Tooltip
    std::string tooltip_text_;
    int32_t tooltip_x_ = 0;
    int32_t tooltip_y_ = 0;
    bool tooltip_visible_ = false;
};

} // namespace hb
