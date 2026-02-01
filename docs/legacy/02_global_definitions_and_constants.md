# Legacy Global Definitions and Constants

> **Document Version:** 1.0
> **Last Updated:** 2024
> **Scope:** All compile-time constants, macros, and global definitions in the legacy Helbreath client codebase

---

## Table of Contents

1. [Overview](#overview)
2. [File Organization](#file-organization)
3. [GlobalDef.h - Core Configuration](#globaldefs---core-configuration)
4. [GlobalVal.h - Global Variables](#globalvalh---global-variables)
5. [Game.h - Game Constants](#gameh---game-constants)
6. [NetMessages.h - Network Protocol Constants](#netmessagesh---network-protocol-constants)
7. [ActionID.h - Action and Motion Constants](#actionidh---action-and-motion-constants)
8. [Item.h - Item System Constants](#itemh---item-system-constants)
9. [Magic.h - Magic System Constants](#magich---magic-system-constants)
10. [Skill.h - Skill System Constants](#skillh---skill-system-constants)
11. [SoundID.h - Audio Constants](#soundidh---audio-constants)
12. [SpriteID.h - Sprite Constants](#spriteidh---sprite-constants)
13. [DynamicObjectID.h - Dynamic Object Constants](#dynamicobjectidh---dynamic-object-constants)
14. [XSocket.h - Socket Constants](#xsocketh---socket-constants)
15. [Localization System](#localization-system)
16. [Cross-Reference Tables](#cross-reference-tables)

---

## Overview

The legacy Helbreath client uses a C++98-era approach to configuration and constants, relying heavily on preprocessor `#define` macros. This approach has several characteristics:

- **Compile-time configuration**: Language selection, version numbers, and feature flags are set at compile time
- **No namespaces**: All constants are in the global namespace
- **Magic numbers**: Many hardcoded values throughout the codebase
- **Conditional compilation**: Heavy use of `#ifdef` blocks for regional/language variants

### Key Statistics

| Category | Approximate Count |
|----------|------------------|
| #define macros in NetMessages.h | 250+ |
| #define macros in localization files | 2000+ per language |
| Game mode constants | 21 |
| Dialog box types | 41 |
| Network message types | 200+ |
| Equipment positions | 15 |
| Item types | 12 |
| Magic types | 23 |
| Sound effect IDs | 17 |
| Dynamic object types | 12 |

---

## File Organization

```
client/
├── GlobalDef.h          # Language selection, version, feature flags
├── GlobalVal.h          # Global runtime variables (extern declarations)
├── Game.h               # CGame class + game-wide constants
├── NetMessages.h        # All network protocol message IDs
├── ActionID.h           # Character action/motion constants
├── Item.h               # Item types and equipment positions
├── Magic.h              # Magic type constants
├── Skill.h              # Skill class definition
├── Effect.h             # Visual effect class
├── Tile.h               # Tile data class
├── TileSpr.h            # Tile sprite data
├── MapData.h            # Map data constants
├── CharInfo.h           # Character information class
├── BuildItem.h          # Crafting system class
├── Sprite.h             # Sprite rendering class
├── SpriteID.h           # Sprite index constants
├── SoundID.h            # Sound effect IDs
├── DynamicObjectID.h    # Dynamic object type IDs
├── XSocket.h            # Network socket constants
├── MouseInterface.h     # Mouse input constants
├── DXC_ddraw.h          # DirectDraw wrapper
├── DXC_dinput.h         # DirectInput wrapper
├── lan_eng.h            # English localization strings
├── lan_kor.h            # Korean localization strings
├── lan_chi.h            # Chinese localization strings
├── lan_tai.h            # Taiwanese localization strings
├── lan_jap.h            # Japanese localization strings
├── Curse.h              # Profanity filter class
├── ItemName.h           # Item name database class
├── GameMonitor.h        # Bad word monitoring class
└── Cint.h               # Atomic integer wrapper
```

---

## GlobalDef.h - Core Configuration

### Purpose
`GlobalDef.h` is the central configuration header that controls:
- Target language/region
- Client version numbers
- Feature flags
- Conditional compilation for regional variants

### Language Selection

```cpp
#define DEF_LANGUAGE 4  // Current selection

// Available options:
// #define DEF_TAIWAN   1
// #define DEF_CHINESE  2
// #define DEF_KOREAN   3
// #define DEF_ENGLISH  4
// #define DEF_JAPANESE 5
```

**Selection Logic:**
```cpp
#ifndef DEF_LANGUAGE
#define DEF_LANGUAGE 3      // Default to Korean if not defined
#endif
#if DEF_LANGUAGE > 5
#define DEF_LANGUAGE 3      // Reset to Korean if invalid
#endif
```

### Version Numbers

Version numbers are language-dependent:

| Language | Upper Version | Lower Version | Full Version |
|----------|---------------|---------------|--------------|
| Taiwan (1) | 2 | 19 | 2.19 |
| China (2) | 2 | 19 | 2.19 |
| Korea (3) | 2 | 20 | 2.20 |
| English (4) | 2 | 20 | 2.20 |
| Japan (5) | 2 | 20 | 2.20 |

```cpp
#define DEF_UPPERVERSION  2
#define DEF_LOWERVERSION  20  // For English
```

### Feature Flags by Region

#### Taiwan (DEF_LANGUAGE == 1)
```cpp
#define DEF_USING_WIN_IME    // Windows IME support for Chinese input
#define DEF_ENGLISHITEM      // Use English item names
#define DEF_USING_GATEWAY    // Connect through gateway server
#define DEF_SHORTCUT         // Enable shortcut keys
#define DEF_GIZON            // Enable Gizon item upgrade currency
```

#### China (DEF_LANGUAGE == 2)
```cpp
#define DEF_USING_WIN_IME    // Windows IME support
#define DEF_ENGLISHITEM      // English item names
#define DEF_SELECTSERVER     // Server selection screen
#define DEF_MAKE_ACCOUNT     // In-client account creation
#define DEF_SHORTCUT         // Enable shortcut keys
#define DEF_GIZON            // Gizon currency
#define DEF_FEEDBACKCARD     // Feedback system
#define DEF_HTMLCOMMOM       // HTML integration
```

#### Korea (DEF_LANGUAGE == 3)
```cpp
#define DEF_SELECTSERVER     // Server selection screen
#define DEF_GIZON            // Gizon currency
```

#### English (DEF_LANGUAGE == 4)
```cpp
#define DEF_ENGLISHITEM      // English item names
#define DEF_SELECTSERVER     // Server selection screen
```

#### Japan (DEF_LANGUAGE == 5)
```cpp
#define DEF_USING_WIN_IME    // Windows IME support
#define DEF_SELECTSERVER     // Server selection
#define DEF_ENGLISHITEM      // English item names
// Terra-specific:
#ifdef DEF_JAPAN_FOR_TERRA
  #define DEF_ACCOUNTLONG    // Longer account names (16 chars)
  #define DEF_ACCOUNTLEN 16  // Account name length
#endif
```

### Development/Debug Flags

```cpp
// Uncomment to enable:
//#define DEF_COLOR           // Color testing
//#define DEF_XMAS            // Christmas event content
//#define DEF_TESTSERVER      // Test server mode (disables server select)
//#define DEF_JAPAN_FOR_TERRA // Terra Japan specific
//#define DEF_FUCK_USA        // US-specific version (disables server select)
//#define DEF_SHOWCURSORPOS   // Debug cursor position display
```

### Debug Mode Overrides

```cpp
#ifdef DEF_TESTSERVER
#undef DEF_SELECTSERVER      // Test server doesn't need selection
#endif

#ifdef DEF_FUCK_USA
#define DEF_ENGLISHITEM
#undef DEF_SELECTSERVER
#endif

#ifdef _DEBUG
#undef DEF_USING_GATEWAY     // Don't use gateway in debug
#endif
```

### Version History (from comments)

The file contains an extensive Korean changelog documenting versions from v2.13 through v2.20:

| Version | Date | Notable Changes |
|---------|------|-----------------|
| v2.20 | 2002-12 | Monster event positioning |
| v2.19 | 2002-11/12 | China HTML integration, agriculture skills |
| v2.18 | 2002-08/10 | Item drop history, loading optimization |
| v2.17 | 2002-07 | Refactoring, party system, force recall timer |
| v2.16 | 2002-07 | Dialog improvements |
| v2.15 | 2002-03/05 | Server selection, new UI, mouse wheel support |
| v2.14 | 2002-02 | Taiwan IME improvements |

---

## GlobalVal.h - Global Variables

### Purpose
Declares global runtime variables shared across the application.

### Defined Variables

```cpp
int   G_iLevel;              // Player level (debug display)
char  G_cWorldServerName[32]; // Current world server name
char  G_cPlayerName[12];      // Current player character name
char  G_cMapName[11];         // Current map name
char  G_cMapMessage[30];      // Current map display message
```

### Debug-Only Variables

```cpp
#ifdef _DEBUG
int   G_iColor = 0;          // Debug color index
int   G_iColorMax = 15;      // Maximum color index for cycling
#endif
```

---

## Game.h - Game Constants

### Resource Limits

These define the maximum capacity of various game systems:

```cpp
// Sprite and Tile Limits
#define DEF_MAXSPRITES          20000   // Maximum loaded sprites
#define DEF_MAXTILES            500     // Maximum tile sprite types
#define DEF_MAXEFFECTSPR        100     // Maximum effect sprite types

// Audio Limits
#define DEF_MAXSOUNDEFFECTS     110     // Concurrent sound effects

// Chat System
#define DEF_MAXCHATMSGS         500     // Total chat message history
#define DEF_MAXWHISPERMSG       5       // Whisper message buffer
#define DEF_MAXCHATSCROLLMSGS   80      // Scrollable chat messages
#define DEF_CHATTIMEOUT_A       4000    // Primary chat timeout (ms)
#define DEF_CHATTIMEOUT_B       500     // Secondary chat timeout (ms)
#define DEF_CHATTIMEOUT_C       2000    // Tertiary chat timeout (ms)

// Inventory and Items
#define DEF_MAXITEMS            50      // Inventory capacity
#define DEF_MAXBANKITEMS        121     // Bank storage (120+1)
#define DEF_MAXMENUITEMS        140     // Shop menu items (was 120)
#define DEF_MAXSELLLIST         12      // Items in sell list

// Effects and Weather
#define DEF_MAXEFFECTS          300     // Concurrent visual effects (was 600)
#define DEF_MAXWHETHEROBJECTS   600     // Weather particle objects

// Game Data
#define DEF_MAXMAGICTYPE        100     // Magic spell types
#define DEF_MAXSKILLTYPE        60      // Skill types
#define DEF_MAXBUILDITEMS       100     // Craftable item recipes
#define DEF_MAXGAMEMSGS         300     // Game message buffer
#define DEF_MAXITEMNAMES        1000    // Item name database size
#define DEF_TEXTDLGMAXLINES     300     // Text dialog lines (was 3000)

// Social Systems
#define DEF_MAXGUILDSMAN        32      // Guild member limit
#define DEF_MAXGUILDNAMES       100     // Cached guild names
#define DEF_MAXPARTYMEMBERS     8       // Party size limit

// War System
#define DEF_MAXCRUSADESTRUCTURES 300    // Crusade structures
```

### UI Button Dimensions (v2.18)

```cpp
#define DEF_BTNSZX              74      // Button width
#define DEF_BTNSZY              20      // Button height
#define DEF_LBTNPOSX            30      // Left button X position
#define DEF_RBTNPOSX            154     // Right button X position
#define DEF_BTNPOSY             292     // Button Y position
```

### Message Buffer Indices

```cpp
#define DEF_INDEX4_MSGID        0       // Message ID offset in packet
#define DEF_INDEX2_MSGTYPE      4       // Message type offset
#define DEF_SOCKETBLOCKLIMIT    300     // Socket blocking limit
```

### Windows Message IDs

```cpp
#define WM_USER_GAMESOCKETEVENT  (WM_USER + 2000)  // Game socket events
#define WM_USER_LOGSOCKETEVENT   (WM_USER + 2001)  // Login socket events
```

### Game Mode Constants

The game operates as a state machine with these modes:

```cpp
#define DEF_GAMEMODE_NULL                   -2   // Uninitialized
#define DEF_GAMEMODE_ONQUIT                 -1   // Quitting
#define DEF_GAMEMODE_ONMAINMENU             0    // Main menu
#define DEF_GAMEMODE_ONCONNECTING           1    // Connecting to server
#define DEF_GAMEMODE_ONLOADING              2    // Loading game data
#define DEF_GAMEMODE_ONWAITINGINITDATA      3    // Waiting for init data
#define DEF_GAMEMODE_ONMAINGAME             4    // In-game (playing)
#define DEF_GAMEMODE_ONCONNECTIONLOST       5    // Connection lost
#define DEF_GAMEMODE_ONMSG                  6    // Displaying message
#define DEF_GAMEMODE_ONCREATENEWACCOUNT     7    // Creating account
#define DEF_GAMEMODE_ONLOGIN                8    // Login screen
#define DEF_GAMEMODE_ONQUERYFORCELOGIN      9    // Force login prompt
#define DEF_GAMEMODE_ONSELECTCHARACTER      10   // Character selection
#define DEF_GAMEMODE_ONCREATENEWCHARACTER   11   // Character creation
#define DEF_GAMEMODE_ONWAITINGRESPONSE      12   // Waiting for server
#define DEF_GAMEMODE_ONQUERYDELETECHARACTER 13   // Delete confirmation
#define DEF_GAMEMODE_ONLOGRESMSG            14   // Login response message
#define DEF_GAMEMODE_ONCHANGEPASSWORD       15   // Password change
// Note: 16 is skipped
#define DEF_GAMEMODE_ONVERSIONNOTMATCH      17   // Version mismatch
#define DEF_GAMEMODE_ONINTRODUCTION         18   // Introduction/tutorial
#define DEF_GAMEMODE_ONAGREEMENT            19   // User agreement
#define DEF_GAMEMODE_ONSELECTSERVER         20   // Server selection
#define DEF_GAMEMODE_ONINPUTKEYCODE         21   // Key code input (China)
```

### Server Type Constants

```cpp
#define DEF_SERVERTYPE_GAME     1        // Game server connection
#define DEF_SERVERTYPE_LOG      2        // Login server connection
```

### Cursor Status Constants

```cpp
#define DEF_CURSORSTATUS_NULL       0    // No special status
#define DEF_CURSORSTATUS_PRESSED    1    // Mouse button pressed
#define DEF_CURSORSTATUS_SELECTED   2    // Object selected
#define DEF_CURSORSTATUS_DRAGGING   3    // Dragging operation
```

### Selected Object Types

```cpp
#define DEF_SELECTEDOBJTYPE_DLGBOX  1    // Dialog box selected
#define DEF_SELECTEDOBJTYPE_ITEM    2    // Item selected
```

### Input Timing

```cpp
#define DEF_DOUBLECLICKTIME     300      // Double-click threshold (ms)
```

---

## NetMessages.h - Network Protocol Constants

### Purpose
Defines all message IDs for client-server communication. This is the most extensive constant file, containing 250+ definitions.

### Message Type Confirmation Codes

```cpp
#define DEF_MSGTYPE_CONFIRM     0x0F14   // Generic confirmation
#define DEF_MSGTYPE_REJECT      0x0F15   // Generic rejection
```

### Core Message IDs

#### Player Initialization
```cpp
#define MSGID_REQUEST_INITPLAYER    0x05040205  // Request player init
#define MSGID_RESPONSE_INITPLAYER   0x05040206  // Response player init
#define MSGID_REQUEST_INITDATA      0x05080404  // Request initial data
#define MSGID_RESPONSE_INITDATA     0x05080405  // Response initial data
```

#### Motion and Events
```cpp
#define MSGID_COMMAND_MOTION        0x0FA314D5  // Motion command
#define MSGID_RESPONSE_MOTION       0x0FA314D6  // Motion response
#define MSGID_EVENT_MOTION          0x0FA314D7  // Motion event
#define MSGID_EVENT_LOG             0x0FA314D8  // Log event
#define MSGID_EVENT_COMMON          0x0FA314DB  // Common event
#define MSGID_COMMAND_COMMON        0x0FA314DC  // Common command
```

### Common Command Types (DEF_COMMONTYPE_*)

These are sub-commands within MSGID_COMMAND_COMMON:

```cpp
// Item Operations
#define DEF_COMMONTYPE_ITEMDROP                 0x0A01  // Drop item
#define DEF_COMMONTYPE_EQUIPITEM                0x0A02  // Equip item
#define DEF_COMMONTYPE_RELEASEITEM              0x0A0A  // Unequip item
#define DEF_COMMONTYPE_SETITEM                  0x0A0C  // Set item position
#define DEF_COMMONTYPE_REQ_USEITEM              0x0A11  // Use item request
#define DEF_COMMONTYPE_REQ_SELLITEM             0x0A13  // Sell item request
#define DEF_COMMONTYPE_REQ_REPAIRITEM           0x0A14  // Repair item request
#define DEF_COMMONTYPE_REQ_SELLITEMCONFIRM      0x0A15  // Confirm sell
#define DEF_COMMONTYPE_REQ_REPAIRITEMCONFIRM    0x0A16  // Confirm repair

// Shop Operations
#define DEF_COMMONTYPE_REQ_LISTCONTENTS         0x0A03  // List shop contents
#define DEF_COMMONTYPE_REQ_PURCHASEITEM         0x0A04  // Purchase item

// Character Interactions
#define DEF_COMMONTYPE_GIVEITEMTOCHAR           0x0A05  // Give item to player
#define DEF_COMMONTYPE_EXCHANGEITEMTOCHAR       0x0A1E  // Exchange items
#define DEF_COMMONTYPE_SETEXCHANGEITEM          0x0A1F  // Set exchange item
#define DEF_COMMONTYPE_CONFIRMEXCHANGEITEM      0x0A20  // Confirm exchange
#define DEF_COMMONTYPE_CANCELEXCHANGEITEM       0x0A21  // Cancel exchange

// Guild Operations
#define DEF_COMMONTYPE_JOINGUILDAPPROVE         0x0A06  // Approve join request
#define DEF_COMMONTYPE_JOINGUILDREJECT          0x0A07  // Reject join request
#define DEF_COMMONTYPE_DISMISSGUILDAPPROVE      0x0A08  // Approve dismissal
#define DEF_COMMONTYPE_DISMISSGUILDREJECT       0x0A09  // Reject dismissal
#define DEF_COMMONTYPE_CLEARGUILDNAME           0x0A25  // Clear guild name
#define DEF_COMMONTYPE_BANGUILD                 0x0A26  // Ban from guild
#define DEF_COMMONTYPE_REQGUILDNAME             0x0A59  // Request guild name

// Combat Modes
#define DEF_COMMONTYPE_TOGGLECOMBATMODE         0x0A0B  // Toggle combat
#define DEF_COMMONTYPE_TOGGLESAFEATTACKMODE     0x0A18  // Toggle safe attack

// Magic and Skills
#define DEF_COMMONTYPE_MAGIC                    0x0A0D  // Cast magic
#define DEF_COMMONTYPE_REQ_STUDYMAGIC           0x0A0E  // Study magic request
#define DEF_COMMONTYPE_REQ_TRAINSKILL           0x0A0F  // Train skill request
#define DEF_COMMONTYPE_REQ_USESKILL             0x0A12  // Use skill request
#define DEF_COMMONTYPE_REQ_SETDOWNSKILLINDEX    0x0A1B  // Set skill index
#define DEF_COMMONTYPE_GETMAGICABILITY          0x0A24  // Get magic ability

// Rewards and Currency
#define DEF_COMMONTYPE_REQ_GETREWARDMONEY       0x0A10  // Get reward money

// Fishing and Crafting
#define DEF_COMMONTYPE_REQ_GETFISHTHISTIME      0x0A17  // Fishing attempt
#define DEF_COMMONTYPE_REQ_CREATEPORTION        0x0A19  // Create potion
#define DEF_COMMONTYPE_BUILDITEM                0x0A23  // Build/craft item

// NPC Interaction
#define DEF_COMMONTYPE_TALKTONPC                0x0A1A  // Talk to NPC

// Quest System
#define DEF_COMMONTYPE_QUESTACCEPTED            0x0A22  // Accept quest
#define DEF_COMMONTYPE_REQUEST_CANCELQUEST      0x0A50  // Cancel quest

// Party System
#define DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY  0x0A30  // Accept party invite
#define DEF_COMMONTYPE_REQUEST_JOINPARTY        0x0A31  // Request join party
#define DEF_COMMONTYPE_RESPONSE_JOINPARTY       0x0A32  // Party join response

// Special Abilities
#define DEF_COMMONTYPE_REQUEST_ACTIVATESPECABLTY 0x0A40 // Activate ability

// Crusade/War System
#define DEF_COMMONTYPE_REQ_GETOCCUPYFLAG        0x0A1C  // Get occupy flag
#define DEF_COMMONTYPE_REQ_GETHEROMANTLE        0x0A1D  // Get hero mantle
#define DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY 0x0A51 // Select crusade duty
#define DEF_COMMONTYPE_REQUEST_MAPSTATUS        0x0A52  // Request map status
#define DEF_COMMONTYPE_REQUEST_HELP             0x0A53  // Request help
#define DEF_COMMONTYPE_SETGUILDTELEPORTLOC      0x0A54  // Set guild teleport
#define DEF_COMMONTYPE_GUILDTELEPORT            0x0A55  // Guild teleport
#define DEF_COMMONTYPE_SUMMONWARUNIT            0x0A56  // Summon war unit
#define DEF_COMMONTYPE_SETGUILDCONSTRUCTLOC     0x0A57  // Set construct loc

// Item Upgrade
#define DEF_COMMONTYPE_UPGRADEITEM              0x0A58  // Upgrade item

// Fight Zone
#define DEF_COMMONTYPE_REQ_GETOCCUPYFIGHTZONETICKET 0x0A25 // Arena ticket

// Hunter Mode
#define DEF_COMMONTYPE_REQUEST_HUNTMODE         0x0A60  // Toggle hunt mode
```

### Notification Types (DEF_NOTIFY_*)

Server-to-client notifications:

```cpp
// Item Notifications
#define DEF_NOTIFY_ITEMOBTAINED             0x0B01  // Item received
#define DEF_NOTIFY_CANNOTCARRYMOREITEM      0x0B05  // Inventory full
#define DEF_NOTIFY_ITEMPURCHASED            0x0B06  // Item purchased
#define DEF_NOTIFY_ITEMLIFESPANEND          0x0B17  // Item expired
#define DEF_NOTIFY_ITEMTOBANK               0x0B19  // Item to bank
#define DEF_NOTIFY_CANNOTITEMTOBANK         0x0B26  // Bank full
#define DEF_NOTIFY_SETITEMCOUNT             0x0B25  // Item count changed
#define DEF_NOTIFY_ITEMDEPLETED_ERASEITEM   0x0B20  // Item depleted
#define DEF_NOTIFY_GIVEITEMFIN_ERASEITEM    0x0B1D  // Given item erased
#define DEF_NOTIFY_DROPITEMFIN_ERASEITEM    0x0B1F  // Dropped item erased
#define DEF_NOTIFY_ITEMRELEASED             0x0B5C  // Item unequipped
#define DEF_NOTIFY_ITEMCOLORCHANGE          0x0B65  // Item color changed
#define DEF_NOTIFY_ITEMATTRIBUTECHANGE      0x0BA3  // Item attr changed
#define DEF_NOTIFY_ITEMPOSLIST              0x0B5B  // Item position list
#define DEF_NOTIFY_ITEMREPAIRED             0x0B30  // Item repaired
#define DEF_NOTIFY_ITEMSOLD                 0x0B31  // Item sold

// Shop Notifications
#define DEF_NOTIFY_SELLITEMPRICE            0x0B2D  // Sell price shown
#define DEF_NOTIFY_CANNOTSELLITEM           0x0B2C  // Cannot sell
#define DEF_NOTIFY_REPAIRITEMPRICE          0x0B2F  // Repair price
#define DEF_NOTIFY_CANNOTREPAIRITEM         0x0B2E  // Cannot repair

// Character Stats
#define DEF_NOTIFY_HP                       0x0B07  // HP changed
#define DEF_NOTIFY_MP                       0x0B14  // MP changed
#define DEF_NOTIFY_SP                       0x0B15  // SP changed
#define DEF_NOTIFY_EXP                      0x0B0A  // Experience gained
#define DEF_NOTIFY_LEVELUP                  0x0B16  // Level up
#define DEF_NOTIFY_CHARISMA                 0x0B32  // Charisma changed
#define DEF_NOTIFY_SKILL                    0x0B23  // Skill update
#define DEF_NOTIFY_HUNGER                   0x0B39  // Hunger status

// Combat Notifications
#define DEF_NOTIFY_KILLED                   0x0B09  // Target killed
#define DEF_NOTIFY_ENEMYKILLREWARD          0x0B1C  // Kill reward
#define DEF_NOTIFY_ENEMYKILLS               0x0B5A  // Kill count
#define DEF_NOTIFY_PKPENALTY                0x0B1A  // PK penalty
#define DEF_NOTIFY_PKCAPTURED               0x0B1B  // PK captured

// Magic Notifications
#define DEF_NOTIFY_MAGICSTUDYSUCCESS        0x0B10  // Magic learned
#define DEF_NOTIFY_MAGICSTUDYFAIL           0x0B11  // Magic study failed
#define DEF_NOTIFY_MAGICEFFECTON            0x0B27  // Magic effect on
#define DEF_NOTIFY_MAGICEFFECTOFF           0x0B28  // Magic effect off

// Skill Notifications
#define DEF_NOTIFY_SKILLTRAINSUCCESS        0x0B12  // Skill trained
#define DEF_NOTIFY_SKILLTRAINFAIL           0x0B13  // Skill train failed
#define DEF_NOTIFY_SKILLUSINGEND            0x0B2A  // Skill use ended
#define DEF_NOTIFY_DOWNSKILLINDEXSET        0x0B59  // Skill index set

// Guild Notifications
#define DEF_NOTIFY_QUERY_JOINGUILDREQPERMISSION   0x0B02  // Join request query
#define DEF_NOTIFY_QUERY_DISMISSGUILDREQPERMISSION 0x0B03 // Dismiss query
#define DEF_NOTIFY_WAITFORGUILDOPERATION    0x0B04  // Wait for guild op
#define DEF_NOTIFY_GUILDDISBANDED           0x0B0B  // Guild disbanded
#define DEF_NOTIFY_CANNOTJOINMOREGUILDSMAN  0x0B0D  // Guild full
#define DEF_NOTIFY_NEWGUILDSMAN             0x0B0E  // New member joined
#define DEF_NOTIFY_DISMISSGUILDSMAN         0x0B0F  // Member dismissed
#define DEF_NOTIFY_NOGUILDMASTERLEVEL       0x0B77  // Not guildmaster level
#define DEF_NOTIFY_SUCCESSBANGUILDMAN       0x0B78  // Ban successful
#define DEF_NOTIFY_CANNOTBANGUILDMAN        0x0B79  // Cannot ban
#define DEF_NOTIFY_REQGUILDNAMEANSWER       0x0BA6  // Guild name answer

// Message and Chat
#define DEF_NOTIFY_EVENTMSGSTRING           0x0B0C  // Event message
#define DEF_NOTIFY_NOTICEMSG                0x0B46  // Notice message
#define DEF_NOTIFY_DEBUGMSG                 0x0B49  // Debug message
#define DEF_NOTIFY_NPCTALK                  0x0B57  // NPC dialog

// World State
#define DEF_NOTIFY_TOTALUSERS               0x0B29  // Total users online
#define DEF_NOTIFY_SHOWMAP                  0x0B2B  // Show map
#define DEF_NOTIFY_SERVERCHANGE             0x0B24  // Server change
#define DEF_NOTIFY_TIMECHANGE               0x0B41  // Time changed
#define DEF_NOTIFY_WHETHERCHANGE            0x0B4D  // Weather changed
#define DEF_NOTIFY_SERVERSHUTDOWN           0x0B4E  // Server shutdown
#define DEF_NOTIFY_TOBERECALLED             0x0B40  // Recall pending

// Dynamic Objects
#define DEF_NOTIFY_NEWDYNAMICOBJECT         0x0B21  // New dynamic object
#define DEF_NOTIFY_DELDYNAMICOBJECT         0x0B22  // Remove dynamic obj

// Player Status
#define DEF_NOTIFY_PLAYERONGAME             0x0B33  // Player online
#define DEF_NOTIFY_PLAYERNOTONGAME          0x0B34  // Player offline
#define DEF_NOTIFY_PLAYERPROFILE            0x0B37  // Player profile
#define DEF_NOTIFY_PLAYERSHUTUP             0x0B42  // Player muted
#define DEF_NOTIFY_WHISPERMODEON            0x0B35  // Whisper mode on
#define DEF_NOTIFY_WHISPERMODEOFF           0x0B36  // Whisper mode off

// Fishing
#define DEF_NOTIFY_EVENTFISHMODE            0x0B47  // Fish event mode
#define DEF_NOTIFY_FISHCHANCE               0x0B48  // Fish chance
#define DEF_NOTIFY_FISHSUCCESS              0x0B4A  // Fish success
#define DEF_NOTIFY_FISHFAIL                 0x0B4B  // Fish failed
#define DEF_NOTIFY_FISHCANCELED             0x0B4C  // Fish canceled

// Crafting
#define DEF_NOTIFY_BUILDITEMSUCCESS         0x0B70  // Craft success
#define DEF_NOTIFY_BUILDITEMFAIL            0x0B71  // Craft failed

// Alchemy
#define DEF_NOTIFY_NOMATCHINGPORTION        0x0B53  // No matching recipe
#define DEF_NOTIFY_LOWPORTIONSKILL          0x0B54  // Skill too low
#define DEF_NOTIFY_PORTIONFAIL              0x0B55  // Alchemy failed
#define DEF_NOTIFY_PORTIONSUCCESS           0x0B56  // Alchemy success

// Exchange
#define DEF_NOTIFY_OPENEXCHANGEWINDOW       0x0B5E  // Open exchange
#define DEF_NOTIFY_SETEXCHANGEITEM          0x0B5F  // Set exchange item
#define DEF_NOTIFY_CANCELEXCHANGEITEM       0x0B60  // Cancel exchange
#define DEF_NOTIFY_EXCHANGEITEMCOMPLETE     0x0B61  // Exchange complete
#define DEF_NOTIFY_CANNOTGIVEITEM           0x0B62  // Cannot give item
#define DEF_NOTIFY_GIVEITEMFIN_COUNTCHANGED 0x0B63  // Give count changed
#define DEF_NOTIFY_DROPITEMFIN_COUNTCHANGED 0x0B64  // Drop count changed

// Quest
#define DEF_NOTIFY_QUESTCONTENTS            0x0B66  // Quest contents
#define DEF_NOTIFY_QUESTABORTED             0x0B67  // Quest aborted
#define DEF_NOTIFY_QUESTCOMPLETED           0x0B68  // Quest completed
#define DEF_NOTIFY_QUESTREWARD              0x0B69  // Quest reward

// Special Abilities
#define DEF_NOTIFY_SPECIALABILITYENABLED    0x0B92  // Ability enabled
#define DEF_NOTIFY_SPECIALABILITYSTATUS     0x0B93  // Ability status

// Crusade/War
#define DEF_NOTIFY_CRUSADE                  0x0B94  // Crusade update
#define DEF_NOTIFY_LOCKEDMAP                0x0B95  // Map locked
#define DEF_NOTIFY_DUTYSELECTED             0x0B96  // Duty selected
#define DEF_NOTIFY_MAPSTATUSNEXT            0x0B97  // Map status next
#define DEF_NOTIFY_MAPSTATUSLAST            0x0B98  // Map status last
#define DEF_NOTIFY_HELP                     0x0B99  // Help received
#define DEF_NOTIFY_HELPFAILED               0x0B9A  // Help failed
#define DEF_NOTIFY_METEORSTRIKECOMING       0x0B9B  // Meteor incoming
#define DEF_NOTIFY_METEORSTRIKEHIT          0x0B9C  // Meteor hit
#define DEF_NOTIFY_GRANDMAGICRESULT         0x0B9D  // Grand magic result
#define DEF_NOTIFY_NOMORECRUSADESTRUCTURE   0x0B9E  // No more structures
#define DEF_NOTIFY_CONSTRUCTIONPOINT        0x0B9F  // Construction points
#define DEF_NOTIFY_TCLOC                    0x0BA0  // Teleport location
#define DEF_NOTIFY_CANNOTCONSTRUCT          0x0BA1  // Cannot construct

// Party
#define DEF_NOTIFY_RESPONSE_CREATENEWPARTY  0x0B80  // Create party response
#define DEF_NOTIFY_QUERY_JOINPARTY          0x0B81  // Join party query
#define DEF_NOTIFY_PARTY                    0x0BA2  // Party update

// Energy Sphere (Special Events)
#define DEF_NOTIFY_ENERGYSPHERECREATED      0x0B90  // Sphere created
#define DEF_NOTIFY_ENERGYSPHEREGOALIN       0x0B91  // Sphere goal

// Observer Mode
#define DEF_NOTIFY_OBSERVERMODE             0x0B72  // Observer mode
#define DEF_NOTIFY_GLOBALATTACKMODE         0x0B73  // Global attack mode
#define DEF_NOTIFY_DAMAGEMOVE               0x0B74  // Damage move

// Connection
#define DEF_NOTIFY_FORCEDISCONN             0x0B75  // Force disconnect

// Fight Zone
#define DEF_NOTIFY_FIGHTZONERESERVE         0x0B76  // Arena reserved

// Item Upgrade
#define DEF_NOTIFY_GIZONITEMUPGRADELEFT     0x0BA4  // Upgrade points left
#define DEF_NOTIFY_GIZONEITEMCHANGE         0x0BA5  // Gizon item change
#define DEF_NOTIFY_ITEMUPGRADEFAIL          0x0BA8  // Upgrade failed

// Misc
#define DEF_NOTIFY_REWARDGOLD               0x0B4F  // Gold reward
#define DEF_NOTIFY_IPACCOUNTINFO            0x0B50  // IP account info
#define DEF_NOTIFY_SAFEATTACKMODE           0x0B51  // Safe attack mode
#define DEF_NOTIFY_SUPERATTACKLEFT          0x0B52  // Super attack left
#define DEF_NOTIFY_ADMINIFO                 0x0B58  // Admin info
#define DEF_NOTIFY_ADMINUSERLEVELLOW        0x0B43  // Admin level low
#define DEF_NOTIFY_CANNOTRATING             0x0B44  // Cannot rate
#define DEF_NOTIFY_RATINGPLAYER             0x0B45  // Rating player
#define DEF_NOTIFY_LIMITEDLEVEL             0x0B18  // Level limited
#define DEF_NOTIFY_TRAVELERLIMITEDLEVEL     0x0B38  // Traveler level limit
#define DEF_NOTIFY_NOTENOUGHGOLD            0x0B08  // Not enough gold
#define DEF_NOTIFY_NOTFLAGSPOT              0x0B5D  // Not a flag spot
#define DEF_NOTIFY_FORCERECALLTIME          0x0BA7  // Force recall time
#define DEF_NOTIFY_RESPONSE_HUNTMODE        0x0BA9  // Hunt mode response

// Monster Events
#define DEF_NOTIFY_MONSTEREVENT_POSITION    0x0BAA  // Monster event pos

// Agriculture
#define DEF_NOTIFY_NOMOREAGRICULTURE        0x0BB0  // No more farming
#define DEF_NOTIFY_AGRICULTURESKILLLIMIT    0x0BB1  // Farm skill limit
#define DEF_NOTIFY_AGRICULTURENOAREA        0x0BB2  // Not farming area
```

### Login Response Types

```cpp
#define DEF_LOGRESMSGTYPE_CONFIRM                   0x0F14  // Login confirmed
#define DEF_LOGRESMSGTYPE_REJECT                    0x0F15  // Login rejected
#define DEF_LOGRESMSGTYPE_PASSWORDMISMATCH          0x0F16  // Wrong password
#define DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT        0x0F17  // No such account
#define DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED         0x0F18  // Account created
#define DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED          0x0F19  // Creation failed
#define DEF_LOGRESMSGTYPE_ALREADYEXISTINGACCOUNT    0x0F1A  // Account exists
#define DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER      0x0F1B  // No such char
#define DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED       0x0F1C  // Char created
#define DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED        0x0F1D  // Char create fail
#define DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER  0x0F1E  // Char exists
#define DEF_LOGRESMSGTYPE_CHARACTERDELETED          0x0F1F  // Char deleted
#define DEF_LOGRESMSGTYPE_NOTENOUGHPOINT            0x0F30  // Not enough points
#define DEF_LOGRESMSGTYPE_ACCOUNTLOCKED             0x0F31  // Account locked
#define DEF_LOGRESMSGTYPE_SERVICENOTAVAILABLE       0x0F32  // Service down
#define DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS     0x0A00  // Password changed
#define DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL        0x0A01  // Change failed
#define DEF_LOGRESMSGTYPE_NOTEXISTINGWORLDSERVER    0x0A02  // No world server
#define DEF_LOGRESMSGTYPE_INPUTKEYCODE              0x0A03  // Need key code
#define DEF_LOGRESMSGTYPE_REALACCOUNT               0x0A04  // Real account
#define DEF_LOGRESMSGTYPE_FORCECHANGEPASSWORD       0x0A05  // Force password change
#define DEF_LOGRESMSGTYPE_INVALIDKOREANSSN          0x0A06  // Invalid SSN
#define DEF_LOGRESMSGTYPE_LESSTHENFIFTEEN           0x0A07  // Under 15 years
```

### Enter Game Response Types

```cpp
// Enter options
#define DEF_ENTERGAMEMSGTYPE_NEW                0x0F1C  // New game entry
#define DEF_ENTERGAMEMSGTYPE_NOENTER_FORCEDISCONN 0x0F1D  // Force disconnect
#define DEF_ENTERGAMEMSGTYPE_CHANGINGSERVER     0x0F1E  // Server change
#define DEF_ENTERGAMEMSGTYPE_NEW_TOWLSBUTMLS    0x0F1F  // TownWLS to MLS

// Enter results
#define DEF_ENTERGAMERESTYPE_PLAYING            0x0F20  // Already playing
#define DEF_ENTERGAMERESTYPE_REJECT             0x0F21  // Rejected
#define DEF_ENTERGAMERESTYPE_CONFIRM            0x0F22  // Confirmed
#define DEF_ENTERGAMERESTYPE_FORCEDISCONN       0x0F23  // Force disconnect
```

### Configuration Message IDs

```cpp
#define MSGID_ITEMCONFIGURATIONCONTENTS         0x0FA314D9  // Item config
#define MSGID_NPCCONFIGURATIONCONTENTS          0x0FA314DA  // NPC config
#define MSGID_MAGICCONFIGURATIONCONTENTS        0x0FA314DB  // Magic config
#define MSGID_SKILLCONFIGURATIONCONTENTS        0x0FA314DC  // Skill config
#define MSGID_PLAYERITEMLISTCONTENTS            0x0FA314DD  // Player items
#define MSGID_PORTIONCONFIGURATIONCONTENTS      0x0FA314DE  // Potion config
#define MSGID_PLAYERCHARACTERCONTENTS           0x0FA40000  // Player char
#define MSGID_QUESTCONFIGURATIONCONTENTS        0x0FA40001  // Quest config
#define MSGID_BUILDITEMCONFIGURATIONCONTENTS    0x0FA40002  // Build items
#define MSGID_DUPITEMIDFILECONTENTS             0x0FA40003  // Dup item IDs
#define MSGID_NOTICEMENTFILECONTENTS            0x0FA40004  // Notices
```

### Gate-Server Messages (GSM_*)

For inter-server communication:

```cpp
#define GSM_REQUEST_FINDCHARACTER       0x01  // Find character
#define GSM_RESPONSE_FINDCHARACTER      0x02  // Find response
#define GSM_GRANDMAGICRESULT            0x03  // Grand magic result
#define GSM_GRANDMAGICLAUNCH            0x04  // Grand magic launch
#define GSM_COLLECTEDMANA               0x05  // Mana collected
#define GSM_BEGINCRUSADE                0x06  // Begin crusade
#define GSM_ENDCRUSADE                  0x07  // End crusade
#define GSM_MIDDLEMAPSTATUS             0x08  // Middle map status
#define GSM_SETGUILDTELEPORTLOC         0x09  // Set guild teleport
#define GSM_CONSTRUCTIONPOINT           0x0A  // Construction point
#define GSM_SETGUILDCONSTRUCTLOC        0x0B  // Set construct loc
#define GSM_CHATMSG                     0x0C  // Chat message
#define GSM_WHISFERMSG                  0x0D  // Whisper message
#define GSM_DISCONNECT                  0x0E  // Disconnect
#define GSM_REQUEST_SUMMONPLAYER        0x0F  // Summon player
#define GSM_REQUEST_SHUTUPPLAYER        0x10  // Mute player
#define GSM_RESPONSE_SHUTUPPLAYER       0x11  // Mute response
#define GSM_REQUEST_SETFORCERECALLTIME  0x12  // Set recall time
```

---

## ActionID.h - Action and Motion Constants

### Character Counts

```cpp
#define DEF_TOTALCHARACTERS     80   // Total character sprite types (was 100)
#define DEF_TOTALACTION         15   // Total action animation types
```

### Object Action States

```cpp
#define DEF_OBJECTSTOP          0    // Standing still
#define DEF_OBJECTMOVE          1    // Walking
#define DEF_OBJECTRUN           2    // Running
#define DEF_OBJECTATTACK        3    // Attacking
#define DEF_OBJECTMAGIC         4    // Casting magic
#define DEF_OBJECTGETITEM       5    // Picking up item
#define DEF_OBJECTDAMAGE        6    // Taking damage
#define DEF_OBJECTDAMAGEMOVE    7    // Taking damage while moving
#define DEF_OBJECTATTACKMOVE    8    // Attack while moving
// Note: 9 is not used
#define DEF_OBJECTDYING         10   // Death animation
#define DEF_OBJECTNULLACTION    100  // No action / null state
#define DEF_OBJECTDEAD          101  // Dead (corpse)
```

### Motion Confirmation Codes

```cpp
#define DEF_OBJECTMOVE_CONFIRM              1001  // Move confirmed
#define DEF_OBJECTMOVE_REJECT               1010  // Move rejected
#define DEF_OBJECTMOTION_CONFIRM            1020  // Motion confirmed
#define DEF_OBJECTMOTION_ATTACK_CONFIRM     1030  // Attack confirmed
#define DEF_OBJECTMOTION_REJECT             1040  // Motion rejected
```

---

## Item.h - Item System Constants

### Maximum Equipment Positions

```cpp
#define DEF_MAXITEMEQUIPPOS     15   // Total equipment slot types
```

### Equipment Slot Constants

```cpp
#define DEF_EQUIPPOS_NONE       0    // Not equippable
#define DEF_EQUIPPOS_HEAD       1    // Helmet slot
#define DEF_EQUIPPOS_BODY       2    // Armor/chest slot
#define DEF_EQUIPPOS_ARMS       3    // Gloves/gauntlets
#define DEF_EQUIPPOS_PANTS      4    // Leggings
#define DEF_EQUIPPOS_BOOTS      5    // Boots/shoes
#define DEF_EQUIPPOS_NECK       6    // Necklace/amulet
#define DEF_EQUIPPOS_LHAND      7    // Left hand (shield)
#define DEF_EQUIPPOS_RHAND      8    // Right hand (weapon)
#define DEF_EQUIPPOS_TWOHAND    9    // Two-handed weapon
#define DEF_EQUIPPOS_RFINGER    10   // Right finger ring
#define DEF_EQUIPPOS_LFINGER    11   // Left finger ring
#define DEF_EQUIPPOS_BACK       12   // Cape/cloak
#define DEF_EQUIPPOS_FULLBODY   13   // Full body armor
// Note: 14 is not explicitly defined but DEF_MAXITEMEQUIPPOS is 15
```

### Item Type Constants

```cpp
#define DEF_ITEMTYPE_NONE                       0   // Invalid/none
#define DEF_ITEMTYPE_EQUIP                      1   // Equippable gear
#define DEF_ITEMTYPE_APPLY                      2   // Apply to use
#define DEF_ITEMTYPE_USE_DEPLETE                3   // Use and deplete
#define DEF_ITEMTYPE_INSTALL                    4   // Installable item
#define DEF_ITEMTYPE_CONSUME                    5   // Consumable
#define DEF_ITEMTYPE_ARROW                      6   // Ammunition
#define DEF_ITEMTYPE_EAT                        7   // Food item
#define DEF_ITEMTYPE_USE_SKILL                  8   // Skill use item
#define DEF_ITEMTYPE_USE_PERM                   9   // Permanent use
#define DEF_ITEMTYPE_USE_SKILL_ENABLEDIALOGBOX  10  // Skill + dialog
#define DEF_ITEMTYPE_USE_DEPLETE_DEST           11  // Deplete on dest
#define DEF_ITEMTYPE_MATERIAL                   12  // Crafting material
```

### CItem Class Properties

The `CItem` class contains these member variables:

```cpp
class CItem {
    char  m_cName[21];           // Item name (20 chars + null)
    char  m_cItemType;           // Item type (from constants above)
    char  m_cEquipPos;           // Equipment position
    char  m_cItemColor;          // Visual color variant
    char  m_cSpeed;              // Attack speed modifier
    char  m_cGenderLimit;        // Gender restriction (0=none, 1=male, 2=female)
    short m_sLevelLimit;         // Minimum level to use
    short m_sSprite;             // Sprite sheet index
    short m_sSpriteFrame;        // Frame within sprite sheet
    short m_sX, m_sY;            // Position in inventory grid
    short m_sItemSpecEffectValue1, m_sItemSpecEffectValue2, m_sItemSpecEffectValue3;
    short m_sItemEffectValue1, m_sItemEffectValue2, m_sItemEffectValue3;
    short m_sItemEffectValue4, m_sItemEffectValue5, m_sItemEffectValue6;
    WORD  m_wCurLifeSpan;        // Current durability
    WORD  m_wMaxLifeSpan;        // Maximum durability
    WORD  m_wPrice, m_wWeight;   // Price and weight
    DWORD m_dwCount;             // Stack count (for stackables)
    DWORD m_dwAttribute;         // Attribute bitmask
};
```

---

## Magic.h - Magic System Constants

### Magic Type Constants

```cpp
#define DEF_MAGICTYPE_DAMAGE_SPOT           1   // Damage single target
#define DEF_MAGICTYPE_HPUP_SPOT             2   // Heal single target
#define DEF_MAGICTYPE_DAMAGE_AREA           3   // Area damage
#define DEF_MAGICTYPE_SPDOWN_SPOT           4   // SP drain single
#define DEF_MAGICTYPE_SPDOWN_AREA           5   // SP drain area
#define DEF_MAGICTYPE_SPUP_SPOT             6   // SP restore single
#define DEF_MAGICTYPE_SPUP_AREA             7   // SP restore area
#define DEF_MAGICTYPE_TELEPORT              8   // Teleportation
#define DEF_MAGICTYPE_SUMMON                9   // Summon creature
#define DEF_MAGICTYPE_CREATE                10  // Create object
#define DEF_MAGICTYPE_PROTECT               11  // Protection buff
#define DEF_MAGICTYPE_HOLDOBJECT            12  // Hold/paralyze
#define DEF_MAGICTYPE_INVISIBILITY          13  // Invisibility
#define DEF_MAGICTYPE_CREATE_DYNAMIC        14  // Create dynamic obj
#define DEF_MAGICTYPE_POSSESSION            15  // Mind control
#define DEF_MAGICTYPE_CONFUSE               16  // Confusion debuff
#define DEF_MAGICTYPE_POISON                17  // Poison debuff
#define DEF_MAGICTYPE_BERSERK               18  // Berserk buff
// Note: 19 is not used
#define DEF_MAGICTYPE_POLYMORPH             20  // Polymorph
#define DEF_MAGICTYPE_DAMAGE_AREA_NOSPOT    21  // Area damage (no target)
#define DEF_MAGICTYPE_TREMOR                22  // Tremor/earthquake
#define DEF_MAGICTYPE_ICE                   23  // Ice effect
```

### CMagic Class Properties

```cpp
class CMagic {
    char m_cName[31];       // Magic spell name
    int  m_sValue1;         // Effect value 1
    int  m_sValue2;         // Effect value 2
    int  m_sValue3;         // Effect value 3
    bool m_bIsVisible;      // Whether spell is visible
};
```

---

## Skill.h - Skill System Constants

### CSkill Class Properties

```cpp
class CSkill {
    char m_cName[21];       // Skill name
    int  m_iLevel;          // Skill level
    BOOL m_bIsUseable;      // Can be used actively
    char m_cUseMethod;      // Usage method
};
```

**Note:** Unlike magic types, skills don't have predefined type constants in the header. The 60 skill types are defined through configuration data.

---

## SoundID.h - Audio Constants

### Sound Effect IDs

```cpp
// Weapon Attack Sounds
#define DEF_SOUND_SHORTSWORDATTACK  0   // Short sword swing
#define DEF_SOUND_LONGSWORDATTACK   1   // Long sword swing
#define DEF_SOUND_BOWAIMING         2   // Bow aiming
#define DEF_SOUND_BOWSHOOT          3   // Arrow release
#define DEF_SOUND_AXEATTACK         4   // Axe swing

// Damage Sounds
#define DEF_SOUND_MENDAMAGE         5   // Male damage grunt
#define DEF_SOUND_WOMENDAMAGE       6   // Female damage grunt

// Movement Sounds
#define DEF_SOUND_WALKLAND          7   // Walk on land
#define DEF_SOUND_WALKGLASS         8   // Walk on glass/crystal
#define DEF_SOUND_RUNLAND           9   // Run on land
#define DEF_SOUND_RUNGLASS          10  // Run on glass/crystal

// Death Sounds
#define DEF_SOUND_MENDYING          11  // Male death sound
#define DEF_SOUND_WOMENDYING        12  // Female death sound

// Combat Impact Sounds
#define DEF_SOUND_BAREHANDHIT       13  // Unarmed hit
#define DEF_SOUND_SWORDHIT          14  // Sword impact
#define DEF_SOUND_MACEHIT           15  // Mace/blunt impact
#define DEF_SOUND_ARROWHIT          16  // Arrow impact
```

---

## SpriteID.h - Sprite Constants

### Interface Sprites

```cpp
#define DEF_SPRID_MOUSECURSOR               0   // Mouse cursor

// Font Sprites
#define DEF_SPRID_INTERFACE_SPRFONTS        22  // Font sprites 1
#define DEF_SPRID_INTERFACE_SPRFONTS2       28  // Font sprites 2
#define DEF_SPRID_INTERFACE_FONT1           30  // Font type 1
#define DEF_SPRID_INTERFACE_FONT2           31  // Font type 2

// Additional Interface
#define DEF_SPRID_INTERFACE_ADDINTERFACE    27  // Additional UI
#define DEF_SPRID_INTERFACE_F1HELPWINDOWS   29  // F1 help window

// Map Sprites (Guide Maps)
#define DEF_SPRID_INTERFACE_NEWMAPS1        35  // Map sprite 1
#define DEF_SPRID_INTERFACE_NEWMAPS2        36  // Map sprite 2
#define DEF_SPRID_INTERFACE_NEWMAPS3        37  // Map sprite 3
#define DEF_SPRID_INTERFACE_NEWMAPS4        38  // Map sprite 4
#define DEF_SPRID_INTERFACE_NEWMAPS5        39  // Map sprite 5

// Feedback Card Sprites
#define DEF_SPRID_INTERFACE_FEEDBACK1       40  // Feedback 1
#define DEF_SPRID_INTERFACE_FEEDBACK2       41  // Feedback 2
#define DEF_SPRID_INTERFACE_FEEDBACK3       42  // Feedback 3
#define DEF_SPRID_INTERFACE_FEEDBACK4       43  // Feedback 4
#define DEF_SPRID_INTERFACE_FEEDBACK5       44  // Feedback 5
#define DEF_SPRID_INTERFACE_FEEDBACK6       45  // Feedback 6
#define DEF_SPRID_INTERFACE_FEEDBACK7       46  // Feedback 7

// Monster and Screen Sprites
#define DEF_SPRID_INTERFACE_MONSTER         50  // Monster interface
#define DEF_SPRID_INTERFACE_ND_LOADING      51  // Loading screen
#define DEF_SPRID_INTERFACE_ND_MAINMENU     52  // Main menu
#define DEF_SPRID_INTERFACE_ND_LOGIN        53  // Login screen
#define DEF_SPRID_INTERFACE_ND_NEWACCOUNT   54  // New account screen
#define DEF_SPRID_INTERFACE_ND_QUIT         55  // Quit screen
#define DEF_SPRID_INTERFACE_ND_AGREEMENT    56  // Agreement screen
#define DEF_SPRID_INTERFACE_ND_SELECTCHAR   57  // Character select
#define DEF_SPRID_INTERFACE_ND_NEWCHAR      58  // New character
#define DEF_SPRID_INTERFACE_ND_NEWEXCHANGE  59  // Exchange window

// Game UI Sprites
#define DEF_SPRID_INTERFACE_ND_GAME1        60  // Game UI 1
#define DEF_SPRID_INTERFACE_ND_GAME2        61  // Game UI 2
#define DEF_SPRID_INTERFACE_ND_GAME3        62  // Game UI 3
#define DEF_SPRID_INTERFACE_ND_GAME4        63  // Game UI 4
#define DEF_SPRID_INTERFACE_ND_ICONPANNEL   64  // Icon panel
#define DEF_SPRID_INTERFACE_ND_INVENTORY    67  // Inventory
#define DEF_SPRID_INTERFACE_ND_TEXT         70  // Text dialog
#define DEF_SPRID_INTERFACE_ND_BUTTON       71  // Buttons
#define DEF_SPRID_INTERFACE_ND_CRUSADE      72  // Crusade UI
#define DEF_SPRID_INTERFACE_GUIDEMAP        73  // Guide/mini map
```

### Item Sprite Pivot Points

```cpp
#define DEF_SPRID_ITEMGROUND_PIVOTPOINT     100  // Ground items
#define DEF_SPRID_ITEMEQUIP_PIVOTPOINT      200  // Equipped items
#define DEF_SPRID_ITEMPACK_PIVOTPOINT       300  // Inventory items
#define DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT    400  // Dynamic items
```

---

## DynamicObjectID.h - Dynamic Object Constants

### Dynamic Object Types

```cpp
#define DEF_DYNAMICOBJECT_FIRE          1   // Fire/flame
#define DEF_DYNAMICOBJECT_FISH          2   // Fishing spot
#define DEF_DYNAMICOBJECT_FISHOBJECT    3   // Fish object
#define DEF_DYNAMICOBJECT_MINERAL1      4   // Mineral type 1
#define DEF_DYNAMICOBJECT_MINERAL2      5   // Mineral type 2
// Note: 6-7 are not defined
#define DEF_DYNAMICOBJECT_ICESTORM      8   // Ice storm effect
#define DEF_DYNAMICOBJECT_SPIKE         9   // Spike trap
#define DEF_DYNAMICOBJECT_PCLOUD_BEGIN  10  // Poison cloud start
#define DEF_DYNAMICOBJECT_PCLOUD_LOOP   11  // Poison cloud loop
#define DEF_DYNAMICOBJECT_PCLOUD_END    12  // Poison cloud end
#define DEF_DYNAMICOBJECT_FIRE2         13  // Fire type 2
```

---

## XSocket.h - Socket Constants

### Socket Types

```cpp
#define DEF_XSOCK_LISTENSOCK        1   // Listening socket
#define DEF_XSOCK_NORMALSOCK        2   // Normal connection
#define DEF_XSOCK_SHUTDOWNEDSOCK    3   // Shutdown socket
```

### Socket Read Status

```cpp
#define DEF_XSOCKSTATUS_READINGHEADER   11  // Reading packet header
#define DEF_XSOCKSTATUS_READINGBODY     12  // Reading packet body
```

### Socket Event Codes

```cpp
#define DEF_XSOCKEVENT_SOCKETMISMATCH           -121  // Socket mismatch
#define DEF_XSOCKEVENT_CONNECTIONESTABLISH      -122  // Connected
#define DEF_XSOCKEVENT_RETRYINGCONNECTION       -123  // Retrying
#define DEF_XSOCKEVENT_ONREAD                   -124  // Reading data
#define DEF_XSOCKEVENT_READCOMPLETE             -125  // Read complete
#define DEF_XSOCKEVENT_UNKNOWN                  -126  // Unknown event
#define DEF_XSOCKEVENT_SOCKETCLOSED             -127  // Socket closed
#define DEF_XSOCKEVENT_BLOCK                    -128  // Blocked
#define DEF_XSOCKEVENT_SOCKETERROR              -129  // Socket error
#define DEF_XSOCKEVENT_CRITICALERROR            -130  // Critical error
#define DEF_XSOCKEVENT_NOTINITIALIZED           -131  // Not initialized
#define DEF_XSOCKEVENT_MSGSIZETOOLARGE          -132  // Message too large
#define DEF_XSOCKEVENT_CONFIRMCODENOTMATCH      -133  // Code mismatch
#define DEF_XSOCKEVENT_QUENEFULL                -134  // Queue full
#define DEF_XSOCKEVENT_UNSENTDATASENDBLOCK      -135  // Unsent data blocked
#define DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE   -136  // Unsent data sent
```

### Socket Limits

```cpp
#define DEF_XSOCKBLOCKLIMIT     300  // Maximum blocking iterations
```

---

## Localization System

### Overview

The game uses compile-time localization through language-specific header files:

| File | Language | Character Set |
|------|----------|---------------|
| lan_eng.h | English | ASCII |
| lan_kor.h | Korean | EUC-KR / CP949 |
| lan_chi.h | Chinese (Simplified) | GB2312 |
| lan_tai.h | Chinese (Traditional) | Big5 |
| lan_jap.h | Japanese | Shift-JIS |

### Localization String Categories

Based on the English localization file (`lan_eng.h`), strings are organized into these categories:

#### Connection Messages
```cpp
#define UPDATE_SCREEN_ON_CONNECTING1    "Press ESC key during long time of no"
#define UPDATE_SCREEN_ON_CONNECTING2    "connection and return to the main menu."
#define UPDATE_SCREEN_ON_CONNECTING3    "  Connecting to server. Please wait..."
```

#### Server Names
```cpp
#define MSG_WORLDNAME1  "ABADDON Server"
#define MSG_WORLDNAME2  "APOCALYPSE Server"
// ... up to MSG_WORLDNAME16
```

#### Character Creation
```cpp
#define _BDRAW_ON_CREATE_NEW_CHARACTER1 "Enter a character name."
#define _BDRAW_ON_CREATE_NEW_CHARACTER2 "Select character's gender."
// ... extensive character creation text
```

#### Account Management
```cpp
#define UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT1  "Enter your account ID."
// ... account creation and management strings
```

#### Keyboard Shortcuts
```cpp
#define ON_KEY_UP1  " There is no item or magic for hotkey selected."
// ... hotkey-related messages
```

#### Combat and Abilities
```cpp
#define ON_KEY_UP29 "Ability that decreases enemy's HP by 50%: Can be used after %d sec"
// ... special ability descriptions
```

#### Notifications
```cpp
#define NOTIFY_MSG_HANDLER1  "%s joined the party."
#define NOTIFY_MSG_HANDLER2  "%s withdrew from the party."
// ... game event notifications
```

#### NPC Names
```cpp
#define NPC_NAME_SLIME  "Slime"
// ... monster and NPC names
```

#### Dialog Boxes
```cpp
#define BDLBBOX_DOUBLE_CLICK_INVENTORY1 "You can use after using the other item."
// ... inventory and dialog messages
```

### String Format Specifiers

Many localization strings use C-style format specifiers:
- `%s` - String substitution (names, items)
- `%d` - Integer substitution (counts, times, levels)

Example:
```cpp
#define NOTIFY_MSG_HANDLER3 "You can't move to other position for %d seconds except %s"
```

---

## Cross-Reference Tables

### Dialog Box Type to Index Mapping

The game uses 41 dialog box types, referenced by index in `m_stDialogBoxInfo[41]`:

| Index | Dialog Type | Draw Function | Click Function |
|-------|-------------|---------------|----------------|
| 1 | Character Info | DrawDialogBox_Character | DlgBoxClick_Character |
| 2 | Inventory | DrawDialogBox_Inventory | DlgBoxClick_Inventory |
| 3 | Magic/Spellbook | DrawDialogBox_Magic | DlgBoxClick_Magic |
| 4 | Item Drop (legacy) | DrawDialogBox_ItemDrop | DlgBoxClick_ItemDrop |
| 5 | 15+ Age Warning | DrawDialogBox_15AgeMsg | DlgBoxClick_15AgeMsg |
| 6 | Warning Message | DrawDialogBox_WarningMsg | DlgBoxClick_WarningMsg |
| 7 | Guild Menu | DrawDialogBox_GuildMenu | DlgBoxClick_GuildMenu |
| 8 | Guild Operation | DrawDialogBox_GuildOperation | DlgBoxClick_GuildOp |
| 9 | Guide Map | DrawDialogBox_GuideMap | - |
| 10 | Chat | DrawDialogBox_Chat | - |
| 11 | Shop | DrawDialogBox_Shop | DlgBoxClick_Shop |
| 12 | Level Up Settings | DrawDialogBox_LevelUpSetting | DlgBoxClick_LevelUpSettings |
| 13 | City Hall Menu | DrawDialogBox_CityHallMenu | DlgBoxClick_CityhallMenu |
| 14 | Bank | DrawDialogBox_Bank | DlgBoxClick_Bank |
| 15 | Skills | DrawDialogBox_Skill | DlgBoxClick_Skill |
| 16 | Magic Shop | DrawDialogBox_MagicShop | DlgBoxClick_MagicShop |
| 17 | Query Drop Amount | DrawDialogBox_QueryDropItemAmount | - |
| 18 | Text Display | DrawDialogBox_Text | DlgBoxClick_Text |
| 19 | System Menu | DrawDialogBox_SysMenu | DlgBoxClick_SysMenu |
| 20 | NPC Action Query | DrawDialogBox_NpcActionQuery | DlgBoxClick_NpcActionQuery |
| 21 | NPC Talk | DrawDialogBox_NpcTalk | DlgBoxClick_NpcTalk |
| 22 | Map Display | DrawDialogBox_Map | - |
| 23 | Sell/Repair Items | DrawDialogBox_SellorRepairItem | DlgBoxClick_ItemSellorRepair |
| 24 | Fishing | DrawDialogBox_Fishing | DlgBoxClick_Fish |
| 25 | Shutdown Message | DrawDialogBox_ShutDownMsg | DlgBoxClick_ShutDownMsg |
| 26 | Skill Dialog | DrawDialogBox_SkillDlg | DlgBoxClick_SkillDlg |
| 27 | Exchange | DrawDialogBox_Exchange | DlgBoxClick_Exchange |
| 28 | Quest | DrawDialogBox_Quest | DlgBoxClick_Quest |
| 29 | Gauge Panel | DrawDialogBox_GaugePannel | - |
| 30 | Icon Panel | DrawDialogBox_IconPannel | DlgBoxClick_IconPannel |
| 31 | Sell List | DrawDialogBox_SellList | DlgBoxClick_SellList |
| 32 | Party | DrawDialogBox_Party | DlgBoxClick_Party |
| 33 | Crusade Job | DrawDialogBox_CrusadeJob | DlgBoxClick_CrusadeJob |
| 34 | Item Upgrade | DrawDialogBox_ItemUpgrade | DlgBoxClick_ItemUpgrade |
| 35 | Help | DrawDialogBox_Help | DlgBoxClick_Help |
| 36 | Commander | DrawDialogBox_Commander | DlgBoxClick_Commander |
| 37 | Constructor | DrawDialogBox_Constructor | DlgBoxClick_Constructor |
| 38 | Soldier | DrawDialogBox_Soldier | DlgBoxClick_Soldier |
| 40 | Feedback Card | DrawDialogBox_FeedBackCard | DlgBoxClick_FeedBackCard |

### Game Mode State Transitions

```
ONMAINMENU (0)
    ├── ONLOGIN (8) ────────────────────────────────────────────────┐
    │       ├── ONCONNECTING (1)                                     │
    │       │       └── ONSELECTCHARACTER (10)                       │
    │       │               ├── ONCREATENEWCHARACTER (11)            │
    │       │               ├── ONQUERYDELETECHARACTER (13)          │
    │       │               ├── ONCHANGEPASSWORD (15)                │
    │       │               └── ONMAINGAME (4) ◄─────────────────────┤
    │       │                       ├── ONCONNECTIONLOST (5)         │
    │       │                       └── ONQUIT (-1)                  │
    │       └── ONQUERYFORCELOGIN (9)                                │
    ├── ONCREATENEWACCOUNT (7)                                       │
    ├── ONSELECTSERVER (20) ─────────────────────────────────────────┘
    └── ONAGREEMENT (19)
```

### Resource Limit Summary

| Resource | Limit | Notes |
|----------|-------|-------|
| Sprites | 20,000 | Loaded sprite sheets |
| Tiles | 500 | Tile sprite types |
| Effect Sprites | 100 | Effect animation types |
| Sound Effects | 110 | Concurrent sounds |
| Chat Messages | 500 | Total history |
| Inventory Slots | 50 | Per character |
| Bank Slots | 121 | Per account |
| Guild Members | 32 | Per guild |
| Party Members | 8 | Per party |
| Magic Types | 100 | Spell definitions |
| Skill Types | 60 | Skill definitions |
| Visual Effects | 300 | Concurrent effects |
| Weather Objects | 600 | Particle count |
| Craftable Items | 100 | Recipe definitions |
| Item Names | 1,000 | Name database |
| Guild Names | 100 | Cached names |
| Crusade Structures | 300 | War buildings |

---

## Notes for Modernization

### Issues to Address

1. **Global namespace pollution**: All constants are in global namespace
2. **Magic numbers**: Many hardcoded values throughout code
3. **Compile-time configuration**: Language/region requires recompilation
4. **No type safety**: All constants are untyped macros
5. **Mixed concerns**: Network, UI, and game logic constants in same files

### Recommended Modern Approach

```cpp
// Example modernized structure
namespace hb {
    namespace net {
        enum class MessageId : uint32_t {
            RequestInitPlayer = 0x05040205,
            ResponseInitPlayer = 0x05040206,
            // ...
        };
    }

    namespace game {
        struct Limits {
            static constexpr int MaxSprites = 20000;
            static constexpr int MaxItems = 50;
            // ...
        };

        enum class Mode {
            Null = -2,
            Quit = -1,
            MainMenu = 0,
            // ...
        };
    }

    namespace item {
        enum class EquipSlot : uint8_t {
            None = 0,
            Head = 1,
            Body = 2,
            // ...
        };

        enum class Type : uint8_t {
            None = 0,
            Equip = 1,
            // ...
        };
    }
}
```

### Localization Migration

Convert from:
```cpp
#define UPDATE_SCREEN_ON_CONNECTING1 "Press ESC key..."
```

To runtime JSON:
```json
{
    "connecting": {
        "hint1": "Press ESC key during long time of no",
        "hint2": "connection and return to the main menu."
    }
}
```

---

## Appendix: Complete File Listings

### Headers with #define Constants

| File | Approximate #define Count |
|------|--------------------------|
| NetMessages.h | 250+ |
| lan_eng.h | 2000+ |
| lan_kor.h | 2000+ |
| Game.h | 50+ |
| GlobalDef.h | 30+ |
| ActionID.h | 15 |
| Item.h | 15 |
| Magic.h | 25 |
| SoundID.h | 20 |
| SpriteID.h | 40+ |
| DynamicObjectID.h | 15 |
| XSocket.h | 25 |
| MouseInterface.h | 5 |

### Class Definition Files

| File | Class | Purpose |
|------|-------|---------|
| Game.h | CGame | Main game class (monolithic) |
| Item.h | CItem | Item instance |
| Magic.h | CMagic | Magic spell definition |
| Skill.h | CSkill | Skill definition |
| Effect.h | CEffect | Visual effect instance |
| Tile.h | CTile | Map tile data |
| TileSpr.h | CTileSpr | Tile sprite reference |
| MapData.h | CMapData | Map manager |
| CharInfo.h | CCharInfo | Character info |
| BuildItem.h | CBuildItem | Crafting recipe |
| Sprite.h | CSprite | Sprite resource |
| XSocket.h | XSocket | Network socket |
| Msg.h | CMsg | Message object |
| ItemName.h | CItemName | Item name entry |
| Curse.h | CCurse | Profanity filter |
| GameMonitor.h | CGameMonitor | Bad word monitor |
| MouseInterface.h | CMouseInterface | Mouse input |
| DXC_ddraw.h | DXC_ddraw | DirectDraw wrapper |
| DXC_dinput.h | DXC_dinput | DirectInput wrapper |
| YWSound.h | YWSound | Sound system |
| SoundBuffer.h | CSoundBuffer | Sound resource |

---

*End of Document*
