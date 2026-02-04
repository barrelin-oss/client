# Character Sounds Reference

This document provides comprehensive documentation for all 24 character sound effects (C1-C24) used in the Helbreath client.

## Overview

Character sounds are played during player actions such as attacking, moving, taking damage, dying, casting spells, eating, and leveling up. Sounds are spatially positioned with distance attenuation (0 tiles = full volume, >14 tiles = silent) and stereo panning based on screen position.

Sound files are located in `bin/assets/data/sounds/` as WAV files (C1.wav through C24.wav).

---

## Sound Reference Table

| Sound | File | Description | Trigger | Frame | Status |
|-------|------|-------------|---------|-------|--------|
| C1 | C1.wav | Small sword swing | Attack with weapon types 1-2 (two-hand) | 5 | Active |
| C2 | C2.wav | Large sword swing | Attack with weapon types 3-19 (swords) | 5 | Active |
| C3 | C3.wav | Bow attack | Attack with weapon types 40-59 (archery) | 5 | Active |
| C4 | C4.wav | Arrow/projectile release | Ranged attack projectile fired, Frost spell | 2 | Active |
| C5 | C5.wav | Punch (unarmed) | Unarmed attack (weapon type 0) | 5 | Unused |
| C6 | C6.wav | Sword impact | Melee damage dealt to target | - | Unused |
| C7 | C7.wav | Arrow impact | Ranged damage dealt to target | - | Unused |
| C8 | C8.wav | Walk footstep | Walking animation | 1, 3 | Active |
| C9 | C9.wav | Monster footstep | Monster walking animation | - | Unused |
| C10 | C10.wav | Run footstep | Running animation | varies | Active |
| C11 | C11.wav | Weapon impact / Rudolph | Rudolph attack, generic weapon impact | - | Active |
| C12 | C12.wav | Male hurt | Male player (types 1-3) takes damage | 5 | Active |
| C13 | C13.wav | Female hurt | Female player (types 4-6) takes damage | 5 | Active |
| C14 | C14.wav | Male death | Male player death animation | 7 | Active |
| C15 | C15.wav | Female death | Female player death animation | 7 | Active |
| C16 | C16.wav | Magic cast begin | Spell casting starts | 1 | Active |
| C17 | C17.wav | Magic failed | Spell casting fails/fizzles | - | Active |
| C18 | C18.wav | Mace swing | Attack with weapon types 20-39 (axes/maces) | 5 | Active |
| C19 | C19.wav | Female eat food | Female consumes food item | - | Active |
| C20 | C20.wav | Male eat food | Male consumes food item | - | Active |
| C21 | C21.wav | Male level up | Male gains level, skill, or contribution | - | Active |
| C22 | C22.wav | Female level up | Female gains level, skill, or contribution | - | Active |
| C23 | C23.wav | Male critical hit | Male super attack (m_sV3 >= 20) | 2 | Active |
| C24 | C24.wav | Female critical hit | Female super attack (m_sV3 >= 20) | 2 | Active |

---

## Detailed Sound Descriptions

### Combat Sounds

#### C1 - Small Sword Swing
- **File:** C1.wav
- **Trigger:** Attack action with weapon types 1-2 (two-handed weapons)
- **Animation Frame:** 5
- **Legacy Code:** `DrawObject_OnAttack()` - `m_Misc.bPlaySound('C', 1, ...)`

#### C2 - Large Sword Swing
- **File:** C2.wav
- **Trigger:** Attack action with weapon types 3-19 (swords)
- **Animation Frame:** 5
- **Legacy Code:** `DrawObject_OnAttack()` - `m_Misc.bPlaySound('C', 2, ...)`

#### C3 - Bow Attack
- **File:** C3.wav
- **Trigger:** Attack action with weapon types 40-59 (bows/crossbows)
- **Animation Frame:** 5
- **Legacy Code:** `DrawObject_OnAttack()` - `m_Misc.bPlaySound('C', 3, ...)`

#### C4 - Arrow/Projectile Release
- **File:** C4.wav
- **Trigger:** Ranged attack projectile is fired; also used for Frost spell effects
- **Animation Frame:** 2
- **Legacy Code:** Ranged attack handlers, frost spell

#### C5 - Punch (Unarmed)
- **File:** C5.wav
- **Trigger:** Unarmed attack (weapon type 0)
- **Animation Frame:** 5
- **Status:** Currently unused in modern client
- **Notes:** Can be re-enabled for fist/brawler combat

#### C6 - Sword Impact
- **File:** C6.wav
- **Trigger:** Melee weapon connects with target
- **Status:** Currently unused
- **Notes:** Was intended for impact confirmation on melee hits

#### C7 - Arrow Impact
- **File:** C7.wav
- **Trigger:** Ranged projectile hits target
- **Status:** Currently unused
- **Notes:** Was intended for projectile hit confirmation

#### C18 - Mace Swing
- **File:** C18.wav
- **Trigger:** Attack action with weapon types 20-39 (axes, hammers, maces)
- **Animation Frame:** 5
- **Legacy Code:** `DrawObject_OnAttack()` - `m_Misc.bPlaySound('C', 18, ...)`

### Movement Sounds

#### C8 - Walk Footstep
- **File:** C8.wav
- **Trigger:** Walking animation (non-running movement)
- **Animation Frames:** 1 and 3 (alternating footsteps)
- **Legacy Code:** `DrawObject_OnMove()` - frame-based playback

#### C9 - Monster Footstep
- **File:** C9.wav
- **Trigger:** Monster walking animation
- **Status:** Currently unused
- **Notes:** Originally for heavy monster footsteps

#### C10 - Run Footstep
- **File:** C10.wav
- **Trigger:** Running animation
- **Animation Frames:** Varies based on run cycle
- **Legacy Code:** `DrawObject_OnRun()` - frame-based playback

### Damage/Death Sounds

#### C11 - Weapon Impact / Rudolph
- **File:** C11.wav
- **Trigger:** Rudolph (special mount) attack; generic weapon impact sound
- **Notes:** Multi-purpose impact sound

#### C12 - Male Hurt
- **File:** C12.wav
- **Trigger:** Male player (types 1-3) takes damage
- **Animation Frame:** 5 of damage animation
- **Player Types:** 1 = Male Human, 2 = Male Elf, 3 = Male Dark Elf
- **Legacy Code:** `DrawObject_OnDamage()` - gender check via player type

#### C13 - Female Hurt
- **File:** C13.wav
- **Trigger:** Female player (types 4-6) takes damage
- **Animation Frame:** 5 of damage animation
- **Player Types:** 4 = Female Human, 5 = Female Elf, 6 = Female Dark Elf
- **Legacy Code:** `DrawObject_OnDamage()` - gender check via player type

#### C14 - Male Death
- **File:** C14.wav
- **Trigger:** Male player death animation
- **Animation Frame:** 7 of dying animation
- **Player Types:** 1-3 (male characters)
- **Legacy Code:** `DrawObject_OnDying()` - gender check via player type

#### C15 - Female Death
- **File:** C15.wav
- **Trigger:** Female player death animation
- **Animation Frame:** 7 of dying animation
- **Player Types:** 4-6 (female characters)
- **Legacy Code:** `DrawObject_OnDying()` - gender check via player type

### Magic Sounds

#### C16 - Magic Cast Begin
- **File:** C16.wav
- **Trigger:** Spell casting begins (start of magic animation)
- **Animation Frame:** 1
- **Legacy Code:** `DrawObject_OnMagic()` - frame 1

#### C17 - Magic Failed
- **File:** C17.wav
- **Trigger:** Spell casting fails/fizzles (insufficient MP, interrupted, etc.)
- **Notes:** Plays when spell is interrupted or fails to cast

### Consumable Sounds

#### C19 - Female Eat Food
- **File:** C19.wav
- **Trigger:** Female character consumes a food item (DEF_ITEMTYPE_EAT)
- **Player Types:** 4-6 (female characters)

#### C20 - Male Eat Food
- **File:** C20.wav
- **Trigger:** Male character consumes a food item (DEF_ITEMTYPE_EAT)
- **Player Types:** 1-3 (male characters)

### Level/Achievement Sounds

#### C21 - Male Level Up
- **File:** C21.wav
- **Trigger:** Male character gains a level, completes skill training, or gains contribution
- **Player Types:** 1-3 (male characters)

#### C22 - Female Level Up
- **File:** C22.wav
- **Trigger:** Female character gains a level, completes skill training, or gains contribution
- **Player Types:** 4-6 (female characters)

### Critical Hit Sounds

#### C23 - Male Critical Hit
- **File:** C23.wav
- **Trigger:** Male character performs a super attack (m_sV3 >= 20)
- **Animation Frame:** 2
- **Player Types:** 1-3 (male characters)
- **Notes:** Super attacks are unlocked at 100% weapon mastery

#### C24 - Female Critical Hit
- **File:** C24.wav
- **Trigger:** Female character performs a super attack (m_sV3 >= 20)
- **Animation Frame:** 2
- **Player Types:** 4-6 (female characters)
- **Notes:** Super attacks are unlocked at 100% weapon mastery

---

## Gender Detection

Player types determine gender for gendered sounds:

| Player Type | Gender | Race |
|-------------|--------|------|
| 1 | Male | Human |
| 2 | Male | Elf |
| 3 | Male | Dark Elf |
| 4 | Female | Human |
| 5 | Female | Elf |
| 6 | Female | Dark Elf |

**Helper function:**
```cpp
bool is_male(int player_type) { return player_type >= 1 && player_type <= 3; }
bool is_female(int player_type) { return player_type >= 4 && player_type <= 6; }
```

---

## Weapon Type Categories

Weapon types determine which swing sound to play:

| Weapon Type Range | Category | Sound |
|-------------------|----------|-------|
| 0 | Unarmed (fist) | C5 |
| 1-2 | Two-handed | C1 |
| 3-19 | Swords | C2 |
| 20-39 | Axes/Maces/Hammers | C18 |
| 40-59 | Bows/Crossbows | C3 |

---

## Integration Points

### Combat System (`combat.cpp`)
- **Weapon swings:** Trigger on attack start based on weapon type
- **Critical hits:** Trigger C23/C24 on super attacks
- **Damage dealt:** Trigger C12/C13 when player takes damage
- **Death:** Trigger C14/C15 on player death

### Entity Animation (`entity.cpp`)
- **Walking:** Trigger C8 on frames 1 and 3 of walk animation
- **Running:** Trigger C10 on run animation frames
- **Dying:** Trigger death sound on frame 7

### Magic System
- **Cast begin:** Trigger C16 on spell cast start
- **Cast fail:** Trigger C17 on spell failure

### Item Usage
- **Eat food:** Trigger C19/C20 when consuming food items

### Level Events
- **Level up:** Trigger C21/C22 on level gain
- **Skill completion:** Same sounds on skill mastery

---

## API Usage

### Using character_sound Enum

```cpp
#include "audio/sound_types.hpp"
#include "audio/sound_manager.hpp"

// Play weapon swing sound based on weapon type
auto swing = get_weapon_swing_sound(weapon_type);
sound_manager.play_character_sound(swing, tile_distance, pan);

// Play gendered hurt sound
auto hurt = get_hurt_sound(player_type);
sound_manager.play_character_sound_at(hurt, world_x, world_y);

// Play level up sound
auto levelup = get_level_up_sound(player_type);
sound_manager.play_character_sound(levelup, 0);  // Full volume, centered
```

### Direct Playback

```cpp
// Using raw character/number (legacy style)
sound_manager.play_sound('C', 12, tile_distance, pan);

// Using position-based playback
sound_manager.play_sound_at('C', 14, world_x, world_y);
```

---

## Distance Attenuation

Sounds are attenuated based on tile distance from the listener:
- **0 tiles:** 100% volume
- **7 tiles:** 50% volume
- **14 tiles:** 0% volume (silent)
- **>14 tiles:** Not played

Formula: `volume = 1.0 - (tile_distance / 14.0)`

---

## Stereo Panning

Sounds are panned based on horizontal offset from screen center:
- **-1.0:** Full left
- **0.0:** Center
- **+1.0:** Full right

Formula: `pan = clamp(dx / 14.0, -1.0, 1.0)` where `dx` is tile offset from listener
