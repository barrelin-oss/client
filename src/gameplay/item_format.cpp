#include "gameplay/item_format.hpp"
#include <format>

namespace hb
{

// ---------------------------------------------------------------------------
// item_attribute_data
// ---------------------------------------------------------------------------

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

std::vector<item_info_line> build_item_info(const item& itm, int32_t total_count)
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
        lines.push_back({"Custom Made", sf::Color(255, 200, 100)});

    // Durability (skip for single-endurance items)
    if (itm.max_durability > 1)
        lines.push_back({std::format("Endurance: {}", itm.durability), sf::Color(180, 180, 180)});

    // Total count across inventory
    if (total_count > 1)
        lines.push_back({std::format("Total: {}", total_count), sf::Color(180, 180, 180)});

    return lines;
}

} // namespace hb
