#pragma once

#include "ui/ui_system.hpp"
#include "core/game_enums.hpp"
#include <functional>
#include <optional>
#include <string>

namespace hb
{

struct stats_component;
struct name_component;
struct combat_component;
struct sprite_component;
class inventory_system;
class sprite_manager;
class entity;
class paperdoll_renderer;

// Character information dialog — comprehensive character sheet
// Shows name/faction, paperdoll with interactive equipment slots, stats, and attributes
class character_dialog : public dialog
{
public:

    character_dialog();
    ~character_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Data source setters — dialog reads live each frame
    void set_player(const entity* player) { player_ = player; }
    void set_inventory(inventory_system* inv) { inventory_ = inv; }
    void set_sprite_manager(sprite_manager* mgr) { sprite_mgr_ = mgr; }
    void set_paperdoll_renderer(paperdoll_renderer* rend) { paperdoll_ = rend; }

    // Stat point allocation
    void set_stat_points(int32_t points) { stat_points_ = points; }

    // Mark a slot as being dragged (renders empty while item follows cursor)
    void set_dragging_slot(equip_pos slot) { dragging_slot_ = slot; }
    void clear_dragging_slot() { dragging_slot_ = std::nullopt; }

    // Callbacks
    using stat_callback = std::function<void(int)>;
    using action_callback = std::function<void()>;
    using slot_callback = std::function<void(equip_pos)>;
    using drag_start_callback = std::function<void(equip_pos slot, int32_t cx, int32_t cy)>;

    void set_on_add_stat(stat_callback cb) { on_add_stat_ = std::move(cb); }
    void set_on_quest(action_callback cb) { on_quest_ = std::move(cb); }
    void set_on_party(action_callback cb) { on_party_ = std::move(cb); }
    void set_on_level_settings(action_callback cb) { on_level_settings_ = std::move(cb); }
    void set_on_double_click_slot(slot_callback cb) { on_double_click_slot_ = std::move(cb); }
    void set_on_drag_start_slot(drag_start_callback cb) { on_drag_start_slot_ = std::move(cb); }

private:
    // Rendering sections
    void render_name_section(renderer& rend, int32_t& y);
    void render_equipment_and_stats(renderer& rend, int32_t& y);
    void render_paperdoll(renderer& rend, int32_t x, int32_t y);
    void render_attributes(renderer& rend, int32_t& y);
    void render_buttons(renderer& rend, int32_t y);

    // Per-pixel equipment hit-testing against paperdoll sprites
    std::optional<equip_pos> slot_at_paperdoll(int32_t x, int32_t y) const;

    // Hit testing for buttons
    void render_on_art(renderer& rend);
    std::optional<int> button_at(int32_t x, int32_t y) const;
    std::optional<int> stat_button_at(int32_t x, int32_t y) const;

    // Computed layout positions (set during render, used for hit-testing)
    int32_t buttons_y_ = 0;
    int32_t attr_y_ = 0;
    int32_t paperdoll_anchor_x_ = 0; // Paperdoll draw anchor, set during render
    int32_t paperdoll_anchor_y_ = 0;

    // Data sources (non-owning, resolved lazily)
    const entity* player_ = nullptr;
    inventory_system* inventory_ = nullptr;
    sprite_manager* sprite_mgr_ = nullptr;
    paperdoll_renderer* paperdoll_ = nullptr;

    int32_t stat_points_ = 0;
    int32_t hovered_button_ = -1;
    std::optional<equip_pos> hovered_equip_slot_;
    std::optional<equip_pos> dragging_slot_;

    // Double-click tracking
    static constexpr float double_click_threshold_ = 0.35f;
    float click_elapsed_ = 0.0f;
    std::optional<equip_pos> last_clicked_slot_;

    // Callbacks
    stat_callback on_add_stat_;
    action_callback on_quest_;
    action_callback on_party_;
    action_callback on_level_settings_;
    slot_callback on_double_click_slot_;
    drag_start_callback on_drag_start_slot_;

    // Layout constants
    static constexpr int32_t dialog_width = 330;
    static constexpr int32_t dialog_height = 420;
    static constexpr int32_t padding = 10;
    static constexpr int32_t paperdoll_area_w = 150;
    static constexpr int32_t paperdoll_area_h = 170;
    static constexpr int32_t stat_button_size = 16;
    static constexpr int32_t button_width = 88;
    static constexpr int32_t button_height = 24;
};

} // namespace hb
