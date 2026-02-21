#pragma once

#include "gameplay/item.hpp"
#include <cstdint>
#include <optional>
#include <array>
#include <functional>

namespace hb
{

// Inventory constants
inline constexpr size_t inventory_size = 50;
inline constexpr size_t equipment_slots = 15;
inline constexpr size_t bank_size = 120;

// Inventory slot
struct inventory_slot
{
    std::optional<item> held_item;
    bool locked = false; // For trade/exchange locking
    int16_t pos_x = 0;  // Free-form: pixel x in bag area
    int16_t pos_y = 0;  // Free-form: pixel y in bag area
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
    std::function<void(size_t slot)> on_item_changed;
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

    // Inventory operations
    bool add_item(const item& itm, size_t preferred_slot = SIZE_MAX);
    bool remove_item(size_t slot, uint32_t amount = 1);
    bool move_item(size_t from_slot, size_t to_slot);
    bool split_stack(size_t slot, uint32_t amount);

    // Equipment operations
    bool equip_item(size_t inventory_slot);
    bool unequip_item(equip_slot slot);
    bool swap_equipment(equip_slot slot, size_t inventory_slot);

    // Slot access
    const inventory_slot& get_slot(size_t index) const;
    inventory_slot& get_slot_mut(size_t index);
    const item* get_item(size_t slot) const;
    const item* get_equipped(equip_slot slot) const;

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

    // Query
    size_t find_item_by_type(uint32_t template_id) const;
    size_t find_empty_slot() const;
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

    // Direct access (for network updates)
    void set_item_at(size_t slot, const item& itm);
    void clear_slot(size_t slot);
    void set_equipped(equip_slot slot, const item& itm);
    void clear_equipped(equip_slot slot);
    void set_item_count(size_t slot, uint32_t count);
    void set_item_color(size_t slot, uint8_t color);
    void set_item_attribute(size_t slot, const item_attribute_data& attribute);

    // Free-form z-ordering: move item to end of occupied slots (highest z-index)
    // Returns new slot index, or SIZE_MAX on failure
    size_t promote_to_top(size_t slot);

    // Update position of a slot (free-form mode)
    void set_slot_position(size_t slot, int16_t x, int16_t y);

private:
    void recalculate_weight();
    void notify_item_changed(size_t slot);
    void notify_equipment_changed(equip_slot slot);

    std::array<inventory_slot, inventory_size> slots_;
    equipment equipped_;
    uint32_t gold_ = 0;
    uint32_t current_weight_ = 0;
    uint32_t max_weight_ = 1000;

    inventory_callbacks callbacks_;
};

} // namespace hb
