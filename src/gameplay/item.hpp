#pragma once

#include "core/game_enums.hpp"
#include <cstdint>
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

// Item attribute types
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

// Special item effect types (from m_dwAttribute encoding)
enum class item_effect_type : uint8_t
{
    none = 0,
    super_attack_bonus = 1,   // +N to super attack damage
    experience_bonus = 2,     // +N% experience gain
    gold_bonus = 3,           // +N% gold drops
    hp_recovery = 4,          // +N HP per tick
    mp_recovery = 5,          // +N MP per tick
    fire_resistance = 6,      // +N×4% fire resistance
    water_resistance = 7,     // +N×5% water resistance
    ice_resistance = 8,       // +N×7% ice resistance
    earth_resistance = 9,     // +N×3% earth resistance
    spell_accuracy = 10,      // +N×3% spell hit chance
    poison_resistance = 11,   // +N% poison resistance
    critical_bonus = 12,      // +N% critical damage
    physical_absorption = 13, // Absorb N% physical damage
    magic_absorption = 14,    // Absorb N% magic damage
    skill_bonus = 15,         // +N to specific skill
};

// Unpacked item attribute data
struct item_effect_data
{
    item_effect_type effect_type = item_effect_type::none;
    uint8_t effect_value = 0; // Effect magnitude
    uint8_t bonus_stat = 0;   // Bonus stat type (bits 28-31)
    uint8_t bonus_value = 0;  // Bonus stat value (bits 20-23)
    bool has_effect = false;

    // Unpack from 32-bit attribute value
    static item_effect_data from_attribute(uint32_t attr)
    {
        item_effect_data data;
        if (attr == 0)
            return data;

        data.has_effect = true;
        // Effect type: bits 16-19
        data.effect_type = static_cast<item_effect_type>((attr >> 16) & 0x0F);
        // Effect value: bits 8-15
        data.effect_value = static_cast<uint8_t>((attr >> 8) & 0xFF);
        // Bonus stat: bits 28-31
        data.bonus_stat = static_cast<uint8_t>((attr >> 28) & 0x0F);
        // Bonus value: bits 20-23
        data.bonus_value = static_cast<uint8_t>((attr >> 20) & 0x0F);

        return data;
    }

    // Calculate effect bonus based on type
    int32_t get_resistance_bonus() const
    {
        switch (effect_type)
        {
        case item_effect_type::fire_resistance:
            return effect_value * 4;
        case item_effect_type::water_resistance:
            return effect_value * 5;
        case item_effect_type::ice_resistance:
            return effect_value * 7;
        case item_effect_type::earth_resistance:
            return effect_value * 3;
        case item_effect_type::poison_resistance:
            return effect_value;
        default:
            return 0;
        }
    }

    int32_t get_damage_bonus() const
    {
        switch (effect_type)
        {
        case item_effect_type::super_attack_bonus:
            return effect_value;
        case item_effect_type::critical_bonus:
            return effect_value;
        default:
            return 0;
        }
    }

    int32_t get_spell_bonus() const
    {
        if (effect_type == item_effect_type::spell_accuracy)
        {
            return effect_value * 3;
        }
        return 0;
    }
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
    uint32_t id = 0;      // Unique instance ID
    uint16_t type_id = 0; // Item template type
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
    uint8_t color = 0;      // Item color variation
    uint32_t attribute = 0; // Encoded attribute flags

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

    // Get unpacked effect data from attribute
    item_effect_data get_effect_data() const { return item_effect_data::from_attribute(attribute); }

    // Check if item has a special effect
    bool has_special_effect() const { return attribute != 0; }

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
    uint16_t type_id;
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
