# PK (Player Killing) and Rating System

## Overview

The Helbreath legacy client implements a sophisticated Player Killing (PK) system that distinguishes between legitimate combat (enemy faction kills) and criminal behavior (killing innocent players). The system tracks PK counts, applies penalties, restricts criminal players from certain activities, and provides visual feedback through name coloring.

## Source Files

- `Game.cpp` / `Game.h` - Main PK logic, status display, combat restrictions
- `NetMessages.h` - Network message definitions for PK notifications
- `lan_eng.h` (and other `lan_*.h`) - Localized strings for PK messages

## Key Data Structures

### Player Status Flags (Game.h:695-754)

```cpp
// Faction and citizenship
BOOL m_bHunter;      // TRUE = Civilian/Hunter, FALSE = Combatant/Soldier
BOOL m_bAresden;     // TRUE = Aresden faction, FALSE = Elvine faction
BOOL m_bCitizen;     // TRUE = Has citizenship, FALSE = Traveller

// Combat modes
BOOL m_bIsCombatMode;       // Attack stance active (weapon drawn)
BOOL m_bIsSafeAttackMode;   // Safe attack mode (prevents accidental PK)
BOOL m_bForceAttack;        // Force attack mode (allows attacking friendlies)
BOOL m_bIsCrusadeMode;      // Crusade/war event active

// PK tracking
int m_iEnemyKillCount;      // Legitimate enemy faction kills
int m_iPKCount;             // Criminal kills (innocent players)
int m_iContribution;        // Contribution points
int m_iWarContribution;     // War/crusade contribution

// Commented out (was planned but not implemented)
// int m_iRating;           // Rating system (commented in source)
```

### Object Status Bit Flags

The `sStatus` field in player objects encodes PK and faction information:

```cpp
// Status bit masks (from DrawObjectName and _iGetFOE)
0x8000  // Bit 15: PK flag (criminal)
0x4000  // Bit 14: Citizen flag
0x2000  // Bit 13: Aresden flag (FALSE = Elvine)
0x1000  // Bit 12: Hunter/Civilian flag
0x0040  // Bit 6:  Frozen status
0x0020  // Bit 5:  Berserk status
0x0010  // Bit 4:  Invisible status
```

## Core Functions

### Friend or Enemy Detection (_iGetFOE)

**Location**: `Game.cpp:24628-24658`

This critical function determines the relationship between the local player and another player based on their status flags.

```cpp
int CGame::_iGetFOE(short sStatus)
{
    BOOL bPK, bCitizen, bAresden, bHunter;

    // If local player is a criminal, everyone is neutral
    if (m_iPKCount != 0) return -1;

    // Decode target's status
    if (sStatus & 0x8000) bPK = TRUE;       // Target is criminal
    if (sStatus & 0x4000) bCitizen = TRUE;  // Target is citizen
    if (sStatus & 0x2000) bAresden = TRUE;  // Target is Aresden
    if (sStatus & 0x1000) bHunter = TRUE;   // Target is hunter/civilian

    if (bPK == TRUE) return -2;             // Criminal = Enemy
    if (bCitizen == FALSE) return 0;        // Traveller = Neutral

    // Target is citizen, not criminal
    if (m_bCitizen == FALSE) return 0;      // Local is traveller = Neutral

    // Same faction check
    if ((m_bAresden == TRUE) && (bAresden == TRUE)) return 1;   // Friendly
    if ((m_bAresden == FALSE) && (bAresden == FALSE)) return 1; // Friendly

    // Different factions
    if (m_bIsCrusadeMode == TRUE) return -1; // Crusade mode = Enemy

    // Non-crusade: Combatants vs Combatants are enemies
    if ((m_bHunter == FALSE) && (bHunter == FALSE)) return -1;
    else return 0; // At least one is civilian = Neutral
}
```

**Return Values**:
| Value | Meaning | Name Color |
|-------|---------|------------|
| -2 | Enemy (Criminal) | Red (255, 0, 0) |
| -1 | Enemy (Faction) | Red (255, 0, 0) |
| 0 | Neutral | Blue (50, 50, 255) |
| 1 | Friendly | Green (30, 200, 30) |

### Drawing Player Names with PK Status

**Location**: `Game.cpp:33462-33618`

```cpp
void CGame::DrawObjectName(short sX, short sY, char * pName, short sStatus)
{
    // Determine base color from FOE status
    iFOE = _iGetFOE(sStatus);
    if (iFOE < 0) { sR = 255; sG = 0; sB = 0; }      // Enemy = Red
    else if (iFOE == 0) { sR = 50; sG = 50; sB = 255; } // Neutral = Blue
    else { sR = 30; sG = 200; sB = 30; }              // Friendly = Green

    // Draw player name in white
    PutString2(sX, sY, cTxt, 255, 255, 255);

    // For local player, check own PK status
    if (memcmp(m_cPlayerName, pName, 10) == 0) {
        if (m_iPKCount != 0) {
            bPK = TRUE;
            sR = 255; sG = 0; sB = 0;  // Force red for criminals
        }
    }
    else {
        // Extract PK flag from status
        if (sStatus & 0x8000) bPK = TRUE;
    }

    // Build faction/status label
    if (bCitizen == FALSE) strcpy(cTxt, "Traveller");
    else {
        if (bAresden) {
            if (bHunter) strcpy(cTxt, "Aresden Civilian");
            else strcpy(cTxt, "Aresden Combatant");
        }
        else {
            if (bHunter) strcpy(cTxt, "Elvine Civilian");
            else strcpy(cTxt, "Elvine Combatant");
        }
    }

    // Override with criminal label if PK
    if (bPK == TRUE) {
        if (bCitizen == FALSE) strcpy(cTxt, "Criminal");
        else {
            if (bAresden) strcpy(cTxt, "Aresden Criminal");
            else strcpy(cTxt, "Elvine Criminal");
        }
    }

    // Draw status label in appropriate color
    PutString2(sX, sY + 14 + iAddY, cTxt, sR, sG, sB);
}
```

### PK Penalty Handler

**Location**: `Game.cpp:44810-44866`

When a player kills an innocent player, the server sends a penalty notification:

```cpp
void CGame::NotifyMsg_PKpenalty(char *pData)
{
    // Parse penalty data from server
    int iExp, iStr, iVit, iDex, iInt, iMag, iChr, iPKcount;

    iExp = *(DWORD*)(cp);      cp += 4;
    iStr = *(DWORD*)(cp);      cp += 4;
    iVit = *(DWORD*)(cp);      cp += 4;
    iDex = *(DWORD*)(cp);      cp += 4;
    iInt = *(DWORD*)(cp);      cp += 4;
    iMag = *(DWORD*)(cp);      cp += 4;
    iChr = *(DWORD*)(cp);      cp += 4;
    iPKcount = *(DWORD*)(cp);  cp += 4;

    // Display penalty message
    wsprintf(G_cTxt, "You received a PK penalty because you killed an innocent player! PK-Count(%d)", iPKcount);
    AddEventList(G_cTxt, 10);

    // Show exp loss
    if (m_iExp > iExp) {
        wsprintf(G_cTxt, "Exp has been decreased by %d points!", m_iExp - iExp);
        AddEventList(G_cTxt, 10);
    }

    // Apply penalties to stats
    m_iExp = iExp;
    m_iStr = iStr;
    m_iVit = iVit;
    m_iDex = iDex;
    m_iInt = iInt;
    m_iMag = iMag;
    m_iCharisma = iChr;
    m_iPKCount = iPKcount;
}
```

**Penalty Structure**:
- **Experience Loss**: Immediate EXP reduction
- **Stat Reduction**: All six stats can be penalized
- **PK Count Increase**: Increments criminal counter

### PK Captured (Bounty Kill) Handler

**Location**: `Game.cpp:44769-44808`

When a player kills a criminal and earns a reward:

```cpp
void CGame::NotifyMsg_PKcaptured(char *pData)
{
    int iPKcount, iLevel, iExp, iRewardGold;
    char cName[12];

    // Parse data
    iPKcount = *(WORD*)(cp);     cp += 2;
    iLevel = *(WORD*)(cp);       cp += 2;
    memcpy(cName, cp, 10);       cp += 10;
    iRewardGold = *(DWORD*)(cp); cp += 4;
    iExp = *(DWORD*)(cp);        cp += 4;

    // Display victory message
    wsprintf(cTxt, "Level %d win in a battle against the Criminal %s(%d)!", iLevel, cName, iPKcount);
    AddEventList(cTxt, 10);

    // Show rewards
    wsprintf(cTxt, "Exp increased by %d points.", iExp - m_iExp);
    AddEventList(cTxt, 10);

    wsprintf(cTxt, "You can receive %d prize gold.", iExp - m_iExp);
    AddEventList(cTxt, 10);
}
```

### Enemy Kill Reward Handler

**Location**: `Game.cpp:43387-43457`

Tracks legitimate enemy faction kills during warfare:

```cpp
void CGame::NotifyMsg_EnemyKillReward(char *pData)
{
    int iExp, iEnemyKillCount, iWarContribution;
    char cName[12], cGuildName[24];

    // Parse kill data
    iExp = *(DWORD*)(cp);           cp += 4;
    iEnemyKillCount = *(DWORD*)(cp); cp += 4;
    memcpy(cName, cp, 10);          cp += 10;
    memcpy(cGuildName, cp, 20);     cp += 20;
    sGuildRank = *(short*)(cp);     cp += 2;
    iWarContribution = *(short*)(cp); cp += 2;

    // Update war contribution
    if (iWarContribution > m_iWarContribution) {
        wsprintf(G_cTxt, "War Contribution +%d!", iWarContribution - m_iWarContribution);
        SetTopMsg(G_cTxt, 5);
    }
    m_iWarContribution = iWarContribution;

    // Display kill message
    if (sGuildRank == -1)
        wsprintf(cTxt, "You killed the enemy %s!", cName);
    else
        wsprintf(cTxt, "You killed the enemy %s of %s guild!", cGuildName, cName);
    AddEventList(cTxt, 10);

    // Update EK count
    if (m_iEnemyKillCount < iEnemyKillCount) {
        wsprintf(cTxt, "Enemy-Kill-Count has been increased by %d points.",
                 iEnemyKillCount - m_iEnemyKillCount);
        AddEventList(cTxt, 10);
    }

    m_iExp = iExp;
    m_iEnemyKillCount = iEnemyKillCount;
    PlaySound('E', 23, 0);  // Victory sound
}
```

## Constants & Limits

### Network Message Types (NetMessages.h)

```cpp
#define DEF_NOTIFY_PKPENALTY           0x0B1A  // PK penalty notification
#define DEF_NOTIFY_PKCAPTURED          0x0B1B  // Criminal killed (bounty)
#define DEF_NOTIFY_ENEMYKILLREWARD     0x0B1C  // Enemy kill reward
#define DEF_NOTIFY_SAFEATTACKMODE      0x0B51  // Safe attack mode toggle
#define DEF_NOTIFY_ENEMYKILLS          0x0B5A  // Enemy kill count update
#define DEF_COMMONTYPE_TOGGLESAFEATTACKMODE  0x0A18  // Request toggle safe attack
```

### Player Type Range

```cpp
// Object types 1-6 are player characters
if ((sObjectType >= 1) && (sObjectType <= 6)) {
    // This is a player, check force attack mode before attacking
    if (m_bForceAttack == FALSE) break;  // Don't attack without force mode
}
```

## Integration Points

### Combat Mode Restrictions

**Location**: `Game.cpp:36445-36446, 36741-36742`

Players cannot attack friendly/neutral players without Force Attack mode:

```cpp
// In mouse click attack handling:
if (_iGetFOE(sObjectStatus) >= 0) break;  // Don't attack friendlies
if ((sObjectType >= 1) && (sObjectType <= 6) && (m_bForceAttack == FALSE)) break;
```

### Safe Zone Detection

**Location**: `Game.cpp:46033`

Criminals have no safe zones:

```cpp
if (m_iPKCount != 0) bNowSafe = FALSE;  // PKers are unsafe everywhere
```

### City Hall Restrictions

**Location**: `Game.cpp:38307-38332`

Criminals cannot use certain city hall services:

```cpp
// Mode change and teleportation require:
// - Not in Crusade mode
// - Must be citizen
// - Must have zero PK count
if ((m_bIsCrusadeMode == FALSE) && m_bCitizen && (m_iPKCount == 0)) {
    // Enable mode change option
    // Enable city teleportation option
}
```

### Hero Promotion Requirements

**Location**: `Game.cpp:22424, 22440`

Becoming a hero requires:
- At least 300 enemy kills
- Zero PK count

```cpp
if (m_iEnemyKillCount < 300) return;  // Need 300 EK
if (m_iPKCount != 0) return;          // Cannot be criminal
```

### Character Dialog Display

**Location**: `Game.cpp:37406-37482`

Criminal status is shown in character info:

```cpp
if (m_iPKCount > 0) {
    wsprintf(cTxt2, "Criminal (%d)", m_iPKCount);
    strcat(G_cTxt, cTxt2);
}

// Also displays contribution
wsprintf(cTxt2, "Contribution (%d)", m_iContribution);
strcat(G_cTxt, cTxt2);

// Enemy Kill Count display
wsprintf(G_cTxt, "%d", m_iEnemyKillCount);
PutAlignedString(sX+180, sX+250, sY + 257, G_cTxt, 45,25,25);
```

### Combat Mode UI Indicators

**Location**: `Game.cpp:20040-20044`

Icon panel shows safe/PK attack mode:

```cpp
if (m_bIsCombatMode) {
    if (m_bIsSafeAttackMode)
        // Draw green "safe" combat indicator (sprite frame 4)
        m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL]->PutSpriteFast(368, 440, 4, dwTime);
    else
        // Draw red "PK" combat indicator (sprite frame 5)
        m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL]->PutSpriteFast(368, 440, 5, dwTime);
}
```

## State Management

### Safe Attack Mode Toggle

**Location**: `Game.cpp:29102, 31058-31067`

Toggled via Home key or server notification:

```cpp
// Sending toggle request
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_TOGGLESAFEATTACKMODE, NULL, NULL, NULL, NULL, NULL);

// Handling server response
case DEF_NOTIFY_SAFEATTACKMODE:
    if (*cp == 1) {
        if (!m_bIsSafeAttackMode) AddEventList("Safe attack mode has been activated.", 10);
        m_bIsSafeAttackMode = TRUE;
    }
    else {
        if (m_bIsSafeAttackMode) AddEventList("Safe attack mode has been deactivated.", 10);
        m_bIsSafeAttackMode = FALSE;
    }
    break;
```

### Force Attack Mode Toggle

**Location**: `Game.cpp:28625-28633`

Toggled via Insert key:

```cpp
if (m_bForceAttack) {
    m_bForceAttack = FALSE;
    AddEventList("Auto Attack Mode has been disabled.", 10);
}
else {
    m_bForceAttack = TRUE;
    AddEventList("Auto Attack Mode has been enabled.", 10);
}
```

### Mana Cost Penalty

**Location**: `Game.cpp:48047`

Safe attack mode increases mana costs by 40%:

```cpp
if (m_bIsSafeAttackMode)
    iManaCost += (iManaCost/2) - (iManaCost/10);  // +40% mana cost
```

## Known Issues / Technical Debt

1. **Commented Rating System**: The `m_iRating` variable is defined but commented out throughout the codebase with notes like "Rating is received but not used... will implement later." The rating system was planned but never fully implemented.

2. **Hardcoded Object Types**: Player types 1-6 are hardcoded throughout the codebase rather than using named constants.

3. **Status Bit Magic Numbers**: The status bit flags (0x8000, 0x4000, etc.) are used as raw hex values without named constants.

4. **Mixed FOE Logic**: The `_iGetFOE` function has complex nested logic that's difficult to follow and maintain.

5. **Localization Inconsistency**: Some PK-related strings are hardcoded in English while others use localized defines.

## Modernization Notes

### Recommended Data Structures

```cpp
namespace hb::pk {

enum class RelationshipType : int8_t {
    Enemy_Criminal = -2,
    Enemy_Faction = -1,
    Neutral = 0,
    Friendly = 1
};

struct PlayerStatus {
    bool is_criminal;      // Has PK count > 0
    bool is_citizen;       // Belongs to a city
    bool is_aresden;       // Aresden (true) or Elvine (false)
    bool is_civilian;      // Civilian (true) or Combatant (false)
    bool is_berserk;
    bool is_frozen;
    bool is_invisible;

    static PlayerStatus from_legacy(uint16_t status);
    uint16_t to_legacy() const;
};

struct CombatRecord {
    int32_t pk_count = 0;           // Criminal kills
    int32_t enemy_kill_count = 0;   // Legitimate enemy kills
    int32_t contribution = 0;       // City contribution
    int32_t war_contribution = 0;   // Crusade contribution
    int32_t rating = 0;             // PvP rating (future)
};

class PKSystem {
public:
    [[nodiscard]] RelationshipType get_relationship(
        const PlayerStatus& self,
        const PlayerStatus& target,
        bool is_crusade_mode) const;

    [[nodiscard]] bool can_attack(
        const PlayerStatus& self,
        const PlayerStatus& target,
        bool force_attack_mode,
        bool is_crusade_mode) const;

    [[nodiscard]] sf::Color get_name_color(RelationshipType relation) const;
    [[nodiscard]] std::string get_status_label(
        const PlayerStatus& status,
        bool is_criminal) const;
};

} // namespace hb::pk
```

### Recommended Protocol Extensions

```cpp
namespace hb::net::notify {

// Existing
constexpr uint16_t PK_PENALTY = 0x0B1A;
constexpr uint16_t PK_CAPTURED = 0x0B1B;
constexpr uint16_t ENEMY_KILL_REWARD = 0x0B1C;
constexpr uint16_t SAFE_ATTACK_MODE = 0x0B51;
constexpr uint16_t ENEMY_KILLS = 0x0B5A;

// Potential future extensions
constexpr uint16_t RATING_CHANGED = 0x0B80;
constexpr uint16_t BOUNTY_PLACED = 0x0B81;
constexpr uint16_t BOUNTY_CLAIMED = 0x0B82;
constexpr uint16_t CRIMINAL_LEVEL_CHANGED = 0x0B83;
constexpr uint16_t AMNESTY_GRANTED = 0x0B84;

} // namespace hb::net::notify
```

### Key Behaviors to Preserve

1. **Name Color System**: Criminals always display with red names/labels
2. **Safe Zone Exclusion**: Criminals cannot benefit from safe zones
3. **Service Restrictions**: Criminals blocked from city hall teleportation and mode changes
4. **Force Attack Requirement**: Attacking non-hostile players requires force attack mode
5. **Safe Attack Mana Penalty**: 40% increased mana cost in safe attack mode
6. **Bounty Rewards**: Killing criminals grants EXP and gold rewards
7. **Stat Penalties**: PK penalty reduces stats and EXP, not just increments counter
8. **Hero Requirements**: 300 EK and 0 PK required for hero status
