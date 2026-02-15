#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <optional>
#include <array>

namespace hb
{

class sprite_manager;

// Inventory dialog - displays player items in a grid
class inventory_dialog : public dialog
{
public:
    static constexpr int32_t grid_cols = 10;
    static constexpr int32_t grid_rows = 5;
    static constexpr int32_t slot_size = 34;
    static constexpr int32_t slot_padding = 2;
    static constexpr int32_t max_slots = 50;

    inventory_dialog();
    ~inventory_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set item in slot
    void set_item(int32_t slot, const item* itm);
    void clear_slot(int32_t slot);
    void clear_all();

    // Set gold and weight
    void set_gold(uint32_t gold) { gold_ = gold; }
    void set_weight(int32_t current, int32_t max)
    {
        weight_ = current;
        max_weight_ = max;
    }

    // Set sprite manager for item rendering
    void set_sprite_manager(sprite_manager* mgr) { sprite_mgr_ = mgr; }

    // Callbacks
    using item_callback = std::function<void(int32_t slot)>;
    using item_drag_callback = std::function<void(int32_t from_slot, int32_t to_slot)>;

    void set_on_item_click(item_callback callback) { on_item_click_ = std::move(callback); }
    void set_on_item_right_click(item_callback callback) { on_item_right_click_ = std::move(callback); }
    void set_on_item_double_click(item_callback callback) { on_item_double_click_ = std::move(callback); }
    void set_on_item_drag(item_drag_callback callback) { on_item_drag_ = std::move(callback); }

    // Get slot at screen position
    std::optional<int32_t> slot_at(int32_t x, int32_t y) const;

    // Get hovered slot
    std::optional<int32_t> hovered_slot() const { return hovered_slot_; }

private:
    void render_slot(renderer& rend, int32_t slot, int32_t x, int32_t y);
    ui_rect get_slot_rect(int32_t slot) const;

    struct slot_data
    {
        const item* item_ptr = nullptr;
        bool occupied = false;
    };

    std::array<slot_data, max_slots> slots_;
    uint32_t gold_ = 0;
    int32_t weight_ = 0;
    int32_t max_weight_ = 100;

    sprite_manager* sprite_mgr_ = nullptr;

    // Interaction state
    std::optional<int32_t> hovered_slot_;
    std::optional<int32_t> dragging_slot_;
    int32_t drag_start_x_ = 0;
    int32_t drag_start_y_ = 0;
    bool is_dragging_ = false;

    // Double-click detection
    int32_t last_click_slot_ = -1;
    float last_click_time_ = 0.0f;
    static constexpr float double_click_time = 0.3f;

    // Callbacks
    item_callback on_item_click_;
    item_callback on_item_right_click_;
    item_callback on_item_double_click_;
    item_drag_callback on_item_drag_;

    // Timer for double-click
    float click_timer_ = 0.0f;
};

} // namespace hb
