#pragma once

// Shops and the bank over the JSON protocol (docs/protocol/npc.md, "NPC Interaction
// Messages"). The catalogue and the bank contents arrive inside player_interact_response;
// buying, the sell quote + confirm, deposit and withdraw are request/response.

#include "network/messages/common.hpp"
#include "network/messages/quest.hpp" // json_text
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hb
{

// One entry of a shop's catalogue (interaction_data.items of interaction_type "shop").
// The server writes the template id under "item_id".
struct shop_catalogue_item
{
    uint32_t template_id = 0;
    std::string name;
    uint32_t price = 0;      // what this player pays (charisma applied)
    uint32_t base_price = 0; // template price
    int32_t level_limit = 0;
    int32_t count = 1;
    int32_t category = 0;

    static shop_catalogue_item from_object(const json& o)
    {
        shop_catalogue_item c;
        c.template_id = o.contains("template_id") ? o.value("template_id", 0u) : o.value("item_id", 0u);
        c.name = json_text(o, "name");
        c.price = o.value("price", 0u);
        c.base_price = o.value("base_price", c.price);
        c.level_limit = o.value("level_limit", 0);
        c.count = o.value("count", 1);
        c.category = o.value("category", 0);
        return c;
    }
};

// One occupied bank slot (interaction_data.items of interaction_type "bank")
struct bank_slot_item
{
    int32_t page = 0;
    int32_t slot = 0;
    uint32_t item_id = 0;
    std::string name;
    uint32_t count = 1;
    uint16_t durability = 0;
    uint16_t max_durability = 0;

    static bank_slot_item from_object(const json& o)
    {
        bank_slot_item b;
        b.page = o.value("page", 0);
        b.slot = o.value("slot", 0);
        b.item_id = o.value("item_id", 0u);
        b.name = json_text(o, "name");
        b.count = o.value("count", 1u);
        b.durability = static_cast<uint16_t>(o.value("durability", 0));
        b.max_durability = static_cast<uint16_t>(o.value("max_durability", 0));
        return b;
    }
};

struct shop_buy_response_data
{
    bool success = false;
    std::string error;
    std::string item_name;
    int32_t count = 0;
    int64_t price_paid = 0;
    int64_t gold_remaining = -1;

    static shop_buy_response_data from_json(const json& j)
    {
        shop_buy_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.error = json_text(d, "error");
        r.item_name = json_text(d, "item_name");
        r.count = d.value("count", 0);
        r.price_paid = d.value("price_paid", 0ll);
        r.gold_remaining = d.value("gold_remaining", -1ll);
        return r;
    }
};

struct shop_sell_response_data
{
    bool success = false;
    std::string error;
    std::string item_name;
    int64_t offered_price = 0;

    static shop_sell_response_data from_json(const json& j)
    {
        shop_sell_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.error = json_text(d, "error");
        r.item_name = json_text(d, "item_name");
        r.offered_price = d.value("offered_price", 0ll);
        return r;
    }
};

struct shop_sell_confirm_response_data
{
    bool success = false;
    std::string error;
    int64_t gold_received = 0;
    int64_t gold_total = -1;

    static shop_sell_confirm_response_data from_json(const json& j)
    {
        shop_sell_confirm_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.error = json_text(d, "error");
        r.gold_received = d.value("gold_received", 0ll);
        r.gold_total = d.value("gold_total", -1ll);
        return r;
    }
};

// bank_deposit_response and bank_withdraw_response
struct bank_action_response_data
{
    bool success = false;
    std::string error;
    std::string item_name;

    static bank_action_response_data from_json(const json& j)
    {
        bank_action_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.error = json_text(d, "error");
        r.item_name = json_text(d, "item_name");
        return r;
    }
};

inline json make_shop_buy_request(uint32_t npc_entity_id, uint32_t item_template_id, int32_t count)
{
    return message_builder(msg_type::shop_buy_request)
        .set("npc_entity_id", npc_entity_id)
        .set("item_template_id", item_template_id)
        .set("count", count)
        .build();
}

inline json make_shop_sell_request(uint32_t npc_entity_id, uint32_t item_id, int32_t count)
{
    return message_builder(msg_type::shop_sell_request)
        .set("npc_entity_id", npc_entity_id)
        .set("item_id", item_id)
        .set("count", count)
        .build();
}

inline json make_shop_sell_confirm_request(uint32_t npc_entity_id, uint32_t item_id, int32_t count)
{
    return message_builder(msg_type::shop_sell_confirm_request)
        .set("npc_entity_id", npc_entity_id)
        .set("item_id", item_id)
        .set("count", count)
        .build();
}

inline json make_bank_deposit_request(uint32_t npc_entity_id, uint32_t item_id)
{
    return message_builder(msg_type::bank_deposit_request).set("npc_entity_id", npc_entity_id).set("item_id", item_id).build();
}

inline json make_bank_withdraw_request(uint32_t npc_entity_id, int32_t page, int32_t slot)
{
    return message_builder(msg_type::bank_withdraw_request)
        .set("npc_entity_id", npc_entity_id)
        .set("bank_page", page)
        .set("bank_slot", slot)
        .build();
}

} // namespace hb
