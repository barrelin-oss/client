# Quest System

## Overview

The Helbreath quest system is a simple, single-active-quest system where players receive tasks from NPCs (primarily City Hall Officers) to hunt monsters or perform observation missions. Quests reward experience points, items, and contribution points that affect the player's standing.

The system supports only **one active quest at a time** - a significant limitation compared to modern MMORPGs. Quest state is stored in a single struct within the CGame class, and all quest logic is handled through network message handlers.

---

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` | Quest struct definition (`m_stQuest`), related member variables |
| `Game.cpp` | Quest message handlers, UI rendering, click handling |
| `NetMessages.h` | Quest-related protocol constants |
| `lan_eng.h` | Quest UI text strings (English localization) |

---

## Key Data Structures

### Quest State Struct (Game.h:595-599)

```cpp
struct {
    BOOL bIsQuestCompleted;      // TRUE if quest objectives met, ready to turn in
    short sWho;                   // NPC giver ID (1-7, with 4 = City Hall Officer)
    short sQuestType;             // Quest type (0=None, 1=Hunt, 7=Observation)
    short sContribution;          // Contribution points reward
    short sTargetType;            // Target monster/NPC type ID
    short sTargetCount;           // Number of targets to kill/observe
    short sX, sY;                 // Target location coordinates
    short sRange;                 // Area range in blocks around target
    char cTargetName[22];         // Map name ("NONE" if anywhere)
} m_stQuest;
```

### Related Member Variables (Game.h)

```cpp
int m_iContribution;              // Player's total contribution points (line 223)
BOOL m_bHunter;                   // Hunt mode flag (line 265)
int m_iHP, m_iMP, m_iSP;         // Stats affected by some quest rewards
int m_iEnemyKillCount;           // Kill tracking (line 277-278)
```

---

## Quest Types

| Type | Name | Description |
|------|------|-------------|
| 0 | None | No active quest |
| 1 | Monster Hunt | Kill a specific number of monsters in a location |
| 7 | Observation | Surveillance mission, observe enemy activity |

### Type 1: Monster Hunt Quest

- **Objective:** Kill `sTargetCount` monsters of type `sTargetType`
- **Location:** Specific map (`cTargetName`) or anywhere ("NONE")
- **Target Area:** Coordinates (`sX`, `sY`) with `sRange` block radius
- **Reward:** Contribution points + experience/items

### Type 7: Observation Quest

- **Objective:** Observe enemy activity in target area
- **Location:** Specific coordinates on a map
- **Mechanics:** Likely involves entering an area without being detected
- **Reward:** Contribution points + experience/items

---

## Network Protocol

### Request Messages (Client → Server)

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_COMMONTYPE_QUESTACCEPTED` | `0x0A22` | Accept a quest from NPC |
| `DEF_COMMONTYPE_REQUEST_CANCELQUEST` | `0x0A50` | Cancel active quest |
| `DEF_COMMONTYPE_REQUEST_HUNTMODE` | `0x0A60` | Toggle hunt mode (v2.83+) |

### Notification Messages (Server → Client)

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_NOTIFY_QUESTCONTENTS` | `0x0B66` | Quest details from server |
| `DEF_NOTIFY_QUESTABORTED` | `0x0B67` | Quest was cancelled |
| `DEF_NOTIFY_QUESTCOMPLETED` | `0x0B68` | Quest objectives completed |
| `DEF_NOTIFY_QUESTREWARD` | `0x0B69` | Quest reward notification |

### Configuration Message

| Constant | Value | Purpose |
|----------|-------|---------|
| `MSGID_QUESTCONFIGURATIONCONTENTS` | `0x0FA40001` | Quest config file data |

---

## Packet Formats

### DEF_NOTIFY_QUESTCONTENTS (Server → Client)

Received when player accepts a quest or logs in with active quest.

```
Offset  Size  Field              Description
------  ----  -----              -----------
+0      2     sWho               NPC giver ID
+2      2     sQuestType         Quest type (1 or 7)
+4      2     sContribution      Contribution reward
+6      2     sTargetType        Target monster type
+8      2     sTargetCount       Kill/observe count
+10     2     sX                 Target X coordinate
+12     2     sY                 Target Y coordinate
+14     2     sRange             Area range in blocks
+16     2     bIsQuestCompleted  Completion status
+18     20    cTargetName        Map name (null-terminated)
```

**Total size:** 38 bytes

### DEF_NOTIFY_QUESTREWARD (Server → Client)

Received when quest is turned in for rewards.

```
Offset  Size  Field              Description
------  ----  -----              -----------
+0      2     sWho               NPC giver ID
+2      2     sFlag              1=completed, 0=declined
+4      4     iAmount            Reward amount (exp or item count)
+8      20    cRewardName        Reward name string
+28     4     iContribution      New total contribution
```

**Total size:** 32 bytes

---

## Core Functions

### Message Handlers (Game.cpp)

#### NotifyMsg_QuestContents (line 25283-25332)

Parses quest details packet and populates `m_stQuest` struct.

```cpp
void CGame::NotifyMsg_QuestContents(char* pData)
{
    // Parse packet starting at DEF_INDEX2_MSGTYPE + 2
    short* sp = (short*)(pData + DEF_INDEX2_MSGTYPE + 2);

    m_stQuest.sWho = sp[0];
    m_stQuest.sQuestType = sp[1];
    m_stQuest.sContribution = sp[2];
    m_stQuest.sTargetType = sp[3];
    m_stQuest.sTargetCount = sp[4];
    m_stQuest.sX = sp[5];
    m_stQuest.sY = sp[6];
    m_stQuest.sRange = sp[7];
    m_stQuest.bIsQuestCompleted = sp[8];

    memcpy(m_stQuest.cTargetName, &sp[9], 20);
}
```

#### NotifyMsg_QuestReward (line 44923-45002)

Processes quest completion rewards and displays notification.

```cpp
void CGame::NotifyMsg_QuestReward(char* pData)
{
    short sWho, sFlag;
    int iAmount, iContribution;
    char cRewardName[21];

    // Parse packet...

    if (sFlag == 1) {
        // Quest completed
        if (strcmp(cRewardName, "exp") == 0) {
            // Experience reward
            wsprintf(cTemp, NOTIFYMSG_QUEST_REWARD1, iAmount);
        } else {
            // Item reward
            wsprintf(cTemp, NOTIFYMSG_QUEST_REWARD2, iAmount, cRewardName);
        }
        AddEventList(cTemp, 10);

        // Contribution change
        if (iContribution > m_iContribution) {
            wsprintf(cTemp, NOTIFYMSG_QUEST_REWARD3, iContribution);
        } else {
            wsprintf(cTemp, NOTIFYMSG_QUEST_REWARD4, iContribution);
        }
        AddEventList(cTemp, 10);
        m_iContribution = iContribution;
    }
}
```

### Request Functions (Game.cpp)

#### Accept Quest (line 20500)

```cpp
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_QUESTACCEPTED,
             NULL, NULL, NULL, NULL, NULL);
```

#### Cancel Quest (line 22530)

```cpp
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_CANCELQUEST,
             NULL, NULL, NULL, NULL, NULL);
```

#### Toggle Hunt Mode (line 22545)

```cpp
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_HUNTMODE,
             NULL, NULL, NULL, NULL, NULL);
```

---

## UI Dialog System

### Dialog Box #28: Quest Information

**Configuration (Game.cpp:715-718):**

```cpp
m_stDialogBoxInfo[28].sX = 0;
m_stDialogBoxInfo[28].sY = 0;
m_stDialogBoxInfo[28].sSizeX = 258;
m_stDialogBoxInfo[28].sSizeY = 339;
```

**Rendering Function:** `DrawDialogBox_Quest(int msX, int msY)` (line 40254-40385)

**Click Handler:** `DlgBoxClick_Quest(int msX, int msY)`

#### Dialog Modes

| Mode | Purpose |
|------|---------|
| 1 | Quest information display |
| 2 | Quest cancelled notification |

#### Mode 1 Display Logic

```cpp
switch (m_stQuest.sQuestType) {
    case 0:
        // "You are not on a quest."
        break;
    case 1:
        // Monster hunt quest
        // Shows: quest type, target monster, count, location, contribution
        if (m_stQuest.bIsQuestCompleted)
            // "You accomplished the monster conquering quest."
        else
            // "You are on a monster conquering quest."
        break;
    case 7:
        // Observation quest
        // Shows: objective, target location, contribution
        break;
}
```

### Dialog Box #21: NPC Quest Acceptance

**Configuration (Game.cpp:673-676):**

```cpp
m_stDialogBoxInfo[21].sX = 337;
m_stDialogBoxInfo[21].sY = 57;
m_stDialogBoxInfo[21].sSizeX = 258;
m_stDialogBoxInfo[21].sSizeY = 339;
```

#### Mode 1: Quest Offer

- **Left Button:** Accept quest → sends `DEF_COMMONTYPE_QUESTACCEPTED`
- **Right Button:** Decline quest → closes dialog

#### Mode 2: Quest Reward Display

- Shows reward items/experience
- Scrollable message list via `sView` pointer

---

## Localization Strings (lan_eng.h)

### Quest Status Messages (lines 1165-1178)

```cpp
#define DRAW_DIALOGBOX_QUEST1   "You are not on a quest."
#define DRAW_DIALOGBOX_QUEST2   "You are on a monster conquering quest."
#define DRAW_DIALOGBOX_QUEST3   "You accomplished the monster conquering quest."
#define DRAW_DIALOGBOX_QUEST5   "Client: %s"
```

### Observation Quest (lines 1522-1525)

```cpp
#define DRAW_DIALOGBOX_QUEST26  "You are on an observation quest"
#define DRAW_DIALOGBOX_QUEST27  "You accomplished observation quest."
#define DRAW_DIALOGBOX_QUEST29  "Client: %s"
#define DRAW_DIALOGBOX_QUEST30  "Objective: Observe the enemy"
#define DRAW_DIALOGBOX_QUEST31  "Location : Anywhere"
#define DRAW_DIALOGBOX_QUEST32  "Map : %s"
#define DRAW_DIALOGBOX_QUEST33  "Position: %d, %d Range: %d block"
#define DRAW_DIALOGBOX_QUEST34  "Contribution: %dPoint"
#define DRAW_DIALOGBOX_QUEST35  "Your quest has been cancelled."
```

### Reward Messages

```cpp
#define NOTIFYMSG_QUEST_REWARD1 "Prize: Experience %d points"
#define NOTIFYMSG_QUEST_REWARD2 "Prize: %d %s"
#define NOTIFYMSG_QUEST_REWARD3 "Contribution ascent to %d points."
#define NOTIFYMSG_QUEST_REWARD4 "Contribution descent to %d points."
```

---

## Quest Flow

### Accepting a Quest

```
1. Player talks to NPC (City Hall Officer)
2. NPC dialog opens (Dialog #21, Mode 1)
3. Player clicks "Accept"
4. Client sends DEF_COMMONTYPE_QUESTACCEPTED
5. Server responds with DEF_NOTIFY_QUESTCONTENTS
6. Client populates m_stQuest struct
7. Quest log available in Dialog #28
```

### Completing a Quest

```
1. Player kills required monsters / completes objective
2. Server tracks progress (server-side)
3. When complete, server sends DEF_NOTIFY_QUESTCOMPLETED
4. m_stQuest.bIsQuestCompleted = TRUE
5. Player returns to NPC
6. Player turns in quest
7. Server sends DEF_NOTIFY_QUESTREWARD
8. Client displays reward notification
9. m_stQuest cleared for next quest
```

### Cancelling a Quest

```
1. Player opens system menu or quest dialog
2. Confirmation dialog (Dialog #13, Mode 8)
3. Player confirms cancellation
4. Client sends DEF_COMMONTYPE_REQUEST_CANCELQUEST
5. Server responds with DEF_NOTIFY_QUESTABORTED
6. Client clears m_stQuest struct
7. Dialog #28 shows "Your quest has been cancelled."
```

---

## Integration Points

### NPC System

- Quest givers identified by `sWho` field (1-7)
- NPC ID 4 = City Hall Officer (primary quest giver)
- Target monster names resolved via `GetNpcName(m_stQuest.sTargetType, cTemp)`
- Map names resolved via `GetOfficialMapName()`

### Contribution System

- Quests award contribution points (`sContribution`)
- Affects player standing and rewards
- Tracked in `m_iContribution` variable
- Can increase or decrease based on quest outcome

### Combat System

- Monster kills tracked server-side
- `m_iEnemyKillCount` may be used for local display
- Kill events sent to server for quest progress

### Dialog System

- Dialog #21: NPC interaction and quest acceptance
- Dialog #28: Quest log and status display
- Dialog #13 Mode 8: Cancel confirmation

---

## Constants & Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| Max Active Quests | 1 | Only one quest at a time |
| Target Name Length | 22 chars | Map name buffer |
| Reward Name Length | 20 chars | Item/reward name buffer |
| Quest Types | 2 known | Hunt (1), Observation (7) |
| NPC Giver IDs | 1-7 | Different quest givers |
| Coordinate Range | short | 0-32767 tiles |

---

## Known Issues / Technical Debt

### Single Quest Limitation

The entire quest system is built around a single `m_stQuest` struct, making it impossible to:
- Track multiple active quests
- Have a quest journal with history
- Queue quests

### Hardcoded Quest Types

Only types 1 and 7 are implemented in the client. Adding new quest types requires:
- Client code changes
- Localization string additions
- Dialog rendering updates

### No Quest Progress Display

The client receives no progress updates during the quest. Players must:
- Manually track kill counts
- Return to NPC to check completion status

### Limited Quest Content

- No quest descriptions or lore
- No intermediate objectives
- No branching or choices
- Simple kill-count or location-based only

### NPC ID Magic Numbers

Quest giver IDs (1-7) are hardcoded with no enum or documentation of what each ID represents beyond ID 4 (City Hall Officer).

---

## Modernization Notes

### Recommended Improvements

1. **Multi-Quest Support**
   - Replace single struct with `std::vector<Quest>`
   - Add quest log with history
   - Support quest categories (main, side, daily)

2. **Quest Progress Tracking**
   - Real-time progress updates from server
   - Progress bar in quest UI
   - Objective checklist

3. **Rich Quest Content**
   - Quest descriptions and lore
   - Multiple objectives per quest
   - Branching dialogue and choices
   - Quest chains

4. **Type-Safe Quest Types**
   ```cpp
   enum class QuestType : uint16_t {
       None = 0,
       MonsterHunt = 1,
       Observation = 7
   };
   ```

5. **Quest Giver Enum**
   ```cpp
   enum class QuestGiver : uint16_t {
       Unknown = 0,
       // ... define 1-7
       CityHallOfficer = 4
   };
   ```

6. **Event-Driven Architecture**
   - Quest accepted event
   - Progress updated event
   - Quest completed event
   - Quest reward event

### Protocol Compatibility

When modernizing, maintain exact packet formats for server compatibility:
- Keep message IDs unchanged
- Preserve field order and sizes
- Handle endianness consistently
