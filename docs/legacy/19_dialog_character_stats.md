# Dialog: Character Stats & Level-Up Settings

## Overview

The Character Stats dialog system consists of two interconnected dialogs:
1. **Character Dialog (Dialog Box 1)** - Displays character information, stats, and equipped items
2. **Level-Up Settings Dialog (Dialog Box 12)** - Allows pre-allocation of stat points for future level-ups

These dialogs are central to the player's character management, showing vital statistics, equipment status, and enabling stat point distribution strategy.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | Dialog rendering (`DrawDialogBox_Character`, `DrawDialogBox_LevelUpSetting`) and click handling |
| `Game.h` | Member variables for stats, dialog info structure, function declarations |
| `Item.h` | Equipment position constants (`DEF_EQUIPPOS_*`) |
| `SpriteID.h` | Interface sprite IDs (`DEF_SPRID_INTERFACE_ND_*`) |
| `lan_eng.h` | English localization strings |
| `lan_chi.h` | Chinese localization strings |
| `lan_jap.h` | Japanese localization strings |
| `lan_kor.h` | Korean localization strings |

---

## Dialog Box 1: Character Information

### Dialog Type ID
```cpp
// Dialog Box Index: 1
m_stDialogBoxInfo[1]
```

### Opening/Closing

The character dialog can be toggled via keyboard shortcut:

```cpp
// In OnKeyDown handler (around line 18777)
if (m_bIsDialogEnabled[1] == TRUE)
    DisableDialogBox(1);
else
    EnableDialogBox(1, NULL, NULL, NULL);
PlaySound('E', 14, 5);
```

### Rendering Function

**Signature:**
```cpp
void CGame::DrawDialogBox_Character(short msX, short msY);
```

**Location:** `Game.cpp:37391`

### Visual Layout

The dialog uses sprite `DEF_SPRID_INTERFACE_ND_TEXT` (ID 70) as the background:

```
+------------------------------------------+
|  [Player Name] : Criminal(X) Contrib(Y)  |  <- Line sY+52
|  [Faction/Guild Status]                  |  <- Line sY+69
+------------------------------------------+
|  Level:          [value]                 |  <- sY+106
|  Exp:            [value]                 |  <- sY+125
|  Next Exp:       [value]                 |  <- sY+142
+------------------------------------------+
|  HP:             [current/max]           |  <- sY+173
|  MP:             [current/max]           |  <- sY+191
|  SP:             [current/max]           |  <- sY+208
+------------------------------------------+
|  Max Load:       [current/max]           |  <- sY+240
|  Enemy Kills:    [value]                 |  <- sY+257
+------------------------------------------+
|                                          |
|     [Character Equipment Display]        |  <- Visual paper doll
|                                          |
+------------------------------------------+
|  STR [val]    INT [val]    VIT [val]    |  <- sY+285
|  DEX [val]    MAG [val]    CHR [val]    |  <- sY+302
+------------------------------------------+
```

### Stat Display Positions

| Stat | X Range | Y Position |
|------|---------|------------|
| Level | sX+180 to sX+250 | sY+106 |
| Experience | sX+180 to sX+250 | sY+125 |
| Next Level Exp | sX+180 to sX+250 | sY+142 |
| HP | sX+180 to sX+250 | sY+173 |
| MP | sX+180 to sX+250 | sY+191 |
| SP | sX+180 to sX+250 | sY+208 |
| Max Load | sX+180 to sX+250 | sY+240 |
| Enemy Kills | sX+180 to sX+250 | sY+257 |
| Strength | sX+48 to sX+82 | sY+285 |
| Vitality | sX+218 to sX+251 | sY+285 |
| Dexterity | sX+48 to sX+82 | sY+302 |
| Intelligence | sX+135 to sX+167 | sY+285 |
| Magic | sX+135 to sX+167 | sY+302 |
| Charisma | sX+218 to sX+251 | sY+302 |

### Derived Stat Formulas

The game calculates maximum values for HP/MP/SP client-side for display:

```cpp
// Max HP calculation
m_iVit * 3 + m_iLevel * 2 + m_iStr / 2

// Max MP calculation
m_iMag * 2 + m_iLevel * 2 + m_iInt / 2

// Max SP calculation
m_iStr * 2 + m_iLevel * 2

// Max carrying capacity (in weight units / 100)
m_iStr * 5 + m_iLevel * 5
```

### Player Status Display

The dialog shows faction and guild information:

```cpp
if (m_bCitizen == FALSE)
    strcpy(G_cTxt, DRAW_DIALOGBOX_CHARACTER7);  // "Traveller"
else {
    if (m_bHunter) {
        if (m_bAresden)
            strcat(G_cTxt, DEF_MSG_ARECIVIL);   // "Aresden Civilian"
        else
            strcat(G_cTxt, DEF_MSG_ELVCIVIL);   // "Elvine Civilian"
    } else {
        if (m_bAresden)
            strcat(G_cTxt, DEF_MSG_ARESOLDIER); // "Aresden Soldier"
        else
            strcat(G_cTxt, DEF_MSG_ELVSOLDIER); // "Elvine Soldier"
    }

    // Guild info
    if (m_iGuildRank >= 0) {
        strcat(G_cTxt, "(");
        strcat(G_cTxt, m_cGuildName);
        if (m_iGuildRank == 0)
            strcat(G_cTxt, DEF_MSG_GUILDMASTER1); // " GuildMaster)"
        else
            strcat(G_cTxt, DEF_MSG_GUILDSMAN1);   // " Guildsman)"
    }
}
```

### Equipment Paper Doll

The dialog renders equipped items visually on a character model:

```cpp
// Character base sprite (male types 1-3)
m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0]->PutSpriteFast(sX + 171, sY + 290, m_sPlayerType-1, m_dwCurTime);

// Hair (if no helmet)
if (cEquipPosStatus[DEF_EQUIPPOS_HEAD] == -1) {
    _GetHairColorRGB(((m_sPlayerAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
    m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18]->PutSpriteRGB(
        sX + 171, sY + 290,
        (m_sPlayerAppr1 & 0x0F00) >> 8,  // Hair style
        iR, iG, iB, m_dwCurTime);
}

// Underwear
m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19]->PutSpriteFast(
    sX + 171, sY + 290,
    (m_sPlayerAppr1 & 0x000F),  // Underwear color
    m_dwCurTime);
```

### Equipment Slot Rendering Order

Equipment is rendered in a specific order for proper layering:

1. **Back** (DEF_EQUIPPOS_BACK) - Position: sX+41, sY+137
2. **Pants** (DEF_EQUIPPOS_PANTS) - Position: sX+171, sY+290
3. **Arms** (DEF_EQUIPPOS_ARMS) - Position: sX+171, sY+290
4. **Boots** (DEF_EQUIPPOS_BOOTS) - Position: sX+171, sY+290
5. **Body** (DEF_EQUIPPOS_BODY) - Position: sX+171, sY+290
6. **Full Body** (DEF_EQUIPPOS_FULLBODY) - Position: sX+171, sY+290
7. **Head** (DEF_EQUIPPOS_HEAD) - Position: sX+72, sY+135
8. **Neck** (DEF_EQUIPPOS_NECK) - Position: sX+35, sY+120
9. **Right Hand** (DEF_EQUIPPOS_RHAND) - Position: sX+57, sY+186
10. **Two-Hand** (DEF_EQUIPPOS_TWOHAND) - Position: sX+57, sY+186
11. **Left Hand** (DEF_EQUIPPOS_LHAND) - Position: sX+90, sY+170
12. **Right Finger** (DEF_EQUIPPOS_RFINGER) - Position: sX+32, sY+193
13. **Left Finger** (DEF_EQUIPPOS_LFINGER) - Position: sX+115, sY+193

### Equipment Click Handler

**Function:** `bDlgBoxPress_Character`

**Location:** `Game.cpp:22087`

```cpp
BOOL CGame::bDlgBoxPress_Character(short msX, short msY)
{
    // Check if query dialog is open (item drop confirmation)
    if (m_bIsDialogEnabled[17] == TRUE) return FALSE;

    // Check collision with each equipped item sprite
    // If clicked, set cursor to item selection mode
    if (m_pSprite[...]->_bCheckCollison(sX + pos, sY + pos, sFrame, msX, msY)) {
        m_stMCursor.cSelectedObjectType = DEF_SELECTEDOBJTYPE_ITEM;
        m_stMCursor.sSelectedObjectID   = m_sItemEquipmentStatus[DEF_EQUIPPOS_*];
        m_stMCursor.sDistX = 0;
        m_stMCursor.sDistY = 0;
        return TRUE;
    }
    // ...
}
```

This allows players to click on equipped items to select them for drag-and-drop operations.

---

## Dialog Box 12: Level-Up Settings

### Dialog Type ID
```cpp
// Dialog Box Index: 12
m_stDialogBoxInfo[12]
```

### Purpose

This dialog allows players to pre-configure how stat points will be distributed when they level up. Points are allocated in advance and automatically applied when the player gains enough experience.

### Automatic Opening

The level-up dialog automatically opens when there are unspent stat points:

```cpp
// In game update loop (around line 4284)
if ((_iCheckLUS() != 0))
    EnableDialogBox(12, NULL, NULL, NULL);
```

### Rendering Function

**Signature:**
```cpp
void CGame::DrawDialogBox_LevelUpSetting(short msX, short msY);
```

**Location:** `Game.cpp:38095` (variant 1) and `Game.cpp:38180` (variant 2)

There are two versions controlled by preprocessor directives for different interface layouts.

### Visual Layout (Standard Version)

```
+------------------------------------------+
|  "When level up, your specific stats"    |  <- sY+50
|  "will be increased by setting."         |  <- sY+65
+------------------------------------------+
|  * Points left: [N]                      |  <- sY+85
+------------------------------------------+
|  Strength     _____ [+] [-]              |  <- sY+110
|  Vitality     _____ [+] [-]              |  <- sY+125
|  Dexterity    _____ [+] [-]              |  <- sY+140
|  Intelligence _____ [+] [-]              |  <- sY+155
|  Magic        _____ [+] [-]              |  <- sY+170
|  Charisma     _____ [+] [-]              |  <- sY+185
+------------------------------------------+
|  "At level up, your specific stat(s)"    |  <- sY+220
|  "will be increased by this setting."    |  <- sY+235
|  "Press the OK button when you"          |  <- sY+250
|  "finish level up setting."              |  <- sY+265
+------------------------------------------+
|                [OK]                       |  <- sY+292
+------------------------------------------+
|  [Error messages if validation fails]    |  <- sY+295
+------------------------------------------+
```

### Member Variables

**Stat Points:**
```cpp
int m_iLU_Point;  // Remaining points to allocate
```

**Pending Allocations:**
```cpp
char m_cLU_Str;   // Points allocated to Strength
char m_cLU_Vit;   // Points allocated to Vitality
char m_cLU_Dex;   // Points allocated to Dexterity
char m_cLU_Int;   // Points allocated to Intelligence
char m_cLU_Mag;   // Points allocated to Magic
char m_cLU_Char;  // Points allocated to Charisma
```

**Current Stats:**
```cpp
int m_iStr;       // Current Strength
int m_iVit;       // Current Vitality
int m_iDex;       // Current Dexterity
int m_iInt;       // Current Intelligence
int m_iMag;       // Current Magic
int m_iCharisma;  // Current Charisma
```

### Click Handler

**Function:** `DlgBoxClick_LevelUpSettings`

**Location:** `Game.cpp:20719` (variant 1) and `Game.cpp:20835` (variant 2)

**Standard Version Button Hit Zones:**

```cpp
// Increment buttons (left column, sprite frame 19)
// Y positions: 110, 125, 140, 155, 170, 185

// STR increment: (sX+184, sY+110) to (sX+197, sY+122)
if ((msX > sX + 184) && (msX < sX + 197) && (msY > sY + 110) && (msY < sY + 122)) {
    if (m_iLU_Point > 0) {
        m_iLU_Point--;
        m_cLU_Str++;
    }
    PlaySound('E', 14, 5);
}

// Decrement buttons (right column, sprite frame 20)
// STR decrement: (sX+199, sY+110) to (sX+212, sY+122)
if ((msX > sX + 199) && (msX < sX + 212) && (msY > sY + 110) && (msY < sY + 122)) {
    if (m_cLU_Str > 0) {
        m_cLU_Str--;
        m_iLU_Point++;
    }
    PlaySound('E', 14, 5);
}
```

### Validation Function

**Function:** `_iCheckLUS`

**Location:** `Game.cpp:14712`

```cpp
int CGame::_iCheckLUS()
{
    // Return 1 if points remain unallocated
    if (m_iLU_Point != 0) return 1;

    // Return 2 if any stat would exceed 200 after allocation
    if ((m_iStr + m_cLU_Str) > 200) return 2;
    if ((m_iDex + m_cLU_Dex) > 200) return 2;
    if ((m_iInt + m_cLU_Int) > 200) return 2;
    if ((m_iVit + m_cLU_Vit) > 200) return 2;
    if ((m_iMag + m_cLU_Mag) > 200) return 2;
    if ((m_iCharisma + m_cLU_Char) > 200) return 2;

    // Return 0 if allocation is valid and complete
    return 0;
}
```

### Validation Error Messages

| Return Code | Message |
|-------------|---------|
| 1 | "Level up setting is not finished yet." |
| 2 | "Current level up setting is incorrect." + "Because when you level up, your specific stat(s) will be over the limit" |

### OK Button

The OK button is only active when `_iCheckLUS() == 0`:

```cpp
if (_iCheckLUS() == 0) {
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) &&
        (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);  // Highlighted
    else
        DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);  // Normal
}
```

**Button Constants:**
```cpp
#define DEF_BTNSZX      74   // Button width
#define DEF_BTNSZY      20   // Button height
#define DEF_RBTNPOSX    154  // Right button X position
#define DEF_BTNPOSY     292  // Button Y position
```

---

## Experience System

### Level Experience Formula

**Function:** `iGetLevelExp`

**Location:** `Game.cpp:23051`

```cpp
int CGame::iGetLevelExp(int iLevel)
{
    int iRet;

    if (iLevel == 0) return 0;

    // Recursive formula
    iRet = iGetLevelExp(iLevel - 1) + iLevel * (50 + (iLevel * (iLevel / 17) * (iLevel / 17)));

    return iRet;
}
```

This is a recursive formula where each level requires:
- Previous level's cumulative exp
- Plus: `level * (50 + (level * (level/17)^2))`

### Experience Display

The gauge panel shows experience progress:

```cpp
int iCurExp = iGetLevelExp(m_iLevel);
int iNextExp = iGetLevelExp(m_iLevel + 1);

// Special handling for levels above 139
if (m_iLevel > 139) {
    iLev = (m_iLevel - 139) * 3;
    iNextExp = iGetLevelExp(m_iLevel + iLev);
}

// Calculate percentage
if (m_iExp < iNextExp) {
    iNextExp = iNextExp - iCurExp;
    if (m_iExp > iCurExp)
        iCurExp = m_iExp - iCurExp;
    else
        iCurExp = 0;

    short sPerc = 0;
    if (iCurExp > 200000)
        sPerc = short(((iCurExp >> 4) * 10000) / (iNextExp >> 4));
    else
        sPerc = (short)((iCurExp * 10000) / iNextExp);

    wsprintf(G_cTxt, DEF_MSG_EXP "%d/%d(%d.%02d%%)",
             iNextExp - iCurExp, iNextExp, sPerc / 100, sPerc % 100);
}
```

---

## Equipment Position Constants

Defined in `Item.h:15-28`:

```cpp
#define DEF_EQUIPPOS_NONE       0   // Not equipped
#define DEF_EQUIPPOS_HEAD       1   // Helmet/Hat
#define DEF_EQUIPPOS_BODY       2   // Armor/Shirt
#define DEF_EQUIPPOS_ARMS       3   // Gloves/Gauntlets
#define DEF_EQUIPPOS_PANTS      4   // Leggings/Pants
#define DEF_EQUIPPOS_BOOTS      5   // Boots/Shoes
#define DEF_EQUIPPOS_NECK       6   // Necklace/Amulet
#define DEF_EQUIPPOS_LHAND      7   // Left Hand (Shield)
#define DEF_EQUIPPOS_RHAND      8   // Right Hand (Weapon)
#define DEF_EQUIPPOS_TWOHAND    9   // Two-Handed Weapon
#define DEF_EQUIPPOS_RFINGER    10  // Right Ring
#define DEF_EQUIPPOS_LFINGER    11  // Left Ring
#define DEF_EQUIPPOS_BACK       12  // Cape/Backpack
#define DEF_EQUIPPOS_FULLBODY   13  // Full Body Armor (robe)
```

---

## Localization Strings

### Character Dialog Strings

| Key | English | Purpose |
|-----|---------|---------|
| `DRAW_DIALOGBOX_CHARACTER1` | "Criminal (%d)" | PK count display |
| `DRAW_DIALOGBOX_CHARACTER2` | "Contribution (%d)" | Contribution display |
| `DRAW_DIALOGBOX_CHARACTER3` | "Aresden %s GuildMaster" | Guild master title |
| `DRAW_DIALOGBOX_CHARACTER4` | "Elvine %s GuildMaster" | Guild master title |
| `DRAW_DIALOGBOX_CHARACTER5` | "Aresden %s Guildsman" | Guild member title |
| `DRAW_DIALOGBOX_CHARACTER6` | "Elvine %s Guildsman" | Guild member title |
| `DRAW_DIALOGBOX_CHARACTER7` | "Traveller" | Non-citizen status |
| `DRAW_DIALOGBOX_CHARACTER8` | "Citizen of Aresden" | Faction status |
| `DRAW_DIALOGBOX_CHARACTER9` | "Citizen of Elvine" | Faction status |

### Level-Up Dialog Strings

| Key | English | Purpose |
|-----|---------|---------|
| `DRAW_DIALOGBOX_LEVELUP_SETTING1` | "When level up, your specific stats" | Instruction line 1 |
| `DRAW_DIALOGBOX_LEVELUP_SETTING2` | "will be increased by setting." | Instruction line 2 |
| `DRAW_DIALOGBOX_LEVELUP_SETTING3` | "* Points left: %d" | Points remaining |
| `DRAW_DIALOGBOX_LEVELUP_SETTING4` | "Strength" | Stat label |
| `DRAW_DIALOGBOX_LEVELUP_SETTING5` | "Vitality" | Stat label |
| `DRAW_DIALOGBOX_LEVELUP_SETTING6` | "Dexterity" | Stat label |
| `DRAW_DIALOGBOX_LEVELUP_SETTING7` | "Intelligence" | Stat label |
| `DRAW_DIALOGBOX_LEVELUP_SETTING8` | "Magic" | Stat label |
| `DRAW_DIALOGBOX_LEVELUP_SETTING9` | "Charisma" | Stat label |
| `DRAW_DIALOGBOX_LEVELUP_SETTING10-13` | Instructions | OK button guidance |
| `DRAW_DIALOGBOX_LEVELUP_SETTING14` | "Level up setting is not finished yet." | Error: points remain |
| `DRAW_DIALOGBOX_LEVELUP_SETTING15-17` | Error messages | Error: stat over limit |

---

## Sprite Resources

### Interface Sprites Used

| Sprite ID | Constant | Purpose |
|-----------|----------|---------|
| 70 | `DEF_SPRID_INTERFACE_ND_TEXT` | Dialog background with text areas |
| 61 | `DEF_SPRID_INTERFACE_ND_GAME2` | Level-up dialog background, +/- buttons |
| 63 | `DEF_SPRID_INTERFACE_ND_GAME4` | Additional decorative elements |
| 71 | `DEF_SPRID_INTERFACE_ND_BUTTON` | OK button (frames 0=normal, 1=hover) |

### Equipment Display Sprites

| Base Sprite | Purpose |
|-------------|---------|
| `DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0` | Male character base |
| `DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18` | Hair |
| `DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19` | Underwear |
| `DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 40` | Female character base |

Equipment sprites are loaded from:
- `sprites\item-equipM.pak` - Male equipment
- `sprites\item-equipW.pak` - Female equipment

---

## Constants & Limits

### Stat Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| Maximum stat value | 200 | Hard cap per stat |
| Minimum stat value | 10 | Starting value (character creation) |
| Points per level | 3 | Stat points gained per level |

### Dialog Dimensions

| Dialog | Width | Height |
|--------|-------|--------|
| Character (Dialog 1) | ~275 pixels | ~340 pixels |
| Level-Up (Dialog 12) | Variable | ~350 pixels |

---

## Integration Points

### With Inventory System
- Equipment slots link to inventory items via `m_pItemList[]` and `m_bIsItemEquipped[]`
- Item dragging from character dialog uses `m_stMCursor` cursor state

### With Network System
- Stat changes are sent to server when level-up allocation is confirmed
- Character data received from server updates all displayed stats

### With Audio System
- Button clicks play sound: `PlaySound('E', 14, 5)` (effect category, sound ID 14, volume 5)

---

## Known Issues / Technical Debt

1. **Hardcoded Positions**: All UI element positions are pixel-perfect constants scattered throughout the code
2. **Multiple Versions**: Two variants of `DrawDialogBox_LevelUpSetting` controlled by preprocessor
3. **Recursive Formula**: `iGetLevelExp` uses recursion without memoization (inefficient for high levels)
4. **Gender Handling**: Separate sprite sets for male/female with magic number offsets (+40 for female)
5. **Korean Comments**: Many comments in Korean require translation for maintainability
6. **CInt Wrapper**: HP/MP/SP use `CInt` wrapper class instead of standard int

---

## Modernization Notes

### For C++20 Port

1. **Separate Dialog Classes**: Create `CharacterDialog` and `LevelUpDialog` classes inheriting from `Dialog`
2. **Data Binding**: Use observer pattern for stat updates instead of direct member access
3. **Layout System**: Replace hardcoded positions with relative/flex layout
4. **Validation**: Move `_iCheckLUS` logic to a proper validator class with clear error types
5. **Experience Table**: Pre-compute experience table at startup instead of recursive calculation
6. **Equipment Rendering**: Use component-based approach with render layers
7. **Localization**: Move to runtime string loading from JSON/YAML instead of compile-time defines
8. **Stat Types**: Create strongly-typed `StatType` enum instead of magic indices 0-5
