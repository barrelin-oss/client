# Chat System

## Overview

The legacy Helbreath chat system handles all player communication including normal chat, shouts, whispers, guild/party chat, and system messages. It includes a profanity filter, message history with scrolling, 3D chat bubbles above characters, and whisper blocking functionality.

The system is tightly integrated with the `CGame` class and uses the `CMsg` helper class for message storage. Messages are displayed both as floating bubbles in the game world and in a scrollable chat dialog window.

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` / `Game.cpp` | Chat arrays, input handling, rendering, network handlers |
| `Msg.h` / `Msg.cpp` | `CMsg` class for message storage |
| `GameMonitor.h` / `GameMonitor.cpp` | Bad word filtering system |
| `NetMessages.h` | Chat-related packet type definitions |

---

## Key Data Structures

### CMsg Class

The `CMsg` class wraps individual chat messages with metadata.

```cpp
// Msg.h
class CMsg
{
public:
    void * operator new (size_t size);   // Custom heap allocation
    void operator delete(void * mem);    // Custom heap deallocation

    CMsg(char cType, char * pMsg, DWORD dwTime);
    virtual ~CMsg();

    char   m_cType;       // Message type identifier (1-60)
    char * m_pMsg;        // Dynamically allocated message text
    short  m_sX, m_sY;    // Screen coordinates for 3D positioning
    DWORD  m_dwTime;      // Timestamp OR color code (dual purpose)
    int    m_iObjectID;   // Object ID of message sender (-1 if none)
};
```

**Constructor Implementation:**
```cpp
CMsg::CMsg(char cType, char * pMsg, DWORD dwTime)
{
    m_cType = cType;
    m_pMsg = NULL;
    m_pMsg = new char[strlen(pMsg) + 1];
    ZeroMemory(m_pMsg, strlen(pMsg) + 1);
    strcpy(m_pMsg, pMsg);  // No bounds checking - potential overflow
    m_dwTime = dwTime;
    m_iObjectID = -1;
}
```

### Message Storage Arrays in CGame

```cpp
// Game.h - Message list members
class CMsg * m_pChatMsgList[DEF_MAXCHATMSGS];        // Master storage (500)
class CMsg * m_pChatScrollList[DEF_MAXCHATSCROLLMSGS]; // Scroll buffer (80)
class CMsg * m_pWhisperMsg[DEF_MAXWHISPERMSG];       // Whisper senders (5)
class CMsg * m_pExID;                                 // Blocked whisper sender
```

### Chat Input Buffers

```cpp
// Game.h - Input buffers
char m_cChatMsg[64];        // Active chat input buffer
char m_cBackupChatMsg[64];  // Backup for restoration
char m_cTopMsg[64];         // Temporary floating message
```

### Whisper Control Flags

```cpp
// Game.h - Whisper/shout toggles
BOOL m_bWhisper;            // Whisper receive enabled
BOOL m_bShout;              // Shout receive enabled
char m_cWhisperIndex;       // Current whisper recipient index
```

### Event History

```cpp
// Game.h - Event/notification storage
struct {
    char cColor;
    char cStr[64];
} m_stEventHistory[6];      // Recent events (normal)

struct {
    char cColor;
    char cStr[64];
} m_stEventHistory2[6];     // System events (color 10)
```

---

## Constants & Limits

```cpp
// GlobalDef.h
#define DEF_MAXCHATMSGS         500   // Total chat message capacity
#define DEF_MAXCHATSCROLLMSGS   80    // Scrollable chat window buffer
#define DEF_MAXWHISPERMSG       5     // Max stored whisper senders

// Chat timeouts (milliseconds)
#define DEF_CHATTIMEOUT_A       4000  // Normal messages (types 1-20)
#define DEF_CHATTIMEOUT_B       500   // Special messages (types 21-40)
#define DEF_CHATTIMEOUT_C       2000  // Critical messages (types 41-60)

// Bad word filter
#define DEF_MAXBADWORD          500   // Maximum filtered words

// Buffer sizes
#define CHAT_INPUT_MAX          64    // m_cChatMsg buffer size
#define WHISPER_NAME_MAX        10    // Max characters in whisper target name
#define CHAT_DISPLAY_LINES      8     // Visible lines in chat dialog
#define CHAT_BUBBLE_WIDTH       305   // Pixel width before text wrap
#define CHAT_BUBBLE_MAX_LINES   3     // Max lines in 3D chat bubble
#define CHAT_LINE_CHARS         20    // Characters per bubble line
```

---

## Message Types

### Type Ranges and Timeouts

| Type Range | Timeout | Purpose |
|------------|---------|---------|
| 1-19 | 4000ms | Normal chat, shouts |
| 20 | 4000ms + 650ms delay | Whisper messages (yellow, delayed display) |
| 21-40 | 500ms | Special/quick messages |
| 41-42 | 2000ms + 650ms delay | Player-to-player system messages (red) |
| 43-60 | 2000ms | Critical/error messages |

### Specific Type Values

```cpp
// From ChatMsgHandler analysis
cMsgType == 0   // Normal chat (IME enabled)
cMsgType == 2   // Shout (IME enabled, special handling)
cMsgType == 3   // Shout variant (IME enabled)
cMsgType == 20  // Whisper (yellow color, delayed)
cMsgType == 41  // System notification (red, delayed)
cMsgType == 42  // System notification (red, delayed)
```

### Chat Dialog Color Codes

The `m_dwTime` field is repurposed as a color code in `DrawDialogBox_Chat`:

| dwTime | RGB Color | Purpose |
|--------|-----------|---------|
| 0 | (230, 230, 230) | Normal chat - light gray/white |
| 1 | (130, 200, 130) | Guild/Party - light green |
| 2 | (255, 130, 130) | Whisper - light red/pink |
| 3 | (130, 130, 255) | System - light blue |
| 4 | (230, 230, 130) | Shout - yellow |
| 10 | (180, 255, 180) | System notification - bright green |
| 20 | (150, 150, 170) | GM/Admin - purplish gray |

---

## Core Functions

### Message Input and Processing

#### ChatMsgHandler
```cpp
// Game.cpp:15678 - Processes incoming chat packets from server
void CGame::ChatMsgHandler(char * pData)
```

**Packet Structure:**
```
Offset 0-3:   Message ID (DWORD)
Offset 4-5:   Object ID (WORD) - sender's entity ID
Offset 6-7:   Sender X coordinate (short)
Offset 8-9:   Sender Y coordinate (short)
Offset 10-19: Sender name (10 bytes, null-padded)
Offset 20:    Message type (char)
Offset 21+:   Message text (null-terminated string)
```

**Processing Logic:**
1. Extract sender info and message type
2. Check whisper/shout toggle settings
3. Check blocked sender list (`m_pExID`)
4. Apply text wrapping for display (305px width)
5. Handle multi-byte characters for Asian languages
6. Add to appropriate message lists

#### bCheckLocalChatCommand
```cpp
// Game.cpp:34000 - Parses local chat commands
BOOL CGame::bCheckLocalChatCommand(char * pMsg)
```

**Supported Commands:**
| Command | Action |
|---------|--------|
| `/whon` | Enable whisper receiving |
| `/whoff` | Disable whisper receiving |
| `/shon` | Enable shout receiving |
| `/shoff` | Disable shout receiving |
| `/toon <name>` | Unblock player from whisper block |
| `/tooff <name>` | Block player from sending whispers |

#### PutChatScrollList
```cpp
// Game.cpp:15662 - Adds message to scrollable chat window
void CGame::PutChatScrollList(char * pMsg, char cType)
```

Shifts existing messages down and inserts new message at top of scroll list.

#### AddEventList
```cpp
// Game.cpp - Adds event notification to history
void CGame::AddEventList(char * pTxt, char cColor, BOOL bDupAllow)
```

- Stores last 6 events in `m_stEventHistory[]`
- If `cColor == 10`, stores in `m_stEventHistory2[]` instead
- `bDupAllow` controls duplicate message filtering

### Message Timeout and Cleanup

#### ReleaseTimeoverChatMsg
```cpp
// Game.cpp:15814 - Removes expired messages based on type timeout
void CGame::ReleaseTimeoverChatMsg()
```

**Timeout Logic:**
```cpp
for (i = 0; i < DEF_MAXCHATMSGS; i++) {
    if (m_pChatMsgList[i] != NULL) {
        if (m_pChatMsgList[i]->m_cType < 20) {
            // Types 1-19: 4 second timeout
            if (dwTime - m_pChatMsgList[i]->m_dwTime > DEF_CHATTIMEOUT_A) {
                delete m_pChatMsgList[i];
                m_pChatMsgList[i] = NULL;
            }
        }
        else if (m_pChatMsgList[i]->m_cType < 40) {
            // Types 20-39: 500ms timeout
            if (dwTime - m_pChatMsgList[i]->m_dwTime > DEF_CHATTIMEOUT_B) {
                delete m_pChatMsgList[i];
                m_pChatMsgList[i] = NULL;
            }
        }
        else {
            // Types 40+: 2 second timeout
            if (dwTime - m_pChatMsgList[i]->m_dwTime > DEF_CHATTIMEOUT_C) {
                delete m_pChatMsgList[i];
                m_pChatMsgList[i] = NULL;
            }
        }
    }
}
```

### Rendering Functions

#### DrawChatMsgBox
```cpp
// Game.cpp:21324 - Renders 3D chat bubbles above characters
void CGame::DrawChatMsgBox(int iX, int iY, int iIndex)
```

**Features:**
- Splits message into max 3 lines (20 chars each)
- Positions bubble above character sprite
- Color varies by message type:
  - Type 1: White (normal)
  - Type 20: Yellow (whisper), 650ms display delay
  - Type 41-42: Red (system), 650ms display delay

#### DrawDialogBox_Chat
```cpp
// Game.cpp - Renders scrollable chat dialog (Dialog ID 10)
void CGame::DrawDialogBox_Chat()
```

**Features:**
- Displays 8 lines of scrollable history
- Scroll bar with proportional thumb
- Color-coded messages based on `m_dwTime` field
- Click-to-scroll interaction

**Scroll Bounds:**
```cpp
if (m_stDialogBoxInfo[10].sView < 0)
    m_stDialogBoxInfo[10].sView = 0;
if (m_stDialogBoxInfo[10].sView > DEF_MAXCHATSCROLLMSGS - 8)
    m_stDialogBoxInfo[10].sView = DEF_MAXCHATSCROLLMSGS - 8;
```

---

## Bad Word Filtering

### CGameMonitor Class

```cpp
// GameMonitor.h
class CGameMonitor
{
public:
    CGameMonitor();
    virtual ~CGameMonitor();

    int iReadBadWordFileList(char * pFn);  // Load filter from file
    BOOL bCheckBadWord(char * pWord);       // Check single word

    class CMsg * m_pWordList[DEF_MAXBADWORD];  // 500 bad words
};
```

### Loading Bad Words

```cpp
// GameMonitor.cpp
int CGameMonitor::iReadBadWordFileList(char * pFn)
{
    // Opens file and reads line by line
    // Tokenizes with separators: "/,\t\n"
    // Stores each word in m_pWordList[] as CMsg object
    // Returns count of loaded words
}
```

### Checking Bad Words

```cpp
// GameMonitor.cpp
BOOL CGameMonitor::bCheckBadWord(char * pWord)
{
    for (int i = 0; i < DEF_MAXBADWORD; i++) {
        if (m_pWordList[i] != NULL) {
            // Prefix match comparison
            // Handles multi-byte characters (byte >= 128)
            if (match_found) return TRUE;
        }
    }
    return FALSE;
}
```

### Integration in CGame

```cpp
// Game.cpp:23841
BOOL CGame::_bCheckBadWords(char * pMsg)
{
    char cStr[64];
    int i, j;

    // Iterate through message
    for (i = 0; i < strlen(pMsg); i++) {
        // Skip multi-byte character sequences
        if ((unsigned char)cStr[i] >= 128) {
            i++;
            continue;
        }
        // Check substring against filter
        if (m_pGameMonitor->bCheckBadWord(&pMsg[i])) {
            return TRUE;  // Bad word found
        }
    }
    return FALSE;
}
```

**Usage Points:**
- Chat message input validation
- Character name validation during creation
- Whisper target validation

---

## Whisper System

### Whisper Blocking

**Block a Player:**
```cpp
// /tooff command handling
if (memcmp(cBuff, "/tooff", 6) == 0) {
    token = strtok(NULL, "/,\t\n");
    if (token != NULL) {
        char cName[11];
        ZeroMemory(cName, sizeof(cName));
        strncpy(cName, token, 10);  // Max 10 chars

        // Replace existing block
        if (m_pExID != NULL) delete m_pExID;
        m_pExID = new class CMsg(NULL, cName, NULL);

        AddEventList("Player blocked from whispers", 10);
    }
}
```

**Unblock a Player:**
```cpp
// /toon command handling
if (memcmp(cBuff, "/toon", 5) == 0) {
    if (m_pExID != NULL) {
        delete m_pExID;
        m_pExID = NULL;
    }
    AddEventList("Whisper block removed", 10);
}
```

### Whisper History

Recent whisper senders are stored for quick reply:

```cpp
class CMsg * m_pWhisperMsg[DEF_MAXWHISPERMSG];  // 5 slots
```

Used with arrow keys to cycle through recent whisper partners for `/to <name>` command auto-completion.

### Toggle Commands

```cpp
// Enable whispers
if (memcmp(cBuff, "/whon", 5) == 0) {
    m_bWhisper = TRUE;
    AddEventList(NOTIFYMSG_WHISPER_ON, 10);  // "Whisper mode activated"
}

// Disable whispers
if (memcmp(cBuff, "/whoff", 6) == 0) {
    m_bWhisper = FALSE;
    AddEventList(NOTIFYMSG_WHISPER_OFF, 10);  // "Whisper mode deactivated"
}

// Enable shouts
if (memcmp(cBuff, "/shon", 5) == 0) {
    m_bShout = TRUE;
    AddEventList(NOTIFYMSG_SHOUT_ON, 10);  // "Shout mode activated"
}

// Disable shouts
if (memcmp(cBuff, "/shoff", 6) == 0) {
    m_bShout = FALSE;
    AddEventList(NOTIFYMSG_SHOUT_OFF, 10);  // "Shout mode deactivated"
}
```

---

## Integration Points

### Network Layer

**Receiving Chat:**
```cpp
void CGame::GameRecvMsgHandler(DWORD dwMsgSize, char * pData)
{
    // Route to ChatMsgHandler based on packet type
    switch (msgId) {
        case CYCLIC_CYCLINGMSG:
        case CYCLIC_CYCLINGMSG2:
            ChatMsgHandler(pData);
            break;
    }
}
```

**Sending Chat:**
```cpp
// Chat is sent via motion/action packets
// Message included in player action data to server
```

### Input System

- Chat input captured in main input loop
- IME support for Asian language input (types 0, 2, 3)
- Enter key submits message
- Escape key cancels input

### UI System

- Dialog ID 10 is the chat dialog
- `m_stDialogBoxInfo[10].sView` controls scroll position
- Mouse wheel and click-drag for scrolling

### Entity System

- `m_iObjectID` in CMsg links message to entity for 3D bubble positioning
- Bubble position calculated from entity screen coordinates

---

## State Management

### Message Lifecycle

1. **Receive** - `ChatMsgHandler` parses incoming packet
2. **Filter** - Check whisper/shout toggles, blocked users, bad words
3. **Store** - Add to `m_pChatMsgList[]` and `m_pChatScrollList[]`
4. **Display** - Render in 3D world and/or chat dialog
5. **Expire** - `ReleaseTimeoverChatMsg` removes based on type timeout
6. **Cleanup** - Destructor frees allocated message text

### Scroll State

```cpp
// Dialog box info structure
m_stDialogBoxInfo[10].sView  // Current scroll position (0 to 72)
// Max scroll = DEF_MAXCHATSCROLLMSGS - CHAT_DISPLAY_LINES = 80 - 8 = 72
```

---

## Known Issues / Technical Debt

1. **Buffer Overflow Risk** - `strcpy` used without bounds checking in CMsg constructor
2. **Memory Management** - Manual new/delete with no smart pointers
3. **Dual-Purpose Fields** - `m_dwTime` used for both timestamp and color code
4. **Magic Numbers** - Hardcoded type ranges (1-20, 21-40, 41-60)
5. **Tight Coupling** - Chat logic spread across CGame methods
6. **No Unicode** - Multi-byte character handling is manual and error-prone
7. **Limited Block List** - Only one player can be blocked at a time (`m_pExID`)
8. **Fixed Buffers** - 64-byte chat input limit is restrictive
9. **No Message Persistence** - Chat history lost on disconnect/restart

---

## Modernization Notes

### Recommended C++20 Improvements

1. **Strong Typing for Message Types**
   ```cpp
   enum class ChatType : uint8_t {
       Normal, Shout, Whisper, Guild, Party, System, GM
   };
   ```

2. **RAII Message Storage**
   ```cpp
   struct ChatMessage {
       ChatType type;
       std::string sender;
       std::string content;
       std::chrono::steady_clock::time_point timestamp;
       sf::Color color;
       std::optional<uint32_t> entityId;
   };
   ```

3. **Container-Based History**
   ```cpp
   std::deque<ChatMessage> m_chatHistory;  // Auto-manages memory
   std::unordered_set<std::string> m_blockedPlayers;  // Multiple blocks
   ```

4. **Event-Driven Architecture**
   ```cpp
   class ChatSystem {
       EventBus& m_events;
       void onMessageReceived(const ChatMessage& msg);
       void publish(ChatEvent event);
   };
   ```

5. **Proper Unicode Support**
   ```cpp
   std::u32string for proper Unicode handling
   // Or use ICU library for full internationalization
   ```

6. **Configuration-Driven**
   ```cpp
   struct ChatConfig {
       size_t maxHistory = 500;
       size_t maxMessageLength = 200;
       std::chrono::milliseconds normalTimeout{4000};
       bool filterProfanity = true;
   };
   ```

### Migration Priority

1. Extract chat logic from CGame into dedicated `ChatSystem` class
2. Replace CMsg with modern `ChatMessage` struct
3. Use `std::deque` for automatic memory management
4. Implement proper Unicode support
5. Add multiple-player block list
6. Create event-based message notification system
