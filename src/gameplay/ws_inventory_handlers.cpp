#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialogs/skills_dialog.hpp"
#include "world/ground_item.hpp"
#include "core/game_enums.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void ws_message_handler::handle_pickup_response(const json& message)
{
    auto response = player_pickup_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Picked up item (slot update will follow)");
    }
    else
    {
        spdlog::debug("Pickup failed: {}", response.error_message);
    }

    if (entity* player = game_->local_player())
    {
        player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());
    }
}

void ws_message_handler::handle_ground_item_removed(const json& message)
{
    auto data = ground_item_removed_data::from_json(message);

    spdlog::debug("Ground item removed: {} picked up {} at ({},{})", data.picker_name, data.item_name, data.x, data.y);

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
    if (data.max_weight)
        game_->inventory().set_max_weight(*data.max_weight);

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

void ws_message_handler::handle_equipment_change_broadcast(const json& message)
{
    auto data = equipment_change_broadcast_data::from_json(message);

    // Skip our own changes — handled by equip/unequip response
    if (data.entity_id == game_->entities().local_player_id())
        return;

    spdlog::debug("Entity {} equipment slot {} changed: item={} template={}",
                  data.entity_id,
                  data.slot,
                  data.item_id,
                  data.template_id);

    // TODO: Update other player's visual equipment when character rendering supports it
}

void ws_message_handler::handle_player_equip_response(const json& message)
{
    auto data = player_equip_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::debug("Equip failed: {}", data.error);
        return;
    }

    spdlog::info("Equipped '{}' to slot {}", data.item_name, data.slot);

    // Server will send stat_update and inventory_data/equipment_data
    // to refresh our state, so we don't need to manually update here
}

void ws_message_handler::handle_player_unequip_response(const json& message)
{
    auto data = player_unequip_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::debug("Unequip failed: {}", data.error);
        return;
    }

    spdlog::info("Unequipped '{}' from slot {} -> inventory slot {}", data.item_name, data.slot, data.inventory_slot);
}


void ws_message_handler::handle_inventory_data(const json& message)
{
    auto data = inventory_data_msg::from_json(message);

    auto& inventory = game_->inventory();
    inventory.clear();
    inventory.set_gold(static_cast<uint32_t>(data.gold));

    // Clear all equipped slots first (equipment is unified with inventory)
    for (int i = 1; i <= 12; ++i)
        inventory.clear_equipped(static_cast<equip_slot>(i));

    for (const auto& inv_item : data.items)
    {
        item itm;
        itm.id = inv_item.item_id;
        itm.template_id = inv_item.template_id;
        itm.name = inv_item.name;
        itm.amount = static_cast<uint32_t>(inv_item.count);
        itm.durability = static_cast<uint16_t>(inv_item.durability);
        itm.max_durability = static_cast<uint16_t>(inv_item.max_durability);
        itm.type = static_cast<item_type>(inv_item.item_type);
        itm.slot = equip_slot_from_server(inv_item.equip_pos);
        itm.sprite_id = static_cast<uint16_t>(inv_item.sprite);
        itm.equipped_sprite_id = static_cast<uint16_t>(inv_item.sprite_frame);
        itm.color = static_cast<uint8_t>(inv_item.color);
        itm.weight = static_cast<uint32_t>(inv_item.weight);
        itm.level_req = static_cast<uint16_t>(inv_item.level_limit);
        itm.attribute = inv_item.attribute;
        inventory.set_item_at(inv_item.slot, itm);

        if (inv_item.pos_x != 0 || inv_item.pos_y != 0)
            inventory.set_slot_position(inv_item.slot, inv_item.pos_x, inv_item.pos_y);

        // Equipped items: inventory items with equipped_slot set
        if (inv_item.equipped_slot >= 0)
        {
            auto eq_slot = equip_slot_from_server(inv_item.equipped_slot);
            if (eq_slot != equip_slot::none)
                inventory.set_equipped(eq_slot, itm);
        }
    }

    spdlog::debug("Inventory refreshed: {} items, {} gold", data.items.size(), data.gold);
}

void ws_message_handler::handle_equipment_data(const json& message)
{
    // DEPRECATED: equipment is unified with inventory. This handler kept for backward compat.
    auto data = equipment_data_msg::from_json(message);

    auto& inventory = game_->inventory();

    for (int i = 1; i <= 12; ++i)
        inventory.clear_equipped(static_cast<equip_slot>(i));

    for (const auto& eq_item : data.equipment)
    {
        item itm;
        itm.id = eq_item.item_id;
        itm.template_id = eq_item.template_id;
        itm.name = eq_item.name;
        itm.durability = static_cast<uint16_t>(eq_item.durability);
        itm.max_durability = static_cast<uint16_t>(eq_item.max_durability);
        itm.type = static_cast<item_type>(eq_item.item_type);
        itm.slot = equip_slot_from_server(eq_item.equip_pos);
        itm.sprite_id = static_cast<uint16_t>(eq_item.sprite);
        itm.equipped_sprite_id = static_cast<uint16_t>(eq_item.sprite_frame);
        itm.color = static_cast<uint8_t>(eq_item.color);
        itm.weight = static_cast<uint32_t>(eq_item.weight);
        itm.level_req = static_cast<uint16_t>(eq_item.level_limit);
        itm.attribute = eq_item.attribute;

        if (eq_item.equipped_slot >= 0)
        {
            auto mapped_slot = equip_slot_from_server(eq_item.equipped_slot);
            if (mapped_slot != equip_slot::none)
                inventory.set_equipped(mapped_slot, itm);
        }
    }

    spdlog::debug("Equipment refreshed: {} items", data.equipment.size());
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

void ws_message_handler::handle_drop_item_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    // Determine which slot was pending
    int32_t slot = d.value("slot", -1);
    if (slot < 0)
        slot = game_->pending_drop_slot();

    bool success = d.value("success", false);
    if (success)
    {
        spdlog::info("Item dropped successfully (slot {})", slot);
        // Server will send inventory_data refresh which clears the slot
    }
    else
    {
        spdlog::debug("Drop failed: {}", d.value("error", std::string("unknown")));
        // Unlock the slot so the player can interact with it again
        if (slot >= 0 && slot < static_cast<int32_t>(inventory_size))
            game_->inventory().get_slot_mut(static_cast<size_t>(slot)).locked = false;
    }

    game_->clear_pending_drop_slot();
}

void ws_message_handler::handle_inventory_slot_update(const json& message)
{
    auto data = inventory_slot_update_msg::from_json(message);
    if (data.slot < 0 || data.slot >= static_cast<int16_t>(inventory_size))
    {
        spdlog::warn("inventory_slot_update: invalid slot {}", data.slot);
        return;
    }

    auto& inventory = game_->inventory();
    auto slot = static_cast<size_t>(data.slot);

    if (!data.has_item)
    {
        // Slot cleared (item dropped, consumed, etc.)
        spdlog::debug("Inventory slot {} cleared", data.slot);
        inventory.clear_slot(slot);
    }
    else
    {
        // Slot updated with new/modified item
        item itm;
        itm.id = data.item.item_id;
        itm.template_id = data.item.template_id;
        itm.name = data.item.name;
        itm.amount = static_cast<uint32_t>(data.item.count);
        itm.durability = static_cast<uint16_t>(data.item.durability);
        itm.max_durability = static_cast<uint16_t>(data.item.max_durability);
        itm.type = static_cast<item_type>(data.item.item_type);
        itm.slot = equip_slot_from_server(data.item.equip_pos);
        itm.sprite_id = static_cast<uint16_t>(data.item.sprite);
        itm.equipped_sprite_id = static_cast<uint16_t>(data.item.sprite_frame);
        itm.color = static_cast<uint8_t>(data.item.color);
        itm.weight = static_cast<uint32_t>(data.item.weight);
        itm.level_req = static_cast<uint16_t>(data.item.level_limit);
        itm.attribute = data.item.attribute;
        inventory.set_item_at(slot, itm);

        // Apply position if server provided it
        if (data.item.pos_x != 0 || data.item.pos_y != 0)
            inventory.set_slot_position(slot, data.item.pos_x, data.item.pos_y);

        // Handle equipped_slot flag
        if (data.item.equipped_slot >= 0)
        {
            auto eq_slot = equip_slot_from_server(data.item.equipped_slot);
            if (eq_slot != equip_slot::none)
                inventory.set_equipped(eq_slot, itm);
        }

        spdlog::debug("Inventory slot {} updated: {} x{}", data.slot, itm.name, itm.amount);
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
