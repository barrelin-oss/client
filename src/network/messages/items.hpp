#pragma once

#include "gameplay/item.hpp"
#include "network/messages/common.hpp"

namespace hb
{

// Pickup response data (bare success/fail — item details come via inventory_slot_update)
struct player_pickup_response_data
{
    bool success = false;
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
    int16_t ground_sprite = 0;       // Sprite category (1=swords, 6=misc, etc.)
    int16_t ground_sprite_frame = 0; // Frame within sprite category
    int8_t item_color = 0;           // Color tint index (0 = no tint)
    std::string reason;              // "drop", "existing", etc.

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
            if (d.contains("ground_sprite"))
                data.ground_sprite = d["ground_sprite"].get<int16_t>();
            if (d.contains("ground_sprite_frame"))
                data.ground_sprite_frame = d["ground_sprite_frame"].get<int16_t>();
            if (d.contains("item_color"))
                data.item_color = d["item_color"].get<int8_t>();
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
        uint32_t item_id = 0;
        std::string name;
        uint32_t template_id = 0;
        int16_t count = 1;
        int16_t durability = 0;
        int16_t max_durability = 0;
        int16_t item_type = 0;      // item_type enum from server
        int16_t equip_pos = -1;     // equipment slot position (-1 = not equippable)
        int16_t sprite = 0;         // sprite category ID
        int16_t sprite_frame = 0;   // frame within sprite category
        int16_t color = 0;
        int16_t weight = 0;
        int16_t level_limit = 0;
        int16_t pos_x = 0;
        int16_t pos_y = 0;
        int32_t z_order = 0;
        int8_t equipped_slot = -1; // -1 = not equipped; >= 0 = equipment slot index
        item_attribute_data attribute;
    };
    std::vector<inv_item> items;
    int32_t gold = 0;

    static inv_item parse_inv_item(const json& item_j)
    {
        inv_item itm;
        if (item_j.contains("item_id"))
            itm.item_id = item_j["item_id"].get<uint32_t>();
        if (item_j.contains("name"))
            itm.name = item_j["name"].get<std::string>();
        if (item_j.contains("template_id"))
            itm.template_id = item_j["template_id"].get<uint32_t>();
        if (item_j.contains("count"))
            itm.count = item_j["count"].get<int16_t>();
        if (item_j.contains("durability"))
            itm.durability = item_j["durability"].get<int16_t>();
        if (item_j.contains("max_durability"))
            itm.max_durability = item_j["max_durability"].get<int16_t>();
        if (item_j.contains("item_type"))
            itm.item_type = item_j["item_type"].get<int16_t>();
        if (item_j.contains("equip_pos"))
            itm.equip_pos = item_j["equip_pos"].get<int16_t>();
        if (item_j.contains("sprite"))
            itm.sprite = item_j["sprite"].get<int16_t>();
        if (item_j.contains("sprite_frame"))
            itm.sprite_frame = item_j["sprite_frame"].get<int16_t>();
        if (item_j.contains("color"))
            itm.color = item_j["color"].get<int16_t>();
        if (item_j.contains("weight"))
            itm.weight = item_j["weight"].get<int16_t>();
        if (item_j.contains("level_limit"))
            itm.level_limit = item_j["level_limit"].get<int16_t>();
        if (item_j.contains("pos_x"))
            itm.pos_x = item_j["pos_x"].get<int16_t>();
        if (item_j.contains("pos_y"))
            itm.pos_y = item_j["pos_y"].get<int16_t>();
        if (item_j.contains("z_order"))
            itm.z_order = item_j["z_order"].get<int32_t>();
        if (item_j.contains("equipped_slot"))
            itm.equipped_slot = item_j["equipped_slot"].get<int8_t>();
        if (item_j.contains("attribute"))
        {
            if (item_j["attribute"].is_object())
                itm.attribute = item_attribute_data::from_json(item_j["attribute"]);
            else if (item_j["attribute"].is_number())
                itm.attribute = item_attribute_data::from_legacy(item_j["attribute"].get<uint32_t>());
        }
        return itm;
    }

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
                    data.items.push_back(parse_inv_item(item_j));
                }
            }
        }
        return data;
    }
};

// Equipment data — DEPRECATED: equipment is now unified with inventory.
// Equipped items are inventory items with equipped_slot set.
// Kept for backward compatibility; server no longer sends this message type.
struct equipment_data_msg
{
    std::vector<inventory_data_msg::inv_item> equipment;

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
                    data.equipment.push_back(inventory_data_msg::parse_inv_item(eq_j));
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

// Incremental skill progress update (server → client, every 5% threshold)
struct skill_progress_msg
{
    uint8_t skill_id = 0;
    int32_t uses_this_level = 0;
    int32_t uses_to_next_level = 0;
    uint8_t percent = 0;

    static skill_progress_msg from_json(const json& j)
    {
        skill_progress_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("skill_id"))
                data.skill_id = d["skill_id"].get<uint8_t>();
            if (d.contains("uses_this_level"))
                data.uses_this_level = d["uses_this_level"].get<int32_t>();
            if (d.contains("uses_to_next_level"))
                data.uses_to_next_level = d["uses_to_next_level"].get<int32_t>();
            if (d.contains("percent"))
                data.percent = d["percent"].get<uint8_t>();
        }
        return data;
    }
};

// Item-keyed inventory update (server pushes after drop, consume, modification, etc.)
// Keyed by item_id. has_item == false means item removed.
struct inventory_item_update_msg
{
    uint32_t item_id = 0;
    bool has_item = false;
    inventory_data_msg::inv_item item; // Only valid when has_item == true

    static inventory_item_update_msg from_json(const json& j)
    {
        inventory_item_update_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item") && !d["item"].is_null())
            {
                data.has_item = true;
                data.item = inventory_data_msg::parse_inv_item(d["item"]);
            }
        }
        return data;
    }
};

// Item removed from inventory (server pushes after confirmed drop, etc.)
struct inventory_item_removed_msg
{
    uint32_t item_id = 0;

    static inventory_item_removed_msg from_json(const json& j)
    {
        inventory_item_removed_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item_id"))
                data.item_id = d["item_id"].get<uint32_t>();
        }
        return data;
    }
};

// Weight update (server pushes after item changes)
struct inventory_weight_update_msg
{
    uint32_t current_weight = 0;
    uint32_t max_weight = 0;

    static inventory_weight_update_msg from_json(const json& j)
    {
        inventory_weight_update_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("current_weight"))
                data.current_weight = d["current_weight"].get<uint32_t>();
            if (d.contains("max_weight"))
                data.max_weight = d["max_weight"].get<uint32_t>();
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

inline json make_player_drop_item_request(uint32_t item_id)
{
    return message_builder(msg_type::player_drop_item_request)
        .set("item_id", item_id)
        .build();
}

// Free-form reposition: update item position within bag area
inline json make_inventory_reposition_request(uint32_t item_id, int16_t pos_x, int16_t pos_y)
{
    return message_builder(msg_type::inventory_reposition_request)
        .set("item_id", item_id)
        .set("pos_x", pos_x)
        .set("pos_y", pos_y)
        .build();
}

} // namespace hb
