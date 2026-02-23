#include "gameplay/inventory.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace hb
{

// --- equipment_state ---

std::optional<uint32_t> equipment_state::get(equip_pos slot) const
{
    auto it = slots_.find(slot);
    if (it == slots_.end())
        return std::nullopt;
    return it->second;
}

void equipment_state::set(equip_pos slot, uint32_t item_id)
{
    slots_[slot] = item_id;
}

void equipment_state::clear(equip_pos slot)
{
    slots_.erase(slot);
}

void equipment_state::clear_all()
{
    slots_.clear();
}

std::optional<equip_pos> equipment_state::find_slot_for(uint32_t item_id) const
{
    for (const auto& [slot, id] : slots_)
    {
        if (id == item_id)
            return slot;
    }
    return std::nullopt;
}

// --- inventory_system ---

void inventory_system::initialize()
{
    clear();
    spdlog::debug("Inventory system initialized");
}

void inventory_system::clear()
{
    bag_items_.clear();
    render_order_.clear();
    equipment_.clear_all();
    gold_ = 0;
    current_weight_ = 0;
}

void inventory_system::add_or_update_item(const bag_item& entry)
{
    uint32_t id = entry.data.item_id;
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

const item* inventory_system::get_equipped_item(equip_pos slot) const
{
    auto id = equipment_.get(slot);
    if (!id)
        return nullptr;
    auto it = bag_items_.find(*id);
    if (it == bag_items_.end())
        return nullptr;
    return &it->second.data;
}

void inventory_system::patch_item(uint32_t item_id, std::optional<uint32_t> count,
                                  std::optional<uint16_t> durability)
{
    auto it = bag_items_.find(item_id);
    if (it == bag_items_.end())
        return;
    if (count)
        it->second.data.count = *count;
    if (durability)
        it->second.data.durability = *durability;
    notify_item_changed(item_id);
}

void inventory_system::set_gold(int64_t gold)
{
    gold_ = gold;
    if (callbacks_.on_gold_changed)
        callbacks_.on_gold_changed(gold_);
}

void inventory_system::set_weight(uint32_t current, uint32_t max)
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
            count += entry.data.count;
    }
    return count;
}

bool inventory_system::has_item(uint32_t template_id, uint32_t amount) const
{
    return count_item_type(template_id) >= amount;
}

void inventory_system::notify_item_changed(uint32_t item_id)
{
    if (callbacks_.on_item_changed)
        callbacks_.on_item_changed(item_id);
}

void inventory_system::notify_equipment_changed(equip_pos slot)
{
    if (callbacks_.on_equipment_changed)
        callbacks_.on_equipment_changed(slot);
}

} // namespace hb
