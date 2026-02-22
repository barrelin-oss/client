#pragma once

#include "gameplay/item.hpp"
#include <cstdint>
#include <optional>
#include <array>
#include <functional>
#include <unordered_map>
#include <vector>

namespace hb
{

// Inventory constants
inline constexpr size_t max_bag_items = 50;
inline constexpr size_t equipment_slots = 15;
inline constexpr size_t bank_size = 120;

// Bag item — an item in the player's bag with free-form position and z-order
struct bag_item
{
    item data;
    int16_t pos_x = 0;
    int16_t pos_y = 0;
    int32_t z_order = 0;
    bool locked = false;
};

// Equipment set
struct equipment
{
    std::optional<item> head;         // slot 1
    std::optional<item> body;         // slot 2
    std::optional<item> arms;         // slot 3
    std::optional<item> pants;        // slot 4
    std::optional<item> boots;        // slot 5
    std::optional<item> neck;         // slot 6
    std::optional<item> left_hand;    // slot 7 (shield)
    std::optional<item> right_hand;   // slot 8 (weapon)
    std::optional<item> left_finger;  // slot 10
    std::optional<item> right_finger; // slot 11
    std::optional<item> back;         // slot 12 (cape)

    std::optional<item>* get_slot(equip_slot slot);
    const std::optional<item>* get_slot(equip_slot slot) const;
};

// Inventory callbacks
struct inventory_callbacks
{
    std::function<void(uint32_t item_id)> on_item_changed;
    std::function<void(equip_slot slot)> on_equipment_changed;
    std::function<void(uint32_t gold)> on_gold_changed;
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

    // Equipment operations
    bool equip_item(uint32_t item_id);
    bool unequip_item(equip_slot slot);

    // Equipment access
    const item* get_equipped(equip_slot slot) const;
    void set_equipped(equip_slot slot, const item& itm);
    void clear_equipped(equip_slot slot);

    // Gold
    void set_gold(uint32_t gold);
    uint32_t gold() const { return gold_; }
    bool spend_gold(uint32_t amount);
    void add_gold(uint32_t amount);

    // Weight
    uint32_t current_weight() const { return current_weight_; }
    uint32_t max_weight() const { return max_weight_; }
    void set_max_weight(uint32_t weight) { max_weight_ = weight; }
    bool is_overweight() const { return current_weight_ > max_weight_; }
    void set_weight_info(uint32_t current, uint32_t max);

    // Query
    uint32_t find_item_by_type(uint32_t template_id) const;
    size_t count_items() const;
    size_t count_item_type(uint32_t template_id) const;
    bool has_item(uint32_t template_id, uint32_t amount = 1) const;

    // Equipment bonuses
    int32_t get_total_bonus(item_attribute attr) const;
    int32_t get_total_defense() const;
    int32_t get_total_magic_defense() const;

    // Resistance calculations
    int32_t get_fire_resistance() const;
    int32_t get_ice_resistance() const;
    int32_t get_poison_resistance() const;
    int32_t get_spell_accuracy_bonus() const;
    int32_t get_super_attack_bonus() const;
    int32_t get_critical_bonus() const;

    // Equipment effect summary
    struct equipment_effects
    {
        int32_t fire_resist = 0;
        int32_t ice_resist = 0;
        int32_t poison_resist = 0;
        int32_t spell_accuracy = 0;
        int32_t super_attack = 0;
        int32_t critical = 0;
        int32_t hp_recovery = 0;
        int32_t mp_recovery = 0;
        int32_t exp_bonus = 0;
        int32_t gold_bonus = 0;
    };
    equipment_effects get_equipment_effects() const;

    // Callbacks
    void set_callbacks(const inventory_callbacks& callbacks) { callbacks_ = callbacks; }

    // Direct access for network updates (by item_id)
    void set_item_color(uint32_t item_id, uint8_t color);
    void set_item_attribute(uint32_t item_id, const item_attribute_data& attribute);

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

private:
    void recalculate_weight();
    void notify_item_changed(uint32_t item_id);
    void notify_equipment_changed(equip_slot slot);
    int32_t max_z_order() const;

    std::unordered_map<uint32_t, bag_item> bag_items_; // keyed by item.data.id
    std::vector<uint32_t> render_order_;               // item_ids sorted by z_order ascending

    equipment equipped_;
    uint32_t gold_ = 0;
    uint32_t current_weight_ = 0;
    uint32_t max_weight_ = 1000;

    inventory_callbacks callbacks_;
};

// Backward compat alias
inline constexpr size_t inventory_size = max_bag_items;

} // namespace hb
