# Legacy Game State Machine

> **Documentation Version**: 1.0
> **Last Updated**: 2026-01-31
> **Source Files**: `Game.h`, `Game.cpp`, `Wmain.cpp`, `GlobalDef.h`
> **Scope**: Complete game state/mode management system

---

## Table of Contents

1. [Overview](#overview)
2. [State Constants](#state-constants)
3. [Core State Management](#core-state-management)
4. [State Transition Diagram](#state-transition-diagram)
5. [Detailed State Specifications](#detailed-state-specifications)
6. [State Variables](#state-variables)
7. [Network Socket Events](#network-socket-events)
8. [Timeout Behaviors](#timeout-behaviors)
9. [Error States & Recovery](#error-states--recovery)
10. [Conditional Compilation](#conditional-compilation)
11. [Startup Sequence](#startup-sequence)
12. [Shutdown Sequence](#shutdown-sequence)

---

## Overview

The Helbreath client uses a **finite state machine** architecture to manage all screens, dialogs, and gameplay modes. The entire state machine is controlled by a single `char` variable (`m_cGameMode`) within the monolithic `CGame` class.

### Key Characteristics

| Property | Value |
|----------|-------|
| Total States | 22 (including NULL and QUIT) |
| State Variable Type | `char` (signed, range -2 to 21) |
| State Handler Location | `Game.cpp` (~48,500 lines) |
| Main Loop Function | `CGame::UpdateScreen()` |
| State Change Function | `CGame::ChangeGameMode(char cMode)` |

### Design Patterns

- **Entry-Once Initialization**: Each state uses `m_cGameModeCount == 0` to detect first frame and perform one-time setup
- **Frame Counter Capping**: `m_cGameModeCount` caps at 100 to prevent overflow
- **Time Reference Tracking**: `m_dwTime` captures `G_dwGlobalTime` on state entry for timeout calculations
- **Modal State Pattern**: Most states follow a render-input-transition cycle

---

## State Constants

Defined in `Game.h`, lines 104-126:

```cpp
#define DEF_GAMEMODE_NULL                  -2   // Null/uninitialized state
#define DEF_GAMEMODE_ONQUIT                -1   // Shutdown/quit state
#define DEF_GAMEMODE_ONMAINMENU             0   // Main menu screen
#define DEF_GAMEMODE_ONCONNECTING           1   // Connecting to login server
#define DEF_GAMEMODE_ONLOADING              2   // Loading resources/sprites
#define DEF_GAMEMODE_ONWAITINGINITDATA      3   // Waiting for initial game data
#define DEF_GAMEMODE_ONMAINGAME             4   // Active gameplay
#define DEF_GAMEMODE_ONCONNECTIONLOST       5   // Connection error/lost
#define DEF_GAMEMODE_ONMSG                  6   // Modal message dialog
#define DEF_GAMEMODE_ONCREATENEWACCOUNT     7   // Create account screen
#define DEF_GAMEMODE_ONLOGIN                8   // Login screen
#define DEF_GAMEMODE_ONQUERYFORCELOGIN      9   // Force login query dialog
#define DEF_GAMEMODE_ONSELECTCHARACTER      10  // Character selection screen
#define DEF_GAMEMODE_ONCREATENEWCHARACTER   11  // Character creation screen
#define DEF_GAMEMODE_ONWAITINGRESPONSE      12  // Waiting for server response
#define DEF_GAMEMODE_ONQUERYDELETECHARACTER 13  // Delete character confirmation
#define DEF_GAMEMODE_ONLOGRESMSG            14  // Login response message display
#define DEF_GAMEMODE_ONCHANGEPASSWORD       15  // Change password screen
// Note: 16 is unused/skipped
#define DEF_GAMEMODE_ONVERSIONNOTMATCH      17  // Version mismatch error
#define DEF_GAMEMODE_ONINTRODUCTION         18  // Introduction/splash screen
#define DEF_GAMEMODE_ONAGREEMENT            19  // User agreement screen
#define DEF_GAMEMODE_ONSELECTSERVER         20  // Server selection screen
#define DEF_GAMEMODE_ONINPUTKEYCODE         21  // Input keycode screen (China only)
```

### State Categories

| Category | States |
|----------|--------|
| **Initialization** | ONLOADING (2) |
| **Menu/Navigation** | ONMAINMENU (0), ONSELECTSERVER (20) |
| **Authentication** | ONLOGIN (8), ONCREATENEWACCOUNT (7), ONAGREEMENT (19), ONCHANGEPASSWORD (15), ONINPUTKEYCODE (21) |
| **Connection** | ONCONNECTING (1), ONWAITINGRESPONSE (12), ONWAITINGINITDATA (3) |
| **Character Management** | ONSELECTCHARACTER (10), ONCREATENEWCHARACTER (11), ONQUERYDELETECHARACTER (13) |
| **Gameplay** | ONMAINGAME (4) |
| **Dialogs/Messages** | ONMSG (6), ONLOGRESMSG (14), ONQUERYFORCELOGIN (9) |
| **Error/Exit** | ONCONNECTIONLOST (5), ONVERSIONNOTMATCH (17), ONQUIT (-1), NULL (-2) |

---

## Core State Management

### Main Update Loop

Located in `Game.cpp`, lines 1106-1235, the `UpdateScreen()` function dispatches to state-specific handlers:

```cpp
void CGame::UpdateScreen()
{
    switch (m_cGameMode) {
        case DEF_GAMEMODE_ONAGREEMENT:
            UpdateScreen_OnAgreement();
            break;
        case DEF_GAMEMODE_ONCREATENEWACCOUNT:
            UpdateScreen_OnCreateNewAccount();
            break;
        case DEF_GAMEMODE_ONVERSIONNOTMATCH:
            UpdateScreen_OnVersionNotMatch();
            break;
        case DEF_GAMEMODE_ONCONNECTING:
            UpdateScreen_OnConnecting();
            break;
        case DEF_GAMEMODE_ONMAINMENU:
            UpdateScreen_OnMainMenu();
            break;
        case DEF_GAMEMODE_ONLOADING:
            UpdateScreen_OnLoading(TRUE);
            break;
        case DEF_GAMEMODE_ONMAINGAME:
            UpdateScreen_OnGame();
            break;
        case DEF_GAMEMODE_ONWAITINGINITDATA:
            UpdateScreen_OnWaitInitData();
            break;
        case DEF_GAMEMODE_ONCONNECTIONLOST:
            UpdateScreen_OnConnectionLost();
            break;
        case DEF_GAMEMODE_ONMSG:
            UpdateScreen_OnMsg();
            break;
        case DEF_GAMEMODE_ONLOGIN:
            UpdateScreen_OnLogin();
            break;
        case DEF_GAMEMODE_ONSELECTSERVER:
            UpdateScreen_OnSelectServer();
            break;
        case DEF_GAMEMODE_ONQUIT:
            UpdateScreen_OnQuit();
            break;
        case DEF_GAMEMODE_ONQUERYFORCELOGIN:
            UpdateScreen_OnQueryForceLogin();
            break;
        case DEF_GAMEMODE_ONSELECTCHARACTER:
            UpdateScreen_OnSelectCharacter();
            break;
        case DEF_GAMEMODE_ONCREATENEWCHARACTER:
            UpdateScreen_OnCreateNewCharacter();
            break;
        case DEF_GAMEMODE_ONWAITINGRESPONSE:
            UpdateScreen_OnWaitingResponse();
            break;
        case DEF_GAMEMODE_ONQUERYDELETECHARACTER:
            UpdateScreen_OnQueryDeleteCharacter();
            break;
        case DEF_GAMEMODE_ONLOGRESMSG:
            UpdateScreen_OnLogResMsg();
            break;
        case DEF_GAMEMODE_ONCHANGEPASSWORD:
            UpdateScreen_OnChangePassword();
            break;
        case DEF_GAMEMODE_ONINPUTKEYCODE:  // China only
            UpdateScreen_OnInputKeyCode();
            break;
    }
}
```

### State Change Mechanism

Located in `Game.cpp`, lines 15511-15524:

```cpp
void CGame::ChangeGameMode(char cMode)
{
    m_cGameMode = cMode;
    m_cGameModeCount = 0;          // Reset frame counter
    m_dwTime = G_dwGlobalTime;     // Capture current time reference

    // Special case: Server selection redirect
    #ifndef DEF_SELECTSERVER
    if (cMode == DEF_GAMEMODE_ONSELECTSERVER) {
        ZeroMemory(m_cWorldServerName, sizeof(m_cWorldServerName));
        strcpy(m_cWorldServerName, "WS1");  // Default to WS1
        m_cGameMode = DEF_GAMEMODE_ONLOGIN; // Redirect to login
    }
    #endif
}
```

**State Change Actions**:
1. Sets `m_cGameMode` to the new state value
2. Resets `m_cGameModeCount` to 0 (enables entry-once initialization)
3. Captures `G_dwGlobalTime` into `m_dwTime` (for timeout tracking)
4. May perform conditional redirects based on compile flags

### Event Loop Integration

Located in `Wmain.cpp`, lines 279-383:

```cpp
void EventLoop()
{
    MSG msg;
    while (1) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            GetMessage(&msg, NULL, 0, 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (G_pGame->m_bIsProgramActive) {
            // Program has focus - run main game loop
            G_pGame->UpdateScreen();
        }
        else if (G_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING) {
            // Keep loading visible even when minimized
            G_pGame->UpdateScreen_OnLoading(FALSE);
        }
        else {
            // Idle - wait for messages
            WaitMessage();
        }
    }
}
```

---

## State Transition Diagram

```
                         APPLICATION START
                               │
                               ▼
                    ┌─────────────────────┐
                    │   ONLOADING (2)     │
                    │ Load sprites, assets│
                    └──────────┬──────────┘
                               │ Loading complete
                               ▼
                    ┌─────────────────────┐
                    │   ONMAINMENU (0)    │
                    │   Main menu screen  │
                    └──┬───────┬───────┬──┘
           ┌───────────┘       │       └───────────┐
           │                   │                   │
           ▼                   ▼                   ▼
    ┌──────────────┐   ┌──────────────┐    ┌─────────────┐
    │ONSELECTSERVER│   │ ONAGREEMENT  │    │  ONQUIT     │
    │    (20)      │   │    (19)      │    │   (-1)      │
    └──────┬───────┘   └──────┬───────┘    └─────────────┘
           │                  │                    │
           │                  ▼                    ▼
           │           ┌──────────────┐      APPLICATION
           │           │ONCREATENEW   │         EXIT
           │           │ACCOUNT (7)   │
           │           └──────┬───────┘
           │                  │
           └────────┬─────────┘
                    │
                    ▼
             ┌──────────────┐
             │  ONLOGIN (8) │
             │ Login screen │
             └──────┬───────┘
                    │ Submit credentials
                    ▼
          ┌─────────────────────┐
          │  ONCONNECTING (1)   │
          │ Connecting to server│
          └─────────┬───────────┘
                    │
        ┌───────────┼───────────┐
        │           │           │
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────────────┐
   │ONLOGIN  │ │ONWAITING│ │ONCONNECTIONLOST │
   │(Failure)│ │RESPONSE │ │      (5)        │
   └─────────┘ │  (12)   │ └─────────────────┘
               └────┬────┘
                    │
                    ▼
          ┌─────────────────────┐
          │   ONLOGRESMSG (14)  │
          │  Response message   │
          └─────────┬───────────┘
                    │
    ┌───────────────┼───────────────┐
    │               │               │
    ▼               ▼               ▼
┌────────┐  ┌───────────────┐  ┌──────────────┐
│ONLOGIN │  │ONQUERYFORCE   │  │ONSELECTCHAR  │
│ Retry  │  │LOGIN (9)      │  │ACTER (10)    │
└────────┘  └───────┬───────┘  └──────┬───────┘
                    │                 │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
       ┌───────────┐  ┌───────────┐  ┌───────────────┐
       │ONCREATENEW│  │ONQUERYdele│  │ONCONNECTING   │
       │CHARACTER  │  │techar(13) │  │(to game srv)  │
       │   (11)    │  └───────────┘  └───────┬───────┘
       └───────────┘                         │
                                             ▼
                               ┌─────────────────────────┐
                               │  ONWAITINGINITDATA (3)  │
                               │   Waiting for map data  │
                               └───────────┬─────────────┘
                                           │
                                           ▼
                               ┌─────────────────────────┐
                               │    ONMAINGAME (4)       │
                               │   *** GAMEPLAY ***      │
                               │  (Main game loop here)  │
                               └───────────┬─────────────┘
                                           │
                              ┌────────────┴────────────┐
                              │                         │
                              ▼                         ▼
                    ┌──────────────────┐      ┌─────────────┐
                    │ONCONNECTIONLOST  │      │  ONQUIT     │
                    │      (5)         │      │   (-1)      │
                    └──────────────────┘      └─────────────┘

Additional States (accessed from specific contexts):
┌────────────────────────────────────────────────────────────┐
│ ONCHANGEPASSWORD (15) - Password change from login        │
│ ONVERSIONNOTMATCH (17) - Version error from login response│
│ ONINTRODUCTION (18) - Splash/intro (rarely used)          │
│ ONINPUTKEYCODE (21) - China client authentication         │
│ ONMSG (6) - Generic modal message (context-dependent)     │
│ NULL (-2) - Cleanup marker for shutdown                   │
└────────────────────────────────────────────────────────────┘
```

---

## Detailed State Specifications

### State 2: ONLOADING

> **Purpose**: Load all game assets from PAK files on startup

**Source Location**: `Game.cpp`, lines 3596-4310

#### Entry Behavior (`m_cGameModeCount == 0`)

- Initialize loading stage counter (`m_cLoading = 0`)
- Begin progressive asset loading sequence

#### Loading Stages

| Stage | Assets Loaded |
|-------|---------------|
| 0 | Interface sprites (cursor, fonts, dialogs) |
| 4 | Tile sprites and structure sprites |
| 8 | Additional tile sprites and map tiles |
| 12 | Item pack/ground sprites |
| 16 | Equipment sprites |
| 20+ | Complete, transition to menu |

#### Update Loop

```cpp
// Simplified logic
if (m_cGameModeCount == 0) {
    // First frame initialization
    m_cGameModeCount = 1;
}

// Progressive loading
switch (m_cLoading) {
    case 0:  LoadInterfaceSprites(); m_cLoading = 4; break;
    case 4:  LoadTileSprites();      m_cLoading = 8; break;
    case 8:  LoadMapSprites();       m_cLoading = 12; break;
    case 12: LoadItemSprites();      m_cLoading = 16; break;
    case 16: LoadEquipSprites();     m_cLoading = 20; break;
    case 20: ChangeGameMode(DEF_GAMEMODE_ONMAINMENU); break;
}
```

#### Rendering

- Clear screen to black
- Draw loading dialog (`DEF_SPRID_INTERFACE_ND_LOADING`)
- Display progress bar/percentage
- Animate rotating loading indicator
- Show version information
- Render mouse cursor

#### Input Processing

| Input | Action |
|-------|--------|
| ESC (held 1+ sec) | Cancel loading, return to server select or main menu |

#### Transitions

| To State | Condition |
|----------|-----------|
| ONMAINMENU (0) | Loading complete |
| ONSELECTSERVER (20) | ESC pressed (if `DEF_SELECTSERVER` defined) |
| ONMAINMENU (0) | ESC pressed (if `DEF_SELECTSERVER` not defined) |
| ONCONNECTIONLOST (5) | Critical asset load error |

#### State Variables

- `m_cLoading`: Stage counter (0, 4, 8, 12, 16, 20)
- `m_cGameModeCount`: Frame counter (caps at 100)

---

### State 0: ONMAINMENU

> **Purpose**: Display main menu with navigation options

**Source Location**: `Game.cpp`, lines 3112-3393

#### Entry Behavior (`m_cGameModeCount == 0`)

- Delete calc socket (external process)
- Unload loading sprite
- Create mouse interface with 3 clickable rectangles
- Initialize keyboard focus: `m_cCurFocus = 1`, `m_cMaxFocus = 3`
- Reset enter/arrow key presses

#### Menu Items

| Index | Label | Action |
|-------|-------|--------|
| 1 | "New Game / Connect" | `ChangeGameMode(DEF_GAMEMODE_ONSELECTSERVER)` |
| 2 | "Create Account" | `ChangeGameMode(DEF_GAMEMODE_ONAGREEMENT)` or open website |
| 3 | "Exit" | `ChangeGameMode(DEF_GAMEMODE_ONQUIT)` |

#### Rendering

- Clear screen
- Draw main menu sprite (`DEF_SPRID_INTERFACE_ND_MAINMENU`)
- Highlight selected option with animated frame
- Display version information
- Draw mouse cursor

#### Input Processing

| Input | Action |
|-------|--------|
| UP arrow | Navigate to previous menu item |
| DOWN arrow | Navigate to next menu item |
| ENTER | Activate selected menu item |
| Mouse click | Activate clicked menu item |
| Mouse hover | Auto-select item under cursor |

#### Transitions

| To State | Condition |
|----------|-----------|
| ONSELECTSERVER (20) | "New Game" selected (if `DEF_SELECTSERVER`) |
| ONLOGIN (8) | "New Game" selected (if no `DEF_SELECTSERVER`) |
| ONAGREEMENT (19) | "Create Account" selected (if `DEF_MAKE_ACCOUNT`) |
| ONQUIT (-1) | "Exit" selected |

#### State Variables

- `m_cCurFocus`: Current menu selection (1-3)
- `m_cMaxFocus`: Maximum focus level (3)
- `m_bEnterPressed`: Enter key pressed flag
- `m_cArrowPressed`: Arrow key direction

---

### State 20: ONSELECTSERVER

> **Purpose**: Display available game servers for selection

**Source Location**: `Game.cpp`, lines 28002-28500

**Prerequisite**: `DEF_SELECTSERVER` must be defined

#### Entry Behavior

- Display list of available game servers
- Initialize server selection UI
- Create mouse interface for server buttons

#### Rendering

- Server list dialog
- Server names with status indicators
- Selected server highlight
- Description text for selected server

#### Input Processing

| Input | Action |
|-------|--------|
| Arrow keys | Navigate server list |
| ENTER | Select highlighted server |
| Mouse click | Select clicked server |
| ESC | Return to main menu |

#### On Server Selection

```cpp
strcpy(m_cWorldServerName, selectedServerName);
ChangeGameMode(DEF_GAMEMODE_ONLOGIN);
```

#### Transitions

| To State | Condition |
|----------|-----------|
| ONLOGIN (8) | Server selected |
| ONAGREEMENT (19) | "Create Account" clicked |
| ONMAINMENU (0) | ESC or Cancel |

#### Auto-Redirect Behavior

If `DEF_SELECTSERVER` is NOT defined, any transition to this state automatically redirects:

```cpp
#ifndef DEF_SELECTSERVER
if (cMode == DEF_GAMEMODE_ONSELECTSERVER) {
    strcpy(m_cWorldServerName, "WS1");
    m_cGameMode = DEF_GAMEMODE_ONLOGIN;
}
#endif
```

---

### State 8: ONLOGIN

> **Purpose**: Authenticate user with account credentials

**Source Location**: `Game.cpp`, lines 27703-27950

#### Entry Behavior (`m_cGameModeCount == 0`)

- Display login dialog
- Create input fields:
  - Account name (username)
  - Password (hidden input)
- Initialize mouse interface (4 clickable areas):
  1. Username input field
  2. Password input field
  3. "Connect" button
  4. "Cancel" button
- Set focus to username field
- Initialize gateway connection if `DEF_USING_GATEWAY`

#### Rendering

- Clear screen
- Draw login dialog background
- Draw input field boxes with current text
- Draw password field (asterisks for characters)
- Draw "Connect" and "Cancel" buttons
- Highlight focused element
- Display version information
- Draw mouse cursor

#### Input Processing

| Input | Action |
|-------|--------|
| Text keys | Enter character in focused field |
| TAB | Move to next field |
| Arrow keys | Navigate between fields/buttons |
| ENTER | Submit login (if username and password provided) |
| ESC | Cancel and return to server select |
| Mouse click | Focus field or activate button |

#### Network Setup (on Submit)

```cpp
// Create login socket
m_pLSock = new class XSocket(m_hWnd, DEF_SOCKETBLOCKLIMIT);
m_pLSock->bConnect(m_cLogServerAddr, m_iLogServerPort + (rand() % 1),
                   WM_USER_LOGSOCKETEVENT);
m_pLSock->bInitBufferSize(30000);

// Set connection mode
m_dwConnectMode = MSGID_REQUEST_LOGIN;  // 0x0001

// Store mode indicator
ZeroMemory(m_cMsg, sizeof(m_cMsg));
strcpy(m_cMsg, "11");  // Login mode

// Transition to connecting state
ChangeGameMode(DEF_GAMEMODE_ONCONNECTING);
```

#### Transitions

| To State | Condition |
|----------|-----------|
| ONCONNECTING (1) | Valid credentials submitted |
| ONSELECTSERVER (20) | Cancel (if `DEF_SELECTSERVER`) |
| ONMAINMENU (0) | Cancel (if no `DEF_SELECTSERVER`) |

#### State Variables

- `m_cAccountName[]`: Username buffer
- `m_cAccountPassword[]`: Password buffer
- `m_cCurFocus`: Selected field/button (1-4)
- `m_cMaxFocus`: Maximum focus (4)
- `m_bIsCheckingGateway`: Gateway auth in progress
- `m_dwConnectMode`: Connection type marker

---

### State 1: ONCONNECTING

> **Purpose**: Display connection progress while establishing server connection

**Source Location**: `Game.cpp`, lines 25608-25718

#### Entry Behavior (`m_cGameModeCount == 0`)

- Initialize animation frame counters
- Set reference time for timeout tracking
- Display "Connecting to Server..." message

#### Update Loop

- Increment animation frame counters
- Rotate spinner through 8 directions
- Calculate elapsed time: `(G_dwGlobalTime - m_dwTime) / 1000`
- Check for ESC key (requires 1+ second wait)
- At 7+ seconds: Show "Press ESC to cancel..." message

#### Rendering

- Clear screen
- Draw connecting dialog (`DEF_SPRID_INTERFACE_ND_GAME4`)
- Display elapsed time: "Connecting to Server... XXX Sec"
- Animated 8-direction spinner icon
- At 7+ seconds: Additional timeout message
- Version information
- Mouse cursor (wait animation, frame 8)

#### Input Processing

| Input | Action |
|-------|--------|
| ESC (after 1 sec) | Cancel connection, close sockets, return to menu |

#### Network Events

Connection success triggers `ConnectionEstablishHandler(DEF_SERVERTYPE_LOG)`:

```cpp
void CGame::ConnectionEstablishHandler(int iServerType)
{
    if (iServerType == DEF_SERVERTYPE_LOG) {
        // Login server connected
        // Send login request packet
        // Transition to ONWAITINGRESPONSE
    }
    else if (iServerType == DEF_SERVERTYPE_GAME) {
        // Game server connected
        // Send initial data request
        // Transition to ONWAITINGINITDATA
    }
}
```

#### Transitions

| To State | Condition |
|----------|-----------|
| ONWAITINGRESPONSE (12) | Login socket connected |
| ONLOGIN (8) | Connection failed |
| ONSELECTSERVER (20) | ESC after 1 second (if `DEF_SELECTSERVER`) |
| ONMAINMENU (0) | ESC after 1 second (if no `DEF_SELECTSERVER`) |

#### State Variables

- `m_cMenuFrame`: Animation frame counter
- `m_cMenuDir`: Spinner direction (1-8)
- `m_cMenuDirCnt`: Direction change counter
- `m_dwTime`: Reference time for elapsed calculation

---

### State 12: ONWAITINGRESPONSE

> **Purpose**: Wait for server response after request submission

**Source Location**: `Game.cpp`, lines 29731-29837

#### Entry Behavior

- Display "Waiting for server response..." dialog
- Initialize timer for timeout tracking

#### Rendering

- Wait dialog with animated indicator
- Elapsed time display
- Timeout warning at extended wait

#### Network Events

- Waits for response packets from login server
- Response type determines next state

#### Transitions

| To State | Condition |
|----------|-----------|
| ONLOGRESMSG (14) | Server response received |
| ONCREATENEWACCOUNT (7) | Retry on error |
| ONSELECTSERVER (20) | Timeout (30+ seconds) |

---

### State 14: ONLOGRESMSG

> **Purpose**: Display login server response messages

**Source Location**: `Game.cpp`, lines 31513-31650

#### Entry Behavior

- Parse response code from server packet
- Determine appropriate message to display

#### Response Codes

| Code Range | Meaning |
|------------|---------|
| 0x0001 | Account doesn't exist |
| 0x0002 | Wrong password |
| 0x0003 | Account locked/banned |
| 0x0004 | Already logged in (force login available) |
| 0x0005 | Server full |
| 0x0006 | Account creation failed |
| 0x0010+ | Success - proceed to character selection |

#### Rendering

- Dialog with centered message text
- "OK" button
- Message varies based on response code

#### Input Processing

| Input | Action |
|-------|--------|
| ENTER | Proceed based on response code |
| Mouse click OK | Proceed based on response code |

#### Transitions

| To State | Condition |
|----------|-----------|
| ONSELECTCHARACTER (10) | Login success (0x0010+) |
| ONQUERYFORCELOGIN (9) | Force login required (0x0004) |
| ONCREATENEWCHARACTER (11) | No characters exist |
| ONLOGIN (8) | Error, retry |
| ONSELECTSERVER (20) | Cancel/back |

---

### State 10: ONSELECTCHARACTER

> **Purpose**: Display and select from available characters

**Source Location**: `Game.cpp`, lines 21539-21950

#### Entry Behavior (`m_cGameModeCount == 0`)

- Load character preview data from `m_pCharList[4]`
- Initialize character preview rendering
- Create mouse interface for character slots

#### Character Data Structure

```cpp
class CCharInfo {
public:
    char m_cName[12];        // Character name
    short m_sLevel;          // Level
    short m_sStr, m_sVit;    // Stats
    short m_sDex, m_sInt;
    short m_sMag, m_sChr;
    char m_cGender;          // Male/Female
    short m_sSkinCol;        // Skin color
    short m_sHairStyle;      // Hair style
    short m_sHairCol;        // Hair color
    short m_sUnderCol;       // Underclothing color
    // Equipment preview data
};
```

#### Rendering

- Character selection dialog
- Up to 4 character slots with:
  - Animated character preview
  - Name and level
  - Equipment display
- Buttons: "Select", "Create New", "Delete", "Cancel"
- Empty slot indicator

#### Input Processing

| Input | Action |
|-------|--------|
| Arrow keys | Navigate between character slots |
| ENTER | Select highlighted character |
| Mouse click character | Select character |
| "Create New" | Create new character |
| "Delete" | Delete selected character |
| "Cancel" | Return to login |

#### On Character Selection

```cpp
// Store selected character
strcpy(m_cPlayerName, m_pCharList[selectedIndex]->m_cName);

// Connect to game server
m_pGSock = new class XSocket(m_hWnd, DEF_SOCKETBLOCKLIMIT);
m_pGSock->bConnect(m_cGameServerAddr, m_iGameServerPort,
                   WM_USER_GAMESOCKETEVENT);

// Set connection mode
m_dwConnectMode = MSGID_REQUEST_ENTERGAME;

// Transition
ChangeGameMode(DEF_GAMEMODE_ONCONNECTING);
```

#### Transitions

| To State | Condition |
|----------|-----------|
| ONCONNECTING (1) | Character selected (to game server) |
| ONCREATENEWCHARACTER (11) | "Create New" clicked |
| ONQUERYDELETECHARACTER (13) | "Delete" clicked |
| ONLOGIN (8) | Cancel |

#### State Variables

- `m_pCharList[4]`: Array of character info objects
- `m_iTotalChar`: Number of characters (0-4)
- Selected character index

---

### State 11: ONCREATENEWCHARACTER

> **Purpose**: Create a new character with customization options

**Source Location**: `Game.cpp`, lines 25923-26100

#### Entry Behavior (`m_cGameModeCount == 0`)

- Display character creation form
- Set default customization values
- Initialize stat point pool

#### Customization Options

| Option | Values |
|--------|--------|
| Name | 1-11 characters |
| Gender | Male (1), Female (2) |
| Hair Style | Multiple options per gender |
| Hair Color | Multiple color palettes |
| Skin Color | Multiple skin tones |
| Underclothing | Multiple color options |

#### Stat Allocation

- **Base Points**: 70 total to distribute
- **Stats**: STR, VIT, DEX, INT, MAG, CHR
- **Minimum**: 10 per stat
- **Maximum**: 14 per stat at creation

#### Rendering

- Character creation dialog
- Animated character preview
- Customization option selectors
- Stat allocation interface with +/- buttons
- Name input field
- "Confirm" and "Cancel" buttons

#### Input Processing

| Input | Action |
|-------|--------|
| Text input | Enter character name |
| TAB | Move between options |
| Arrow keys | Adjust current selection |
| +/- buttons | Adjust stat points |
| ENTER | Submit character creation |
| ESC | Cancel |

#### Validation Rules

- Name must be 1-11 characters
- Name cannot contain special characters
- Name cannot be reserved (system names)
- All stat points must be allocated
- Stats must be within min/max bounds

#### On Submission

```cpp
// Build character creation packet
// Send to server
ChangeGameMode(DEF_GAMEMODE_ONWAITINGRESPONSE);
```

#### Transitions

| To State | Condition |
|----------|-----------|
| ONWAITINGRESPONSE (12) | Valid form submitted |
| ONLOGRESMSG (14) | Server response |
| ONSELECTCHARACTER (10) | Cancel |

---

### State 4: ONMAINGAME

> **Purpose**: Main gameplay loop - handles all in-game activity

**Source Location**: `Game.cpp`, lines 34802-48100

This is the **most complex state** comprising over 13,000 lines of code.

#### Entry Behavior (`m_cGameModeCount == 0`)

```cpp
// Start background music
if (m_bSoundFlag) {
    StartBGM();
    LoadBGM(m_cMapName);
}

// Initialize timing
m_dwFPStime = G_dwGlobalTime;
m_sFrameCount = 0;

// Display welcome message for new players
if (m_iLevel < 40) {
    AddEventList("Welcome! Press F1 for help.", 10);
}

// Start calc socket for Korean features
#ifdef DEF_USING_CALCSOCKET
StartCalcSocket();
#endif
```

#### Main Loop Structure

Each frame executes:

```cpp
void CGame::UpdateScreen_OnGame()
{
    // 1. Input Processing
    ProcessMouseInput();
    ProcessKeyboardInput();

    // 2. Game Logic Updates
    UpdatePlayerMovement();
    UpdateCharacterAnimations();
    UpdateEffects();
    UpdateWeather();

    // 3. Network Processing
    ProcessIncomingPackets();
    SendPendingPackets();

    // 4. Camera Updates
    UpdateCameraPosition();

    // 5. Rendering
    RenderBackground();
    RenderTiles();
    RenderCharacters();
    RenderEffects();
    RenderUI();
    RenderChat();
    RenderCursor();

    // 6. Frame Timing
    CalculateFPS();

    // 7. Flip buffers
    m_DDraw.Flip();
}
```

#### Subsystems Active

| Subsystem | Description |
|-----------|-------------|
| **Movement** | 8-directional player and NPC movement |
| **Combat** | Attack commands, damage calculation, PvP |
| **Magic** | 100+ spells, mana management, casting |
| **Skills** | 60 skills, activation, cooldowns |
| **Inventory** | 50 slots, item management, drag-drop |
| **Equipment** | 15 slots, equip/unequip, stat bonuses |
| **Chat** | Multiple channels, filtering, history |
| **Dialogs** | 41 dialog types, modal and modeless |
| **Effects** | 300+ concurrent visual effects |
| **Weather** | 7 weather types, environmental effects |
| **Entities** | Players, NPCs, monsters, items |

#### Dialog System

41 dialog boxes managed via:

```cpp
bool m_bIsDialogEnabled[41];           // Visibility flags
struct DialogBoxInfo m_stDialogBoxInfo[41];  // Dialog state

struct DialogBoxInfo {
    short sX, sY;      // Position
    short sView;       // View mode
    short sV1, sV2;    // View parameters
    short sV3, sV4;
    short sV5, sV6;
    short sV7, sV8;
    short sItemIndex;  // Selected item
    short sMode;       // Dialog mode
    short sSizeX, sSizeY;  // Dimensions
    char cStr[32];     // String data
    char cStr2[32];
    char cStr3[32];
    // Additional fields...
};
```

#### Network Message Processing

**Incoming Messages**:
- `MSGID_RESPONSE_MOTION`: Character movement
- `MSGID_NOTIFY_*`: Status updates (100+ types)
- `MSGID_RESPONSE_MAPDATASTATUS`: Map data
- `MSGID_RESPONSE_ITEMNAME`: Item database

**Outgoing Messages**:
- `MSGID_COMMAND_MOVE`: Player movement
- `MSGID_COMMAND_ATTACK`: Attack command
- `MSGID_COMMAND_MAGIC`: Cast spell
- `MSGID_COMMAND_SKILL`: Use skill
- `MSGID_COMMAND_CHAT`: Send chat message
- 100+ other command types

#### Input Processing

**Keyboard**:
- Arrow keys/WASD: Movement
- Ctrl+Arrow: Attack direction
- F1-F12: Quick slots (spells/items)
- Enter: Toggle chat input
- Tab: Switch dialog focus
- ESC: Close top dialog

**Mouse**:
- Left click: Move, interact, select
- Right click: Attack, context menu
- Double-click: Use item, equip
- Drag: Move items, dialogs

#### Rendering Pipeline

1. Clear back buffer
2. Draw background tiles (based on camera position)
3. Draw tile objects and decorations
4. Sort entities by Y position (painter's algorithm)
5. Draw all characters/monsters/NPCs
6. Draw floating damage numbers
7. Draw particle effects
8. Draw UI dialogs (back to front)
9. Draw chat messages
10. Draw status indicators
11. Draw mouse cursor
12. Flip to front buffer

#### Transitions

| To State | Condition |
|----------|-----------|
| ONCONNECTIONLOST (5) | Network error/disconnect |
| ONQUIT (-1) | Explicit logout or force disconnect timeout |

#### Key State Variables

```cpp
// Player stats
int m_iHP, m_iMP, m_iSP;
int m_iLevel, m_iExp;
int m_iAC, m_iTHAC0;

// Position
short m_sPlayerX, m_sPlayerY;
short m_sViewPointX, m_sViewPointY;

// State flags
bool m_bIsCombatMode;
bool m_bRunningMode;
char m_cPlayerTurn;  // Direction (0-8)

// Map
char m_cMapName[32];
class CMapData* m_pMapData;

// Inventory
class CItem* m_pItemList[50];
class CItem* m_pEquipment[15];

// Magic/Skills
short m_sMagicMastery[100];
short m_sSkillMastery[60];
```

---

### State 5: ONCONNECTIONLOST

> **Purpose**: Handle network disconnection and offer recovery options

**Source Location**: `Game.cpp`, lines 25773-25923

#### Trigger Conditions

- Socket disconnection event
- Socket error event
- Critical network error
- Server-initiated disconnect
- Network timeout

#### Entry Behavior (`m_cGameModeCount == 0`)

```cpp
// Record error condition
// Close sockets if still open
if (m_pGSock != NULL) {
    delete m_pGSock;
    m_pGSock = NULL;
}
if (m_pLSock != NULL) {
    delete m_pLSock;
    m_pLSock = NULL;
}
// Clean up game state
```

#### Rendering

- "Connection Lost" error dialog
- Reason text (if available)
- "Reconnect" button
- "Exit" button

#### Input Processing

| Input | Action |
|-------|--------|
| ENTER / "Reconnect" | Return to login screen |
| "Exit" | Return to main menu |

#### Transitions

| To State | Condition |
|----------|-----------|
| ONLOGIN (8) | Reconnect selected |
| ONMAINMENU (0) | Exit selected |

---

### State 9: ONQUERYFORCELOGIN

> **Purpose**: Confirm force disconnection of existing session

**Source Location**: `Game.cpp`, lines 29333-29400

#### Context

Triggered when another client is already logged in with the same account.

#### Rendering

- Confirmation dialog
- Message: "Another client is using this account. Disconnect existing session?"
- "Yes" and "No" buttons

#### Input Processing

| Input | Action |
|-------|--------|
| ENTER / "Yes" | Force disconnect old session, continue login |
| ESC / "No" | Cancel, return to login |

#### Transitions

| To State | Condition |
|----------|-----------|
| ONSELECTCHARACTER (10) | User confirms force login |
| ONLOGIN (8) | User declines |

---

### State 13: ONQUERYDELETECHARACTER

> **Purpose**: Confirm character deletion

**Source Location**: `Game.cpp`, lines 29837-29900

#### Rendering

- Confirmation dialog
- Character name to be deleted
- Message: "Delete character [NAME]? This cannot be undone."
- Password field (for security, some versions)
- "Yes" and "No" buttons

#### Input Processing

| Input | Action |
|-------|--------|
| ENTER / "Yes" | Send deletion request |
| ESC / "No" | Cancel, return to character selection |

#### On Confirmation

```cpp
// Send character deletion packet
SendDeleteCharacterRequest(characterName);
ChangeGameMode(DEF_GAMEMODE_ONWAITINGRESPONSE);
```

#### Transitions

| To State | Condition |
|----------|-----------|
| ONWAITINGRESPONSE (12) | Deletion request sent |
| ONSELECTCHARACTER (10) | Cancelled |

---

### State 15: ONCHANGEPASSWORD

> **Purpose**: Change account password

**Source Location**: `Game.cpp`, lines 32673-33050

#### Rendering

- Password change dialog
- Input fields:
  - Current password
  - New password
  - Confirm new password
- "Submit" and "Cancel" buttons

#### Validation

- Current password must be correct
- New passwords must match
- Minimum password length
- Cannot be blank

#### Transitions

| To State | Condition |
|----------|-----------|
| ONWAITINGRESPONSE (12) | Valid form submitted |
| ONLOGIN (8) | Cancel or error |

---

### State 6: ONMSG

> **Purpose**: Display generic modal messages

**Source Location**: `Game.cpp`, lines 15456-15510

#### Entry Behavior

- Display message from `m_cMsg` buffer
- Initialize OK button (or OK/Cancel)

#### Common Messages

- "Character created successfully"
- "Account creation failed"
- "Item cannot be used here"
- "You must be in town to use this"
- Quest completion messages
- Error notifications

#### Rendering

- Modal dialog with centered text
- Message content from `m_cMsg`
- OK button (always)
- Cancel button (context-dependent)

#### Input Processing

| Input | Action |
|-------|--------|
| ENTER / OK | Dismiss, proceed |
| ESC / Cancel | Dismiss without action |

#### Transitions

Varies by context - stored in `m_cMsgBoxBtnMode`

---

### State 17: ONVERSIONNOTMATCH

> **Purpose**: Display version mismatch error

**Source Location**: `Game.cpp`, lines 33650-34800

#### Trigger

Server version does not match client version during login response.

#### Rendering

- Error dialog
- Expected version number
- Actual version number
- Instructions to download update
- "OK" button

#### Transitions

| To State | Condition |
|----------|-----------|
| ONLOGIN (8) | OK clicked |

---

### State 19: ONAGREEMENT

> **Purpose**: Display user agreement / terms of service

**Source Location**: `Game.cpp`, lines 26509-26650

**Prerequisite**: `DEF_MAKE_ACCOUNT` must be defined

#### Rendering

- Agreement text (scrollable)
- "Accept" and "Decline" buttons

#### Input Processing

| Input | Action |
|-------|--------|
| Scroll keys | Navigate agreement text |
| "Accept" | Proceed to account creation |
| "Decline" / ESC | Return to menu |

#### Transitions

| To State | Condition |
|----------|-----------|
| ONCREATENEWACCOUNT (7) | Accept clicked |
| ONSELECTSERVER (20) | Decline (if `DEF_SELECTSERVER`) |
| ONMAINMENU (0) | Decline (if no `DEF_SELECTSERVER`) |

---

### State 7: ONCREATENEWACCOUNT

> **Purpose**: Create new game account

**Source Location**: `Game.cpp`, lines 26635-27150

#### Rendering

- Account creation form
- Input fields (vary by region):
  - Account name
  - Password
  - Confirm password
  - Email (optional)
  - Security question (optional)
  - Country/region (language-dependent)

#### Validation

- Minimum password length
- Password confirmation match
- Valid email format (if required)
- Character restrictions

#### Transitions

| To State | Condition |
|----------|-----------|
| ONWAITINGRESPONSE (12) | Valid form submitted |
| ONSELECTSERVER (20) | Cancel |
| ONLOGRESMSG (14) | Server response |

---

### State 3: ONWAITINGINITDATA

> **Purpose**: Wait for initial game data after connecting to game server

**Source Location**: `Game.cpp`, lines 25720-25771

#### Context

After selecting a character and connecting to game server, wait for:
- Map data
- Character data
- Nearby entity data
- Initial inventory state

#### Rendering

- "Waiting for response..." dialog
- Elapsed time display
- Timeout warning at 7+ seconds

#### Input Processing

| Input | Action |
|-------|--------|
| ESC (after 7 sec) | Cancel, return to menu |

#### Network Events

Waits for `MSGID_RESPONSE_INITDATA` from game server.

#### Transitions

| To State | Condition |
|----------|-----------|
| ONMAINGAME (4) | Initial data received |
| ONCONNECTIONLOST (5) | Connection error/timeout |
| ONMAINMENU (0) | ESC after timeout |

---

### State 21: ONINPUTKEYCODE

> **Purpose**: Input authentication key code (China only)

**Source Location**: `Game.cpp`, lines 48096-48300

**Prerequisite**: `DEF_LANGUAGE == 2` (Chinese)

#### Purpose

China client authentication system requiring keycode input.

#### Rendering

- Keycode input dialog
- Numeric input field

#### Input Processing

- Numeric keys only
- Submit via ENTER

#### Transitions

| To State | Condition |
|----------|-----------|
| ONCONNECTING (1) | Valid keycode |
| ONLOGIN (8) | Invalid keycode |

---

### State -1: ONQUIT

> **Purpose**: Graceful application shutdown

**Source Location**: `Game.cpp`, lines 29256-29330

#### Entry Behavior (`m_cGameModeCount == 0`)

- Display quit confirmation dialog
- Initialize fade-out animation
- Set frame countdown (typically 120 frames)

#### Update Loop

- Render quit dialog
- Apply fade effect overlay
- Decrement frame counter
- On completion: Signal application exit

#### On Exit

```cpp
m_bIsProgramActive = FALSE;
// Triggers WM_CLOSE handling
```

#### Transitions

| To State | Condition |
|----------|-----------|
| (Exit) | Animation complete |
| (Previous) | Cancel (if available) |

---

### State -2: NULL

> **Purpose**: Cleanup state marker

Used internally during shutdown to indicate state machine should not process.

---

## State Variables

### Primary State Control

| Variable | Type | Description |
|----------|------|-------------|
| `m_cGameMode` | `char` | Current game mode (-2 to 21) |
| `m_cGameModeCount` | `char` | Frame counter since state entry (caps at 100) |
| `m_dwTime` | `DWORD` | Reference time when mode changed |

### Time Tracking

| Variable | Type | Description |
|----------|------|-------------|
| `G_dwGlobalTime` | `DWORD` | Global game clock (milliseconds) |
| `m_dwCurTime` | `DWORD` | Current frame time |
| `m_dwCheckConnTime` | `DWORD` | Connection check timestamp |
| `m_dwCheckSprTime` | `DWORD` | Sprite refresh timestamp |
| `m_dwCheckChatTime` | `DWORD` | Chat update timestamp |
| `m_dwFPStime` | `DWORD` | FPS calculation reference |

### Network State

| Variable | Type | Description |
|----------|------|-------------|
| `m_pGSock` | `XSocket*` | Game server socket |
| `m_pLSock` | `XSocket*` | Login server socket |
| `m_dwConnectMode` | `DWORD` | Current connection request type |
| `m_cMsg[]` | `char[64]` | Mode-specific message buffer |

### Input State

| Variable | Type | Description |
|----------|------|-------------|
| `m_bEnterPressed` | `bool` | Enter key pressed flag |
| `m_bEscPressed` | `bool` | Escape key pressed flag |
| `m_cArrowPressed` | `char` | Arrow direction (0=none, 1-4=directions) |
| `m_bCtrlPressed` | `bool` | Ctrl key held flag |
| `m_bInputStatus` | `bool` | Text input active flag |

### UI Focus

| Variable | Type | Description |
|----------|------|-------------|
| `m_cCurFocus` | `char` | Currently focused UI element |
| `m_cMaxFocus` | `char` | Maximum focus value for current state |
| `m_bIsDialogEnabled[41]` | `bool[]` | Dialog visibility flags |
| `m_stDialogBoxInfo[41]` | `struct[]` | Dialog state data |

### Player Data

| Variable | Type | Description |
|----------|------|-------------|
| `m_cPlayerName[]` | `char[12]` | Current character name |
| `m_cAccountName[]` | `char[12]` | Account login name |
| `m_cAccountPassword[]` | `char[12]` | Account password |
| `m_cWorldServerName[]` | `char[32]` | Selected game server |

---

## Network Socket Events

### Window Messages

| Message | Handler | Purpose |
|---------|---------|---------|
| `WM_USER_GAMESOCKETEVENT` | `OnGameSocketEvent()` | Game server events |
| `WM_USER_LOGSOCKETEVENT` | `OnLogSocketEvent()` | Login server events |

### Socket Event Types

```cpp
#define DEF_XSOCKEVENT_CONNECTIONESTABLISH   1  // Connection successful
#define DEF_XSOCKEVENT_READCOMPLETE          2  // Data received
#define DEF_XSOCKEVENT_SOCKETCLOSED          3  // Connection closed
#define DEF_XSOCKEVENT_SOCKETERROR           4  // Socket error
#define DEF_XSOCKEVENT_CRITICALERROR         5  // Unrecoverable error
```

### Event Handling

```cpp
void CGame::OnGameSocketEvent(WPARAM wParam, LPARAM lParam)
{
    switch (WSAGETSELECTEVENT(lParam)) {
        case FD_CONNECT:
            if (WSAGETSELECTERROR(lParam) == 0)
                ConnectionEstablishHandler(DEF_SERVERTYPE_GAME);
            else
                ChangeGameMode(DEF_GAMEMODE_ONCONNECTIONLOST);
            break;

        case FD_READ:
            ReadGameServerData();
            break;

        case FD_CLOSE:
            ChangeGameMode(DEF_GAMEMODE_ONCONNECTIONLOST);
            break;
    }
}
```

---

## Timeout Behaviors

| State | Timeout | Behavior |
|-------|---------|----------|
| ONCONNECTING | 7 seconds | Shows "Press ESC to cancel" message |
| ONWAITINGINITDATA | 7 seconds | Shows timeout warning, enables ESC |
| ONWAITINGRESPONSE | 30 seconds | Returns to server selection |
| ONLOADING | Varies | Can ESC after 1 second |
| ONMAINGAME | None | Network-driven (server timeout) |
| Menu states | Infinite | User-driven |

---

## Error States & Recovery

### Connection Lost Recovery Flow

```
Socket Error/Disconnect
         │
         ▼
┌─────────────────────┐
│ ONCONNECTIONLOST    │
│ - Close sockets     │
│ - Clean game state  │
└─────────┬───────────┘
          │
    ┌─────┴─────┐
    │           │
    ▼           ▼
"Reconnect"   "Exit"
    │           │
    ▼           ▼
ONLOGIN    ONMAINMENU
```

### Version Mismatch Recovery

```
Login Response (Version Error)
         │
         ▼
┌─────────────────────┐
│ ONVERSIONNOTMATCH   │
│ - Show versions     │
│ - Update prompt     │
└─────────┬───────────┘
          │
          ▼
        "OK"
          │
          ▼
      ONLOGIN
```

### Force Login Recovery

```
Login Response (Already Logged In)
         │
         ▼
┌─────────────────────┐
│ ONQUERYFORCELOGIN   │
│ - Confirm dialog    │
└─────────┬───────────┘
          │
    ┌─────┴─────┐
    │           │
   "Yes"       "No"
    │           │
    ▼           ▼
ONSELECTCHAR  ONLOGIN
```

---

## Conditional Compilation

### DEF_SELECTSERVER

Enables server selection screen.

**When Defined**:
- ONSELECTSERVER state is accessible
- Main menu "New Game" → ONSELECTSERVER

**When Not Defined**:
- ONSELECTSERVER redirects to ONLOGIN
- Default server "WS1" is used
- Skip server selection entirely

### DEF_MAKE_ACCOUNT

Enables in-client account creation.

**When Defined**:
- ONAGREEMENT and ONCREATENEWACCOUNT states are accessible
- Main menu shows "Create Account" option

**When Not Defined**:
- "Create Account" opens web browser to registration page

### DEF_USING_GATEWAY

Enables gateway authentication system.

**When Defined**:
- Additional gateway check during ONLOGIN
- `m_bIsCheckingGateway` flag used
- ISP-specific authentication (Korea)

### DEF_LANGUAGE

Enables language-specific features.

| Value | Language | Special Features |
|-------|----------|------------------|
| 0 | English | None |
| 1 | Korean | Calc socket, gateway |
| 2 | Chinese | ONINPUTKEYCODE state |
| 3 | Japanese | None |
| 4 | Taiwanese | None |

---

## Startup Sequence

### Application Launch to Main Menu

```
1. WinMain()
   │
   ├─► Initialize Windows application
   ├─► Create main window
   ├─► Create CGame instance
   │     └─► Constructor sets m_cGameMode = DEF_GAMEMODE_ONLOADING
   │
   ├─► CGame::bInit()
   │     ├─► Initialize DirectDraw (graphics)
   │     ├─► Initialize DirectInput (keyboard/mouse)
   │     ├─► Initialize DirectSound (audio)
   │     ├─► Load magic configuration
   │     ├─► Load skill configuration
   │     ├─► Load item names
   │     ├─► Load game messages
   │     └─► Load bad word filter
   │
   └─► EventLoop()
         │
         └─► [ONLOADING state begins]
               │
               ├─► Load interface sprites (stage 0)
               ├─► Load tile sprites (stage 4)
               ├─► Load map sprites (stage 8)
               ├─► Load item sprites (stage 12)
               ├─► Load equipment sprites (stage 16)
               │
               └─► ChangeGameMode(DEF_GAMEMODE_ONMAINMENU)
                     │
                     └─► [ONMAINMENU state - user ready]
```

### Login to Gameplay

```
ONMAINMENU
    │ "New Game" clicked
    ▼
ONSELECTSERVER (or skip if not defined)
    │ Server selected
    ▼
ONLOGIN
    │ Credentials submitted
    ▼
ONCONNECTING (to login server)
    │ Socket connected
    ▼
ONWAITINGRESPONSE
    │ Login response received
    ▼
ONLOGRESMSG
    │ Success (0x0010+)
    ▼
ONSELECTCHARACTER
    │ Character selected
    ▼
ONCONNECTING (to game server)
    │ Socket connected
    ▼
ONWAITINGINITDATA
    │ Map/character data received
    ▼
ONMAINGAME
    │
    └─► [Gameplay begins]
```

---

## Shutdown Sequence

### Graceful Shutdown from Gameplay

Located in `Wmain.cpp`, lines 388-400:

```
WM_CLOSE message received
         │
         ▼
Is m_cGameMode == ONMAINGAME?
         │
    ┌────┴────┐
   Yes       No
    │         │
    ▼         ▼
Send logout   Skip
request
    │
    ▼
Wait up to 11 seconds for graceful disconnect
         │
         ▼
WM_DESTROY received
         │
         ▼
┌─────────────────────────────┐
│ OnDestroy()                 │
│ - Stop timer                │
│ - G_pGame->Quit()           │
│   - Close all sockets       │
│   - Release DirectDraw      │
│   - Release DirectInput     │
│   - Release DirectSound     │
│   - Free all memory         │
│ - WSACleanup()              │
│ - Open homepage (optional)  │
│ - PostQuitMessage(0)        │
└─────────────────────────────┘
         │
         ▼
Application terminates
```

### CGame::Quit() Cleanup

```cpp
void CGame::Quit()
{
    // Set null state
    m_cGameMode = DEF_GAMEMODE_NULL;

    // Close network sockets
    if (m_pGSock != NULL) {
        delete m_pGSock;
        m_pGSock = NULL;
    }
    if (m_pLSock != NULL) {
        delete m_pLSock;
        m_pLSock = NULL;
    }

    // Release DirectX resources
    m_DDraw.Quit();
    m_DInput.Quit();
    m_DSound.Quit();

    // Free sprite memory
    for (int i = 0; i < DEF_MAXSPRITES; i++) {
        if (m_pSprite[i] != NULL) {
            delete m_pSprite[i];
            m_pSprite[i] = NULL;
        }
    }

    // Free tile memory
    for (int i = 0; i < DEF_MAXTILES; i++) {
        if (m_pTile[i] != NULL) {
            delete m_pTile[i];
            m_pTile[i] = NULL;
        }
    }

    // Free effect sprites
    // Free item list
    // Free character list
    // Free map data
    // etc.
}
```

---

## Key Implementation Notes

### State Entry Pattern

Every state handler follows this pattern:

```cpp
void CGame::UpdateScreen_OnSomeState()
{
    // Entry-once initialization
    if (m_cGameModeCount == 0) {
        // First frame setup
        // Initialize UI elements
        // Reset state variables
        // Create mouse interface
        m_cGameModeCount = 1;
    }

    // Cap frame counter
    if (m_cGameModeCount < 100) m_cGameModeCount++;

    // Update logic
    // ...

    // Render
    // ...

    // Input processing
    // ...

    // State transitions
    if (someCondition) {
        ChangeGameMode(DEF_GAMEMODE_NEXT);
    }
}
```

### Frame Counter Usage

The `m_cGameModeCount` variable serves multiple purposes:

1. **Entry detection**: `== 0` means first frame in state
2. **Animation timing**: Used for fade effects, spinners
3. **Delay enforcement**: Wait X frames before allowing action
4. **Overflow prevention**: Caps at 100

### Time Reference Pattern

```cpp
// On state entry
m_dwTime = G_dwGlobalTime;

// During state
DWORD elapsed = G_dwGlobalTime - m_dwTime;
if (elapsed > 7000) {  // 7 seconds
    // Show timeout message
}
```

### Mouse Interface Pattern

```cpp
// Create clickable regions
m_stMCursor.Init();
m_stMCursor.AddRect(x1, y1, x2, y2);  // Button 1
m_stMCursor.AddRect(x1, y1, x2, y2);  // Button 2
m_stMCursor.AddRect(x1, y1, x2, y2);  // Button 3

// Check for clicks
int clicked = m_stMCursor.CheckClickedRecID();
if (clicked == 1) {
    // Button 1 clicked
}
```

---

## Modernization Considerations

When modernizing this state machine:

1. **Replace char with enum class**: Type-safe state representation
2. **Extract state classes**: Each state becomes a class implementing IGameState
3. **Use state pattern**: Polymorphic state handling
4. **Add state stack**: Support for modal states and pause
5. **Implement state transitions**: Transition objects with enter/exit hooks
6. **Decouple rendering**: Separate render logic from state logic
7. **Add state serialization**: Save/restore state for debugging
8. **Implement async transitions**: Non-blocking state changes
9. **Add state history**: Track state transitions for debugging

### Proposed Modern Interface

```cpp
namespace hb::gameplay {

enum class GameStateType {
    Loading,
    MainMenu,
    ServerSelect,
    Login,
    Connecting,
    // ...
};

class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void update(f32 deltaTime) = 0;
    virtual void render(gfx::IRenderer& renderer) = 0;
    virtual bool handleInput(const input::InputSystem& input) = 0;

    [[nodiscard]] virtual GameStateType type() const = 0;
};

class GameStateMachine {
public:
    void changeState(GameStateType newState);
    void pushState(GameStateType state);  // For modal states
    void popState();

    void update(f32 deltaTime);
    void render(gfx::IRenderer& renderer);

    [[nodiscard]] GameStateType currentState() const;

private:
    std::unordered_map<GameStateType, std::unique_ptr<IGameState>> m_states;
    std::vector<IGameState*> m_stateStack;
};

} // namespace hb::gameplay
```

---

## References

### Source Files

| File | Lines | Content |
|------|-------|---------|
| `Game.h` | 104-126 | State constant definitions |
| `Game.cpp` | 1106-1235 | Main UpdateScreen() dispatcher |
| `Game.cpp` | 15511-15524 | ChangeGameMode() function |
| `Game.cpp` | 3112-3393 | ONMAINMENU handler |
| `Game.cpp` | 3596-4310 | ONLOADING handler |
| `Game.cpp` | 34802-48100 | ONMAINGAME handler |
| `Wmain.cpp` | 279-383 | Event loop |
| `Wmain.cpp` | 388-400 | Shutdown handling |
| `GlobalDef.h` | Various | Compile-time flags |

### Related Documentation

- `01_cgame_monolithic_class.md` - CGame class structure
- `02_global_definitions.md` - Global constants and defines
- `10_xsocket_networking.md` - Network socket implementation
- `11_network_protocol.md` - Protocol message definitions
