#pragma once

#include "ui/dialog_base.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <optional>

namespace hb
{

class sprite_manager;
class inventory_system;

// Inventory dialog - displays player items with real sprites in free-form layout
// Items render at pixel positions within the bag area, with array index as z-order.
// Dragging moves the original item's position (legacy behavior) — no separate copy.
class inventory_dialog : public dialog
{
public:
    static constexpr int32_t grid_cols = 10;
    static constexpr int32_t grid_rows = 5;
    static constexpr int32_t cell_size = 34;
    static constexpr int32_t max_slots = 50;
    static constexpr int32_t chrome_top = 32;
    static constexpr int32_t chrome_bottom = 24;
    static constexpr int32_t chrome_sides = 8;
    static constexpr int32_t min_width = grid_cols * cell_size + chrome_sides * 2;
    static constexpr int32_t min_height = grid_rows * cell_size + chrome_top + chrome_bottom;
    static constexpr int32_t resize_handle_size = 12;

    inventory_dialog();
    ~inventory_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Data binding
    void set_inventory(inventory_system* inv) { inventory_ = inv; }
    void set_sprite_manager(sprite_manager* mgr) { sprite_mgr_ = mgr; }

    // Drag start callback (notifies ui_system to begin cross-dialog drag tracking)
    // offset_x/y = click position relative to item's draw position
    using drag_start_callback = std::function<void(int32_t slot, int32_t offset_x, int32_t offset_y)>;
    void set_on_drag_start(drag_start_callback cb) { on_drag_start_ = std::move(cb); }

    // Called by ui_system when drag ends
    void clear_dragging_slot() { dragging_slot_ = -1; }
    bool is_dragging() const { return dragging_slot_ >= 0; }

    // Restore item to its position before the current drag started
    void restore_drag_position();

    // Bag area rectangle (screen coords)
    ui_rect bag_area() const;

private:
    void render_item(renderer& rend, int32_t slot, int32_t x, int32_t y);
    std::optional<int32_t> item_at(int32_t screen_x, int32_t screen_y) const;

    // Get the screen-space bounding rect of an item's sprite (accounts for pivot offset).
    // Returns a cell_size fallback if sprite is unavailable.
    ui_rect item_sprite_rect(int32_t slot) const;

    inventory_system* inventory_ = nullptr;
    sprite_manager* sprite_mgr_ = nullptr;

    // Resize
    bool resizing_ = false;
    int32_t resize_start_w_ = 0;
    int32_t resize_start_h_ = 0;
    int32_t resize_start_mx_ = 0;
    int32_t resize_start_my_ = 0;

    // Item drag (moves original item position — legacy behavior)
    int32_t dragging_slot_ = -1;
    int32_t item_drag_off_x_ = 0; // Click offset relative to item draw position
    int32_t item_drag_off_y_ = 0;
    int16_t pre_drag_x_ = 0; // Position before drag started (for restore on drop-to-world)
    int16_t pre_drag_y_ = 0;

    // Click detection
    std::optional<int32_t> pressed_slot_;

    drag_start_callback on_drag_start_;
};

} // namespace hb
