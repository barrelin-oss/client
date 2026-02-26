#pragma once

#include "gameplay/item.hpp"
#include "network/messages/common.hpp"
#include <optional>
#include <unordered_map>

namespace hb
{

// ---------------------------------------------------------------------------
// Inventory messages (v2)
// ---------------------------------------------------------------------------

// Full inventory refresh
struct inventory_data_msg
{
    struct positioned_item
    {
        item data;
        int16_t pos_x = 0;
        int16_t pos_y = 0;
        int32_t z_order = 0;
    };
    std::vector<positioned_item> items;
    std::unordered_map<equip_pos, uint32_t> equipment_slots;
    int64_t gold = 0;
    uint32_t weight = 0;
    uint32_t max_weight = 0;

    static inventory_data_msg from_json(const json& j)
    {
        inventory_data_msg data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("gold"))
                data.gold = d["gold"].get<int64_t>();
            if (d.contains("weight"))
                data.weight = d["weight"].get<uint32_t>();
            if (d.contains("max_weight"))
                data.max_weight = d["max_weight"].get<uint32_t>();
            if (d.contains("items") && d["items"].is_array())
            {
                for (const auto& item_j : d["items"])
                {
                    positioned_item pi;
                    if (item_j.contains("item"))
                        pi.data = item::from_json(item_j["item"]);
                    if (item_j.contains("pos_x"))
                        pi.pos_x = item_j["pos_x"].get<int16_t>();
                    if (item_j.contains("pos_y"))
                        pi.pos_y = item_j["pos_y"].get<int16_t>();
                    if (item_j.contains("z_order"))
                        pi.z_order = item_j["z_order"].get<int32_t>();
                    data.items.push_back(std::move(pi));
                }
            }
            if (d.contains("equipment_slots") && d["equipment_slots"].is_object())
            {
                for (auto& [key, val] : d["equipment_slots"].items())
                {
                    auto pos = equip_pos_from_string(key);
                    if (pos != equip_pos::none)
                        data.equipment_slots[pos] = val.get<uint32_t>();
                }
            }
        }
        return data;
    }
};

// New item added to inventory
struct inventory_item_add_msg
{
    item data;
    int16_t pos_x = 0;
    int16_t pos_y = 0;
    int32_t z_order = 0;

    static inventory_item_add_msg from_json(const json& j)
    {
        inventory_item_add_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item"))
                msg.data = item::from_json(d["item"]);
            if (d.contains("pos_x"))
                msg.pos_x = d["pos_x"].get<int16_t>();
            if (d.contains("pos_y"))
                msg.pos_y = d["pos_y"].get<int16_t>();
            if (d.contains("z_order"))
                msg.z_order = d["z_order"].get<int32_t>();
        }
        return msg;
    }
};

// Existing item fully replaced
struct inventory_item_update_msg
{
    item data;
    int16_t pos_x = 0;
    int16_t pos_y = 0;
    int32_t z_order = 0;

    static inventory_item_update_msg from_json(const json& j)
    {
        inventory_item_update_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item"))
                msg.data = item::from_json(d["item"]);
            if (d.contains("pos_x"))
                msg.pos_x = d["pos_x"].get<int16_t>();
            if (d.contains("pos_y"))
                msg.pos_y = d["pos_y"].get<int16_t>();
            if (d.contains("z_order"))
                msg.z_order = d["z_order"].get<int32_t>();
        }
        return msg;
    }
};

// Item removed from inventory
struct inventory_item_removed_msg
{
    uint32_t item_id = 0;

    static inventory_item_removed_msg from_json(const json& j)
    {
        inventory_item_removed_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item_id"))
                msg.item_id = d["item_id"].get<uint32_t>();
        }
        return msg;
    }
};

// Partial update (count/durability only)
struct inventory_item_delta_msg
{
    uint32_t item_id = 0;
    std::optional<uint32_t> count;
    std::optional<uint16_t> durability;

    static inventory_item_delta_msg from_json(const json& j)
    {
        inventory_item_delta_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item_id"))
                msg.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("count") && !d["count"].is_null())
                msg.count = d["count"].get<uint32_t>();
            if (d.contains("durability") && !d["durability"].is_null())
                msg.durability = d["durability"].get<uint16_t>();
        }
        return msg;
    }
};

// Gold changed
struct inventory_gold_update_msg
{
    int64_t gold = 0;

    static inventory_gold_update_msg from_json(const json& j)
    {
        inventory_gold_update_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("gold"))
                msg.gold = d["gold"].get<int64_t>();
        }
        return msg;
    }
};

// Weight changed
struct inventory_weight_update_msg
{
    uint32_t weight = 0;
    uint32_t max_weight = 0;

    static inventory_weight_update_msg from_json(const json& j)
    {
        inventory_weight_update_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("weight"))
                msg.weight = d["weight"].get<uint32_t>();
            if (d.contains("max_weight"))
                msg.max_weight = d["max_weight"].get<uint32_t>();
        }
        return msg;
    }
};

// ---------------------------------------------------------------------------
// Equipment messages (v2)
// ---------------------------------------------------------------------------

// Equip result (ack)
struct equip_result_msg
{
    bool success = false;
    equip_pos slot = equip_pos::none;

    static equip_result_msg from_json(const json& j)
    {
        equip_result_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                msg.success = d["success"].get<bool>();
            if (d.contains("slot"))
                msg.slot = equip_pos_from_string(d["slot"].get<std::string>());
        }
        return msg;
    }
};

// Unequip result (ack)
struct unequip_result_msg
{
    bool success = false;
    equip_pos slot = equip_pos::none;

    static unequip_result_msg from_json(const json& j)
    {
        unequip_result_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                msg.success = d["success"].get<bool>();
            if (d.contains("slot"))
                msg.slot = equip_pos_from_string(d["slot"].get<std::string>());
        }
        return msg;
    }
};

// Server-initiated unequip
struct force_unequip_msg
{
    equip_pos slot = equip_pos::none;
    std::string reason;

    static force_unequip_msg from_json(const json& j)
    {
        force_unequip_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("slot"))
                msg.slot = equip_pos_from_string(d["slot"].get<std::string>());
            if (d.contains("reason"))
                msg.reason = d["reason"].get<std::string>();
        }
        return msg;
    }
};

// Equipment change broadcast
struct equipment_change_msg
{
    uint32_t entity_id = 0;
    equip_pos slot = equip_pos::none;
    std::optional<item> item_data; // nullopt = unequipped

    static equipment_change_msg from_json(const json& j)
    {
        equipment_change_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                msg.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("slot"))
                msg.slot = equip_pos_from_string(d["slot"].get<std::string>());
            if (d.contains("item") && !d["item"].is_null())
                msg.item_data = item::from_json(d["item"]);
        }
        return msg;
    }
};

// ---------------------------------------------------------------------------
// Ground item messages (v2)
// ---------------------------------------------------------------------------

// Ground item spawn
struct ground_item_spawn_msg
{
    item item_data;
    std::string map;
    int16_t x = 0;
    int16_t y = 0;

    static ground_item_spawn_msg from_json(const json& j)
    {
        ground_item_spawn_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item"))
                msg.item_data = item::from_json(d["item"]);
            if (d.contains("map"))
                msg.map = d["map"].get<std::string>();
            if (d.contains("x"))
                msg.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                msg.y = d["y"].get<int16_t>();
        }
        return msg;
    }
};

// Ground item removed
struct ground_item_removed_msg
{
    uint32_t item_id = 0;
    std::string map;
    int16_t x = 0;
    int16_t y = 0;

    static ground_item_removed_msg from_json(const json& j)
    {
        ground_item_removed_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("item_id"))
                msg.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("map"))
                msg.map = d["map"].get<std::string>();
            if (d.contains("x"))
                msg.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                msg.y = d["y"].get<int16_t>();
        }
        return msg;
    }
};

// ---------------------------------------------------------------------------
// Pickup / Drop result messages (v2)
// ---------------------------------------------------------------------------

// Drop result (ack)
struct drop_result_msg
{
    bool success = false;

    static drop_result_msg from_json(const json& j)
    {
        drop_result_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                msg.success = d["success"].get<bool>();
        }
        return msg;
    }
};

// Pickup result (ack)
struct pickup_result_msg
{
    bool success = false;
    std::string error;

    static pickup_result_msg from_json(const json& j)
    {
        pickup_result_msg msg;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                msg.success = d["success"].get<bool>();
            if (d.contains("error"))
                msg.error = d["error"].get<std::string>();
        }
        return msg;
    }
};

// ---------------------------------------------------------------------------
// Skills (unchanged)
// ---------------------------------------------------------------------------

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

// Incremental skill progress update (server -> client, every 5% threshold)
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

// ---------------------------------------------------------------------------
// Request builders (v2)
// ---------------------------------------------------------------------------

inline json make_pickup_request(std::string_view map, int32_t x, int32_t y)
{
    return message_builder(msg_type::pickup_request)
        .set("map", std::string(map))
        .set("x", x)
        .set("y", y)
        .build();
}

inline json make_drop_request(uint32_t item_id)
{
    return message_builder(msg_type::drop_request)
        .set("item_id", item_id)
        .build();
}

inline json make_equip_request(uint32_t item_id, equip_pos slot)
{
    return message_builder(msg_type::equip_request)
        .set("item_id", item_id)
        .set("slot", std::string(equip_pos_to_string(slot)))
        .build();
}

inline json make_unequip_request(equip_pos slot,
                                 std::optional<int16_t> pos_x = std::nullopt,
                                 std::optional<int16_t> pos_y = std::nullopt)
{
    auto b = message_builder(msg_type::unequip_request)
        .set("slot", std::string(equip_pos_to_string(slot)));
    if (pos_x) b.set("pos_x", *pos_x);
    if (pos_y) b.set("pos_y", *pos_y);
    return b.build();
}

inline json make_inventory_reposition_request(uint32_t item_id, int16_t pos_x, int16_t pos_y)
{
    return message_builder(msg_type::inventory_reposition_request)
        .set("item_id", item_id)
        .set("pos_x", pos_x)
        .set("pos_y", pos_y)
        .build();
}

} // namespace hb
