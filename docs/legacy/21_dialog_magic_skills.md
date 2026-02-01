# Dialog System: Magic & Skills

## Overview

This document covers the three dialogs related to magic and skills in the legacy Helbreath client:

1. **Spellbook Dialog (Index 3)** - View learned spells, cast spells, assign hotkeys
2. **Magic Shop Dialog (Index 16)** - Learn new spells from NPCs
3. **Skills Dialog (Index 15)** - View skill masteries, activate skills, assign hotkeys

These dialogs integrate tightly with the magic system (`Magic.h/cpp`) and skill system (`Skill.h/cpp`) for data, while CGame handles all rendering and input.

---

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | All dialog rendering (`DrawDialogBox_*`) and click handling (`DlgBoxClick_*`) |
| `Game.h` | Dialog state arrays, spell/skill mastery arrays |
| `Magic.h` / `Magic.cpp` | CMagic class definition, spell properties |
| `Skill.h` / `Skill.cpp` | CSkill class definition, skill properties |
| `NetMessages.h` | Network message IDs for spell/skill operations |

---

## Key Data Structures

### DialogBoxInfo Structure

```cpp
struct DialogBoxInfo {
    short sX, sY;           // Screen position
    short sSizeX, sSizeY;   // Dialog dimensions
    short sView;            // Scroll offset / current circle (0-9 for magic)
    char  cMode;            // Dialog mode/state
    short sV1, sV2, sV3;    // Additional state variables
    short sV4, sV5, sV6;
    DWORD dwT1, dwT2;       // Timing values
    char  cStr[256];        // String buffer
    BOOL  bFlag;            // Generic flag
    BOOL  bIsScrollSelected;// Scrollbar being dragged
};
```

### CMagic Class

```cpp
class CMagic {
public:
    char  m_cName[31];      // Spell name (e.g., "Magic Missile")
    int   m_sValue1;        // Base mana cost
    int   m_sValue2;        // Required Intelligence to learn
    int   m_sValue3;        // Gold cost to learn
    int   m_sValue4;        // (unused in client)
    int   m_sValue5;        // (unused in client)
    int   m_sValue6;        // (unused in client)
    bool  m_bIsVisible;     // Whether to show in spellbook UI
};
```

### CSkill Class

```cpp
class CSkill {
public:
    char  m_cName[21];      // Skill name (e.g., "Mining")
    int   m_iLevel;         // Current mastery level 0-100
    BOOL  m_bIsUseable;     // Can be actively triggered
    char  m_cUseMethod;     // 0=direct, 1=requires item, 2=opens dialog
};
```

### Mastery Arrays in CGame

```cpp
// Spell mastery: 0 = not learned, 1-255 = mastery level
char m_cMagicMastery[100];

// Skill mastery: 0-100 percentage
unsigned char m_cSkillMastery[60];

// Spell/skill configuration loaded from server
CMagic* m_pMagicCfgList[100];
CSkill* m_pSkillCfgList[60];

// Currently hotkeyed skill (-1 = none)
int m_iDownSkillIndex;
```

---

## 1. Spellbook Dialog (Index 3)

### Dialog Properties

| Property | Value |
|----------|-------|
| Dialog Index | 3 |
| Sprite | `DEF_SPRID_INTERFACE_ND_GAME1` |
| Text Panel | `DEF_SPRID_INTERFACE_ND_TEXT` |
| Size | ~256x300 pixels |
| Spells Visible | 9 per circle |
| Line Height | 18 pixels |

### Drawing Function

**Location:** `Game.cpp:39369` - `CGame::DrawDialogBox_Magic()`

```cpp
void CGame::DrawDialogBox_Magic(short msX, short msY, short msZ, char cLB)
{
    short sX = m_stDialogBoxInfo[3].sX;
    short sY = m_stDialogBoxInfo[3].sY;

    // Draw dialog background
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME1, sX, sY, 2);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX+18, sY+34, 1);

    // Calculate starting spell index from current circle
    int iCPivot = m_stDialogBoxInfo[3].sView * 10;

    // Draw each spell in current circle
    for (int i = 0; i < 10; i++) {
        if (m_cMagicMastery[iCPivot + i] == 0) continue;  // Not learned
        if (m_pMagicCfgList[iCPivot + i] == NULL) continue;

        // Calculate Y position
        int iYloc = i * 18;

        // Get spell name (replace hyphens with spaces)
        char cTxt[64];
        strcpy(cTxt, m_pMagicCfgList[iCPivot + i]->m_cName);
        for (int j = 0; cTxt[j] != 0; j++)
            if (cTxt[j] == '-') cTxt[j] = ' ';

        // Determine color based on mana and hover state
        DWORD dwColor;
        if (iGetManaCost(iCPivot + i) > m_iMP) {
            dwColor = RGB(41, 16, 41);  // Gray - insufficient mana
        } else if (msX >= sX+30 && msX <= sX+240 &&
                   msY >= sY+70+iYloc && msY <= sY+88+iYloc) {
            dwColor = RGB(255, 255, 255);  // White - hovered
        } else {
            dwColor = RGB(8, 0, 66);  // Dark blue - normal
        }

        // Draw spell name
        PutString(sX + 30, sY + 70 + iYloc, cTxt, dwColor);

        // Draw mana cost (right-aligned)
        wsprintf(cTxt, "%3d", iGetManaCost(iCPivot + i));
        PutString(sX + 206, sY + 70 + iYloc, cTxt, dwColor);
    }

    // Draw circle success rate at bottom
    int iSuccessRate = _iCalcTotalMagicEffect(m_stDialogBoxInfo[3].sView + 1);
    wsprintf(cTxt, DEF_MSG_MAGICABILITY, iSuccessRate);  // "Circle %d ability: %d%%"
    PutString(sX + 30, sY + 267, cTxt, RGB(45, 25, 25));

    // Draw circle selection buttons (10 buttons)
    DrawCircleButtons(sX, sY);
}
```

### Circle Navigation

Spells are organized into 10 circles (tiers). The UI shows 10 clickable icons at the bottom:

```cpp
// Circle button positions (Y range: sY+240 to sY+268)
Circle 0:  sX+16  to sX+38
Circle 1:  sX+39  to sX+56
Circle 2:  sX+57  to sX+81
Circle 3:  sX+82  to sX+101
Circle 4:  sX+102 to sX+116
Circle 5:  sX+117 to sX+137
Circle 6:  sX+138 to sX+165
Circle 7:  sX+166 to sX+197
Circle 8:  sX+198 to sX+217
Circle 9:  sX+218 to sX+239
```

Mouse wheel also navigates circles:
- Wheel up (msZ > 0): Previous circle
- Wheel down (msZ < 0): Next circle

### Click Handler

**Location:** `Game.cpp:42455` - `CGame::DlgBoxClick_Magic()`

```cpp
void CGame::DlgBoxClick_Magic(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[3].sX;
    short sY = m_stDialogBoxInfo[3].sY;
    int iCPivot = m_stDialogBoxInfo[3].sView * 10;

    // Check spell clicks
    for (int i = 0; i < 10; i++) {
        if (m_cMagicMastery[iCPivot + i] == 0) continue;
        if (m_pMagicCfgList[iCPivot + i] == NULL) continue;

        int iYloc = i * 18;

        if (msX >= sX+30 && msX <= sX+240 &&
            msY >= sY+70+iYloc && msY <= sY+88+iYloc) {

            // Cast this spell
            PlaySound('E', 14, 5);
            UseMagic(iCPivot + i);
            return;
        }
    }

    // Check circle button clicks
    if (msY >= sY+240 && msY <= sY+268) {
        // Determine which circle clicked...
        m_stDialogBoxInfo[3].sView = clickedCircle;
    }

    // Check close button
    if (msX >= sX+154 && msX <= sX+228 &&
        msY >= sY+285 && msY <= sY+305) {
        DisableDialogBox(3);
    }
}
```

### Spell Casting Flow

```cpp
void CGame::UseMagic(int iMagicNo)
{
    // Validate spell is learned
    if (m_cMagicMastery[iMagicNo] == 0) return;

    // Check mana
    if (iGetManaCost(iMagicNo) > m_iMP) {
        AddEventList(MSG_NOTENOUGHMANA, 10);
        return;
    }

    // Check if holding an item (can't cast while holding)
    if (m_bIsItemEquipped) {
        AddEventList(MSG_CANTCASTWHILEHOLDING, 10);
        return;
    }

    // Enter targeting mode for the spell
    m_bIsGetPointingMode = TRUE;
    m_iPointingSpell = iMagicNo;

    // Close spellbook
    DisableDialogBox(3);
}
```

### Mana Cost Calculation

**Location:** `Game.cpp:48009` - `CGame::iGetManaCost()`

```cpp
int CGame::iGetManaCost(int iMagicNo)
{
    int iManaCost = m_pMagicCfgList[iMagicNo]->m_sValue1;  // Base cost

    // Safe attack mode adds 40% mana cost
    if (m_bIsSafeAttackMode) {
        iManaCost += (iManaCost / 2) - (iManaCost / 10);  // +40%
    }

    // Check equipped items for mana savings
    int iManaSave = 0;

    // Scan equipment for mana-saving items
    for (int i = 0; i < DEF_MAXITEMS; i++) {
        if (m_pItemList[i] == NULL) continue;
        if (!m_bIsItemEquipped[i]) continue;

        // MagicWand(MS10) = 10 points
        // MagicWand(MS20) = 20 points
        // MagicWand(MS30-LLF) = 30 points
        // MagicNecklace(MS10) = 10 points
        // DarkMageMagicWand = 28 points
        // DarkMageMagicStaff = 25 points
        // NecklaceOfLiche = 15 points
        // (item name string matching logic)
    }

    // Apply mana savings
    if (iManaSave > 0) {
        double dReduction = (double)iManaSave / 100.0;
        iManaCost -= (int)(iManaCost * dReduction);
    }

    if (iManaCost < 1) iManaCost = 1;
    return iManaCost;
}
```

### Magic Success Rate Calculation

```cpp
int CGame::_iCalcTotalMagicEffect(int iMagicCircle)
{
    // Base success rates by circle
    static int _tmp_iMCProb[11] = {0, 300, 250, 200, 150, 100, 80, 70, 60, 50, 40};
    static int _tmp_iMLevelPenalty[11] = {0, 5, 5, 8, 8, 10, 14, 28, 32, 36, 40};

    int iSuccessRate = _tmp_iMCProb[iMagicCircle];

    // Add magic skill mastery bonus
    iSuccessRate += m_cSkillMastery[4];  // Skill 4 = Magic

    // Add Intelligence bonus (INT > 50)
    if (m_iInt > 50) {
        iSuccessRate += (m_iInt - 50) / 2;
    }

    // Subtract level penalty for over-leveling
    if (m_iLevel > iMagicCircle * 10) {
        iSuccessRate -= _tmp_iMLevelPenalty[iMagicCircle];
    }

    // Hunger penalty
    if (m_iHunger < 30) {
        iSuccessRate -= 10;
    }

    // Equipped magic items bonus (+3% per point)
    // (check for MagicWand, etc.)

    if (iSuccessRate > 100) iSuccessRate = 100;
    if (iSuccessRate < 0) iSuccessRate = 0;

    return iSuccessRate;
}
```

---

## 2. Magic Shop Dialog (Index 16)

### Dialog Properties

| Property | Value |
|----------|-------|
| Dialog Index | 16 |
| Sprite | `DEF_SPRID_INTERFACE_ND_GAME4` |
| Text Panel | `DEF_SPRID_INTERFACE_ND_TEXT` (panel 14) |
| Size | ~280x300 pixels |
| Spells Visible | 9 per circle |

### Drawing Function

**Location:** `Game.cpp:39558` - `CGame::DrawDialogBox_MagicShop()`

```cpp
void CGame::DrawDialogBox_MagicShop(short msX, short msY, short msZ, char cLB)
{
    short sX = m_stDialogBoxInfo[16].sX;
    short sY = m_stDialogBoxInfo[16].sY;

    // Draw dialog background
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, sX, sY, 0);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX+18, sY+34, 14);

    // Column headers
    PutString(sX+40, sY+55, "Spell Name", RGB(45, 25, 25));
    PutString(sX+212, sY+55, "Req.Int", RGB(45, 25, 25));
    PutString(sX+250, sY+55, "Cost", RGB(45, 25, 25));

    int iCPivot = m_stDialogBoxInfo[16].sView * 10;

    for (int i = 0; i < 10; i++) {
        if (m_pMagicCfgList[iCPivot + i] == NULL) continue;

        int iYloc = i * 18;

        // Color based on learned status
        DWORD dwColor;
        if (m_cMagicMastery[iCPivot + i] != 0) {
            dwColor = RGB(41, 16, 41);  // Gray - already learned
        } else if (/* mouse hover check */) {
            dwColor = RGB(255, 255, 255);  // White - hovered
        } else {
            dwColor = RGB(8, 0, 66);  // Dark - available
        }

        // Spell name
        PutString(sX+30, sY+70+iYloc,
                  m_pMagicCfgList[iCPivot+i]->m_cName, dwColor);

        // Required INT
        wsprintf(cTxt, "%3d", m_pMagicCfgList[iCPivot+i]->m_sValue2);
        PutString(sX+212, sY+70+iYloc, cTxt, dwColor);

        // Gold cost
        wsprintf(cTxt, "%5d", m_pMagicCfgList[iCPivot+i]->m_sValue3);
        PutString(sX+250, sY+70+iYloc, cTxt, dwColor);
    }

    // Help text
    PutString(sX+30, sY+275, "Select a spell to learn.", RGB(45, 25, 25));
}
```

### Click Handler

**Location:** `Game.cpp:22720` - `CGame::DlgBoxClick_MagicShop()`

```cpp
void CGame::DlgBoxClick_MagicShop(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[16].sX;
    short sY = m_stDialogBoxInfo[16].sY;
    int iCPivot = m_stDialogBoxInfo[16].sView * 10;

    for (int i = 0; i < 10; i++) {
        if (m_pMagicCfgList[iCPivot + i] == NULL) continue;
        if (m_cMagicMastery[iCPivot + i] != 0) continue;  // Already learned

        int iYloc = i * 18;

        if (msX >= sX+24 && msX <= sX+159 &&
            msY >= sY+70+iYloc && msY <= sY+84+iYloc) {

            // Request to learn this spell
            PlaySound('E', 14, 5);

            bSendCommand(MSGID_COMMAND_COMMON,
                         DEF_COMMONTYPE_REQ_STUDYMAGIC,
                         NULL, NULL, NULL, NULL,
                         m_pMagicCfgList[iCPivot + i]->m_cName);
            return;
        }
    }

    // Circle button handling (same as spellbook)
    // Close button handling
}
```

### Network Messages

| Message | ID | Purpose |
|---------|-----|---------|
| `DEF_COMMONTYPE_REQ_STUDYMAGIC` | 0x0A0E | Request to learn a spell |
| `DEF_NOTIFY_MAGICSTUDYSUCCESS` | 0x0B10 | Spell learned successfully |
| `DEF_NOTIFY_MAGICSTUDYFAIL` | 0x0B11 | Learning failed (gold/INT/etc.) |

---

## 3. Skills Dialog (Index 15)

### Dialog Properties

| Property | Value |
|----------|-------|
| Dialog Index | 15 |
| Sprite | `DEF_SPRID_INTERFACE_ND_GAME2` (mode 0) |
| Text Panel | `DEF_SPRID_INTERFACE_ND_TEXT` (panel 1) |
| Size | ~260x350 pixels |
| Visible Rows | 17 skills |
| Row Height | 15 pixels |

### Drawing Function

**Location:** `Game.cpp:41134` - `CGame::DrawDialogBox_Skill()`

```cpp
void CGame::DrawDialogBox_Skill(short msX, short msY, short msZ, char cLB)
{
    short sX = m_stDialogBoxInfo[15].sX;
    short sY = m_stDialogBoxInfo[15].sY;

    // Draw dialog background
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 0);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX+18, sY+12, 1);

    // Column headers
    PutString(sX+30, sY+30, "Skill Name", RGB(45, 25, 25));
    PutString(sX+183, sY+30, "%", RGB(45, 25, 25));

    // Draw visible skills (scrollable list)
    for (int i = 0; i < 17; i++) {
        int skillIdx = i + m_stDialogBoxInfo[15].sView;
        if (skillIdx >= DEF_MAXSKILLTYPE) break;
        if (m_pSkillCfgList[skillIdx] == NULL) continue;

        int iYloc = i * 15;

        // Determine color
        DWORD dwColor;
        if (m_cSkillMastery[skillIdx] == 0) {
            dwColor = RGB(5, 5, 5);  // Dark gray - no skill
        } else if (/* mouse hover check */) {
            dwColor = RGB(255, 255, 255);  // White - hovered
        } else {
            dwColor = RGB(34, 30, 120);  // Blue - has skill
        }

        // Skill name
        PutString(sX+30, sY+45+iYloc,
                  m_pSkillCfgList[skillIdx]->m_cName, dwColor);

        // Mastery percentage
        wsprintf(cTxt, "%3d%%", m_cSkillMastery[skillIdx]);
        PutString(sX+183, sY+45+iYloc, cTxt, dwColor);

        // Hotkey button (sX+215 area)
        if (skillIdx == m_iDownSkillIndex) {
            // Draw "hotkeyed" indicator
            DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX+215, sY+45+iYloc, 21);
        }
    }

    // Draw scrollbar
    DrawScrollbar(sX+242, sY+35, 274, m_stDialogBoxInfo[15].sView,
                  DEF_MAXSKILLTYPE - 17);
}
```

### Click Handler

**Location:** `Game.cpp:42843` - `CGame::DlgBoxClick_Skill()`

```cpp
void CGame::DlgBoxClick_Skill(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[15].sX;
    short sY = m_stDialogBoxInfo[15].sY;

    for (int i = 0; i < 17; i++) {
        int skillIdx = i + m_stDialogBoxInfo[15].sView;
        if (skillIdx >= DEF_MAXSKILLTYPE) break;
        if (m_pSkillCfgList[skillIdx] == NULL) continue;

        int iYloc = i * 15;

        // Check skill name click (activate skill)
        if (msX >= sX+25 && msX <= sX+179 &&
            msY >= sY+45+iYloc && msY <= sY+59+iYloc) {

            // Validate skill can be used
            if (!m_pSkillCfgList[skillIdx]->m_bIsUseable) return;
            if (m_pSkillCfgList[skillIdx]->m_iLevel == 0) return;
            if (m_bSkillUsingStatus) {
                AddEventList(MSG_SKILL_ALREADY_USING, 10);
                return;
            }
            if (m_iHP <= 0 || !m_bCommandAvailable) {
                AddEventList(MSG_CANNOT_USE_SKILL, 10);
                return;
            }

            // Activate skill
            PlaySound('E', 14, 5);
            m_bSkillUsingStatus = TRUE;

            bSendCommand(MSGID_COMMAND_COMMON,
                         DEF_COMMONTYPE_REQ_USESKILL,
                         NULL, skillIdx, NULL, NULL, NULL);

            DisableDialogBox(15);
            return;
        }

        // Check hotkey button click (sX+215 to sX+240)
        if (msX >= sX+215 && msX <= sX+240 &&
            msY >= sY+45+iYloc && msY <= sY+59+iYloc) {

            // Set this skill as hotkeyed
            PlaySound('E', 14, 5);

            bSendCommand(MSGID_COMMAND_COMMON,
                         DEF_COMMONTYPE_REQ_SETDOWNSKILLINDEX,
                         NULL, skillIdx, NULL, NULL, NULL);
            return;
        }
    }

    // Scrollbar click handling
    if (msX >= sX+240 && msX <= sX+260) {
        // Calculate new scroll position from click Y
    }

    // Close button
    if (/* close button bounds */) {
        DisableDialogBox(15);
    }
}
```

### Scrolling

```cpp
// Mouse wheel scrolling
if (msZ > 0 && m_stDialogBoxInfo[15].sView > 0) {
    m_stDialogBoxInfo[15].sView--;  // Scroll up
}
if (msZ < 0 && m_stDialogBoxInfo[15].sView < DEF_MAXSKILLTYPE - 17) {
    m_stDialogBoxInfo[15].sView++;  // Scroll down
}
```

### Network Messages

| Message | ID | Purpose |
|---------|-----|---------|
| `DEF_COMMONTYPE_REQ_USESKILL` | 0x0A12 | Activate a skill |
| `DEF_COMMONTYPE_REQ_SETDOWNSKILLINDEX` | 0x0A1B | Set hotkeyed skill |
| `DEF_COMMONTYPE_REQ_TRAINSKILL` | 0x0A0F | Train skill at NPC |
| `DEF_NOTIFY_SKILL` | 0x0B23 | Skill level changed |
| `DEF_NOTIFY_SKILLUSINGEND` | 0x0B2A | Skill use completed |
| `DEF_NOTIFY_DOWNSKILLINDEXSET` | 0x0B59 | Hotkey confirmed |

---

## Constants & Limits

```cpp
#define DEF_MAXMAGICTYPE        100     // Maximum spell definitions
#define DEF_MAXSKILLTYPE        60      // Maximum skill definitions

// Dialog button sizes
#define DEF_BTNSZX              74      // Button width
#define DEF_BTNSZY              20      // Button height
#define DEF_RBTNPOSX            154     // Right button X offset
#define DEF_LBTNPOSX            30      // Left button X offset
#define DEF_BTNPOSY             292     // Button Y offset

// Skill IDs (some key ones)
#define DEF_SKILL_MAGIC         4       // Magic casting skill
#define DEF_SKILL_MANUFACTURING 13      // Crafting skill
```

### Skill Types (60 Total)

| ID | Skill Name | Useable |
|----|------------|---------|
| 0 | Mining | Yes |
| 1 | Fishing | Yes |
| 2 | Manufacturing | No (opens dialog) |
| 3 | Alchemy | Yes |
| 4 | Magic | No (passive) |
| 5 | Long Sword | No (passive) |
| 6 | Short Sword | No (passive) |
| 7 | Hammer | No (passive) |
| 8 | Axe | No (passive) |
| 9 | Staff | No (passive) |
| 10 | Bow/Crossbow | No (passive) |
| 11 | Shield | No (passive) |
| 12 | Pretend Corpse | Yes |
| 13-59 | (Various combat/utility) | Mixed |

---

## Integration Points

### With Magic System (Magic.h/cpp)

- `m_pMagicCfgList[]` loaded from server on login
- `m_cMagicMastery[]` tracks learned spells
- `iGetManaCost()` calculates actual mana cost
- `UseMagic()` initiates casting

### With Skill System (Skill.h/cpp)

- `m_pSkillCfgList[]` loaded from server on login
- `m_cSkillMastery[]` tracks skill percentages (0-100)
- `m_iDownSkillIndex` tracks hotkeyed skill

### With Inventory System

- Equipment affects mana costs (wands, necklaces)
- Some skills require held items
- Crafting skill opens item dialog

### With Combat System

- `m_bSkillUsingStatus` prevents overlapping skill use
- `m_bIsSafeAttackMode` affects mana costs
- Combat skills integrate with attack system

---

## State Management

### Dialog State Variables

```cpp
// Spellbook (Dialog 3)
m_stDialogBoxInfo[3].sView    // Current circle (0-9)
m_stDialogBoxInfo[3].sX/sY    // Position

// Magic Shop (Dialog 16)
m_stDialogBoxInfo[16].sView   // Current circle (0-9)
m_stDialogBoxInfo[16].sX/sY   // Position

// Skills (Dialog 15)
m_stDialogBoxInfo[15].sView   // Scroll offset (0 to 43)
m_stDialogBoxInfo[15].sX/sY   // Position
```

### Global State

```cpp
m_bIsGetPointingMode    // TRUE when selecting spell target
m_iPointingSpell        // Spell ID being cast
m_bSkillUsingStatus     // TRUE when skill is active
m_iDownSkillIndex       // Currently hotkeyed skill (-1 = none)
```

---

## Known Issues / Technical Debt

1. **Hardcoded Layout** - All pixel positions are magic numbers scattered throughout drawing code
2. **No Tooltips** - Spell/skill descriptions not shown (would require additional data)
3. **Duplicate Code** - Circle navigation logic duplicated between spellbook and magic shop
4. **String Matching** - Mana-saving items detected by name string matching, fragile
5. **Mixed Concerns** - Drawing, input, and game logic all in same functions
6. **No Caching** - Mana cost recalculated every frame during hover
7. **Fixed Resolution** - Layout assumes 800x600, no scaling

---

## Modernization Notes

### Suggested Improvements

1. **Data-Driven Layout** - Define dialog layouts in YAML/JSON
2. **Separate Components** - Split drawing, input handling, and logic
3. **Spell Tooltips** - Add hover tooltips with spell descriptions
4. **Keyboard Navigation** - Support keyboard for accessibility
5. **Spell Search** - Add search/filter functionality
6. **Drag to Hotbar** - Drag spells directly to action bar
7. **Skill Categories** - Group skills by type (combat, crafting, etc.)
8. **Resolution Scaling** - Support arbitrary resolutions

### Modern Class Structure

```cpp
namespace hb::ui {

class SpellbookDialog : public Dialog {
public:
    void setSpells(std::span<const Spell> spells);
    void setCurrentCircle(uint8_t circle);

    Signal<SpellId> onSpellCast;
    Signal<SpellId> onSpellHotkeyAssign;

private:
    std::array<std::vector<SpellId>, 10> m_spellsByCircle;
    uint8_t m_currentCircle = 0;
};

class SkillsDialog : public Dialog {
public:
    void setSkills(std::span<const Skill> skills);
    void setFilter(SkillCategory category);

    Signal<SkillId> onSkillActivate;
    Signal<SkillId> onSkillHotkey;

private:
    std::vector<SkillId> m_filteredSkills;
    int m_scrollOffset = 0;
};

}
```
