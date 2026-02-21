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
    common = 0,
    uncommon = 1,
    rare = 2,
    epic = 3,
    legendary = 4,
    unique = 5,
};

// Item attribute types (stat bonuses on items)
enum class item_attribute : uint8_t
{
    none = 0,
    strength = 1,
    vitality = 2,
    dexterity = 3,
    intelligence = 4,
    magic = 5,
    charisma = 6,
    hp = 7,
    mp = 8,
    sp = 9,
    attack = 10,
    defense = 11,
    magic_resist = 12,
    hit_rate = 13,
    dodge_rate = 14,
    critical_rate = 15,
    attack_speed = 16,
    move_speed = 17,
};

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
    fire = 8,
    ancient = 9,
    magic_damage = 10,
    mana_conversion = 11,
    charge_critical = 12,
};

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

// Unpacked item attribute data (matches server item_attribute struct)
struct item_attribute_data
{
    uint8_t upgrade_level = 0;
    enchantment_type main_type = enchantment_type::none;
    uint8_t main_value = 0;
    sub_enchantment_type sub_type = sub_enchantment_type::none;
    uint8_t sub_value = 0;
    bool custom_made = false;
    int8_t custom_quality = 0;

    bool is_empty() const;
    static item_attribute_data from_json(const nlohmann::json& j);
    static item_attribute_data from_legacy(uint32_t dw);
};

// Item attribute bonus
struct item_bonus
{
    item_attribute attribute = item_attribute::none;
    int16_t value = 0;
};

// Complete item definition
struct item
{
    // Identification
    uint32_t id = 0;          // Unique instance ID
    uint32_t template_id = 0; // Item template for sprite/stat lookup
    std::string name;

    // Classification
    item_type type = item_type::none;
    equip_slot slot = equip_slot::none;
    item_rarity rarity = item_rarity::common;

    // Requirements
    uint16_t level_req = 0;
    uint16_t strength_req = 0;
    uint16_t dexterity_req = 0;
    uint16_t intelligence_req = 0;
    uint16_t magic_req = 0;

    // Properties
    uint32_t amount = 1;
    uint32_t max_stack = 1;
    uint16_t durability = 100;
    uint16_t max_durability = 100;
    uint32_t weight = 1;
    uint32_t price = 0;

    // Combat stats
    int32_t damage_min = 0;
    int32_t damage_max = 0;
    int32_t defense = 0;
    int32_t magic_defense = 0;
    float attack_speed = 1.0f;

    // Bonuses (up to 6)
    static constexpr size_t max_bonuses = 6;
    item_bonus bonuses[max_bonuses] = {};
    uint8_t bonus_count = 0;

    // Sprite info
    uint16_t sprite_id = 0;
    uint16_t equipped_sprite_id = 0;

    // Visual/Network properties
    uint8_t color = 0;              // Item color variation
    item_attribute_data attribute{}; // Enchantment/upgrade data

    // Flags
    bool tradeable = true;
    bool droppable = true;
    bool bound = false;
    bool identified = true;

    // Helper methods
    bool is_weapon() const { return slot == equip_slot::right_hand || slot == equip_slot::two_hand; }
    bool is_armor() const
    {
        return slot == equip_slot::body || slot == equip_slot::pants || slot == equip_slot::boots ||
               slot == equip_slot::arms;
    }
    bool is_accessory() const
    {
        return slot == equip_slot::neck || slot == equip_slot::left_finger || slot == equip_slot::right_finger;
    }
    bool is_stackable() const { return max_stack > 1; }
    bool is_damaged() const { return durability < max_durability; }

    // Get total bonus for an attribute
    int32_t get_bonus(item_attribute attr) const
    {
        int32_t total = 0;
        for (uint8_t i = 0; i < bonus_count && i < max_bonuses; ++i)
        {
            if (bonuses[i].attribute == attr)
            {
                total += bonuses[i].value;
            }
        }
        return total;
    }

    // Check if item has a special effect (enchantment, upgrade, custom-made)
    bool has_special_effect() const { return !attribute.is_empty(); }

    // Add a bonus to the item
    bool add_bonus(item_attribute attr, int16_t value)
    {
        if (bonus_count >= max_bonuses)
            return false;
        bonuses[bonus_count].attribute = attr;
        bonuses[bonus_count].value = value;
        bonus_count++;
        return true;
    }

    // Calculate random damage for weapons
    int32_t roll_damage() const
    {
        if (damage_min >= damage_max)
            return damage_min;
        // Would use random here
        return (damage_min + damage_max) / 2;
    }
};

// Item template (static data)
struct item_template
{
    uint32_t template_id;
    std::string name;
    item_type type;
    equip_slot slot;
    item_rarity base_rarity;

    uint16_t level_req;
    uint16_t strength_req;
    uint16_t dexterity_req;
    uint16_t intelligence_req;
    uint16_t magic_req;

    uint32_t max_stack;
    uint16_t max_durability;
    uint32_t weight;
    uint32_t base_price;

    int32_t damage_min;
    int32_t damage_max;
    int32_t defense;
    int32_t magic_defense;
    float attack_speed;

    uint16_t sprite_id;
    uint16_t equipped_sprite_id;
};

} // namespace hb
