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
    bag_items_.clear();
    render_order_.clear();
    equipped_ = {};
    gold_ = 0;
    current_weight_ = 0;
}

void inventory_system::add_or_update_item(const bag_item& entry)
{
    uint32_t id = entry.data.id;
    auto it = bag_items_.find(id);
    if (it != bag_items_.end())
    {
        // Update existing
        it->second = entry;
    }
    else
    {
        // Insert new
        bag_items_.emplace(id, entry);
        render_order_.push_back(id);
    }
    notify_item_changed(id);
    recalculate_weight();
}

void inventory_system::remove_item(uint32_t item_id)
{
    auto it = bag_items_.find(item_id);
    if (it == bag_items_.end())
        return;

    // Remove from render_order_ first (before erasing the bag_item)
    std::erase(render_order_, item_id);
    bag_items_.erase(it);

    notify_item_changed(item_id);
    recalculate_weight();
}

bag_item* inventory_system::get_bag_item(uint32_t item_id)
{
    auto it = bag_items_.find(item_id);
    return it != bag_items_.end() ? &it->second : nullptr;
}

const bag_item* inventory_system::get_bag_item(uint32_t item_id) const
{
    auto it = bag_items_.find(item_id);
    return it != bag_items_.end() ? &it->second : nullptr;
}

void inventory_system::bring_to_front(uint32_t item_id)
{
    auto it = bag_items_.find(item_id);
    if (it == bag_items_.end())
        return;

    // Bump z_order above current max
    it->second.z_order = max_z_order() + 1;

    // Move to end of render_order_
    std::erase(render_order_, item_id);
    render_order_.push_back(item_id);
}

void inventory_system::set_position(uint32_t item_id, int16_t x, int16_t y)
{
    auto it = bag_items_.find(item_id);
    if (it != bag_items_.end())
    {
        it->second.pos_x = x;
        it->second.pos_y = y;
    }
}

void inventory_system::rebuild_render_order()
{
    render_order_.clear();
    render_order_.reserve(bag_items_.size());
    for (const auto& [id, entry] : bag_items_)
    {
        render_order_.push_back(id);
    }
    std::sort(render_order_.begin(), render_order_.end(),
              [this](uint32_t a, uint32_t b)
              {
                  return bag_items_.at(a).z_order < bag_items_.at(b).z_order;
              });
}

int32_t inventory_system::max_z_order() const
{
    int32_t max_z = 0;
    for (const auto& [id, entry] : bag_items_)
    {
        if (entry.z_order > max_z)
            max_z = entry.z_order;
    }
    return max_z;
}

bool inventory_system::equip_item(uint32_t item_id)
{
    auto* entry = get_bag_item(item_id);
    if (!entry)
        return false;

    const auto& itm = entry->data;
    if (itm.slot == equip_slot::none)
    {
        spdlog::warn("Item {} cannot be equipped", itm.name);
        return false;
    }

    auto* eq_slot = equipped_.get_slot(itm.slot);
    if (!eq_slot)
        return false;

    // Swap: unequip current into bag, equip this item
    if (*eq_slot)
    {
        // Put old equipment into a new bag_item
        bag_item old_entry;
        old_entry.data = **eq_slot;
        old_entry.z_order = max_z_order() + 1;
        add_or_update_item(old_entry);
    }

    *eq_slot = itm;
    remove_item(item_id);
    notify_equipment_changed(itm.slot);
    recalculate_weight();
    return true;
}

bool inventory_system::unequip_item(equip_slot slot)
{
    auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot || !*eq_slot)
        return false;

    if (bag_items_.size() >= max_bag_items)
    {
        spdlog::warn("Inventory full, cannot unequip");
        return false;
    }

    bag_item entry;
    entry.data = std::move(**eq_slot);
    entry.z_order = max_z_order() + 1;
    eq_slot->reset();

    add_or_update_item(entry);
    notify_equipment_changed(slot);
    recalculate_weight();
    return true;
}

const item* inventory_system::get_equipped(equip_slot slot) const
{
    const auto* eq_slot = equipped_.get_slot(slot);
    if (!eq_slot || !*eq_slot)
        return nullptr;
    return &(**eq_slot);
}

void inventory_system::set_gold(uint32_t gold)
{
    gold_ = gold;
    if (callbacks_.on_gold_changed)
        callbacks_.on_gold_changed(gold_);
}

bool inventory_system::spend_gold(uint32_t amount)
{
    if (gold_ < amount)
        return false;
    gold_ -= amount;
    if (callbacks_.on_gold_changed)
        callbacks_.on_gold_changed(gold_);
    return true;
}

void inventory_system::add_gold(uint32_t amount)
{
    gold_ += amount;
    if (callbacks_.on_gold_changed)
        callbacks_.on_gold_changed(gold_);
}

void inventory_system::set_weight_info(uint32_t current, uint32_t max)
{
    current_weight_ = current;
    max_weight_ = max;
    if (callbacks_.on_weight_changed)
        callbacks_.on_weight_changed(current_weight_, max_weight_);
}

uint32_t inventory_system::find_item_by_type(uint32_t template_id) const
{
    for (const auto& [id, entry] : bag_items_)
    {
        if (entry.data.template_id == template_id)
            return id;
    }
    return 0; // 0 = not found (valid item_ids are > 0)
}

size_t inventory_system::count_items() const
{
    return bag_items_.size();
}

size_t inventory_system::count_item_type(uint32_t template_id) const
{
    size_t count = 0;
    for (const auto& [id, entry] : bag_items_)
    {
        if (entry.data.template_id == template_id)
            count += entry.data.amount;
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
            total += itm->get_bonus(attr);
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
            total += itm->defense;
    };

    add_defense(equipped_.head);
    add_defense(equipped_.body);
    add_defense(equipped_.arms);
    add_defense(equipped_.pants);
    add_defense(equipped_.boots);
    add_defense(equipped_.left_hand);

    return total;
}

int32_t inventory_system::get_total_magic_defense() const
{
    int32_t total = 0;

    auto add_defense = [&](const std::optional<item>& itm)
    {
        if (itm)
            total += itm->magic_defense;
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

    effects.fire_resist = std::min(100, effects.fire_resist);
    effects.ice_resist = std::min(100, effects.ice_resist);
    effects.poison_resist = std::min(100, effects.poison_resist);

    return effects;
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

void inventory_system::set_item_color(uint32_t item_id, uint8_t color)
{
    auto* entry = get_bag_item(item_id);
    if (!entry)
        return;
    entry->data.color = color;
    notify_item_changed(item_id);
}

void inventory_system::set_item_attribute(uint32_t item_id, const item_attribute_data& attribute)
{
    auto* entry = get_bag_item(item_id);
    if (!entry)
        return;
    entry->data.attribute = attribute;
    notify_item_changed(item_id);
}

void inventory_system::recalculate_weight()
{
    current_weight_ = 0;

    for (const auto& [id, entry] : bag_items_)
    {
        current_weight_ += entry.data.weight * entry.data.amount;
    }

    auto add_weight = [&](const std::optional<item>& itm)
    {
        if (itm)
            current_weight_ += itm->weight;
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
        callbacks_.on_weight_changed(current_weight_, max_weight_);
}

void inventory_system::notify_item_changed(uint32_t item_id)
{
    if (callbacks_.on_item_changed)
        callbacks_.on_item_changed(item_id);
}

void inventory_system::notify_equipment_changed(equip_slot slot)
{
    if (callbacks_.on_equipment_changed)
        callbacks_.on_equipment_changed(slot);
}

} // namespace hb
