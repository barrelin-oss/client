# Skill System

## Overview

The Helbreath skill system is a player progression mechanic that tracks mastery levels across 60 different skills. Skills are organized into implicit categories (combat, magic, crafting, gathering, miscellaneous) and provide passive bonuses or active abilities. Each skill has an experience value internally tracked as 0-100% mastery displayed to the player.

The skill system integrates deeply with:
- **Combat** - Weapon mastery affects damage and unlocks super attacks
- **Magic** - Magic skill affects spell effectiveness and resistance
- **Crafting** - Manufacturing skill determines craftable items
- **Items** - Some items require specific skill levels to use

## Source Files

| File | Purpose |
|------|---------|
| `Skill.h` | CSkill class definition |
| `Skill.cpp` | CSkill constructor/destructor |
| `Game.h` | Skill arrays and state variables in CGame |
| `Game.cpp` | Skill loading, UI, networking, and usage logic |
| `skillcfg.txt` | Skill definitions configuration file |

## Key Data Structures

### CSkill Class

```cpp
// Skill.h
class CSkill {
public:
    CSkill();
    virtual ~CSkill();

    char m_cName[21];       // Skill name (max 20 characters + null)
    int  m_iLevel;          // Current mastery level (0-100)
    BOOL m_bIsUseable;      // TRUE if skill can be actively used
    char m_cUseMethod;      // Activation method (0, 1, or 2)
};
```

**Field Details:**
- `m_cName` - Display name loaded from skillcfg.txt
- `m_iLevel` - Synchronized with `m_cSkillMastery[]` array
- `m_bIsUseable` - FALSE for passive skills, TRUE for active skills
- `m_cUseMethod` - Determines how skill is activated:
  - `0` = Direct activation (no targeting/items needed)
  - `1` = Item-based activation (requires specific item)
  - `2` = Dialog-based activation (opens crafting dialog)

### CGame Skill Members

```cpp
// Game.h (within CGame class)
CSkill* m_pSkillCfgList[60];        // Skill definition pointers
unsigned char m_cSkillMastery[60];  // Quick mastery level lookup (0-100)

// Skill usage state
BOOL m_bSkillUsingStatus;           // TRUE while using an active skill
int  m_iDownSkillIndex;             // Currently hotkeyed skill (-1 = none)

// Super attack tracking
int  m_iSuperAttackLeft;            // Remaining super attacks available
BOOL m_bSuperAttackMode;            // TRUE when super attack mode active
```

## Skill Definitions

### Configuration File Format

**File:** `skillcfg.txt`

```
skill = [ID]  [Name]           [IsUseable]  [UseMethod]
```

### Complete Skill List

| ID | Name | Useable | Method | Category | Description |
|----|------|---------|--------|----------|-------------|
| 0 | Mining | 0 | 1 | Gathering | Ore/gem extraction efficiency |
| 1 | Fishing | 0 | 1 | Gathering | Fish catch rate |
| 2 | Farming | 0 | 1 | Gathering | Crop harvesting |
| 3 | Magic-Resistance | 0 | 0 | Magic | Reduces magic damage taken |
| 4 | Magic | 0 | 0 | Magic | Spell effectiveness and mana |
| 5 | Hand-Attack | 0 | 0 | Combat | Unarmed combat damage |
| 6 | Archery | 0 | 0 | Combat | Bow/crossbow proficiency |
| 7 | Short-Sword | 0 | 0 | Combat | Daggers/short blades |
| 8 | Long-Sword | 0 | 0 | Combat | Swords/long blades |
| 9 | Fencing | 0 | 0 | Combat | Rapiers/spears |
| 10 | Axe-Attack | 0 | 0 | Combat | Axes |
| 11 | Shield | 0 | 0 | Combat | Shield blocking |
| 12 | Alchemy | 0 | 0 | Crafting | Potion creation |
| 13 | Manufacturing | 0 | 2 | Crafting | Item crafting (opens dialog) |
| 14 | Hammer | 0 | 0 | Combat | Hammers/maces |
| 15 | ???? | 1 | 1 | Unknown | Undocumented |
| 16 | Crafting | 0 | 1 | Crafting | General crafting bonus |
| 17 | ???? | 1 | 0 | Unknown | Undocumented |
| 18 | ???? | 1 | 1 | Unknown | Undocumented |
| 19 | Pretend-Corpse | 1 | 0 | Misc | Play dead to avoid enemies |
| 20 | ???? | 0 | 0 | Unknown | Undocumented |
| 21 | Staff-Attack | 0 | 0 | Combat | Staves/rods |
| 22 | ???? | 1 | 2 | Unknown | Undocumented |
| 23 | Poison-Resistance | 0 | 0 | Misc | Reduces poison damage |
| 24-59 | (Various) | - | - | - | Additional skills (server-defined) |

**Note:** Skills 15, 17, 18, 20, and 22 are marked "????" in the config file. These may be region-specific, unimplemented, or reserved for special events.

## Core Functions

### Initialization

```cpp
// Game.cpp:8521
BOOL CGame::bInitSkillCfgList()
```

Loads skill definitions from `skillcfg.txt`:
1. Opens configuration file
2. Parses each `skill = ...` line
3. Creates CSkill objects in `m_pSkillCfgList[]`
4. Sets name, useable flag, and use method

### Weapon Skill Type Detection

```cpp
// Game.cpp:23769
int CGame::_iGetWeaponSkillType()
```

Maps currently equipped weapon to its corresponding skill ID:

```cpp
WORD wWeaponType = ((m_sPlayerAppr2 & 0x0FF0) >> 4);

if (wWeaponType == 0)           return 5;   // Hand-Attack (unarmed)
else if (wWeaponType <= 2)      return 7;   // Short-Sword
else if (wWeaponType <= 19) {
    if (wWeaponType == 7)       return 9;   // Axe-Attack (specific type)
    else                        return 8;   // Long-Sword (default melee)
}
else if (wWeaponType <= 29)     return 10;  // Shield (shield bash)
else if (wWeaponType <= 34)     return 14;  // Hammer
else if (wWeaponType <= 39)     return 21;  // Staff-Attack
else if (wWeaponType >= 40)     return 6;   // Archery (ranged)

return 1; // Fallback
```

### Skill Dialog Drawing

```cpp
// Game.cpp:41134
void CGame::DrawDialogBox_Skill()
```

Renders the skill list dialog (dialog index 15):
- Displays up to 17 skills per screen with scrolling
- Shows skill name and mastery percentage
- Color coding:
  - **Blue (34,30,120)** - Active useable skill with level > 0
  - **White** - Hovered skill
  - **Dark gray (5,5,5)** - Inactive or level 0 skill
- Highlights currently hotkeyed skill with overlay

### Manufacturing Dialog

```cpp
// Game.cpp:41233
void CGame::DrawDialogBox_SkillDlg()
```

Renders the crafting/manufacturing dialog (dialog index 26):
- Mode 1: Item selection with animation
- Mode 2-6: Crafting animation and result display
- Checks `m_cSkillMastery[13]` against item's skill requirement

### Skill Click Handler

```cpp
// Game.cpp:42843
void CGame::DlgBoxClick_Skill()
```

Handles clicks in the skill dialog:
1. Validates skill is useable (`m_bIsUseable == TRUE`)
2. Checks skill level > 0
3. Verifies no other skill in use (`m_bSkillUsingStatus == FALSE`)
4. Sends skill use request to server
5. Sets `m_bSkillUsingStatus = TRUE`

### Skill Usage Clearing

```cpp
// Game.cpp:34184
void CGame::ClearSkillUsingStatus()
```

Resets skill usage state after completion or failure:
- Sets `m_bSkillUsingStatus = FALSE`
- Re-enables skill dialog interaction

## Constants & Limits

```cpp
#define DEF_MAXSKILLTYPE    60      // Maximum skill types

// Mastery range
// 0 = untrained
// 100 = fully mastered

// Dialog constants
#define SKILL_DIALOG_INDEX  15      // Skill list dialog
#define SKILLDLG_DIALOG_INDEX 26    // Manufacturing dialog
#define VISIBLE_SKILLS      17      // Skills shown per screen
```

## Integration Points

### Combat System

**Weapon Mastery:**
- Each weapon type maps to a skill via `_iGetWeaponSkillType()`
- Skill level affects hit chance and damage
- 100% mastery unlocks super attacks

**Super Attack System:**
```cpp
// When mastery reaches 100%
if (m_cSkillMastery[weaponSkillType] == 100) {
    // Super attack available
    // Uses m_iSuperAttackLeft counter
}
```

**Super Attack Mapping:**

| Skill ID | Weapon Type | Super Attack ID |
|----------|-------------|-----------------|
| 5 | Hand Attack | 20 |
| 7 | Short Sword | 21 |
| 9 | Axe | 22 |
| 8 | Long Sword | 23 |
| 10 | Shield | 24 |
| 6 | Archery | 25 |
| 14 | Hammer | 26 |
| 21 | Staff | 27 |

### Magic System

**Magic Skill (ID 4):**
```cpp
// Game.cpp:39487
double dV1;
if (m_cSkillMastery[4] == 0)
    dV1 = 1.0;
else
    dV1 = (double)m_cSkillMastery[4];
// Used as multiplier for magic calculations
```

**Magic Resistance (ID 3):**
- Reduces incoming magic damage
- Higher mastery = more damage reduction

### Crafting System

**Manufacturing Skill (ID 13):**
```cpp
// Game.cpp:24043
if (m_cSkillMastery[13] >= m_pBuildItemList[i]->m_iSkillLimit) {
    // Player can craft this item
}
```

- Each craftable item has `m_iSkillLimit` requirement
- Player's manufacturing mastery must meet or exceed limit

### Item System

**Skill-Based Items:**
- `DEF_ITEMTYPE_USE_SKILL` - Items that activate skills
- `DEF_ITEMTYPE_USE_SKILL_ENABLEDIALOGBOX` - Items opening crafting dialog

## State Management

### Skill Mastery Loading

```cpp
// Game.cpp:17860-17866 (Initial character data)
for (i = 0; i < DEF_MAXSKILLTYPE; i++) {
    m_cSkillMastery[i] = (unsigned char)*cp;
    if (m_pSkillCfgList[i] != NULL)
        m_pSkillCfgList[i]->m_iLevel = (int)*cp;
    cp++;
}
```

Server sends 60 bytes, one per skill, containing mastery level (0-100).

### Initialization Reset

```cpp
// Game.cpp:5042
for (i = 0; i < DEF_MAXSKILLTYPE; i++)
    m_cSkillMastery[i] = NULL;

// Game.cpp:5070-5071
m_iSuperAttackLeft = 0;
m_bSuperAttackMode = FALSE;
```

## Network Messages

### Client to Server

| Message ID | Name | Purpose |
|------------|------|---------|
| 0x0A0F | DEF_COMMONTYPE_REQ_TRAINSKILL | Request skill training at NPC |
| 0x0A12 | DEF_COMMONTYPE_REQ_USESKILL | Activate an active skill |
| 0x0A1B | DEF_COMMONTYPE_REQ_SETDOWNSKILLINDEX | Set hotkeyed skill |

### Server to Client

| Message ID | Name | Purpose |
|------------|------|---------|
| 0x0B12 | DEF_NOTIFY_SKILLTRAINSUCCESS | Training completed successfully |
| 0x0B13 | DEF_NOTIFY_SKILLTRAINFAIL | Training failed (reason code) |
| 0x0B23 | DEF_NOTIFY_SKILL | Skill level changed (up or down) |
| 0x0B2A | DEF_NOTIFY_SKILLUSINGEND | Active skill use completed |
| 0x0B54 | DEF_NOTIFY_LOWPORTIONSKILL | Low potion skill notification |
| 0x0B55 | DEF_NOTIFY_SUPERSKILLLEFT | Update super attack counter |
| 0x0B59 | DEF_NOTIFY_DOWNSKILLINDEXSET | Confirm hotkey binding |

### Message Handlers

```cpp
void CGame::NotifyMsg_Skill(char *pData);
void CGame::NotifyMsg_SkillTrainSuccess(char *pData);
void CGame::NotifyMsg_SkillUsingEnd(char *pData);
void CGame::NotifyMsg_DownSkillIndexSet(char *pData);
```

### Skill Level Change Display

When `DEF_NOTIFY_SKILL` received:
- Displays floating text: `"Skill %s: %d%% increased"` or `"decreased"`
- Text appears over player character
- Color: Green for increase, red for decrease
- Sound effect played (slot 23 gain, slot 24 loss)

## Skill Usage Flow

### Active Skill Activation

1. **Player clicks skill in dialog**
   ```cpp
   DlgBoxClick_Skill() {
       // Validate conditions
       if (!m_pSkillCfgList[skillIndex]->m_bIsUseable) return;
       if (m_pSkillCfgList[skillIndex]->m_iLevel == 0) return;
       if (m_bSkillUsingStatus) return;
       if (!m_bCommandAvailable) return;
   ```

2. **Send request to server**
   ```cpp
       bSendCommand(DEF_COMMONTYPE_REQ_USESKILL, skillIndex);
       m_bSkillUsingStatus = TRUE;
       // Play sound effect
   }
   ```

3. **Server validates and executes**
   - Checks level requirements
   - Checks resource costs (MP/SP)
   - Executes skill effect
   - Sends result notification

4. **Client receives result**
   ```cpp
   NotifyMsg_SkillUsingEnd(pData) {
       // Display success/failure message
       // Clear usage state
       m_bSkillUsingStatus = FALSE;
   }
   ```

### Training Flow

1. **Player interacts with training NPC**
2. **Client sends** `DEF_COMMONTYPE_REQ_TRAINSKILL`
3. **Server checks:**
   - Character level requirement
   - Stat requirements
   - Gold cost
   - Prerequisite skills
4. **Server responds:**
   - `DEF_NOTIFY_SKILLTRAINSUCCESS` with new level
   - OR `DEF_NOTIFY_SKILLTRAINFAIL` with reason

## Known Issues / Technical Debt

1. **Hardcoded skill IDs** - Weapon skill mapping uses magic numbers
2. **Undocumented skills** - 5+ skills marked "????" with unknown purpose
3. **Monolithic code** - All skill logic embedded in 48k line Game.cpp
4. **No cooldown tracking** - Client relies entirely on server for timing
5. **Limited validation** - Most checks done server-side, client trusts responses
6. **Fixed array size** - 60 skill limit is compile-time constant
7. **Mixed concerns** - UI rendering, network handling, and game logic interleaved

## Modernization Notes

### Recommended Changes

1. **Extract skill system** to dedicated `SkillSystem` class
2. **Define skill IDs** as named constants or enum class
3. **Use data-driven definitions** - Load from JSON/YAML instead of custom format
4. **Add client-side cooldown tracking** for responsive UI
5. **Implement skill categories** as explicit enum
6. **Create skill effect interfaces** for extensibility
7. **Separate UI logic** from skill data management
8. **Add experience tracking** - Currently only mastery level stored

### Protocol Compatibility

Must preserve exact message formats:
- 0x0A0F, 0x0A12, 0x0A1B for requests
- 0x0B12, 0x0B13, 0x0B23, 0x0B2A, 0x0B54, 0x0B55, 0x0B59 for notifications
- 60-byte skill array in character data

### Skill Categories (Suggested)

```cpp
enum class SkillCategory {
    Combat,     // 5, 6, 7, 8, 9, 10, 11, 14, 21
    Magic,      // 3, 4
    Crafting,   // 12, 13, 16
    Gathering,  // 0, 1, 2
    Misc        // 19, 23
};
```
