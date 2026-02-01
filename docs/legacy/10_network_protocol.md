# Legacy Network Protocol Documentation

## Table of Contents

1. [Overview](#overview)
2. [Socket Layer (XSocket)](#socket-layer-xsocket)
3. [Packet Format](#packet-format)
4. [Encryption/Obfuscation](#encryptionobfuscation)
5. [Connection Architecture](#connection-architecture)
6. [Message Categories](#message-categories)
7. [Login Server Protocol](#login-server-protocol)
8. [Game Server Protocol](#game-server-protocol)
9. [Notification Messages](#notification-messages)
10. [Motion and Event Messages](#motion-and-event-messages)
11. [Common Command Messages](#common-command-messages)
12. [Configuration Data Messages](#configuration-data-messages)
13. [Guild and Party Messages](#guild-and-party-messages)
14. [Crusade/War Messages](#crusadewar-messages)
15. [Administrative Messages](#administrative-messages)
16. [Data Type Reference](#data-type-reference)
17. [Connection Flow Diagrams](#connection-flow-diagrams)

---

## Overview

The Helbreath client uses a custom binary protocol over TCP sockets for all server communication. The protocol is built on WinSock2 and uses asynchronous event-driven socket handling through Windows messages.

### Key Characteristics

| Property | Value |
|----------|-------|
| Transport | TCP/IP |
| Socket API | WinSock2 |
| Event Model | WSAAsyncSelect (Windows message-based) |
| Byte Order | Little-endian (x86 native) |
| Packet Encoding | Binary with optional XOR encryption |
| Buffer Size | Configurable, typically 40KB |
| Max Queued Messages | 300 (DEF_XSOCKBLOCKLIMIT) |
| Socket Buffer Size | 40,960 bytes (8192 * 5) |

### Protocol Version

The client protocol version is defined in `GlobalDef.h`:

```cpp
#define DEF_UPPERVERSION    2
#define DEF_LOWERVERSION    20
```

This corresponds to client version **2.20** (English international version).

---

## Socket Layer (XSocket)

### Source Files

- `XSocket.h` - Class declaration
- `XSocket.cpp` - Implementation

### Class Definition

```cpp
class XSocket {
public:
    // Socket types
    // DEF_XSOCK_LISTENSOCK (1) - Server listening socket
    // DEF_XSOCK_NORMALSOCK (2) - Normal client socket
    // DEF_XSOCK_SHUTDOWNEDSOCK (3) - Closed socket

    // Reading states
    // DEF_XSOCKSTATUS_READINGHEADER (11) - Reading 3-byte header
    // DEF_XSOCKSTATUS_READINGBODY (12) - Reading message body

    // Member variables
    char   m_cType;              // Socket type
    char * m_pRcvBuffer;         // Receive buffer
    char * m_pSndBuffer;         // Send buffer
    DWORD  m_dwBufferSize;       // Buffer size
    SOCKET m_Sock;               // WinSock socket handle
    char   m_cStatus;            // Current read state
    DWORD  m_dwReadSize;         // Bytes remaining to read
    DWORD  m_dwTotalReadSize;    // Total bytes read so far
    char   m_pAddr[30];          // Server address
    int    m_iPortNum;           // Server port
    HWND   m_hWnd;               // Window handle for messages
    unsigned int m_uiMsg;        // Windows message ID
    BOOL   m_bIsAvailable;       // Connection established
    BOOL   m_bIsWriteEnabled;    // Ready to send

    // Unsent data queue (circular buffer)
    char * m_pUnsentDataList[DEF_XSOCKBLOCKLIMIT];
    int    m_iUnsentDataSize[DEF_XSOCKBLOCKLIMIT];
    short  m_sHead, m_sTail;
};
```

### Socket Events

The socket generates the following event codes:

| Event Code | Value | Description |
|------------|-------|-------------|
| `DEF_XSOCKEVENT_SOCKETMISMATCH` | -121 | Socket handle mismatch |
| `DEF_XSOCKEVENT_CONNECTIONESTABLISH` | -122 | Connection established successfully |
| `DEF_XSOCKEVENT_RETRYINGCONNECTION` | -123 | Retrying connection after failure |
| `DEF_XSOCKEVENT_ONREAD` | -124 | Currently reading message |
| `DEF_XSOCKEVENT_READCOMPLETE` | -125 | Full message received |
| `DEF_XSOCKEVENT_UNKNOWN` | -126 | Unknown event |
| `DEF_XSOCKEVENT_SOCKETCLOSED` | -127 | Socket was closed |
| `DEF_XSOCKEVENT_BLOCK` | -128 | Send blocked (would block) |
| `DEF_XSOCKEVENT_SOCKETERROR` | -129 | Socket error occurred |
| `DEF_XSOCKEVENT_CRITICALERROR` | -130 | Critical error, terminate |
| `DEF_XSOCKEVENT_NOTINITIALIZED` | -131 | Socket not initialized |
| `DEF_XSOCKEVENT_MSGSIZETOOLARGE` | -132 | Message too large for buffer |
| `DEF_XSOCKEVENT_CONFIRMCODENOTMATCH` | -133 | Confirmation code mismatch |
| `DEF_XSOCKEVENT_QUENEFULL` | -134 | Send queue full |
| `DEF_XSOCKEVENT_UNSENTDATASENDBLOCK` | -135 | Blocked while sending queued data |
| `DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE` | -136 | All queued data sent |

### Key Methods

#### `bConnect(char* pAddr, int iPort, unsigned int uiMsg)`

Initiates an asynchronous connection:

1. Creates TCP socket
2. Registers for FD_CONNECT, FD_READ, FD_WRITE, FD_CLOSE events
3. Initiates non-blocking connect
4. Sets socket buffer sizes to 40,960 bytes

#### `iSendMsg(char* cData, DWORD dwSize, char cKey = NULL)`

Sends a message with the 3-byte header:

1. Validates message size
2. Prepends header (key byte + size word)
3. Optionally encrypts data if `cKey != 0`
4. Queues if socket not ready, otherwise sends immediately

#### `pGetRcvDataPointer(DWORD* pMsgSize, char* pKey = NULL)`

Returns pointer to received message body:

1. Extracts encryption key from byte 0
2. Calculates body size from bytes 1-2
3. Decrypts data if key is non-zero
4. Returns pointer to byte 3 (start of payload)

#### `_iOnRead()`

Handles incoming data in two phases:

**Phase 1: Reading Header (3 bytes)**
- Byte 0: Encryption key
- Bytes 1-2: Total message size (WORD, little-endian)

**Phase 2: Reading Body**
- Reads (size - 3) bytes of payload
- Returns READCOMPLETE when done

---

## Packet Format

### Wire Format

All packets follow this structure:

```
┌─────────────────────────────────────────────────────────┐
│                    PACKET HEADER (3 bytes)              │
├─────────┬───────────────────────────────────────────────┤
│ Key (1) │ Total Size (2)                                │
├─────────┴───────────────────────────────────────────────┤
│                    PACKET BODY (variable)               │
├─────────────────────────────────────────────────────────┤
│ MsgID (4) │ MsgType (2) │ Payload (Size - 9)...         │
└─────────────────────────────────────────────────────────┘
```

### Header Fields

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0 | 1 | char | Encryption key (0 = no encryption) |
| 1 | 2 | WORD | Total packet size including header |

### Body Fields

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0 | 4 | DWORD | Message ID (DEF_INDEX4_MSGID = 0) |
| 4 | 2 | WORD | Message Type/Subtype (DEF_INDEX2_MSGTYPE = 4) |
| 6+ | varies | - | Message-specific payload |

### Index Constants

```cpp
#define DEF_INDEX4_MSGID    0   // Offset of 4-byte message ID
#define DEF_INDEX2_MSGTYPE  4   // Offset of 2-byte message type
```

---

## Encryption/Obfuscation

### XOR Encryption Algorithm

When `cKey != 0`, the payload is encrypted/decrypted using a position-dependent XOR:

```cpp
// Encryption (in iSendMsg)
for (i = 0; i < dwSize; i++) {
    m_pSndBuffer[3+i] += (i ^ cKey);
    m_pSndBuffer[3+i] = (char)(m_pSndBuffer[3+i] ^ (cKey ^ (dwSize - i)));
}

// Decryption (in pGetRcvDataPointer)
for (i = 0; i < dwSize; i++) {
    m_pRcvBuffer[3+i] = (char)(m_pRcvBuffer[3+i] ^ (cKey ^ (dwSize - i)));
    m_pRcvBuffer[3+i] -= (i ^ cKey);
}
```

### Key Generation

The encryption key is generated randomly for each outgoing message:

```cpp
cKey = (char)(rand() % 255) + 1;  // Range: 1-255 (0 means no encryption)
```

### Security Notes

- This is a weak obfuscation, not true encryption
- The key is transmitted in plaintext (byte 0 of packet)
- Pattern is easily reversible by any observer
- Primarily prevents casual packet inspection

---

## Connection Architecture

### Dual Socket System

The client maintains two separate socket connections:

| Socket | Variable | Windows Message | Purpose |
|--------|----------|-----------------|---------|
| Login Server | `m_pLSock` | `WM_USER_LOGSOCKETEVENT` (WM_USER + 2001) | Authentication, character management |
| Game Server | `m_pGSock` | `WM_USER_GAMESOCKETEVENT` (WM_USER + 2000) | Gameplay communication |

### Server Types

```cpp
#define DEF_SERVERTYPE_GAME  1
#define DEF_SERVERTYPE_LOG   2
```

### Connection Sequence

```
┌──────────┐                    ┌─────────────┐                    ┌─────────────┐
│  Client  │                    │ Login Server│                    │ Game Server │
└────┬─────┘                    └──────┬──────┘                    └──────┬──────┘
     │                                 │                                  │
     │  1. TCP Connect                 │                                  │
     │────────────────────────────────>│                                  │
     │                                 │                                  │
     │  2. MSGID_REQUEST_LOGIN         │                                  │
     │────────────────────────────────>│                                  │
     │                                 │                                  │
     │  3. MSGID_RESPONSE_LOG          │                                  │
     │<────────────────────────────────│                                  │
     │     (character list)            │                                  │
     │                                 │                                  │
     │  4. MSGID_REQUEST_ENTERGAME     │                                  │
     │────────────────────────────────>│                                  │
     │                                 │                                  │
     │  5. MSGID_RESPONSE_ENTERGAME    │                                  │
     │<────────────────────────────────│                                  │
     │     (game server address)       │                                  │
     │                                 │                                  │
     │  6. TCP Connect                 │                                  │
     │─────────────────────────────────────────────────────────────────────>│
     │                                 │                                  │
     │  7. MSGID_REQUEST_INITPLAYER    │                                  │
     │─────────────────────────────────────────────────────────────────────>│
     │                                 │                                  │
     │  8. MSGID_RESPONSE_INITPLAYER   │                                  │
     │<─────────────────────────────────────────────────────────────────────│
     │                                 │                                  │
     │  9. MSGID_REQUEST_INITDATA      │                                  │
     │─────────────────────────────────────────────────────────────────────>│
     │                                 │                                  │
     │ 10. MSGID_RESPONSE_INITDATA     │                                  │
     │<─────────────────────────────────────────────────────────────────────│
     │     (player position, map data) │                                  │
     │                                 │                                  │
     │ 11. Configuration Messages      │                                  │
     │<─────────────────────────────────────────────────────────────────────│
     │     (items, magic, skills...)   │                                  │
     │                                 │                                  │
     │         [GAME LOOP BEGINS]      │                                  │
     │                                 │                                  │
```

---

## Message Categories

### Message ID Ranges

Messages are identified by 32-bit IDs with the following patterns:

| Pattern | Category | Example |
|---------|----------|---------|
| `0x05xxxxxx` | Init Player/Data | `MSGID_REQUEST_INITPLAYER` |
| `0x0FAxxxxx` | Motion/Events | `MSGID_COMMAND_MOTION` |
| `0x0FCxxxxx` | Login/Account | `MSGID_REQUEST_LOGIN` |
| `0x0DFxxxxx` | Player Data Save | `MSGID_REQUEST_SAVEPLAYERDATA` |
| `0x12Axxxxx` | Dynamic Objects | `MSGID_DYNAMICOBJECT` |
| `0x167xxxxx` | Occupy Flags | `MSGID_OCCUPYFLAGDATA` |

### Confirm/Reject Codes

```cpp
#define DEF_MSGTYPE_CONFIRM  0x0F14
#define DEF_MSGTYPE_REJECT   0x0F15
```

---

## Login Server Protocol

### MSGID_REQUEST_LOGIN (0x0FC94201)

**Direction:** Client → Login Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID (0x0FC94201)
4       2     WORD    MsgType (NULL)
6       10    char[]  Account Name (padded)
16      10    char[]  Password (padded)
26      30    char[]  World Server Name
──────────────────────────────────────────
Total: 56 bytes
```

For long account names (Japanese Terra servers):

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       16    char[]  Account Name (DEF_ACCOUNTLEN)
22      10    char[]  Password
32      30    char[]  World Server Name
──────────────────────────────────────────
Total: 62 bytes
```

### MSGID_RESPONSE_LOG (0x0FC94203)

**Direction:** Login Server → Client

**Response Codes:**

| Code | Constant | Description |
|------|----------|-------------|
| 0x0F14 | `DEF_LOGRESMSGTYPE_CONFIRM` | Login successful |
| 0x0F15 | `DEF_LOGRESMSGTYPE_REJECT` | Login rejected |
| 0x0F16 | `DEF_LOGRESMSGTYPE_PASSWORDMISMATCH` | Wrong password |
| 0x0F17 | `DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT` | Account not found |
| 0x0F18 | `DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED` | Account created |
| 0x0F19 | `DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED` | Account creation failed |
| 0x0F1A | `DEF_LOGRESMSGTYPE_ALREADYEXISTINGACCOUNT` | Account exists |
| 0x0F1B | `DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER` | Character not found |
| 0x0F1C | `DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED` | Character created |
| 0x0F1D | `DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED` | Character creation failed |
| 0x0F1E | `DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER` | Character exists |
| 0x0F1F | `DEF_LOGRESMSGTYPE_CHARACTERDELETED` | Character deleted |
| 0x0F30 | `DEF_LOGRESMSGTYPE_NOTENOUGHPOINT` | Insufficient points |
| 0x0F31 | `DEF_LOGRESMSGTYPE_ACCOUNTLOCKED` | Account locked |
| 0x0F32 | `DEF_LOGRESMSGTYPE_SERVICENOTAVAILABLE` | Service unavailable |
| 0x0A00 | `DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS` | Password changed |
| 0x0A01 | `DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL` | Password change failed |
| 0x0A02 | `DEF_LOGRESMSGTYPE_NOTEXISTINGWORLDSERVER` | World server not found |
| 0x0A03 | `DEF_LOGRESMSGTYPE_INPUTKEYCODE` | Key code required |
| 0x0A04 | `DEF_LOGRESMSGTYPE_REALACCOUNT` | Real money account |
| 0x0A05 | `DEF_LOGRESMSGTYPE_FORCECHANGEPASSWORD` | Must change password |
| 0x0A06 | `DEF_LOGRESMSGTYPE_INVALIDKOREANSSN` | Invalid Korean SSN |
| 0x0A07 | `DEF_LOGRESMSGTYPE_LESSTHENFIFTEEN` | Under 15 years old |

### MSGID_REQUEST_CREATENEWACCOUNT (0x0FC94202)

**Direction:** Client → Login Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       10    char[]  Account Name
16      10    char[]  Password
26      50    char[]  Email Address
76      10    char[]  Gender ("Male"/"Female")
86      10    char[]  Age
96      4     char[]  Country Code ("xxxx")
100     2     char[]  Day ("xx")
102     2     char[]  Month ("xx")
104     17    char[]  Country Name
121     28    char[]  SSN (Social Security Number)
149     45    char[]  Security Quiz
194     20    char[]  Quiz Answer
214     50    char[]  Command Line Token
──────────────────────────────────────────
Total: 264 bytes
```

### MSGID_REQUEST_CREATENEWCHARACTER (0x0FC94204)

**Direction:** Client → Login Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       10    char[]  Player Name
16      10    char[]  Account Name
26      10    char[]  Account Password
36      30    char[]  World Server Name
66      1     char    Gender (1=Male, 2=Female)
67      1     char    Skin Color
68      1     char    Hair Style
69      1     char    Hair Color
70      1     char    Underwear Color
71      1     char    Strength (10-14)
72      1     char    Vitality (10-14)
73      1     char    Dexterity (10-14)
74      1     char    Intelligence (10-14)
75      1     char    Magic (10-14)
76      1     char    Charisma (10-14)
──────────────────────────────────────────
Total: 77 bytes
```

### MSGID_REQUEST_ENTERGAME (0x0FC94205)

**Direction:** Client → Login Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (Enter Type)
6       10    char[]  Player Name
16      10    char[]  Map Name
26      10    char[]  Account Name
36      10    char[]  Account Password
46      4     int     Level
50      30    char[]  World Server Name
80      120   char[]  Command Line Token
──────────────────────────────────────────
Total: 200 bytes
```

**Enter Game Types (MsgType):**

| Value | Constant | Description |
|-------|----------|-------------|
| 0x0F1C | `DEF_ENTERGAMEMSGTYPE_NEW` | New character entering |
| 0x0F1D | `DEF_ENTERGAMEMSGTYPE_NOENTER_FORCEDISCONN` | Force disconnect |
| 0x0F1E | `DEF_ENTERGAMEMSGTYPE_CHANGINGSERVER` | Changing server |
| 0x0F1F | `DEF_ENTERGAMEMSGTYPE_NEW_TOWLSBUTMLS` | World to login transfer |

### MSGID_RESPONSE_ENTERGAME (0x0FC94206)

**Direction:** Login Server → Client

**Response Types:**

| Value | Constant | Description |
|-------|----------|-------------|
| 0x0F20 | `DEF_ENTERGAMERESTYPE_PLAYING` | Already playing |
| 0x0F21 | `DEF_ENTERGAMERESTYPE_REJECT` | Entry rejected |
| 0x0F22 | `DEF_ENTERGAMERESTYPE_CONFIRM` | Entry confirmed |
| 0x0F23 | `DEF_ENTERGAMERESTYPE_FORCEDISCONN` | Forced disconnect |

### MSGID_REQUEST_DELETECHARACTER (0x0FC94207)

**Direction:** Client → Login Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (Character Index)
6       10    char[]  Character Name
16      10    char[]  Account Name
26      10    char[]  Account Password
36      30    char[]  World Server Name
──────────────────────────────────────────
Total: 66 bytes
```

### MSGID_REQUEST_CHANGEPASSWORD (0x0FC94210)

**Direction:** Client → Login Server

**Packet Structure (International):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       10    char[]  Account Name
16      10    char[]  Current Password
26      10    char[]  New Password
36      10    char[]  Confirm New Password
──────────────────────────────────────────
Total: 46 bytes
```

**Packet Structure (Chinese/Taiwan with SSN):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       10    char[]  Account Name
16      10    char[]  Current Password
26      18    char[]  SSN
44      10    char[]  New Password
54      10    char[]  Confirm New Password
──────────────────────────────────────────
Total: 64 bytes
```

---

## Game Server Protocol

### MSGID_REQUEST_INITPLAYER (0x05040205)

**Direction:** Client → Game Server

Sent immediately after connecting to game server.

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       10    char[]  Player Name
16      10    char[]  Account Name
26      10    char[]  Account Password
36      1     char    Observer Mode Flag
37      20    char[]  Game Server Name
──────────────────────────────────────────
Total: 57 bytes
```

### MSGID_RESPONSE_INITPLAYER (0x05040206)

**Direction:** Game Server → Client

**Response Types:**

| MsgType | Description | Action |
|---------|-------------|--------|
| `DEF_MSGTYPE_CONFIRM` (0x0F14) | Player initialized | Request init data |
| `DEF_MSGTYPE_REJECT` (0x0F15) | Initialization failed | Show error |

### MSGID_REQUEST_INITDATA (0x05080404)

**Direction:** Client → Game Server

Requests full player and map initialization data.

**Packet Structure:** Same as MSGID_REQUEST_INITPLAYER

### MSGID_RESPONSE_INITDATA (0x05080405)

**Direction:** Game Server → Client

Contains all initial game state data.

**Packet Structure (Initial Fields):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType
6       2     short   Player Object ID
8       2     short   X Position
10      2     short   Y Position
12      2     short   Player Type
14      2     short   Appearance 1
16      2     short   Appearance 2
18      2     short   Appearance 3
20      2     short   Appearance 4
22      4     int     Appearance Color
26      2     short   Status Flags
...     ...   ...     Additional data follows
```

The response continues with:
- Map file name
- Weather conditions
- Player stats (HP, MP, SP, etc.)
- Equipment data
- Current effects
- And more...

### MSGID_COMMAND_MOTION (0x0FA314D5)

**Direction:** Client → Game Server

General movement and action command.

**Packet Structure (Default):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Action Type
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction (1-8)
11      2     short   Target/Param 1
13      2     short   Target/Param 2
15      2     short   Target/Param 3
17      4     DWORD   Timestamp
──────────────────────────────────────────
Total: 21 bytes
```

**Packet Structure (Attack/AttackMove):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Action Type
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction
11      2     short   Target X
13      2     short   Target Y
15      2     short   Target Object ID
17      2     short   Attack Type
19      4     DWORD   Timestamp
──────────────────────────────────────────
Total: 23 bytes
```

**Action Types:**

| Value | Constant | Description |
|-------|----------|-------------|
| 0 | `DEF_OBJECTSTOP` | Stop/Idle |
| 1 | `DEF_OBJECTMOVE` | Walk |
| 2 | `DEF_OBJECTRUN` | Run |
| 3 | `DEF_OBJECTATTACK` | Attack |
| 4 | `DEF_OBJECTMAGIC` | Cast magic |
| 5 | `DEF_OBJECTGETITEM` | Pick up item |
| 6 | `DEF_OBJECTDAMAGE` | Take damage |
| 7 | `DEF_OBJECTDAMAGEMOVE` | Damage + knockback |
| 8 | `DEF_OBJECTATTACKMOVE` | Attack while moving |
| 10 | `DEF_OBJECTDYING` | Dying animation |
| 100 | `DEF_OBJECTNULLACTION` | No action |
| 101 | `DEF_OBJECTDEAD` | Dead |

### MSGID_RESPONSE_MOTION (0x0FA314D6)

**Direction:** Game Server → Client

Confirms or rejects motion commands.

**Response Types:**

| Value | Constant | Description |
|-------|----------|-------------|
| 1001 | `DEF_OBJECTMOVE_CONFIRM` | Move confirmed + map data |
| 1010 | `DEF_OBJECTMOVE_REJECT` | Move rejected + correction |
| 1020 | `DEF_OBJECTMOTION_CONFIRM` | Generic motion confirmed |
| 1030 | `DEF_OBJECTMOTION_ATTACK_CONFIRM` | Attack confirmed |
| 1040 | `DEF_OBJECTMOTION_REJECT` | Motion rejected |

**Move Confirm Packet:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    DEF_OBJECTMOVE_CONFIRM
6       2     short   New X Position
8       2     short   New Y Position
10      1     char    Direction
11      1     char    SP Cost
12      1     char    Occupy Status
13+     var   bytes   Map tile data...
```

**Move Reject Packet:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    DEF_OBJECTMOVE_REJECT
6       2     short   Correct X Position
8       2     short   Correct Y Position
```

### MSGID_EVENT_MOTION (0x0FA314D7)

**Direction:** Game Server → Client

Broadcasts motion events from other entities.

**Packet Structure (Player, ObjectID < 10000):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Event Type
6       2     WORD    Object ID
8       2     short   X Position
10      2     short   Y Position
12      2     short   Entity Type
14      1     char    Direction
15      10    char[]  Name
25      2     short   Appearance 1
27      2     short   Appearance 2
29      2     short   Appearance 3
31      2     short   Appearance 4
33      4     int     Appearance Color
37      2     short   Status Flags
39      1     char    Location Flag
```

**Packet Structure (NPC/Monster, ObjectID >= 10000):**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Event Type
6       2     WORD    Object ID
8       2     short   X Position
10      2     short   Y Position
12      2     short   Entity Type
14      1     char    Direction
15      5     char[]  Name (truncated)
20      2     short   Appearance 2
22      2     short   Status Flags
```

### MSGID_COMMAND_CHATMSG (0x03203204)

**Direction:** Client → Game Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       2     short   Player X
8       2     short   Player Y
10      10    char[]  Player Name
20      1     char    Chat Type
21      var   char[]  Message (null-terminated)
──────────────────────────────────────────
Total: 22 + strlen(message) bytes
```

**Chat Types:**

| Value | Description |
|-------|-------------|
| 0 | Normal chat |
| 1 | Shout |
| 2 | Whisper |
| 3 | Guild |
| 4 | Party |
| 10 | GM message |

### MSGID_COMMAND_CHECKCONNECTION (0x03203203)

**Direction:** Client → Game Server

Keepalive/ping packet.

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    MsgType (NULL)
6       4     DWORD   Timestamp
──────────────────────────────────────────
Total: 10 bytes
```

---

## Notification Messages

### MSGID_NOTIFY (0x0FA314D0)

**Direction:** Game Server → Client

General notification container. The actual notification type is in the MsgType field.

### Notification Types

#### Player State Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_HP` | 0x0B07 | HP changed |
| `DEF_NOTIFY_MP` | 0x0B14 | MP changed |
| `DEF_NOTIFY_SP` | 0x0B15 | SP changed |
| `DEF_NOTIFY_EXP` | 0x0B0A | Experience changed |
| `DEF_NOTIFY_LEVELUP` | 0x0B16 | Level up |
| `DEF_NOTIFY_SKILL` | 0x0B23 | Skill changed |
| `DEF_NOTIFY_CHARISMA` | 0x0B32 | Charisma changed |
| `DEF_NOTIFY_HUNGER` | 0x0B39 | Hunger state |

#### Item Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_ITEMOBTAINED` | 0x0B01 | Item received |
| `DEF_NOTIFY_ITEMPURCHASED` | 0x0B06 | Item purchased |
| `DEF_NOTIFY_ITEMLIFESPANEND` | 0x0B17 | Item broke |
| `DEF_NOTIFY_ITEMTOBANK` | 0x0B19 | Item sent to bank |
| `DEF_NOTIFY_CANNOTCARRYMOREITEM` | 0x0B05 | Inventory full |
| `DEF_NOTIFY_SETITEMCOUNT` | 0x0B25 | Item stack count |
| `DEF_NOTIFY_GIVEITEMFIN_ERASEITEM` | 0x0B1D | Item given away |
| `DEF_NOTIFY_DROPITEMFIN_ERASEITEM` | 0x0B1F | Item dropped |
| `DEF_NOTIFY_ITEMDEPLETED_ERASEITEM` | 0x0B20 | Item consumed |
| `DEF_NOTIFY_ITEMREPAIRED` | 0x0B30 | Item repaired |
| `DEF_NOTIFY_ITEMSOLD` | 0x0B31 | Item sold |
| `DEF_NOTIFY_ITEMCOLORCHANGE` | 0x0B65 | Item color changed |
| `DEF_NOTIFY_ITEMATTRIBUTECHANGE` | 0x0BA3 | Item attributes changed |
| `DEF_NOTIFY_ITEMRELEASED` | 0x0B5C | Item unequipped |
| `DEF_NOTIFY_GIZONEITEMCHANGE` | 0x0BA5 | Gizon upgrade result |

#### Combat Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_KILLED` | 0x0B09 | Entity killed |
| `DEF_NOTIFY_PKPENALTY` | 0x0B1A | PK penalty applied |
| `DEF_NOTIFY_PKCAPTURED` | 0x0B1B | PK captured |
| `DEF_NOTIFY_ENEMYKILLREWARD` | 0x0B1C | Enemy kill reward |
| `DEF_NOTIFY_ENEMYKILLS` | 0x0B5A | Kill count |
| `DEF_NOTIFY_DAMAGEMOVE` | 0x0B74 | Damage with knockback |
| `DEF_NOTIFY_SUPERATTACKLEFT` | 0x0B52 | Super attack charges |
| `DEF_NOTIFY_SAFEATTACKMODE` | 0x0B51 | Safe attack toggle |

#### Magic and Skill Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_MAGICSTUDYSUCCESS` | 0x0B10 | Learned magic |
| `DEF_NOTIFY_MAGICSTUDYFAIL` | 0x0B11 | Failed to learn |
| `DEF_NOTIFY_SKILLTRAINSUCCESS` | 0x0B12 | Skill trained |
| `DEF_NOTIFY_SKILLTRAINFAIL` | 0x0B13 | Skill training failed |
| `DEF_NOTIFY_MAGICEFFECTON` | 0x0B27 | Magic effect started |
| `DEF_NOTIFY_MAGICEFFECTOFF` | 0x0B28 | Magic effect ended |
| `DEF_NOTIFY_SKILLUSINGEND` | 0x0B2A | Skill use finished |

#### Guild Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_QUERY_JOINGUILDREQPERMISSION` | 0x0B02 | Join request pending |
| `DEF_NOTIFY_QUERY_DISMISSGUILDREQPERMISSION` | 0x0B03 | Dismiss request pending |
| `DEF_NOTIFY_WAITFORGUILDOPERATION` | 0x0B04 | Waiting for guild op |
| `DEF_NOTIFY_GUILDDISBANDED` | 0x0B0B | Guild disbanded |
| `DEF_NOTIFY_CANNOTJOINMOREGUILDSMAN` | 0x0B0D | Guild full |
| `DEF_NOTIFY_NEWGUILDSMAN` | 0x0B0E | New guild member |
| `DEF_NOTIFY_DISMISSGUILDSMAN` | 0x0B0F | Member dismissed |
| `DEF_NOTIFY_NOGUILDMASTERLEVEL` | 0x0B77 | Insufficient guild rank |
| `DEF_NOTIFY_SUCCESSBANGUILDMAN` | 0x0B78 | Member banned |
| `DEF_NOTIFY_CANNOTBANGUILDMAN` | 0x0B79 | Cannot ban member |
| `DEF_NOTIFY_REQGUILDNAMEANSWER` | 0x0BA6 | Guild name response |

#### Party Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_RESPONSE_CREATENEWPARTY` | 0x0B80 | Party created |
| `DEF_NOTIFY_QUERY_JOINPARTY` | 0x0B81 | Party invite received |
| `DEF_NOTIFY_PARTY` | 0x0BA2 | General party update |

#### Dynamic World Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_NEWDYNAMICOBJECT` | 0x0B21 | Dynamic object spawned |
| `DEF_NOTIFY_DELDYNAMICOBJECT` | 0x0B22 | Dynamic object removed |
| `DEF_NOTIFY_TIMECHANGE` | 0x0B41 | Day/night change |
| `DEF_NOTIFY_WHETHERCHANGE` | 0x0B4D | Weather change |
| `DEF_NOTIFY_SHOWMAP` | 0x0B2B | Show minimap |

#### Shop and Economy

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_NOTENOUGHGOLD` | 0x0B08 | Insufficient gold |
| `DEF_NOTIFY_CANNOTSELLITEM` | 0x0B2C | Cannot sell item |
| `DEF_NOTIFY_SELLITEMPRICE` | 0x0B2D | Sell price quote |
| `DEF_NOTIFY_CANNOTREPAIRITEM` | 0x0B2E | Cannot repair |
| `DEF_NOTIFY_REPAIRITEMPRICE` | 0x0B2F | Repair price quote |
| `DEF_NOTIFY_REWARDGOLD` | 0x0B4F | Gold reward |

#### Exchange Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_OPENEXCHANGEWINDOW` | 0x0B5E | Open trade window |
| `DEF_NOTIFY_SETEXCHANGEITEM` | 0x0B5F | Trade item set |
| `DEF_NOTIFY_CANCELEXCHANGEITEM` | 0x0B60 | Trade cancelled |
| `DEF_NOTIFY_EXCHANGEITEMCOMPLETE` | 0x0B61 | Trade completed |
| `DEF_NOTIFY_CANNOTGIVEITEM` | 0x0B62 | Cannot give item |

#### Quest Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_QUESTCONTENTS` | 0x0B66 | Quest details |
| `DEF_NOTIFY_QUESTABORTED` | 0x0B67 | Quest cancelled |
| `DEF_NOTIFY_QUESTCOMPLETED` | 0x0B68 | Quest completed |
| `DEF_NOTIFY_QUESTREWARD` | 0x0B69 | Quest reward |

#### Crafting Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_BUILDITEMSUCCESS` | 0x0B70 | Crafting success |
| `DEF_NOTIFY_BUILDITEMFAIL` | 0x0B71 | Crafting failed |
| `DEF_NOTIFY_NOMATCHINGPORTION` | 0x0B53 | No matching recipe |
| `DEF_NOTIFY_LOWPORTIONSKILL` | 0x0B54 | Skill too low |
| `DEF_NOTIFY_PORTIONFAIL` | 0x0B55 | Potion failed |
| `DEF_NOTIFY_PORTIONSUCCESS` | 0x0B56 | Potion success |

#### Fishing Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_EVENTFISHMODE` | 0x0B47 | Fishing mode active |
| `DEF_NOTIFY_FISHCHANCE` | 0x0B48 | Fishing opportunity |
| `DEF_NOTIFY_FISHSUCCESS` | 0x0B4A | Caught fish |
| `DEF_NOTIFY_FISHFAIL` | 0x0B4B | Fish escaped |
| `DEF_NOTIFY_FISHCANCELED` | 0x0B4C | Fishing cancelled |

#### Crusade Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_CRUSADE` | 0x0B94 | Crusade event |
| `DEF_NOTIFY_LOCKEDMAP` | 0x0B95 | Map locked |
| `DEF_NOTIFY_DUTYSELECTED` | 0x0B96 | Duty assigned |
| `DEF_NOTIFY_MAPSTATUSNEXT` | 0x0B97 | Map status (more) |
| `DEF_NOTIFY_MAPSTATUSLAST` | 0x0B98 | Map status (final) |
| `DEF_NOTIFY_METEORSTRIKECOMING` | 0x0B9B | Meteor warning |
| `DEF_NOTIFY_METEORSTRIKEHIT` | 0x0B9C | Meteor impact |
| `DEF_NOTIFY_GRANDMAGICRESULT` | 0x0B9D | Grand magic result |
| `DEF_NOTIFY_NOMORECRUSADESTRUCTURE` | 0x0B9E | No more structures |
| `DEF_NOTIFY_CONSTRUCTIONPOINT` | 0x0B9F | Construction points |
| `DEF_NOTIFY_CANNOTCONSTRUCT` | 0x0BA1 | Cannot build |
| `DEF_NOTIFY_TCLOC` | 0x0BA0 | Teleport/construct location |

#### Special Ability Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_ENERGYSPHERECREATED` | 0x0B90 | Energy sphere created |
| `DEF_NOTIFY_ENERGYSPHEREGOALIN` | 0x0B91 | Sphere goal reached |
| `DEF_NOTIFY_SPECIALABILITYENABLED` | 0x0B92 | Special ability ready |
| `DEF_NOTIFY_SPECIALABILITYSTATUS` | 0x0B93 | Ability status |

#### System Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_SERVERCHANGE` | 0x0B24 | Server transfer |
| `DEF_NOTIFY_SERVERSHUTDOWN` | 0x0B4E | Server shutdown |
| `DEF_NOTIFY_TOTALUSERS` | 0x0B29 | Total users online |
| `DEF_NOTIFY_NOTICEMSG` | 0x0B46 | System notice |
| `DEF_NOTIFY_EVENTMSGSTRING` | 0x0B0C | Event message |
| `DEF_NOTIFY_DEBUGMSG` | 0x0B49 | Debug message |
| `DEF_NOTIFY_FORCEDISCONN` | 0x0B75 | Forced disconnect |
| `DEF_NOTIFY_FORCERECALLTIME` | 0x0BA7 | Force recall time |
| `DEF_NOTIFY_OBSERVERMODE` | 0x0B72 | Observer mode |
| `DEF_NOTIFY_GLOBALATTACKMODE` | 0x0B73 | Global attack mode |
| `DEF_NOTIFY_FIGHTZONERESERVE` | 0x0B76 | Fight zone reserved |

#### Player Status Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_PLAYERONGAME` | 0x0B33 | Player online |
| `DEF_NOTIFY_PLAYERNOTONGAME` | 0x0B34 | Player offline |
| `DEF_NOTIFY_WHISPERMODEON` | 0x0B35 | Whisper enabled |
| `DEF_NOTIFY_WHISPERMODEOFF` | 0x0B36 | Whisper disabled |
| `DEF_NOTIFY_PLAYERPROFILE` | 0x0B37 | Player profile |
| `DEF_NOTIFY_PLAYERSHUTUP` | 0x0B42 | Player muted |
| `DEF_NOTIFY_ADMINUSERLEVELLOW` | 0x0B43 | Admin level low |
| `DEF_NOTIFY_CANNOTRATING` | 0x0B44 | Cannot rate |
| `DEF_NOTIFY_RATINGPLAYER` | 0x0B45 | Rating player |
| `DEF_NOTIFY_NPCTALK` | 0x0B57 | NPC dialogue |
| `DEF_NOTIFY_ADMINIFO` | 0x0B58 | Admin info |
| `DEF_NOTIFY_HELP` | 0x0B99 | Help response |
| `DEF_NOTIFY_HELPFAILED` | 0x0B9A | Help failed |
| `DEF_NOTIFY_TOBERECALLED` | 0x0B40 | Recall imminent |

#### Level Restrictions

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_LIMITEDLEVEL` | 0x0B18 | Level restricted |
| `DEF_NOTIFY_TRAVELERLIMITEDLEVEL` | 0x0B38 | Traveler level limit |

#### Agriculture Notifications

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_NOMOREAGRICULTURE` | 0x0BB0 | Agriculture limit |
| `DEF_NOTIFY_AGRICULTURESKILLLIMIT` | 0x0BB1 | Agri skill limit |
| `DEF_NOTIFY_AGRICULTURENOAREA` | 0x0BB2 | Not agriculture area |

#### Hunting Mode

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_RESPONSE_HUNTMODE` | 0x0BA9 | Hunt mode response |

#### Item Upgrade

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_GIZONITEMUPGRADELEFT` | 0x0BA4 | Upgrade attempts left |
| `DEF_NOTIFY_ITEMUPGRADEFAIL` | 0x0BA8 | Upgrade failed |

#### Monster Events

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_NOTIFY_MONSTEREVENT_POSITION` | 0x0BAA | Monster event location |

---

## Common Command Messages

### MSGID_COMMAND_COMMON (0x0FA314DC)

**Direction:** Client → Game Server

General gameplay command container.

**Base Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Command Type
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction
11+     var   -       Command-specific data
```

### Common Type Constants

#### Item Operations

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_ITEMDROP` | 0x0A01 | Drop item |
| `DEF_COMMONTYPE_EQUIPITEM` | 0x0A02 | Equip item |
| `DEF_COMMONTYPE_RELEASEITEM` | 0x0A0A | Unequip item |
| `DEF_COMMONTYPE_SETITEM` | 0x0A0C | Set item position |
| `DEF_COMMONTYPE_REQ_USEITEM` | 0x0A11 | Use item |
| `DEF_COMMONTYPE_GIVEITEMTOCHAR` | 0x0A05 | Give item to player |
| `DEF_COMMONTYPE_EXCHANGEITEMTOCHAR` | 0x0A1E | Exchange items |
| `DEF_COMMONTYPE_SETEXCHANGEITEM` | 0x0A1F | Set exchange item |
| `DEF_COMMONTYPE_CONFIRMEXCHANGEITEM` | 0x0A20 | Confirm exchange |
| `DEF_COMMONTYPE_CANCELEXCHANGEITEM` | 0x0A21 | Cancel exchange |
| `DEF_COMMONTYPE_UPGRADEITEM` | 0x0A58 | Upgrade item |

#### Shop Operations

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_REQ_LISTCONTENTS` | 0x0A03 | Request shop list |
| `DEF_COMMONTYPE_REQ_PURCHASEITEM` | 0x0A04 | Purchase item |
| `DEF_COMMONTYPE_REQ_SELLITEM` | 0x0A13 | Sell item |
| `DEF_COMMONTYPE_REQ_REPAIRITEM` | 0x0A14 | Repair item |
| `DEF_COMMONTYPE_REQ_SELLITEMCONFIRM` | 0x0A15 | Confirm sell |
| `DEF_COMMONTYPE_REQ_REPAIRITEMCONFIRM` | 0x0A16 | Confirm repair |

#### Magic and Skills

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_MAGIC` | 0x0A0D | Cast spell |
| `DEF_COMMONTYPE_REQ_STUDYMAGIC` | 0x0A0E | Learn spell |
| `DEF_COMMONTYPE_REQ_TRAINSKILL` | 0x0A0F | Train skill |
| `DEF_COMMONTYPE_REQ_USESKILL` | 0x0A12 | Use skill |
| `DEF_COMMONTYPE_REQ_SETDOWNSKILLINDEX` | 0x0A1B | Set skill hotkey |
| `DEF_COMMONTYPE_GETMAGICABILITY` | 0x0A24 | Get magic ability |

#### Guild Operations

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_JOINGUILDAPPROVE` | 0x0A06 | Approve join |
| `DEF_COMMONTYPE_JOINGUILDREJECT` | 0x0A07 | Reject join |
| `DEF_COMMONTYPE_DISMISSGUILDAPPROVE` | 0x0A08 | Approve dismiss |
| `DEF_COMMONTYPE_DISMISSGUILDREJECT` | 0x0A09 | Reject dismiss |
| `DEF_COMMONTYPE_CLEARGUILDNAME` | 0x0A25 | Clear guild name |
| `DEF_COMMONTYPE_BANGUILD` | 0x0A26 | Ban from guild |
| `DEF_COMMONTYPE_REQGUILDNAME` | 0x0A59 | Request guild name |
| `DEF_COMMONTYPE_SETGUILDTELEPORTLOC` | 0x0A54 | Set guild TP location |
| `DEF_COMMONTYPE_GUILDTELEPORT` | 0x0A55 | Guild teleport |
| `DEF_COMMONTYPE_SETGUILDCONSTRUCTLOC` | 0x0A57 | Set construct location |

#### Combat

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_TOGGLECOMBATMODE` | 0x0A0B | Toggle combat mode |
| `DEF_COMMONTYPE_TOGGLESAFEATTACKMODE` | 0x0A18 | Toggle safe attack |

#### Quest and Rewards

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_QUESTACCEPTED` | 0x0A22 | Accept quest |
| `DEF_COMMONTYPE_REQUEST_CANCELQUEST` | 0x0A50 | Cancel quest |
| `DEF_COMMONTYPE_REQ_GETREWARDMONEY` | 0x0A10 | Claim reward |

#### Party Operations

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY` | 0x0A30 | Accept party invite |
| `DEF_COMMONTYPE_REQUEST_JOINPARTY` | 0x0A31 | Request to join |
| `DEF_COMMONTYPE_RESPONSE_JOINPARTY` | 0x0A32 | Response to join |
| `DEF_COMMONTYPE_REQUEST_ACTIVATESPECABLTY` | 0x0A40 | Activate ability |

#### NPC Interaction

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_TALKTONPC` | 0x0A1A | Talk to NPC |

#### Fishing

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_REQ_GETFISHTHISTIME` | 0x0A17 | Catch fish |

#### Crafting

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_REQ_CREATEPORTION` | 0x0A19 | Create potion |
| `DEF_COMMONTYPE_BUILDITEM` | 0x0A23 | Build item |

#### Crusade

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_REQ_GETOCCUPYFLAG` | 0x0A1C | Get occupy flag |
| `DEF_COMMONTYPE_REQ_GETHEROMANTLE` | 0x0A1D | Get hero mantle |
| `DEF_COMMONTYPE_REQ_GETOCCUPYFIGHTZONETICKET` | 0x0A25 | Fight zone ticket |
| `DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY` | 0x0A51 | Select duty |
| `DEF_COMMONTYPE_REQUEST_MAPSTATUS` | 0x0A52 | Request map status |
| `DEF_COMMONTYPE_SUMMONWARUNIT` | 0x0A56 | Summon war unit |

#### Miscellaneous

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_COMMONTYPE_REQUEST_HELP` | 0x0A53 | Request help |
| `DEF_COMMONTYPE_REQUEST_HUNTMODE` | 0x0A60 | Toggle hunt mode |

### Common Command Packet Examples

#### DEF_COMMONTYPE_BUILDITEM Packet

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    DEF_COMMONTYPE_BUILDITEM
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction
11      20    char[]  Item Name
31      1     char    Ingredient Slot 1
32      1     char    Ingredient Slot 2
33      1     char    Ingredient Slot 3
34      1     char    Ingredient Slot 4
35      1     char    Ingredient Slot 5
36      1     char    Ingredient Slot 6
──────────────────────────────────────────
Total: 37 bytes
```

#### DEF_COMMONTYPE_REQ_CREATEPORTION Packet

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    DEF_COMMONTYPE_REQ_CREATEPORTION
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction
11      1     char    Ingredient Slot 1
12      1     char    Ingredient Slot 2
13      1     char    Ingredient Slot 3
14      1     char    Ingredient Slot 4
15      1     char    Ingredient Slot 5
16      1     char    Ingredient Slot 6
──────────────────────────────────────────
Total: 18 bytes
```

#### Generic Common Command Packet (without string)

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Command Type
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction
11      4     int     Parameter 1
15      4     int     Parameter 2
19      4     int     Parameter 3
23      4     DWORD   Timestamp
──────────────────────────────────────────
Total: 27 bytes
```

#### Generic Common Command Packet (with string)

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    Command Type
6       2     short   Player X
8       2     short   Player Y
10      1     char    Direction
11      4     int     Parameter 1
15      4     int     Parameter 2
19      4     int     Parameter 3
23      30    char[]  String Parameter
53      4     int     Parameter 4
──────────────────────────────────────────
Total: 57 bytes
```

---

## Configuration Data Messages

### MSGID_ITEMCONFIGURATIONCONTENTS (0x0FA314D9)

**Direction:** Game Server → Client

Contains item definition data for the client.

### MSGID_NPCCONFIGURATIONCONTENTS (0x0FA314DA)

**Direction:** Game Server → Client

Contains NPC definition data.

### MSGID_MAGICCONFIGURATIONCONTENTS (0x0FA314DB)

**Direction:** Game Server → Client

Contains magic/spell configuration.

### MSGID_SKILLCONFIGURATIONCONTENTS (0x0FA314DC)

**Direction:** Game Server → Client

Contains skill configuration.

### MSGID_PLAYERITEMLISTCONTENTS (0x0FA314DD)

**Direction:** Game Server → Client

Contains player's inventory items.

### MSGID_PORTIONCONFIGURATIONCONTENTS (0x0FA314DE)

**Direction:** Game Server → Client

Contains potion recipe configuration.

### MSGID_PLAYERCHARACTERCONTENTS (0x0FA40000)

**Direction:** Game Server → Client

Contains player character statistics.

### MSGID_QUESTCONFIGURATIONCONTENTS (0x0FA40001)

**Direction:** Game Server → Client

Contains quest definitions.

### MSGID_BUILDITEMCONFIGURATIONCONTENTS (0x0FA40002)

**Direction:** Game Server → Client

Contains craftable item definitions.

### MSGID_DUPITEMIDFILECONTENTS (0x0FA40003)

**Direction:** Game Server → Client

Contains duplicate item ID mapping.

### MSGID_NOTICEMENTFILECONTENTS (0x0FA40004)

**Direction:** Game Server → Client

Contains server notices.

---

## Guild and Party Messages

### Guild Messages

#### MSGID_REQUEST_CREATENEWGUILD (0x0FC94208)

**Direction:** Client → Game Server

**Packet Structure:**

```
Offset  Size  Type    Field
──────────────────────────────────────────
0       4     DWORD   MsgID
4       2     WORD    DEF_MSGTYPE_CONFIRM
6       10    char[]  Player Name
16      10    char[]  Account Name
26      10    char[]  Account Password
36      20    char[]  Guild Name (spaces → underscores)
──────────────────────────────────────────
Total: 56 bytes
```

#### MSGID_RESPONSE_CREATENEWGUILD (0x0FC94209)

**Direction:** Game Server → Client

#### MSGID_REQUEST_DISBANDGUILD (0x0FC9420A)

**Direction:** Client → Game Server

Same structure as create guild.

#### MSGID_RESPONSE_DISBANDGUILD (0x0FC9420B)

**Direction:** Game Server → Client

#### MSGID_GUILDNOTIFY (0x0DF30760)

**Direction:** Login Server → Game Server

Guild system notifications.

**Subtypes:**

| Value | Constant | Description |
|-------|----------|-------------|
| 0x1F00 | `DEF_GUILDNOTIFY_NEWGUILDSMAN` | New guild member |

### Party Messages

#### MSGID_PARTYOPERATION (0x3C00123A)

**Direction:** Both

Party system operations.

---

## Crusade/War Messages

### MSGID_COLLECTEDMANA (0x3AE90000)

**Direction:** Game Server → Client

Mana collection for grand magic.

### MSGID_METEORSTRIKE (0x3AE90001)

**Direction:** Game Server → Client

Meteor strike incoming.

### MSGID_OCCUPYFLAGDATA (0x167C0A30)

**Direction:** Game Server → Client

Occupation flag status.

### MSGID_REQUEST_SAVEARESDENOCCUPYFLAGDATA (0x167C0A31)

**Direction:** Game Server → Login Server

Save Aresden occupation data.

### MSGID_REQUEST_SAVEELVINEOCCUPYFLAGDATA (0x167C0A32)

**Direction:** Game Server → Login Server

Save Elvine occupation data.

---

## Administrative Messages

### MSGID_ADMINUSER (0x12A01003)

**Direction:** Game Server → Client

Admin user login notification.

### MSGID_BWM_INIT (0x19CC0F82)

**Direction:** System

Bad word monitor initialization.

### MSGID_BWM_COMMAND_SHUTUP (0x19CC0F84)

**Direction:** Admin → Game Server

Mute player command.

### GSM Gateway-Server Messages

| Constant | Value | Description |
|----------|-------|-------------|
| `GSM_REQUEST_FINDCHARACTER` | 0x01 | Find character |
| `GSM_RESPONSE_FINDCHARACTER` | 0x02 | Character found |
| `GSM_GRANDMAGICRESULT` | 0x03 | Grand magic result |
| `GSM_GRANDMAGICLAUNCH` | 0x04 | Grand magic launched |
| `GSM_COLLECTEDMANA` | 0x05 | Mana collected |
| `GSM_BEGINCRUSADE` | 0x06 | Crusade started |
| `GSM_ENDCRUSADE` | 0x07 | Crusade ended |
| `GSM_MIDDLEMAPSTATUS` | 0x08 | Middle map status |
| `GSM_SETGUILDTELEPORTLOC` | 0x09 | Set guild TP |
| `GSM_CONSTRUCTIONPOINT` | 0x0A | Construction points |
| `GSM_SETGUILDCONSTRUCTLOC` | 0x0B | Set construct location |
| `GSM_CHATMSG` | 0x0C | Cross-server chat |
| `GSM_WHISFERMSG` | 0x0D | Cross-server whisper |
| `GSM_DISCONNECT` | 0x0E | Disconnect player |
| `GSM_REQUEST_SUMMONPLAYER` | 0x0F | Summon player |
| `GSM_REQUEST_SHUTUPPLAYER` | 0x10 | Mute player request |
| `GSM_RESPONSE_SHUTUPPLAYER` | 0x11 | Mute response |
| `GSM_REQUEST_SETFORCERECALLTIME` | 0x12 | Force recall time |

---

## Data Type Reference

### Primitive Types

| Type | Size | Range | Notes |
|------|------|-------|-------|
| char | 1 byte | -128 to 127 | Signed |
| BYTE | 1 byte | 0 to 255 | Unsigned |
| short | 2 bytes | -32768 to 32767 | Little-endian |
| WORD | 2 bytes | 0 to 65535 | Unsigned, little-endian |
| int | 4 bytes | -2^31 to 2^31-1 | Little-endian |
| DWORD | 4 bytes | 0 to 2^32-1 | Unsigned, little-endian |

### Common Field Sizes

| Field | Size | Description |
|-------|------|-------------|
| Account Name | 10 bytes | Null-padded |
| Password | 10 bytes | Null-padded |
| Player Name | 10 bytes | Null-padded |
| Map Name | 10 bytes | Null-padded |
| World Server | 30 bytes | Null-padded |
| Guild Name | 20 bytes | Spaces → underscores |
| Item Name | 20 bytes | Null-padded |
| Email | 50 bytes | Null-padded |

### Direction Values

```cpp
// 8-directional movement
//     8  1  2
//     7  0  3
//     6  5  4
```

| Value | Direction |
|-------|-----------|
| 0 | None/Center |
| 1 | North |
| 2 | Northeast |
| 3 | East |
| 4 | Southeast |
| 5 | South |
| 6 | Southwest |
| 7 | West |
| 8 | Northwest |

### Object ID Ranges

| Range | Type |
|-------|------|
| 1-9999 | Players |
| 10000-29999 | NPCs/Monsters |
| 30000+ | Items/Effects |

### Status Flags

Status is a 16-bit bitfield:

| Bit | Flag | Description |
|-----|------|-------------|
| 0x0001 | - | Unknown |
| 0x0002 | - | Unknown |
| 0x0004 | - | Unknown |
| 0x0008 | - | Unknown |
| 0x0010 | - | PK status |
| 0x0020 | - | Combat mode |
| 0x0040 | - | Guild status |
| ... | ... | ... |

---

## Connection Flow Diagrams

### Complete Login Sequence

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           CLIENT STATE MACHINE                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────┐                                                       │
│  │  MAIN MENU   │                                                       │
│  └──────┬───────┘                                                       │
│         │ User clicks "Login"                                           │
│         ▼                                                               │
│  ┌──────────────┐     TCP Connect      ┌──────────────┐                │
│  │  CONNECTING  │─────────────────────>│ Login Server │                │
│  └──────┬───────┘                      └──────────────┘                │
│         │ Connection established                                        │
│         ▼                                                               │
│  ┌──────────────┐    REQUEST_LOGIN     ┌──────────────┐                │
│  │   WAITING    │─────────────────────>│ Login Server │                │
│  │   RESPONSE   │<─────────────────────│              │                │
│  └──────┬───────┘    RESPONSE_LOG      └──────────────┘                │
│         │ (character list)                                              │
│         ▼                                                               │
│  ┌──────────────┐                                                       │
│  │   SELECT     │ User selects character                                │
│  │  CHARACTER   │                                                       │
│  └──────┬───────┘                                                       │
│         ▼                                                               │
│  ┌──────────────┐   REQUEST_ENTERGAME  ┌──────────────┐                │
│  │   WAITING    │─────────────────────>│ Login Server │                │
│  │   RESPONSE   │<─────────────────────│              │                │
│  └──────┬───────┘  RESPONSE_ENTERGAME  └──────────────┘                │
│         │ (game server address)                                         │
│         ▼                                                               │
│  ┌──────────────┐     TCP Connect      ┌──────────────┐                │
│  │  CONNECTING  │─────────────────────>│ Game Server  │                │
│  │  TO GAME     │                      └──────────────┘                │
│  └──────┬───────┘                                                       │
│         │ Connection established                                        │
│         ▼                                                               │
│  ┌──────────────┐  REQUEST_INITPLAYER  ┌──────────────┐                │
│  │   WAITING    │─────────────────────>│ Game Server  │                │
│  │   INIT       │<─────────────────────│              │                │
│  └──────┬───────┘  RESPONSE_INITPLAYER └──────────────┘                │
│         │                                                               │
│         ▼                                                               │
│  ┌──────────────┐  REQUEST_INITDATA    ┌──────────────┐                │
│  │   LOADING    │─────────────────────>│ Game Server  │                │
│  │   DATA       │<─────────────────────│              │                │
│  └──────┬───────┘  RESPONSE_INITDATA   └──────────────┘                │
│         │          + Config messages                                    │
│         ▼                                                               │
│  ┌──────────────┐                                                       │
│  │  MAIN GAME   │  ◄── Game loop begins                                │
│  └──────────────┘                                                       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Game Loop Message Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            GAME LOOP                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                     CLIENT ACTIONS                               │   │
│  │                                                                   │   │
│  │  Movement          Attack            Chat            Use Item    │   │
│  │     │                 │                │                 │       │   │
│  │     ▼                 ▼                ▼                 ▼       │   │
│  │  COMMAND_         COMMAND_         COMMAND_         COMMAND_    │   │
│  │  MOTION           MOTION           CHATMSG          COMMON      │   │
│  │  (OBJECTMOVE)     (OBJECTATTACK)                    (USEITEM)   │   │
│  │     │                 │                │                 │       │   │
│  └─────┼─────────────────┼────────────────┼─────────────────┼───────┘   │
│        │                 │                │                 │           │
│        └─────────────────┴────────────────┴─────────────────┘           │
│                                    │                                    │
│                                    ▼                                    │
│                          ┌─────────────────┐                           │
│                          │   GAME SERVER   │                           │
│                          └────────┬────────┘                           │
│                                   │                                    │
│        ┌──────────────────────────┼──────────────────────────┐         │
│        │                          │                          │         │
│        ▼                          ▼                          ▼         │
│  ┌───────────────┐       ┌───────────────┐       ┌───────────────┐    │
│  │   RESPONSE_   │       │   EVENT_      │       │   NOTIFY      │    │
│  │   MOTION      │       │   MOTION      │       │   (various)   │    │
│  │ (confirm/rej) │       │ (other chars) │       │               │    │
│  └───────────────┘       └───────────────┘       └───────────────┘    │
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                     SERVER BROADCASTS                            │   │
│  │                                                                   │   │
│  │  Other player      NPC spawns       Weather         System       │   │
│  │  movements         deaths           changes         notices      │   │
│  │                                                                   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Message ID Quick Reference

### Request Messages (Client → Server)

| ID | Name |
|----|------|
| 0x05040205 | MSGID_REQUEST_INITPLAYER |
| 0x05080404 | MSGID_REQUEST_INITDATA |
| 0x0FC94201 | MSGID_REQUEST_LOGIN |
| 0x0FC94202 | MSGID_REQUEST_CREATENEWACCOUNT |
| 0x0FC94204 | MSGID_REQUEST_CREATENEWCHARACTER |
| 0x0FC94205 | MSGID_REQUEST_ENTERGAME |
| 0x0FC94207 | MSGID_REQUEST_DELETECHARACTER |
| 0x0FC94208 | MSGID_REQUEST_CREATENEWGUILD |
| 0x0FC9420A | MSGID_REQUEST_DISBANDGUILD |
| 0x0FC9420E | MSGID_REQUEST_CIVILRIGHT |
| 0x0FC94210 | MSGID_REQUEST_CHANGEPASSWORD |
| 0x0FC94212 | MSGID_REQUEST_INPUTKEYCODE |
| 0x0DF30751 | MSGID_REQUEST_RETRIEVEITEM |
| 0x0DF40000 | MSGID_REQUEST_FULLOBJECTDATA |
| 0x0EA03201 | MSGID_REQUEST_TELEPORT |
| 0x0EA03202 | MSGID_REQUEST_TELEPORT_LIST |
| 0x0EA03204 | MSGID_REQUEST_CHARGED_TELEPORT |
| 0x180ACE0A | MSGID_REQUEST_SETITEMPOS |
| 0x220B2F00 | MSGID_REQUEST_NOTICEMENT |
| 0x27B314D0 | MSGID_REQUEST_PANNING |
| 0x28010EEE | MSGID_REQUEST_RESTART |
| 0x2900AD30 | MSGID_REQUEST_SELLITEMLIST |
| 0x12A01007 | MSGID_REQUEST_FIGHTZONE_RESERVE |

### Response Messages (Server → Client)

| ID | Name |
|----|------|
| 0x05040206 | MSGID_RESPONSE_INITPLAYER |
| 0x05080405 | MSGID_RESPONSE_INITDATA |
| 0x0FA314D6 | MSGID_RESPONSE_MOTION |
| 0x0FC94203 | MSGID_RESPONSE_LOG |
| 0x0FC94206 | MSGID_RESPONSE_ENTERGAME |
| 0x0FC94209 | MSGID_RESPONSE_CREATENEWGUILD |
| 0x0FC9420B | MSGID_RESPONSE_DISBANDGUILD |
| 0x0FC9420F | MSGID_RESPONSE_CIVILRIGHT |
| 0x0FC94211 | MSGID_RESPONSE_CHANGEPASSWORD |
| 0x0FC94213 | MSGID_RESPONSE_INPUTKEYCODE |
| 0x0DF30752 | MSGID_RESPONSE_RETRIEVEITEM |
| 0x0EA03203 | MSGID_RESPONSE_TELEPORT_LIST |
| 0x0EA03205 | MSGID_RESPONSE_CHARGED_TELEPORT |
| 0x220B2F01 | MSGID_RESPONSE_NOTICEMENT |
| 0x27B314D1 | MSGID_RESPONSE_PANNING |
| 0x12A01008 | MSGID_RESPONSE_FIGHTZONE_RESERVE |

### Event/Command Messages

| ID | Name |
|----|------|
| 0x0FA314D5 | MSGID_COMMAND_MOTION |
| 0x0FA314D7 | MSGID_EVENT_MOTION |
| 0x0FA314D8 | MSGID_EVENT_LOG |
| 0x0FA314DB | MSGID_EVENT_COMMON |
| 0x0FA314DC | MSGID_COMMAND_COMMON |
| 0x0FA314D0 | MSGID_NOTIFY |
| 0x03203203 | MSGID_COMMAND_CHECKCONNECTION |
| 0x03203204 | MSGID_COMMAND_CHATMSG |

### Configuration Messages

| ID | Name |
|----|------|
| 0x0FA314D9 | MSGID_ITEMCONFIGURATIONCONTENTS |
| 0x0FA314DA | MSGID_NPCCONFIGURATIONCONTENTS |
| 0x0FA314DB | MSGID_MAGICCONFIGURATIONCONTENTS |
| 0x0FA314DC | MSGID_SKILLCONFIGURATIONCONTENTS |
| 0x0FA314DD | MSGID_PLAYERITEMLISTCONTENTS |
| 0x0FA314DE | MSGID_PORTIONCONFIGURATIONCONTENTS |
| 0x0FA40000 | MSGID_PLAYERCHARACTERCONTENTS |
| 0x0FA40001 | MSGID_QUESTCONFIGURATIONCONTENTS |
| 0x0FA40002 | MSGID_BUILDITEMCONFIGURATIONCONTENTS |

---

## Source File Reference

| File | Purpose |
|------|---------|
| `NetMessages.h` | All message ID and type definitions |
| `XSocket.h` | Socket class declaration |
| `XSocket.cpp` | Socket implementation |
| `Game.cpp` | Message handlers (bSendCommand, GameRecvMsgHandler, LogRecvMsgHandler, NotifyMsgHandler, etc.) |
| `Game.h` | Handler function declarations, packet index constants |
| `ActionID.h` | Object action definitions |
| `GlobalDef.h` | Protocol version, language settings |

---

## Version History

| Version | Changes |
|---------|---------|
| 1.4 | Added encryption key support |
| 1.41 | Chat message handling improvements |
| 1.43 | Teleport request blocking |
| 1.4311-3 | Fight zone reservation messages |
| 2.15 | Gizon item upgrade system |
| 2.17 | Force recall time, guild name requests |
| 2.171 | Attack confirmation timestamps |
| 2.18 | International version adjustments |
| 2.19 | Agriculture notifications |
| 2.20 | Current version (English/Japanese) |

---

## Notes for Implementers

1. **Byte Order**: All multi-byte values are little-endian (x86 native)
2. **String Fields**: Use null-padding, not null-termination
3. **Encryption**: Optional per-packet, key in byte 0
4. **Timestamps**: Use `timeGetTime()` for consistency
5. **Error Handling**: Check socket events carefully
6. **Buffer Management**: Unsent data queue prevents loss during blocking
7. **Connection Recovery**: Auto-retry on FD_CONNECT failure
8. **Message Size**: 3-byte header + body; size includes header

---

*This document was generated from analysis of the legacy Helbreath client source code, version 2.20 (English International). Last updated: 2026*
