# Dialog System Overview

## Overview

The Helbreath client implements a monolithic dialog system with **41 dialog box types** managed entirely within the `CGame` class. Each dialog has dedicated rendering and input handling functions, with position, size, scroll state, and mode data stored in a centralized array structure. The system uses DirectDraw 7 sprite rendering with multiple transparency levels and manual input dispatching via switch statements.

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` | Dialog structure definitions, function declarations, state arrays |
| `Game.cpp` | All dialog rendering (`DrawDialogBox_*`) and input (`DlgBoxClick_*`) functions |
| `SpriteID.h` | Dialog sprite resource IDs |
| `GlobalDef.h` | Dialog-related constants and limits |

### Key Locations in Game.cpp

| Component | Approximate Lines |
|-----------|-------------------|
| Dialog initialization | 543-785 |
| Drawing dispatcher (`DrawDialogBoxs`) | 18027-18142 |
| Input dispatcher (`_bCheckDlgBoxClick`) | 4351-4470 |
| Double-click handling | 4430+ |
| Enable/Disable functions | 18186+ |
| Individual dialog renderers | Throughout (17000-39000) |

---

## Key Data Structures

### DialogBoxInfo Structure

Each dialog's state is stored in `m_stDialogBoxInfo[41]`:

```cpp
struct {
    // Generic variable storage (14 int values for dialog-specific data)
    int   sV1, sV2, sV3, sV4, sV5, sV6, sV7;
    int   sV8, sV9, sV10, sV11, sV12, sV13, sV14;

    // Time/state tracking
    DWORD dwV1, dwV2, dwT1;
    BOOL  bFlag;

    // Position and size (pixels)
    short sX, sY;           // Top-left corner on screen
    short sSizeX, sSizeY;   // Width and height

    // Scroll state
    short sView;            // Current scroll offset for scrollable content

    // String storage (4 buffers for dynamic text)
    char  cStr[32];
    char  cStr2[32];
    char  cStr3[32];
    char  cStr4[32];

    // Mode indicator
    char  cMode;            // Dialog-specific mode/subtype

    // Scroll interaction tracking
    BOOL  bIsScrollSelected;
} m_stDialogBoxInfo[41];
```

### State Management Arrays

```cpp
BOOL m_bIsDialogEnabled[41];  // Visibility flags (TRUE = shown)
char m_cDialogBoxOrder[42];   // Rendering order (null-terminated, back-to-front)
```

### Mouse Cursor State

Used for dialog input tracking:

```cpp
struct {
    short sX, sY;                    // Current mouse position
    short sCursorFrame;              // Cursor animation frame
    char  cPrevStatus;               // Previous state (see below)
    char  cSelectedObjectType;       // What's selected (dialog, item, etc.)
    short sSelectedObjectID;         // Which dialog/item is selected
    short sPrevX, sPrevY;            // Previous position (for drag)
    short sDistX, sDistY;            // Drag distance
    DWORD dwSelectClickTime;         // Timestamp for double-click detection
    short sClickX, sClickY;          // Click position
} m_stMCursor;
```

**Cursor Status Values:**
| Constant | Value | Meaning |
|----------|-------|---------|
| `DEF_CURSORSTATUS_NULL` | 0 | No selection |
| `DEF_CURSORSTATUS_PRESSED` | 1 | Mouse button pressed |
| `DEF_CURSORSTATUS_SELECTED` | 2 | Item/dialog selected |
| `DEF_CURSORSTATUS_DRAGGING` | 3 | Drag in progress |

---

## Complete Dialog Type Reference

| Index | Name | Purpose | Keyboard | Default Position |
|-------|------|---------|----------|------------------|
| 1 | Character Info | Player stats, experience, attributes | F5 | (30, 30) |
| 2 | Inventory | 50-slot item grid management | F6 | (380, 210) |
| 3 | Magic Circle | Spell selection and casting | F7 | (337, 57) |
| 4 | Item Drop | Confirm item drop with amount | - | Dynamic |
| 5 | 15+ Age Warning | Age verification (conditional) | - | Center |
| 6 | Warning Message | Generic warning/notification | - | Center |
| 7 | Guild Menu | Guild membership options | - | Dynamic |
| 8 | Guild Operation | Manage guild members/ranks | - | Dynamic |
| 9 | Guide Map | Mini-map navigation | - | (512, 0) |
| 10 | Chat History | Scrollable chat message log | F9 | (135, 273) |
| 11 | Shop | NPC vendor buy interface | - | (70, 50) |
| 12 | Level-Up Setting | Allocate stat points on level | - | Center |
| 13 | City Hall Menu | Town administration options | - | Dynamic |
| 14 | Bank | 121-slot bank deposit/withdraw | - | Dynamic |
| 15 | Skill Menu | Skill list and training | F8 | Dynamic |
| 16 | Magic Shop | Purchase spells from NPC | - | Dynamic |
| 17 | Query Drop Amount | Enter quantity to drop | - | Dynamic |
| 18 | Text Dialog | Generic scrollable text display | - | Dynamic |
| 19 | System Menu | Game options/settings | F12 | Center |
| 20 | NPC Action Query | NPC interaction menu | - | Dynamic |
| 21 | NPC Talk | NPC conversation/quest text | - | Dynamic |
| 22 | Map | Full world map view | - | Center |
| 23 | Sell/Repair Item | Vendor sell/repair interface | - | Dynamic |
| 24 | Fishing | Fishing minigame UI | - | Dynamic |
| 25 | Shutdown Message | Server shutdown notice | - | Center |
| 26 | Skill Dialog | Skill usage and description | - | Dynamic |
| 27 | Exchange/Trade | Player-to-player trading | - | (100, 30) |
| 28 | Quest | Quest information and log | - | Dynamic |
| 29 | Gauge Panel | HP/MP/SP status bars (HUD) | - | (0, 434) |
| 30 | Icon Panel | Quick-access toolbar | - | (0, 427) |
| 31 | Sell List | NPC sell price listing | - | Dynamic |
| 32 | Party | Party member list/management | - | Dynamic |
| 33 | Crusade Job | War duty assignment selection | - | Dynamic |
| 34 | Item Upgrade | Item enhancement interface | - | Dynamic |
| 35 | Help/Manual | In-game help system | F1 | Center |
| 36 | Crusade Commander | War commander controls | - | Dynamic |
| 37 | Crusade Constructor | Building construction UI | - | Dynamic |
| 38 | Crusade Soldier | Soldier interface | - | Dynamic |
| 39 | (Reserved) | Unused slot | - | - |
| 40 | Feedback Card | Bug report (conditional compile) | - | Center |

**Note:** Dialog 40 (Feedback Card) only exists when `DEF_FEEDBACKCARD` is defined (Chinese client).

---

## Core Functions

### Dialog Lifecycle

#### EnableDialogBox - Open a Dialog

```cpp
void CGame::EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char *pString)
```

**Parameters:**
- `iBoxID` - Dialog index (1-40)
- `cType` - Dialog type/mode (stored in sV1 typically)
- `sV1`, `sV2` - Dialog-specific parameters
- `pString` - Optional string data

**Behavior:**
1. Sets `m_bIsDialogEnabled[iBoxID] = TRUE`
2. Stores parameters in `m_stDialogBoxInfo[iBoxID]`
3. Appends dialog ID to `m_cDialogBoxOrder[]` array
4. Dialog renders on next frame

#### DisableDialogBox - Close a Dialog

```cpp
void CGame::DisableDialogBox(int iBoxID)
```

**Behavior:**
1. Sets `m_bIsDialogEnabled[iBoxID] = FALSE`
2. Removes dialog ID from `m_cDialogBoxOrder[]`
3. Clears associated input state
4. Dialog disappears immediately

### Rendering Pipeline

#### Main Dispatcher

```cpp
void CGame::DrawDialogBoxs(short msX, short msY, short msZ, char cLB)
```

**Parameters:**
- `msX`, `msY` - Current mouse position
- `msZ` - Mouse wheel delta
- `cLB` - Left mouse button state

**Process:**
1. Skip if in observer mode
2. Iterate through `m_cDialogBoxOrder[]` (back to front)
3. For each enabled dialog, call its `DrawDialogBox_*()` function
4. Pass mouse state for hover effects

#### Dialog-Specific Renderers

Each dialog has a dedicated rendering function:

```cpp
void DrawDialogBox_Character(short msX, short msY);
void DrawDialogBox_Inventory(int msX, int msY);
void DrawDialogBox_Magic(short msX, short msY, short msZ);
void DrawDialogBox_Chat(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_Shop(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_Bank(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_Skill(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_Exchange(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_Quest(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_GaugePanel(short msX, short msY, char cLB);
void DrawDialogBox_IconPanel(short msX, short msY, char cLB);
void DrawDialogBox_SellList(short msX, short msY, short msZ, char cLB);
void DrawDialogBox_Party(short msX, short msY, char cLB);
void DrawDialogBox_CrusadeJob(short msX, short msY, char cLB);
void DrawDialogBox_ItemUpgrade(short msX, short msY, char cLB);
void DrawDialogBox_Help(short msX, short msY, short msZ, char cLB);
// ... additional dialogs
```

#### Background Rendering

```cpp
void DrawNewDialogBox(char cType, int sX, int sY, int iFrame,
                      BOOL bIsNoColorKey = FALSE, BOOL bIsTrans = FALSE)
```

Renders dialog background sprite from GameDialog PAK resources.

### Input Handling

#### Main Input Dispatcher

```cpp
BOOL CGame::_bCheckDlgBoxClick(short msX, short msY)
```

**Returns:** `TRUE` if a dialog consumed the input (blocks game world interaction)

**Process:**
1. Iterate `m_cDialogBoxOrder[]` in reverse (top dialog first)
2. Check if mouse position is within dialog bounds
3. Dispatch to appropriate `DlgBoxClick_*()` handler
4. Return `TRUE` if handled

#### Dialog-Specific Input Handlers

```cpp
void DlgBoxClick_Character(short msX, short msY);
void DlgBoxClick_Inventory(short msX, short msY);
void DlgBoxClick_Magic(short msX, short msY);
void DlgBoxClick_Shop(short msX, short msY);
void DlgBoxClick_Bank(short msX, short msY);
void DlgBoxClick_Skill(short msX, short msY);
void DlgBoxClick_Exchange(short msX, short msY);
void DlgBoxClick_Quest(short msX, short msY);
void DlgBoxClick_IconPanel(short msX, short msY);
void DlgBoxClick_Party(short msX, short msY);
// ... additional handlers
```

#### Double-Click Handling

```cpp
BOOL CGame::_bCheckDlgBoxDoubleClick(short msX, short msY)

void DlbBoxDoubleClick_Character(short msX, short msY);
void DlbBoxDoubleClick_Inventory(short msX, short msY);
void DlbBoxDoubleClick_Bank(short msX, short msY);
void DlbBoxDoubleClick_Exchange(short msX, short msY);
```

Detection uses timestamp comparison:
```cpp
if ((dwCurrentTime - m_stMCursor.dwSelectClickTime) < DEF_DOUBLECLICKTIME) {
    // Double-click detected
}
```

---

## Constants & Limits

### Dialog System Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| Max Dialogs | 41 | Hard-coded array size |
| Dialog Order Size | 42 | Rendering stack (null-terminated) |
| String Buffers | 4 | cStr, cStr2, cStr3, cStr4 per dialog |
| String Length | 32 | Max characters per string buffer |
| Variable Storage | 14 | sV1-sV14 int values per dialog |

### Standard Dimensions

| Dialog | Width | Height | Notes |
|--------|-------|--------|-------|
| Character Info | 270 | 376 | Stats panel |
| Inventory | 225 | 185 | 5x10 item grid |
| Magic Circle | 258 | 328 | Spell selection |
| Shop | 258 | 339 | Vendor interface |
| Chat History | 364 | 162 | Message log |
| Icon Panel | 640 | 53 | Bottom toolbar (full width) |
| Gauge Panel | 157 | 53 | HP/MP bars |
| Guide Map | 128 | 128 | Mini-map |
| Exchange | 520 | 357 | Trade window |
| Bank | ~370 | ~380 | 11x11 grid |

### Grid Standards

| Grid Type | Columns | Rows | Slot Size | Total Slots |
|-----------|---------|------|-----------|-------------|
| Inventory | 5 | 10 | 32x32 | 50 |
| Bank | 11 | 11 | 32x32 | 121 |
| Sell List | 1-2 | Variable | Text rows | N/A |
| Shop | 1-2 | Variable | 32x32 | ~20 |

### Sprite Resource IDs

| ID | Constant | Purpose |
|----|----------|---------|
| 60 | `DEF_SPRID_INTERFACE_ND_GAME1` | Standard dialog frame |
| 61 | `DEF_SPRID_INTERFACE_ND_GAME2` | Alternate frame 1 |
| 62 | `DEF_SPRID_INTERFACE_ND_GAME3` | Alternate frame 2 |
| 63 | `DEF_SPRID_INTERFACE_ND_GAME4` | Alternate frame 3 |
| 64 | `DEF_SPRID_INTERFACE_ND_ICONPANNEL` | Icon panel graphics |
| 67 | `DEF_SPRID_INTERFACE_ND_INVENTORY` | Inventory background |
| 70 | `DEF_SPRID_INTERFACE_ND_TEXT` | Text dialog frame |
| 71 | `DEF_SPRID_INTERFACE_ND_BUTTON` | Button graphics |
| 72 | `DEF_SPRID_INTERFACE_ND_CRUSADE` | Crusade-themed frame |
| 73 | `DEF_SPRID_INTERFACE_GUIDEMAP` | Map background |
| 22 | `DEF_SPRID_INTERFACE_SPRFONTS` | Standard font |
| 28 | `DEF_SPRID_INTERFACE_SPRFONTS2` | Alternate font |
| 29 | `DEF_SPRID_INTERFACE_F1HELPWINDOWS` | Help window frame |

---

## Common Rendering Patterns

### Pattern 1: Basic Dialog with Background

```cpp
void CGame::DrawDialogBox_Example(short msX, short msY) {
    short sX = m_stDialogBoxInfo[DIALOG_ID].sX;
    short sY = m_stDialogBoxInfo[DIALOG_ID].sY;

    // Draw background sprite
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME1, sX, sY, 0);

    // Draw title text
    PutString(sX + 20, sY + 10, "Dialog Title", RGB(200, 200, 200));

    // Draw content
    PutString(sX + 20, sY + 40, "Content text here", RGB(255, 255, 255));
}
```

### Pattern 2: Button with Hover State

```cpp
// Check if mouse is over button area
int btnX = sX + 100;
int btnY = sY + 200;
int btnW = 80;
int btnH = 20;

BOOL isHovered = (msX >= btnX && msX < btnX + btnW &&
                  msY >= btnY && msY < btnY + btnH);

// Draw button with appropriate frame (normal vs hover)
int spriteFrame = isHovered ? 2 : 1;
m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->PutSpriteFast(
    btnX, btnY, spriteFrame, m_dwCurTime);

// Draw button text
PutString(btnX + 10, btnY + 3, "OK", RGB(255, 255, 255));
```

### Pattern 3: Scrollable List

```cpp
short sView = m_stDialogBoxInfo[DIALOG_ID].sView;
int maxVisible = 10;  // Items visible at once
int totalItems = GetTotalItemCount();

// Render visible items only
for (int i = sView; i < sView + maxVisible && i < totalItems; i++) {
    int screenY = sY + 30 + (i - sView) * 20;
    PutString(sX + 10, screenY, itemNames[i], RGB(255, 255, 255));
}

// Draw scroll bar
if (totalItems > maxVisible) {
    int scrollHeight = 150;
    int thumbPos = (sView * scrollHeight) / (totalItems - maxVisible);
    // Draw scroll track and thumb...
}
```

### Pattern 4: Item Grid (Inventory Style)

```cpp
// 5-column grid with 32x32 slots
int gridStartX = sX + 32;
int gridStartY = sY + 44;
int slotSize = 32;
int columns = 5;

for (int i = 0; i < DEF_MAXITEMS; i++) {
    if (m_pItemList[i] == NULL) continue;

    int col = i % columns;
    int row = i / columns;
    int screenX = gridStartX + col * slotSize;
    int screenY = gridStartY + row * slotSize;

    // Draw item sprite
    int spriteID = m_pItemList[i]->m_sSpriteID;
    int frame = m_pItemList[i]->m_sFrame;
    m_pSprite[spriteID]->PutSpriteFast(screenX, screenY, frame, m_dwCurTime);

    // Draw quantity if stackable
    if (m_pItemList[i]->m_dwCount > 1) {
        char szCount[8];
        sprintf(szCount, "%d", m_pItemList[i]->m_dwCount);
        PutString_SprNum(screenX, screenY + 20, szCount, 0, 0);
    }
}
```

### Pattern 5: Highlight on Hover

```cpp
// Check if mouse is over this item slot
int slotX = gridStartX + col * slotSize;
int slotY = gridStartY + row * slotSize;

if (msX >= slotX && msX < slotX + slotSize &&
    msY >= slotY && msY < slotY + slotSize) {
    // Draw highlight overlay (50% transparency)
    m_DDraw.DrawShadowBox(slotX, slotY, slotX + slotSize, slotY + slotSize);

    // Show item tooltip
    ShowItemInfo(m_pItemList[i]);
}
```

---

## Common Input Patterns

### Pattern 1: Button Click Detection

```cpp
void CGame::DlgBoxClick_Example(short msX, short msY) {
    short sX = m_stDialogBoxInfo[DIALOG_ID].sX;
    short sY = m_stDialogBoxInfo[DIALOG_ID].sY;

    // Close button (top-right corner)
    if (msX >= sX + 240 && msX <= sX + 260 &&
        msY >= sY + 5 && msY <= sY + 20) {
        DisableDialogBox(DIALOG_ID);
        return;
    }

    // OK button
    if (msX >= sX + 100 && msX <= sX + 180 &&
        msY >= sY + 200 && msY <= sY + 220) {
        // Process OK action
        PerformAction();
        DisableDialogBox(DIALOG_ID);
        return;
    }
}
```

### Pattern 2: Grid Slot Selection

```cpp
// Calculate which slot was clicked
int relX = msX - gridStartX;
int relY = msY - gridStartY;

if (relX >= 0 && relY >= 0) {
    int col = relX / slotSize;
    int row = relY / slotSize;

    if (col < columns && row < rows) {
        int slotIndex = row * columns + col;

        if (slotIndex < DEF_MAXITEMS && m_pItemList[slotIndex] != NULL) {
            // Item clicked - store selection
            m_stMCursor.sSelectedObjectID = slotIndex;
            m_stMCursor.cSelectedObjectType = DEF_SELECTEDOBJTYPE_ITEM;
        }
    }
}
```

### Pattern 3: Scroll Bar Interaction

```cpp
// Scroll bar area
int scrollX = sX + 220;
int scrollY = sY + 30;
int scrollHeight = 150;

if (msX >= scrollX && msX <= scrollX + 15 &&
    msY >= scrollY && msY <= scrollY + scrollHeight) {

    // Calculate new scroll position
    int relY = msY - scrollY;
    int totalItems = GetTotalItemCount();
    int maxVisible = 10;

    if (totalItems > maxVisible) {
        int newView = (relY * (totalItems - maxVisible)) / scrollHeight;
        m_stDialogBoxInfo[DIALOG_ID].sView = newView;
    }

    m_stDialogBoxInfo[DIALOG_ID].bIsScrollSelected = TRUE;
}
```

### Pattern 4: Drag and Drop

```cpp
// On mouse down - start potential drag
if (m_stMCursor.cPrevStatus == DEF_CURSORSTATUS_NULL) {
    m_stMCursor.cPrevStatus = DEF_CURSORSTATUS_PRESSED;
    m_stMCursor.sPrevX = msX;
    m_stMCursor.sPrevY = msY;
    m_stMCursor.sSelectedObjectID = slotIndex;
}

// On mouse move with button held - detect drag
if (m_stMCursor.cPrevStatus == DEF_CURSORSTATUS_PRESSED) {
    int dx = abs(msX - m_stMCursor.sPrevX);
    int dy = abs(msY - m_stMCursor.sPrevY);

    if (dx > 3 || dy > 3) {
        m_stMCursor.cPrevStatus = DEF_CURSORSTATUS_DRAGGING;
    }
}

// On mouse up - complete drag
if (m_stMCursor.cPrevStatus == DEF_CURSORSTATUS_DRAGGING) {
    // Determine drop target and validate
    int dropSlot = CalculateDropSlot(msX, msY);
    if (dropSlot >= 0) {
        bItemDrop_Inventory(m_stMCursor.sSelectedObjectID, dropSlot);
    }
    m_stMCursor.cPrevStatus = DEF_CURSORSTATUS_NULL;
}
```

---

## Integration Points

### With Game State

- Dialogs read from global game state variables (`m_iLevel`, `m_iStr`, `m_pItemList[]`, etc.)
- Dialog actions modify game state directly or send network packets
- State changes reflected on next render frame

### With Network System

- Many dialogs trigger network messages on user actions:
  - Inventory: `CYCMOVE`, `CYCID_EXCHANGEITEM`
  - Shop: `bSendCommand(CYCMOVE_PURCHASEITEM, ...)`
  - Bank: `bSendCommand(CYCMOVE_BANKDEPOSIT/WITHDRAW, ...)`
  - Guild: Guild operation packets

### With Input System

- `_bCheckDlgBoxClick()` called from main input handler
- Returns TRUE to prevent input from reaching game world
- Keyboard shortcuts bypass click system (F5-F12)

### With Sprite System

- All dialog rendering uses `m_pSprite[]` array
- Sprites loaded from PAK files during initialization
- Common drawing functions: `PutSpriteFast()`, `PutTransSprite()`, etc.

---

## State Management

### Dialog Enable/Disable Flow

```
User presses F6 (Inventory)
    |
    v
EnableDialogBox(2, 0, 0, 0, NULL)
    |
    v
m_bIsDialogEnabled[2] = TRUE
m_cDialogBoxOrder[] += 2
    |
    v
DrawDialogBoxs() sees dialog 2 in order
    |
    v
DrawDialogBox_Inventory() renders
    |
    v
User clicks close button
    |
    v
DlgBoxClick_Inventory() detects close
    |
    v
DisableDialogBox(2)
    |
    v
m_bIsDialogEnabled[2] = FALSE
m_cDialogBoxOrder[] -= 2
```

### Z-Order Management

- `m_cDialogBoxOrder[42]` is a null-terminated array
- Dialogs render in array order (first = back, last = front)
- Input checks reverse order (front dialogs get input first)
- No explicit z-index values - order is implicit

### Scroll State Persistence

- `m_stDialogBoxInfo[i].sView` persists while dialog is open
- Reset to 0 when dialog is re-enabled (usually)
- Some dialogs preserve scroll on close/reopen

---

## Known Issues / Technical Debt

### Architectural Issues

1. **Monolithic Design** - All 41 dialogs in single 48,500-line file
2. **No Base Class** - Each dialog reimplements common patterns
3. **Tight Coupling** - Dialogs directly access game state
4. **Magic Numbers** - Hard-coded positions, sizes, and offsets throughout
5. **Fixed Array Size** - Cannot add dialog types beyond 41

### State Management Issues

1. **Generic Variables** - sV1-sV14 reused for different purposes per dialog
2. **Limited String Storage** - Only 4x32-char buffers per dialog
3. **No State Validation** - Invalid states can cause rendering issues
4. **Global Mutation** - Dialogs modify global state directly

### Rendering Issues

1. **Full Redraw** - No dirty rectangle optimization
2. **No Batching** - Sprites rendered individually
3. **Z-Order Coupling** - Order tied to enable sequence
4. **Fixed Resolution** - 800x600 positions hard-coded

### Input Issues

1. **No Event Queue** - Synchronous input processing
2. **Click-Only Focus** - No tab navigation or keyboard focus
3. **Manual Hit Testing** - Coordinate math repeated per dialog
4. **Double-Click Timing** - Uses global timestamp, can misfire

---

## Modernization Notes

### Recommended Architecture

1. **Component-Based UI**
   - Base `Dialog` class with virtual methods
   - Separate classes per dialog type
   - Widget components (Button, ScrollBar, Grid, etc.)

2. **Data-Driven Definitions**
   - YAML/JSON dialog layout files
   - Runtime loading and hot-reload
   - Separation of layout from logic

3. **Event System**
   - Event queue for input
   - Publish/subscribe for dialog events
   - Decoupled input handling

4. **Flexible Layout**
   - Anchor-based positioning
   - Resolution-independent coordinates
   - Automatic layout calculation

5. **State Management**
   - Typed state objects per dialog
   - Observer pattern for data binding
   - Immutable state updates

### Migration Strategy

1. Create base `UIElement` and `Dialog` classes
2. Extract common widgets (Button, ScrollBar, ItemGrid)
3. Convert one dialog at a time to new system
4. Maintain legacy rendering during transition
5. Replace direct state access with data binding
6. Add YAML definition support incrementally

### Preserving Behavior

- Exact pixel positions for authentic look
- Same transparency levels (100%, 70%, 50%, 25%)
- Identical grid layouts (5x10 inventory, 11x11 bank)
- Same keyboard shortcuts (F5-F12)
- Compatible mouse interaction (click, double-click, drag)
