# Crusade/War System

## Overview

The Crusade system is Helbreath's large-scale PvP war feature between the two factions: **Aresden** and **Elvine**. When a crusade is active, players can choose roles (duties), build defensive structures, summon war units, and earn contribution points that translate to experience rewards.

The crusade takes place primarily on the "middleland" map (752x752 tiles), a contested zone between the two cities.

## Source Files

- `Game.cpp` - All crusade logic embedded in the monolithic CGame class
- `Game.h` - Crusade-related member variables and structures
- `NetMessages.h` - Network protocol definitions for crusade messages

## Key Data Structures

### Crusade Structure Info

Tracks up to 300 structures on the battlefield:

```cpp
#define DEF_MAXCRUSADESTRUCTURES 300

struct {
    short sX, sY;      // Map coordinates (0-752 range)
    char cType;        // Structure type (see Structure Types below)
    char cSide;        // 1 = Aresden, 2 = Elvine (0 = neutral/unknown)
} m_stCrusadeStructureInfo[DEF_MAXCRUSADESTRUCTURES];
```

### Crusade State Variables

```cpp
// Core state
BOOL m_bIsCrusadeMode;              // TRUE when crusade is active
int m_iCrusadeDuty;                 // Current duty: 0=none, 1=commander, 2=constructor, 3=soldier

// Resource tracking
int m_iConstructionPoint;           // Points for summoning/building
int m_iWarContribution;             // Combat contribution (converts to EXP)

// Location tracking
int m_iConstructLocX, m_iConstructLocY;  // Constructor build target location
int m_iTeleportLocX, m_iTeleportLocY;    // Guild teleport destination
char m_cTeleportMapName[12];             // Map name for teleport destination

// Faction identification
BOOL m_bCitizen;                    // Has citizenship (not a traveler)
BOOL m_bAresden;                    // TRUE = Aresden, FALSE = Elvine
BOOL m_bHunter;                     // Hunter mode flag
int m_iPKCount;                     // Player kills (affects FOE status)
```

## Crusade Duties

Players select a duty when crusade begins. Duty selection is restricted by guild membership:

| Duty | ID | Description | Requirements |
|------|----|-------------|--------------|
| Soldier | 3 | Combat-focused role | Any citizen |
| Commander | 1 | Strategic leadership, can summon units | Non-guild master |
| Constructor | 2 | Builds defensive structures | Guild member (rank != -1) |
| None | 0 | Non-participant (travelers) | N/A |

### Duty Selection Logic

```cpp
void CGame::DlgBoxClick_CrusadeJob(short msX, short msY)
{
    // Guild masters (rank 0) can only be Soldiers
    if (m_iGuildRank == 0) {
        // Only soldier option available
        bSendCommand(..., DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY, NULL, 3, ...);
    }
    else {
        // Non-guild masters can choose Commander
        bSendCommand(..., DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY, NULL, 1, ...);

        // Guild members can also choose Constructor
        if (m_iGuildRank != -1) {
            bSendCommand(..., DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY, NULL, 2, ...);
        }
    }
}
```

## Structure Types

### Buildable Structures (Constructor)

| Type ID | Name | Function |
|---------|------|----------|
| 36 | Mana Collector | Generates mana/resources |
| 37 | Detect Tower | Reveals enemies |
| 38 | Arrow Guard Tower | Ranged defense |
| 39 | Cannon Guard Tower | Heavy defense |

### Special Markers

| Type ID | Purpose |
|---------|---------|
| 41 | Construction target marker |
| 42 | Teleport location marker |
| 43 | Player position indicator |

### Map Rendering

Structures are rendered on the mini-map with faction colors:
- Sprite 36-39: Structure icons by type
- Sprite 37/39: Aresden-colored variants (cSide == 1)
- Sprite 36/38: Elvine-colored variants (cSide != 1)

## Summonable War Units

Commanders can summon powerful units using construction points:

### Aresden Units

| Unit ID | Name | Cost | Description |
|---------|------|------|-------------|
| 47 | Battle Golem | 3000 | Heavy melee unit |
| 46 | Temple Knight | 2000 | Elite cavalry |
| 43 | Light War Beetle | 1000 | Fast attack unit |
| 51 | Catapult | 5000 | Siege weapon |

### Elvine Units

| Unit ID | Name | Cost | Description |
|---------|------|------|-------------|
| 45 | God's Hand Knight Cavalry | 3000 | Heavy cavalry |
| 44 | God's Hand Knight | 2000 | Elite infantry |
| 43 | Light War Beetle | 1000 | Fast attack unit |
| 51 | Catapult | 5000 | Siege weapon |

### Summon Modes

When summoning, commanders choose between:
- **Mode 0 (Escort)**: Unit follows and protects
- **Mode 1 (Hold Position)**: Unit stays at current location

```cpp
// Summon command format
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_SUMMONWARUNIT, NULL,
             unitType,    // Unit ID (43-51)
             1,           // Always 1
             summonMode,  // 0=escort, 1=hold
             NULL);
```

## Friend-or-Foe (FOE) System

During crusade, the game determines ally/enemy status using the `_iGetFOE()` function:

```cpp
int CGame::_iGetFOE(short sStatus)
{
    // Status bits from entity
    BOOL bPK      = (sStatus & 0x8000);  // Player killer
    BOOL bCitizen = (sStatus & 0x4000);  // Has citizenship
    BOOL bAresden = (sStatus & 0x2000);  // Aresden faction
    BOOL bHunter  = (sStatus & 0x1000);  // Hunter mode

    // Self is PK - everyone is enemy
    if (m_iPKCount != 0) return -1;

    // Target is PK - always enemy
    if (bPK) return -2;

    // Non-citizen (traveler) - neutral
    if (!bCitizen) return 0;

    // Self is non-citizen - treat others as neutral
    if (!m_bCitizen) return 0;

    // Same faction = ally
    if (m_bAresden == bAresden) return 1;

    // During crusade, different faction = enemy
    if (m_bIsCrusadeMode) return -1;

    // Outside crusade, hunter status matters
    if (!m_bHunter && !bHunter) return -1;  // Both non-hunters = enemy
    else return 0;  // At least one hunter = neutral
}
```

### FOE Return Values

| Value | Meaning | Effect |
|-------|---------|--------|
| 1 | Ally | Cannot attack, green name |
| 0 | Neutral | Can attack freely |
| -1 | Enemy | Hostile, red indicator |
| -2 | Criminal (PK) | Always hostile |

### Visual Indicator

Enemies display a red aura during crusade:

```cpp
void CGame::DrawObjectFOE(int ix, int iy, int iFrame)
{
    if (_iGetFOE(_tmp_sStatus) < 0) {
        // Draw red enemy indicator sprite
        if (iFrame <= 4)
            m_pEffectSpr[38]->PutTransSprite(ix, iy, iFrame, G_dwGlobalTime);
    }
}
```

## Network Protocol

### Crusade Notifications

#### DEF_NOTIFY_CRUSADE (0x0B94)

Main crusade state update from server:

```cpp
case DEF_NOTIFY_CRUSADE:
    // Packet format:
    // int iV1 - Crusade active (0=ended, non-0=active)
    // int iV2 - Assigned duty
    // int iV3 - War contribution result
    // int iV4 - Winner side (on end) or -1 for ineligible

    if (m_bIsCrusadeMode == FALSE) {
        if (iV1 != 0) {
            // Crusade starting
            m_bIsCrusadeMode = TRUE;
            m_iCrusadeDuty = iV2;

            // Non-soldiers need map status
            if (m_iCrusadeDuty != 3 && m_bCitizen)
                _RequestMapStatus("middleland", 3);

            // Show duty selection or info dialog
            if (m_iCrusadeDuty != NULL)
                EnableDialogBox(33, 2, iV2, NULL);
            else
                EnableDialogBox(33, 1, NULL, NULL);

            // Announcement message by faction
            if (!m_bCitizen) EnableDialogBox(18, 800, NULL, NULL);
            else if (m_bAresden) EnableDialogBox(18, 801, NULL, NULL);
            else EnableDialogBox(18, 802, NULL, NULL);
        }

        if (iV3 != 0) {
            // Contribution reward
            CrusadeContributionResult(iV3);
        }
    }
    else {
        if (iV1 == 0) {
            // Crusade ending
            m_bIsCrusadeMode = FALSE;
            m_iCrusadeDuty = NULL;
            CrusadeWarResult(iV4);  // Show winner
        }
        else if (m_iCrusadeDuty != iV2) {
            // Duty changed
            m_iCrusadeDuty = iV2;
            EnableDialogBox(33, 2, iV2, NULL);
        }
    }
    break;
```

#### DEF_NOTIFY_NOMORECRUSADESTRUCTURE (0x0B9E)

Sent when structure limit reached.

### Command Messages

| Message | Value | Description |
|---------|-------|-------------|
| `DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY` | 0x0A51 | Request duty assignment |
| `DEF_COMMONTYPE_SETGUILDTELEPORTLOC` | 0x0A54 | Set guild teleport target |
| `DEF_COMMONTYPE_GUILDTELEPORT` | 0x0A55 | Execute guild teleport |
| `DEF_COMMONTYPE_SUMMONWARUNIT` | 0x0A56 | Summon a war unit |
| `DEF_COMMONTYPE_REQUEST_MAPSTATUS` | (varies) | Request structure positions |

### Map Status Updates

Structure positions are requested via `_RequestMapStatus()`:

```cpp
void CGame::_RequestMapStatus(char* pMapName, int iMode)
{
    // Mode 1 = Request teleport locations
    // Mode 3 = Request all structure positions
    bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_MAPSTATUS,
                 NULL, iMode, NULL, NULL, pMapName);
}
```

Response is handled by `AddMapStatusInfo()`:

```cpp
void CGame::AddMapStatusInfo(char* pData, BOOL bIsLastData)
{
    // Parse structure data
    // Format: mapName(10) + index(2) + count(1) + [type(1) + x(2) + y(2) + side(1)]...

    for (i = 1; i <= cTotal; i++) {
        m_stCrusadeStructureInfo[sIndex].cType = *cp++;
        m_stCrusadeStructureInfo[sIndex].sX = *(short*)cp; cp += 2;
        m_stCrusadeStructureInfo[sIndex].sY = *(short*)cp; cp += 2;
        m_stCrusadeStructureInfo[sIndex].cSide = *cp++;
        sIndex++;
    }

    // Clear remaining entries if last packet
    if (bIsLastData) {
        while (sIndex < DEF_MAXCRUSADESTRUCTURES) {
            m_stCrusadeStructureInfo[sIndex].cType = NULL;
            // ... clear other fields
            sIndex++;
        }
    }
}
```

## UI Dialogs

### Dialog IDs

| ID | Name | Function |
|----|------|----------|
| 33 | Crusade Job | Duty selection and info |
| 36 | Commander | Command interface |
| 37 | Constructor | Building interface |
| 38 | Soldier | Teleport interface |

### Commander Dialog (36)

Modes:
- **Mode 0**: Main menu - teleport setup, confirm teleport, summon units, construction locations
- **Mode 1**: Teleport location selection (click on map)
- **Mode 2**: Teleport confirmation
- **Mode 3**: Unit summoning interface
- **Mode 4**: Construction location overview

Features:
- Mini-map of middleland showing all structures
- Faction-specific unit summoning
- Auto-refreshes every 10 seconds

```cpp
void CGame::DrawDialogBox_Commander(int msX, int msY)
{
    // Auto-refresh map status every 10 seconds
    if ((dwTime - m_dwCommanderCommandRequestedTime) > 1000*10) {
        _RequestMapStatus("middleland", 3);  // Structures
        _RequestMapStatus("middleland", 1);  // Teleport locs
        m_dwCommanderCommandRequestedTime = dwTime;
    }
    // ... render interface
}
```

### Constructor Dialog (37)

Features:
- Build location selection
- Structure type selection (Guard Towers, Mana Collectors)
- Teleport to friendly locations

### Soldier Dialog (38)

Features:
- Simplified teleport interface
- View structure locations
- Request teleport to battle

## Crusade Results

### Contribution Reward

```cpp
void CGame::CrusadeContributionResult(int iWarContribution)
{
    if (iWarContribution > 0) {
        // Victory! Display EXP reward
        PlaySound('E', 23, 0, 0);  // Victory fanfare
        // Message: "Congratulations! Your side won!"
        // "+%d Exp Points!"
    }
    else if (iWarContribution < 0) {
        // Defeat
        PlaySound('E', 24, 0, 0);  // Defeat sound
        // Message: "Your side has lost..."
    }
    else {
        // iWarContribution == 0
        // Crusade ended before player could contribute
        // (joined late or already ended)
    }
}
```

### War End Result

```cpp
void CGame::CrusadeWarResult(int iWinnerSide)
{
    // iWinnerSide: 0=Draw, 1=Aresden, 2=Elvine

    int iPlayerSide;
    if (!m_bCitizen) iPlayerSide = 0;      // Traveler
    else if (m_bAresden) iPlayerSide = 1;  // Aresden
    else iPlayerSide = 2;                   // Elvine

    if (iWinnerSide == 0) {
        // Draw - no winner
    }
    else if (iWinnerSide == iPlayerSide) {
        // Player's side won!
        PlaySound('E', 23, 0, 0);
        PlaySound('C', 21, 0, 0);  // Cheering
    }
    else {
        // Player's side lost
        PlaySound('E', 24, 0, 0);
        PlaySound('C', 12, 0, 0);  // Mourning
    }
}
```

## Icon Panel Integration

During crusade, an extra button appears on the icon panel:

```cpp
void CGame::DrawDialogBox_IconPannel(short msX, short msY)
{
    if ((m_bIsCrusadeMode) && (m_iCrusadeDuty != 0)) {
        // Show crusade button based on faction
        if (m_bAresden == TRUE) {
            // Aresden-colored crusade icon (sprite 1/2)
            m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL]->PutSpriteFast(322, 434,
                (mouseHover ? 1 : 2), dwTime);
        }
        else {
            // Elvine-colored crusade icon (sprite 0/15)
            m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL]->PutSpriteFast(322, 434,
                (mouseHover ? 0 : 15), dwTime);
        }
    }
}
```

Clicking this button opens the appropriate duty dialog (36/37/38).

## On-Screen Indicators

### Construction/Teleport Markers

During crusade, target locations are shown on the main game view:

```cpp
if (m_bIsCrusadeMode) {
    // Construction target (green marker)
    if (m_iConstructLocX != -1) {
        DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_CRUSADE,
                         m_iConstructLocX*32 - m_sViewPointX,
                         m_iConstructLocY*32 - m_sViewPointY, 41);
    }

    // Teleport target (blue marker)
    if (m_iTeleportLocX != -1) {
        DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_CRUSADE,
                         m_iTeleportLocX*32 - m_sViewPointX,
                         m_iTeleportLocY*32 - m_sViewPointY, 42);
    }
}
```

## Constants & Limits

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_MAXCRUSADESTRUCTURES` | 300 | Maximum tracked structures |
| Middleland map size | 752x752 | Battle zone dimensions |
| Mini-map display size | 280x253 | Pixels in dialog |
| Map status refresh | 10 sec | Auto-update interval |
| Battle Golem cost | 3000 | Construction points |
| Temple Knight cost | 2000 | Construction points |
| Light War Beetle cost | 1000 | Construction points |
| Catapult cost | 5000 | Construction points |

## Integration Points

### Systems That Check Crusade Mode

1. **Combat System** - FOE determination changes
2. **Rendering** - Enemy indicators drawn
3. **Input Handling** - Blocked actions during crusade
4. **Teleportation** - Normal teleport disabled
5. **Map System** - Structure overlay rendering
6. **Icon Panel** - Extra crusade button
7. **Chat System** - War announcements

### Restricted Actions During Crusade

```cpp
// Various places in code check:
if (m_bIsCrusadeMode) return;  // Blocked action

// Examples:
// - Cannot use normal teleport scrolls
// - Cannot challenge duels
// - Trade restrictions may apply
```

## State Management

### Initialization

```cpp
// In CGame initialization:
m_bIsCrusadeMode = FALSE;
m_iCrusadeDuty = NULL;
m_iConstructionPoint = NULL;
m_iWarContribution = NULL;
m_iTeleportLocX = m_iTeleportLocY = -1;
m_iConstructLocX = m_iConstructLocY = -1;

// Clear all structure info
for (i = 0; i < DEF_MAXCRUSADESTRUCTURES; i++) {
    m_stCrusadeStructureInfo[i].cType = NULL;
    m_stCrusadeStructureInfo[i].cSide = NULL;
    m_stCrusadeStructureInfo[i].sX = NULL;
    m_stCrusadeStructureInfo[i].sY = NULL;
}
```

### Resource Updates

Construction points and war contribution are updated via `DEF_NOTIFY_CONSTRUCTIONPOINT`:

```cpp
// Tracked changes displayed to player:
if (sV1 > m_iConstructionPoint && sV2 > m_iWarContribution) {
    wsprintf(G_cTxt, "%s +%d, %s +%d",
             "Construction Points", (sV1 - m_iConstructionPoint),
             "War Contribution", (sV2 - m_iWarContribution));
}
m_iConstructionPoint = sV1;
m_iWarContribution = sV2;
```

## Known Issues / Technical Debt

1. **Hardcoded Map Name** - "middleland" is hardcoded throughout
2. **Magic Numbers** - Structure types (36-42) not defined as constants
3. **Copy-Paste Code** - Dialog rendering has significant duplication between duties
4. **No Abstraction** - All crusade logic directly in CGame class
5. **Korean Comments** - Most inline comments are in Korean
6. **Mixed Coordinate Systems** - Map coordinates (0-752) vs pixel coordinates
7. **No Validation** - Construction point costs not validated client-side (server authoritative)

## Modernization Notes

### Recommended Architecture

```
crusade/
├── CrusadeSystem.cpp/h        # Main crusade manager
├── CrusadeState.h             # State machine for crusade phases
├── CrusadeStructure.h         # Structure data and types
├── CrusadeUnit.h              # War unit definitions
├── CrusadeDuty.h              # Duty roles and permissions
├── CrusadeUI/
│   ├── CommanderDialog.cpp/h  # Commander interface
│   ├── ConstructorDialog.cpp/h# Constructor interface
│   ├── SoldierDialog.cpp/h    # Soldier interface
│   └── CrusadeMap.cpp/h       # Mini-map rendering
└── CrusadeNetwork.cpp/h       # Protocol handling
```

### Key Improvements

1. **Enum Classes** for duty types, structure types, unit types
2. **Event System** for crusade state changes
3. **Separate Renderer** for mini-map and indicators
4. **Data-Driven** unit costs and properties
5. **Localized Strings** instead of compile-time defines
6. **Unit Tests** for FOE calculation logic
