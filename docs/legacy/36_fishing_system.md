# Fishing System

## Overview

The fishing system is a minigame within Helbreath that allows players to catch fish from designated fishing spots on maps. When a player with the Fishing skill approaches a fishing location, a fish appears and the player has a chance to catch it based on their skill level. The system uses dialog box #24 for the UI and involves server coordination for success/failure determination.

Fishing is one of the "gathering" skills in the game, along with Mining. It provides both a gameplay activity and an economic source of items/gold.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | Main fishing logic, dialog rendering, click handling, network message processing |
| `Game.h` | Function declarations, dialog box structure |
| `MapData.cpp` | Dynamic fish object spawning, movement, and animation |
| `DynamicObjectID.h` | Fish dynamic object type definitions |
| `NetMessages.h` | Fishing-related network protocol definitions |
| `lan_*.h` | Localized fishing strings (5 language files) |

## Key Data Structures

### Dynamic Object Types

```cpp
// DynamicObjectID.h
#define DEF_DYNAMICOBJECT_FISH          2   // Active fish that can be caught
#define DEF_DYNAMICOBJECT_FISHOBJECT    3   // Fishing spot indicator (bubbles)
```

### Dialog Box #24 (Fishing Dialog)

The fishing dialog uses the standard `m_stDialogBoxInfo[24]` structure:

```cpp
struct {
    int   sV1;      // Fish catch probability (0-100%)
    int   sV2;      // Fish value in gold
    int   sV3;      // Sprite ID for fish item
    int   sV4;      // Sprite frame for fish item
    // ... other fields
    short sX, sY;   // Dialog position
    char  cStr[32]; // Fish/item name
    char  cMode;    // Dialog mode (always 0 for fishing)
} m_stDialogBoxInfo[24];
```

### Network Protocol Constants

```cpp
// NetMessages.h - Request
#define DEF_COMMONTYPE_REQ_GETFISHTHISTIME    0x0A17  // Attempt to catch fish

// NetMessages.h - Notifications (Server -> Client)
#define DEF_NOTIFY_EVENTFISHMODE              0x0B47  // Fish appeared, show dialog
#define DEF_NOTIFY_FISHCHANCE                 0x0B48  // Update catch probability
#define DEF_NOTIFY_FISHSUCCESS                0x0B4A  // Catch successful
#define DEF_NOTIFY_FISHFAIL                   0x0B4B  // Catch failed
#define DEF_NOTIFY_FISHCANCELED               0x0B4C  // Fishing interrupted
```

## Core Functions

### Dialog Rendering

```cpp
// Game.cpp:38861
void CGame::DrawDialogBox_Fishing(short msX, short msY)
```

Renders the fishing dialog which displays:
- Fish sprite (item preview)
- Fish name
- Value in gold (`sV2`)
- Catch probability percentage (`sV1`)
- "Try Now!" button with hover highlight

**Dialog Layout:**
- Position: Variable (set by server notification)
- Size: Uses `DEF_SPRID_INTERFACE_ND_GAME1` sprite #2
- Fish sprite at: `(sX + 18 + 35, sY + 18 + 17)`
- Item name at: `(sX + 98, sY + 14)`
- Value text at: `(sX + 98, sY + 28)`
- Probability label at: `(sX + 97, sY + 43)`
- Probability value at: `(sX + 157, sY + 40)`
- "Try Now!" button at: `(sX + 160, sY + 70)` to `(sX + 253, sY + 90)`

### Click Handler

```cpp
// Game.cpp:42433
void CGame::DlgBoxClick_Fish(short msX, short msY)
```

Handles player clicking the "Try Now!" button:
1. Checks if click is within button bounds
2. Sends `DEF_COMMONTYPE_REQ_GETFISHTHISTIME` command to server
3. Displays "Attempting to Fish...." message
4. Closes the fishing dialog
5. Plays sound effect 'E' #14 (UI click)

### Network Message Handlers

#### Fish Appeared (Event Fish Mode)

```cpp
// Game.cpp:43459
void CGame::NotifyMsg_EventFishMode(char * pData)
```

**Packet Format:**
| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0 | 2 | wPrice | Fish value in gold |
| +2 | 2 | sSprite | Item sprite ID |
| +4 | 2 | sSpriteFrame | Item sprite frame |
| +6 | 20 | cName | Fish/item name (null-terminated) |

**Actions:**
1. Parses packet data
2. Opens dialog box #24 with fish info
3. Sets sprite and frame for preview
4. Displays "Started to fish..." event message

#### Catch Probability Update

```cpp
// Game.cpp:25082
void CGame::NotifyMsg_FishChance(char * pData)
```

**Packet Format:**
| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0 | 2 | iFishChance | Catch probability (0-100) |

Updates `m_stDialogBoxInfo[24].sV1` with the current catch probability.

#### Fishing Result Handlers

```cpp
// Game.cpp:31100-31148 (in NotifyMsgHandler switch)

case DEF_NOTIFY_FISHCANCELED:
    // Reason codes:
    // 0 = Player moved or took damage
    // 1 = Another player caught the fish first
    // 2 = Fish swam away (timeout)

case DEF_NOTIFY_FISHSUCCESS:
    // Plays success sounds, character celebration

case DEF_NOTIFY_FISHFAIL:
    // Plays failure sound
```

**Fish Canceled Reason Codes:**

| Code | Meaning | Message |
|------|---------|---------|
| 0 | Player disturbed | "You can't fish anymore because you were disturbed!" |
| 1 | Stolen by other player | "The other player took first the fish. What a pity it is!" |
| 2 | Fish escaped | "You can't fish anymore. The fish left!" |

**Success Sound Effects:**
- Sound 'E' #23 (splash/water)
- Sound 'E' #17 (item pickup)
- Sound 'C' #21 or #22 (character voice, based on player type 1-3 or 4-6)

**Failure Sound Effect:**
- Sound 'E' #24 (failure/miss)

## Fish Dynamic Object Behavior

### Spawning (MapData.cpp)

```cpp
// Game.cpp:3627
case DEF_DYNAMICOBJECT_FISH:
    m_pData[dX][dY].m_cDynamicObjectData1 = (rand() % 40) - 20;  // X offset (-20 to +19)
    m_pData[dX][dY].m_cDynamicObjectData2 = (rand() % 40) - 20;  // Y offset (-20 to +19)
    m_pData[dX][dY].m_cDynamicObjectData3 = (rand() % 10) - 5;   // X velocity (-5 to +4)
    m_pData[dX][dY].m_cDynamicObjectData4 = (rand() % 10) - 5;   // Y velocity (-5 to +4)
```

Fish spawn at a random offset from the tile center with initial random velocity.

### Movement Animation (MapData.cpp)

```cpp
// MapData.cpp:1812-1858
case DEF_DYNAMICOBJECT_FISH:
    // Update every 100ms
    if ((dwTime - m_pData[dX][dY].m_dwDynamicObjectTime) < 100) break;

    // Random bubble effect (1/15 chance per tick)
    if ((rand() % 15) == 1)
        m_pGame->bAddNewEffect(13, ...);  // Effect #13 = water bubble

    // Calculate direction toward center (0,0)
    cDir = m_pGame->m_Misc.cGetNextMoveDir(Data1, Data2, 0, 0);

    // Apply velocity based on direction (8 directions)
    // Velocity clamped to range [-12, +12]

    // Update position
    m_pData[dX][dY].m_cDynamicObjectData1 += Data3;  // Position += Velocity
    m_pData[dX][dY].m_cDynamicObjectData2 += Data4;
```

The fish swims in a constrained area, always trending back toward the center of its tile. This creates a natural "swimming in a pond" effect.

### Rendering (Game.cpp)

```cpp
// Game.cpp:2742
case DEF_DYNAMICOBJECT_FISH:
    // Calculate facing direction based on movement
    cTmpDOdir = m_Misc.cCalcDirection(Data1, Data2, Data1 + Data3, Data2 + Data4);

    // Select frame: 8 directions * 4 animation frames
    cTmpDOframe = ((cTmpDOdir - 1) * 4) + (rand() % 4);

    // Draw with transparency
    m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 0]->PutTransSprite2(
        ix + Data1, iy + Data2, cTmpDOframe, dwTime);
```

Fish are rendered using sprite `DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 0` with 32 frames (8 directions x 4 animation frames). The random frame selection creates a natural fin-flapping animation.

### Fishing Spot Indicator (Bubbles)

```cpp
// MapData.cpp:1807
case DEF_DYNAMICOBJECT_FISHOBJECT:
    // 1/12 chance per tick to spawn bubble effect
    if ((rand() % 12) == 1)
        m_pGame->bAddNewEffect(13, ...);  // Water bubble effect
```

The fishing spot itself shows occasional bubbles to indicate where fish may appear.

## Constants & Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| Dialog Box ID | 24 | Fishing dialog identifier |
| Fish sprite frames | 32 | 8 directions x 4 animation frames |
| Movement tick rate | 100ms | Fish position update frequency |
| Velocity range | -12 to +12 | Maximum fish movement speed |
| Spawn offset range | -20 to +19 | Initial position randomization |
| Bubble chance (fish) | 1/15 | Probability of bubble effect per tick |
| Bubble chance (spot) | 1/12 | Probability of bubble at fishing spot |
| Effect ID (bubbles) | 13 | Water bubble visual effect |

## Integration Points

### Skill System
- Fishing is skill ID #1 (index 0 in some systems, 1 in others)
- Skill level affects catch probability (server-side calculation)
- The Fishing skill must be trained at skill trainers

### Dialog System
- Uses dialog box #24 in the 41-dialog array
- Sets `m_bSkillUsingStatus = TRUE` while dialog is open
- Dialog is disabled when fishing concludes or is interrupted

### Dynamic Object System
- Fish are `DEF_DYNAMICOBJECT_FISH` (type 2)
- Fishing spots are `DEF_DYNAMICOBJECT_FISHOBJECT` (type 3)
- Both use the dynamic object data fields for position/velocity

### Effect System
- Effect #13 (water bubbles) used for visual feedback
- Spawned randomly during fish movement and at fishing spots

### Sound System
- 'E' category sounds for UI and fishing events
- 'C' category sounds for character voice reactions

### Network System
- `MSGID_COMMAND_COMMON` with `DEF_COMMONTYPE_REQ_GETFISHTHISTIME` for catch attempt
- Five notification types for fishing state updates

## State Management

### Fishing Flow

```
1. Player approaches fishing spot with Fishing skill
2. Server detects opportunity and sends DEF_NOTIFY_EVENTFISHMODE
3. Client:
   - Opens dialog #24 with fish info
   - Sets m_bSkillUsingStatus = TRUE
   - Displays "Started to fish..." message

4. Server periodically sends DEF_NOTIFY_FISHCHANCE
5. Client updates displayed probability

6. Player clicks "Try Now!" button
7. Client:
   - Sends DEF_COMMONTYPE_REQ_GETFISHTHISTIME
   - Closes dialog
   - Displays "Attempting to Fish...."

8. Server determines outcome and sends one of:
   - DEF_NOTIFY_FISHSUCCESS → Item added to inventory
   - DEF_NOTIFY_FISHFAIL → Nothing happens
   - DEF_NOTIFY_FISHCANCELED → With reason code

9. Client:
   - Sets m_bSkillUsingStatus = FALSE
   - Plays appropriate sound effects
   - Displays result message
```

### State Flags

| Flag | Purpose |
|------|---------|
| `m_bIsDialogEnabled[24]` | Whether fishing dialog is open |
| `m_bSkillUsingStatus` | TRUE while fishing (prevents other actions) |
| `m_stDialogBoxInfo[24].cMode` | Always 0 for fishing dialog |

## Localized Strings

### English (lan_eng.h)

```cpp
#define NOTIFY_MSG_HANDLER52    "You can't fish anymore because you were disturbed!"
#define NOTIFY_MSG_HANDLER53    "The other player took first the fish. What a pity it is!"
#define NOTIFY_MSG_HANDLER54    "You can't fish anymore. The fish left!"
#define NOTIFY_MSG_HANDLER55    "You were successful on fishing!!! "
#define NOTIFY_MSG_HANDLER56    "You failed to fish..."
#define DRAW_DIALOGBOX_FISHING1 "Value: %d Gold"
#define DRAW_DIALOGBOX_FISHING2 "Probability:"
#define DLGBOX_CLICK_FISH1      "Attempting to Fish...."
#define NOTIFYMSG_EVENTFISHMODE1 "Started to fish..."
```

### Korean (lan_kor.h)

```cpp
#define DRAW_DIALOGBOX_FISHING1 "????ġ: %dGold"       // Value: %dGold
#define DRAW_DIALOGBOX_FISHING2 "???? Ȯ??:"          // Catch probability:
#define DLGBOX_CLICK_FISH1      "????ø? ?õ??մϴ?!..." // Attempting to fish!...
#define NOTIFYMSG_EVENTFISHMODE1 "???ð? ???۵Ǿ????ϴ?..." // Fishing has started...
```

## Known Issues / Technical Debt

### Hardcoded Values
- Dialog button coordinates are hardcoded pixel positions
- Sound effect IDs are magic numbers (14, 17, 23, 24)
- Effect ID 13 for bubbles is a magic number
- Movement tick rate (100ms) is hardcoded

### Race Conditions
- Multiple players can attempt to catch the same fish
- Server arbitrates but client shows dialog until result arrives
- "Another player took the fish" scenario handles this

### Missing Features
- No visual indicator of fishing skill level in dialog
- No fishing pole/bait item requirement visible in client (server-side only)
- No fishing statistics or history tracking

### Code Duplication
- Success sound logic duplicated for male/female player types
- Similar dialog patterns repeated across gathering skills

### Language-Specific Rendering
- Japanese version has different button text rendering (`#if DEF_LANGUAGE == 1`)
- Uses different font/encoding for the "Try Now!" button

## Modernization Notes

### Recommended Improvements

1. **Extract FishingSystem class**
   - Move all fishing logic from CGame to dedicated class
   - Encapsulate state (dialog open, probability, fish info)

2. **Use strong typing**
   ```cpp
   enum class FishingResult { Success, Fail, Canceled };
   enum class CancelReason { Disturbed, Stolen, Escaped };
   ```

3. **Event-based communication**
   - Publish fishing events to EventBus
   - Separate UI updates from game logic

4. **Data-driven fish definitions**
   ```yaml
   fishing_spots:
     - type: common_fish
       value_range: [10, 50]
       base_probability: 0.3
       sprite: "fish_common"
   ```

5. **Replace magic numbers with constants**
   ```cpp
   namespace fishing {
       constexpr int DIALOG_ID = 24;
       constexpr int BUBBLE_EFFECT_ID = 13;
       constexpr Duration UPDATE_TICK = 100ms;
   }
   ```

6. **Unified localization**
   - Move strings to JSON/YAML files
   - Support runtime language switching

7. **Async/await pattern for network**
   ```cpp
   auto result = co_await fishingSystem.attemptCatch();
   if (result == FishingResult::Success) {
       // Handle success
   }
   ```
