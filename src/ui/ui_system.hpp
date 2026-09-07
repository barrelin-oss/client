#pragma once

#include "ui/ui_element.hpp"
#include "ui/dialog_base.hpp"
#include "core/game_enums.hpp"
#include "gameplay/item.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <functional>
#include <filesystem>

namespace hb
{

class renderer;
class input;
class sprite_manager;
class dialog_manager;
class managed_dialog;
class yaml_icon_panel_dialog;
class icon_panel_interface;
enum class render_mode;

// UI visual style
enum class ui_style
{
    modern, // Programmatic rendering with modern look
    classic // Sprite-based rendering matching original Helbreath
};

// Cross-dialog item drag state
struct ui_drag_state
{
    bool active = false;
    std::optional<item> held_item;
    uint32_t source_item_id = 0;
    equip_pos source_equip = equip_pos::none;
    dialog_type source_dialog = dialog_type::none;
    int32_t cursor_x = 0;
    int32_t cursor_y = 0;
    int32_t offset_x = 0; // Click-to-item-origin offset (drag from click point)
    int32_t offset_y = 0;
};

// UI system manager
class ui_system
{
public:
    ui_system();
    ~ui_system();

    ui_system(const ui_system&) = delete;
    ui_system& operator=(const ui_system&) = delete;

    // Initialize/shutdown
    void initialize();
    void shutdown();

    // UI style management
    void set_style(ui_style style) { style_ = style; }
    ui_style style() const { return style_; }

    // Sprite manager for classic UI rendering
    void set_sprite_manager(sprite_manager* sprites)
    {
        sprites_ = sprites;
        dialog::set_sprite_manager(sprites);
    }
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
    icon_panel_interface* get_icon_panel();
    void add_dialog(dialog_type type, std::unique_ptr<dialog> dlg);
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
    void create_shop_sell_dialog();
    void create_party_dialog();
    void create_guild_dialog();
    void create_npc_dialog();
    void create_options_dialog();
    void create_trade_dialog();
    void create_craft_dialog();
    void create_map_dialog();
    void create_repair_dialog();
    void create_help_dialog();
    void create_message_box(std::string_view title, std::string_view message, std::function<void()> on_ok = nullptr);
    void create_confirm_box(std::string_view title, std::string_view message, std::function<void(bool)> on_result);
    void
    create_input_box(std::string_view title, std::string_view prompt, std::function<void(std::string_view)> on_submit);

    // Drop confirmation dialog — shows item info, yes/no buttons, and a "don't ask again" toggle.
    // Callback receives (confirmed, skip_next_for_this_type).
    void create_drop_confirm_box(const item& itm,
                                 std::function<void(bool confirmed, bool skip_next)> on_result);

    // Connection/waiting dialog
    // Shows "Waiting for response from the server" with animated dots
    // After 7 seconds, shows "press escape to cancel" hint
    void show_connection_dialog(std::function<void()> on_cancel = nullptr);
    void show_error_dialog(std::string_view message, std::function<void()> on_cancel = nullptr);
    void hide_connection_dialog();

    // Phase 3 dialogs
    void create_character_create_dialog();
    void create_icon_panel_dialog();
    void create_gauge_panel_dialog();
    void create_levelup_dialog();
    void create_fishing_dialog();
    void create_quest_dialog();
    void create_level_up_settings_dialog();

    // Tooltip
    void show_tooltip(std::string_view text, int32_t x, int32_t y);
    void hide_tooltip();

    // Focus
    void set_focus(ui_element* element);
    ui_element* focused_element() const { return focused_; }

    // Debug stats
    size_t open_dialog_count() const
    {
        size_t count = 0;
        for (const auto* d : dialog_order_)
        {
            if (d->is_open())
                ++count;
        }
        return count;
    }

    // Check if any dialog is blocking input
    bool is_modal_open() const;

    // Check if any dialog is consuming text input (e.g. search field active)
    bool has_text_focus() const;

    // Check if a screen point is over any open dialog
    bool is_point_over_dialog(int32_t x, int32_t y) const;
    bool is_any_dialog_dragging() const;

    // Item drag system
    const ui_drag_state& drag_state() const { return drag_state_; }
    void begin_item_drag(const item& itm, uint32_t item_id, equip_pos equip, dialog_type source,
                         int32_t cursor_x, int32_t cursor_y, int32_t offset_x = 0, int32_t offset_y = 0);
    void update_drag_position(int32_t x, int32_t y);
    void end_item_drag(int32_t x, int32_t y, bool shift_held);
    void cancel_item_drag();
    bool is_dragging_item() const { return drag_state_.active; }

    using drop_in_world_callback = std::function<void(uint32_t item_id)>;
    using equip_from_drag_callback = std::function<void(uint32_t item_id)>;
    using unequip_from_drag_callback = std::function<void(equip_pos slot, int32_t bag_x, int32_t bag_y)>;
    using reposition_callback = std::function<void(uint32_t item_id, int32_t x, int32_t y, bool shift_held)>;
    void set_on_drop_in_world(drop_in_world_callback cb) { on_drop_in_world_ = std::move(cb); }
    void set_on_equip_from_drag(equip_from_drag_callback cb) { on_equip_from_drag_ = std::move(cb); }
    void set_on_unequip_from_drag(unequip_from_drag_callback cb) { on_unequip_from_drag_ = std::move(cb); }
    void set_on_reposition_item(reposition_callback cb) { on_reposition_item_ = std::move(cb); }
    // Item dragged from the inventory and dropped on the open bank dialog
    using deposit_to_bank_callback = std::function<void(uint32_t item_id)>;
    void set_on_deposit_to_bank(deposit_to_bank_callback cb) { on_deposit_to_bank_ = std::move(cb); }

    // Track whether a mouse button press was consumed by the UI.
    // Returns true from press until release, preventing held-button
    // game world actions after closing a dialog with a click.
    bool is_mouse_consumed(sf::Mouse::Button btn) const;
    void update_mouse_consumed(const input& inp);
    void clear_mouse_consumed()
    {
        mouse_consumed_left_ = false;
        mouse_consumed_right_ = false;
    }

    // === Data-driven dialog manager ===
    // Provides access to the new YAML/JSON-based dialog system

    dialog_manager& dialogs();
    const dialog_manager& dialogs() const;

    // Load dialog definitions from YAML/JSON files
    void load_dialog_definitions(const std::filesystem::path& path);
    void load_dialog_definitions_from_directory(const std::filesystem::path& dir);

private:
    void bring_to_front(dialog* dlg);
    void clear_focus_if_owned_by(ui_element* root);
    void render_held_item_info(renderer& rend, const item& itm, int32_t x, int32_t y);

    std::unordered_map<dialog_type, std::unique_ptr<dialog>> dialogs_;
    std::vector<dialog*> dialog_order_; // For z-ordering
    ui_element* focused_ = nullptr;

    // UI style (default to classic for authentic Helbreath experience)
    ui_style style_ = ui_style::classic;
    sprite_manager* sprites_ = nullptr;

    // Set during update() when a dialog consumed text input
    bool text_input_active_ = false;

    // Tooltip
    std::string tooltip_text_;
    int32_t tooltip_x_ = 0;
    int32_t tooltip_y_ = 0;
    bool tooltip_visible_ = false;

    // Data-driven dialog system (owned via unique_ptr to allow forward declaration)
    std::unique_ptr<dialog_manager> dialog_manager_;

    // YAML-based icon panel (managed by dialog_manager, we just hold a pointer)
    yaml_icon_panel_dialog* yaml_icon_panel_ = nullptr;

    // Track which mouse buttons had their press consumed by the UI
    bool mouse_consumed_left_ = false;
    bool mouse_consumed_right_ = false;

    // Item drag system
    ui_drag_state drag_state_;
    drop_in_world_callback on_drop_in_world_;
    equip_from_drag_callback on_equip_from_drag_;
    deposit_to_bank_callback on_deposit_to_bank_;
    unequip_from_drag_callback on_unequip_from_drag_;
    reposition_callback on_reposition_item_;
};

} // namespace hb
