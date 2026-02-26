#pragma once

#include "ui/ui_system.hpp"
#include "core/game_enums.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <optional>
#include <array>

namespace hb
{

class sprite_manager;

// Equipment dialog - displays equipped items on character silhouette
class equipment_dialog : public dialog
{
public:
    static constexpr int32_t slot_size = 40;
    static constexpr size_t max_equip_poss = 15; // full_body (13) + auxiliary + 1

    equipment_dialog();
    ~equipment_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set equipped item
    void set_equipped(equip_pos slot, const item* itm);
    void clear_slot(equip_pos slot);
    void clear_all();

    // Get equipped item
    const item* get_equipped(equip_pos slot) const;

    // Set sprite manager
    void set_sprite_manager(sprite_manager* mgr) { sprite_mgr_ = mgr; }

    // Mark a slot as being dragged (renders empty while item follows cursor)
    void set_dragging_slot(equip_pos slot) { dragging_slot_ = slot; }
    void clear_dragging_slot() { dragging_slot_ = std::nullopt; }

    // Callbacks
    using slot_callback = std::function<void(equip_pos)>;
    using drag_start_callback = std::function<void(equip_pos slot, int32_t cx, int32_t cy)>;

    void set_on_slot_click(slot_callback callback) { on_slot_click_ = std::move(callback); }
    void set_on_slot_right_click(slot_callback callback) { on_slot_right_click_ = std::move(callback); }
    void set_on_double_click_slot(slot_callback callback) { on_double_click_slot_ = std::move(callback); }
    void set_on_drag_start_slot(drag_start_callback callback) { on_drag_start_slot_ = std::move(callback); }

private:
    void render_slot(renderer& rend, equip_pos slot);
    ui_rect get_slot_rect(equip_pos slot) const;
    std::optional<equip_pos> slot_at(int32_t x, int32_t y) const;

    struct equipment_slot_data
    {
        const item* item_ptr = nullptr;
    };

    // Equipment slots - indexed by equip_pos enum
    std::array<equipment_slot_data, static_cast<size_t>(max_equip_poss)> slots_;

    sprite_manager* sprite_mgr_ = nullptr;
    std::optional<equip_pos> hovered_slot_;
    std::optional<equip_pos> dragging_slot_;

    // Double-click tracking
    static constexpr float double_click_threshold_ = 0.35f;
    float click_elapsed_ = 0.0f;
    std::optional<equip_pos> last_clicked_slot_;

    slot_callback on_slot_click_;
    slot_callback on_slot_right_click_;
    slot_callback on_double_click_slot_;
    drag_start_callback on_drag_start_slot_;

    // Slot positions relative to dialog (for character silhouette layout)
    struct slot_position
    {
        int32_t x;
        int32_t y;
        const char* label;
    };

    static const std::array<slot_position, static_cast<size_t>(max_equip_poss)> slot_positions_;
};

} // namespace hb
