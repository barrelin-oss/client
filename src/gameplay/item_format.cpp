#include "gameplay/item_format.hpp"
#include <nlohmann/json.hpp>
#include <format>

namespace hb
{

// ---------------------------------------------------------------------------
// item_attribute_data
// ---------------------------------------------------------------------------

bool item_attribute_data::is_empty() const
{
    return upgrade_level == 0 && main_type == enchantment_type::none && sub_type == sub_enchantment_type::none &&
           !custom_made;
}

item_attribute_data item_attribute_data::from_json(const nlohmann::json& j)
{
    item_attribute_data d;
    if (j.contains("upgrade"))
        d.upgrade_level = j["upgrade"].get<uint8_t>();
    if (j.contains("main_type"))
        d.main_type = static_cast<enchantment_type>(j["main_type"].get<uint8_t>());
    if (j.contains("main_value"))
        d.main_value = j["main_value"].get<uint8_t>();
    if (j.contains("sub_type"))
        d.sub_type = static_cast<sub_enchantment_type>(j["sub_type"].get<uint8_t>());
    if (j.contains("sub_value"))
        d.sub_value = j["sub_value"].get<uint8_t>();
    if (j.contains("custom_made"))
        d.custom_made = j["custom_made"].get<bool>();
    if (j.contains("custom_quality"))
        d.custom_quality = j["custom_quality"].get<int8_t>();
    return d;
}

item_attribute_data item_attribute_data::from_legacy(uint32_t dw)
{
    item_attribute_data d;
    if (dw == 0)
        return d;

    // Legacy bit layout:
    // bits 31-28: upgrade level
    // bits 23-20: main enchantment type
    // bits 19-16: main enchantment value
    // bits 15-12: sub enchantment type
    // bits 11-8:  sub enchantment value
    // bit 0:      custom made flag
    d.upgrade_level = static_cast<uint8_t>((dw >> 28) & 0x0F);
    d.main_type = static_cast<enchantment_type>((dw >> 20) & 0x0F);
    d.main_value = static_cast<uint8_t>((dw >> 16) & 0x0F);
    d.sub_type = static_cast<sub_enchantment_type>((dw >> 12) & 0x0F);
    d.sub_value = static_cast<uint8_t>((dw >> 8) & 0x0F);
    d.custom_made = (dw & 0x01) != 0;
    return d;
}

// ---------------------------------------------------------------------------
// Display formatting helpers
// ---------------------------------------------------------------------------

static const char* enchantment_name(enchantment_type t)
{
    switch (t)
    {
    case enchantment_type::critical_bonus:
        return "Critical";
    case enchantment_type::poison:
        return "Poisoning";
    case enchantment_type::righteous:
        return "Righteous";
    case enchantment_type::spell_on_hit:
        return "Spell on Hit";
    case enchantment_type::damage_reduction:
        return "Damage Reduction";
    case enchantment_type::light:
        return "Light";
    case enchantment_type::sharp:
        return "Sharp";
    case enchantment_type::fire:
        return "Fire";
    case enchantment_type::ancient:
        return "Ancient";
    case enchantment_type::magic_damage:
        return "Magic Damage";
    case enchantment_type::mana_conversion:
        return "Mana Conversion";
    case enchantment_type::charge_critical:
        return "Charge Critical";
    default:
        return nullptr;
    }
}

static const char* sub_enchantment_name(sub_enchantment_type t)
{
    switch (t)
    {
    case sub_enchantment_type::physical_resist:
        return "Phys Resist";
    case sub_enchantment_type::attack_rating:
        return "Attack Rating";
    case sub_enchantment_type::defense_rating:
        return "Defense Rating";
    case sub_enchantment_type::hp_recovery:
        return "HP Recovery";
    case sub_enchantment_type::sp_recovery:
        return "SP Recovery";
    case sub_enchantment_type::mp_recovery:
        return "MP Recovery";
    case sub_enchantment_type::magic_resist:
        return "Magic Resist";
    case sub_enchantment_type::physical_absorption:
        return "Phys Absorb";
    case sub_enchantment_type::magic_absorption:
        return "Magic Absorb";
    case sub_enchantment_type::critical_damage:
        return "Critical Damage";
    case sub_enchantment_type::exp_bonus:
        return "EXP Bonus";
    case sub_enchantment_type::gold_bonus:
        return "Gold Bonus";
    default:
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

sf::Color item_name_color(const item& itm)
{
    if (!itm.attribute.is_empty())
        return sf::Color(100, 255, 100); // Green for enchanted/upgraded
    if (itm.is_weapon() || itm.is_armor())
        return sf::Color(200, 200, 255); // Light blue for equipment
    return sf::Color::White;
}

std::vector<item_info_line> build_item_info(const item& itm)
{
    std::vector<item_info_line> lines;

    // Item name (with upgrade level prefix if applicable)
    const auto& attr = itm.attribute;
    if (attr.upgrade_level > 0)
        lines.push_back({std::format("{} (+{})", itm.name, attr.upgrade_level), item_name_color(itm)});
    else
        lines.push_back({itm.name, item_name_color(itm)});

    // Main enchantment
    if (attr.main_type != enchantment_type::none)
    {
        if (const char* name = enchantment_name(attr.main_type))
            lines.push_back({std::format("{} +{}", name, attr.main_value), sf::Color(255, 255, 100)});
    }

    // Sub enchantment
    if (attr.sub_type != sub_enchantment_type::none)
    {
        if (const char* name = sub_enchantment_name(attr.sub_type))
            lines.push_back({std::format("{} +{}", name, attr.sub_value), sf::Color(180, 220, 255)});
    }

    // Custom-made indicator
    if (attr.custom_made)
    {
        if (attr.custom_quality > 0)
            lines.push_back({std::format("Custom Made (Quality +{})", attr.custom_quality), sf::Color(255, 200, 100)});
        else if (attr.custom_quality < 0)
            lines.push_back(
                {std::format("Custom Made (Quality {})", attr.custom_quality), sf::Color(200, 150, 100)});
        else
            lines.push_back({"Custom Made", sf::Color(255, 200, 100)});
    }

    // Durability
    if (itm.max_durability > 0)
        lines.push_back({std::format("Endurance: {}", itm.durability), sf::Color(180, 180, 180)});

    return lines;
}

} // namespace hb
