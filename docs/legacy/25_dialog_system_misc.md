# Miscellaneous System Dialogs

## Overview

This document covers all system-level dialogs in the legacy Helbreath client that handle game settings, help information, warnings, and special notifications. These dialogs are not directly related to gameplay content but manage the player's interaction with the game client itself.

**Dialogs Covered:**
- System Menu (Dialog 19) - Settings, logout, volume controls
- Help Dialog (Dialog 35) - In-game help topics
- Warning Message (Dialog 6) - Danger zone notifications
- Age Verification (Dialog 5) - 15+ age check / Feedback cards
- Shutdown Notification (Dialog 25) - Server maintenance alerts
- Feedback Card (Dialog 40) - Empty stub implementation

---

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` | Dialog index declarations, member variables |
| `Game.cpp` | `DrawDialogBox_*` and `DlgBoxClick_*` implementations |
| `GlobalDef.h` | Conditional compilation flags, sprite IDs |
| `lan_eng.h` | English localization strings |
| `lan_kor.h` | Korean localization strings |
| `lan_chi.h` | Chinese localization strings |
| `lan_jap.h` | Japanese localization strings |

---

## Common Dialog Infrastructure

### DialogBoxInfo Structure

All dialogs share a common info structure stored in `m_stDialogBoxInfo[41]`:

```cpp
struct DialogBoxInfo {
    int   sV1, sV2, sV3, sV4, sV5, sV6, sV7, sV8;
    int   sV9, sV10, sV11, sV12, sV13, sV14;
    DWORD dwV1, dwV2, dwT1;
    BOOL  bFlag;
    short sX, sY;           // Dialog position
    short sSizeX, sSizeY;   // Dialog dimensions
    short sView;            // Current view/page/scroll position
    char  cStr[32];         // String data slot 1
    char  cStr2[32];        // String data slot 2
    char  cStr3[32];        // String data slot 3
    char  cStr4[32];        // String data slot 4
    char  cMode;            // State/mode flag
    BOOL  bIsScrollSelected;
};
```

### Button Position Constants

```cpp
#define DEF_BTNSZX      74    // Button width in pixels
#define DEF_BTNSZY      20    // Button height in pixels
#define DEF_LBTNPOSX    30    // Left button X offset from dialog origin
#define DEF_RBTNPOSX    154   // Right button X offset from dialog origin
#define DEF_BTNPOSY     292   // Standard button Y offset from dialog origin
```

### Common Sprite IDs

```cpp
DEF_SPRID_INTERFACE_ND_GAME1    // Generic dialog background
DEF_SPRID_INTERFACE_ND_GAME2    // Dialog background variant (help)
DEF_SPRID_INTERFACE_ND_GAME4    // Alert/notification background
DEF_SPRID_INTERFACE_ND_TEXT     // Text overlay sprite
DEF_SPRID_INTERFACE_ND_BUTTON   // Button sprite sheet
```

---

## 1. System Menu Dialog (ID: 19)

### Purpose

The System Menu provides access to game settings, audio controls, chat filters, and logout functionality. It is the primary settings interface during gameplay.

### Function Signatures

```cpp
void CGame::DrawDialogBox_SysMenu(short msX, short msY, char cLB);
void CGame::DlgBoxClick_SysMenu(short msX, short msY);
```

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `m_cDetailLevel` | char | Graphics detail: 0=Low, 1=Normal, 2=High |
| `m_bSoundFlag` | BOOL | Sound hardware available |
| `m_bSoundStat` | BOOL | Sound effects enabled |
| `m_bMusicStat` | BOOL | Background music enabled |
| `m_cSoundVolume` | char | Sound volume (0-100) |
| `m_cMusicVolume` | char | Music volume (0-100) |
| `m_bWhisper` | BOOL | Whisper messages enabled |
| `m_bShout` | BOOL | Shout messages enabled |
| `m_bDialogTrans` | BOOL | Dialog transparency enabled |
| `m_bIsDialogEnabled[9]` | BOOL | Guide map visibility |
| `m_cLogOutCount` | char | Logout countdown (-1 = ready) |
| `m_cRestartCount` | char | Restart countdown (-1 = ready) |
| `m_dwRestartCountTime` | DWORD | Restart initiation timestamp |
| `m_bForceDisconn` | CInt | Server forcing disconnect |

### UI Layout

```
+------------------------------------------+
|  System Menu                             |
+------------------------------------------+
|  World Server: [WS1-WS16 name]           |
|                                          |
|  Detail Level: [Low] [Normal] [High]     |
|                                          |
|  Sound: [On/Off]    Music: [On/Off]      |
|                                          |
|  Whisper: [On/Off]  Shout: [On/Off]      |
|                                          |
|  Sound Volume: [====|======] 50          |
|  Music Volume: [========|==] 80          |
|                                          |
|  [ ] Dialog Transparency                 |
|  [ ] Show Guide Map                      |
|                                          |
|  Local Time: 01:15:23:45:30              |
|                                          |
|  [Log Out]              [Restart]        |
+------------------------------------------+
```

### Control Areas and Behaviors

#### Detail Level (Lines 41773-41785)

| Setting | Click Area | Value |
|---------|------------|-------|
| Low | sX+120 to sX+150, sY+63 to sY+74 | 0 |
| Normal | sX+151 to sX+200, sY+63 to sY+74 | 1 |
| High | sX+201 to sX+234, sY+63 to sY+74 | 2 |

**Visual Feedback:**
- Selected: White text (255, 255, 255)
- Unselected: Dark text (45, 25, 25)
- Click plays sound effect 'E', index 14

#### Sound Toggle (Lines 41786-41792)

| State | Condition | Color |
|-------|-----------|-------|
| On | `m_bSoundStat == TRUE` | White (255, 255, 255) |
| Off | `m_bSoundStat == FALSE` | Gray (200, 200, 200) |
| Disabled | `m_bSoundFlag == FALSE` | Dark Gray (100, 100, 100) |

**Click Area:** sX+24 to sX+115, sY+81 to sY+100
**Special Behavior:** Toggling off stops `m_pESound[38]`

#### Music Toggle (Lines 41794-41800)

Same state/color logic as Sound Toggle.
**Click Area:** sX+116 to sX+202, sY+81 to sY+100
**Special Behavior:** Stops/restarts `m_pBGM` background music player

#### Whisper Toggle (Lines 41803-41806)

**Click Area:** sX+23 to sX+108, sY+108 to sY+119
**Events:** Adds `BCHECK_LOCAL_CHAT_COMMAND6` or `BCHECK_LOCAL_CHAT_COMMAND7` to event list

#### Shout Toggle (Lines 41808-41811)

**Click Area:** sX+123 to sX+203, sY+108 to sY+119
**Events:** Adds `BCHECK_LOCAL_CHAT_COMMAND8` or `BCHECK_LOCAL_CHAT_COMMAND9` to event list

#### Volume Sliders (Lines 41813-41819)

**Sound Volume:**
- Click Area: sX+127 to sX+238, sY+122 to sY+138
- Slider Position: sX + 130 + m_cSoundVolume
- Sprite: `DEF_SPRID_INTERFACE_ND_GAME2`, frame 8

**Music Volume:**
- Click Area: sX+127 to sX+238, sY+139 to sY+155
- Slider Position: sX + 130 + m_cMusicVolume
- Real-time DirectSound adjustment:
  ```cpp
  iVol = (m_cMusicVolume - 100) * 20;  // Range: -2000 to 0
  // Clamped to DirectSound range: -10000 to 0
  m_pBGM->SetVolume(iVol);
  ```

#### Dialog Transparency (Lines 41821-41824)

**Click Area:** sX+28 to sX+235, sY+156 to sY+171
**Variable:** `m_bDialogTrans`
**Effect:** Toggles alpha blending on all dialog boxes

#### Guide Map Toggle (Lines 41826-41829)

**Click Area:** sX+28 to sX+235, sY+178 to sY+193
**Variable:** `m_bIsDialogEnabled[9]`
**Functions Called:**
- Enable: `EnableDialogBox(9, 0, 0, 0, NULL)`
- Disable: `DisableDialogBox(9)`

#### World Server Display (Lines 41845-41915)

Displays connected world server name using message constants:
- `MSG_WORLDNAME1` through `MSG_WORLDNAME16`
- Position: sX + 23, sY + 46

#### Local Time Display (Lines 41832-41841)

Uses `GetLocalTime()` Windows API.

**Korean Format:** `"Month일 Day일 Hour시 Minute분 Second초"`
**Other Languages:** `"Month:Day:Hour:Minute:Second"`

Position: sX + 23, sY + 204

### Buttons

#### Log Out / Continue Button

| State | Label | Condition |
|-------|-------|-----------|
| Log Out | Ready to logout | `m_cLogOutCount == -1` |
| Continue | Cancel logout | `m_cLogOutCount != -1` |

**Position:** sX + DEF_LBTNPOSX (30), sY + 225
**Button Frames:** 6/7 (Continue), 8/9 (Log Out)

**Click Behavior:**
```cpp
if (m_cLogOutCount == -1) {
    m_cLogOutCount = 11;  // Start 11-second countdown (1 in debug)
} else {
    m_cLogOutCount = -1;  // Cancel logout
    DisableDialogBox(19);
    AddEventList(MSG_LOGOUT_CANCELLED);
}
```

**Logout Prevention:** If `m_bForceDisconn == TRUE`, logout is blocked (server disconnecting)

#### Restart Button

**Visibility Condition:** `m_iHP <= 0 && m_cRestartCount == -1`
**Position:** sX + DEF_RBTNPOSX (154), sY + 225
**Button Frames:** 36/37

**Click Behavior:**
```cpp
m_cRestartCount = 5;  // 5-second countdown
m_dwRestartCountTime = timeGetTime();
AddEventList(MSG_RESTART_COUNTDOWN);
DisableDialogBox(19);
```

### Background Rendering

- Base: `DEF_SPRID_INTERFACE_ND_GAME1`, frame 0
- Text area: `DEF_SPRID_INTERFACE_ND_TEXT`, frame 6

---

## 2. Help Dialog (ID: 35)

### Purpose

Provides access to in-game help topics covering all major game systems. Clicking a topic opens the Text Dialog (ID: 18) with the relevant help content.

### Function Signatures

```cpp
void CGame::DrawDialogBox_Help(int msX, int msY);
void CGame::DlgBoxClick_Help(int msX, int msY);
```

### Help Topics

| # | Topic | Mode | Description |
|---|-------|------|-------------|
| 1 | Helbreath World? | 900 | Introduction to the game world |
| 2 | Notices | 1000 | Game announcements (web dialog if DEF_HTMLCOMMOM) |
| 3 | How to Move | 901 | Movement controls |
| 4 | Attack, Defence, Enemy, Friend | 902 | Combat basics |
| 5 | Interface | 903 | UI explanation (sets `m_bIsF1HelpWindowEnabled`) |
| 6 | Magic System | 904 | Spellcasting guide |
| 7 | Skills | 905 | Skill system overview |
| 8 | Guilds | 906 | Guild mechanics |
| 9 | Items/Equipment | 907 | Item management |
| 10 | Communication | 908 | Chat system |
| 11 | Player Status | 909 | Character stats |
| 12 | Crusade System | 910 | War mechanics |
| 13 | Commands Reference (F.A.Q.) | 911 | Slash commands |
| 14 | Beginner's Guide | 912 | New player tutorial |
| 15 | Hotkey Reference 1 | 913 | Korean only (DEF_LANGUAGE == 3) |
| 16 | Hotkey Reference 2 | 914 | Korean only (DEF_LANGUAGE == 3) |

### UI Layout

```
+----------------------------------+
|  Help                            |
+----------------------------------+
|  > Helbreath World?              |
|  > Notices                       |
|  > How to Move                   |
|  > Attack, Defence, Enemy...     |
|  > Interface                     |
|  > Magic System                  |
|  > Skills                        |
|  > Guilds                        |
|  > Items/Equipment               |
|  > Communication                 |
|  > Player Status                 |
|  > Crusade System                |
|  > Commands Reference            |
|  > Beginner's Guide              |
|                                  |
|                      [Close]     |
+----------------------------------+
```

### Click Areas

Each menu item:
- X range: sX + 25 to sX + 248
- Y range: sY + 50 + (item * 15) to sY + 50 + (item * 15) + 15
- Vertical spacing: 15 pixels per item

### Visual Feedback

| State | Color |
|-------|-------|
| Hover | White (255, 255, 255) |
| Normal | Dark Blue (4, 0, 50) |

### Click Behavior

```cpp
// General pattern for each help item:
DisableDialogBox(18);  // Close any open text dialog
EnableDialogBox(18, NULL, NULL, NULL);
m_stDialogBoxInfo[18].sV1 = MODE_CODE;  // e.g., 901 for "How to Move"
```

### Close Button

**Position:** sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY
**Button Frames:** 0/1
**Action:** Plays sound 'E' index 14, disables dialog 35

### Background

Sprite: `DEF_SPRID_INTERFACE_ND_GAME2`, frame 2

---

## 3. Warning Message Dialog (ID: 6)

### Purpose

Displays a warning when the player enters a dangerous PvP zone where they are not protected from attacks.

### Function Signatures

```cpp
void CGame::DrawDialogBox_WarningMsg(short msX, short msY);
void CGame::DlgBoxClick_WarningMsg(short msX, short msY);
```

### Content

```
+------------------------------------------+
|                                          |
|      ** This is a battle area **         |
|                                          |
|  This is a dangerous area where you      |
|  cannot protected from others' attack.   |
|  To play the game in safe, go to the     |
|  cityhall and change to civilian mode.   |
|                                          |
|              [OK]                         |
+------------------------------------------+
```

### Text Rendering

| Line | Position | Constant | Color |
|------|----------|----------|-------|
| Title | sX+63, sY+35 | `DEF_MSG_WARNING1` | RGB(200, 200, 25) |
| Line 1 | sX+30, sY+57 | `DEF_MSG_WARNING2` | RGB(220, 130, 45) |
| Line 2 | sX+30, sY+74 | `DEF_MSG_WARNING3` | RGB(220, 130, 45) |
| Line 3 | sX+30, sY+92 | `DEF_MSG_WARNING4` | RGB(220, 130, 45) |
| Line 4 | sX+30, sY+110 | `DEF_MSG_WARNING5` | RGB(220, 130, 45) |

**Japanese Exception:** Line positions start at sX+59 instead of sX+30

### OK Button

**Position:** sX + 122, sY + 127
**Action:** Disables dialog 6

### Background

Sprite: `DEF_SPRID_INTERFACE_ND_GAME4`, frame 2

---

## 4. Age Verification / Feedback Card Dialog (ID: 5)

### Purpose

Dual-purpose dialog that either displays:
1. Age verification message (15+ rating notice) - Default behavior
2. Feedback cards for surveys - When `DEF_FEEDBACKCARD` is defined

### Function Signatures

```cpp
void CGame::DrawDialogBox_15AgeMsg(short msX, short msY);
void CGame::DlgBoxClick_15AgeMsg(short msX, short msY);
```

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `m_iFeedBackCardIndex` | int | Which feedback card to show (1-7, -1=none) |

### Mode 1: Age Verification (Default)

Displayed when `DEF_FEEDBACKCARD` is NOT defined.

```
+------------------------------------------+
|                                          |
|  2003년 1월 16일 게임의법은 '인터넷      |
|  보호 센터'의 공식적인 권고에 따른       |
|  자체 검수 체계를 통해 15세 이상만       |
|  플레이할 수 있도록 하고 있습니다.       |
|  혹시 문의 사항이 있으시면 홈페이지      |
|  방문해 주세요.                          |
|                                          |
|              [OK]                         |
+------------------------------------------+
```

**Text Position:** sX + 30, starting at sY + 26, 17-pixel line spacing
**Color:** RGB(200, 200, 45) - Yellow

### Mode 2: Feedback Cards (Conditional)

Displayed when `DEF_FEEDBACKCARD` is defined (Chinese version).

**Feedback Card Sprites:**

| Index | Sprite Constant |
|-------|-----------------|
| 1 | `DEF_SPRID_INTERFACE_FEEDBACK1` |
| 2 | `DEF_SPRID_INTERFACE_FEEDBACK2` |
| 3 | `DEF_SPRID_INTERFACE_FEEDBACK3` |
| 4 | `DEF_SPRID_INTERFACE_FEEDBACK4` |
| 5 | `DEF_SPRID_INTERFACE_FEEDBACK5` |
| 6 | `DEF_SPRID_INTERFACE_FEEDBACK6` |
| 7 | `DEF_SPRID_INTERFACE_FEEDBACK7` |

**Click Behavior:** Sets `m_iFeedBackCardIndex = -1` and disables dialog 5

### Early Return

If `m_iFeedBackCardIndex == -1`, the rendering function returns immediately without drawing.

### OK Button

**Position:** sX + 122, sY + 127
**Action:** Disables dialog 5 (or resets feedback index in FEEDBACKCARD mode)

---

## 5. Shutdown Notification Dialog (ID: 25)

### Purpose

Notifies players of imminent server shutdown for maintenance, giving them time to safely log out.

### Function Signatures

```cpp
void CGame::DrawDialogBox_ShutDownMsg(short msX, short msY);
void CGame::DlgBoxClick_ShutDownMsg(short msX, short msY);
```

### Mode 1: Shutdown Warning (Countdown)

**Condition:** `m_stDialogBoxInfo[25].cMode == 1`

```
+------------------------------------------+
|                                          |
|  GameServer will be shut down after      |
|  [X] Minutes!                            |
|                                          |
|  You have to logout now to save your     |
|  data safely. After beginning of         |
|  shutdown, user's data are not saved     |
|  and the lost data cannot be recovered.  |
|  So be careful!!                         |
|                                          |
|                            [OK]          |
+------------------------------------------+
```

**Dynamic Countdown:** `m_stDialogBoxInfo[25].sV1` contains minutes remaining
- If sV1 != 0: Uses `DRAW_DIALOGBOX_NOTICEMSG1` with %d format
- If sV1 == 0: Uses `DRAW_DIALOGBOX_NOTICEMSG2` ("soon!")

### Mode 2: Shutdown In Progress

**Condition:** `m_stDialogBoxInfo[25].cMode == 2`

```
+------------------------------------------+
|                                          |
|  GameServer has been shut down!!!        |
|                                          |
|  Your connection will be lost            |
|  automatically soon. Don't drop items    |
|  or give them other people. Data could   |
|  be lost. And lost data cannot be        |
|  recovered.                              |
|                                          |
|                            [OK]          |
+------------------------------------------+
```

### Message Constants

| Constant | Content |
|----------|---------|
| `DRAW_DIALOGBOX_NOTICEMSG1` | "GameServer will be shut down after %d Minutes!" |
| `DRAW_DIALOGBOX_NOTICEMSG2` | "GameServer will be shut down soon!" |
| `DRAW_DIALOGBOX_NOTICEMSG3` | "You have to logout now to save your data" |
| `DRAW_DIALOGBOX_NOTICEMSG4` | "safely. After beginning of shutdown, user's" |
| `DRAW_DIALOGBOX_NOTICEMSG5` | "data are not saved and the lost data cannot" |
| `DRAW_DIALOGBOX_NOTICEMSG6` | "be recovered. So be careful!!" |
| `DRAW_DIALOGBOX_NOTICEMSG7` | "GameServer has been shut down!!!" |
| `DRAW_DIALOGBOX_NOTICEMSG8` | "Your connection will be lost automatically" |
| `DRAW_DIALOGBOX_NOTICEMSG9` | "soon. Don't drop items or give them other" |
| `DRAW_DIALOGBOX_NOTICEMSG10` | "Data could be lost. And lost data" |
| `DRAW_DIALOGBOX_NOTICEMSG11` | "cannot be recovered." |

### Text Rendering

All messages use:
- Color: RGB(100, 10, 10) - Dark Red
- Alignment: `PutAlignedString()` centered across dialog width
- Line spacing: 17 pixels

### OK Button

**Position:** sX + 210, sY + 127
**Action:** Disables dialog 25

---

## 6. Feedback Card Dialog (ID: 40)

### Purpose

Intended for feedback form submission. Currently an empty stub.

### Function Signatures

```cpp
void CGame::DrawDialogBox_FeedBackCard(short msX, short msY);
void CGame::DlgBoxClick_FeedBackCard(short msX, short msY);
```

### Implementation

```cpp
void CGame::DrawDialogBox_FeedBackCard(short msX, short msY)
{
    // Empty - no rendering implemented
}

void CGame::DlgBoxClick_FeedBackCard(short msX, short msY)
{
    // Empty - no click handling implemented
}
```

### Notes

- Related to `DEF_FEEDBACKCARD` conditional compilation
- Actual feedback functionality implemented through Dialog 5 (Age Verification)
- This dialog index appears unused in practice

---

## Conditional Compilation Flags

| Flag | Effect | Language |
|------|--------|----------|
| `DEF_FEEDBACKCARD` | Enables feedback card system in Dialog 5 | Chinese (2) |
| `DEF_HTMLCOMMOM` | Uses web browser for "Notices" help item | Chinese (2) |
| `DEF_TESTSERVER` | Shows "TEST SERVER" in SysMenu | Debug builds |
| `DEF_LANGUAGE == 3` | Adds Korean-specific help items (913-914) | Korean |
| `DEF_LANGUAGE == 5` | Adjusts warning message positioning | Japanese |
| `_DEBUG` | Logout countdown = 1 instead of 11 | Debug builds |

---

## Language-Specific Variations

### Korean (DEF_LANGUAGE == 3)

- Time format: `"Month일 Day일 Hour시 Minute분 Second초"`
- Two additional help topics (Hotkey Reference 1 & 2)
- World server names in Korean

### Japanese (DEF_LANGUAGE == 5)

- Warning message text positioned at sX+59 (centered differently)

### Chinese (DEF_LANGUAGE == 2)

- Feedback card system available via DEF_FEEDBACKCARD
- Web dialog support via DEF_HTMLCOMMOM

### English (DEF_LANGUAGE == 4)

- Default behavior for all dialogs
- Time format: `"Month:Day:Hour:Minute:Second"`

---

## Integration Points

### Dialog Enable/Disable

```cpp
void CGame::EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char* pString);
void CGame::DisableDialogBox(int iBoxID);
```

### Event System Integration

System Menu generates events for chat filter changes:
- `BCHECK_LOCAL_CHAT_COMMAND6` / `7` - Whisper toggle
- `BCHECK_LOCAL_CHAT_COMMAND8` / `9` - Shout toggle

### Audio System Integration

System Menu directly controls:
- `m_pBGM` - Background music player (CBGMPlayer)
- `m_pESound[38]` - Ambient sound effect
- DirectSound volume: `SetVolume((volume - 100) * 20)`

### Help System Chaining

Help Dialog enables Text Dialog (18) with mode codes:
- Modes 900-914 correspond to different help topics
- F1 Help Window flag set when "Interface" topic selected

---

## State Management

### Logout Sequence

```
State 1: m_cLogOutCount = -1 (Ready)
         Button shows "Log Out"

State 2: m_cLogOutCount = 11 (or 1 in debug)
         Button shows "Continue"
         Countdown decrements each second

State 3: m_cLogOutCount = 0
         Logout executed, connection closed

Cancel:  Click "Continue" -> m_cLogOutCount = -1
```

### Restart Sequence (Death)

```
Condition: m_iHP <= 0 AND m_cRestartCount == -1

State 1: Click Restart
         m_cRestartCount = 5
         m_dwRestartCountTime = current time

State 2: Countdown decrements

State 3: m_cRestartCount = 0
         Respawn packet sent to server
```

---

## Known Issues / Technical Debt

1. **Empty Feedback Card Dialog** - Dialog 40 is a stub with no implementation
2. **Hardcoded Positions** - All click areas use magic numbers instead of constants
3. **Language-Specific Code Paths** - Multiple `#if DEF_LANGUAGE ==` blocks scattered throughout
4. **DirectSound Direct Access** - Volume control bypasses any audio abstraction
5. **Global State** - Settings stored in CGame member variables with no persistence layer
6. **Mixed Coordinate Systems** - Some positions relative to dialog, some absolute

---

## Modernization Notes

### Recommended Changes

1. **Settings Persistence** - Save audio/visual settings to config file
2. **Abstract Audio Control** - Route volume changes through AudioSystem
3. **Help Content Loading** - Load help text from external files instead of compiled strings
4. **Dialog Factory** - Create dialogs from data definitions (YAML/JSON)
5. **Event-Driven Settings** - Publish setting changes via EventBus for subsystem decoupling
6. **Localization System** - Replace compile-time strings with runtime localization
7. **Remove Feedback Stub** - Either implement or remove Dialog 40

### C++20 Opportunities

```cpp
// Volume slider with std::clamp
m_cSoundVolume = std::clamp(newVolume, 0, 100);

// Settings struct with designated initializers
struct GameSettings {
    int detailLevel = 1;
    bool soundEnabled = true;
    bool musicEnabled = true;
    int soundVolume = 50;
    int musicVolume = 50;
    bool whisperEnabled = true;
    bool shoutEnabled = true;
    bool dialogTransparency = false;
};

// Help topics as constexpr array
constexpr std::array<HelpTopic, 14> helpTopics = {{
    {900, "Helbreath World?"},
    {901, "How to Move"},
    // ...
}};
```
