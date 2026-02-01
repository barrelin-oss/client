# Navigation Dialogs

## Overview

The Helbreath client provides several navigation-related dialogs to help players orient themselves in the game world. These include the mini-map (Guide Map), the full world map display, and the teleportation system accessed through City Hall NPCs. The navigation system uses a combination of pre-rendered map sprites and dynamically calculated player positions.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | All dialog drawing and input handling functions |
| `Game.h` | Dialog state variables and teleport data structures |
| `MapData.h` | Map index constants and definitions |
| `NetMessages.h` | Teleportation protocol message IDs |
| `lan_*.h` | Localized UI strings for navigation elements |

## Dialog Types

### Dialog Index Reference

| Index | Name | Purpose |
|-------|------|---------|
| 9 | Guide Map | Mini-map with zoom levels |
| 13 | City Hall Menu | Contains teleportation submenu (mode 10) |
| 20 | NPC Action Query | NPC interaction menu |
| 21 | NPC Talk | NPC dialogue display |
| 22 | World Map | Full map display |

---

## 1. Guide Map Dialog (Index 9)

### Overview

The Guide Map is a 128x128 pixel mini-map that shows the player's current location within the game world. It supports two zoom levels and displays named locations for major cities.

### Display Function

```cpp
void CGame::DrawDialogBox_GuideMap(short sX, short sY, int iFrame)
// Location: Game.cpp:17871
```

### Rendering Modes

| Mode | Description | Sprite Used |
|------|-------------|-------------|
| Zoomed Out | Full world view at reduced scale | `DEF_SPRID_INTERFACE_GUIDEMAP + m_cMapIndex` |
| Zoomed In | 128x128 tile area centered on player | `DEF_SPRID_INTERFACE_GUIDEMAP + m_cMapIndex + 1` |

### Player Position Marker

The player's position is marked using sprite #37 from `DEF_SPRID_INTERFACE_ND_CRUSADE`. Position calculation:

```cpp
// Zoomed out mode - scale to 128x128 display
int playerX = (m_sPlayerX * 128) / mapWidth;
int playerY = (m_sPlayerY * 128) / mapHeight;

// Zoomed in mode - offset from center
int playerX = 64 + (m_sPlayerX - centerX);
int playerY = 64 + (m_sPlayerY - centerY);
```

### Grid Lines

When zoomed in, grid lines are drawn every 32 pixels to help with orientation.

### Named Location Overlays

The Guide Map displays location names when the player's position falls within predefined coordinate ranges. These are hardcoded per map.

#### Aresden (m_cMapIndex == 11)

| Location | X Range | Y Range |
|----------|---------|---------|
| Cityhall | 165-225 | 136-175 |
| Magic Tower | 47-76 | 134-165 |
| Dungeon | 99-124 | 204-227 |
| Dungeon (alt) | 175-202 | 97-120 |
| Warehouse | 125-157 | 185-218 |
| Warehouse (alt) | 262-290 | 148-178 |
| Barrack | 147-184 | 65-97 |
| Guild Hall | 155-185 | 110-136 |
| Shop | 166-193 | 185-211 |
| Blacksmith | 201-229 | 220-245 |
| Cathedral | 205-249 | 98-140 |

#### Elvine (m_cMapIndex == 3)

| Location | X Range | Y Range |
|----------|---------|---------|
| Magic Tower | 77-114 | 81-114 |
| Guild Hall | 88-120 | 151-183 |
| Cathedral | 126-171 | 97-141 |
| Cityhall | 157-194 | 150-190 |
| Barrack | 171-207 | 76-107 |
| Dungeon | 207-231 | 99-124 |
| Dungeon (alt) | 301-330 | 239-265 |
| Warehouse | 247-277 | 139-170 |
| Warehouse (alt) | 237-270 | 225-258 |
| Shop | 258-287 | 109-137 |
| Blacksmith | 302-333 | 147-175 |

#### Aresden Dungeon (m_cMapIndex == 5)

| Location | X Range | Y Range |
|----------|---------|---------|
| Warehouse | 62-78 | 178-192 |
| Shop | 82-95 | 163-174 |
| Blacksmith | 107-122 | 177-189 |

#### Elvine Dungeon (m_cMapIndex == 6)

| Location | X Range | Y Range |
|----------|---------|---------|
| Warehouse | 35-48 | 70-85 |
| Blacksmith | 55-73 | 77-90 |
| Shop | 53-66 | 53-65 |

### Monster Event Indicator

When a monster event is active and the event time is less than 30 seconds, a special indicator is shown on the map.

### Input Handling

| Input | Action |
|-------|--------|
| Left Click | World coordinate callback (converts pixel to world position) |
| Right Click | Start panning mode |
| `+` Key / Scroll Up | Zoom in (max 4.0x) |
| `-` Key / Scroll Down | Zoom out (min 0.25x) |
| HOME Key | Reset zoom and pan to default |

---

## 2. World Map Dialog (Index 22)

### Overview

The World Map dialog displays a full pre-rendered map image with the player's position marked. It supports 8+ different map variations.

### Display Function

```cpp
void CGame::DrawDialogBox_Map(short sX, short sY)
// Location: Game.cpp:24767
```

### Map Definitions

| Index | Map Name | Sprite | Offset X,Y | Size W×H |
|-------|----------|--------|------------|----------|
| 0 | Aresden | NEWMAPS1[0] | 19, 20 | 260×260 |
| 1 | Elvine | NEWMAPS1[1] | 20, 18 | 260×260 |
| 2 | Middleland | NEWMAPS2[0] | 11, 31 | 280×253 |
| 3 | Default | NEWMAPS2[1] | 52, 42 | 200×200 |
| 4 | Aresden South | NEWMAPS3[0] | 40, 40 | 220×220 |
| 5 | Elvine South | NEWMAPS3[1] | 40, 40 | 220×220 |
| 6 | Aresden North | NEWMAPS4[0] | 40, 40 | 220×220 |
| 7 | Elvine North | NEWMAPS4[1] | 40, 40 | 220×220 |
| 8 | Aresden Underground | NEWMAPS5[0] | 40, ... | 220×220 |

### State Variables

```cpp
m_stDialogBoxInfo[22].sV1  // Map group (1 = main cities, other = subzones)
m_stDialogBoxInfo[22].sV2  // Map index (0-8)
```

---

## 3. NPC Talk Dialog (Index 21)

### Overview

The NPC Talk dialog displays conversation text from NPCs and provides options for player responses. It supports scrolling for longer text and multiple dialog modes.

### Display Function

```cpp
void CGame::DrawDialogBox_NpcTalk(short sX, short sY, short sFrame)
// Location: Game.cpp:20409
```

### Dialog Modes

| Mode | Purpose | Buttons |
|------|---------|---------|
| 0 | Information display | OK |
| 1 | Quest acceptance | Accept / Decline |
| 2 | Multi-page dialog | Next |

### Message Storage

```cpp
char* m_pMsgTextList2[300];  // Up to 300 lines of dialog text
int m_iNpcTalkScrollPos;     // Current scroll position
```

### Scrolling

- Visible lines: 17 per page
- Scroll bar appears if total lines > 17
- Line height: 15 pixels

### Button Coordinates

| Button | X Offset | Y Offset | Width | Height |
|--------|----------|----------|-------|--------|
| OK | sX + 154 | sY + 292 | 74 | 20 |
| Accept | sX + 30 | sY + 292 | 74 | 20 |
| Decline | sX + 154 | sY + 292 | 74 | 20 |
| Next | sX + 154 | sY + 292 | 74 | 20 |

---

## 4. NPC Action Query Dialog (Index 20)

### Overview

When a player interacts with an NPC, this dialog appears to let them choose an action (Trade, Talk, Withdraw, etc.).

### Display Function

```cpp
void CGame::DrawDialogBox_NpcActionQuery(short sX, short sY)
// Location: Game.cpp:39693
```

### NPC Types and Actions

| NPC Type ID | NPC Role | Primary Action | Secondary Action |
|-------------|----------|----------------|------------------|
| 15 | Shop Keeper | Trade | Talk |
| 19 | Magician | Trade | Talk |
| 20 | Warehouse Keeper | Withdraw | Talk |
| 24 | Blacksmith | Trade | Talk |
| 25 | City Hall Officer | Offer | Talk |
| 26 | Guild Hall Officer | Trade | Talk |

### Dialog Modes

| Mode | Purpose |
|------|---------|
| 0 | Initial NPC interaction options |
| 1 | Item exchange confirmation |

### Button Hitboxes

```cpp
// Primary action button
if ((msX >= sX + 25) && (msX <= sX + 100) &&
    (msY >= sY + 55) && (msY <= sY + 70))

// Secondary action button
if ((msX >= sX + 125) && (msX <= sX + 180) &&
    (msY >= sY + 55) && (msY <= sY + 70))
```

---

## 5. Teleportation System (City Hall Menu Mode 10)

### Overview

The teleportation system allows players to travel between maps for a gold cost. It is accessed through the City Hall Menu dialog (Index 13) in mode 10.

### Display Function

```cpp
void CGame::DrawDialogBox_CityHallMenu(short sX, short sY)
// Location: Game.cpp:38263 (mode 10 section)
```

### Data Structure

```cpp
struct TeleportLocation {
    int iIndex;           // Server-assigned location index
    char mapname[12];     // Destination map name (e.g., "aresden", "elvine")
    int iX;               // Destination X coordinate
    int iY;               // Destination Y coordinate
    int iCost;            // Gold cost to teleport
};

TeleportLocation m_stTeleportList[20];  // Up to 20 teleport destinations
int m_iTeleportMapCount;                // Number of available locations
```

### State Values

| Value | Meaning |
|-------|---------|
| -1 | Loading (requesting list from server) |
| 0 | No available locations |
| > 0 | Number of available teleport destinations |

### Prerequisites for Teleportation

The teleport option is only shown when all conditions are met:

```cpp
if (m_bCitizen == TRUE &&      // Must be a citizen
    m_iPKCount == 0 &&          // Must not be a criminal (PK)
    m_bIsCrusadeMode == FALSE)  // Crusade mode must be inactive
```

### Network Protocol

#### Request Teleport List

```cpp
// Message ID
#define MSGID_REQUEST_TELEPORT_LIST  0x0EA03202

// Sent when player opens teleport menu
bSendCommand(MSGID_REQUEST_TELEPORT_LIST, NULL, NULL, NULL, NULL, NULL, NULL);
```

#### Response Teleport List

```cpp
// Message ID
#define MSGID_RESPONSE_TELEPORT_LIST  0x0EA03203

// Packet structure
struct TeleportListPacket {
    // Header (6 bytes)
    int m_iTeleportMapCount;  // Number of locations

    // For each location (26 bytes each):
    struct {
        int iIndex;           // Offset +0
        char mapname[10];     // Offset +4
        int iX;               // Offset +14
        int iY;               // Offset +18
        int iCost;            // Offset +22
    } locations[m_iTeleportMapCount];
};
```

#### Handler Function

```cpp
void CGame::ResponseTeleportList(char* pData)
// Location: Game.cpp:43057
```

#### Request Charged Teleport

```cpp
// Message ID
#define MSGID_REQUEST_CHARGED_TELEPORT  0x0EA03204

// Sent when player selects a destination
bSendCommand(MSGID_REQUEST_CHARGED_TELEPORT, NULL, NULL,
             m_stTeleportList[selectedIndex].iIndex, NULL, NULL, NULL);
```

#### Response Charged Teleport

```cpp
// Message ID
#define MSGID_RESPONSE_CHARGED_TELEPORT  0x0EA03205

// Rejection reason codes (offset DEF_INDEX2_MSGTYPE+2)
enum TeleportRejection {
    INSUFFICIENT_GOLD = 1,
    AREA_NOT_ACCESSIBLE = 2,
    LEVEL_RESTRICTION = 3,
    CIVILIAN_RESTRICTION = 4,
    STATUS_EFFECT_PREVENTS = 5,
    OTHER_ERROR = 6
};
```

### UI Layout

#### Loading State

```
"Now it's searching for possible area"
"to teleport."
"Please wait for a moment."
```

#### No Locations Available

```
"There is no area that you can teleport."
```

#### Location List

```
Map Name: aresden    Cost: 5000
Map Name: elvine     Cost: 5000
Map Name: middleland Cost: 10000
...
```

### Click Handling

```cpp
// DlgBoxClick_CityHallMenu - Game.cpp:22556
case 10:  // Teleport mode
    if (m_iTeleportMapCount > 0) {
        for (int i = 0; i < m_iTeleportMapCount; i++) {
            // Each list item is 15 pixels tall, starting at Y+130
            if ((msX >= sX + 30) && (msX <= sX + 228) &&
                (msY >= sY + 130 + i*15) && (msY <= sY + 144 + i*15)) {
                bSendCommand(MSGID_REQUEST_CHARGED_TELEPORT, NULL, NULL,
                           m_stTeleportList[i].iIndex, NULL, NULL, NULL);
                DisableDialogBox(13);
                return;
            }
        }
    }
    break;
```

---

## 6. Teleport Location Marker (In-World)

### Overview

When a teleport destination is selected, a marker can be displayed in the game world showing the target location.

### Variables

```cpp
int m_iTeleportLocX;  // Target X coordinate (-1 if none)
int m_iTeleportLocY;  // Target Y coordinate (-1 if none)
```

### Rendering

```cpp
// Game.cpp:15876
if (m_iTeleportLocX != -1) {
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_CRUSADE,
                     m_iTeleportLocX * 32 - m_sViewPointX,
                     m_iTeleportLocY * 32 - m_sViewPointY,
                     42);  // Sprite frame 42
}
```

### Setting from Network

```cpp
// Game.cpp:30364-30368
m_iTeleportLocX = *sp;  // Read X coordinate
sp += 2;
m_iTeleportLocY = *sp;  // Read Y coordinate
```

---

## Constants & Limits

### Map Constants

| Constant | Value | Description |
|----------|-------|-------------|
| Guide Map Size | 128x128 | Pixel dimensions of mini-map |
| Zoom Min | 0.25x | Minimum zoom level |
| Zoom Max | 4.0x | Maximum zoom level |
| Grid Spacing | 32 | Pixels between grid lines |
| Max Teleport Locations | 20 | Maximum destinations in list |

### Dialog Dimensions

| Dialog | Width | Height |
|--------|-------|--------|
| Guide Map | 128 | 128 |
| World Map | ~300 | ~300 (varies by map) |
| NPC Talk | 258 | 312 |
| NPC Action Query | 200 | 85 |

---

## Localization Strings

### Guide Map

```cpp
#define DEF_MSG_GUIDEMAP_MIN     "(-) : Minimize map"
#define DEF_MSG_GUIDEMAP_MAX     "(+) : Maximize map"
```

### Location Names

```cpp
#define DEF_MSG_MAPNAME_CITYHALL    "Cityhall"
#define DEF_MSG_MAPNAME_MAGICTOWER  "Magicshop"
#define DEF_MSG_MAPNAME_DUNGEON     "Dungeon"
#define DEF_MSG_MAPNAME_WAREHOUSE   "Warehouse"
#define DEF_MSG_MAPNAME_BARRACK     "Barrack"
#define DEF_MSG_MAPNAME_GUILDHALL   "GuildHall"
#define DEF_MSG_MAPNAME_SHOP        "Shop"
#define DEF_MSG_MAPNAME_BLACKSMITH  "BlackSmith"
#define DEF_MSG_MAPNAME_CATH        "Church"
```

### Teleport Dialog

```cpp
#define DRAW_DIALOGBOX_CITYHALL_MENU69    "Teleporting to dungeon level 2."
#define DRAW_DIALOGBOX_CITYHALL_MENU70    "5000Gold is required"
#define DRAW_DIALOGBOX_CITYHALL_MENU71    "to teleport to dungeon level 2."
#define DRAW_DIALOGBOX_CITYHALL_MENU72    "Would you like to teleport?"
#define DRAW_DIALOGBOX_CITYHALL_MENU72_1  "Civilians cannot go some area."
#define DRAW_DIALOGBOX_CITYHALL_MENU73    "Now it's searching for possible area"
#define DRAW_DIALOGBOX_CITYHALL_MENU74    "to teleport."
#define DRAW_DIALOGBOX_CITYHALL_MENU75    "Please wait for a moment."
#define DRAW_DIALOGBOX_CITYHALL_MENU76    "There is no area that you can teleport."
#define DRAW_DIALOGBOX_CITYHALL_MENU77    "Map Name:%s Cost : %d"
```

### NPC Actions

```cpp
#define DRAW_DIALOGBOX_NPCACTION_QUERY13  "Offer"
#define DRAW_DIALOGBOX_NPCACTION_QUERY17  "Withdraw"
#define DRAW_DIALOGBOX_NPCACTION_QUERY21  "Trade"
#define DRAW_DIALOGBOX_NPCACTION_QUERY25  "Talk"
```

---

## Integration Points

### With Network System

- Teleport list requests and responses via XSocket
- Location data received from server dynamically
- Teleport execution confirmed by server

### With Map System

- Map index determines which guide map sprite to use
- Player coordinates translated to mini-map position
- Named location overlays based on map-specific coordinate ranges

### With Game State

- Teleport availability depends on citizenship, PK status, crusade mode
- Dialog visibility controlled by game state machine

### With Input System

- Mouse clicks for location selection
- Keyboard shortcuts for zoom control
- Scroll wheel support for mini-map zoom

---

## Known Issues / Technical Debt

1. **Hardcoded Location Coordinates**: All named location ranges are hardcoded per map, making it difficult to add new maps or modify city layouts

2. **Magic Numbers**: Button coordinates, list item heights, and offsets are hardcoded throughout

3. **Fixed Teleport List Size**: Maximum of 20 teleport locations is hardcoded in array size

4. **No Caching**: Map sprites are not cached; each dialog open may reload sprites

5. **Pixel-Perfect Hit Detection**: Click detection uses exact pixel coordinates, no margin for error

6. **Mixed Responsibilities**: Drawing and input handling are interleaved in the same large functions

7. **Limited Zoom Persistence**: Zoom level resets when dialog is closed

---

## Modernization Notes

### Recommended Improvements

1. **Data-Driven Location Names**: Load named location coordinates from configuration files instead of hardcoding

2. **Separate Map Dialog Classes**: Create distinct classes for GuideMap, WorldMap, and TeleportDialog

3. **Zoom State Persistence**: Save and restore zoom level between dialog opens

4. **Sprite Caching**: Implement proper sprite caching for map backgrounds

5. **Flexible Teleport List**: Use `std::vector` instead of fixed-size array for teleport locations

6. **Event-Based Input**: Use event system instead of coordinate-based hit testing

7. **Localization Integration**: Load location names and UI strings from localization system

### Modern Class Structure

```cpp
namespace hb::ui {
    class GuideMapDialog : public Dialog {
        float m_zoomLevel = 1.0f;
        Vec2 m_panOffset;
        std::vector<NamedLocation> m_locations;

        void onDraw(Renderer& renderer) override;
        bool onMouseWheel(float delta) override;
        void updatePlayerMarker(Vec2 worldPos);
    };

    class TeleportDialog : public Dialog {
        std::vector<TeleportDestination> m_destinations;
        int m_selectedIndex = -1;
        LoadingState m_loadState;

        void requestTeleportList();
        void onDestinationSelected(int index);
    };
}
```
