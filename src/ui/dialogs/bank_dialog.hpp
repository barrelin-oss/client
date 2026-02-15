#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <optional>
#include <array>

namespace hb
{

// Bank dialog - for storing items in bank/warehouse
class bank_dialog : public dialog
{
public:
    static constexpr int32_t grid_cols = 11;
    static constexpr int32_t grid_rows = 11;
    static constexpr int32_t slot_size = 30;
    static constexpr int32_t slot_padding = 2;
    static constexpr int32_t max_slots = 121;

    bank_dialog();
    ~bank_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set item in slot
    void set_item(int32_t slot, const item* itm);
    void clear_slot(int32_t slot);
    void clear_all();

    // Set gold in bank
    void set_stored_gold(uint32_t gold) { stored_gold_ = gold; }
    uint32_t stored_gold() const { return stored_gold_; }

    // Callbacks
    using slot_callback = std::function<void(int32_t slot)>;
    using drag_callback = std::function<void(int32_t from_slot, int32_t to_slot)>;
    using deposit_callback = std::function<void(int32_t inventory_slot, int32_t bank_slot)>;
    using withdraw_callback = std::function<void(int32_t bank_slot)>;
    using gold_callback = std::function<void(uint32_t amount, bool deposit)>;

    void set_on_slot_click(slot_callback callback) { on_slot_click_ = std::move(callback); }
    void set_on_slot_right_click(slot_callback callback) { on_slot_right_click_ = std::move(callback); }
    void set_on_item_drag(drag_callback callback) { on_item_drag_ = std::move(callback); }
    void set_on_deposit(deposit_callback callback) { on_deposit_ = std::move(callback); }
    void set_on_withdraw(withdraw_callback callback) { on_withdraw_ = std::move(callback); }
    void set_on_gold_transfer(gold_callback callback) { on_gold_transfer_ = std::move(callback); }

    // Get slot at screen position
    std::optional<int32_t> slot_at(int32_t x, int32_t y) const;

private:
    void render_slot(renderer& rend, int32_t slot, int32_t x, int32_t y);
    ui_rect get_slot_rect(int32_t slot) const;

    struct slot_data
    {
        const item* item_ptr = nullptr;
        bool occupied = false;
    };

    std::array<slot_data, max_slots> slots_;
    uint32_t stored_gold_ = 0;

    // Interaction state
    std::optional<int32_t> hovered_slot_;
    std::optional<int32_t> dragging_slot_;
    bool is_dragging_ = false;
    int32_t drag_start_x_ = 0;
    int32_t drag_start_y_ = 0;

    // Callbacks
    slot_callback on_slot_click_;
    slot_callback on_slot_right_click_;
    drag_callback on_item_drag_;
    deposit_callback on_deposit_;
    withdraw_callback on_withdraw_;
    gold_callback on_gold_transfer_;
};

} // namespace hb
