#pragma once

#include "core/game_enums.hpp"
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

namespace hb
{

// Item rarity
enum class item_rarity : uint8_t
{
    none = 0,
    common,
    uncommon,
    rare,
    epic,
    legendary,
};

inline constexpr std::string_view item_rarity_to_string(item_rarity r)
{
    switch (r)
    {
    case item_rarity::none:      return "none";
    case item_rarity::common:    return "common";
    case item_rarity::uncommon:  return "uncommon";
    case item_rarity::rare:      return "rare";
    case item_rarity::epic:      return "epic";
    case item_rarity::legendary: return "legendary";
    }
    return "none";
}

inline constexpr item_rarity item_rarity_from_string(std::string_view s)
{
    if (s == "common")    return item_rarity::common;
    if (s == "uncommon")  return item_rarity::uncommon;
    if (s == "rare")      return item_rarity::rare;
    if (s == "epic")      return item_rarity::epic;
    if (s == "legendary") return item_rarity::legendary;
    return item_rarity::none;
}

// Effect types (stat bonuses on items)
enum class effect_type : uint8_t
{
    none = 0,
    str_bonus,
    dex_bonus,
    int_bonus,
    mag_bonus,
    vit_bonus,
    chr_bonus,
    hp_bonus,
    mp_bonus,
    sp_bonus,
    hit_bonus,
    dodge_bonus,
};

inline constexpr std::string_view effect_type_to_string(effect_type t)
{
    switch (t)
    {
    case effect_type::none:        return "none";
    case effect_type::str_bonus:   return "str_bonus";
    case effect_type::dex_bonus:   return "dex_bonus";
    case effect_type::int_bonus:   return "int_bonus";
    case effect_type::mag_bonus:   return "mag_bonus";
    case effect_type::vit_bonus:   return "vit_bonus";
    case effect_type::chr_bonus:   return "chr_bonus";
    case effect_type::hp_bonus:    return "hp_bonus";
    case effect_type::mp_bonus:    return "mp_bonus";
    case effect_type::sp_bonus:    return "sp_bonus";
    case effect_type::hit_bonus:   return "hit_bonus";
    case effect_type::dodge_bonus: return "dodge_bonus";
    }
    return "none";
}

inline constexpr effect_type effect_type_from_string(std::string_view s)
{
    if (s == "str_bonus")   return effect_type::str_bonus;
    if (s == "dex_bonus")   return effect_type::dex_bonus;
    if (s == "int_bonus")   return effect_type::int_bonus;
    if (s == "mag_bonus")   return effect_type::mag_bonus;
    if (s == "vit_bonus")   return effect_type::vit_bonus;
    if (s == "chr_bonus")   return effect_type::chr_bonus;
    if (s == "hp_bonus")    return effect_type::hp_bonus;
    if (s == "mp_bonus")    return effect_type::mp_bonus;
    if (s == "sp_bonus")    return effect_type::sp_bonus;
    if (s == "hit_bonus")   return effect_type::hit_bonus;
    if (s == "dodge_bonus") return effect_type::dodge_bonus;
    return effect_type::none;
}

// Main enchantment types (from server item_attribute.main_type)
enum class enchantment_type : uint8_t
{
    none = 0,
    critical_bonus = 1,
    poison = 2,
    righteous = 3,
    spell_on_hit = 4,
    damage_reduction = 5,
    light = 6,
    sharp = 7,
    ancient = 8,
    magic_damage = 9,
    mana_conversion = 10,
    charge_critical = 11,
};

inline constexpr std::string_view enchantment_type_to_string(enchantment_type t)
{
    switch (t)
    {
    case enchantment_type::none:             return "none";
    case enchantment_type::critical_bonus:   return "critical_bonus";
    case enchantment_type::poison:           return "poison";
    case enchantment_type::righteous:        return "righteous";
    case enchantment_type::spell_on_hit:     return "spell_on_hit";
    case enchantment_type::damage_reduction: return "damage_reduction";
    case enchantment_type::light:            return "light";
    case enchantment_type::sharp:            return "sharp";
    case enchantment_type::ancient:          return "ancient";
    case enchantment_type::magic_damage:     return "magic_damage";
    case enchantment_type::mana_conversion:  return "mana_conversion";
    case enchantment_type::charge_critical:  return "charge_critical";
    }
    return "none";
}

inline constexpr enchantment_type enchantment_type_from_string(std::string_view s)
{
    if (s == "critical_bonus")   return enchantment_type::critical_bonus;
    if (s == "poison")           return enchantment_type::poison;
    if (s == "righteous")        return enchantment_type::righteous;
    if (s == "spell_on_hit")     return enchantment_type::spell_on_hit;
    if (s == "damage_reduction") return enchantment_type::damage_reduction;
    if (s == "light")            return enchantment_type::light;
    if (s == "sharp")            return enchantment_type::sharp;
    if (s == "ancient")          return enchantment_type::ancient;
    if (s == "magic_damage")     return enchantment_type::magic_damage;
    if (s == "mana_conversion")  return enchantment_type::mana_conversion;
    if (s == "charge_critical")  return enchantment_type::charge_critical;
    return enchantment_type::none;
}

// Sub-enchantment types (from server item_attribute.sub_type)
enum class sub_enchantment_type : uint8_t
{
    none = 0,
    physical_resist = 1,
    attack_rating = 2,
    defense_rating = 3,
    hp_recovery = 4,
    sp_recovery = 5,
    mp_recovery = 6,
    magic_resist = 7,
    physical_absorption = 8,
    magic_absorption = 9,
    critical_damage = 10,
    exp_bonus = 11,
    gold_bonus = 12,
};

inline constexpr std::string_view sub_enchantment_type_to_string(sub_enchantment_type t)
{
    switch (t)
    {
    case sub_enchantment_type::none:                return "none";
    case sub_enchantment_type::physical_resist:     return "physical_resist";
    case sub_enchantment_type::attack_rating:       return "attack_rating";
    case sub_enchantment_type::defense_rating:      return "defense_rating";
    case sub_enchantment_type::hp_recovery:         return "hp_recovery";
    case sub_enchantment_type::sp_recovery:         return "sp_recovery";
    case sub_enchantment_type::mp_recovery:         return "mp_recovery";
    case sub_enchantment_type::magic_resist:        return "magic_resist";
    case sub_enchantment_type::physical_absorption: return "physical_absorption";
    case sub_enchantment_type::magic_absorption:    return "magic_absorption";
    case sub_enchantment_type::critical_damage:     return "critical_damage";
    case sub_enchantment_type::exp_bonus:           return "exp_bonus";
    case sub_enchantment_type::gold_bonus:          return "gold_bonus";
    }
    return "none";
}

inline constexpr sub_enchantment_type sub_enchantment_type_from_string(std::string_view s)
{
    if (s == "physical_resist")     return sub_enchantment_type::physical_resist;
    if (s == "attack_rating")       return sub_enchantment_type::attack_rating;
    if (s == "defense_rating")      return sub_enchantment_type::defense_rating;
    if (s == "hp_recovery")         return sub_enchantment_type::hp_recovery;
    if (s == "sp_recovery")         return sub_enchantment_type::sp_recovery;
    if (s == "mp_recovery")         return sub_enchantment_type::mp_recovery;
    if (s == "magic_resist")        return sub_enchantment_type::magic_resist;
    if (s == "physical_absorption") return sub_enchantment_type::physical_absorption;
    if (s == "magic_absorption")    return sub_enchantment_type::magic_absorption;
    if (s == "critical_damage")     return sub_enchantment_type::critical_damage;
    if (s == "exp_bonus")           return sub_enchantment_type::exp_bonus;
    if (s == "gold_bonus")          return sub_enchantment_type::gold_bonus;
    return sub_enchantment_type::none;
}

// Unpacked item attribute data (matches server item_attribute struct)
struct item_attribute_data
{
    uint8_t upgrade_level = 0;
    enchantment_type main_type = enchantment_type::none;
    uint8_t main_value = 0;
    sub_enchantment_type sub_type = sub_enchantment_type::none;
    uint8_t sub_value = 0;
    bool custom_made = false;

    bool is_empty() const;
    static item_attribute_data from_json(const nlohmann::json& j);
};

// Item effect (stat bonus on an item)
struct item_effect
{
    effect_type type = effect_type::none;
    int16_t value = 0;
};

// Complete item definition
struct item
{
    // Identity
    uint32_t item_id = 0;
    uint32_t template_id = 0;
    std::string name;
    std::string description; // tooltip text from the server (may be empty)

    // Classification
    item_type type = item_type::none;
    equip_pos equip_position = equip_pos::none;
    weapon_type weapon = weapon_type::none;
    item_rarity rarity = item_rarity::none;

    // Stack / weight / price
    uint32_t count = 1;
    uint32_t weight = 0;
    uint32_t price = 0;

    // Combat stats
    int32_t damage_min = 0;
    int32_t damage_max = 0;
    int32_t defense = 0;
    int32_t magic_defense = 0;

    // Durability
    uint16_t durability = 0;
    uint16_t max_durability = 0;

    // Requirements
    uint16_t level_req = 0;
    uint16_t str_req = 0;
    uint16_t dex_req = 0;
    uint16_t int_req = 0;
    uint16_t mag_req = 0;

    // Effects (up to 6 stat bonuses)
    static constexpr size_t max_effects = 6;
    item_effect effects[max_effects] = {};
    uint8_t effect_count = 0;

    // Attribute (upgrade / enchantment)
    item_attribute_data attribute{};

    // Special ability (empty = none)
    std::string special_ability;

    // Visual
    uint8_t color = 0;
    uint16_t sprite_id = 0;
    uint16_t sprite_frame = 0;

    // Flags
    bool tradeable = true;
    bool droppable = true;
    uint32_t bound_to = 0;
    bool two_handed = false;

    // Helpers
    bool is_weapon() const { return equip_position == equip_pos::weapon || equip_position == equip_pos::twohand; }
    bool is_armor() const
    {
        return equip_position == equip_pos::body || equip_position == equip_pos::pants ||
               equip_position == equip_pos::boots || equip_position == equip_pos::arms ||
               equip_position == equip_pos::full_body;
    }
    bool is_accessory() const
    {
        return equip_position == equip_pos::ring_left || equip_position == equip_pos::ring_right ||
               equip_position == equip_pos::amulet || equip_position == equip_pos::cape ||
               equip_position == equip_pos::angel;
    }
    bool is_equippable() const { return equip_position != equip_pos::none; }
    bool is_damaged() const { return max_durability > 0 && durability < max_durability; }
    bool has_special_effect() const { return !attribute.is_empty(); }
    bool is_bound() const { return bound_to != 0; }
    int32_t get_effect(effect_type t) const
    {
        int32_t total = 0;
        for (uint8_t i = 0; i < effect_count && i < max_effects; ++i)
        {
            if (effects[i].type == t)
                total += effects[i].value;
        }
        return total;
    }

    // JSON parsing
    static item from_json(const nlohmann::json& j);
};

} // namespace hb
