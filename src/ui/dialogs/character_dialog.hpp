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
// Shows name/faction, paperdoll with equipment, stats, attributes, and action buttons
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

    // Callbacks
    using stat_callback = std::function<void(int)>;
    using action_callback = std::function<void()>;

    void set_on_add_stat(stat_callback cb) { on_add_stat_ = std::move(cb); }
    void set_on_quest(action_callback cb) { on_quest_ = std::move(cb); }
    void set_on_party(action_callback cb) { on_party_ = std::move(cb); }
    void set_on_level_settings(action_callback cb) { on_level_settings_ = std::move(cb); }

private:
    // Rendering sections
    void render_name_section(renderer& rend, int32_t& y);
    void render_equipment_and_stats(renderer& rend, int32_t& y);
    void render_paperdoll(renderer& rend, int32_t x, int32_t y);
    void render_attributes(renderer& rend, int32_t& y);
    void render_buttons(renderer& rend, int32_t y);

    // Hit testing
    std::optional<int> button_at(int32_t x, int32_t y) const;
    std::optional<int> stat_button_at(int32_t x, int32_t y) const;

    // Computed layout: y position where buttons start (set during render)
    int32_t buttons_y_ = 0;
    int32_t attr_y_ = 0;

    // Data sources (non-owning, resolved lazily)
    const entity* player_ = nullptr;
    inventory_system* inventory_ = nullptr;
    sprite_manager* sprite_mgr_ = nullptr;
    paperdoll_renderer* paperdoll_ = nullptr;

    int32_t stat_points_ = 0;
    int32_t hovered_button_ = -1;

    // Callbacks
    stat_callback on_add_stat_;
    action_callback on_quest_;
    action_callback on_party_;
    action_callback on_level_settings_;

    // Layout constants
    static constexpr int32_t dialog_width = 330;
    static constexpr int32_t dialog_height = 420;
    static constexpr int32_t padding = 10;
    static constexpr int32_t stat_button_size = 16;
    static constexpr int32_t button_width = 88;
    static constexpr int32_t button_height = 24;
};

} // namespace hb
