#pragma once

#include "network/messages/common.hpp"

namespace hb
{

// Pickup response data
struct player_pickup_response_data
{
    bool success = false;
    uint32_t item_id = 0;
    std::string item_name;
    int16_t quantity = 0;
    uint8_t inventory_slot = 0;
    std::string error_message;

    static player_pickup_response_data from_json(const json& j)
    {
        player_pickup_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error_message = d["error"].get<std::string>();

            // Success data is nested in "result" object per protocol
            if (data.success && d.contains("result"))
            {
                const auto& r = d["result"];
                if (r.contains("item_id"))
                    data.item_id = r["item_id"].get<uint32_t>();
                if (r.contains("item_name"))
                    data.item_name = r["item_name"].get<std::string>();
                if (r.contains("quantity"))
                    data.quantity = r["quantity"].get<int16_t>();
                if (r.contains("inventory_slot"))
                    data.inventory_slot = r["inventory_slot"].get<uint8_t>();
            }
        }
        return data;
    }
};

// Ground item removed broadcast data
struct ground_item_removed_data
{
    uint32_t picker_id = 0;
    std::string picker_name;
    uint32_t item_id = 0;
    std::string item_name;
    int32_t x = 0;
    int32_t y = 0;

    static ground_item_removed_data from_json(const json& j)
    {
        ground_item_removed_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("picker_id"))
                data.picker_id = d["picker_id"].get<uint32_t>();
            if (d.contains("picker_name"))
                data.picker_name = d["picker_name"].get<std::string>();
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item_name"))
                data.item_name = d["item_name"].get<std::string>();
            if (d.contains("x"))
                data.x = d["x"].get<int32_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int32_t>();
        }
        return data;
    }
};

// Ground item spawn data
struct ground_item_spawn_data
{
    uint32_t item_id = 0;
    uint32_t template_id = 0;
    std::string item_name;
    int16_t count = 1;
    int16_t x = 0;
    int16_t y = 0;
    std::string reason; // "drop", "existing", etc.

    static ground_item_spawn_data from_json(const json& j)
    {
        ground_item_spawn_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("template_id"))
                data.template_id = d["template_id"].get<uint32_t>();
            if (d.contains("item_name"))
                data.item_name = d["item_name"].get<std::string>();
            if (d.contains("count"))
                data.count = d["count"].get<int16_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
            if (d.contains("reason"))
                data.reason = d["reason"].get<std::string>();
        }
        return data;
    }
};

// Player equip response data
struct player_equip_response_data
{
    bool success = false;
    uint8_t slot = 0;
    uint32_t item_id = 0;
    std::string item_name;
    int16_t durability = 0;
    int16_t max_durability = 0;
    uint32_t swapped_item_id = 0;
    uint8_t swapped_to_inv_slot = 0;
    std::string error;

    static player_equip_response_data from_json(const json& j)
    {
        player_equip_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("slot"))
                data.slot = d["slot"].get<uint8_t>();
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item_name"))
                data.item_name = d["item_name"].get<std::string>();
            if (d.contains("durability"))
                data.durability = d["durability"].get<int16_t>();
            if (d.contains("max_durability"))
                data.max_durability = d["max_durability"].get<int16_t>();
            if (d.contains("swapped_item_id"))
                data.swapped_item_id = d["swapped_item_id"].get<uint32_t>();
            if (d.contains("swapped_to_inv_slot"))
                data.swapped_to_inv_slot = d["swapped_to_inv_slot"].get<uint8_t>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Player unequip response data
struct player_unequip_response_data
{
    bool success = false;
    uint8_t slot = 0;
    uint32_t item_id = 0;
    std::string item_name;
    uint8_t inventory_slot = 0;
    std::string error;

    static player_unequip_response_data from_json(const json& j)
    {
        player_unequip_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("slot"))
                data.slot = d["slot"].get<uint8_t>();
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item_name"))
                data.item_name = d["item_name"].get<std::string>();
            if (d.contains("inventory_slot"))
                data.inventory_slot = d["inventory_slot"].get<uint8_t>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Equipment change broadcast data
struct equipment_change_broadcast_data
{
    uint32_t entity_id = 0;
    uint8_t slot = 0;
    uint32_t item_id = 0;
    uint32_t template_id = 0;

    static equipment_change_broadcast_data from_json(const json& j)
    {
        equipment_change_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("slot"))
                data.slot = d["slot"].get<uint8_t>();
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("template_id"))
                data.template_id = d["template_id"].get<uint32_t>();
        }
        return data;
    }
};

// Inventory data (full refresh)
struct inventory_data_msg
{
    struct inv_item
    {
        uint8_t slot = 0;
        uint32_t item_id = 0;
        std::string name;
        int16_t count = 1;
        int16_t durability = 100;
        int16_t max_durability = 100;
    };
    std::vector<inv_item> items;
    int32_t gold = 0;

    static inventory_data_msg from_json(const json& j)
    {
        inventory_data_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("gold"))
                data.gold = d["gold"].get<int32_t>();
            if (d.contains("items") && d["items"].is_array())
            {
                for (const auto& item_j : d["items"])
                {
                    inv_item itm;
                    if (item_j.contains("slot"))
                        itm.slot = item_j["slot"].get<uint8_t>();
                    if (item_j.contains("item_id"))
                        itm.item_id = item_j["item_id"].get<uint32_t>();
                    if (item_j.contains("name"))
                        itm.name = item_j["name"].get<std::string>();
                    if (item_j.contains("count"))
                        itm.count = item_j["count"].get<int16_t>();
                    if (item_j.contains("durability"))
                        itm.durability = item_j["durability"].get<int16_t>();
                    if (item_j.contains("max_durability"))
                        itm.max_durability = item_j["max_durability"].get<int16_t>();
                    data.items.push_back(std::move(itm));
                }
            }
        }
        return data;
    }
};

// Equipment data (full refresh)
struct equipment_data_msg
{
    struct equip_item
    {
        uint8_t slot = 0;
        uint32_t item_id = 0;
        std::string name;
        int16_t durability = 100;
        int16_t max_durability = 100;
    };
    std::vector<equip_item> equipment;

    static equipment_data_msg from_json(const json& j)
    {
        equipment_data_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("equipment") && d["equipment"].is_array())
            {
                for (const auto& eq_j : d["equipment"])
                {
                    equip_item itm;
                    if (eq_j.contains("slot"))
                        itm.slot = eq_j["slot"].get<uint8_t>();
                    if (eq_j.contains("item_id"))
                        itm.item_id = eq_j["item_id"].get<uint32_t>();
                    if (eq_j.contains("name"))
                        itm.name = eq_j["name"].get<std::string>();
                    if (eq_j.contains("durability"))
                        itm.durability = eq_j["durability"].get<int16_t>();
                    if (eq_j.contains("max_durability"))
                        itm.max_durability = eq_j["max_durability"].get<int16_t>();
                    data.equipment.push_back(std::move(itm));
                }
            }
        }
        return data;
    }
};

// Skills data (full refresh)
struct skills_data_msg
{
    struct skill_entry
    {
        uint8_t skill_id = 0;
        int16_t level = 0;
        int32_t total_uses = 0;         // Lifetime use count
        int32_t uses_this_level = 0;    // Uses accumulated at current level (progress numerator)
        int32_t uses_to_next_level = 0; // Uses required to reach next level (progress denominator)
    };
    std::vector<skill_entry> skills;

    static skills_data_msg from_json(const json& j)
    {
        skills_data_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("skills") && d["skills"].is_array())
            {
                for (const auto& sk_j : d["skills"])
                {
                    skill_entry sk;
                    if (sk_j.contains("skill_id"))
                        sk.skill_id = sk_j["skill_id"].get<uint8_t>();
                    if (sk_j.contains("level"))
                        sk.level = sk_j["level"].get<int16_t>();
                    if (sk_j.contains("total_uses"))
                        sk.total_uses = sk_j["total_uses"].get<int32_t>();
                    if (sk_j.contains("uses_this_level"))
                        sk.uses_this_level = sk_j["uses_this_level"].get<int32_t>();
                    if (sk_j.contains("uses_to_next_level"))
                        sk.uses_to_next_level = sk_j["uses_to_next_level"].get<int32_t>();
                    data.skills.push_back(sk);
                }
            }
        }
        return data;
    }
};

// Convenience functions
inline json make_player_pickup_request(int32_t x, int32_t y, uint32_t item_id = 0)
{
    return message_builder(msg_type::player_pickup_request)
        .set("x", x)
        .set("y", y)
        .set("item_id", item_id)
        .set("timestamp",
             static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count()))
        .build();
}

} // namespace hb
