# Party System

## Overview

The party system allows up to 8 players to join together for cooperative gameplay. Players in a party share experience based on level ratios and can attack each other without safe attack mode restrictions. The system is fully integrated into the monolithic `CGame` class and uses dialog-based UI (dialog index 32) with network messaging to synchronize party state.

**Key Features:**
- 8-member maximum capacity
- Experience sharing by level ratio
- Safe attack mode bypass for party members
- Visual identification in game world (", Party Member" suffix)
- Invitation-based joining with accept/reject

---

## Source Files

| File | Description |
|------|-------------|
| `Game.h` | Party member structures, status variables, constants |
| `Game.cpp` | Dialog rendering, click handling, notification processing |
| `NetMessages.h` | Party-related network message definitions |
| `lan_eng.h` (and variants) | Localized UI text strings |

---

## Key Data Structures

### Party Member Storage

**Location:** `Game.h` lines 604-614

```cpp
// Primary member storage (8 slots)
struct {
    char cStatus;        // Member status (0 = inactive/empty slot)
    char cName[12];      // Character name (11 chars + null terminator)
} m_stPartyMember[DEF_MAXPARTYMEMBERS];

// Display name list (9 elements - one extra unused)
struct {
    char cName[12];      // Member name for display
} m_stPartyMemberNameList[DEF_MAXPARTYMEMBERS+1];
```

**Notes:**
- `m_stPartyMember[].cStatus` is initialized but never actually used in logic
- All member tracking uses `m_stPartyMemberNameList[].cName` instead
- Extra element in name list (9 vs 8) appears unused

### Party Status Variables

**Location:** `Game.h` lines 791-792

```cpp
int m_iTotalPartyMember;  // Current number of party members
int m_iPartyStatus;       // Party state (0, 1, or 2)
```

**Party Status Values:**

| Value | Meaning |
|-------|---------|
| 0 | Not in a party |
| 1 | Party created, pending acceptance |
| 2 | Actively in party with members |

### Dialog Box Info

Party dialog uses index 32 of the dialog info array:

```cpp
m_stDialogBoxInfo[32].cMode   // Current dialog screen (0-11)
m_stDialogBoxInfo[32].cStr    // Inviter/target name buffer (32 bytes)
m_stDialogBoxInfo[32].sX/sY   // Dialog position
```

---

## Constants & Limits

**Location:** `Game.h` line 140

```cpp
#define DEF_MAXPARTYMEMBERS    8    // Maximum party members
```

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_MAXPARTYMEMBERS` | 8 | Hard limit on party size |
| Dialog Index | 32 | Party dialog box identifier |
| Name Buffer | 12 bytes | Character name storage (11 + null) |
| Network Name | 10-11 bytes | Transmitted name size |

---

## Dialog Modes

The party dialog (`m_stDialogBoxInfo[32].cMode`) displays different screens:

| Mode | Display | Purpose |
|------|---------|---------|
| 0 | Main menu | Choose: Create, Leave, View Members |
| 1 | Invitation received | "X offered you to join..." with Accept/Reject |
| 2 | Targeting mode | "Click character to invite..." |
| 3 | Awaiting response | "You asked X to join..." with Cancel |
| 4 | Member roster | List of all party member names |
| 5 | Withdrawing | "Withdrawing from party..." status |
| 6 | Left successfully | "You withdrew from party." confirmation |
| 7 | Leave failed | Error with retry suggestion |
| 8 | Joined party | Success + party benefits explanation |
| 9 | Join failed | Error with possible reasons |
| 10 | Disbanded | "Party has been dismissed." notice |
| 11 | Confirm leave | "Are you sure?" with Confirm/Cancel |

---

## Core Functions

### Dialog Rendering

**Location:** `Game.cpp` line 39971

```cpp
void CGame::DrawDialogBox_Party(short msX, short msY)
```

Renders the party dialog based on current mode. Uses sprites:
- `DEF_SPRID_INTERFACE_ND_GAME2` - Dialog background
- `DEF_SPRID_INTERFACE_ND_TEXT` - Text overlay
- `DEF_SPRID_INTERFACE_ND_BUTTON` - Buttons

**Mode 0 Rendering Logic:**
- Shows three clickable options
- Colors based on `m_iPartyStatus`:
  - Gray (65,65,65) - Inactive/disabled
  - White (255,255,255) - Hovered
  - Dark blue (4,0,50) - Not available in current state

**Mode 4 Member List Rendering:**
```cpp
// Lists all members with 15-pixel vertical spacing
for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) {
    if (strlen(m_stPartyMemberNameList[i].cName) > 0) {
        // Draw member name at (sX + offset, sY + 50 + (i * 15))
    }
}
```

### Dialog Click Handler

**Location:** `Game.cpp` line 19043

```cpp
void CGame::DlgBoxClick_Party(short msX, short msY)
```

**Mode 0 - Menu Navigation:**
```cpp
// "Join party" - Only when m_iPartyStatus == 0
m_stDialogBoxInfo[32].cMode = 2;
m_bIsGetPointingMode = TRUE;
m_iPointCommandType = 200;  // Party targeting flag

// "Leave party" - Only when m_iPartyStatus != 0
m_stDialogBoxInfo[32].cMode = 11;  // Confirmation

// "View members" - Only when m_iPartyStatus != 0
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_JOINPARTY,
             NULL, 2, NULL, NULL, m_cMCName);
m_stDialogBoxInfo[32].cMode = 4;
```

**Mode 1 - Invitation Response:**
```cpp
// Accept button
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY,
             NULL, 1, NULL, NULL, m_stDialogBoxInfo[32].cStr);

// Reject button
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY,
             NULL, 0, NULL, NULL, m_stDialogBoxInfo[32].cStr);
```

**Mode 11 - Confirm Leave:**
```cpp
// Confirm button
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_JOINPARTY,
             NULL, NULL, NULL, NULL, m_cMCName);
m_stDialogBoxInfo[32].cMode = 5;  // "Withdrawing..."
```

### Initialization

**Location:** `Game.cpp` lines 5058-5061, 5141-5143

```cpp
// Clear all member data
for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) {
    m_stPartyMember[i].cStatus = 0;
    ZeroMemory(m_stPartyMember[i].cName, sizeof(m_stPartyMember[i].cName));
    ZeroMemory(m_stPartyMemberNameList[i].cName,
               sizeof(m_stPartyMemberNameList[i].cName));
}
m_iTotalPartyMember = 0;
m_iPartyStatus = 0;
```

---

## Network Protocol

### Message Types

**Location:** `NetMessages.h`

```cpp
// Command messages
#define MSGID_COMMAND_COMMON                   0x0FA314DC
#define DEF_COMMONTYPE_REQUEST_JOINPARTY       0x0A31  // Request join/list
#define DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY 0x0A30  // Accept/reject invite

// Notification messages
#define MSGID_NOTIFY                           0x0FA314D0
#define DEF_NOTIFY_RESPONSE_CREATENEWPARTY     0x0B80  // Party created
#define DEF_NOTIFY_QUERY_JOINPARTY             0x0B81  // Invitation received
#define DEF_NOTIFY_PARTY                       0x0BA2  // General party update
```

### Request Packets

**Join Party Request:**
```
Message: MSGID_COMMAND_COMMON + DEF_COMMONTYPE_REQUEST_JOINPARTY
Parameter 3:
  - 1: Create/send invitation to target
  - 2: Request member list
  - NULL: Leave party
Parameter 6: Player's character name (m_cMCName)
```

**Accept/Reject Request:**
```
Message: MSGID_COMMAND_COMMON + DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY
Parameter 3:
  - 1: Accept invitation
  - 0: Reject invitation
  - 2: Cancel pending request
Parameter 6: Inviter's name
```

### Notification Handler

**Location:** `Game.cpp` line 30207

```cpp
case DEF_NOTIFY_PARTY:
    cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
    sp = (short *)cp;
    sV1 = *sp;  // Operation type
    cp += 2;
    sp = (short *)cp;
    sV2 = *sp;  // Sub-operation/status
    cp += 2;
    sp = (short *)cp;
    sV3 = *sp;  // Additional param (member count)
    cp += 2;
    // Next 10 bytes: character name (if applicable)
```

**Operation Types (sV1):**

| sV1 | sV2 | Meaning | Client Action |
|-----|-----|---------|---------------|
| 1 | 0 | Create failed | Show mode 9 (error) |
| 1 | 1 | Create successful | Request member list, show mode 8 |
| 2 | - | Party disbanded | Clear data, show mode 10 |
| 4 | 0 | Join failed | Show mode 9 (error) |
| 4 | 1 | Join successful | Add member, set status=2, show mode 8 |
| 5 | - | Member list sync | Populate roster with sV3 members |
| 6 | 0 | Leave failed | Show mode 7 (error) |
| 6 | 1 | Leave successful | Remove member, update status |
| 7 | - | Leader disbanded | Show mode 9 |
| 8 | - | Forced leave | Clear all data, reset status |

### Invitation Handler

**Location:** `Game.cpp` line 30820

```cpp
case DEF_NOTIFY_QUERY_JOINPARTY:
    EnableDialogBox(32, NULL, NULL, NULL);
    m_stDialogBoxInfo[32].cMode = 1;
    ZeroMemory(m_stDialogBoxInfo[32].cStr, sizeof(m_stDialogBoxInfo[32].cStr));
    cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
    strcpy(m_stDialogBoxInfo[32].cStr, cp);  // Store inviter's name
```

---

## Member List Management

### Adding Members

**Location:** `Game.cpp` lines 30269-30276

```cpp
m_iTotalPartyMember++;
for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) {
    if (strlen(m_stPartyMemberNameList[i].cName) == 0) {
        ZeroMemory(m_stPartyMemberNameList[i].cName,
                   sizeof(m_stPartyMemberNameList[i].cName));
        memcpy(m_stPartyMemberNameList[i].cName, cTxt, 10);
        goto NMH_LOOPBREAK1;  // Exit loop
    }
}
```

### Removing Members

**Location:** `Game.cpp` lines 30325-30331

```cpp
for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) {
    if (strcmp(m_stPartyMemberNameList[i].cName, cTxt) == 0) {
        ZeroMemory(m_stPartyMemberNameList[i].cName,
                   sizeof(m_stPartyMemberNameList[i].cName));
        m_iTotalPartyMember--;
        goto NMH_LOOPBREAK2;
    }
}
```

### Full List Sync

**Location:** `Game.cpp` lines 30286-30295

```cpp
case 5:  // Server sends full member list
    m_iTotalPartyMember = NULL;  // Clear (should be = 0)
    for (i = 0; i < DEF_MAXPARTYMEMBERS; i++)
        ZeroMemory(m_stPartyMemberNameList[i].cName, ...);

    m_iTotalPartyMember = sV3;  // Set count from packet
    for (i = 1; i <= sV3; i++) {  // Note: 1-indexed loop
        memcpy(m_stPartyMemberNameList[i-1].cName, cp, 10);
        cp += 11;  // Each entry: 10 name + 1 padding
    }
```

---

## Party Targeting System

When creating a party, the client enters point-and-click targeting mode:

```cpp
m_bIsGetPointingMode = TRUE;    // Enable targeting cursor
m_iPointCommandType = 200;      // Party targeting identifier
```

**Point Command Type Ranges:**
- `0-99`: Item-based commands
- `100-199`: Skill-based targeting
- `200+`: Party/special system commands

Player clicks on a character in the game world, and `PointCommandHandler()` processes the click to send the invitation.

---

## World Rendering Integration

**Location:** `Game.cpp` lines 33498-33505

Party members are visually identified when rendering player names:

```cpp
if (m_iPartyStatus != NULL) {
    for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) {
        if (strcmp(m_stPartyMemberNameList[i].cName, pName) == 0) {
            strcat(cTxt, BGET_NPC_NAME23);  // ", Party Member"
            break;
        }
    }
}
```

---

## Event Notifications

Party events are announced via the event list:

```cpp
// Member joined
wsprintf(G_cTxt, NOTIFY_MSG_HANDLER1, cTxt);  // "%s joined the party."
AddEventList(G_cTxt, 10);  // 10 = yellow/special color

// Member left
wsprintf(G_cTxt, NOTIFY_MSG_HANDLER2, cTxt);  // "%s withdrew from the party."
AddEventList(G_cTxt, 10);
```

---

## State Machine

```
[No Party] m_iPartyStatus = 0
    │
    ├─► User clicks "Join" ─► Mode 2 (Targeting)
    │       │
    │       └─► Click target ─► Send REQUEST_JOINPARTY
    │               │
    │               ├─► Success ─► [Pending] m_iPartyStatus = 1
    │               └─► Failure ─► Mode 9 (Error)
    │
[Pending] m_iPartyStatus = 1
    │
    ├─► Target accepts ─► [Active] m_iPartyStatus = 2, Mode 8
    └─► Target rejects ─► Mode 9, reset to status 0

[Active] m_iPartyStatus = 2
    │
    ├─► User clicks "Leave" ─► Mode 11 (Confirm?)
    │       ├─► Confirms ─► Mode 5 ─► Success: Mode 6, status 0
    │       │                      └─► Failure: Mode 7
    │       └─► Cancels ─► Mode 0
    │
    ├─► Member leaves ─► Update roster, show event
    ├─► Leader disbands ─► Mode 10, status 0
    └─► Forced leave ─► Clear data, status 0

[Incoming Invitation]
    │
    └─► DEF_NOTIFY_QUERY_JOINPARTY ─► Mode 1
            ├─► Accept ─► [Active] status 2, Mode 8
            └─► Reject ─► Close dialog
```

---

## Localization Strings

**Location:** `lan_eng.h` lines 1094-1154

Key string constants:

```cpp
#define DRAW_DIALOGBOX_PARTY1   "Join a party."
#define DRAW_DIALOGBOX_PARTY4   "Withdraw from the party."
#define DRAW_DIALOGBOX_PARTY7   "See the list of party members."
#define DRAW_DIALOGBOX_PARTY10  "You are not in a party."
#define DRAW_DIALOGBOX_PARTY16  "%s offered you to"
#define DRAW_DIALOGBOX_PARTY21  "Would you like to join party?"
#define DRAW_DIALOGBOX_PARTY26  "You asked %s to"
#define DRAW_DIALOGBOX_PARTY35  "You withdrew from the party."
#define DRAW_DIALOGBOX_PARTY40  "You joined the party."
#define DRAW_DIALOGBOX_PARTY52  "Your party has been dismissed."
#define DRAW_DIALOGBOX_PARTY55  "Would you like to withdraw from the party?"

#define NOTIFY_MSG_HANDLER1     "%s joined the party."
#define NOTIFY_MSG_HANDLER2     "%s withdrew from the party."
#define BGET_NPC_NAME23         ", Party Member"
```

---

## Integration Points

### Depends On
- **Dialog System** - `m_stDialogBoxInfo`, sprite-based rendering
- **Network System** - `bSendCommand()`, notification handlers
- **Targeting System** - `m_iPointCommandType`, `m_bIsGetPointingMode`
- **Event List** - `AddEventList()` for notifications
- **Localization** - Party UI text strings

### Used By
- **Combat System** - Safe attack mode bypass for party members
- **Experience System** - Level-based sharing (server-side calculation)
- **Name Rendering** - Party member label suffix
- **Chat System** - Potential party chat channel

---

## Known Issues / Technical Debt

1. **Unused Status Field**: `m_stPartyMember[].cStatus` is initialized but never used - all logic uses name list instead

2. **Array Size Mismatch**: `m_stPartyMemberNameList[DEF_MAXPARTYMEMBERS+1]` has 9 elements but only 8 are used

3. **Off-by-One Loop**: Member list sync uses 1-indexed loop with `i-1` array access

4. **NULL vs 0**: Code uses `NULL` for integer assignments (`m_iTotalPartyMember = NULL`)

5. **Goto Statements**: Uses `goto NMH_LOOPBREAK` instead of `break` for loop exits

6. **Buffer Size Inconsistency**: Names stored in 12-byte buffers but transmitted as 10-11 bytes

7. **Global Buffer Risk**: All formatting uses `G_cTxt` (128 bytes) - potential overflow

8. **No Persistence**: Party state is session-only, logging out dissolves party

---

## Modernization Notes

### Recommended Changes

1. **Proper Data Structure**:
   ```cpp
   struct PartyMember {
       std::string name;
       bool isActive{false};
       // Add: HP percentage, position, online status
   };
   std::array<PartyMember, 8> m_partyMembers;
   ```

2. **State Machine Class**:
   ```cpp
   enum class PartyState { None, Creating, Pending, Active };
   class PartySystem {
       PartyState m_state{PartyState::None};
       std::vector<PartyMember> m_members;
   };
   ```

3. **Event-Based Updates**: Use EventBus for party notifications instead of direct dialog manipulation

4. **Type-Safe Protocol**: Replace raw pointer arithmetic with proper packet reader class

5. **Separate Dialog Logic**: Move dialog rendering/handling to dedicated `PartyDialog` class

6. **Const Correctness**: Mark read-only methods as `const`

7. **Remove Gotos**: Replace with proper `break` statements or early returns
