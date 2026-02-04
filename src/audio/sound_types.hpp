#pragma once

#include <cstdint>

namespace hb
{

// Character sound effects (C1-C24)
// See docs/character_sounds.md for detailed descriptions
enum class character_sound : uint8_t
{
    small_sword_swing = 1,   // C1 - two-hand weapons (types 1-2)
    large_sword_swing = 2,   // C2 - swords (types 3-19)
    bow_attack = 3,          // C3 - archery (types 40-59)
    arrow_release = 4,       // C4 - projectile fired
    punch = 5,               // C5 - unarmed (currently unused)
    sword_impact = 6,        // C6 - melee damage (currently unused)
    arrow_impact = 7,        // C7 - ranged damage (currently unused)
    walk_step = 8,           // C8 - walking footstep
    monster_step = 9,        // C9 - monster walk (currently unused)
    run_step = 10,           // C10 - running footstep
    weapon_impact = 11,      // C11 - weapon hit / Rudolph
    male_hurt = 12,          // C12 - male takes damage
    female_hurt = 13,        // C13 - female takes damage
    male_death = 14,         // C14 - male dies
    female_death = 15,       // C15 - female dies
    magic_cast = 16,         // C16 - spell begins
    magic_failed = 17,       // C17 - spell fails
    mace_swing = 18,         // C18 - axes/maces (types 20-39)
    female_eat = 19,         // C19 - female consumes food
    male_eat = 20,           // C20 - male consumes food
    male_level_up = 21,      // C21 - male level/skill up
    female_level_up = 22,    // C22 - female level/skill up
    male_critical = 23,      // C23 - male super attack
    female_critical = 24,    // C24 - female super attack
};

// Gender detection helpers
// Player types 1-3 are male, 4-6 are female
inline bool is_male_player_type(int player_type)
{
    return player_type >= 1 && player_type <= 3;
}

inline bool is_female_player_type(int player_type)
{
    return player_type >= 4 && player_type <= 6;
}

// Get hurt sound based on player type (gender)
inline character_sound get_hurt_sound(int player_type)
{
    return is_male_player_type(player_type)
        ? character_sound::male_hurt
        : character_sound::female_hurt;
}

// Get death sound based on player type (gender)
inline character_sound get_death_sound(int player_type)
{
    return is_male_player_type(player_type)
        ? character_sound::male_death
        : character_sound::female_death;
}

// Get level up sound based on player type (gender)
inline character_sound get_level_up_sound(int player_type)
{
    return is_male_player_type(player_type)
        ? character_sound::male_level_up
        : character_sound::female_level_up;
}

// Get eat/consume food sound based on player type (gender)
inline character_sound get_eat_sound(int player_type)
{
    return is_male_player_type(player_type)
        ? character_sound::male_eat
        : character_sound::female_eat;
}

// Get critical hit / super attack sound based on player type (gender)
inline character_sound get_critical_sound(int player_type)
{
    return is_male_player_type(player_type)
        ? character_sound::male_critical
        : character_sound::female_critical;
}

// Get weapon swing sound based on weapon type
// Weapon type ranges:
//   0      - unarmed (fist)
//   1-2    - two-hand weapons (small sword swing)
//   3-19   - swords (large sword swing)
//   20-39  - axes/maces/hammers (mace swing)
//   40-59  - bows/crossbows (bow attack)
inline character_sound get_weapon_swing_sound(int weapon_type)
{
    if (weapon_type == 0)
    {
        return character_sound::punch;
    }
    if (weapon_type >= 1 && weapon_type <= 2)
    {
        return character_sound::small_sword_swing;
    }
    if (weapon_type >= 3 && weapon_type <= 19)
    {
        return character_sound::large_sword_swing;
    }
    if (weapon_type >= 20 && weapon_type <= 39)
    {
        return character_sound::mace_swing;
    }
    if (weapon_type >= 40 && weapon_type <= 59)
    {
        return character_sound::bow_attack;
    }
    // Fallback for unknown weapon types
    return character_sound::punch;
}

// Check if weapon type is ranged (bow/crossbow)
inline bool is_ranged_weapon(int weapon_type)
{
    return weapon_type >= 40 && weapon_type <= 59;
}

} // namespace hb
