#pragma once

#include "gameplay/item.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace hb
{

// Inventory constants
inline constexpr size_t max_bag_items = 50;
inline constexpr size_t bank_size = 120;

// Bag item -- an item in the player's bag with free-form position and z-order
struct bag_item
{
    item data;
    int16_t pos_x = 0;
    int16_t pos_y = 0;
    int32_t z_order = 0;
    bool locked = false;
};

// Equipment state (map-based slot storage keyed by equip_pos)
struct equipment_state
{
    std::optional<uint32_t> get(equip_pos slot) const;
    void set(equip_pos slot, uint32_t item_id);
    void clear(equip_pos slot);
    void clear_all();
    std::optional<equip_pos> find_slot_for(uint32_t item_id) const;

    template<typename Func>
        requires std::invocable<Func, equip_pos, uint32_t>
    void for_each(Func&& func) const
    {
        for (const auto& [slot, id] : slots_)
            func(slot, id);
    }

private:
    std::unordered_map<equip_pos, uint32_t> slots_;
};

// Inventory callbacks
struct inventory_callbacks
{
    std::function<void(uint32_t item_id)> on_item_changed;
    std::function<void(equip_pos slot)> on_equipment_changed;
    std::function<void(int64_t gold)> on_gold_changed;
    std::function<void(uint32_t weight, uint32_t max_weight)> on_weight_changed;
};

class inventory_system
{
public:
    inventory_system() = default;
    ~inventory_system() = default;

    // Initialization
    void initialize();
    void clear();

    // Bag item operations (item_id-keyed)
    void add_or_update_item(const bag_item& entry);
    void remove_item(uint32_t item_id);
    bag_item* get_bag_item(uint32_t item_id);
    const bag_item* get_bag_item(uint32_t item_id) const;
    void bring_to_front(uint32_t item_id);
    void set_position(uint32_t item_id, int16_t x, int16_t y);
    const std::vector<uint32_t>& render_order() const { return render_order_; }
    size_t bag_count() const { return bag_items_.size(); }
    void rebuild_render_order();

    // Equipment
    equipment_state& equipment() { return equipment_; }
    const equipment_state& equipment() const { return equipment_; }
    const item* get_equipped_item(equip_pos slot) const;

    // Item patching (partial update)
    void patch_item(uint32_t item_id, std::optional<uint32_t> count, std::optional<uint16_t> durability);

    // Gold
    void set_gold(int64_t gold);
    int64_t gold() const { return gold_; }

    // Weight
    uint32_t current_weight() const { return current_weight_; }
    uint32_t max_weight() const { return max_weight_; }
    bool is_overweight() const { return current_weight_ > max_weight_; }
    void set_weight(uint32_t current, uint32_t max);

    // Query
    uint32_t find_item_by_type(uint32_t template_id) const;
    size_t count_items() const;
    size_t count_item_type(uint32_t template_id) const;
    bool has_item(uint32_t template_id, uint32_t amount = 1) const;

    // Callbacks
    void set_callbacks(const inventory_callbacks& callbacks) { callbacks_ = callbacks; }

    // Iterate all bag items (for rendering, etc.)
    template<typename Func>
        requires std::invocable<Func, uint32_t, const bag_item&>
    void for_each_bag_item(Func&& func) const
    {
        for (const auto& [id, entry] : bag_items_)
        {
            func(id, entry);
        }
    }

    // Notify helpers (public for network handlers)
    void notify_item_changed(uint32_t item_id);

private:
    void notify_equipment_changed(equip_pos slot);
    int32_t max_z_order() const;

    std::unordered_map<uint32_t, bag_item> bag_items_; // keyed by item.data.item_id
    std::vector<uint32_t> render_order_;               // item_ids sorted by z_order ascending

    equipment_state equipment_;
    int64_t gold_ = 0;
    uint32_t current_weight_ = 0;
    uint32_t max_weight_ = 1000;

    inventory_callbacks callbacks_;
};

// Backward compat alias
inline constexpr size_t inventory_size = max_bag_items;

} // namespace hb
