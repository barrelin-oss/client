# Teleportation System

## Overview

The Helbreath teleportation system allows players to instantly travel between locations in the game world. It encompasses multiple teleportation methods: spell-based recall, gold-based NPC teleportation, guild teleportation, and forced recall mechanics for PvP/warfare zones. The system integrates with the magic system, network protocol, UI dialogs, and map/tile systems.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | Main teleportation logic, message handlers, UI flow |
| `Game.h` | Teleport-related member variables and declarations |
| `NetMessages.h` | Teleport packet type definitions |
| `Magic.h` / `Magic.cpp` | Recall spell definition (spell ID 90) |
| `GlobalDef.h` | Teleport-related constants |

## Key Data Structures

### Teleport Location Data

Teleport destinations are managed through dialog interactions and server responses. Locations include:

- **Town halls** - Primary teleport destinations
- **Dungeon levels** - e.g., "Dungeon Level 2"
- **Guild halls** - Via guild teleport system
- **Safe zones** - Designated safe teleport areas
- **PvP/Warfare zones** - Strategic teleportation during crusades

### Map Marker for Teleports

```cpp
// Teleport locations displayed on map (type 4)
struct MapMarker {
    int x;              // World X coordinate (tile position)
    int y;              // World Y coordinate (tile position)
    char name[32];      // Location name
    int type;           // 4 = teleport location
};
```

### Spell Structure (Recall)

```cpp
// Recall spell (ID 90) properties
struct Magic {
    int id;             // 90 for Recall
    char name[32];      // "Recall"
    int mpCost;         // 20 MP
    int castTime;       // 5000 ms (5 seconds)
    int cooldown;       // 60000 ms (60 seconds)
    int levelReq;       // Level 1
    int magicType;      // DEF_MAGICTYPE_TELEPORT
};
```

## Teleportation Types

### 1. Spell-Based Teleportation (Recall)

**Spell ID:** 90
**Magic Type:** `DEF_MAGICTYPE_TELEPORT`

| Property | Value |
|----------|-------|
| MP Cost | 20 |
| Cast Time | 5.0 seconds |
| Cooldown | 60 seconds |
| Level Requirement | 1 |
| Circle/Tier | 1 |
| Target Type | Self |

The Recall spell teleports the player back to their designated town. It must be learned from an NPC before use.

### 2. Gold-Based NPC Teleportation (Charged Teleport)

Players can pay gold to teleport via NPCs at city halls. Features:

- Variable gold cost based on destination
- Different prices for different towns
- Server validates sufficient gold before teleport
- Cannot teleport to current location

### 3. Guild Teleportation

Guild masters can set a recall point for guild members:

- **Set Location:** Common type `0x0A54` (set_guild_teleport_loc)
- **Use Teleport:** Common type `0x0A55` (guild_teleport)
- Guild members can return to the guild's set location
- Cost may be variable or free depending on server configuration

### 4. Force Recall (Forced Teleportation)

Used for PvP/warfare zone time limits:

- **Trigger:** Player exceeds time limit in enemy territory
- **Destination:** Default town (opposite team's town)
- **Timer:** Countdown shown to player
- **Restriction:** "Recall is not available in the opposite town"

## Network Protocol

### Request Messages (Client → Server)

| Message ID | Name | Purpose |
|------------|------|---------|
| `0x0EA03201` | MSGID_REQUEST_TELEPORT | Request teleportation to destination |
| `0x0EA03202` | MSGID_REQUEST_TELEPORT_LIST | Request available destinations |
| `0x0EA03204` | MSGID_REQUEST_CHARGED_TELEPORT | Gold-based teleportation request |

### Response Messages (Server → Client)

| Message ID | Name | Purpose |
|------------|------|---------|
| `0x0EA03203` | MSGID_RESPONSE_TELEPORT_LIST | List of available destinations with costs |
| `0x0EA03205` | MSGID_RESPONSE_CHARGED_TELEPORT | Charged teleport result |

### Notification Messages

| Notification Type | Name | Purpose |
|-------------------|------|---------|
| `0x0B40` | DEF_NOTIFY_TOBERECALLED | Player is being recalled to town |
| `0x0BA7` | DEF_NOTIFY_FORCERECALLTIME | Force recall countdown timer |

### Packet Format: Teleport List Response

```
MSGID_RESPONSE_TELEPORT_LIST (0x0EA03203)
├── Header
│   ├── Message ID (4 bytes)
│   └── Length (2 bytes)
├── Destination Count (2 bytes)
└── Destinations[] (variable)
    ├── Name (32 bytes, null-terminated)
    ├── Tile X (2 bytes)
    ├── Tile Y (2 bytes)
    └── Gold Cost (4 bytes)
```

## Core Functions

### Teleportation Request Handling

```cpp
// Send teleport request to server
void CGame::RequestTeleport(int destX, int destY);

// Send charged teleport request with gold payment
void CGame::RequestChargedTeleport(int destIndex, int goldCost);

// Request available teleport destinations
void CGame::RequestTeleportList();
```

### Message Handlers

```cpp
// Handle teleport list response from server
void CGame::ResponseTeleportList(char* pData);

// Handle charged teleport response
void CGame::ResponseChargedTeleport(char* pData);

// Handle force recall notification
void CGame::NotificationHandler_ToBeRecalled(char* pData);

// Handle force recall timer notification
void CGame::NotificationHandler_ForceRecallTime(char* pData);
```

### Recall Spell Casting

```cpp
// Initiate recall spell casting
void CGame::StartSpellCasting(int spellID);  // spellID = 90 for Recall

// Process spell casting completion
void CGame::FinishSpellCasting();

// Check if recall is available
bool CGame::CanUseRecall();
```

### Dialog Functions

```cpp
// Draw city hall teleport menu
void CGame::DrawDialogBox_CityHallMenu();

// Handle teleport destination selection
void CGame::HandleTeleportSelection(int index);

// Display teleport confirmation
void CGame::ShowTeleportConfirmation(const char* destName, int goldCost);
```

## UI Flow

### Spell-Based Recall Flow

```
1. Player activates Recall spell (hotkey or spellbook)
   └── Client validates: spell learned, MP available, not on cooldown

2. Casting begins (5 second cast time)
   ├── Cast bar displayed
   ├── Character plays casting animation
   └── Can be interrupted by damage or movement

3. Casting completes
   └── Client sends MSGID_REQUEST_TELEPORT

4. Server validates and responds
   ├── Success: Player position updated
   └── Failure: Error message displayed

5. Client receives position update
   ├── Player sprite moves to destination
   ├── Camera follows
   └── Cooldown begins (60 seconds)
```

### NPC Teleport Flow

```
1. Player interacts with City Hall NPC
   └── Dialog opens: DRAW_DIALOGBOX_CITYHALL_MENU

2. Client requests available destinations
   └── Sends MSGID_REQUEST_TELEPORT_LIST

3. Server responds with destination list
   └── Dialog populated with options and prices

4. Player selects destination
   └── Message: "Click and select the teleport location"

5. Confirmation prompt displayed
   └── "[Amount] gold to teleport"

6. Player confirms
   └── Sends MSGID_REQUEST_CHARGED_TELEPORT

7. Server processes
   ├── Validates gold amount
   ├── Deducts gold
   └── Sends MSGID_RESPONSE_CHARGED_TELEPORT

8. Client handles response
   ├── Success: Player teleports
   └── Failure: Error message
```

## Constants & Limits

### Message ID Constants

```cpp
#define MSGID_REQUEST_TELEPORT          0x0EA03201
#define MSGID_REQUEST_TELEPORT_LIST     0x0EA03202
#define MSGID_RESPONSE_TELEPORT_LIST    0x0EA03203
#define MSGID_REQUEST_CHARGED_TELEPORT  0x0EA03204
#define MSGID_RESPONSE_CHARGED_TELEPORT 0x0EA03205
```

### Common Type Constants

```cpp
#define DEF_COMMONTYPE_SET_GUILD_TELEPORT_LOC   0x0A54
#define DEF_COMMONTYPE_GUILD_TELEPORT           0x0A55
```

### Notification Type Constants

```cpp
#define DEF_NOTIFY_TOBERECALLED         0x0B40
#define DEF_NOTIFY_FORCERECALLTIME      0x0BA7
```

### Spell Constants

```cpp
#define DEF_MAGICTYPE_TELEPORT          8
#define DEF_SPELL_RECALL                90

// Recall spell values
#define RECALL_MP_COST                  20
#define RECALL_CAST_TIME_MS             5000
#define RECALL_COOLDOWN_MS              60000
#define RECALL_LEVEL_REQ                1
```

### Tile Constants

```cpp
#define DEF_TILEFLAG_TELEPORT           (1 << 5)    // Bit 5: teleport location
#define DEF_TILEFLAG_SAFE_ZONE          (1 << 8)    // Bit 8: safe zone
#define DEF_TILEFLAG_PVP_ZONE           (1 << 9)    // Bit 9: PvP zone
```

## Restrictions & Blocking Conditions

Teleportation is blocked when:

| Condition | Error Message |
|-----------|---------------|
| In combat | (Spell casting interrupted) |
| In opposite team's territory | "Recall is not available in the opposite town" |
| Insufficient gold | "You need more gold to teleport" |
| Same destination | "You may not teleport to the same town" |
| No valid destinations | "There is no area that you can teleport" |
| Spell on cooldown | (Cooldown timer displayed) |
| Casting interrupted | (Cast bar canceled) |
| Map locked (crusade) | (Map access denied notification) |

### Tile-Level Restrictions

- Tiles with `DEF_TILEFLAG_BLOCKS_MAGIC` prevent spell casting
- Certain maps disable teleportation during crusade events
- PvP zones (`DEF_TILEFLAG_PVP_ZONE`) may have restricted teleport types

## Force Recall System

The force recall system ensures players don't stay indefinitely in enemy territory:

### Force Recall Flow

```
1. Player enters enemy territory (PvP/war zone)
   └── Server starts tracking time

2. Time limit approaching
   └── Server sends DEF_NOTIFY_FORCERECALLTIME
   └── Client shows: "You have %d minutes to get forced recall"

3. Timer expires
   └── Server sends DEF_NOTIFY_TOBERECALLED
   └── Client shows: "Being recalled..."

4. Force teleportation
   └── Player moved to default town
   └── May be disconnected if unresponsive
```

### Force Recall Messages

```cpp
// Timer warning
"You have %d minutes to get forced recall"

// Alternative warning
"You'll be forced to recall soon"

// Recall executing
"Being recalled..."

// Mode status
"Force recall mode has been set"
"Force recall mode has been released"
```

## Integration Points

### Magic System Integration

- Recall spell (ID 90) defined in magic system
- Magic type `DEF_MAGICTYPE_TELEPORT` (value 8) for teleport spells
- Spell learning tracked via `DEF_NOTIFY_MAGICSTUDYSUCCESS`
- Mastery levels (0-20) can improve spell effectiveness

### Dialog System Integration

- City Hall Menu dialog (dialog types 71-76)
- Map dialog for teleport location display (marker type 4)
- Confirmation dialogs for teleport requests
- Error message dialogs for failed teleports

### Network System Integration

- All teleport requests require server validation
- Position updates synchronized via network
- Gold transactions handled server-side
- Cooldowns may be server-enforced

### Map System Integration

- Teleport destinations defined per map
- Tile flags indicate valid teleport zones
- Map markers (type 4) show teleport locations
- Coordinate conversion: tile position * 32 = pixel position

## State Management

### Client-Side State

```cpp
// In CGame class
bool m_bRecallAvailable;        // Whether recall can be used
int m_iRecallCooldown;          // Cooldown timer (ms)
int m_iForceRecallTimer;        // Force recall countdown
bool m_bCastingRecall;          // Currently casting recall
int m_iCastProgress;            // Cast bar progress (0-100)

// Teleport destination list (from server)
struct TeleportDest {
    char name[32];
    int x, y;
    int goldCost;
} m_TeleportDests[MAX_TELEPORT_DESTS];
int m_iTeleportDestCount;
```

### Server-Side State (Client Perspective)

- Player position validated server-side
- Gold amount verified before deduction
- Cooldowns may be server-authoritative
- Force recall timers tracked per-player

## Known Issues / Technical Debt

1. **Hardcoded Spell ID** - Recall spell ID (90) hardcoded in multiple locations
2. **Magic Number Constants** - Teleport-related constants scattered across files
3. **Tightly Coupled** - Teleport logic embedded in CGame class
4. **No Abstraction** - Direct WinSock calls for teleport messages
5. **Limited Error Handling** - Many error conditions not gracefully handled
6. **UI Coupling** - Teleport selection tightly coupled to dialog rendering
7. **Duplicate Code** - Similar logic for different teleport types
8. **Global State** - Teleport state stored in global/class variables

## Modernization Notes

### Recommended Architecture

```cpp
namespace hb::gameplay {
    class TeleportSystem {
    public:
        // Spell-based teleport
        bool canUseRecall() const;
        void startRecall();
        void cancelRecall();

        // NPC teleport
        std::future<TeleportResult> requestDestinations();
        std::future<TeleportResult> teleportTo(const TeleportDestination& dest);

        // Guild teleport
        void setGuildTeleportLocation(Vec2 position);
        void useGuildTeleport();

        // State queries
        float getRecallCooldown() const;
        std::optional<float> getForceRecallTimer() const;

    private:
        std::vector<TeleportDestination> m_destinations;
        std::chrono::steady_clock::time_point m_lastRecallTime;
        std::optional<ForceRecallState> m_forceRecallState;
    };
}
```

### Key Improvements

1. **Decouple from CGame** - Extract to dedicated TeleportSystem class
2. **Type-Safe Messages** - Use PacketBuilder for teleport packets
3. **Async Operations** - Use std::future for server requests
4. **Event-Based** - Publish teleport events via EventBus
5. **Configurable** - Load spell costs/cooldowns from data files
6. **Testable** - Interface-based design for unit testing
7. **Error Handling** - Use std::expected for result types
