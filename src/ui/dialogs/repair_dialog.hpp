#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <vector>
#include <string>
#include <optional>

namespace hb {

// Repair item info
struct repair_item_info {
    int32_t inventory_slot = -1;
    const item* itm = nullptr;
    int32_t current_durability = 0;
    int32_t max_durability = 0;
    uint32_t repair_cost = 0;
};

// Repair dialog - for repairing items at NPC smiths
class repair_dialog : public dialog {
public:
    repair_dialog();
    ~repair_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // NPC info
    void set_npc_name(std::string_view name) { npc_name_ = name; }

    // Items that can be repaired
    void set_repair_items(std::vector<repair_item_info> items) { repair_items_ = std::move(items); }
    void add_repair_item(repair_item_info item) { repair_items_.push_back(std::move(item)); }
    void clear_repair_items() { repair_items_.clear(); selected_item_ = -1; }

    // Player gold
    void set_player_gold(uint32_t gold) { player_gold_ = gold; }

    // Select item to repair
    void select_item(int32_t index);
    int32_t selected_item() const { return selected_item_; }

    // Callbacks
    using repair_callback = std::function<void(int32_t inventory_slot)>;
    using repair_all_callback = std::function<void()>;

    void set_on_repair(repair_callback callback) { on_repair_ = std::move(callback); }
    void set_on_repair_all(repair_all_callback callback) { on_repair_all_ = std::move(callback); }

private:
    void render_item_list(renderer& rend, int32_t x, int32_t y);
    void render_selected_info(renderer& rend, int32_t x, int32_t y);
    std::optional<int32_t> item_at(int32_t mx, int32_t my) const;
    uint32_t calculate_total_repair_cost() const;

    std::string npc_name_;
    std::vector<repair_item_info> repair_items_;
    uint32_t player_gold_ = 0;
    int32_t selected_item_ = -1;

    int32_t scroll_offset_ = 0;
    static constexpr int32_t visible_items = 6;
    static constexpr int32_t item_row_height = 32;

    std::optional<int32_t> hovered_item_;

    int32_t list_area_x_ = 0;
    int32_t list_area_y_ = 0;

    repair_callback on_repair_;
    repair_all_callback on_repair_all_;
};

} // namespace hb
