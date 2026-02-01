# CGame Monolithic Class - Exhaustive Documentation

## Overview

The `CGame` class is the central monolithic class of the Helbreath game client, containing virtually all game logic, rendering, networking, UI, and game mechanics in a single 48,500+ line file. This document provides an exhaustive analysis of the class structure, member variables, methods, and internal systems.

**Files:**
- `Game.h` - 909 lines (class declaration, structures, constants)
- `Game.cpp` - 48,506 lines (implementation)

**Creation Date:** Original GUID suggests Visual C++ 6.0 era (1998-2002)
```cpp
// AFX_GAME_H__0089D9E3_74E6_11D2_A8E6_00001C7030A6__INCLUDED_
```

---

## Table of Contents

1. [Class Architecture](#class-architecture)
2. [Memory Management](#memory-management)
3. [Constants and Limits](#constants-and-limits)
4. [Game Modes](#game-modes)
5. [Member Structures](#member-structures)
6. [Member Variables](#member-variables)
7. [Subsystem Objects](#subsystem-objects)
8. [Method Categories](#method-categories)
9. [Game Loop](#game-loop)
10. [Initialization and Shutdown](#initialization-and-shutdown)
11. [Rendering System](#rendering-system)
12. [Input Handling](#input-handling)
13. [Network Communication](#network-communication)
14. [Dialog System](#dialog-system)
15. [Combat and Magic](#combat-and-magic)
16. [Inventory and Items](#inventory-and-items)
17. [Guild System](#guild-system)
18. [Party System](#party-system)
19. [Chat System](#chat-system)
20. [Map and World](#map-and-world)
21. [Effects and Animation](#effects-and-animation)
22. [Localization](#localization)
23. [Configuration](#configuration)
24. [External Dependencies](#external-dependencies)

---

## Class Architecture

### Custom Memory Allocation

The CGame class overrides `new` and `delete` operators to use Windows Heap functions:

```cpp
void * operator new (size_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
};

void operator delete(void * mem)
{
    HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, mem);
};
```

**Implications:**
- Memory is zero-initialized on allocation (`HEAP_ZERO_MEMORY`)
- No serialization on free (`HEAP_NO_SERIALIZE`) - not thread-safe
- Direct Windows heap usage bypasses C++ runtime

### Include Dependencies

The class depends on the following headers:

| Header | Purpose |
|--------|---------|
| `GlobalDef.h` | Language defines, version numbers, feature flags |
| `DXC_ddraw.h` | DirectDraw 7 wrapper |
| `DXC_dinput.h` | DirectInput 7 wrapper |
| `YWSound.h` | DirectSound 7 wrapper |
| `SoundBuffer.h` | Sound effect management |
| `XSocket.h` | WinSock2 socket wrapper |
| `Sprite.h` | Sprite loading and rendering |
| `SpriteID.h` | Sprite identifier constants |
| `Misc.h` | Utility functions |
| `StrTok.h` | String tokenizer |
| `Msg.h` | Message object wrapper |
| `Effect.h` | Visual effect management |
| `MapData.h` | Map tile data |
| `ActionID.h` | Action/command identifiers |
| `NetMessages.h` | Network protocol definitions |
| `MouseInterface.h` | Mouse interaction system |
| `CharInfo.h` | Character information |
| `Item.h` | Item properties |
| `Magic.h` | Magic/spell definitions |
| `Skill.h` | Skill definitions |
| `DynamicObjectID.h` | Dynamic object types |
| `GameMonitor.h` | Bad word filtering |
| `BuildItem.h` | Crafting system |
| `ItemName.h` | Item name database |
| `Curse.h` | Curse/profanity system |
| `Cint.h` | Thread-safe integer wrapper |

### Conditional Compilation

Language-specific includes based on `DEF_LANGUAGE`:
```cpp
#if DEF_LANGUAGE == 1
#include "lan_tai.h"    // Taiwan/Traditional Chinese
#elif DEF_LANGUAGE == 2
#include "lan_chi.h"    // Simplified Chinese
#elif DEF_LANGUAGE == 3
#include "lan_kor.h"    // Korean
#elif DEF_LANGUAGE == 4
#include "lan_eng.h"    // English
#elif DEF_LANGUAGE == 5
#include "lan_jap.h"    // Japanese
#endif
```

---

## Constants and Limits

### UI Constants

```cpp
#define DEF_BTNSZX              74      // Button size X
#define DEF_BTNSZY              20      // Button size Y
#define DEF_LBTNPOSX            30      // Left button position X
#define DEF_RBTNPOSX            154     // Right button position X
#define DEF_BTNPOSY             292     // Button position Y
```

### Protocol Constants

```cpp
#define DEF_INDEX4_MSGID        0       // Message ID offset (4 bytes)
#define DEF_INDEX2_MSGTYPE      4       // Message type offset (2 bytes)
#define DEF_SOCKETBLOCKLIMIT    300     // Socket blocking limit
```

### Resource Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_MAXSPRITES` | 20,000 | Maximum sprite assets |
| `DEF_MAXTILES` | 500 | Maximum tile sprites |
| `DEF_MAXEFFECTSPR` | 100 | Maximum effect sprites |
| `DEF_MAXSOUNDEFFECTS` | 110 | Maximum sound effects per category |
| `DEF_MAXCHATMSGS` | 500 | Maximum chat message history |
| `DEF_MAXWHISPERMSG` | 5 | Maximum whisper message slots |
| `DEF_MAXCHATSCROLLMSGS` | 80 | Maximum scrollable chat messages |
| `DEF_MAXEFFECTS` | 300 | Maximum concurrent visual effects |
| `DEF_MAXITEMS` | 50 | Maximum inventory slots |
| `DEF_MAXBANKITEMS` | 121 | Maximum bank slots (120+1) |
| `DEF_MAXGUILDSMAN` | 32 | Maximum guild members |
| `DEF_MAXMENUITEMS` | 140 | Maximum shop menu items |
| `DEF_TEXTDLGMAXLINES` | 300 | Maximum text dialog lines |
| `DEF_MAXMAGICTYPE` | 100 | Maximum magic spell types |
| `DEF_MAXSKILLTYPE` | 60 | Maximum skill types |
| `DEF_MAXWHETHEROBJECTS` | 600 | Maximum weather objects |
| `DEF_MAXBUILDITEMS` | 100 | Maximum craftable items |
| `DEF_MAXGAMEMSGS` | 300 | Maximum game message strings |
| `DEF_MAXITEMNAMES` | 1000 | Maximum item name entries |
| `DEF_MAXGUILDNAMES` | 100 | Maximum cached guild names |
| `DEF_MAXSELLLIST` | 12 | Maximum items in sell list |
| `DEF_MAXPARTYMEMBERS` | 8 | Maximum party members |
| `DEF_MAXCRUSADESTRUCTURES` | 300 | Maximum crusade structures |

### Timing Constants

```cpp
#define DEF_CHATTIMEOUT_A       4000    // Chat timeout A (ms)
#define DEF_CHATTIMEOUT_B       500     // Chat timeout B (ms)
#define DEF_CHATTIMEOUT_C       2000    // Chat timeout C (ms)
#define DEF_DOUBLECLICKTIME     300     // Double-click threshold (ms)
```

### Windows Message Constants

```cpp
#define WM_USER_GAMESOCKETEVENT WM_USER + 2000  // Game socket events
#define WM_USER_LOGSOCKETEVENT  WM_USER + 2001  // Login socket events
```

---

## Game Modes

The game operates as a state machine with the following modes:

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_GAMEMODE_NULL` | -2 | Null/uninitialized state |
| `DEF_GAMEMODE_ONQUIT` | -1 | Quitting application |
| `DEF_GAMEMODE_ONMAINMENU` | 0 | Main menu screen |
| `DEF_GAMEMODE_ONCONNECTING` | 1 | Connecting to server |
| `DEF_GAMEMODE_ONLOADING` | 2 | Loading game assets |
| `DEF_GAMEMODE_ONWAITINGINITDATA` | 3 | Waiting for server init data |
| `DEF_GAMEMODE_ONMAINGAME` | 4 | **Main gameplay state** |
| `DEF_GAMEMODE_ONCONNECTIONLOST` | 5 | Connection lost |
| `DEF_GAMEMODE_ONMSG` | 6 | Displaying message |
| `DEF_GAMEMODE_ONCREATENEWACCOUNT` | 7 | Account creation screen |
| `DEF_GAMEMODE_ONLOGIN` | 8 | Login screen |
| `DEF_GAMEMODE_ONQUERYFORCELOGIN` | 9 | Force login query |
| `DEF_GAMEMODE_ONSELECTCHARACTER` | 10 | Character selection |
| `DEF_GAMEMODE_ONCREATENEWCHARACTER` | 11 | Character creation |
| `DEF_GAMEMODE_ONWAITINGRESPONSE` | 12 | Waiting for server response |
| `DEF_GAMEMODE_ONQUERYDELETECHARACTER` | 13 | Delete character confirmation |
| `DEF_GAMEMODE_ONLOGRESMSG` | 14 | Login response message |
| `DEF_GAMEMODE_ONCHANGEPASSWORD` | 15 | Password change screen |
| `DEF_GAMEMODE_ONVERSIONNOTMATCH` | 17 | Version mismatch error |
| `DEF_GAMEMODE_ONINTRODUCTION` | 18 | Introduction/tutorial |
| `DEF_GAMEMODE_ONAGREEMENT` | 19 | User agreement screen |
| `DEF_GAMEMODE_ONSELECTSERVER` | 20 | Server selection |
| `DEF_GAMEMODE_ONINPUTKEYCODE` | 21 | Key code input (China) |

### Server Types

```cpp
#define DEF_SERVERTYPE_GAME     1   // Game server connection
#define DEF_SERVERTYPE_LOG      2   // Login server connection
```

### Cursor Status

```cpp
#define DEF_CURSORSTATUS_NULL       0   // No special cursor state
#define DEF_CURSORSTATUS_PRESSED    1   // Mouse button pressed
#define DEF_CURSORSTATUS_SELECTED   2   // Object selected
#define DEF_CURSORSTATUS_DRAGGING   3   // Dragging item/object
```

### Selected Object Types

```cpp
#define DEF_SELECTEDOBJTYPE_DLGBOX  1   // Dialog box selected
#define DEF_SELECTEDOBJTYPE_ITEM    2   // Item selected
```

---

## Member Structures

### Mouse Cursor State (`m_stMCursor`)

```cpp
struct {
    short sX;                   // Current X position
    short sY;                   // Current Y position
    short sCursorFrame;         // Cursor animation frame
    char  cPrevStatus;          // Previous cursor status
    char  cSelectedObjectType;  // Type of selected object
    short sSelectedObjectID;    // ID of selected object
    short sPrevX, sPrevY;       // Previous position
    short sDistX, sDistY;       // Distance from start
    DWORD dwSelectClickTime;    // Time of selection click
    short sClickX, sClickY;     // Click position
} m_stMCursor;
```

### Dialog Box Info (`m_stDialogBoxInfo[41]`)

Each of the 41 dialog types has the following structure:

```cpp
struct {
    int   sV1, sV2, sV3, sV4, sV5, sV6, sV7, sV8, sV9, sV10, sV11, sV12, sV13, sV14;
    DWORD dwV1, dwV2, dwT1;
    BOOL  bFlag;
    short sX, sY;               // Dialog position
    short sSizeX, sSizeY;       // Dialog size
    short sView;                // Scroll view offset
    char  cStr[32], cStr2[32], cStr3[32], cStr4[32];  // String storage
    char  cMode;                // Dialog mode/state
    BOOL  bIsScrollSelected;    // Scroll selected flag
} m_stDialogBoxInfo[41];
```

### Sell Item List (`m_stSellItemList[12]`)

```cpp
struct {
    int iIndex;     // Item index
    int iAmount;    // Quantity to sell
} m_stSellItemList[DEF_MAXSELLLIST];
```

### Guild Operation List (`m_stGuildOpList[100]`)

```cpp
struct {
    char cName[22];     // Guild member name
    char cOpMode;       // Operation mode
} m_stGuildOpList[100];
```

### Event History (`m_stEventHistory[6]`, `m_stEventHistory2[6]`)

```cpp
struct {
    DWORD dwTime;       // Event timestamp
    char  cColor;       // Text color
    char  cTxt[96];     // Event text
} m_stEventHistory[6];
```

### Weather Objects (`m_stWhetherObject[600]`)

```cpp
struct {
    short sX, sY;       // Position
    char cStep;         // Animation step
} m_stWhetherObject[DEF_MAXWHETHEROBJECTS];
```

### Quest Info (`m_stQuest`)

```cpp
struct {
    BOOL  bIsQuestCompleted;
    short sWho;                 // Quest giver type
    short sQuestType;           // Quest type ID
    short sContribution;        // Contribution points
    short sTargetType;          // Target entity type
    short sTargetCount;         // Target count required
    short sX, sY;               // Target location
    short sRange;               // Valid range
    char  cTargetName[22];      // Target name
} m_stQuest;
```

### Party Members (`m_stPartyMember[8]`)

```cpp
struct {
    char cStatus;       // Member status
    char cName[12];     // Member name
} m_stPartyMember[DEF_MAXPARTYMEMBERS];
```

### Crusade Structures (`m_stCrusadeStructureInfo[300]`)

```cpp
struct {
    short sX, sY;       // Structure position
    char cType;         // Structure type
    char cSide;         // Faction side
} m_stCrusadeStructureInfo[DEF_MAXCRUSADESTRUCTURES];
```

### Party Member Names (`m_stPartyMemberNameList[9]`)

```cpp
struct {
    char cName[12];     // Party member name
} m_stPartyMemberNameList[DEF_MAXPARTYMEMBERS+1];
```

### Guild Names Cache (`m_stGuildName[100]`)

```cpp
struct {
    DWORD dwRefTime;        // Last reference time
    int   iGuildRank;       // Guild rank
    char  cCharName[12];    // Character name
    char  cGuildName[24];   // Guild name
} m_stGuildName[DEF_MAXGUILDNAMES];
```

### Teleport List (`m_stTeleportList[20]`)

```cpp
struct {
    int  iIndex;            // Teleport index
    char mapname[12];       // Destination map name
    int  iX, iY;            // Destination coordinates
    int  iCost;             // Gold cost
} m_stTeleportList[20];
```

---

## Member Variables

### Subsystem Objects

```cpp
// Audio System
class YWSound m_DSound;                                 // DirectSound wrapper
class CSoundBuffer * m_pCSound[DEF_MAXSOUNDEFFECTS];   // Character sounds
class CSoundBuffer * m_pMSound[DEF_MAXSOUNDEFFECTS];   // Music sounds
class CSoundBuffer * m_pESound[DEF_MAXSOUNDEFFECTS];   // Effect sounds
class CSoundBuffer * m_pBGM;                            // Background music

// Graphics System
class DXC_ddraw  m_DDraw;                              // DirectDraw wrapper
class DXC_dinput m_DInput;                             // DirectInput wrapper
class CMisc      m_Misc;                               // Utility class

// Sprite Arrays
class CSprite  * m_pSprite[DEF_MAXSPRITES];            // General sprites
class CSprite  * m_pTileSpr[DEF_MAXTILES];             // Tile sprites
class CSprite  * m_pEffectSpr[DEF_MAXEFFECTSPR];       // Effect sprites

// Map and World
class CMapData * m_pMapData;                           // Map tile data

// Networking
class XSocket * m_pGSock;                              // Game server socket
class XSocket * m_pLSock;                              // Login server socket

// Message Lists
class CMsg    * m_pChatMsgList[DEF_MAXCHATMSGS];       // Character chat bubbles
class CMsg    * m_pChatScrollList[DEF_MAXCHATSCROLLMSGS]; // Chat scroll history
class CMsg    * m_pWhisperMsg[DEF_MAXWHISPERMSG];      // Whisper messages

// Effects
class CEffect * m_pEffectList[DEF_MAXEFFECTS];         // Active visual effects

// Items
class CItem   * m_pItemList[DEF_MAXITEMS];             // Inventory items
class CItem   * m_pBankList[DEF_MAXBANKITEMS];         // Bank items

// Configuration Lists
class CMagic * m_pMagicCfgList[DEF_MAXMAGICTYPE];      // Magic definitions
class CSkill * m_pSkillCfgList[DEF_MAXSKILLTYPE];      // Skill definitions

// Text Content
class CMsg * m_pMsgTextList[DEF_TEXTDLGMAXLINES];      // Text dialog content
class CMsg * m_pMsgTextList2[DEF_TEXTDLGMAXLINES];     // Secondary text
class CMsg * m_pAgreeMsgTextList[DEF_TEXTDLGMAXLINES]; // Agreement text

// Crafting
class CBuildItem * m_pBuildItemList[DEF_MAXBUILDITEMS];     // Craftable items
class CBuildItem * m_pDispBuildItemList[DEF_MAXBUILDITEMS]; // Display list

// Filtering
class CGameMonitor * m_pCGameMonitor;                  // Bad word filter

// Shop Items
class CItem * m_pItemForSaleList[DEF_MAXMENUITEMS];    // Items for sale

// Characters
class CCharInfo * m_pCharList[4];                      // Character slots

// Game Messages
class CMsg * m_pGameMsgList[DEF_MAXGAMEMSGS];          // Game text strings
class CItemName * m_pItemNameList[DEF_MAXITEMNAMES];   // Item name database
```

### Timing Variables

```cpp
DWORD G_dwGlobalTime;           // Global time (from timeGetTime())
DWORD m_dwCommandTime;          // Speed hack prevention timestamp
DWORD m_dwConnectMode;          // Connection mode tracking
DWORD m_dwTime;                 // General timer
DWORD m_dwCurTime;              // Current frame time
DWORD m_dwCheckConnTime;        // Connection check time
DWORD m_dwCheckSprTime;         // Sprite check time
DWORD m_dwCheckChatTime;        // Chat check time
DWORD m_dwDialogCloseTime;      // Dialog close animation time
CInt  m_dwLogOutCountTime;      // Logout countdown (thread-safe)
DWORD m_dwRestartCountTime;     // Restart countdown
DWORD m_dwWOFtime;              // Wall of Fire time
DWORD m_dwObserverCamTime;      // Observer camera time
DWORD m_dwDamagedTime;          // Damage effect time
DWORD m_dwSpecialAbilitySettingTime;  // Special ability cooldown
DWORD m_dwCommanderCommandRequestedTime; // Commander command time
DWORD m_dwTopMsgTime;           // Top message display time
DWORD m_dwEnvEffectTime;        // Environment effect time
DWORD m_dwMonsterEventTime;     // Monster event time
```

### Boolean Flags

```cpp
// Core State Flags
BOOL m_bZoomMap;                    // Minimap zoom enabled
BOOL m_bIsProgramActive;            // Application active
CInt m_bCommandAvailable;           // Can send commands (thread-safe)
BOOL m_bSoundFlag;                  // Sound system initialized
BOOL m_bSoundStat, m_bMusicStat;    // Sound/music enabled
BOOL m_bDialogTrans;                // Dialog transparency
BOOL m_bIsCombatMode;               // Combat mode active
BOOL m_bIsSafeAttackMode;           // Safe attack mode
BOOL m_bSuperAttackMode;            // Super attack mode
BOOL m_bIsObserverMode;             // Observer/spectator mode
BOOL m_bIsObserverCommanded;        // Observer command sent
BOOL m_bIsFirstConn;                // First connection flag
BOOL m_bIsConfusion;                // Confusion status effect
BOOL m_bIsRedrawPDBGS;              // Redraw pre-drawn background
BOOL m_bDrawFlagDir;                // Draw flag direction
BOOL m_bIsCrusadeMode;              // Crusade/war active
BOOL m_bInputStatus;                // Input field active
BOOL m_bToggleScreen;               // Screen toggle (debug)
BOOL m_bIsSpecial;                  // Special mode flag
BOOL m_bIsF1HelpWindowEnabled;      // F1 help visible
BOOL m_bIsPrevMoveBlocked;          // Previous movement blocked
BOOL m_bIsHideLocalCursor;          // Hide system cursor
BOOL m_bIsCheckingGateway;          // Gateway check in progress
BOOL m_bForceAttack;                // Force attack mode
BOOL m_bParalyze;                   // Paralysis status
BOOL m_bWhisper;                    // Whisper enabled
BOOL m_bShout;                      // Shout enabled
BOOL m_bItemDrop;                   // Item drop history active

// Thread-safe flags (CInt wrapper)
CInt m_bIsItemEquipped[DEF_MAXITEMS];   // Item equipped status
CInt m_bIsItemDisabled[DEF_MAXITEMS];   // Item disabled status
CInt m_bIsGetPointingMode;              // Point targeting mode
CInt m_bIsDialogEnabled[41];            // Dialog visibility
CInt m_bSkillUsingStatus;               // Skill in use
CInt m_bItemUsingStatus;                // Item in use
BOOL m_bIsWhetherEffect;                // Weather effect active
CInt m_bIsPoisoned;                     // Poison status
CInt m_bIsSpecialAbilityEnabled;        // Special ability active
CInt m_bIsTeleportRequested;            // Teleport pending
CInt m_bForceDisconn;                   // Force disconnect flag

// Input State
BOOL m_bEnterPressed, m_bEscPressed;
BOOL m_bCtrlPressed, m_bRunningMode;
BOOL m_bShiftPressed;

// Hunter Mode (v2.183)
BOOL m_bHunter;                     // Hunter mode active
BOOL m_bAresden;                    // Aresden faction
BOOL m_bCitizen;                    // Citizen status
```

### Player Statistics

```cpp
// Vital Stats (thread-safe)
CInt m_iHP;                 // Current hit points
CInt m_iMP;                 // Current mana points
CInt m_iSP;                 // Current stamina points

// Combat Stats
int m_iAC;                  // Armor class
int m_iTHAC0;               // To Hit Armor Class 0

// Character Stats
int m_iLevel;               // Character level
int m_iStr;                 // Strength
int m_iInt;                 // Intelligence
int m_iVit;                 // Vitality
int m_iDex;                 // Dexterity
int m_iMag;                 // Magic
int m_iCharisma;            // Charisma
int m_iExp;                 // Experience points
int m_iContribution;        // Contribution points

// Progress Tracking
int m_iEnemyKillCount;      // Enemies killed
int m_iPKCount;             // Player kills (PK)
int m_iRewardGold;          // Pending gold reward

// Guild
int m_iGuildRank;           // Guild rank
int m_iTotalGuildsMan;      // Total guild members

// Level Up
int m_iLU_Point;            // Available level-up points
char m_cLU_Str, m_cLU_Vit, m_cLU_Dex;  // Stat allocations
char m_cLU_Int, m_cLU_Mag, m_cLU_Char;

// Combat
int m_iSuperAttackLeft;     // Super attacks remaining
int m_iCastingMagicType;    // Currently casting spell
int m_iPointCommandType;    // Point command type
int m_iSpecialAbilityType;  // Active special ability
int m_iSpecialAbilityTimeLeftSec;  // Ability time remaining

// Crusade
int m_iCrusadeDuty;         // Crusade role (0=none, 1=commander, 2=constructor, 3=soldier)
int m_iConstructionPoint;   // Construction points
int m_iWarContribution;     // War contribution

// Equipment Status
short m_sItemEquipmentStatus[DEF_MAXITEMEQUIPPOS];  // Equipped items by slot
```

### Player Position and Appearance

```cpp
short m_sPlayerX, m_sPlayerY;       // Player grid position
short m_sPlayerObjectID;            // Player's object ID
short m_sPlayerType;                // Player character type
short m_sPlayerAppr1, m_sPlayerAppr2; // Appearance values 1-2
short m_sPlayerAppr3, m_sPlayerAppr4; // Appearance values 3-4
short m_sPlayerStatus;              // Status flags
int   m_iPlayerApprColor;           // Appearance color

// Character creation
char m_cGender;                     // 1=Male, 2=Female
char m_cSkinCol;                    // Skin color index
char m_cHairStyle;                  // Hair style index
char m_cHairCol;                    // Hair color index
char m_cUnderCol;                   // Undergarment color

// Initial stat allocation
char m_ccStr, m_ccVit, m_ccDex;
char m_ccInt, m_ccMag, m_ccChr;
```

### View and Camera

```cpp
short m_sViewDstX, m_sViewDstY;     // View destination (smooth scroll target)
short m_sViewPointX, m_sViewPointY; // Current view position
char  m_sViewDX, m_sViewDY;         // View velocity
short m_sVDL_X, m_sVDL_Y;           // View delta limit
int   m_iPDBGSdivX, m_iPDBGSdivY;   // Pre-drawn background surface div
int   m_iCameraShakingDegree;       // Camera shake intensity
```

### Account and Session

```cpp
char m_cAccountName[12];            // Account name
char m_cAccountPassword[12];        // Account password
char m_cAccountAge[12];             // Account age verification
char m_cNewPassword[12];            // New password (change)
char m_cNewPassConfirm[12];         // Password confirmation
char m_cAccountCountry[18];         // Country
char m_cAccountSSN[32];             // Social security number (Korea)
char m_cEmailAddr[52];              // Email address
char m_cAccountQuiz[46];            // Security question
char m_cAccountAnswer[22];          // Security answer
char m_cWorldServerName[32];        // Selected world server
char m_cGameServerName[22];         // Game server name (Gateway)

// Character Info
char m_cPlayerName[12];             // Current character name
char m_cPlayerDir;                  // Facing direction (1-8)
char m_cLocation[12];               // Faction location (aresden/elvine)
char m_cCurLocation[12];            // Current map location
char m_cGuildName[22];              // Guild name
char m_cMapName[12];                // Current map name
char m_cMapMessage[32];             // Map description message
char m_cMapIndex;                   // Map index number

// Login Server
char m_cLogServerAddr[16];          // Login server IP
int  m_iLogServerPort;              // Login server port
```

### Mastery and Skills

```cpp
char m_cMagicMastery[DEF_MAXMAGICTYPE];         // Magic mastery levels (100 spells)
unsigned char m_cSkillMastery[DEF_MAXSKILLTYPE]; // Skill mastery levels (60 skills)
short m_sMagicShortCut;                          // Magic shortcut key assignment
short m_sRecentShortCut;                         // Recently used shortcut
short m_sShortCut[5];                            // 5 shortcut slots
int m_iDownSkillIndex;                           // Downgrade skill index
```

### FPS and Performance

```cpp
short m_sFrameCount;                // Frames this second
short m_sFPS;                       // Current FPS
DWORD m_dwFPStime;                  // FPS calculation time
BOOL  m_bShowFPS;                   // Show FPS counter
char  m_cDetailLevel;               // Graphics detail (0-2)
```

### Color Tables

```cpp
// Object colors (for sprites, items, effects)
WORD m_wR[16], m_wG[16], m_wB[16];      // RGB color components

// Weather/environment colors
WORD m_wWR[16], m_wWG[16], m_wWB[16];   // Weather RGB components
```

### Input Buffer

```cpp
char * m_pInputBuffer;              // Current input field buffer
unsigned char m_cInputMaxLen;       // Maximum input length
int m_iInputX, m_iInputY;           // Input field position
```

---

## Subsystem Objects

### Graphics System

The `m_DDraw` object (`DXC_ddraw` class) handles:
- DirectDraw 7 initialization
- Surface creation and management
- Pixel format detection (RGB555/RGB565)
- Text rendering
- Alpha blending at 5 levels (100%, 70%, 50%, 25%, 2%)

### Input System

The `m_DInput` object (`DXC_dinput` class) handles:
- DirectInput 7 initialization
- Keyboard state polling
- Mouse position and button states
- Mouse wheel delta (`m_sZ`)

### Audio System

The `m_DSound` object (`YWSound` class) handles:
- DirectSound 7 initialization
- Sound buffer creation

Sound buffers are organized into three categories:
- `m_pCSound[110]` - Character sounds (footsteps, attacks)
- `m_pMSound[110]` - Music/ambient sounds
- `m_pESound[110]` - Effect sounds

### Network System

Two socket connections are maintained:
- `m_pGSock` - Game server socket
- `m_pLSock` - Login server socket

Both use the `XSocket` class, a WinSock2 wrapper with:
- Asynchronous event handling
- Message queuing
- Blocking mode support

### Map System

The `m_pMapData` object (`CMapData` class) handles:
- Tile grid (40x35 visible area)
- Object placement
- Walkability checking
- Dynamic object tracking

---

## Method Categories

### Total Method Count: ~350+ methods

### Initialization and Shutdown (5 methods)

```cpp
CGame();                                    // Constructor
virtual ~CGame();                           // Destructor
BOOL bInit(HWND hWnd, HINSTANCE hInst, char * pCmdLine);  // Initialize game
void Quit();                                // Cleanup and exit
void InitGameSettings();                    // Reset game settings
```

### Game Loop (3 methods)

```cpp
void UpdateScreen();                        // Main update dispatcher
void OnTimer();                             // Timer event handler
void ChangeGameMode(char cMode);            // State machine transition
```

### Screen Update Methods (25+ methods)

```cpp
void UpdateScreen_OnMainMenu();
void UpdateScreen_OnGame();
void UpdateScreen_OnConnecting();
void UpdateScreen_OnLoading(bool bActive);
void UpdateScreen_OnLoading_Progress();
void UpdateScreen_OnConnectionLost();
void UpdateScreen_OnLogin();
void UpdateScreen_OnMsg();
void UpdateScreen_OnQuit();
void UpdateScreen_OnQueryForceLogin();
void UpdateScreen_OnSelectCharacter();
void UpdateScreen_OnCreateNewCharacter();
void UpdateScreen_OnWaitingResponse();
void UpdateScreen_OnQueryDeleteCharacter();
void UpdateScreen_OnLogResMsg();
void UpdateScreen_OnChangePassword();
void UpdateScreen_OnWaitInitData();
void UpdateScreen_OnVersionNotMatch();
void UpdateScreen_OnSelectServer();
void UpdateScreen_OnAgreement();            // Conditional (DEF_MAKE_ACCOUNT)
void UpdateScreen_OnCreateNewAccount();     // Conditional (DEF_MAKE_ACCOUNT)
void UpdateScreen_OnInputKeyCode();         // Conditional (DEF_LANGUAGE == 2)
```

### Rendering Methods (40+ methods)

```cpp
// Object Drawing
void DrawObjects(short sPivotX, short sPivotY, ...);
BOOL DrawObject_OnStop(int indexX, int indexY, ...);
BOOL DrawObject_OnMove(int indexX, int indexY, ...);
BOOL DrawObject_OnDamageMove(int indexX, int indexY, ...);
BOOL DrawObject_OnRun(int indexX, int indexY, ...);
BOOL DrawObject_OnAttack(int indexX, int indexY, ...);
BOOL DrawObject_OnAttackMove(int indexX, int indexY, ...);
BOOL DrawObject_OnMagic(int indexX, int indexY, ...);
BOOL DrawObject_OnGetItem(int indexX, int indexY, ...);
BOOL DrawObject_OnDamage(int indexX, int indexY, ...);
BOOL DrawObject_OnDying(int indexX, int indexY, ...);
BOOL DrawObject_OnDead(int indexX, int indexY, ...);
BOOL DrawObject_OnMove_ForMenu(int indexX, int indexY, ...);

// Background and UI
void DrawBackground(short sDivX, short sModX, short sDivY, short sModY);
void DrawDialogBoxs(short msX, short msY, short msZ, char cLB);
void DrawEffects();
void DrawEffectLights();
void DrawChatMsgs(short sX, short sY, short dX, short dY);
void DrawChatMsgBox(short sX, short sY, int iChatIndex, BOOL bIsPreDC);
void DrawWhetherEffects();
void DrawTopMsg();
void DrawVersion();
void DrawLine(int x0, int y0, int x1, int y1, int iR, int iG, int iB);
void DrawLine2(int x0, int y0, int x1, int y1, int iR, int iG, int iB);

// Names and Labels
void DrawNpcName(short sX, short sY, short sOwnerType, short sStatus);
void DrawObjectName(short sX, short sY, char * pName, short sStatus);
void DrawObjectFOE(int ix, int iy, int iFrame);

// Special Effects
void _DrawThunderEffect(int sX, int sY, int dX, int dY, int rX, int rY, char cType);
void _DrawBlackRect(int iSize);
```

### Dialog Box Methods (76+ methods)

Each dialog has a Draw and Click handler:

```cpp
// Drawing (41 dialog types)
void DrawDialogBox_Character(short msX, short msY);         // #1
void DrawDialogBox_Inventory(int msX, int msY);             // #2
void DrawDialogBox_Magic(short msX, short msY, short msZ);  // #3
void DrawDialogBox_ItemDrop(short msX, short msY);          // #4
void DrawDialogBox_15AgeMsg(short msX, short msY);          // #5
void DrawDialogBox_WarningMsg(short msX, short msY);        // #6
void DrawDialogBox_GuildMenu(short msX, short msY);         // #7
void DrawDialogBox_GuildOperation(short msX, short msY);    // #8
void DrawDialogBox_GuideMap(short msX, short msY, char cLB);// #9
void DrawDialogBox_Chat(short msX, short msY, short msZ, char cLB); // #10
void DrawDialogBox_Shop(short msX, short msY, short msZ, char cLB); // #11
void DrawDialogBox_LevelUpSetting(short msX, short msY);    // #12
void DrawDialogBox_CityHallMenu(short msX, short msY);      // #13
void DrawDialogBox_Bank(short msX, short msY, short msZ, char cLB); // #14
void DrawDialogBox_Skill(short msX, short msY, short msZ, char cLB); // #15
void DrawDialogBox_MagicShop(short msX, short msY, short msZ); // #16
void DrawDialogBox_QueryDropItemAmount();                    // #17
void DrawDialogBox_Text(short msX, short msY, short msZ, char cLB); // #18
void DrawDialogBox_SysMenu(short msX, short msY, char cLB); // #19
void DrawDialogBox_NpcActionQuery(short msX, short msY);    // #20
void DrawDialogBox_NpcTalk(short msX, short msY, char cLB); // #21
void DrawDialogBox_Map();                                    // #22
void DrawDialogBox_SellorRepairItem(short msX, short msY);  // #23
void DrawDialogBox_Fishing(short msX, short msY);           // #24
void DrawDialogBox_ShutDownMsg(short msX, short msY);       // #25
void DrawDialogBox_SkillDlg(short msX, short msY, short msZ, char cLB); // #26
void DrawDialogBox_Exchange(short msX, short msY);          // #27
void DrawDialogBox_Quest(int msX, int msY);                 // #28
void DrawDialogBox_GaugePannel();                           // #29
void DrawDialogBox_IconPannel(short msX, short msY);        // #30
void DrawDialogBox_SellList(short msX, short msY);          // #31
void DrawDialogBox_Party(short msX, short msY);             // #32
void DrawDialogBox_CrusadeJob(short msX, short msY);        // #33
void DrawDialogBox_ItemUpgrade(int msX, int msY);           // #34
void DrawDialogBox_Help(int msX, int msY);                  // #35
void DrawDialogBox_Commander(int msX, int msY);             // #36
void DrawDialogBox_Constructor(int msX, int msY);           // #37
void DrawDialogBox_Soldier(int msX, int msY);               // #38
void DrawDialogBox_FeedBackCard(short msX, short msY);      // #40

// Click Handlers
void DlgBoxClick_Character(short msX, short msY);
void DlgBoxClick_Inventory(short msX, short msY);
void DlgBoxClick_Magic(short msX, short msY);
// ... (one for each dialog)

// Double-Click Handlers
void DlbBoxDoubleClick_Inventory(short msX, short msY);
void DlbBoxDoubleClick_Character(short msX, short msY);
void DlbBoxDoubleClick_GuideMap(short msX, short msY);

// Dialog Management
void EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char * pString);
void DisableDialogBox(int iBoxID);
BOOL _bCheckDlgBoxClick(short msX, short msY);
BOOL _bCheckDlgBoxDoubleClick(short msX, short msY);
int  _iCheckDlgBoxFocus(short msX, short msY, char cButtonSide);
int  iGetTopDialogBoxIndex();
```

### Network Methods (60+ methods)

```cpp
// Socket Events
void OnGameSocketEvent(WPARAM wParam, LPARAM lParam);
void OnLogSocketEvent(WPARAM wParam, LPARAM lParam);

// Sending Commands
BOOL bSendCommand(DWORD dwMsgID, WORD wCommand, char cDir, int iV1, int iV2, int iV3, char * pString, int iV4);

// Message Handlers
void GameRecvMsgHandler(DWORD dwMsgSize, char * pData);
void LogRecvMsgHandler(char * pData);
void ConnectionEstablishHandler(char cWhere);

// Response Handlers
void InitDataResponseHandler(char * pData);
void InitPlayerResponseHandler(char * pData);
void MotionResponseHandler(char * pData);
void LogResponseHandler(char * pData);

// Event Handlers
void CommonEventHandler(char * pData);
void MotionEventHandler(char * pData);
void LogEventHandler(char * pData);
void ChatMsgHandler(char * pData);
void DynamicObjectHandler(char * pData);
void NpcTalkHandler(char * pData);
void NoticementHandler(char * pData);
void ResponsePanningHandler(char * pData);

// Notify Message Handlers (70+ specific handlers)
void NotifyMsgHandler(char * pData);
void NotifyMsg_HP(char * pData);
void NotifyMsg_MP(char * pData);
void NotifyMsg_SP(char * pData);
void NotifyMsg_Exp(char * pData);
void NotifyMsg_LevelUp(char * pData);
void NotifyMsg_ItemObtained(char * pData);
void NotifyMsg_ItemPurchased(char * pData);
// ... (many more)
```

### Input Handling (10+ methods)

```cpp
void OnKeyDown(WPARAM wParam);
void OnKeyUp(WPARAM wParam);
void OnSysKeyDown(WPARAM wParam);
void OnSysKeyUp(WPARAM wParam);
void CommandProcessor(short msX, short msY, short indexX, short indexY, char cLB, char cRB);
void PointCommandHandler(int indexX, int indexY, char cItemID);
bool GetText(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
void StartInputString(int sX, int sY, unsigned char iLen, char * pBuffer, BOOL bIsHide);
void EndInputString();
void ReceiveString(char * pString);
void ClearInputString();
void ShowReceivedString(BOOL bIsHide);
```

### Item Management (25+ methods)

```cpp
void InitItemList(char * pData);
void EraseItem(char cItemID);
void ItemEquipHandler(char cItemID);
void ReleaseEquipHandler(char cEquipPos);
void RetrieveItemHandler(char * pData);
void bItemDrop_ExternalScreen(char cItemID, short msX, short msY);
void bItemDrop_Inventory(short msX, short msY);
void bItemDrop_Character();
void bItemDrop_IconPannel(short msX, short msY);
void bItemDrop_SkillDialog();
void bItemDrop_Bank(short msX, short msY);
void bItemDrop_SellList(short msX, short msY);
void bItemDrop_ExchangeDialog(short msX, short msY);
void bItemDrop_ItemUpgrade();
void GetItemName(char * cItemName, DWORD dwAttribute, char *pStr1, char *pStr2, char *pStr3);
void GetItemName(class CItem * pItem, char * pStr1, char * pStr2, char * pStr3);
void SetItemCount(char * pItemName, DWORD dwCount);
void _SetItemOrder(char cWhere, char cItemID);
BOOL _bCheckItemByType(char cType);
BOOL _bIsItemOnHand();
BOOL bCheckItemOperationEnabled(char cItemID);
int  _iCalcTotalWeight();
int  _iGetTotalItemNum();
int  _iGetBankItemCount();
BOOL _ItemDropHistory(char * ItemName);
```

### Combat and Magic (15+ methods)

```cpp
int  iGetManaCost(int iMagicNo);
void UseMagic(int iMagicNo);
void UseShortCut(int num);
void ClearSkillUsingStatus();
int  _iGetAttackType();
int  _iGetWeaponSkillType();
void SetCameraShakingEffect(short sDist, int iMul);
void MeteorStrikeComing(int iCode);
void GrandMagicResult(char * pMapName, int iV1, int iV2, int iV3, int iV4, ...);
void CrusadeWarResult(int iWinnerSide);
void CrusadeContributionResult(int iWarContribution);
void CannotConstruct(int iCode);
```

### Guild Methods (15+ methods)

```cpp
void CreateNewGuildResponseHandler(char * pData);
void DisbandGuildResponseHandler(char * pData);
void InitPlayerCharacteristics(char * pData);
void _PutGuildOperationList(char * pName, char cOpMode);
void _ShiftGuildOperationList();
void ClearGuildNameList();
BOOL FindGuildName(char* pName, int* ipIndex);
```

### Party Methods (5+ methods)

```cpp
// Handled primarily through NotifyMsg_* handlers
// Party dialog click handling in DlgBoxClick_Party
```

### Chat Methods (10+ methods)

```cpp
void PutChatScrollList(char * pMsg, char cType);
void ReleaseTimeoverChatMsg();
void _RemoveChatMsgListByObjectID(int iObjectID);
void AddEventList(char * pTxt, char cColor, BOOL bDupAllow);
void ShowEventList(DWORD dwTime);
BOOL _bCheckBadWords(char * pMsg);
BOOL bCheckLocalChatCommand(char * pMsg);
```

### Sprite Management (10+ methods)

```cpp
void MakeSprite(char* FileName, short sStart, short sCount, bool bAlphaEffect);
void MakeTileSpr(char* FileName, short sStart, short sCount, bool bAlphaEffect);
void MakeEffectSpr(char* FileName, short sStart, short sCount, bool bAlphaEffect);
void RestoreSprites();
void ReleaseUnusedSprites();
```

### Effect Methods (10+ methods)

```cpp
void bAddNewEffect(short sType, int sX, int sY, int dX, int dY, char cStartFrame, int iV1);
BOOL bEffectFrameCounter();
void _SetIlusionEffect(int iOwnerH);
void WhetherObjectFrameCounter();
void SetWhetherStatus(BOOL bStart, char cType);
```

### Configuration Methods (15+ methods)

```cpp
void ReadSettings();
void WriteSettings();
BOOL bReadLoginConfigFile(char * cFn);
BOOL bReadItemNameConfigFile();
BOOL bInitMagicCfgList();
BOOL bInitSkillCfgList();
BOOL _bDecodeBuildItemContents();
BOOL __bDecodeBuildItemContents(char * pBuffer);
BOOL _bCheckBuildItemStatus();
BOOL _bCheckCurrentBuildItemStatus();
void _LoadShopMenuContents(char cType);
void _LoadTextDlgContents(int cType);
int  _iLoadTextDlgContents2(int iType);
void _LoadGameMsgTextContents();
void _LoadAgreementTextContents(char cType);
```

### Map Methods (10+ methods)

```cpp
void _ReadMapData(short sPivotX, short sPivotY, char * pData);
void CalcViewPoint();
char cGetNextMoveDir(short sX, short sY, short dstX, short dstY, BOOL bMoveCheck);
BOOL _bCheckMoveable(short sx, short sy);
void AddMapStatusInfo(char * pData, BOOL bIsLastData);
void _RequestMapStatus(char * pMapName, int iMode);
char GetOfficialMapName(char * pMapName, char * pName);
void GetNpcName(short sType, char * pName);
```

### Utility Methods (20+ methods)

```cpp
void PutString(int iX, int iY, char * pString, COLORREF color);
void PutString(int iX, int iY, char * pString, COLORREF color, BOOL bHide, char cBGtype, BOOL bIsPreDC);
void PutString2(int iX, int iY, char * pString, short sR, short sG, short sB);
void PutAlignedString(int iX1, int iX2, int iY, char * pString, short sR, short sG, short sB);
void PutString_SprFont(int iX, int iY, char * pStr, short sR, short sG, short sB);
void PutString_SprFont2(int iX, int iY, char * pStr, short sR, short sG, short sB);
void PutString_SprFont3(int iX, int iY, char * pStr, short sR, short sG, short sB, BOOL bTrans, int iType);
void PutString_SprNum(int iX, int iY, char * pStr, short sR, short sG, short sB);
void DisplayGold(int iGold);
void CreateScreenShot();
void PlaySound(char cType, int iNum, int iDist, long lPan);
void StartBGM();
void GoHomepage();
LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
int  GetCharKind(char *str, int index);
BOOL _bGetIsStringIsNumber(char * pStr);
void _GetHairColorRGB(int iColorType, int * pR, int * pG, int * pB);
int  iGetLevelExp(int iLevel);
void GetPlayerTurn();
void RequestFullObjectData(WORD wObjectID);
void RequestTeleportAndWaitData();
void SetTopMsg(char * pString, unsigned char iLastSec);
int  _iGetFOE(short sStatus);
```

---

## Game Loop

### Main Loop (`UpdateScreen`)

The `UpdateScreen()` method is called every frame and dispatches to the appropriate screen handler based on `m_cGameMode`:

```cpp
void CGame::UpdateScreen()
{
    G_dwGlobalTime = timeGetTime();

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
        UpdateScreen_OnGame();          // Main gameplay
        break;
    // ... other modes
    }

    // Input handling for Windows IME
    #ifdef DEF_USING_WIN_IME
    // Enter key, Tab key, Left arrow handling
    #endif
}
```

### Main Gameplay Loop (`UpdateScreen_OnGame`)

The main game rendering loop (not shown in full, approximately 3000+ lines) performs:

1. **Timing calculations**
2. **Input processing** via `CommandProcessor()`
3. **Camera/view updates** via `CalcViewPoint()`
4. **Background rendering** via `DrawBackground()`
5. **Object rendering** via `DrawObjects()`
6. **Effect rendering** via `DrawEffects()`, `DrawEffectLights()`
7. **Weather effects** via `DrawWhetherEffects()`
8. **Chat messages** via `DrawChatMsgs()`
9. **Dialog boxes** via `DrawDialogBoxs()`
10. **Cursor rendering**
11. **Frame buffer flip**
12. **Frame counting**

### Timer Handler (`OnTimer`)

Called periodically to handle:

1. **Connection timeout checking**
2. **Sprite cache cleanup**
3. **Chat message timeout**
4. **Effect frame advancement**
5. **Animation updates**
6. **Server ping/keepalive**

---

## Initialization and Shutdown

### Initialization (`bInit`)

The `bInit()` method (lines 811-1019) performs:

1. **Command line parsing**
   - Tokenizes up to 5 command line parameters
   - Checks for `/egparam` flag

2. **Resource validation**
   - `bCheckImportantFile()` - Verifies file checksums
   - `_bDecodeBuildItemContents()` - Decodes crafting data

3. **Configuration loading**
   - `bReadLoginConfigFile("login.cfg")`
   - `bReadItemNameConfigFile()`
   - `bInitMagicCfgList()` - Load MAGICCFG.TXT
   - `bInitSkillCfgList()` - Load SKILLCFG.TXT

4. **DirectX initialization**
   - `m_DDraw.bInit(m_hWnd)` - DirectDraw 7
   - `m_DInput.bInit(hWnd, hInst)` - DirectInput 7
   - `m_DSound.Create(m_hWnd)` - DirectSound 7

5. **Initial sprite loading**
   - Loading screen sprites from `New-Dialog.pak`
   - Interface sprites from `interface2.pak`

6. **Map data initialization**
   - Creates `CMapData` instance

7. **Color table initialization**
   - Sets up 16 RGB color values for object/weather rendering

8. **Bad word filter initialization**
   - Loads and decrypts `badword.txt`
   - Loads curse words (language-specific)

9. **Game message loading**
   - `_LoadGameMsgTextContents()`

### Shutdown (`Quit`)

The `Quit()` method (lines 1023-1103) performs cleanup:

1. **Save settings** via `WriteSettings()`
2. **Change game mode** to `DEF_GAMEMODE_NULL`
3. **Delete all dynamically allocated arrays:**
   - Sprites, tiles, effect sprites
   - Sound buffers (3 categories)
   - BGM
   - Character list (4 slots)
   - Item lists (inventory, bank)
   - Effect list
   - Chat message lists
   - Magic/skill configuration lists
   - Text dialog content
   - Build item lists
   - Game message lists
   - Item name list
4. **Delete map data**
5. **Delete socket connections**
6. **Delete game monitor**

---

## Rendering System

### Background Rendering

`DrawBackground()` renders the tile-based map background:

1. **Calculates visible tile range** based on view position
2. **Iterates through visible tiles** (25x19 grid)
3. **Renders terrain tiles** from tile sprite arrays
4. **Handles animated tiles** with frame timing

### Object Rendering

`DrawObjects()` renders all game entities:

1. **Dead objects first** (corpses on ground)
2. **Ground items** (dropped loot)
3. **Living entities** based on action state:
   - `DrawObject_OnStop()` - Idle
   - `DrawObject_OnMove()` - Walking
   - `DrawObject_OnRun()` - Running
   - `DrawObject_OnAttack()` - Attacking
   - `DrawObject_OnMagic()` - Casting
   - `DrawObject_OnDamage()` - Taking damage
   - `DrawObject_OnDying()` - Death animation
   - `DrawObject_OnDead()` - Dead/corpse
4. **Dynamic objects** (fires, minerals, fish, etc.)
5. **Tree/object occlusion** with transparency
6. **Mouse cursor collision detection**

### Sprite Rendering Modes

Sprites support multiple rendering modes:
- `PutSpriteFast()` - No transparency
- `PutTransSprite()` - Color key transparency
- `PutTransSprite2()` - Alternative transparency
- `PutTransSprite25_NoColorKey()` - 25% opacity
- `PutTransSprite50_NoColorKey()` - 50% opacity
- `PutTransSprite70_NoColorKey()` - 70% opacity
- `PutSpriteRGB()` - RGB color tinting
- `PutTransSpriteRGB()` - Transparent with RGB tint
- `PutShadowSprite()` - Shadow rendering
- `PutFadeSprite()` - Fade effect

---

## Network Communication

### Message Protocol

Messages use a binary protocol with:
- 4-byte message ID at offset 0
- 2-byte message type at offset 4
- Variable payload starting at offset 6
- XOR encryption with random key

### Message Categories

#### Request Messages (Client → Server)

| Message ID | Purpose |
|------------|---------|
| `MSGID_REQUEST_LOGIN` | Login authentication |
| `MSGID_REQUEST_CREATENEWACCOUNT` | Create account |
| `MSGID_REQUEST_CREATENEWCHARACTER` | Create character |
| `MSGID_REQUEST_DELETECHARACTER` | Delete character |
| `MSGID_REQUEST_ENTERGAME` | Enter game world |
| `MSGID_REQUEST_INITPLAYER` | Initialize player data |
| `MSGID_REQUEST_INITDATA` | Request initial game data |
| `MSGID_COMMAND_COMMON` | Common game commands |
| `MSGID_COMMAND_CHATMSG` | Chat message |
| `MSGID_REQUEST_TELEPORT` | Request teleport |
| `MSGID_REQUEST_CREATENEWGUILD` | Create guild |
| `MSGID_REQUEST_DISBANDGUILD` | Disband guild |
| `MSGID_REQUEST_CIVILRIGHT` | Citizenship request |
| `MSGID_REQUEST_RETRIEVEITEM` | Retrieve from bank |
| `MSGID_REQUEST_SETITEMPOS` | Set item position |
| `MSGID_REQUEST_PANNING` | Observer camera pan |
| `MSGID_REQUEST_SELLITEMLIST` | Sell multiple items |
| `MSGID_REQUEST_RESTART` | Restart character |
| `MSGID_LEVELUPSETTINGS` | Apply level-up stats |

#### Response Messages (Server → Client)

| Message ID | Purpose |
|------------|---------|
| `MSGID_RESPONSE_LOG` | Login response |
| `MSGID_RESPONSE_INITPLAYER` | Player initialization |
| `MSGID_RESPONSE_INITDATA` | Game data initialization |
| `MSGID_RESPONSE_MOTION` | Movement confirmation |
| `MSGID_RESPONSE_CREATENEWGUILD` | Guild creation result |
| `MSGID_RESPONSE_DISBANDGUILD` | Guild disband result |
| `MSGID_RESPONSE_PANNING` | Panning result |

#### Event Messages (Server → Client broadcasts)

| Message ID | Purpose |
|------------|---------|
| `MSGID_EVENT_MOTION` | Entity movement event |
| `MSGID_EVENT_COMMON` | Common game event |
| `MSGID_EVENT_LOG` | Logging event |
| `MSGID_NOTIFY` | Notification with subtype |
| `MSGID_DYNAMICOBJECT` | Dynamic object spawn/update |
| `MSGID_PLAYERITEMLISTCONTENTS` | Inventory update |

### Notify Subtypes

The `MSGID_NOTIFY` message has 100+ subtypes including:
- `DEF_NOTIFY_HP`, `DEF_NOTIFY_MP`, `DEF_NOTIFY_SP` - Stat updates
- `DEF_NOTIFY_LEVELUP`, `DEF_NOTIFY_EXP` - Progression
- `DEF_NOTIFY_ITEMOBTAINED`, `DEF_NOTIFY_ITEMRELEASED` - Items
- `DEF_NOTIFY_KILLED`, `DEF_NOTIFY_PKPENALTY` - Combat
- `DEF_NOTIFY_PARTY`, `DEF_NOTIFY_CRUSADE` - Social
- `DEF_NOTIFY_MAGICEFFECTON/OFF` - Buffs/debuffs

### Common Command Types

The `MSGID_COMMAND_COMMON` message uses subtypes:
- `DEF_COMMONTYPE_ITEMDROP` - Drop item
- `DEF_COMMONTYPE_EQUIPITEM` - Equip item
- `DEF_COMMONTYPE_MAGIC` - Cast spell
- `DEF_COMMONTYPE_REQ_USEITEM` - Use consumable
- `DEF_COMMONTYPE_REQ_USESKILL` - Use skill
- `DEF_COMMONTYPE_TOGGLECOMBATMODE` - Toggle combat
- `DEF_COMMONTYPE_BUILDITEM` - Craft item
- ... (58+ command types)

---

## Dialog System

### Dialog Box Indices

| Index | Dialog Type | Default Position | Size |
|-------|-------------|------------------|------|
| 1 | Character Info (F5) | (30, 30) | 270x376 |
| 2 | Inventory (F6) | (380, 210) | 225x185 |
| 3 | Magic Circle (F7) | (337, 57) | 258x328 |
| 4 | Item Drop (legacy) | (0, 0) | 270x105 |
| 5 | 15Age Msg / Feedback | (0, 0) | 310-320x170-320 |
| 6 | Warning Msg | (0, 0) | 310x170 |
| 7 | Guild Menu | (337, 57) | 258x339 |
| 8 | Guild Operation | (337, 57) | 295x346 |
| 9 | Guide Map | (512, 0) | 128x128 |
| 10 | Chat (F9) | (135, 273) | 364x162 |
| 11 | Shop/Store | (70, 50) | 258x339 |
| 12 | Level-Up Setting | (0, 0) | 258x339 |
| 13 | City Hall Menu | (337, 57) | 258x339 |
| 14 | Bank | (60, 50) | 258x339 |
| 15 | Skills (F8) | (337, 57) | 258x339 |
| 16 | Magic Shop | (30, 30) | 304x328 |
| 17 | Drop Item Amount | (0, 0) | 215x87 |
| 18 | Text Display | (5, 65) | 258x339 |
| 19 | System Menu (F12) | (337, 107) | 258x268 |
| 20 | NPC Action Query | (237, 57) | 252x87 |
| 21 | NPC Talk | (337, 57) | 258x339 |
| 22 | World Map | (336, 88) | 270x346 |
| 23 | Sell/Repair | (337, 57) | 258x339 |
| 24 | Fishing | (193, 241) | 263x100 |
| 25 | Shutdown Msg | (162, 40) | 315x171 |
| 26 | Manufacture | (100, 60) | 258x339 |
| 27 | Exchange | (100, 30) | 520x357 |
| 28 | Quest | (0, 0) | 258x339 |
| 29 | Gauge Panel | (0, 434) | 157x53 |
| 30 | Icon Panel | (0, 427) | 640x53 |
| 31 | Sell List | (170, 70) | 258x339 |
| 32 | Party | (0, 0) | 258x339 |
| 33 | Crusade Job | (360, 65) | 258x339 |
| 34 | Item Upgrade | (60, 50) | 258x339 |
| 35 | Help (F1) | (358, 65) | 258x339 |
| 36 | Commander | (20, 20) | 310x386 |
| 37 | Constructor | (20, 20) | 310x386 |
| 38 | Soldier | (20, 20) | 310x386 |
| 39 | Reserved | (0, 0) | 291x413 |
| 40 | Feedback Card | - | - |

### Dialog Z-Order

Dialog stacking is managed through `m_cDialogBoxOrder[42]`:
- Index 40 = Gauge Panel (#29) - Always on top
- Index 39 = Icon Panel (#30) - Always on top
- Lower indices = Higher z-order (drawn last)

---

## Combat and Magic

### Attack Types

Determined by `_iGetAttackType()`:
- 0 = Unarmed
- 1 = Short sword
- 2 = Long sword
- 3 = Axe
- 4 = Bow
- 5 = Staff
- ... (type-specific attack animations)

### Weapon Skill Types

Determined by `_iGetWeaponSkillType()`:
- Returns skill ID based on equipped weapon

### Magic System

- 100 spell slots (`m_cMagicMastery[100]`)
- Mana cost calculation via `iGetManaCost()`
- Spell casting via `UseMagic()`
- 23 magic effect types (defined in Magic.h)

### Combat Modes

- Normal attack
- Safe attack (`m_bIsSafeAttackMode`) - No friendly fire
- Super attack (`m_bSuperAttackMode`) - Special attacks
- Force attack (`m_bForceAttack`) - Attack anything

---

## Inventory and Items

### Inventory Structure

- 50 inventory slots (`DEF_MAXITEMS`)
- 121 bank slots (`DEF_MAXBANKITEMS`)
- 15 equipment slots (defined by `DEF_MAXITEMEQUIPPOS`)

### Equipment Slots

1. Head
2. Body
3. Arms
4. Pants
5. Boots
6. Neck
7. Left Hand
8. Right Hand
9. Two-Hand
10. Right Finger
11. Left Finger
12. Back
13. Full Body
14-15. Reserved

### Item Operations

- Equip/unequip via `ItemEquipHandler()`/`ReleaseEquipHandler()`
- Drag-drop via `bItemDrop_*` methods
- Bank storage via dialog #14
- Item trading via Exchange dialog (#27)

---

## Guild System

### Guild Limits

- 32 members maximum
- 100 guild names cached for display

### Guild Operations

- Create: `MSGID_REQUEST_CREATENEWGUILD`
- Disband: `MSGID_REQUEST_DISBANDGUILD`
- Join approval: `DEF_COMMONTYPE_JOINGUILDAPPROVE`
- Join rejection: `DEF_COMMONTYPE_JOINGUILDREJECT`
- Dismiss: `DEF_COMMONTYPE_DISMISSGUILDAPPROVE/REJECT`
- Ban: `DEF_COMMONTYPE_BANGUILD`

---

## Party System

### Party Limits

- 8 members maximum

### Party Operations

- Create: `DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY`
- Join: `DEF_COMMONTYPE_REQUEST_JOINPARTY`
- Response: `DEF_COMMONTYPE_RESPONSE_JOINPARTY`
- Leave: Handled via `DEF_NOTIFY_PARTY` subtype 6

---

## Chat System

### Message Types

- Normal
- Shout (area-wide)
- Whisper (private)
- Guild
- Party
- System
- GM (Game Master)

### Chat Features

- 500 message history
- 80 scrollable messages
- 5 whisper slots
- Bad word filtering via `_bCheckBadWords()`
- Local commands via `bCheckLocalChatCommand()`

### Local Chat Commands

Handled in `bCheckLocalChatCommand()`:
- `/showframe` - Toggle FPS display
- `/fi` - Show character coordinates
- Other debug commands

---

## Map and World

### Map Grid

- 752x752 total tile map
- 40x35 visible area per section
- 32x32 pixel tiles
- 25x19 tiles visible on screen (800x600 / 32)

### View System

- Smooth scrolling with velocity (`m_sViewDX`, `m_sViewDY`)
- Camera follows player
- Camera shaking for effects

### Map Data Structure

Stored in `CMapData` class:
- Terrain tiles
- Object placement
- Walkability flags
- Dynamic objects

---

## Effects and Animation

### Effect Types

- 300 concurrent effects maximum
- 100 effect sprite frames
- Effect types defined in `Effect.h`

### Weather System

- 600 weather objects maximum
- 7 weather types (`m_cWhetherEffectType`)
- Weather status (`m_cWhetherStatus`)

### Animation

- Frame-based animation
- Direction-based sprites (8 directions)
- Action states (15 types)

---

## Localization

### Supported Languages

| Code | Language | Header |
|------|----------|--------|
| 1 | Taiwan/Traditional Chinese | lan_tai.h |
| 2 | Simplified Chinese | lan_chi.h |
| 3 | Korean | lan_kor.h |
| 4 | English | lan_eng.h |
| 5 | Japanese | lan_jap.h |

### Compile-Time String Selection

All UI strings are `#define` macros in language-specific headers, selected at compile time based on `DEF_LANGUAGE`.

---

## Configuration

### Settings Storage

Settings stored in Windows Registry:
```
HKEY_CURRENT_USER\Software\Siementech\Helbreath\Settings
```

Values:
- `Magic` - Magic shortcut assignment
- `ShortCut0-4` - Quick slot assignments

### Login Configuration

`login.cfg` file contains:
- Login server address
- Login server port
- Other connection settings

### Item Names

`ItemName.cfg` contains:
- 1000 item name entries
- Loaded at startup

---

## External Dependencies

### DirectX 7.0a Components

- DirectDraw 7 (`ddraw.lib`)
- DirectInput 7 (`dinput.lib`)
- DirectSound 7 (`dsound.lib`)

### Windows Libraries

- WinSock2 (`ws2_32.lib`)
- Windows Multimedia (`winmm.lib`)
- GDI32 (`gdi32.lib`)

### PAK File Format

Custom archive format for game assets:
- Sprites
- Sounds
- Map data
- Content files

---

## Known Issues and Limitations

### Architectural Issues

1. **Monolithic design** - All functionality in single class
2. **No separation of concerns** - Rendering, logic, networking intertwined
3. **Global state** - Heavy use of member variables as global state
4. **Thread-safety** - Limited; `CInt` wrapper used for some variables
5. **Memory management** - Manual allocation/deallocation prone to leaks

### Code Quality Issues

1. **Magic numbers** - Many hardcoded values
2. **Deep nesting** - Switch statements 1000+ lines
3. **Code duplication** - Similar patterns repeated
4. **No error handling** - Many operations don't check return values
5. **Mixed languages** - Korean comments in English codebase

### Technical Debt

1. **DirectX 7** - Obsolete API (1999)
2. **16-bit color** - RGB555/RGB565 only
3. **Fixed resolution** - 800x600 hardcoded
4. **WinSock2** - No modern async patterns
5. **No configuration** - Most values hardcoded

---

## Modernization Notes

When refactoring this code:

1. **Split into subsystems** matching the target architecture
2. **Extract constants** to configuration files
3. **Implement proper state machine** for game modes
4. **Use modern networking** (asio or similar)
5. **Abstract graphics API** for D3D11 or SDL2
6. **Implement ECS** for entity management
7. **Use smart pointers** for all dynamic allocations
8. **Add unit tests** for game logic
9. **Implement localization** as runtime loading

---

*Document Version: 1.0*
*Generated from: Helbreath Client Source Code Analysis*
*Lines Analyzed: ~49,000*
