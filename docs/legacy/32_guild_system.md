# Guild System

## Overview

The Guild System provides social organization for players, allowing them to form groups with shared identity, hierarchy, and privileges. Guilds have a maximum of 32 members, led by a Guild Master who controls membership and special operations. The system integrates with the Crusade/War system for territory control and arena access.

Guild operations follow a two-tier approval model where membership changes require Guild Master authorization, creating an asynchronous request/response flow between players.

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` | Guild data structures, member variables, constants |
| `Game.cpp` | Guild handlers, dialog rendering, operation logic |
| `NetMessages.h` | Guild-related network message definitions |
| `lan_eng.h` | English guild UI strings (83+ strings) |
| `lan_kor.h` | Korean guild UI strings |
| `lan_chi.h` | Chinese guild UI strings |
| `lan_jap.h` | Japanese guild UI strings |

## Constants & Limits

```cpp
#define DEF_MAXGUILDSMAN        32      // Maximum members per guild
#define DEF_MAXGUILDNAMES       100     // Guild name cache size (v2.171)
```

| Constant | Value | Description |
|----------|-------|-------------|
| Max Members | 32 | Hard limit on guild membership |
| Name Cache | 100 | LRU cache entries for guild lookups |
| Guild Name Length | 21 | Maximum characters for guild name |
| Character Name Length | 12 | Member name storage |
| Operation Queue | 100 | Pending operation slots |
| Create Cost | 5 Gold | Guild creation fee |
| Join Ticket | 50 Gold | Admission ticket cost |
| Leave Ticket | 50 Gold | Withdrawal ticket cost |
| Arena Reservation | 1500 Gold | Crusade arena booking |

## Key Data Structures

### Guild Name Cache Entry

Stores cached guild information for other players (used for display).

```cpp
// Game.h, lines 617-622
struct {
    DWORD dwRefTime;        // Last access timestamp (LRU eviction)
    int   iGuildRank;       // Cached rank (-1 = not in guild)
    char  cCharName[12];    // Character name (lookup key)
    char  cGuildName[24];   // Guild name (supports underscores)
} m_stGuildName[DEF_MAXGUILDNAMES];
```

### Guild Operation Queue Entry

Tracks pending guild operations awaiting Guild Master response.

```cpp
// Game.h, lines 575-576
struct {
    char  cName[22];        // Requesting character name
    char  cOpMode;          // Operation type (1-8)
} m_stGuildOpList[100];
```

**Operation Modes (cOpMode):**

| Value | Operation | Description |
|-------|-----------|-------------|
| 0 | Empty | Slot available |
| 1 | Join Request | Player requesting to join |
| 2 | Leave Request | Member requesting to leave |
| 3 | Join Approved | Confirmation of acceptance |
| 4 | Join Rejected | Confirmation of rejection |
| 5 | Leave Approved | Confirmation of withdrawal |
| 6 | Leave Rejected | Confirmation of denial |
| 7 | Guild Disbanded | Guild no longer exists |
| 8 | Member Banned | Forceful removal (v1.4311-3) |

### Player Guild State

Member variables tracking the local player's guild membership.

```cpp
// Game.h
char    m_cGuildName[22];       // Player's guild name ("" if none)
int     m_iGuildRank;           // Player's rank (-1, 0, or 1+)
int     m_iTotalGuildsMan;      // Current member count
```

## Guild Ranks

| Rank Value | Title | Permissions |
|------------|-------|-------------|
| -1 | Not in Guild | Can request to join, create new guild |
| 0 | Guild Master | Full control: create, disband, approve/reject, ban, crusade ops |
| 1+ | Guild Member | Participate in activities, request withdrawal |

## Dialog Boxes

### Dialog 7: Guild Menu

**Position:** 337, 57
**Size:** 258 x 339 pixels
**Drawing:** `DrawDialogBox_GuildMenu()` (Game.cpp:38907)
**Click Handler:** `DlgBoxClick_GuildMenu()` (Game.cpp:5188)

**Menu Options:**

| Option | Requirements | Cost |
|--------|--------------|------|
| Create Guild | Charisma >= 20, Level >= 20, not in guild | 5 Gold |
| Disband Guild | Guild Master only | 5 Gold |
| Buy Admission Ticket | Not in guild | 50 Gold |
| Buy Withdrawal Ticket | Guild member | 50 Gold |
| Reserve Arena | Guild Master, Crusade mode | 1500 Gold |

**Dialog Modes:**

| Mode | State |
|------|-------|
| 0 | Main menu display |
| 1 | Guild name input |
| 2-4 | Creation responses |
| 5-8 | Disband flow |
| 9-12 | Ticket purchase flow |
| 13-22 | Arena reservation (Crusade) |

### Dialog 8: Guild Operations

**Position:** 337, 57
**Size:** 295 x 346 pixels
**Drawing:** `DrawDialogBox_GuildOperation()` (Game.cpp:39219)
**Click Handler:** `DlgBoxClick_GuildOp()` (Game.cpp:5712)

Displays pending operations for Guild Master approval.

**Button Layout:**
```cpp
#define DEF_LBTNPOSX    30      // Left button (Approve/OK)
#define DEF_RBTNPOSX    154     // Right button (Reject)
#define DEF_BTNPOSY     292     // Button Y position
#define DEF_BTNSZX      74      // Button width
#define DEF_BTNSZY      20      // Button height
```

## Network Protocol

### Request/Response Messages

```cpp
// Guild Creation
MSGID_REQUEST_CREATENEWGUILD        0x0FC94208
MSGID_RESPONSE_CREATENEWGUILD       0x0FC94209

// Guild Dissolution
MSGID_REQUEST_DISBANDGUILD          0x0FC9420A
MSGID_RESPONSE_DISBANDGUILD         0x0FC9420B

// Membership Updates (to Login Server)
MSGID_REQUEST_UPDATEGUILDINFO_NEWGUILDSMAN  0x0FC9420C
MSGID_REQUEST_UPDATEGUILDINFO_DELGUILDSMAN  0x0FC9420D

// Guild Notifications
MSGID_GUILDNOTIFY                   0x0DF30760
```

### Common Type Messages

```cpp
// Membership Actions
DEF_COMMONTYPE_JOINGUILDAPPROVE         0x0A06
DEF_COMMONTYPE_JOINGUILDREJECT          0x0A07
DEF_COMMONTYPE_DISMISSGUILDAPPROVE      0x0A08
DEF_COMMONTYPE_DISMISSGUILDREJECT       0x0A09
DEF_COMMONTYPE_CLEARGUILDNAME           0x0A25
DEF_COMMONTYPE_BANGUILD                 0x0A26  // v1.4311-3

// Crusade Integration
DEF_COMMONTYPE_SETGUILDTELEPORTLOC      0x0A54
DEF_COMMONTYPE_GUILDTELEPORT            0x0A55
DEF_COMMONTYPE_SETGUILDCONSTRUCTLOC     0x0A57

// Name Lookups (v2.171)
DEF_COMMONTYPE_REQGUILDNAME             0x0A59
```

### Notification Messages

```cpp
// Membership Queries (to Guild Master)
DEF_NOTIFY_QUERY_JOINGUILDREQPERMISSION         0x0B02
DEF_NOTIFY_QUERY_DISMISSGUILDREQPERMISSION      0x0B03
DEF_NOTIFY_WAITFORGUILDOPERATION                0x0B04

// Membership Results
DEF_NOTIFY_CANNOTJOINMOREGUILDSMAN              0x0B0D
DEF_NOTIFY_NEWGUILDSMAN                         0x0B0E
DEF_NOTIFY_DISMISSGUILDSMAN                     0x0B0F
DEF_NOTIFY_GUILDDISBANDED                       0x0B0B

// Ban System (v1.4311-3)
DEF_NOTIFY_NOGUILDMASTERLEVEL                   0x0B77
DEF_NOTIFY_SUCCESSBANGUILDMAN                   0x0B78
DEF_NOTIFY_CANNOTBANGUILDMAN                    0x0B79

// Name Lookup Response (v2.171)
DEF_NOTIFY_REQGUILDNAMEANSWER                   0x0BA6

// Guild Notify Subtypes
DEF_GUILDNOTIFY_NEWGUILDSMAN                    0x1F00
```

## Core Functions

### Guild Name Cache

```cpp
// Find or create guild name cache entry
// Returns TRUE if found, FALSE if new entry created
// Implements LRU eviction based on dwRefTime
BOOL CGame::FindGuildName(char* pName, int* ipIndex);

// Clear entire guild name cache
// Called on DEF_COMMONTYPE_CLEARGUILDNAME
void CGame::ClearGuildNameList();
```

### Operation Queue Management

```cpp
// Add operation to pending queue
// Finds first empty slot (cOpMode == NULL)
void CGame::_PutGuildOperationList(char* pName, char cOpMode);

// Remove first operation (FIFO)
// Shifts remaining operations down
void CGame::_ShiftGuildOperationList();
```

### Message Handlers

```cpp
// Join/Leave Permission Queries (Guild Master receives)
void NotifyMsg_QueryJoinGuildPermission(char* pData);
void NotifyMsg_QueryDismissGuildPermission(char* pData);

// Join Results (Applicant receives)
void NotifyMsg_JoinGuildApprove(char* pData);
void NotifyMsg_JoinGuildReject(char* pData);

// Leave Results (Member receives)
void NotifyMsg_DismissGuildApprove(char* pData);
void NotifyMsg_DismissGuildReject(char* pData);

// Guild Events
void NotifyMsg_GuildDisbanded(char* pData);
void NotifyMsg_NewGuildsMan(char* pData);
void NotifyMsg_DismissGuildsMan(char* pData);
void NotifyMsg_CannotJoinMoreGuildsMan(char* pData);
void NotifyMsg_BanGuildMan(char* pData);

// Response Handlers
void CreateNewGuildResponseHandler(char* pData);
void DisbandGuildResponseHandler(char* pData);
```

## Operation Flows

### Create Guild

```
┌─────────────────────────────────────────────────────────┐
│ Requirements: Charisma >= 20, Level >= 20, no guild    │
│ Cost: 5 Gold                                            │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
              Player opens Guild Menu (Dialog 7)
                           │
                           ▼
              Selects "Create Guild" option
                           │
                           ▼
              Enters guild name (max 21 chars)
              ┌────────────────────────────────┐
              │ Validation:                    │
              │ - Not empty                    │
              │ - Not "NONE"                   │
              │ - Spaces → underscores         │
              └────────────────────────────────┘
                           │
                           ▼
    bSendCommand(MSGID_REQUEST_CREATENEWGUILD, guildName)
                           │
                           ▼
              Server creates guild record
                           │
                           ▼
    MSGID_RESPONSE_CREATENEWGUILD received
                           │
                           ▼
    CreateNewGuildResponseHandler()
    ┌────────────────────────────────────┐
    │ Updates:                           │
    │ - m_cGuildName = guildName         │
    │ - m_iGuildRank = 0 (Master)        │
    │ - m_iTotalGuildsMan = 1            │
    └────────────────────────────────────┘
```

### Join Guild (Two-Phase Approval)

```
┌─────────────────────────────────────────────────────────┐
│ Phase 1: Player purchases admission ticket (50 Gold)   │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
         Player gives ticket to Guild Master
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│ Phase 2: Server sends to Guild Master                  │
│ DEF_NOTIFY_QUERY_JOINGUILDREQPERMISSION                │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
    NotifyMsg_QueryJoinGuildPermission()
    _PutGuildOperationList(playerName, 1)
                           │
                           ▼
    Guild Operation Dialog (8) opens for Master
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
         [Approve]                  [Reject]
              │                         │
              ▼                         ▼
    bSendCommand(                bSendCommand(
      JOINGUILDAPPROVE)            JOINGUILDREJECT)
              │                         │
              ▼                         ▼
    _ShiftGuildOperationList()   _ShiftGuildOperationList()
              │                         │
              ▼                         ▼
    Server adds member           Server rejects
              │                         │
              ▼                         ▼
    DEF_NOTIFY_NEWGUILDSMAN     Player notified
    sent to all members          of rejection
```

### Leave Guild (Two-Phase Approval)

```
┌─────────────────────────────────────────────────────────┐
│ Player purchases withdrawal ticket (50 Gold)           │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
         Player requests withdrawal
                           │
                           ▼
    Server sends to Guild Master:
    DEF_NOTIFY_QUERY_DISMISSGUILDREQPERMISSION
                           │
                           ▼
    NotifyMsg_QueryDismissGuildPermission()
    _PutGuildOperationList(playerName, 2)
                           │
                           ▼
    Guild Master reviews in Dialog 8
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
         [Approve]                  [Reject]
              │                         │
              ▼                         ▼
    DISMISSGUILDAPPROVE         DISMISSGUILDREJECT
              │                         │
              ▼                         ▼
    NotifyMsg_DismissGuildApprove()  NotifyMsg_DismissGuildReject()
    ┌─────────────────────────┐
    │ m_cGuildName = ""       │
    │ m_iGuildRank = -1       │
    └─────────────────────────┘
```

### Disband Guild

```
┌─────────────────────────────────────────────────────────┐
│ Requirements: Guild Master (m_iGuildRank == 0)         │
│ Cost: 5 Gold                                            │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
    Guild Master selects "Disband Guild"
                           │
                           ▼
    Confirmation dialog (Mode 5)
                           │
                           ▼
    bSendCommand(MSGID_REQUEST_DISBANDGUILD)
                           │
                           ▼
    Server removes guild record
                           │
                           ▼
    MSGID_RESPONSE_DISBANDGUILD received
                           │
                           ▼
    DisbandGuildResponseHandler()
                           │
                           ▼
    All members receive:
    DEF_NOTIFY_GUILDDISBANDED
                           │
                           ▼
    NotifyMsg_GuildDisbanded()
    _PutGuildOperationList(guildName, 7)
    ┌─────────────────────────┐
    │ m_cGuildName = ""       │
    │ m_iGuildRank = -1       │
    └─────────────────────────┘
```

## Guild Name Display

In-game character names show guild affiliation:

```cpp
// Localization strings
DEF_MSG_GUILDMASTER     "%s Guildmaster"    // "PlayerName Guildmaster"
DEF_MSG_GUILDSMAN       "%s Guildsman"      // "PlayerName Guildsman"
```

**Name Formatting:**
- Guild names support spaces via underscore substitution
- On send: `m_Misc.ReplaceString(cTemp, ' ', '_')`
- On receive: `m_Misc.ReplaceString(m_cGuildName, '_', ' ')`

## Crusade Integration

Guild Masters have special Crusade privileges:

### Arena Reservation
- Cost: 1500 Gold
- Duration: 2 hours
- Provides: 50 event tickets
- Faction restrictions:
  - **Aresden:** Even arenas (2,4,6,8) on even-numbered days
  - **Elvine:** Odd arenas (1,3,5,7) on odd-numbered days

### Guild Teleportation
```cpp
DEF_COMMONTYPE_SETGUILDTELEPORTLOC      0x0A54  // Set destination
DEF_COMMONTYPE_GUILDTELEPORT            0x0A55  // Teleport members
DEF_COMMONTYPE_SETGUILDCONSTRUCTLOC     0x0A57  // Construction point
```

## Validation Rules

### Guild Creation
```cpp
if (m_iGuildRank != -1) return;         // Already in guild
if (m_iCharisma < 20) return;           // Charisma too low
if (m_iLevel < 20) return;              // Level too low
if (m_bIsCrusadeMode) return;           // During crusade
if (strcmp(name, "NONE") == 0) return;  // Reserved name
if (strlen(name) == 0) return;          // Empty name
```

### Guild Operations
```cpp
if (m_iGuildRank != 0) return;          // Not Guild Master
if (m_iTotalGuildsMan >= 32) {
    // DEF_NOTIFY_CANNOTJOINMOREGUILDSMAN
}
```

## Localization Strings

### Guild Menu (lan_eng.h:929-1002)

```cpp
DRAW_DIALOGBOX_GUILDMENU1   "Make a new guild"
DRAW_DIALOGBOX_GUILDMENU4   "Break up your guild"
DRAW_DIALOGBOX_GUILDMENU7   "Buy a guild admission ticket"
DRAW_DIALOGBOX_GUILDMENU9   "Buy a guild withdrawal ticket"
DRAW_DIALOGBOX_GUILDMENU11  "Reserve the arena"
DRAW_DIALOGBOX_GUILDMENU13  "Get an entrance ticket"
// 83 total guild menu strings
```

### Guild Operations (lan_eng.h:1004-1032)

```cpp
DRAW_DIALOGBOX_GUILD_OPERATION1   "Guild admission request:"
DRAW_DIALOGBOX_GUILD_OPERATION2   "* This player wants to join your guild."
// 27 operation dialog strings
```

### Notifications (lan_eng.h:1337-1503)

```cpp
NOTIFYMSG_CANNOT_JOIN_MOREGUILDMAN1 "%s cannot join our guild."
NOTIFYMSG_CANNOT_JOIN_MOREGUILDMAN2 "Guild is full."
NOTIFYMSG_DISMISS_GUILDMAN1         "%s has withdrawn from our guild."
NOTIFYMSG_NEW_GUILDMAN1             "%s has joined your guild."
```

## Initialization

```cpp
// Game.cpp, Line 4958 - Guild name cache
for (i = 0; i < DEF_MAXGUILDNAMES; i++) {
    m_stGuildName[i].dwRefTime = 0;
    m_stGuildName[i].iGuildRank = -1;
    ZeroMemory(m_stGuildName[i].cCharName, sizeof(m_stGuildName[i].cCharName));
    ZeroMemory(m_stGuildName[i].cGuildName, sizeof(m_stGuildName[i].cGuildName));
}

// Game.cpp, Line 4997 - Operation queue
for (i = 0; i < 100; i++) {
    m_stGuildOpList[i].cOpMode = NULL;
    ZeroMemory(m_stGuildOpList[i].cName, sizeof(m_stGuildOpList[i].cName));
}

// Player state
m_cGuildName[0] = '\0';     // No guild
m_iGuildRank = -1;          // Not in guild
m_iTotalGuildsMan = 0;      // No members
```

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.4311-3 | - | Added ban guild member feature (DEF_COMMONTYPE_BANGUILD) |
| v2.171 | 2002-6-14 | Added guild name database (100 entries), guild teleport |
| v2.18+ | - | Crusade system integration with guild arenas |

## Known Issues / Technical Debt

1. **Hardcoded Limits:** 32 members and 100 cache entries are compile-time constants
2. **Blocking Operations:** Join/leave require real-time Guild Master presence
3. **No Offline Support:** Operations fail if Guild Master is offline
4. **Name Collision:** Underscore substitution can cause display issues
5. **Cache Eviction:** LRU based on `dwRefTime` may evict frequently-accessed entries during high activity
6. **Queue Overflow:** 100-slot operation queue has no overflow handling
7. **No Rank Hierarchy:** Only two effective ranks (Master/Member)

## Modernization Notes

### Recommended Changes

1. **Configurable Limits:** Move DEF_MAXGUILDSMAN to runtime config
2. **Async Operations:** Queue join/leave for offline Guild Master approval
3. **Rank System:** Add customizable ranks with granular permissions
4. **Event System:** Replace notification handlers with publish/subscribe
5. **Data Binding:** Separate guild data from CGame class
6. **Cache Strategy:** Consider time-based expiration over pure LRU
7. **Validation:** Centralize guild name validation rules

### Suggested Architecture

```cpp
namespace hb::guild {
    class GuildSystem {
    public:
        void initialize();
        void update();

        // Player's guild
        [[nodiscard]] std::optional<GuildMembership> membership() const;

        // Operations
        std::future<Result> createGuild(std::string_view name);
        std::future<Result> disbandGuild();
        std::future<Result> requestJoin(EntityId guildMaster);
        std::future<Result> requestLeave();

        // Guild Master operations
        void approveJoin(std::string_view playerName);
        void rejectJoin(std::string_view playerName);
        void approveLeave(std::string_view playerName);
        void rejectLeave(std::string_view playerName);
        void banMember(std::string_view playerName);

        // Lookups
        [[nodiscard]] std::optional<GuildInfo> lookupGuild(std::string_view playerName);

    private:
        GuildMembership m_membership;
        LRUCache<std::string, GuildInfo> m_nameCache{100};
        std::queue<PendingOperation> m_operationQueue;
    };

    struct GuildMembership {
        std::string guildName;
        GuildRank rank;
        int memberCount;
    };

    enum class GuildRank {
        None = -1,
        Master = 0,
        Member = 1
    };
}
```
