# Social Dialogs

## Overview

The social dialog systems in Helbreath handle all player-to-player interaction interfaces including guild management, party coordination, and chat communication. These dialogs are tightly integrated with the networking layer, sending real-time requests to the server and receiving asynchronous responses.

The social systems comprise four main dialog types:
- **Guild Menu (ID 7)** - Guild creation, membership, and fightzone reservation
- **Guild Operations (ID 8)** - Approval queue for guild join/leave requests
- **Party List (ID 32)** - Party formation, membership, and disbanding
- **Chat Window (ID 10)** - Message display, scrolling, and input handling

---

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` | Dialog structures, member variables, constants |
| `Game.cpp` | `DrawDialogBox_*` and `DlgBoxClick_*` implementations |
| `NetMessages.h` | Network message IDs for guild/party/chat operations |
| `Msg.h` / `Msg.cpp` | Chat message wrapper class |

---

## Key Data Structures

### Dialog Box Info Structure

All 41 dialogs share this common structure:

```cpp
struct {
    int sV1, sV2, sV3, sV4, sV5, sV6, sV7, sV8;
    int sV9, sV10, sV11, sV12, sV13, sV14;
    DWORD dwV1, dwV2, dwT1;
    BOOL bFlag;
    short sX, sY;           // Dialog position
    short sSizeX, sSizeY;   // Dialog dimensions
    short sView;            // Scroll position
    char cStr[32];          // String buffer 1
    char cStr2[32];         // String buffer 2
    char cStr3[32];         // String buffer 3
    char cStr4[32];         // String buffer 4
    char cMode;             // Current dialog state/mode
    BOOL bIsScrollSelected; // Scroll interaction flag
} m_stDialogBoxInfo[41];
```

### Guild Operation List

Queue for pending guild membership operations:

```cpp
struct {
    char cName[22];    // Player name involved in operation
    char cOpMode;      // Operation type (1-7)
} m_stGuildOpList[100];
```

### Party Member Structure

```cpp
struct {
    char cStatus;      // Member status flags
    char cName[12];    // Member character name
} m_stPartyMember[DEF_MAXPARTYMEMBERS];  // 8 members max
```

### Chat Message Class (CMsg)

```cpp
class CMsg {
public:
    DWORD m_dwTime;    // Message type (determines color)
    char* m_pMsg;      // Message text content
    // Constructor/destructor manage m_pMsg allocation
};
```

---

## Constants & Limits

### Dialog IDs

```cpp
#define DEF_DLGBOXTYPE_GUILDMENU      7
#define DEF_DLGBOXTYPE_GUILDOPERATION 8
#define DEF_DLGBOXTYPE_CHAT           10
#define DEF_DLGBOXTYPE_PARTY          32
```

### Button Layout Constants

```cpp
#define DEF_BTNSZX      74    // Standard button width
#define DEF_BTNSZY      20    // Standard button height
#define DEF_LBTNPOSX    30    // Left button X offset from dialog origin
#define DEF_RBTNPOSX    154   // Right button X offset from dialog origin
#define DEF_BTNPOSY     292   // Standard button Y offset from dialog origin
```

### Social System Limits

```cpp
#define DEF_MAXPARTYMEMBERS    8     // Maximum party size
#define DEF_MAXGUILDSMAN       32    // Maximum guild members
#define DEF_MAXCHATMSGS        500   // Chat history capacity
#define DEF_MAXCHATSCROLLMSGS  80    // Scrollable chat messages
#define DEF_MAXWHISPERMSG      5     // Tracked whisper messages
```

---

## Guild Menu Dialog (ID 7)

### Core Functions

```cpp
void CGame::DrawDialogBox_GuildMenu(short msX, short msY);
void CGame::DlgBoxClick_GuildMenu(short msX, short msY);
```

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `m_iGuildRank` | int | Player's guild rank (-1 = none, 0 = leader, 1+ = member) |
| `m_cGuildName[22]` | char[] | Current guild name |
| `m_iCharisma` | int | Character CHR stat (guild creation requires >= 20) |
| `m_iLevel` | int | Character level (guild creation requires >= 20) |
| `m_bIsCrusadeMode` | BOOL | Whether crusade war is active |
| `m_iFightzoneNumber` | int | Reserved fightzone (-1 = none, 0 = pending, 1-8 = zone) |

### Dialog Modes

| Mode | State | Description |
|------|-------|-------------|
| 0 | Main Menu | Shows available guild operations |
| 1 | Name Input | Guild name entry for creation |
| 2 | Processing | "Guild creation is being processed..." |
| 3 | Success | Guild created successfully |
| 4 | Rejected | Guild name already exists |
| 5 | Disband Confirm | Confirmation to disband guild |
| 6 | Disband Processing | "Guild disbanding in progress..." |
| 7 | Disband Success | Guild disbanded successfully |
| 8 | Disband Failed | Disband operation failed |
| 9 | Join Cost | Join guild costs 5 Gold |
| 10 | Join Submitted | Admission request submitted |
| 11 | Leave Cost | Leave guild costs 5 Gold |
| 12 | Leave Submitted | Secession request submitted |
| 13 | Fightzone Select | Select crusade fightzone (1-8) |
| 14-22 | Crusade Messages | Various crusade-related status messages |

### Menu Items (Mode 0)

| Item | Hit Box (relative to sX, sY) | Requirements |
|------|------------------------------|--------------|
| Create Guild | X: [67, 210], Y: [93, 108] | Rank == -1, CHR >= 20, Level >= 20 |
| Disband Guild | X: [59, 222], Y: [112, 129] | Rank == 0 (leader only) |
| Join Guild | X: [48, 238], Y: [133, 150] | None |
| Leave Guild | X: [47, 239], Y: [153, 169] | None |
| Reserve Fightzone | X: [59, 216], Y: [173, 199] | Rank == 0, Not in crusade mode |

### Dialog Dimensions

- **Width:** 257 pixels
- **Height:** 320 pixels

### Network Messages

**Outgoing:**

| Action | Message ID | Data |
|--------|------------|------|
| Create Guild | `MSGID_REQUEST_CREATENEWGUILD` | Guild name string |
| Disband Guild | `MSGID_REQUEST_DISBANDGUILD` | None |
| Buy Admission Ticket | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_REQ_PURCHASEITEM` | Item type |
| Buy Secession Ticket | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_REQ_PURCHASEITEM` | Item type |
| Reserve Fightzone | `MSGID_REQUEST_FIGHTZONE_RESERVE` | Zone number (1-8) |

**Response Handlers:**

| Function | Purpose |
|----------|---------|
| `CreateNewGuildResponseHandler()` | Updates `m_iGuildRank` on success/failure |
| `DisbandGuildResponseHandler()` | Clears guild name and rank |

---

## Guild Operations Dialog (ID 8)

### Core Functions

```cpp
void CGame::DrawDialogBox_GuildOperation(short msX, short msY);
void CGame::DlgBoxClick_GuildOp(short msX, short msY);
```

### Supporting Functions

```cpp
// Add operation to the queue
void CGame::_PutGuildOperationList(char* pName, char cOpMode);

// Process next operation in queue
void CGame::_ShiftGuildOperationList();
```

### Operation Modes

| OpMode | Description | User Actions |
|--------|-------------|--------------|
| 1 | Membership request received | Approve / Reject |
| 2 | Secession request received | Approve / Reject |
| 3 | Secession completed | OK to acknowledge |
| 4 | Membership request rejected | OK to acknowledge |
| 5 | Secession request rejected | OK to acknowledge |
| 6 | Member banned from guild | OK to acknowledge |
| 7 | Guild disbanded | OK to acknowledge |

### Network Messages

| Action | Message ID | Additional Data |
|--------|------------|-----------------|
| Approve Join | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_JOINGUILDAPPROVE` | Player name |
| Reject Join | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_JOINGUILDREJECT` | Player name |
| Approve Leave | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_DISMISSGUILDAPPROVE` | Player name |
| Reject Leave | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_DISMISSGUILDREJECT` | Player name |

### Queue Processing

The guild operations use a 100-entry queue (`m_stGuildOpList`) to handle multiple pending requests:

1. New operations added via `_PutGuildOperationList()`
2. Current operation displayed from `m_stGuildOpList[0]`
3. After user action, `_ShiftGuildOperationList()` moves queue forward
4. Dialog remains open if more operations pending

---

## Party List Dialog (ID 32)

### Core Functions

```cpp
void CGame::DrawDialogBox_Party(short msX, short msY);
void CGame::DlgBoxClick_Party(short msX, short msY);
```

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `m_stPartyMember[8]` | struct[] | Party member array |
| `m_stPartyMemberNameList[9]` | char[][] | Name list for display |
| `m_iTotalPartyMember` | int | Current party size |
| `m_iPartyStatus` | int | Party state (0 = none, 1-2 = active) |
| `m_cMCName[12]` | char[] | Local player's character name |
| `m_iPointCommandType` | int | Targeting mode (200 = party creation) |

### Dialog Modes

| Mode | State | Description |
|------|-------|-------------|
| 0 | Main Menu | Shows party options |
| 1 | Invitation Received | Accept/reject party invite |
| 2 | Creation Pending | Waiting for party creation approval |
| 3 | Creation Submitted | Request sent to server |
| 4 | Join Pending | Membership request in progress |
| 5 | Join Processing | Server processing join |
| 6 | Disbanded | Party has been disbanded |
| 7 | Disband Failed | Disband operation failed |
| 8 | Level Mismatch | Party members incompatible |
| 9 | Leader Changed | New party leader assigned |
| 10 | Roster Full | Cannot add more members |
| 11 | Disband Confirm | Confirmation to disband |

### Menu Items (Mode 0)

| Item | Hit Box (relative to sX, sY) | Condition |
|------|------------------------------|-----------|
| Create Party | X: [80, 195], Y: [80, 100] | `m_iPartyStatus == 0` |
| Disband Party | X: [80, 195], Y: [100, 120] | `m_iPartyStatus != 0` |
| Join Party | X: [80, 195], Y: [120, 140] | `m_iPartyStatus != 0` |

### Party Creation Flow

1. User clicks "Create Party" in menu
2. `m_iPointCommandType` set to 200 (targeting mode)
3. User clicks on target player in game world
4. `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_REQUEST_JOINPARTY` sent
5. Target receives invitation (Mode 1)
6. Target accepts/rejects
7. Server updates both players' party status

### Network Messages

| Action | Message ID | Data |
|--------|------------|------|
| Create/Join Request | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_REQUEST_JOINPARTY` | Target name |
| Accept/Reject Invite | `MSGID_COMMAND_COMMON` + `DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY` | 1 = accept, 0 = reject |
| Disband Party | `MSGID_COMMAND_COMMON` + disband command | None |

### Party Notifications

| Function | Trigger |
|----------|---------|
| `NotifyMsg_PartyCreated()` | Party successfully formed |
| `NotifyMsg_PartyDisbanded()` | Party has been disbanded |
| `NotifyMsg_PartyMemberLeft()` | Member left the party |
| `NotifyMsg_PartyLeaderChanged()` | Leadership transferred |

---

## Chat Window Dialog (ID 10)

### Core Functions

```cpp
void CGame::DrawDialogBox_Chat(short msX, short msY, short msZ, char cLB);
```

Note: This function takes additional parameters:
- `msZ` - Mouse wheel delta for scrolling
- `cLB` - Left button state for scroll bar dragging

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `m_pChatScrollList[80]` | CMsg*[] | Scrollable message buffer |
| `m_pChatMsgList[500]` | CMsg*[] | Full chat history |
| `m_pWhisperMsg[5]` | CMsg*[] | Recent whisper messages |
| `m_cChatMsg[64]` | char[] | Current input buffer |
| `m_cBackupChatMsg[64]` | char[] | Last sent message backup |
| `m_cWhisperIndex` | char | Whisper tracking index |

### Message Display

| Property | Value |
|----------|-------|
| Visible Lines | 8 messages |
| Line Height | 13 pixels |
| Text Area X | sX + 25 |
| Text Area Y | sY + 41 to sY + 127 (bottom to top) |
| Max Scroll | 72 (80 - 8 visible) |

### Message Color Coding

| Type Value | RGB Color | Category |
|------------|-----------|----------|
| 0 | (230, 230, 230) | Normal chat |
| 1 | (130, 200, 130) | Guild message |
| 2 | (255, 130, 130) | Enemy/PvP message |
| 3 | (130, 130, 255) | Whisper/Private |
| 4 | (230, 230, 130) | System message |
| 10 | (180, 255, 180) | Special message |
| 20 | (150, 150, 170) | Admin/GM message |

### Scroll Bar Interaction

| Element | Position (relative to sX, sY) |
|---------|-------------------------------|
| Up Arrow | X: [336, 361], Y: [18, 28] |
| Down Arrow | X: [336, 361], Y: [140, 163] |
| Draggable Track | X: [336, 361], Y: [28, 140] |

**Scroll Position Calculation:**
```cpp
sView = (int)((msY - (sY + 28)) * (DEF_MAXCHATSCROLLMSGS - 8) / 105.0f);
```

### Chat Input

Chat input is handled via `StartInputString()` which creates an input field:
- Maximum input length: 64 characters
- Backup feature: Previous message stored in `m_cBackupChatMsg`
- Submit: Calls `bSendCommand()` for network transmission

### Mouse Wheel Support

The `msZ` parameter enables mouse wheel scrolling:
- Positive values: Scroll up (newer messages)
- Negative values: Scroll down (older messages)
- Step size: 1 message per wheel tick

---

## Integration Points

### Guild System Integration

| System | Integration |
|--------|-------------|
| **Character Stats** | Guild creation requires CHR >= 20, Level >= 20 |
| **Inventory** | Admission/secession tickets purchased as items |
| **Crusade** | Fightzone reservation tied to guild leadership |
| **Chat** | Guild chat messages routed through chat system |

### Party System Integration

| System | Integration |
|--------|-------------|
| **Entity System** | Party creation targets other player entities |
| **Combat** | Party affects PvP targeting rules |
| **UI Targeting** | Uses `m_iPointCommandType` for player selection |

### Chat System Integration

| System | Integration |
|--------|-------------|
| **Network** | Real-time message delivery |
| **Localization** | System messages use localized strings |
| **Game Monitor** | Bad word filtering applied |
| **Whisper** | Separate tracking for private messages |

---

## State Management

### Guild State

```cpp
// Guild membership state
m_iGuildRank = -1;  // No guild
m_iGuildRank = 0;   // Guild leader
m_iGuildRank > 0;   // Guild member (rank number)

// Guild name (empty if no guild)
m_cGuildName[22];

// Operation queue
m_stGuildOpList[100];  // Pending operations
```

### Party State

```cpp
// Party status
m_iPartyStatus = 0;  // Not in party
m_iPartyStatus = 1;  // In party (member)
m_iPartyStatus = 2;  // In party (leader)

// Party roster
m_stPartyMember[8];      // Member data
m_iTotalPartyMember;     // Current count
```

### Chat State

```cpp
// Message buffers
m_pChatScrollList[80];   // Visible scrollable
m_pChatMsgList[500];     // Full history

// Scroll state
m_stDialogBoxInfo[10].sView;             // Current position
m_stDialogBoxInfo[10].bIsScrollSelected; // Dragging scroll
```

---

## Known Issues / Technical Debt

### Guild System

1. **Hardcoded Requirements** - Guild creation stats (CHR 20, Level 20) are magic numbers
2. **Fixed Queue Size** - 100-entry operation queue could overflow on busy servers
3. **No Pagination** - Guild member list has no scrolling for 32 members
4. **Synchronous Tickets** - Item purchase blocks on server response

### Party System

1. **Magic Targeting Mode** - `m_iPointCommandType = 200` is undocumented magic number
2. **Level Mismatch Handling** - Error mode 8 lacks detailed information
3. **No Party Chat** - Party communication uses general chat with type flag

### Chat System

1. **Fixed Buffer Sizes** - 500 history, 80 scrollable are arbitrary limits
2. **Color Magic Numbers** - Message type colors hardcoded, not configurable
3. **No Timestamps** - `m_dwTime` misused for type, not actual time
4. **Scroll Math** - Float division for scroll position could cause jitter

### General Issues

1. **No Validation** - Hit box checks don't validate dialog visibility
2. **Global State** - All state in CGame member variables
3. **Tight Coupling** - Dialog logic mixed with network handling
4. **No Error Recovery** - Failed operations may leave dialogs in bad state

---

## Modernization Notes

### Recommended Architecture

```
src/ui/dialogs/
    guild_menu_dialog.cpp/hpp      # Guild menu UI
    guild_operations_dialog.cpp/hpp # Guild queue UI
    party_dialog.cpp/hpp           # Party management UI
    chat_dialog.cpp/hpp            # Chat window UI

src/social/
    guild_system.cpp/hpp           # Guild business logic
    party_system.cpp/hpp           # Party business logic
    chat_system.cpp/hpp            # Chat message management
```

### Key Improvements

1. **Separate UI from Logic** - Dialogs should only handle rendering/input
2. **Event-Driven Updates** - Use observer pattern for state changes
3. **Type-Safe Messages** - Replace magic numbers with enums
4. **Configurable Colors** - Load chat colors from configuration
5. **Proper Timestamps** - Store actual time with messages
6. **Dynamic Buffers** - Use `std::vector` instead of fixed arrays
7. **Async Operations** - Non-blocking network requests with callbacks
8. **Input Validation** - Centralized validation for guild names, etc.

### Data Structures

```cpp
// Modern guild member
struct GuildMember {
    std::string name;
    GuildRank rank;
    bool online;
    std::chrono::system_clock::time_point lastSeen;
};

// Modern party member
struct PartyMember {
    EntityId entityId;
    std::string name;
    int healthPercent;
    bool inRange;
};

// Modern chat message
struct ChatMessage {
    ChatMessageType type;
    std::string sender;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    bool isRead;
};
```

### Network Abstraction

```cpp
class GuildService {
public:
    std::future<GuildCreateResult> createGuild(std::string_view name);
    std::future<void> disbandGuild();
    std::future<void> respondToApplication(std::string_view name, bool accept);

    // Events
    EventSubscription onMemberJoined(std::function<void(const GuildMember&)>);
    EventSubscription onMemberLeft(std::function<void(std::string_view)>);
};
```
