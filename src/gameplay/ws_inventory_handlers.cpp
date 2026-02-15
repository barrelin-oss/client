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
        spdlog::info(
            "Picked up item: {} x{} (slot {})", response.item_name, response.quantity, response.inventory_slot);
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

    for (const auto& inv_item : data.items)
    {
        item itm;
        itm.id = inv_item.item_id;
        itm.type_id = static_cast<uint16_t>(inv_item.item_id);
        itm.name = inv_item.name;
        itm.amount = static_cast<uint32_t>(inv_item.count);
        itm.durability = static_cast<uint16_t>(inv_item.durability);
        itm.max_durability = static_cast<uint16_t>(inv_item.max_durability);
        inventory.set_item_at(inv_item.slot, itm);
    }

    spdlog::debug("Inventory refreshed: {} items, {} gold", data.items.size(), data.gold);
}

void ws_message_handler::handle_equipment_data(const json& message)
{
    auto data = equipment_data_msg::from_json(message);

    auto& inventory = game_->inventory();

    // Clear all equipped slots first
    for (int i = 1; i <= 11; ++i)
        inventory.clear_equipped(static_cast<equip_slot>(i));

    for (const auto& eq_item : data.equipment)
    {
        item itm;
        itm.id = eq_item.item_id;
        itm.type_id = static_cast<uint16_t>(eq_item.item_id);
        itm.name = eq_item.name;
        itm.durability = static_cast<uint16_t>(eq_item.durability);
        itm.max_durability = static_cast<uint16_t>(eq_item.max_durability);

        auto slot = equip_slot_from_server(eq_item.slot);
        if (slot != equip_slot::none)
            inventory.set_equipped(slot, itm);
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
