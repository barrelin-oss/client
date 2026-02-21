#include "gameplay/inventory.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb
{

std::optional<item>* equipment::get_slot(equip_slot slot)
{
    switch (slot)
    {
    case equip_slot::head:
        return &head;
    case equip_slot::body:
        return &body;
    case equip_slot::arms:
        return &arms;
    case equip_slot::pants:
        return &pants;
    case equip_slot::boots:
        return &boots;
    case equip_slot::neck:
        return &neck;
    case equip_slot::left_hand:
        return &left_hand;
    case equip_slot::right_hand:
        return &right_hand;
    case equip_slot::left_finger:
        return &left_finger;
    case equip_slot::right_finger:
        return &right_finger;
    case equip_slot::back:
        return &back;
    default:
        return nullptr;
    }
}

const std::optional<item>* equipment::get_slot(equip_slot slot) const
{
    return const_cast<equipment*>(this)->get_slot(slot);
}

void inventory_system::initialize()
{
    clear();
    spdlog::debug("Inventory system initialized");
}

void inventory_system::clear()
{
    for (auto& slot : slots_)
    {
        slot.held_item.reset();
        slot.locked = false;
        slot.pos_x = 0;
        slot.pos_y = 0;
    }
    equipped_ = {};
    gold_ = 0;
    current_weight_ = 0;
}

bool inventory_system::add_item(const item& itm, size_t preferred_slot)
{
    // Try to stack with existing items first
    if (itm.is_stackable())
    {
        for (size_t i = 0; i < inventory_size; ++i)
        {
            if (slots_[i].held_item && slots_[i].held_item->template_id == itm.template_id)
            {
                uint32_t can_add = slots_[i].held_item->max_stack - slots_[i].held_item->amount;
                if (can_add > 0)
                {
                    slots_[i].held_item->amount += std::min(can_add, itm.amount);
                    notify_item_changed(i);
                    recalculate_weight();
                    return true;
                }
            }
        }
    }

    // Find empty slot
    size_t slot = SIZE_MAX;
    if (preferred_slot < inventory_size && !slots_[preferred_slot].held_item)
    {
        slot = preferred_slot;
    }
    else
    {
        slot = find_empty_slot();
    }

    if (slot == SIZE_MAX)
    {
        spdlog::warn("Inventory full, cannot add item");
        return false;
    }

    slots_[slot].held_item = itm;
    notify_item_changed(slot);
    recalculate_weight();
    return true;
}

bool inventory_system::remove_item(size_t slot, uint32_t amount)
{
    if (slot >= inventory_size || !slots_[slot].held_item)
    {
        return false;
    }

    auto& itm = *slots_[slot].held_item;
    if (itm.amount <= amount)
    {
        slots_[slot].held_item.reset();
    }
    else
    {
        itm.amount -= amount;
    }

    notify_item_changed(slot);
    recalculate_weight();
    return true;
}

bool inventory_system::move_item(size_t from_slot, size_t to_slot)
{
    if (from_slot >= inventory_size || to_slot >= inventory_size)
    {
        return false;
    }

    if (from_slot == to_slot)
    {
        return true;
    }

    if (!slots_[from_slot].held_item)
    {
        return false;
    }

    // Try to stack if possible
    if (slots_[to_slot].held_item && slots_[from_slot].held_item->template_id == slots_[to_slot].held_item->template_id &&
        slots_[from_slot].held_item->is_stackable())
    {
        auto& from = *slots_[from_slot].held_item;
        auto& to = *slots_[to_slot].held_item;

        uint32_t can_add = to.max_stack - to.amount;
        uint32_t to_add = std::min(can_add, from.amount);

        to.amount += to_add;
        from.amount -= to_add;

        if (from.amount == 0)
        {
            slots_[from_slot].held_item.reset();
        }
    }
    else
    {
        // Swap items
        std::swap(slots_[from_slot].held_item, slots_[to_slot].held_item);
    }

    notify_item_changed(from_slot);
    notify_item_changed(to_slot);
    return true;
}

bool inventory_system::split_stack(size_t slot, uint32_t amount)
{
    if (slot >= inventory_size || !slots_[slot].held_item)
    {
        return false;
    }

    auto& itm = *slots_[slot].held_item;
    if (!itm.is_stackable() || itm.amount <= amount)
    {
        return false;
    }

    size_t empty_slot = find_empty_slot();
    if (empty_slot == SIZE_MAX)
    {
        return false;
    }

    item new_item = itm;
    new_item.amount = amount;
    itm.amount -= amount;

    slots_[empty_slot].held_item = new_item;

    notify_item_changed(slot);
    notify_item_changed(empty_slot);
    return true;
}

bool inventory_system::equip_item(size_t inventory_slot)
{
    if (inventory_slot >= inventory_size || !slots_[inventory_slot].held_item)
    {
        return false;
    }

    const auto& itm = *slots_[inventory_slot].held_item;
    if (itm.slot == equip_slot::none)
    {
        spdlog::warn("Item {} cannot be equipped", itm.name);
        return false;
    }

    auto* eq_slot = equipped_.get_slot(itm.slot);
    if (!eq_slot)
    {
        return false;
    }

    // Swap equipped item with inventory item
    std::swap(*eq_slot, slots_[inventory_slot].held_item);

    notify_item_changed(inventory_slot);
    notify_equipment_changed(itm.slot);
    recalculate_weight();
    return true;
}

bool inventory_system::unequip_item(equip_slot slot)
{
    auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot || !*eq_slot)
    {
        return false;
    }

    size_t inv_slot = find_empty_slot();
    if (inv_slot == SIZE_MAX)
    {
        spdlog::warn("Inventory full, cannot unequip");
        return false;
    }

    slots_[inv_slot].held_item = std::move(*eq_slot);
    eq_slot->reset();

    notify_item_changed(inv_slot);
    notify_equipment_changed(slot);
    recalculate_weight();
    return true;
}

bool inventory_system::swap_equipment(equip_slot slot, size_t inventory_slot)
{
    auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot)
    {
        return false;
    }

    if (inventory_slot >= inventory_size)
    {
        return false;
    }

    std::swap(*eq_slot, slots_[inventory_slot].held_item);

    notify_item_changed(inventory_slot);
    notify_equipment_changed(slot);
    recalculate_weight();
    return true;
}

const inventory_slot& inventory_system::get_slot(size_t index) const
{
    static inventory_slot empty;
    if (index >= inventory_size)
    {
        return empty;
    }
    return slots_[index];
}

inventory_slot& inventory_system::get_slot_mut(size_t index)
{
    static inventory_slot empty;
    if (index >= inventory_size)
    {
        return empty;
    }
    return slots_[index];
}

const item* inventory_system::get_item(size_t slot) const
{
    if (slot >= inventory_size || !slots_[slot].held_item)
    {
        return nullptr;
    }
    return &(*slots_[slot].held_item);
}

const item* inventory_system::get_equipped(equip_slot slot) const
{
    const auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot || !*eq_slot)
    {
        return nullptr;
    }
    return &(**eq_slot);
}

void inventory_system::set_gold(uint32_t gold)
{
    gold_ = gold;
    if (callbacks_.on_gold_changed)
    {
        callbacks_.on_gold_changed(gold_);
    }
}

bool inventory_system::spend_gold(uint32_t amount)
{
    if (gold_ < amount)
    {
        return false;
    }
    gold_ -= amount;
    if (callbacks_.on_gold_changed)
    {
        callbacks_.on_gold_changed(gold_);
    }
    return true;
}

void inventory_system::add_gold(uint32_t amount)
{
    gold_ += amount;
    if (callbacks_.on_gold_changed)
    {
        callbacks_.on_gold_changed(gold_);
    }
}

size_t inventory_system::find_item_by_type(uint32_t template_id) const
{
    for (size_t i = 0; i < inventory_size; ++i)
    {
        if (slots_[i].held_item && slots_[i].held_item->template_id == template_id)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t inventory_system::find_empty_slot() const
{
    for (size_t i = 0; i < inventory_size; ++i)
    {
        if (!slots_[i].held_item)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t inventory_system::count_items() const
{
    size_t count = 0;
    for (const auto& slot : slots_)
    {
        if (slot.held_item)
        {
            ++count;
        }
    }
    return count;
}

size_t inventory_system::count_item_type(uint32_t template_id) const
{
    size_t count = 0;
    for (const auto& slot : slots_)
    {
        if (slot.held_item && slot.held_item->template_id == template_id)
        {
            count += slot.held_item->amount;
        }
    }
    return count;
}

bool inventory_system::has_item(uint32_t template_id, uint32_t amount) const
{
    return count_item_type(template_id) >= amount;
}

int32_t inventory_system::get_total_bonus(item_attribute attr) const
{
    int32_t total = 0;

    auto add_bonus = [&](const std::optional<item>& itm)
    {
        if (itm)
        {
            total += itm->get_bonus(attr);
        }
    };

    add_bonus(equipped_.head);
    add_bonus(equipped_.body);
    add_bonus(equipped_.arms);
    add_bonus(equipped_.pants);
    add_bonus(equipped_.boots);
    add_bonus(equipped_.neck);
    add_bonus(equipped_.left_hand);
    add_bonus(equipped_.right_hand);
    add_bonus(equipped_.left_finger);
    add_bonus(equipped_.right_finger);
    add_bonus(equipped_.back);

    return total;
}

int32_t inventory_system::get_total_defense() const
{
    int32_t total = 0;

    auto add_defense = [&](const std::optional<item>& itm)
    {
        if (itm)
        {
            total += itm->defense;
        }
    };

    add_defense(equipped_.head);
    add_defense(equipped_.body);
    add_defense(equipped_.arms);
    add_defense(equipped_.pants);
    add_defense(equipped_.boots);
    add_defense(equipped_.left_hand); // Shield

    return total;
}

int32_t inventory_system::get_total_magic_defense() const
{
    int32_t total = 0;

    auto add_defense = [&](const std::optional<item>& itm)
    {
        if (itm)
        {
            total += itm->magic_defense;
        }
    };

    add_defense(equipped_.head);
    add_defense(equipped_.body);
    add_defense(equipped_.arms);
    add_defense(equipped_.pants);
    add_defense(equipped_.boots);

    return total;
}

int32_t inventory_system::get_fire_resistance() const
{
    // Fire resistance comes from main enchantment type "fire"
    int32_t total = 0;
    auto add = [&](const std::optional<item>& itm)
    {
        if (itm && itm->attribute.main_type == enchantment_type::fire)
            total += itm->attribute.main_value;
    };
    add(equipped_.head);
    add(equipped_.body);
    add(equipped_.arms);
    add(equipped_.pants);
    add(equipped_.boots);
    add(equipped_.neck);
    add(equipped_.left_finger);
    add(equipped_.right_finger);
    add(equipped_.back);
    return std::min(100, total);
}

int32_t inventory_system::get_ice_resistance() const
{
    // No direct ice enchantment in the new model; use physical_resist sub-enchantment as proxy
    int32_t total = 0;
    auto add = [&](const std::optional<item>& itm)
    {
        if (itm && itm->attribute.sub_type == sub_enchantment_type::physical_resist)
            total += itm->attribute.sub_value;
    };
    add(equipped_.head);
    add(equipped_.body);
    add(equipped_.arms);
    add(equipped_.pants);
    add(equipped_.boots);
    add(equipped_.neck);
    add(equipped_.left_finger);
    add(equipped_.right_finger);
    add(equipped_.back);
    return std::min(100, total);
}

int32_t inventory_system::get_poison_resistance() const
{
    int32_t total = 0;
    auto add = [&](const std::optional<item>& itm)
    {
        if (itm && itm->attribute.main_type == enchantment_type::poison)
            total += itm->attribute.main_value;
    };
    add(equipped_.head);
    add(equipped_.body);
    add(equipped_.arms);
    add(equipped_.pants);
    add(equipped_.boots);
    add(equipped_.neck);
    add(equipped_.left_finger);
    add(equipped_.right_finger);
    add(equipped_.back);
    return std::min(100, total);
}

int32_t inventory_system::get_spell_accuracy_bonus() const
{
    // Spell accuracy maps to attack_rating sub-enchantment
    int32_t total = 0;
    auto add = [&](const std::optional<item>& itm)
    {
        if (itm && itm->attribute.sub_type == sub_enchantment_type::attack_rating)
            total += itm->attribute.sub_value;
    };
    add(equipped_.head);
    add(equipped_.body);
    add(equipped_.right_hand);
    add(equipped_.left_hand);
    add(equipped_.neck);
    add(equipped_.left_finger);
    add(equipped_.right_finger);
    return total;
}

int32_t inventory_system::get_super_attack_bonus() const
{
    // Maps to main enchantment sharp or magic_damage
    int32_t total = 0;
    auto add = [&](const std::optional<item>& itm)
    {
        if (itm && (itm->attribute.main_type == enchantment_type::sharp ||
                    itm->attribute.main_type == enchantment_type::magic_damage))
            total += itm->attribute.main_value;
    };
    add(equipped_.right_hand);
    add(equipped_.neck);
    add(equipped_.left_finger);
    add(equipped_.right_finger);
    return total;
}

int32_t inventory_system::get_critical_bonus() const
{
    int32_t total = 0;
    auto add = [&](const std::optional<item>& itm)
    {
        if (itm)
        {
            if (itm->attribute.main_type == enchantment_type::critical_bonus ||
                itm->attribute.main_type == enchantment_type::charge_critical)
                total += itm->attribute.main_value;
            if (itm->attribute.sub_type == sub_enchantment_type::critical_damage)
                total += itm->attribute.sub_value;
        }
    };
    add(equipped_.right_hand);
    add(equipped_.neck);
    add(equipped_.left_finger);
    add(equipped_.right_finger);
    return total;
}

inventory_system::equipment_effects inventory_system::get_equipment_effects() const
{
    equipment_effects effects;

    auto process_item = [&](const std::optional<item>& itm)
    {
        if (!itm || itm->attribute.is_empty())
            return;

        const auto& attr = itm->attribute;

        // Main enchantment contributions
        switch (attr.main_type)
        {
        case enchantment_type::fire:
            effects.fire_resist += attr.main_value;
            break;
        case enchantment_type::critical_bonus:
        case enchantment_type::charge_critical:
            effects.critical += attr.main_value;
            break;
        case enchantment_type::sharp:
        case enchantment_type::magic_damage:
            effects.super_attack += attr.main_value;
            break;
        default:
            break;
        }

        // Sub enchantment contributions
        switch (attr.sub_type)
        {
        case sub_enchantment_type::physical_resist:
            effects.ice_resist += attr.sub_value;
            break;
        case sub_enchantment_type::magic_resist:
            effects.spell_accuracy += attr.sub_value;
            break;
        case sub_enchantment_type::hp_recovery:
            effects.hp_recovery += attr.sub_value;
            break;
        case sub_enchantment_type::mp_recovery:
            effects.mp_recovery += attr.sub_value;
            break;
        case sub_enchantment_type::critical_damage:
            effects.critical += attr.sub_value;
            break;
        case sub_enchantment_type::exp_bonus:
            effects.exp_bonus += attr.sub_value;
            break;
        case sub_enchantment_type::gold_bonus:
            effects.gold_bonus += attr.sub_value;
            break;
        default:
            break;
        }
    };

    process_item(equipped_.head);
    process_item(equipped_.body);
    process_item(equipped_.arms);
    process_item(equipped_.pants);
    process_item(equipped_.boots);
    process_item(equipped_.neck);
    process_item(equipped_.left_hand);
    process_item(equipped_.right_hand);
    process_item(equipped_.left_finger);
    process_item(equipped_.right_finger);
    process_item(equipped_.back);

    // Cap resistances at 100%
    effects.fire_resist = std::min(100, effects.fire_resist);
    effects.ice_resist = std::min(100, effects.ice_resist);
    effects.poison_resist = std::min(100, effects.poison_resist);

    return effects;
}

void inventory_system::set_item_at(size_t slot, const item& itm)
{
    if (slot >= inventory_size)
        return;
    slots_[slot].held_item = itm;
    // Default free-form position based on slot index
    auto col = static_cast<int32_t>(slot % 10);
    auto row = static_cast<int32_t>(slot / 10);
    slots_[slot].pos_x = static_cast<int16_t>(col * 34);
    slots_[slot].pos_y = static_cast<int16_t>(row * 34);
    notify_item_changed(slot);
    recalculate_weight();
}

void inventory_system::clear_slot(size_t slot)
{
    if (slot >= inventory_size)
        return;
    slots_[slot].held_item.reset();
    notify_item_changed(slot);
    recalculate_weight();
}

void inventory_system::set_equipped(equip_slot slot, const item& itm)
{
    auto* eq_slot = equipped_.get_slot(slot);
    if (eq_slot)
    {
        *eq_slot = itm;
        notify_equipment_changed(slot);
        recalculate_weight();
    }
}

void inventory_system::clear_equipped(equip_slot slot)
{
    auto* eq_slot = equipped_.get_slot(slot);
    if (eq_slot)
    {
        eq_slot->reset();
        notify_equipment_changed(slot);
        recalculate_weight();
    }
}

void inventory_system::set_item_count(size_t slot, uint32_t count)
{
    if (slot >= inventory_size || !slots_[slot].held_item)
        return;
    slots_[slot].held_item->amount = count;
    notify_item_changed(slot);
}

void inventory_system::set_item_color(size_t slot, uint8_t color)
{
    if (slot >= inventory_size || !slots_[slot].held_item)
        return;
    slots_[slot].held_item->color = color;
    notify_item_changed(slot);
}

void inventory_system::set_item_attribute(size_t slot, const item_attribute_data& attribute)
{
    if (slot >= inventory_size || !slots_[slot].held_item)
        return;
    slots_[slot].held_item->attribute = attribute;
    notify_item_changed(slot);
}

void inventory_system::recalculate_weight()
{
    current_weight_ = 0;

    for (const auto& slot : slots_)
    {
        if (slot.held_item)
        {
            current_weight_ += slot.held_item->weight * slot.held_item->amount;
        }
    }

    auto add_weight = [&](const std::optional<item>& itm)
    {
        if (itm)
        {
            current_weight_ += itm->weight;
        }
    };

    add_weight(equipped_.head);
    add_weight(equipped_.body);
    add_weight(equipped_.arms);
    add_weight(equipped_.pants);
    add_weight(equipped_.boots);
    add_weight(equipped_.neck);
    add_weight(equipped_.left_hand);
    add_weight(equipped_.right_hand);
    add_weight(equipped_.left_finger);
    add_weight(equipped_.right_finger);
    add_weight(equipped_.back);

    if (callbacks_.on_weight_changed)
    {
        callbacks_.on_weight_changed(current_weight_, max_weight_);
    }
}

void inventory_system::notify_item_changed(size_t slot)
{
    if (callbacks_.on_item_changed)
    {
        callbacks_.on_item_changed(slot);
    }
}

void inventory_system::notify_equipment_changed(equip_slot slot)
{
    if (callbacks_.on_equipment_changed)
    {
        callbacks_.on_equipment_changed(slot);
    }
}

size_t inventory_system::promote_to_top(size_t slot)
{
    if (slot >= inventory_size || !slots_[slot].held_item)
        return SIZE_MAX;

    // Find the last occupied slot
    size_t last_occupied = 0;
    for (size_t i = 0; i < inventory_size; ++i)
    {
        if (slots_[i].held_item)
            last_occupied = i;
    }

    // Already at or past the last occupied slot
    if (slot >= last_occupied)
        return slot;

    // Find the first empty slot at or after last_occupied
    size_t dest = SIZE_MAX;
    for (size_t i = last_occupied + 1; i < inventory_size; ++i)
    {
        if (!slots_[i].held_item)
        {
            dest = i;
            break;
        }
    }

    // If no empty slot after the last item, swap with last_occupied + shift
    // Simpler approach: just swap to the end of occupied range
    if (dest == SIZE_MAX)
    {
        // All slots from slot+1..49 are occupied; just swap with last_occupied
        if (slot == last_occupied)
            return slot;
        dest = last_occupied;
    }

    // Move the item
    slots_[dest] = std::move(slots_[slot]);
    slots_[slot].held_item.reset();
    slots_[slot].pos_x = 0;
    slots_[slot].pos_y = 0;

    notify_item_changed(slot);
    notify_item_changed(dest);
    return dest;
}

void inventory_system::set_slot_position(size_t slot, int16_t x, int16_t y)
{
    if (slot >= inventory_size)
        return;
    slots_[slot].pos_x = x;
    slots_[slot].pos_y = y;
}

} // namespace hb
