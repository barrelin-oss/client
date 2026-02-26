#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialogs/skills_dialog.hpp"
#include "world/ground_item.hpp"
#include "core/game_enums.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void ws_message_handler::handle_pickup_result(const json& message)
{
    auto data = pickup_result_msg::from_json(message);
    if (data.success)
        spdlog::info("Picked up item (item update will follow)");
    else
        spdlog::debug("Pickup failed: {}", data.error);

    if (entity* player = game_->local_player())
    {
        if (!data.success)
        {
            // Hard rejection (dead, disconnected, etc.) — interrupt and apply cooldown
            player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());
            auto& input = game_->input_handler();
            input.set_blocked_movement_cooldown(input_handler::blocked_movement_cooldown_duration);
        }
        // On success, let the animation play to completion naturally
    }
}

void ws_message_handler::handle_ground_item_removed(const json& message)
{
    auto data = ground_item_removed_msg::from_json(message);
    spdlog::debug("Ground item removed: id={} at ({},{})", data.item_id, data.x, data.y);
    game_->ground_items().remove(data.item_id);
}

void ws_message_handler::handle_hunger_update(const json& message)
{
    auto data = hunger_update_data::from_json(message);

    entity* player = game_->local_player();
    if (!player)
        return;

    player->stats().hunger = static_cast<uint8_t>(std::max(0, static_cast<int>(data.level)));

    auto& sl = game_->get_status_log();
    if (data.is_starving)
    {
        sl.set_message("hunger", "Starving! Regeneration blocked.", status_severity::critical);
        spdlog::warn("Player is starving! Regeneration blocked.");
    }
    else if (data.level < 30)
    {
        sl.set_message("hunger", "Hungry - regeneration delayed", status_severity::warning);
        spdlog::debug("Hunger low: {} - regeneration delayed", data.level);
    }
    else
    {
        sl.remove_message("hunger");
        spdlog::debug("Hunger updated: {}", data.level);
    }
}

void ws_message_handler::handle_stat_update(const json& message)
{
    auto data = stat_update_data::from_json(message);

    entity* player = game_->local_player();
    if (!player)
        return;

    auto& stats = player->stats();
    stats.max_hp = data.max_hp;
    stats.max_mp = data.max_mp;
    stats.max_sp = data.max_sp;
    stats.attack_power = data.attack_power;
    stats.magic_power = data.magic_power;
    stats.defense = data.defense;
    stats.magic_resist = data.magic_defense;
    stats.hit_ratio = data.hit_rate;
    stats.dodge_ratio = data.dodge_rate;
    stats.critical_ratio = data.critical_rate;

    // Full stat updates (teleport/respawn) include current vitals
    if (data.hp)
        stats.hp = *data.hp;
    if (data.mp)
        stats.mp = *data.mp;
    if (data.sp)
        stats.sp = *data.sp;
    if (data.experience)
        stats.experience = *data.experience;
    if (data.level)
        stats.level = *data.level;
    if (data.hunger_level)
        stats.hunger = *data.hunger_level;
    if (data.pk_count && player->has_combat())
        player->combat().pk_count = *data.pk_count;
    // TODO: max_weight updates now come via inventory_weight_update message
    // if (data.max_weight)
    //     game_->inventory().set_weight(0, *data.max_weight);

    game_->update_icon_panel();

    spdlog::debug("Stats updated: max_hp={} max_mp={} max_sp={} atk={} def={}",
                  data.max_hp,
                  data.max_mp,
                  data.max_sp,
                  data.attack_power,
                  data.defense);
}

void ws_message_handler::handle_entity_hp_update(const json& message)
{
    auto data = entity_hp_update_data::from_json(message);
    auto& entities = game_->entities();

    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
    {
        spdlog::debug("HP update for unknown entity {}", data.entity_id);
        return;
    }

    if (ent->has_stats())
    {
        ent->stats().hp = data.hp;
        if (data.hp_max > 0)
            ent->stats().max_hp = data.hp_max;
    }

    // Update HUD if this is the local player
    if (data.entity_id == entities.local_player_id())
        game_->update_icon_panel();

    spdlog::debug("Entity {} HP: {}/{}", data.entity_id, data.hp, data.hp_max);
}

void ws_message_handler::handle_inventory_data(const json& message)
{
    auto data = inventory_data_msg::from_json(message);
    auto& inventory = game_->inventory();
    inventory.clear();
    inventory.set_gold(data.gold);
    inventory.set_weight(data.weight, data.max_weight);

    // Load equipment slot references
    inventory.equipment().clear_all();
    for (const auto& [slot, item_id] : data.equipment_slots)
        inventory.equipment().set(slot, item_id);

    // Add all items to bag
    for (const auto& pi : data.items)
    {
        bag_item entry;
        entry.data = pi.data;
        entry.pos_x = pi.pos_x;
        entry.pos_y = pi.pos_y;
        entry.z_order = pi.z_order;
        inventory.add_or_update_item(entry);
    }

    inventory.rebuild_render_order();
    spdlog::debug("Inventory refreshed: {} items, {} gold", data.items.size(), data.gold);
}

void ws_message_handler::handle_inventory_item_add(const json& message)
{
    auto data = inventory_item_add_msg::from_json(message);
    bag_item entry;
    entry.data = data.data;
    entry.pos_x = data.pos_x;
    entry.pos_y = data.pos_y;
    entry.z_order = data.z_order;
    game_->inventory().add_or_update_item(entry);
    spdlog::debug("Inventory item added: {} (id={})", data.data.name, data.data.item_id);
}

void ws_message_handler::handle_inventory_item_update(const json& message)
{
    auto data = inventory_item_update_msg::from_json(message);
    bag_item entry;
    entry.data = data.data;
    entry.pos_x = data.pos_x;
    entry.pos_y = data.pos_y;
    entry.z_order = data.z_order;
    game_->inventory().add_or_update_item(entry);
    spdlog::debug("Inventory item updated: {} (id={})", data.data.name, data.data.item_id);
}

void ws_message_handler::handle_inventory_item_removed(const json& message)
{
    auto data = inventory_item_removed_msg::from_json(message);
    if (data.item_id == 0)
    {
        spdlog::warn("inventory_item_removed: missing item_id");
        return;
    }

    spdlog::debug("Inventory item {} removed", data.item_id);
    game_->inventory().remove_item(data.item_id);
}

void ws_message_handler::handle_inventory_item_delta(const json& message)
{
    auto data = inventory_item_delta_msg::from_json(message);
    game_->inventory().patch_item(data.item_id, data.count, data.durability);
    spdlog::debug("Inventory item delta: id={}", data.item_id);
}

void ws_message_handler::handle_inventory_gold_update(const json& message)
{
    auto data = inventory_gold_update_msg::from_json(message);
    game_->inventory().set_gold(data.gold);
    spdlog::debug("Gold updated: {}", data.gold);
}

void ws_message_handler::handle_inventory_weight_update(const json& message)
{
    auto data = inventory_weight_update_msg::from_json(message);
    game_->inventory().set_weight(data.weight, data.max_weight);
    spdlog::debug("Weight updated: {}/{}", data.weight, data.max_weight);
}

void ws_message_handler::handle_equip_result(const json& message)
{
    auto data = equip_result_msg::from_json(message);
    if (!data.success)
    {
        spdlog::debug("Equip failed for slot {}", equip_pos_to_string(data.slot));
        return;
    }
    spdlog::info("Equip successful: slot {}", equip_pos_to_string(data.slot));
}

void ws_message_handler::handle_unequip_result(const json& message)
{
    auto data = unequip_result_msg::from_json(message);
    if (!data.success)
    {
        spdlog::debug("Unequip failed for slot {}", equip_pos_to_string(data.slot));
        return;
    }
    spdlog::info("Unequip successful: slot {}", equip_pos_to_string(data.slot));
}

void ws_message_handler::handle_force_unequip(const json& message)
{
    auto data = force_unequip_msg::from_json(message);
    game_->inventory().equipment().clear(data.slot);
    game_->inventory().notify_equipment_changed(data.slot);
    spdlog::info("Force unequipped slot {}: {}", equip_pos_to_string(data.slot), data.reason);
}

void ws_message_handler::handle_equipment_change(const json& message)
{
    auto data = equipment_change_msg::from_json(message);
    auto local_id = game_->entities().local_player_id();

    if (data.entity_id == local_id)
    {
        if (data.item_data)
            game_->inventory().equipment().set(data.slot, data.item_data->item_id);
        else
            game_->inventory().equipment().clear(data.slot);

        game_->inventory().notify_equipment_changed(data.slot);
    }

    spdlog::debug("Equipment change: entity {} slot {} {}", data.entity_id,
                  equip_pos_to_string(data.slot), data.item_data ? "equipped" : "unequipped");
}

void ws_message_handler::handle_drop_result(const json& message)
{
    auto data = drop_result_msg::from_json(message);
    if (data.success)
    {
        spdlog::info("Item dropped successfully");
    }
    else
    {
        spdlog::debug("Drop failed");
        uint32_t item_id = game_->pending_drop_item_id();
        if (item_id != 0)
        {
            auto* entry = game_->inventory().get_bag_item(item_id);
            if (entry)
                entry->locked = false;
        }
    }
    game_->clear_pending_drop_item();
}

void ws_message_handler::handle_skills_data(const json& message)
{
    auto data = skills_data_msg::from_json(message);

    auto& skills = game_->skills();
    for (const auto& sk : data.skills)
    {
        uint8_t mastery = static_cast<uint8_t>(std::min(static_cast<int16_t>(100), sk.level));
        skills.set_mastery(sk.skill_id, mastery);

        float progress = 0.0f;
        if (sk.uses_to_next_level > 0)
            progress = static_cast<float>(sk.uses_this_level) / static_cast<float>(sk.uses_to_next_level);
        skills.set_sub_progress(sk.skill_id, progress);

        spdlog::info("  skill_id={} level={} mastery={} uses={}/{} progress={:.2f}",
                     sk.skill_id,
                     sk.level,
                     mastery,
                     sk.uses_this_level,
                     sk.uses_to_next_level,
                     progress);
    }

    // Push skill data to the dialog
    if (auto* dlg = dynamic_cast<skills_dialog*>(game_->ui().get_dialog(dialog_type::skills)))
    {
        auto all = skills.get_all_skills();
        std::vector<skill> skill_list;
        skill_list.reserve(all.size());
        for (const auto* s : all)
            skill_list.push_back(*s);
        dlg->set_skills(skill_list);
        spdlog::info("Pushed {} skills to dialog", skill_list.size());
    }
    else
    {
        spdlog::warn("Skills dialog not found when trying to push data");
    }
}

void ws_message_handler::handle_skill_progress(const json& message)
{
    auto data = skill_progress_msg::from_json(message);

    auto& skills = game_->skills();

    float progress = 0.0f;
    if (data.uses_to_next_level > 0)
        progress = static_cast<float>(data.uses_this_level) / static_cast<float>(data.uses_to_next_level);
    skills.set_sub_progress(data.skill_id, progress);

    spdlog::debug("skill_progress: skill_id={} {}% ({}/{})",
                  data.skill_id, data.percent, data.uses_this_level, data.uses_to_next_level);

    // Update skills dialog if open
    if (auto* dlg = dynamic_cast<skills_dialog*>(game_->ui().get_dialog(dialog_type::skills)))
    {
        auto all = skills.get_all_skills();
        std::vector<skill> skill_list;
        skill_list.reserve(all.size());
        for (const auto* s : all)
            skill_list.push_back(*s);
        dlg->set_skills(skill_list);
    }
}

void ws_message_handler::handle_player_skill_response(const json& message)
{
    if (message.contains("data"))
    {
        const auto& d = message["data"];
        bool success = d.value("success", false);
        if (!success)
        {
            std::string error = d.value("error", "Unknown error");
            spdlog::debug("Skill use failed: {}", error);
            return;
        }

        if (d.contains("result"))
        {
            const auto& r = d["result"];
            uint32_t skill_id = r.value("skill_id", 0u);
            int32_t effect = r.value("effect_value", 0);
            spdlog::debug("Skill {} used, effect={}", skill_id, effect);
        }
    }
}

void ws_message_handler::handle_player_interact_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    bool success = d.value("success", false);
    if (!success)
    {
        std::string error = d.value("error", "Interaction failed");
        spdlog::debug("Interaction failed: {}", error);
        return;
    }

    if (!d.contains("result"))
        return;
    const auto& result = d["result"];

    std::string interaction_type = result.value("interaction_type", "");

    if (interaction_type == "shop")
    {
        spdlog::info("Shop opened: {}", result["interaction_data"].value("npc_name", ""));
        // TODO: Open shop dialog with items from interaction_data
    }
    else if (interaction_type == "bank")
    {
        spdlog::info("Bank opened: {}", result["interaction_data"].value("npc_name", ""));
        // TODO: Open bank dialog with items from interaction_data
    }
    else if (interaction_type == "dialog")
    {
        spdlog::info("NPC dialog: {}", result["interaction_data"].value("npc_name", ""));
        // TODO: Open NPC dialog with text and options from interaction_data
    }
    else
    {
        spdlog::debug("Unknown interaction type: {}", interaction_type);
    }
}

} // namespace hb
