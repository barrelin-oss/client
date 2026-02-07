#include "gameplay/inventory.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

std::optional<item>* equipment::get_slot(equip_slot slot) {
    switch (slot) {
        case equip_slot::head: return &head;
        case equip_slot::body: return &body;
        case equip_slot::arms: return &arms;
        case equip_slot::pants: return &pants;
        case equip_slot::boots: return &boots;
        case equip_slot::neck: return &neck;
        case equip_slot::left_hand: return &left_hand;
        case equip_slot::right_hand: return &right_hand;
        case equip_slot::left_finger: return &left_finger;
        case equip_slot::right_finger: return &right_finger;
        case equip_slot::back: return &back;
        default: return nullptr;
    }
}

const std::optional<item>* equipment::get_slot(equip_slot slot) const {
    return const_cast<equipment*>(this)->get_slot(slot);
}

void inventory_system::initialize() {
    clear();
    spdlog::debug("Inventory system initialized");
}

void inventory_system::clear() {
    for (auto& slot : slots_) {
        slot.held_item.reset();
        slot.locked = false;
    }
    equipped_ = {};
    gold_ = 0;
    current_weight_ = 0;
}

bool inventory_system::add_item(const item& itm, size_t preferred_slot) {
    // Try to stack with existing items first
    if (itm.is_stackable()) {
        for (size_t i = 0; i < inventory_size; ++i) {
            if (slots_[i].held_item && slots_[i].held_item->type_id == itm.type_id) {
                uint32_t can_add = slots_[i].held_item->max_stack - slots_[i].held_item->amount;
                if (can_add > 0) {
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
    if (preferred_slot < inventory_size && !slots_[preferred_slot].held_item) {
        slot = preferred_slot;
    } else {
        slot = find_empty_slot();
    }

    if (slot == SIZE_MAX) {
        spdlog::warn("Inventory full, cannot add item");
        return false;
    }

    slots_[slot].held_item = itm;
    notify_item_changed(slot);
    recalculate_weight();
    return true;
}

bool inventory_system::remove_item(size_t slot, uint32_t amount) {
    if (slot >= inventory_size || !slots_[slot].held_item) {
        return false;
    }

    auto& itm = *slots_[slot].held_item;
    if (itm.amount <= amount) {
        slots_[slot].held_item.reset();
    } else {
        itm.amount -= amount;
    }

    notify_item_changed(slot);
    recalculate_weight();
    return true;
}

bool inventory_system::move_item(size_t from_slot, size_t to_slot) {
    if (from_slot >= inventory_size || to_slot >= inventory_size) {
        return false;
    }

    if (from_slot == to_slot) {
        return true;
    }

    if (!slots_[from_slot].held_item) {
        return false;
    }

    // Try to stack if possible
    if (slots_[to_slot].held_item &&
        slots_[from_slot].held_item->type_id == slots_[to_slot].held_item->type_id &&
        slots_[from_slot].held_item->is_stackable()) {

        auto& from = *slots_[from_slot].held_item;
        auto& to = *slots_[to_slot].held_item;

        uint32_t can_add = to.max_stack - to.amount;
        uint32_t to_add = std::min(can_add, from.amount);

        to.amount += to_add;
        from.amount -= to_add;

        if (from.amount == 0) {
            slots_[from_slot].held_item.reset();
        }
    } else {
        // Swap items
        std::swap(slots_[from_slot].held_item, slots_[to_slot].held_item);
    }

    notify_item_changed(from_slot);
    notify_item_changed(to_slot);
    return true;
}

bool inventory_system::split_stack(size_t slot, uint32_t amount) {
    if (slot >= inventory_size || !slots_[slot].held_item) {
        return false;
    }

    auto& itm = *slots_[slot].held_item;
    if (!itm.is_stackable() || itm.amount <= amount) {
        return false;
    }

    size_t empty_slot = find_empty_slot();
    if (empty_slot == SIZE_MAX) {
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

bool inventory_system::equip_item(size_t inventory_slot) {
    if (inventory_slot >= inventory_size || !slots_[inventory_slot].held_item) {
        return false;
    }

    const auto& itm = *slots_[inventory_slot].held_item;
    if (itm.slot == equip_slot::none) {
        spdlog::warn("Item {} cannot be equipped", itm.name);
        return false;
    }

    auto* eq_slot = equipped_.get_slot(itm.slot);
    if (!eq_slot) {
        return false;
    }

    // Swap equipped item with inventory item
    std::swap(*eq_slot, slots_[inventory_slot].held_item);

    notify_item_changed(inventory_slot);
    notify_equipment_changed(itm.slot);
    recalculate_weight();
    return true;
}

bool inventory_system::unequip_item(equip_slot slot) {
    auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot || !*eq_slot) {
        return false;
    }

    size_t inv_slot = find_empty_slot();
    if (inv_slot == SIZE_MAX) {
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

bool inventory_system::swap_equipment(equip_slot slot, size_t inventory_slot) {
    auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot) {
        return false;
    }

    if (inventory_slot >= inventory_size) {
        return false;
    }

    std::swap(*eq_slot, slots_[inventory_slot].held_item);

    notify_item_changed(inventory_slot);
    notify_equipment_changed(slot);
    recalculate_weight();
    return true;
}

const inventory_slot& inventory_system::get_slot(size_t index) const {
    static inventory_slot empty;
    if (index >= inventory_size) {
        return empty;
    }
    return slots_[index];
}

inventory_slot& inventory_system::get_slot_mut(size_t index) {
    static inventory_slot empty;
    if (index >= inventory_size) {
        return empty;
    }
    return slots_[index];
}

const item* inventory_system::get_item(size_t slot) const {
    if (slot >= inventory_size || !slots_[slot].held_item) {
        return nullptr;
    }
    return &(*slots_[slot].held_item);
}

const item* inventory_system::get_equipped(equip_slot slot) const {
    const auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot || !*eq_slot) {
        return nullptr;
    }
    return &(**eq_slot);
}

void inventory_system::set_gold(uint32_t gold) {
    gold_ = gold;
    if (callbacks_.on_gold_changed) {
        callbacks_.on_gold_changed(gold_);
    }
}

bool inventory_system::spend_gold(uint32_t amount) {
    if (gold_ < amount) {
        return false;
    }
    gold_ -= amount;
    if (callbacks_.on_gold_changed) {
        callbacks_.on_gold_changed(gold_);
    }
    return true;
}

void inventory_system::add_gold(uint32_t amount) {
    gold_ += amount;
    if (callbacks_.on_gold_changed) {
        callbacks_.on_gold_changed(gold_);
    }
}

size_t inventory_system::find_item_by_type(uint16_t type_id) const {
    for (size_t i = 0; i < inventory_size; ++i) {
        if (slots_[i].held_item && slots_[i].held_item->type_id == type_id) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t inventory_system::find_empty_slot() const {
    for (size_t i = 0; i < inventory_size; ++i) {
        if (!slots_[i].held_item) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t inventory_system::count_items() const {
    size_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.held_item) {
            ++count;
        }
    }
    return count;
}

size_t inventory_system::count_item_type(uint16_t type_id) const {
    size_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.held_item && slot.held_item->type_id == type_id) {
            count += slot.held_item->amount;
        }
    }
    return count;
}

bool inventory_system::has_item(uint16_t type_id, uint32_t amount) const {
    return count_item_type(type_id) >= amount;
}

int32_t inventory_system::get_total_bonus(item_attribute attr) const {
    int32_t total = 0;

    auto add_bonus = [&](const std::optional<item>& itm) {
        if (itm) {
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

int32_t inventory_system::get_total_defense() const {
    int32_t total = 0;

    auto add_defense = [&](const std::optional<item>& itm) {
        if (itm) {
            total += itm->defense;
        }
    };

    add_defense(equipped_.head);
    add_defense(equipped_.body);
    add_defense(equipped_.arms);
    add_defense(equipped_.pants);
    add_defense(equipped_.boots);
    add_defense(equipped_.left_hand);  // Shield

    return total;
}

int32_t inventory_system::get_total_magic_defense() const {
    int32_t total = 0;

    auto add_defense = [&](const std::optional<item>& itm) {
        if (itm) {
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

int32_t inventory_system::get_fire_resistance() const {
    int32_t total = 0;

    auto add_resist = [&](const std::optional<item>& itm) {
        if (itm) {
            auto effect = itm->get_effect_data();
            if (effect.effect_type == item_effect_type::fire_resistance) {
                total += effect.get_resistance_bonus();
            }
        }
    };

    add_resist(equipped_.head);
    add_resist(equipped_.body);
    add_resist(equipped_.arms);
    add_resist(equipped_.pants);
    add_resist(equipped_.boots);
    add_resist(equipped_.neck);
    add_resist(equipped_.left_finger);
    add_resist(equipped_.right_finger);
    add_resist(equipped_.back);

    return std::min(100, total);  // Cap at 100%
}

int32_t inventory_system::get_ice_resistance() const {
    int32_t total = 0;

    auto add_resist = [&](const std::optional<item>& itm) {
        if (itm) {
            auto effect = itm->get_effect_data();
            if (effect.effect_type == item_effect_type::ice_resistance) {
                total += effect.get_resistance_bonus();
            }
        }
    };

    add_resist(equipped_.head);
    add_resist(equipped_.body);
    add_resist(equipped_.arms);
    add_resist(equipped_.pants);
    add_resist(equipped_.boots);
    add_resist(equipped_.neck);
    add_resist(equipped_.left_finger);
    add_resist(equipped_.right_finger);
    add_resist(equipped_.back);

    return std::min(100, total);
}

int32_t inventory_system::get_poison_resistance() const {
    int32_t total = 0;

    auto add_resist = [&](const std::optional<item>& itm) {
        if (itm) {
            auto effect = itm->get_effect_data();
            if (effect.effect_type == item_effect_type::poison_resistance) {
                total += effect.get_resistance_bonus();
            }
        }
    };

    add_resist(equipped_.head);
    add_resist(equipped_.body);
    add_resist(equipped_.arms);
    add_resist(equipped_.pants);
    add_resist(equipped_.boots);
    add_resist(equipped_.neck);
    add_resist(equipped_.left_finger);
    add_resist(equipped_.right_finger);
    add_resist(equipped_.back);

    return std::min(100, total);
}

int32_t inventory_system::get_spell_accuracy_bonus() const {
    int32_t total = 0;

    auto add_bonus = [&](const std::optional<item>& itm) {
        if (itm) {
            auto effect = itm->get_effect_data();
            total += effect.get_spell_bonus();
        }
    };

    add_bonus(equipped_.head);
    add_bonus(equipped_.body);
    add_bonus(equipped_.right_hand);
    add_bonus(equipped_.left_hand);
    add_bonus(equipped_.neck);
    add_bonus(equipped_.left_finger);
    add_bonus(equipped_.right_finger);

    return total;
}

int32_t inventory_system::get_super_attack_bonus() const {
    int32_t total = 0;

    auto add_bonus = [&](const std::optional<item>& itm) {
        if (itm) {
            auto effect = itm->get_effect_data();
            if (effect.effect_type == item_effect_type::super_attack_bonus) {
                total += effect.get_damage_bonus();
            }
        }
    };

    add_bonus(equipped_.right_hand);  // Weapon
    add_bonus(equipped_.neck);
    add_bonus(equipped_.left_finger);
    add_bonus(equipped_.right_finger);

    return total;
}

int32_t inventory_system::get_critical_bonus() const {
    int32_t total = 0;

    auto add_bonus = [&](const std::optional<item>& itm) {
        if (itm) {
            auto effect = itm->get_effect_data();
            if (effect.effect_type == item_effect_type::critical_bonus) {
                total += effect.get_damage_bonus();
            }
        }
    };

    add_bonus(equipped_.right_hand);
    add_bonus(equipped_.neck);
    add_bonus(equipped_.left_finger);
    add_bonus(equipped_.right_finger);

    return total;
}

inventory_system::equipment_effects inventory_system::get_equipment_effects() const {
    equipment_effects effects;

    auto process_item = [&](const std::optional<item>& itm) {
        if (!itm) return;

        auto effect = itm->get_effect_data();
        if (!effect.has_effect) return;

        switch (effect.effect_type) {
            case item_effect_type::fire_resistance:
                effects.fire_resist += effect.get_resistance_bonus();
                break;
            case item_effect_type::ice_resistance:
                effects.ice_resist += effect.get_resistance_bonus();
                break;
            case item_effect_type::poison_resistance:
                effects.poison_resist += effect.get_resistance_bonus();
                break;
            case item_effect_type::spell_accuracy:
                effects.spell_accuracy += effect.get_spell_bonus();
                break;
            case item_effect_type::super_attack_bonus:
                effects.super_attack += effect.get_damage_bonus();
                break;
            case item_effect_type::critical_bonus:
                effects.critical += effect.get_damage_bonus();
                break;
            case item_effect_type::hp_recovery:
                effects.hp_recovery += effect.effect_value;
                break;
            case item_effect_type::mp_recovery:
                effects.mp_recovery += effect.effect_value;
                break;
            case item_effect_type::experience_bonus:
                effects.exp_bonus += effect.effect_value;
                break;
            case item_effect_type::gold_bonus:
                effects.gold_bonus += effect.effect_value;
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

void inventory_system::set_item_at(size_t slot, const item& itm) {
    if (slot >= inventory_size) return;
    slots_[slot].held_item = itm;
    notify_item_changed(slot);
    recalculate_weight();
}

void inventory_system::clear_slot(size_t slot) {
    if (slot >= inventory_size) return;
    slots_[slot].held_item.reset();
    notify_item_changed(slot);
    recalculate_weight();
}

void inventory_system::set_equipped(equip_slot slot, const item& itm) {
    auto* eq_slot = equipped_.get_slot(slot);
    if (eq_slot) {
        *eq_slot = itm;
        notify_equipment_changed(slot);
        recalculate_weight();
    }
}

void inventory_system::clear_equipped(equip_slot slot) {
    auto* eq_slot = equipped_.get_slot(slot);
    if (eq_slot) {
        eq_slot->reset();
        notify_equipment_changed(slot);
        recalculate_weight();
    }
}

void inventory_system::set_item_count(size_t slot, uint32_t count) {
    if (slot >= inventory_size || !slots_[slot].held_item) return;
    slots_[slot].held_item->amount = count;
    notify_item_changed(slot);
}

void inventory_system::set_item_color(size_t slot, uint8_t color) {
    if (slot >= inventory_size || !slots_[slot].held_item) return;
    slots_[slot].held_item->color = color;
    notify_item_changed(slot);
}

void inventory_system::set_item_attribute(size_t slot, uint32_t attribute) {
    if (slot >= inventory_size || !slots_[slot].held_item) return;
    slots_[slot].held_item->attribute = attribute;
    notify_item_changed(slot);
}

void inventory_system::recalculate_weight() {
    current_weight_ = 0;

    for (const auto& slot : slots_) {
        if (slot.held_item) {
            current_weight_ += slot.held_item->weight * slot.held_item->amount;
        }
    }

    auto add_weight = [&](const std::optional<item>& itm) {
        if (itm) {
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

    if (callbacks_.on_weight_changed) {
        callbacks_.on_weight_changed(current_weight_, max_weight_);
    }
}

void inventory_system::notify_item_changed(size_t slot) {
    if (callbacks_.on_item_changed) {
        callbacks_.on_item_changed(slot);
    }
}

void inventory_system::notify_equipment_changed(equip_slot slot) {
    if (callbacks_.on_equipment_changed) {
        callbacks_.on_equipment_changed(slot);
    }
}

} // namespace hb
