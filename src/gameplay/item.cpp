#include "gameplay/item.hpp"
#include <nlohmann/json.hpp>

namespace hb
{

// ---------------------------------------------------------------------------
// item_attribute_data
// ---------------------------------------------------------------------------

bool item_attribute_data::is_empty() const
{
    return upgrade_level == 0 && main_type == enchantment_type::none &&
           sub_type == sub_enchantment_type::none && !custom_made;
}

item_attribute_data item_attribute_data::from_json(const nlohmann::json& j)
{
    item_attribute_data d;
    if (j.contains("upgrade_level"))
        d.upgrade_level = j["upgrade_level"].get<uint8_t>();

    if (j.contains("main_enchant") && !j["main_enchant"].is_null())
    {
        const auto& me = j["main_enchant"];
        if (me.contains("type"))
            d.main_type = enchantment_type_from_string(me["type"].get<std::string>());
        if (me.contains("value"))
            d.main_value = me["value"].get<uint8_t>();
    }

    if (j.contains("sub_enchant") && !j["sub_enchant"].is_null())
    {
        const auto& se = j["sub_enchant"];
        if (se.contains("type"))
            d.sub_type = sub_enchantment_type_from_string(se["type"].get<std::string>());
        if (se.contains("value"))
            d.sub_value = se["value"].get<uint8_t>();
    }

    if (j.contains("custom_made"))
        d.custom_made = j["custom_made"].get<bool>();

    return d;
}

// ---------------------------------------------------------------------------
// item
// ---------------------------------------------------------------------------

item item::from_json(const nlohmann::json& j)
{
    item itm;

    // Identity
    itm.item_id = j.value("item_id", 0u);
    itm.template_id = j.value("template_id", 0u);
    itm.name = j.value("name", std::string{});

    // Classification (string enums)
    if (j.contains("type") && j["type"].is_string())
        itm.type = item_type_from_string(j["type"].get<std::string>());
    if (j.contains("equip_pos") && j["equip_pos"].is_string())
        itm.equip_position = equip_pos_from_string(j["equip_pos"].get<std::string>());
    if (j.contains("weapon_type") && j["weapon_type"].is_string())
        itm.weapon = weapon_type_from_string(j["weapon_type"].get<std::string>());
    if (j.contains("rarity") && j["rarity"].is_string())
        itm.rarity = item_rarity_from_string(j["rarity"].get<std::string>());

    // Stack / weight / price
    itm.count = j.value("count", 1u);
    itm.weight = j.value("weight", 0u);
    itm.price = j.value("price", 0u);
    itm.description = j.value("description", std::string{});

    // Combat stats
    itm.damage_min = j.value("damage_min", 0);
    itm.damage_max = j.value("damage_max", 0);
    itm.defense = j.value("defense", 0);
    itm.magic_defense = j.value("magic_defense", 0);

    // Durability
    itm.durability = j.value("durability", static_cast<uint16_t>(0));
    itm.max_durability = j.value("max_durability", static_cast<uint16_t>(0));

    // Requirements
    itm.level_req = j.value("level_req", static_cast<uint16_t>(0));
    itm.str_req = j.value("str_req", static_cast<uint16_t>(0));
    itm.dex_req = j.value("dex_req", static_cast<uint16_t>(0));
    itm.int_req = j.value("int_req", static_cast<uint16_t>(0));
    itm.mag_req = j.value("mag_req", static_cast<uint16_t>(0));

    // Effects array
    if (j.contains("effects") && j["effects"].is_array())
    {
        uint8_t idx = 0;
        for (const auto& eff_j : j["effects"])
        {
            if (idx >= item::max_effects)
                break;
            item_effect eff;
            if (eff_j.contains("type") && eff_j["type"].is_string())
                eff.type = effect_type_from_string(eff_j["type"].get<std::string>());
            if (eff_j.contains("value"))
                eff.value = eff_j["value"].get<int16_t>();
            if (eff.type != effect_type::none)
            {
                itm.effects[idx] = eff;
                ++idx;
            }
        }
        itm.effect_count = idx;
    }

    // Attribute (upgrade / enchantment)
    if (j.contains("attribute") && j["attribute"].is_object())
        itm.attribute = item_attribute_data::from_json(j["attribute"]);

    // Special ability
    itm.special_ability = j.value("special_ability", std::string{});

    // Visual
    itm.color = j.value("color", static_cast<uint8_t>(0));
    itm.sprite_id = j.value("sprite_id", static_cast<uint16_t>(0));
    itm.sprite_frame = j.value("sprite_frame", static_cast<uint16_t>(0));

    // Flags
    itm.tradeable = j.value("tradeable", true);
    itm.droppable = j.value("droppable", true);
    itm.bound_to = j.value("bound_to", 0u);
    itm.two_handed = j.value("two_handed", false);

    return itm;
}

} // namespace hb
