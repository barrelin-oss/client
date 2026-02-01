# Legacy Character System Documentation

## Overview

The Character System in the Helbreath client manages all player character data, including creation, selection, statistics, appearance, level progression, and faction affiliation. This system is spread across multiple files with the core logic residing in the monolithic `CGame` class.

---

## Table of Contents

1. [Core Data Structures](#1-core-data-structures)
2. [Character Properties](#2-character-properties)
3. [Character Statistics](#3-character-statistics)
4. [Appearance System](#4-appearance-system)
5. [Character Creation](#5-character-creation)
6. [Character Selection](#6-character-selection)
7. [Character Initialization](#7-character-initialization)
8. [Level Up System](#8-level-up-system)
9. [Faction System](#9-faction-system)
10. [Network Protocol](#10-network-protocol)
11. [Derived Statistics Formulas](#11-derived-statistics-formulas)
12. [Constants and Limits](#12-constants-and-limits)
13. [Related Functions](#13-related-functions)

---

## 1. Core Data Structures

### 1.1 CCharInfo Class

**File**: `CharInfo.h`, `CharInfo.cpp`

The `CCharInfo` class stores character data for the character selection screen. Up to 4 characters can be stored per account.

```cpp
class CCharInfo
{
public:
    CCharInfo();
    virtual ~CCharInfo();

    // Character identification
    char m_cName[12];           // Character name (max 10 chars + null)
    char m_cMapName[12];        // Current map location name

    // Appearance data
    short m_sSkinCol;           // Skin color (1-3)
    short m_sSex;               // Gender (1=Male, 2=Female, NULL=empty slot)
    short m_sAppr1;             // Appearance value 1 (hair style, color, underwear)
    short m_sAppr2;             // Appearance value 2 (armor/equipment)
    short m_sAppr3;             // Appearance value 3 (weapon)
    short m_sAppr4;             // Appearance value 4 (shield/accessory)

    // Base statistics
    short m_sStr;               // Strength
    short m_sVit;               // Vitality
    short m_sDex;               // Dexterity
    short m_sInt;               // Intelligence
    short m_sMag;               // Magic
    short m_sChr;               // Charisma

    // Progression
    CInt  m_sLevel;             // Character level (CInt for anti-hack protection)
    int   m_iExp;               // Experience points
    int   m_iApprColor;         // Appearance color modifier

    // Creation timestamp
    int   m_iYear;              // Year created
    int   m_iMonth;             // Month created
    int   m_iDay;               // Day created
    int   m_iHour;              // Hour created
    int   m_iMinute;            // Minute created
};
```

### 1.2 CInt Anti-Cheat Wrapper

**File**: `Cint.h`

The `CInt` class wraps integer values with memory protection to prevent memory editing cheats.

```cpp
class CInt
{
public:
    CInt();
    ~CInt();
    int Get();
    void Set(const int & iValue);
    CInt & operator=(const int &iValue);
    operator int();

protected:
    int * m_pValue;         // Pointer to actual value (stored in protected memory)
    int m_iKeyValue;        // XOR key for value obfuscation
    unsigned long old;      // Previous memory protection flags
};
```

**Usage**: Critical character values like `m_sLevel`, `m_iHP`, `m_iMP`, `m_iSP` use `CInt` instead of plain `int` to prevent memory editing.

### 1.3 Character List in CGame

**File**: `Game.h`

```cpp
class CCharInfo * m_pCharList[4];  // Array of 4 character slots per account
```

---

## 2. Character Properties

### 2.1 Active Player Character Variables

These variables in `CGame` represent the currently logged-in character:

```cpp
// Identity
char m_cPlayerName[12];         // Character name
char m_cAccountName[12];        // Account name
char m_cAccountPassword[12];    // Account password (for session)

// Location
char m_cLocation[12];           // Faction affiliation (aresden/elvine/etc.)
char m_cCurLocation[12];        // Current map location
char m_cMapName[12];            // Current map name

// Guild
char m_cGuildName[22];          // Guild name (20 chars max)
int  m_iGuildRank;              // Guild rank (-1 = not in guild)
int  m_iTotalGuildsMan;         // Total guild members

// Position
short m_sPlayerX, m_sPlayerY;   // World coordinates
short m_sPlayerObjectID;        // Network object ID
short m_sPlayerType;            // Character type (1-6)
char  m_cPlayerDir;             // Facing direction (1-8)
short m_sPlayerStatus;          // Status flags

// Appearance
short m_sPlayerAppr1;           // Appearance value 1
short m_sPlayerAppr2;           // Appearance value 2
short m_sPlayerAppr3;           // Appearance value 3
short m_sPlayerAppr4;           // Appearance value 4
int   m_iPlayerApprColor;       // Color modifier

// Character creation temporary values
char m_cGender;                 // 1=Male, 2=Female
char m_cSkinCol;                // 1-3 skin color
char m_cHairStyle;              // 0-7 hair style
char m_cHairCol;                // 0-15 hair color
char m_cUnderCol;               // 0-7 underwear color

// Initial stat allocation (creation)
char m_ccStr, m_ccVit, m_ccDex; // Creation stats
char m_ccInt, m_ccMag, m_ccChr;
```

### 2.2 Player Type Mapping

The `m_sPlayerType` determines the base character model:

| Type | Description |
|------|-------------|
| 1 | Male, Skin Color 1 (Light) |
| 2 | Male, Skin Color 2 (Medium) |
| 3 | Male, Skin Color 3 (Dark) |
| 4 | Female, Skin Color 1 (Light) |
| 5 | Female, Skin Color 2 (Medium) |
| 6 | Female, Skin Color 3 (Dark) |

**Calculation**:
```cpp
// From character creation
switch (m_cGender) {
case 1: _tmp_sOwnerType = 1; break;  // Male base
case 2: _tmp_sOwnerType = 4; break;  // Female base
}
_tmp_sOwnerType += m_cSkinCol - 1;   // Add skin color offset
```

---

## 3. Character Statistics

### 3.1 Primary Statistics

| Stat | Variable | Range | Description |
|------|----------|-------|-------------|
| Strength | `m_iStr` | 10-200 | Physical damage, carry weight, stamina |
| Vitality | `m_iVit` | 10-200 | Hit points, physical defense |
| Dexterity | `m_iDex` | 10-200 | Attack speed, accuracy, dodge |
| Intelligence | `m_iInt` | 10-200 | Mana points, magic damage |
| Magic | `m_iMag` | 10-200 | Mana points, spell power |
| Charisma | `m_iCharisma` | 10-200 | NPC prices, guild requirements |

### 3.2 Derived Statistics

| Stat | Variable | Formula |
|------|----------|---------|
| Hit Points (HP) | `m_iHP` | `Vit * 3 + Level * 2 + Str / 2` |
| Mana Points (MP) | `m_iMP` | `Mag * 2 + Level * 2 + Int / 2` |
| Stamina Points (SP) | `m_iSP` | `Str * 2 + Level * 2` |
| Max Carry Weight | - | `Str * 5 + Level * 5` |
| Armor Class (AC) | `m_iAC` | Server-calculated defense rating |
| THAC0 | `m_iTHAC0` | Server-calculated hit rating |

### 3.3 Combat Statistics

```cpp
int m_iAC;                      // Armor Class (defense)
int m_iTHAC0;                   // To Hit Armor Class 0 (attack rating)
int m_iEnemyKillCount;          // Total enemy kills
int m_iPKCount;                 // Player kill count
int m_iRewardGold;              // Bounty gold on head
int m_iContribution;            // War contribution points
int m_iSuperAttackLeft;         // Remaining super attacks
int m_iWarContribution;         // Crusade contribution
```

### 3.4 Progression Statistics

```cpp
int m_iLevel;                   // Current level (1-180+)
int m_iExp;                     // Current experience points
int m_iLU_Point;                // Unspent level-up points (3 per level)

// Level-up point allocation (pending)
char m_cLU_Str;                 // Points to add to Str
char m_cLU_Vit;                 // Points to add to Vit
char m_cLU_Dex;                 // Points to add to Dex
char m_cLU_Int;                 // Points to add to Int
char m_cLU_Mag;                 // Points to add to Mag
char m_cLU_Char;                // Points to add to Chr
```

---

## 4. Appearance System

### 4.1 Appearance Value Encoding

Character appearance is encoded into 4 short integers (`m_sAppr1` through `m_sAppr4`):

#### Appr1 - Hair and Underwear
```cpp
m_sAppr1 = (HairStyle << 8) | (HairColor << 4) | (UnderwearColor);

// Decoding:
UnderwearColor = m_sAppr1 & 0x000F;         // Bits 0-3
HairColor      = (m_sAppr1 & 0x00F0) >> 4;  // Bits 4-7
HairStyle      = (m_sAppr1 & 0x0F00) >> 8;  // Bits 8-11
```

| Component | Bits | Range | Values |
|-----------|------|-------|--------|
| Underwear Color | 0-3 | 0-7 | 8 colors |
| Hair Color | 4-7 | 0-15 | 16 colors |
| Hair Style | 8-11 | 0-7 | 8 styles |

#### Appr2 - Armor/Body Equipment
Encodes the visual appearance of body armor and clothing.

#### Appr3 - Weapon
Encodes the equipped weapon sprite.

#### Appr4 - Shield/Accessory
Encodes shield and accessory visuals.

### 4.2 Hair Color RGB Values

**Function**: `_GetHairColorRGB(int iColorType, int* pR, int* pG, int* pB)`

| Index | Color | Approximate RGB |
|-------|-------|-----------------|
| 0 | Black | Dark |
| 1-3 | Brown variations | Brown spectrum |
| 4-6 | Blonde variations | Yellow spectrum |
| 7-9 | Red variations | Red spectrum |
| 10-12 | Gray variations | Gray spectrum |
| 13-15 | Special colors | Various |

### 4.3 Skin Color Options

| Value | Description |
|-------|-------------|
| 1 | Light skin |
| 2 | Medium skin |
| 3 | Dark skin |

### 4.4 Gender Values

| Value | Description |
|-------|-------------|
| 1 | Male |
| 2 | Female |
| NULL (0) | Empty character slot |

---

## 5. Character Creation

### 5.1 Creation Flow

1. **Initialize Creation Screen**: `_InitOnCreateNewCharacter()`
2. **Display Creation UI**: `UpdateScreen_OnCreateNewCharacter()`
3. **Draw Character Preview**: `_bDraw_OnCreateNewCharacter()`
4. **Submit to Server**: `MSGID_REQUEST_CREATENEWCHARACTER`
5. **Receive Response**: `LogResponseHandler()` → `DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED`

### 5.2 Initialization Function

**Function**: `_InitOnCreateNewCharacter()` (Game.cpp:15484)

```cpp
void CGame::_InitOnCreateNewCharacter()
{
    m_cGender    = rand() % 2 + 1;    // Random gender (1 or 2)
    m_cSkinCol   = rand() % 3 + 1;    // Random skin (1-3)
    m_cHairStyle = rand() % 8;        // Random hair style (0-7)
    m_cHairCol   = rand() % 16;       // Random hair color (0-15)
    m_cUnderCol  = rand() % 8;        // Random underwear (0-7)

    // All stats start at 10
    m_ccStr = 10;
    m_ccVit = 10;
    m_ccDex = 10;
    m_ccInt = 10;
    m_ccMag = 10;
    m_ccChr = 10;
}
```

### 5.3 Stat Point Allocation

- **Total Points Available**: 70 (base 60 + 10 bonus)
- **Initial Stats**: 6 stats × 10 = 60
- **Distributable Points**: 10 (70 - 60)
- **Min Stat Value**: 10
- **Max Stat Value**: 14
- **Max Per Stat Gain**: 4 points

```cpp
// Point calculation
iPoint = m_ccStr + m_ccVit + m_ccDex + m_ccInt + m_ccMag + m_ccChr;
iPoint = 70 - iPoint;  // Remaining points to distribute
```

### 5.4 Preset Stat Templates

The creation screen offers three preset configurations:

#### Warrior Preset (Button 26)
```cpp
m_ccStr = 14;  m_ccVit = 12;  m_ccDex = 14;
m_ccInt = 10;  m_ccMag = 10;  m_ccChr = 10;
```

#### Mage Preset (Button 27)
```cpp
m_ccStr = 10;  m_ccVit = 12;  m_ccDex = 10;
m_ccInt = 14;  m_ccMag = 14;  m_ccChr = 10;
```

#### Merchant/Social Preset (Button 28)
```cpp
m_ccStr = 14;  m_ccVit = 10;  m_ccDex = 12;
m_ccInt = 10;  m_ccMag = 10;  m_ccChr = 14;
```

### 5.5 Initial Derived Stats at Creation

```cpp
// Displayed during character creation (level 1)
HP = m_ccVit * 3 + 2 + m_ccStr / 2;    // e.g., 10*3+2+10/2 = 37
MP = m_ccMag * 2 + 2 + m_ccInt / 2;    // e.g., 10*2+2+10/2 = 27
SP = m_ccStr * 2 + 2;                   // e.g., 10*2+2 = 22
```

### 5.6 Name Validation

**Function**: `m_Misc.bCheckValidName(pName)`

- Maximum 10 characters
- No special ASCII codes
- No reserved/bad words (`_bCheckBadWords()`)
- Must pass `bCheckValidString()` validation

### 5.7 Creation Packet Format

**Message ID**: `MSGID_REQUEST_CREATENEWCHARACTER` (0x0FC94204)

```cpp
// Packet structure (77 bytes total)
struct CreateCharacterPacket {
    DWORD dwMsgID;              // Message ID
    WORD  wMsgType;             // Message type
    char  cCharName[10];        // Character name
    char  cAccountName[10];     // Account name
    char  cAccountPassword[10]; // Account password
    char  cWorldServerName[30]; // Server name
    char  cGender;              // 1=Male, 2=Female
    char  cSkinCol;             // 1-3
    char  cHairStyle;           // 0-7
    char  cHairCol;             // 0-15
    char  cUnderCol;            // 0-7
    char  cStr;                 // Initial Strength (10-14)
    char  cVit;                 // Initial Vitality (10-14)
    char  cDex;                 // Initial Dexterity (10-14)
    char  cInt;                 // Initial Intelligence (10-14)
    char  cMag;                 // Initial Magic (10-14)
    char  cChr;                 // Initial Charisma (10-14)
};
```

---

## 6. Character Selection

### 6.1 Selection Flow

1. **Receive Character List**: Server sends `MSGID_RESPONSE_LOG` with character data
2. **Parse Character Data**: `LogResponseHandler()` populates `m_pCharList[]`
3. **Display Selection Screen**: `UpdateScreen_OnSelectCharacter()`
4. **User Selects Character**: Sets `m_wEnterGameType` (1-4)
5. **Enter Game Request**: `MSGID_REQUEST_ENTERGAME`
6. **Initialize Player**: `InitPlayerResponseHandler()`

### 6.2 Character List Parsing

**Location**: Game.cpp lines 14825-14927

```cpp
for (i = 0; i < 4; i++) {
    if (m_pCharList[i] != NULL) {
        delete m_pCharList[i];
        m_pCharList[i] = NULL;
    }

    // Parse character data from server packet
    m_pCharList[i] = new class CCharInfo;
    memcpy(m_pCharList[i]->m_cName, cp, 10);
    cp += 10;

    if (m_pCharList[i]->m_cName[0] == NULL) {
        m_pCharList[i]->m_sSex = NULL;  // Empty slot marker
    }
    else {
        // Parse appearance values
        wp = (WORD *)cp;
        m_pCharList[i]->m_sAppr1 = *wp; cp += 2;
        m_pCharList[i]->m_sAppr2 = *wp; cp += 2;
        m_pCharList[i]->m_sAppr3 = *wp; cp += 2;
        m_pCharList[i]->m_sAppr4 = *wp; cp += 2;

        // Parse basic info
        m_pCharList[i]->m_sSex = *wp;     cp += 2;
        m_pCharList[i]->m_sSkinCol = *wp; cp += 2;
        m_pCharList[i]->m_sLevel = *wp;   cp += 2;

        // Parse experience
        dwp = (DWORD *)cp;
        m_pCharList[i]->m_iExp = *dwp; cp += 4;

        // Parse stats
        m_pCharList[i]->m_sStr = *wp; cp += 2;
        m_pCharList[i]->m_sVit = *wp; cp += 2;
        m_pCharList[i]->m_sDex = *wp; cp += 2;
        m_pCharList[i]->m_sInt = *wp; cp += 2;
        m_pCharList[i]->m_sMag = *wp; cp += 2;
        m_pCharList[i]->m_sChr = *wp; cp += 2;

        // Parse appearance color
        ip = (int *)cp;
        m_pCharList[i]->m_iApprColor = *ip; cp += 4;

        // Parse creation timestamp
        m_pCharList[i]->m_iYear = (int)*wp;   cp += 2;
        m_pCharList[i]->m_iMonth = (int)*wp;  cp += 2;
        m_pCharList[i]->m_iDay = (int)*wp;    cp += 2;
        m_pCharList[i]->m_iHour = (int)*wp;   cp += 2;
        m_pCharList[i]->m_iMinute = (int)*wp; cp += 2;

        // Parse map name
        memcpy(m_pCharList[i]->m_cMapName, cp, 10); cp += 10;
    }
}
```

### 6.3 Character Selection Variables

```cpp
short m_wEnterGameType;         // Selected character slot (1-4)
char  m_cCurFocus;              // Current UI focus
char  m_cMaxFocus;              // Maximum UI focus positions
int   m_iTotalChar;             // Number of characters on account
```

### 6.4 Delete Character Confirmation

Characters under level 50 can be deleted:
```cpp
if ((m_pCharList[m_cCurFocus - 1] != NULL) &&
    (m_pCharList[m_cCurFocus - 1]->m_sLevel < 50)) {
    // Allow deletion
    ChangeGameMode(DEF_GAMEMODE_ONQUERYDELETECHARACTER);
}
```

---

## 7. Character Initialization

### 7.1 Initialization Flow

1. **Enter Game Confirmed**: Server sends `DEF_ENTERGAMERESTYPE_CONFIRM`
2. **Request Init Data**: Client sends `MSGID_REQUEST_INITDATA`
3. **Receive Init Data**: `InitDataResponseHandler()` - map, items, skills
4. **Receive Character Data**: `MSGID_PLAYERCHARACTERCONTENTS`
5. **Initialize Characteristics**: `InitPlayerCharacteristics()`

### 7.2 InitPlayerCharacteristics Function

**Location**: Game.cpp:5452-5592

This function parses the complete character state from the server:

```cpp
void CGame::InitPlayerCharacteristics(char * pData)
{
    char * cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
    int * ip;

    // Core stats
    ip = (int *)cp; m_iHP = *ip;       cp += 4;
    ip = (int *)cp; m_iMP = *ip;       cp += 4;
    ip = (int *)cp; m_iSP = *ip;       cp += 4;
    ip = (int *)cp; m_iAC = *ip;       cp += 4;  // Defense
    ip = (int *)cp; m_iTHAC0 = *ip;    cp += 4;  // Attack rating
    ip = (int *)cp; m_iLevel = *ip;    cp += 4;
    ip = (int *)cp; m_iStr = *ip;      cp += 4;
    ip = (int *)cp; m_iInt = *ip;      cp += 4;
    ip = (int *)cp; m_iVit = *ip;      cp += 4;
    ip = (int *)cp; m_iDex = *ip;      cp += 4;
    ip = (int *)cp; m_iMag = *ip;      cp += 4;
    ip = (int *)cp; m_iCharisma = *ip; cp += 4;

    // Level-up points allocation
    m_cLU_Str = *cp++;
    m_cLU_Vit = *cp++;
    m_cLU_Dex = *cp++;
    m_cLU_Int = *cp++;
    m_cLU_Mag = *cp++;
    m_cLU_Char = *cp++;

    // Calculate remaining level-up points
    m_iLU_Point = 3 - (m_cLU_Str + m_cLU_Vit + m_cLU_Dex +
                       m_cLU_Int + m_cLU_Mag + m_cLU_Char);

    // Experience and combat stats
    ip = (int *)cp; m_iExp = *ip;            cp += 4;
    ip = (int *)cp; m_iEnemyKillCount = *ip; cp += 4;
    ip = (int *)cp; m_iPKCount = *ip;        cp += 4;
    ip = (int *)cp; m_iRewardGold = *ip;     cp += 4;

    // Faction/location
    memcpy(m_cLocation, cp, 10); cp += 10;
    // Parse faction from location string...

    // Guild information
    memcpy(m_cGuildName, cp, 20); cp += 20;
    ip = (int *)cp; m_iGuildRank = *ip; cp += 4;

    // Special abilities
    m_iSuperAttackLeft = (int)*cp++;
    ip = (int *)cp; m_iFightzoneNumber = *ip; cp += 4;
}
```

---

## 8. Level Up System

### 8.1 Level Up Notification

**Message**: `DEF_NOTIFY_LEVELUP` (0x0B16)

**Function**: `NotifyMsg_LevelUp()` (Game.cpp:44368)

```cpp
void CGame::NotifyMsg_LevelUp(char * pData)
{
    int iPrevLevel = m_iLevel;
    char * cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

    // Parse new stats from server
    ip = (int *)cp; m_iLevel = *ip;    cp += 4;
    ip = (int *)cp; m_iStr = *ip;      cp += 4;
    ip = (int *)cp; m_iVit = *ip;      cp += 4;
    ip = (int *)cp; m_iDex = *ip;      cp += 4;
    ip = (int *)cp; m_iInt = *ip;      cp += 4;
    ip = (int *)cp; m_iMag = *ip;      cp += 4;
    ip = (int *)cp; m_iCharisma = *ip; cp += 4;

    // Display level up message
    wsprintf(cTxt, "Level up! Level %d", m_iLevel);
    AddEventList(cTxt, 10);

    // Play gender-appropriate sound
    switch (m_sPlayerType) {
    case 1: case 2: case 3:
        PlaySound('C', 21, 0);  // Male level up
        break;
    case 4: case 5: case 6:
        PlaySound('C', 22, 0);  // Female level up
        break;
    }

    // Display "Level up!" above character
    // ...
}
```

### 8.2 Level Up Point Distribution

Each level grants **3 stat points** to distribute.

**Dialog**: `DrawDialogBox_LevelUpSetting()` (Dialog 12)

**Point Distribution UI**:
- Click + to add point to stat
- Click - to remove point from stat
- Confirm button sends points to server

```cpp
// Adding a point to Strength
if (m_iLU_Point > 0) {
    m_iLU_Point--;
    m_cLU_Str++;
}

// Removing a point from Strength
if (m_cLU_Str > 0) {
    m_cLU_Str--;
    m_iLU_Point++;
}
```

### 8.3 Level Up Validation

**Function**: `_iCheckLUS()` (Game.cpp:14714)

```cpp
int CGame::_iCheckLUS()
{
    // Check if all points allocated
    if (m_iLU_Point != 0) return 1;

    // Check stat caps (max 200 each)
    if ((m_iStr + m_cLU_Str) > 200) return 2;
    if ((m_iDex + m_cLU_Dex) > 200) return 2;
    if ((m_iInt + m_cLU_Int) > 200) return 2;
    if ((m_iVit + m_cLU_Vit) > 200) return 2;
    if ((m_iMag + m_cLU_Mag) > 200) return 2;
    if ((m_iCharisma + m_cLU_Char) > 200) return 2;

    return 0;  // Valid
}
```

### 8.4 Level Up Packet

**Message ID**: `MSGID_LEVELUPSETTINGS` (0x11A01000)

```cpp
// Packet includes allocated points
*cp = m_cLU_Str;  cp++;
*cp = m_cLU_Vit;  cp++;
*cp = m_cLU_Dex;  cp++;
*cp = m_cLU_Int;  cp++;
*cp = m_cLU_Mag;  cp++;
*cp = m_cLU_Char; cp++;
```

---

## 9. Faction System

### 9.1 Faction Types

| Location String | Faction | Citizen | Hunter |
|----------------|---------|---------|--------|
| `aresden` | Aresden | Yes | No |
| `arehunter` | Aresden | Yes | Yes |
| `elvine` | Elvine | Yes | No |
| `elvhunter` | Elvine | Yes | Yes |
| Other | Traveler | No | Yes |

### 9.2 Faction Variables

```cpp
BOOL m_bHunter;     // Is hunter mode enabled
BOOL m_bAresden;    // Is Aresden faction (vs Elvine)
BOOL m_bCitizen;    // Is citizen (vs traveler)
```

### 9.3 Faction Parsing

```cpp
if (memcmp(m_cLocation, "aresden", 7) == 0) {
    m_bAresden = TRUE;
    m_bCitizen = TRUE;
    m_bHunter = FALSE;
}
else if (memcmp(m_cLocation, "arehunter", 9) == 0) {
    m_bAresden = TRUE;
    m_bCitizen = TRUE;
    m_bHunter = TRUE;
}
else if (memcmp(m_cLocation, "elvine", 6) == 0) {
    m_bAresden = FALSE;
    m_bCitizen = TRUE;
    m_bHunter = FALSE;
}
else if (memcmp(m_cLocation, "elvhunter", 9) == 0) {
    m_bAresden = FALSE;
    m_bCitizen = TRUE;
    m_bHunter = TRUE;
}
else {
    // Traveler (not affiliated)
    m_bAresden = TRUE;   // Default display
    m_bCitizen = FALSE;
    m_bHunter = TRUE;
}
```

### 9.4 Faction Requirements

- **Guild Creation**: Requires Charisma ≥ 20 AND Level ≥ 20
- **Citizen Status**: Required for some game features

```cpp
if (m_iCharisma < 20) return;
if (m_iLevel < 20) return;
if (m_bCitizen == FALSE) return;
```

---

## 10. Network Protocol

### 10.1 Character-Related Message IDs

| Message ID | Name | Direction | Purpose |
|------------|------|-----------|---------|
| `0x0FC94201` | `MSGID_REQUEST_LOGIN` | C→S | Login request |
| `0x0FC94203` | `MSGID_RESPONSE_LOG` | S→C | Login response with char list |
| `0x0FC94204` | `MSGID_REQUEST_CREATENEWCHARACTER` | C→S | Create character |
| `0x0FC94205` | `MSGID_REQUEST_ENTERGAME` | C→S | Enter game with character |
| `0x0FC94206` | `MSGID_RESPONSE_ENTERGAME` | S→C | Enter game response |
| `0x0FC94207` | `MSGID_REQUEST_DELETECHARACTER` | C→S | Delete character |
| `0x05040205` | `MSGID_REQUEST_INITPLAYER` | C→S | Request player init |
| `0x05040206` | `MSGID_RESPONSE_INITPLAYER` | S→C | Player init response |
| `0x0FA40000` | `MSGID_PLAYERCHARACTERCONTENTS` | S→C | Full character data |
| `0x11A01000` | `MSGID_LEVELUPSETTINGS` | C→S | Level up point allocation |

### 10.2 Character Notification Messages

| Notify ID | Name | Purpose |
|-----------|------|---------|
| `0x0B07` | `DEF_NOTIFY_HP` | HP change |
| `0x0B14` | `DEF_NOTIFY_MP` | MP change |
| `0x0B15` | `DEF_NOTIFY_SP` | SP change |
| `0x0B0A` | `DEF_NOTIFY_EXP` | Experience gained |
| `0x0B16` | `DEF_NOTIFY_LEVELUP` | Level up |
| `0x0B23` | `DEF_NOTIFY_SKILL` | Skill change |
| `0x0B32` | `DEF_NOTIFY_CHARISMA` | Charisma change |

### 10.3 Log Response Types

| Response Type | Name | Meaning |
|---------------|------|---------|
| `0x0F14` | `DEF_LOGRESMSGTYPE_CONFIRM` | Success |
| `0x0F15` | `DEF_LOGRESMSGTYPE_REJECT` | Rejected |
| `0x0F1C` | `DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED` | Character created |
| `0x0F1D` | `DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED` | Creation failed |
| `0x0F1E` | `DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER` | Name taken |
| `0x0F1F` | `DEF_LOGRESMSGTYPE_CHARACTERDELETED` | Deleted |

---

## 11. Derived Statistics Formulas

### 11.1 Experience Required Per Level

**Function**: `iGetLevelExp()` (Game.cpp:23051)

```cpp
int CGame::iGetLevelExp(int iLevel)
{
    if (iLevel == 0) return 0;

    // Recursive formula
    return iGetLevelExp(iLevel - 1) +
           iLevel * (50 + (iLevel * (iLevel / 17) * (iLevel / 17)));
}
```

**Experience Table (Sample)**:
| Level | Exp Required | Cumulative |
|-------|-------------|------------|
| 1 | 50 | 50 |
| 10 | 500 | ~2,750 |
| 20 | 1,000 | ~11,000 |
| 50 | 2,850 | ~70,000 |
| 100 | 16,000 | ~850,000 |

### 11.2 HP/MP/SP Formulas

```cpp
// Maximum Hit Points
MaxHP = Vitality * 3 + Level * 2 + Strength / 2

// Maximum Mana Points
MaxMP = Magic * 2 + Level * 2 + Intelligence / 2

// Maximum Stamina Points
MaxSP = Strength * 2 + Level * 2

// Maximum Carry Weight (in units of 100)
MaxWeight = Strength * 5 + Level * 5
```

### 11.3 Initial Character Formulas (Level 1)

```cpp
// At creation with base 10 stats:
InitialHP = 10 * 3 + 1 * 2 + 10 / 2 = 30 + 2 + 5 = 37
InitialMP = 10 * 2 + 1 * 2 + 10 / 2 = 20 + 2 + 5 = 27
InitialSP = 10 * 2 + 1 * 2 = 20 + 2 = 22
```

### 11.4 Shop Discount Formula

```cpp
// Charisma affects shop prices
iDiscountRatio = (m_iCharisma - 10) / 4;
// At Charisma 10: 0% discount
// At Charisma 50: 10% discount
// At Charisma 100: 22% discount
```

### 11.5 Magic Casting Bonus

```cpp
// Intelligence bonus for magic
if (m_iInt > 50)
    iResult += (m_iInt - 50) / 2;

// Level-based magic bonus
sLevelMagic = (m_iLevel / 10);
```

---

## 12. Constants and Limits

### 12.1 Character Limits

| Constant | Value | Description |
|----------|-------|-------------|
| Max Characters Per Account | 4 | `m_pCharList[4]` |
| Max Character Name Length | 10 | `m_cName[12]` |
| Max Account Name Length | 10 | `m_cAccountName[12]` |
| Max Guild Name Length | 20 | `m_cGuildName[22]` |
| Max Map Name Length | 10 | `m_cMapName[12]` |

### 12.2 Stat Limits

| Constant | Value | Description |
|----------|-------|-------------|
| Initial Stat Value | 10 | Base for all stats |
| Max Initial Stat | 14 | Maximum at creation |
| Min Stat Value | 10 | Cannot go below |
| Max Stat Value | 200 | Hard cap |
| Level Up Points | 3 | Per level |
| Total Creation Points | 70 | 60 base + 10 bonus |

### 12.3 Appearance Limits

| Constant | Value | Description |
|----------|-------|-------------|
| Gender Values | 1-2 | Male, Female |
| Skin Colors | 1-3 | Light, Medium, Dark |
| Hair Styles | 0-7 | 8 styles |
| Hair Colors | 0-15 | 16 colors |
| Underwear Colors | 0-7 | 8 colors |

### 12.4 Equipment Positions

**File**: `Item.h`

```cpp
#define DEF_MAXITEMEQUIPPOS     15      // Total equipment slots

#define DEF_EQUIPPOS_NONE       0       // Not equippable
#define DEF_EQUIPPOS_HEAD       1       // Helmet
#define DEF_EQUIPPOS_BODY       2       // Chest armor
#define DEF_EQUIPPOS_ARMS       3       // Gloves
#define DEF_EQUIPPOS_PANTS      4       // Leggings
#define DEF_EQUIPPOS_BOOTS      5       // Boots
#define DEF_EQUIPPOS_NECK       6       // Necklace
#define DEF_EQUIPPOS_LHAND      7       // Left hand (shield)
#define DEF_EQUIPPOS_RHAND      8       // Right hand (weapon)
#define DEF_EQUIPPOS_TWOHAND    9       // Two-handed weapon
#define DEF_EQUIPPOS_RFINGER    10      // Right ring
#define DEF_EQUIPPOS_LFINGER    11      // Left ring
#define DEF_EQUIPPOS_BACK       12      // Cape/cloak
#define DEF_EQUIPPOS_FULLBODY   13      // Full body armor
```

---

## 13. Related Functions

### 13.1 Character Creation Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `_InitOnCreateNewCharacter()` | Game.cpp:15484 | Initialize creation defaults |
| `UpdateScreen_OnCreateNewCharacter()` | Game.cpp:25923 | Creation screen game loop |
| `_bDraw_OnCreateNewCharacter()` | Game.cpp:25819 | Draw creation UI |
| `ClearContents_OnSelectCharacter()` | Game.cpp:21533 | Clear selection data |

### 13.2 Character Selection Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `UpdateScreen_OnSelectCharacter()` | Game.cpp:21539 | Selection screen game loop |
| `LogResponseHandler()` | Game.cpp:14800+ | Parse character list |
| `ClearContents_OnSelectCharacter()` | Game.cpp:21533 | Clear selection state |

### 13.3 Character Initialization Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `InitPlayerResponseHandler()` | Game.cpp:3085 | Handle init response |
| `InitPlayerCharacteristics()` | Game.cpp:5452 | Parse full char data |
| `InitDataResponseHandler()` | Game.cpp | Handle init data |

### 13.4 Character Dialog Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `DrawDialogBox_Character()` | Game.cpp:37450+ | Draw character dialog |
| `DlgBoxClick_Character()` | Game.cpp | Handle char dialog clicks |
| `DrawDialogBox_LevelUpSetting()` | Game.cpp:38100+ | Level up dialog |
| `DlgBoxClick_LevelUpSettings()` | Game.cpp:20700+ | Level up click handling |

### 13.5 Character Rendering Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `_Draw_CharacterBody()` | Game.cpp | Draw character body |
| `DrawObject_OnMove_ForMenu()` | Game.cpp | Draw animated char |
| `_GetHairColorRGB()` | Game.cpp | Get hair color values |

### 13.6 Utility Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `iGetLevelExp()` | Game.cpp:23051 | Calculate exp for level |
| `_iCheckLUS()` | Game.cpp:14714 | Validate level up settings |
| `_iCalcTotalWeight()` | Game.cpp | Calculate inventory weight |

---

## Appendix A: Character Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     CHARACTER LIFECYCLE                          │
└─────────────────────────────────────────────────────────────────┘

    ┌──────────────┐
    │ Main Menu    │
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐     MSGID_REQUEST_LOGIN
    │ Login Screen │ ─────────────────────────────► Log Server
    └──────┬───────┘
           │
           │ ◄─────────────────────────────────── Character List
           ▼                                      (MSGID_RESPONSE_LOG)
    ┌──────────────────┐
    │ Character Select │ ◄── m_pCharList[4] populated
    └──────┬───────────┘
           │
    ┌──────┴──────┐
    │             │
    ▼             ▼
┌────────┐   ┌─────────────────┐
│ Delete │   │ Create New Char │
└────────┘   └────────┬────────┘
                      │
                      │ MSGID_REQUEST_CREATENEWCHARACTER
                      ▼
               ┌──────────────┐
               │ Enter Game   │
               └──────┬───────┘
                      │
                      │ MSGID_REQUEST_ENTERGAME
                      ▼
               ┌──────────────────┐
               │ Game Server Conn │
               └──────┬───────────┘
                      │
                      │ MSGID_REQUEST_INITPLAYER
                      │ MSGID_REQUEST_INITDATA
                      ▼
               ┌──────────────────────────┐
               │ InitPlayerCharacteristics │
               │ - Stats loaded           │
               │ - Equipment loaded       │
               │ - Skills loaded          │
               └──────┬───────────────────┘
                      │
                      ▼
               ┌──────────────┐
               │ Main Game    │
               │ (Playing)    │
               └──────────────┘
```

---

## Appendix B: Character Selection Screen Layout

```
┌─────────────────────────────────────────────────────────────┐
│                    CHARACTER SELECT                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Char 1  │  │ Char 2  │  │ Char 3  │  │ Char 4  │        │
│  │         │  │         │  │         │  │ (Empty) │        │
│  │ [sprite]│  │ [sprite]│  │ [sprite]│  │         │        │
│  │         │  │         │  │         │  │         │        │
│  ├─────────┤  ├─────────┤  ├─────────┤  ├─────────┤        │
│  │ Name    │  │ Name    │  │ Name    │  │ New     │        │
│  │ Level   │  │ Level   │  │ Level   │  │ Char    │        │
│  │ Exp     │  │ Exp     │  │ Exp     │  │         │        │
│  │ Created │  │ Created │  │ Created │  │         │        │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘        │
│                                                             │
│  [Enter Game]     [Delete Character]     [Cancel]           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Appendix C: Character Creation Screen Layout

```
┌─────────────────────────────────────────────────────────────┐
│                   CREATE NEW CHARACTER                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Character Name: [____________]                             │
│                                                             │
│  Appearance:                    │  Preview:                 │
│  ┌────────────────────────────┐ │  ┌─────────────────────┐  │
│  │ Gender:    [◄] Male   [►]  │ │  │                     │  │
│  │ Skin:      [◄] Light  [►]  │ │  │    [Character]      │  │
│  │ Hair Style:[◄] Style1 [►]  │ │  │    [Animation]      │  │
│  │ Hair Color:[◄] Black  [►]  │ │  │                     │  │
│  │ Underwear: [◄] White  [►]  │ │  └─────────────────────┘  │
│  └────────────────────────────┘ │                           │
│                                 │  HP: 37                   │
│  Stats (10 points to spend):    │  MP: 27                   │
│  ┌────────────────────────────┐ │  SP: 22                   │
│  │ Str: [◄] 10 [►]            │ │                           │
│  │ Vit: [◄] 10 [►]            │ │                           │
│  │ Dex: [◄] 10 [►]            │ │                           │
│  │ Int: [◄] 10 [►]            │ │                           │
│  │ Mag: [◄] 10 [►]            │ │                           │
│  │ Chr: [◄] 10 [►]            │ │                           │
│  └────────────────────────────┘ │                           │
│                                                             │
│  [Warrior] [Mage] [Merchant]   [Create] [Cancel]            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Appendix D: Complete Member Variable Reference

### CGame Character Variables

```cpp
// Character Identity
char m_cPlayerName[12];
char m_cAccountName[12];
char m_cAccountPassword[12];
WORD m_wEnterGameType;

// Character Position
short m_sPlayerX, m_sPlayerY;
short m_sPlayerObjectID;
short m_sPlayerType;
char  m_cPlayerDir;
short m_sPlayerStatus;

// Character Appearance
short m_sPlayerAppr1, m_sPlayerAppr2;
short m_sPlayerAppr3, m_sPlayerAppr4;
int   m_iPlayerApprColor;

// Creation Appearance
char m_cGender;
char m_cSkinCol;
char m_cHairStyle;
char m_cHairCol;
char m_cUnderCol;

// Creation Stats
char m_ccStr, m_ccVit, m_ccDex;
char m_ccInt, m_ccMag, m_ccChr;

// Active Stats (CInt for protection)
CInt m_iHP, m_iMP, m_iSP;
int  m_iAC, m_iTHAC0;
int  m_iLevel;
int  m_iStr, m_iInt, m_iVit;
int  m_iDex, m_iMag, m_iCharisma;
int  m_iExp;

// Level Up
int  m_iLU_Point;
char m_cLU_Str, m_cLU_Vit, m_cLU_Dex;
char m_cLU_Int, m_cLU_Mag, m_cLU_Char;

// Combat Stats
int m_iEnemyKillCount;
int m_iPKCount;
int m_iRewardGold;
int m_iContribution;
int m_iSuperAttackLeft;
int m_iWarContribution;

// Location/Faction
char m_cLocation[12];
char m_cCurLocation[12];
char m_cMapName[12];
BOOL m_bHunter;
BOOL m_bAresden;
BOOL m_bCitizen;

// Guild
char m_cGuildName[22];
int  m_iGuildRank;
int  m_iTotalGuildsMan;

// Character List (Selection)
CCharInfo * m_pCharList[4];
int m_iTotalChar;
```

---

*Documentation generated from legacy Helbreath client source code analysis.*
*Last updated: Based on code circa 2002-2003*
