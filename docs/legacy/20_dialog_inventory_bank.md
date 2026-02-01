# Inventory and Bank Dialogs

## Overview

The Inventory and Bank dialogs are two of the most frequently used interfaces in Helbreath. The inventory manages the player's 50-slot item storage and provides access to equipment, while the bank dialog interfaces with NPC bankers to store up to 121 items. Both systems rely heavily on drag-drop mechanics and share common item rendering patterns.

These dialogs are implemented directly within the monolithic `CGame` class in `Game.cpp`, with supporting item data structures defined in `Item.h/Item.cpp`.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | Dialog rendering (`DrawDialogBox_Inventory`, `DrawDialogBox_Bank`), click handlers, drag-drop logic |
| `Game.h` | Dialog state variables, item arrays, constants |
| `Item.h` | `CItem` class definition |
| `Item.cpp` | `CItem` implementation |

## Dialog Identifiers

```cpp
// Dialog indices in the m_stDialogBoxInfo array
#define DIALOG_INVENTORY    2    // Player inventory
#define DIALOG_BANK        14    // Bank storage interface
#define DIALOG_QUANTITY    17    // Quantity input for stackables
```

## Key Data Structures

### Item Class (CItem)

```cpp
class CItem {
public:
    char  m_cName[21];           // Item name (max 20 chars + null)
    char  m_cItemType;           // Item type (0-12)
    char  m_cEquipPos;           // Equipment slot (0-14)
    char  m_cItemColor;          // Color variation (0-7)

    short m_sSprite;             // Sprite resource ID
    short m_sSpriteFrame;        // Frame within sprite sheet
    short m_sX, m_sY;            // Position in inventory grid (pixels)
    short m_sLevelLimit;         // Minimum level to equip

    WORD  m_wPrice;              // Base price
    WORD  m_wWeight;             // Item weight
    WORD  m_wCurLifeSpan;        // Current durability
    WORD  m_wMaxLifeSpan;        // Maximum durability

    DWORD m_dwCount;             // Stack count (1 for non-stackable)
    DWORD m_dwAttribute;         // Special effects bitmask

    // Effect slots (6 maximum)
    char  m_cEffect[6];          // Effect type IDs
    short m_sEffectValue[6];     // Effect magnitudes
};
```

### Item Type Enumeration

```cpp
#define ITEMTYPE_NONE                    0
#define ITEMTYPE_EQUIP                   1   // Weapons, armor
#define ITEMTYPE_APPLY                   2   // Apply-type items
#define ITEMTYPE_USE_DEPLETE             3   // Consumables (count decreases)
#define ITEMTYPE_INSTALL                 4   // Installation items
#define ITEMTYPE_CONSUME                 5   // Stackable consumables
#define ITEMTYPE_ARROW                   6   // Projectiles (arrows, bolts)
#define ITEMTYPE_EAT                     7   // Food items
#define ITEMTYPE_USE_SKILL               8   // Skill activation items
#define ITEMTYPE_USE_PERM                9   // Permanent use items
#define ITEMTYPE_USE_SKILL_ENABLEDIALOGBOX 10 // Opens skill dialog
#define ITEMTYPE_USE_DEPLETE_DEST       11   // Consumables with target
#define ITEMTYPE_MATERIAL               12   // Crafting materials
```

### Equipment Slot Enumeration

```cpp
#define EQUIPPOS_NONE        0
#define EQUIPPOS_HEAD        1    // Helmet, hat
#define EQUIPPOS_BODY        2    // Chest armor
#define EQUIPPOS_ARMS        3    // Gloves, bracers
#define EQUIPPOS_PANTS       4    // Leg armor
#define EQUIPPOS_BOOTS       5    // Footwear
#define EQUIPPOS_NECK        6    // Necklace, amulet
#define EQUIPPOS_LHAND       7    // Shield, off-hand
#define EQUIPPOS_RHAND       8    // Weapon (main hand)
#define EQUIPPOS_TWOHAND     9    // Two-handed weapon
#define EQUIPPOS_RFINGER    10    // Right ring
#define EQUIPPOS_LFINGER    11    // Left ring
#define EQUIPPOS_BACK       12    // Cape, backpack
#define EQUIPPOS_FULLBODY   13    // Full body armor (special)
```

### Inventory State Variables (Game.h)

```cpp
// Item storage arrays
CItem * m_pItemList[DEF_MAXITEMS];           // Inventory items (50 slots)
CItem * m_pBankList[DEF_MAXBANKITEMS];       // Bank items (121 slots)

// Item state tracking
bool m_bIsItemEquipped[DEF_MAXITEMS];        // TRUE if item is equipped
bool m_bIsItemDisabled[DEF_MAXITEMS];        // TRUE if item locked (trade, etc.)
char m_cItemOrder[DEF_MAXITEMS];             // Display order (-1 = empty)

// Drag-drop state
bool m_bItemDrop;                            // TRUE when dragging item
short m_sItemDragStartX, m_sItemDragStartY;  // Drag start position

// Selected item tracking
struct {
    short sSelectedObjectID;                 // Currently selected item index
    // ... other cursor state
} m_stMCursor;
```

### Constants

```cpp
#define DEF_MAXITEMS        50    // Maximum inventory slots
#define DEF_MAXBANKITEMS   121    // Maximum bank slots (120 usable + 1)

// Inventory sprite resource
#define DEF_SPRID_INTERFACE_ND_INVENTORY  // Inventory background sprite

// Grid dimensions (derived from sprite layout)
// Inventory: 10 columns x 5 rows
// Bank: 11 columns x 11 rows
```

---

## Inventory Dialog

### Dialog Box Info Structure

```cpp
m_stDialogBoxInfo[DIALOG_INVENTORY] = {
    sX, sY,          // Top-left position
    sV1,             // Scroll state / mode
    // ... additional state
};
```

### Rendering (DrawDialogBox_Inventory)

The inventory dialog renders in several layers:

**1. Background Frame**
```cpp
// Draw dialog background sprite
m_pSprite[DEF_SPRID_INTERFACE_ND_INVENTORY]->PutSpriteFast(
    sX, sY, 0, dwTime);
```

**2. Item Grid**
```cpp
for (int i = 0; i < DEF_MAXITEMS; i++) {
    if (m_pItemList[i] != NULL) {
        // Get item position within grid
        int itemX = sX + m_pItemList[i]->m_sX;
        int itemY = sY + m_pItemList[i]->m_sY;

        // Draw item sprite
        if (m_cItemOrder[i] != -1) {
            // Apply color variation if present
            if (m_pItemList[i]->m_cItemColor != 0) {
                // RGB color shift based on m_cItemColor
                DrawColoredItem(itemX, itemY, item);
            } else {
                // Normal item rendering
                m_pSprite[item->m_sSprite]->PutSpriteFast(
                    itemX, itemY, item->m_sSpriteFrame, dwTime);
            }
        }

        // Draw quantity for stackables
        if (item->m_dwCount > 1) {
            DrawQuantityText(itemX, itemY, item->m_dwCount);
        }
    }
}
```

**3. Item Position Grid Layout**

Items store their position as pixel offsets within the inventory:
```cpp
// Item positions (m_sX, m_sY) are pixel offsets
// X range: 0-170 (fits ~5 items horizontally at 34px each)
// Y range: -10 to 95 (fits ~3 items vertically)

// Slot calculation from position:
int col = m_pItemList[i]->m_sX / 34;  // ~34 pixels per slot
int row = m_pItemList[i]->m_sY / 34;
```

**4. Equipped Item Indicators**
```cpp
if (m_bIsItemEquipped[i]) {
    // Draw "E" badge or highlight
    // Equipped items may be hidden from main grid
}
```

**5. UI Buttons**
```cpp
// Equipment button area
DrawButton(sX + 23, sY + 172, "Equipment");

// Amulet/Skills button area
DrawButton(sX + 140, sY + 172, "Skills");
```

### Click Handling (DlgBoxClick_Inventory)

```cpp
void CGame::DlgBoxClick_Inventory(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[DIALOG_INVENTORY].sX;
    short sY = m_stDialogBoxInfo[DIALOG_INVENTORY].sY;

    // Check Equipment button (opens character/equipment dialog)
    if ((msX >= sX + 23) && (msX <= sX + 76) &&
        (msY >= sY + 172) && (msY <= sY + 184)) {
        EnableDialogBox(DIALOG_CHARACTER, NULL, NULL, NULL);
        return;
    }

    // Check Skills/Amulet button
    if ((msX >= sX + 140) && (msX <= sX + 212) &&
        (msY >= sY + 172) && (msY <= sY + 184)) {
        EnableDialogBox(DIALOG_SKILL, NULL, NULL, NULL);
        return;
    }
}
```

### Double-Click Handling

```cpp
void CGame::DlgBoxDoubleClick_Inventory(short msX, short msY)
{
    // Find item under cursor
    int itemIndex = _iGetItemIndex(msX, msY);
    if (itemIndex == -1) return;

    CItem * pItem = m_pItemList[itemIndex];

    // Action depends on item type
    switch (pItem->m_cItemType) {
        case ITEMTYPE_EQUIP:
            // Try to equip item
            RequestEquipItem(itemIndex);
            break;

        case ITEMTYPE_USE_DEPLETE:
        case ITEMTYPE_CONSUME:
        case ITEMTYPE_EAT:
            // Use consumable
            RequestUseItem(itemIndex);
            break;

        case ITEMTYPE_USE_SKILL_ENABLEDIALOGBOX:
            // Open skill dialog with this item
            EnableDialogBox(DIALOG_SKILL, pItem, NULL, NULL);
            break;
    }
}
```

---

## Bank Dialog

### Dialog Box Info Structure

```cpp
m_stDialogBoxInfo[DIALOG_BANK] = {
    sX, sY,          // Dialog position (centered)
    sV1,             // Number of visible rows (8-10)
    sView,           // Current scroll offset
    // ... additional state
};
```

### Bank Layout

The bank displays items in a scrollable list format:

```
+----------------------------------+
|           BANK STORAGE           |
+----------------------------------+
| Stored Gold: 12,345              |
+----------------------------------+
| [Item 1 - Long Sword +3]    [>]  |  <- Scrollable
| [Item 2 - Health Potion x50]     |     item list
| [Item 3 - Plate Mail]            |
| [Item 4 - ...]                   |
| ...                              |
+----------------------------------+
|  [Scrollbar]                     |
+----------------------------------+
| Right-click to withdraw          |
+----------------------------------+
```

### Rendering (DrawDialogBox_Bank)

**1. Background and Header**
```cpp
void CGame::DrawDialogBox_Bank()
{
    short sX = m_stDialogBoxInfo[DIALOG_BANK].sX;
    short sY = m_stDialogBoxInfo[DIALOG_BANK].sY;

    // Draw dialog background
    DrawNewDialogBox(DIALOG_BANK, sX, sY);

    // Draw stored gold
    char szGold[32];
    sprintf(szGold, "Stored Gold: %d", m_dwStoredGold);
    PutString(sX + 20, sY + 20, szGold, RGB(255, 215, 0));
}
```

**2. Item List Rendering**
```cpp
// Calculate visible range
int startIndex = m_stDialogBoxInfo[DIALOG_BANK].sView;
int visibleRows = m_stDialogBoxInfo[DIALOG_BANK].sV1;  // Usually 8-10

for (int i = 0; i < visibleRows; i++) {
    int bankIndex = startIndex + i;
    if (bankIndex >= DEF_MAXBANKITEMS) break;

    if (m_pBankList[bankIndex] != NULL) {
        CItem * pItem = m_pBankList[bankIndex];

        // Calculate row Y position
        int rowY = sY + 40 + (i * 28);  // 28 pixels per row

        // Draw item name with attributes
        char szItemText[128];
        FormatItemName(pItem, szItemText);

        // Highlight if hovered
        if (IsMouseOverRow(rowY)) {
            DrawRect(sX + 10, rowY, 200, 26, RGB(60, 60, 100));
        }

        PutString(sX + 15, rowY + 4, szItemText, GetItemColor(pItem));
    }
}
```

**3. Scrollbar**
```cpp
// Scrollbar dimensions
#define SCROLLBAR_X       (sX + 230)
#define SCROLLBAR_Y       (sY + 40)
#define SCROLLBAR_HEIGHT  274
#define SCROLLBAR_WIDTH   20

// Calculate scrollbar position
int totalItems = _iGetBankItemCount();
int maxScroll = max(0, totalItems - visibleRows);
int scrollPos = 0;
if (maxScroll > 0) {
    scrollPos = (sView * SCROLLBAR_HEIGHT) / maxScroll;
}

// Draw scrollbar track
DrawRect(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT,
         RGB(40, 40, 40));

// Draw scrollbar handle
DrawRect(SCROLLBAR_X, SCROLLBAR_Y + scrollPos, SCROLLBAR_WIDTH, 20,
         RGB(100, 100, 100));
```

### Bank Click Handling (DlgBoxClick_Bank)

```cpp
void CGame::DlgBoxClick_Bank(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[DIALOG_BANK].sX;
    short sY = m_stDialogBoxInfo[DIALOG_BANK].sY;

    // Check scrollbar area
    if ((msX >= sX + 230) && (msX <= sX + 260)) {
        // Handle scrollbar drag
        int newView = ((msY - sY - 40) * maxItems) / 274;
        m_stDialogBoxInfo[DIALOG_BANK].sView =
            max(0, min(newView, maxScroll));
        return;
    }

    // Check item list area (right-click to withdraw)
    if ((msX >= sX + 10) && (msX <= sX + 220) &&
        (msY >= sY + 40) && (msY <= sY + 314)) {

        int rowIndex = (msY - sY - 40) / 28;
        int bankIndex = m_stDialogBoxInfo[DIALOG_BANK].sView + rowIndex;

        if (m_pBankList[bankIndex] != NULL) {
            // Check if inventory has space
            if (_iGetTotalItemNum() >= DEF_MAXITEMS) {
                AddSystemMessage("Inventory full!");
                return;
            }

            // Send withdraw request to server
            SendRequestRetrieveItem(bankIndex);
        }
    }
}
```

### Deposit Flow (bItemDrop_Bank)

When dragging an item from inventory to the bank dialog:

```cpp
bool CGame::bItemDrop_Bank(short msX, short msY)
{
    // Validate bank is open and not full
    if (_iGetBankItemCount() >= 120) {
        AddSystemMessage("Bank is full!");
        return false;
    }

    // Validate no conflicting operations
    if (m_cCommand < 0) return false;

    // Get dragged item
    int itemIndex = m_stMCursor.sSelectedObjectID;
    if (itemIndex < 0 || itemIndex >= DEF_MAXITEMS) return false;

    CItem * pItem = m_pItemList[itemIndex];
    if (pItem == NULL) return false;

    // Check if item is disabled (in trade, etc.)
    if (m_bIsItemDisabled[itemIndex]) return false;

    // Handle stackable items
    if (pItem->m_dwCount > 1) {
        // Show quantity dialog
        EnableDialogBox(DIALOG_QUANTITY, NULL, NULL, NULL);
        m_stDialogBoxInfo[DIALOG_QUANTITY].sV3 = 1002;  // Bank NPC ID
        m_stDialogBoxInfo[DIALOG_QUANTITY].sV4 = itemIndex;
        return true;
    }

    // Single item - deposit directly
    SendGiveItemToBank(itemIndex, 1);
    return true;
}
```

### Withdraw Flow

```cpp
void CGame::SendRequestRetrieveItem(int bankSlot)
{
    // Build network message
    char cMsg[32];
    DWORD * dwp = (DWORD *)cMsg;
    WORD * wp;

    *dwp = MSGID_REQUEST_RETRIEVEITEM;
    wp = (WORD *)(cMsg + 4);
    *wp = (WORD)bankSlot;

    // Send to server
    m_pXSocket->SendMsg(cMsg, 6);

    // Set waiting state
    m_stDialogBoxInfo[DIALOG_BANK].sV1 = -1;  // Processing
}
```

---

## Drag-Drop System

### Drag Initiation

```cpp
void CGame::OnMouseMove(short msX, short msY)
{
    // Check if in inventory dialog and holding mouse button
    if (!m_bItemDrop && m_bMousePressed) {
        if (IsPointInDialog(DIALOG_INVENTORY, msX, msY)) {
            // Find item under cursor
            int itemIndex = _iGetItemIndex(msX, msY);
            if (itemIndex != -1 && !m_bIsItemDisabled[itemIndex]) {
                // Start drag
                m_bItemDrop = true;
                m_stMCursor.sSelectedObjectID = itemIndex;
                m_sItemDragStartX = msX;
                m_sItemDragStartY = msY;
            }
        }
    }
}
```

### Drag Tracking

```cpp
// During drag, item follows cursor
if (m_bItemDrop) {
    CItem * pItem = m_pItemList[m_stMCursor.sSelectedObjectID];

    // Draw item sprite at cursor position
    m_pSprite[pItem->m_sSprite]->PutSpriteFast(
        msX - 16, msY - 16,  // Center on cursor
        pItem->m_sSpriteFrame, dwTime);
}
```

### Drop Handling

```cpp
void CGame::OnMouseUp(short msX, short msY)
{
    if (!m_bItemDrop) return;

    int itemIndex = m_stMCursor.sSelectedObjectID;

    // Determine drop target
    if (IsPointInDialog(DIALOG_INVENTORY, msX, msY)) {
        // Reorder within inventory
        bItemDrop_Inventory(msX, msY);
    }
    else if (IsPointInDialog(DIALOG_BANK, msX, msY)) {
        // Deposit to bank
        bItemDrop_Bank(msX, msY);
    }
    else if (IsPointInDialog(DIALOG_CHARACTER, msX, msY)) {
        // Try to equip
        bItemDrop_Character(msX, msY);
    }
    else if (IsPointInDialog(DIALOG_SKILL, msX, msY)) {
        // Assign to skill slot
        bItemDrop_SkillDialog(msX, msY);
    }
    else if (IsPointInDialog(DIALOG_SELLLIST, msX, msY)) {
        // Add to sell list
        bItemDrop_SellList(msX, msY);
    }
    else if (!IsPointInAnyDialog(msX, msY)) {
        // Drop to ground
        RequestDropItem(itemIndex, msX, msY);
    }

    // Clear drag state
    m_bItemDrop = false;
    m_stMCursor.sSelectedObjectID = -1;
}
```

### Inventory Reordering (bItemDrop_Inventory)

```cpp
bool CGame::bItemDrop_Inventory(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[DIALOG_INVENTORY].sX;
    short sY = m_stDialogBoxInfo[DIALOG_INVENTORY].sY;

    int itemIndex = m_stMCursor.sSelectedObjectID;
    CItem * pItem = m_pItemList[itemIndex];

    // Calculate new position within inventory bounds
    int dX = msX - sX - 16;  // Center offset
    int dY = msY - sY - 16;

    // Clamp to valid range
    if (dX < 0) dX = 0;
    if (dX > 170) dX = 170;
    if (dY < -10) dY = -10;
    if (dY > 95) dY = 95;

    // Update item position
    pItem->m_sX = dX;
    pItem->m_sY = dY;

    // Shift-drag: move all items of same type
    if (m_bShiftPressed) {
        for (int i = 0; i < DEF_MAXITEMS; i++) {
            if (i != itemIndex && m_pItemList[i] != NULL) {
                if (strcmp(m_pItemList[i]->m_cName, pItem->m_cName) == 0) {
                    m_pItemList[i]->m_sX = dX;
                    m_pItemList[i]->m_sY = dY;
                }
            }
        }
    }

    return true;
}
```

---

## Utility Functions

### Item Counting

```cpp
int CGame::_iGetTotalItemNum()
{
    int count = 0;
    for (int i = 0; i < DEF_MAXITEMS; i++) {
        if (m_pItemList[i] != NULL) count++;
    }
    return count;
}

int CGame::_iGetBankItemCount()
{
    int count = 0;
    for (int i = 0; i < DEF_MAXBANKITEMS; i++) {
        if (m_pBankList[i] != NULL) count++;
    }
    return count;
}
```

### Item Index Lookup

```cpp
int CGame::_iGetItemIndex(short msX, short msY)
{
    short sX = m_stDialogBoxInfo[DIALOG_INVENTORY].sX;
    short sY = m_stDialogBoxInfo[DIALOG_INVENTORY].sY;

    for (int i = 0; i < DEF_MAXITEMS; i++) {
        if (m_pItemList[i] != NULL && m_cItemOrder[i] != -1) {
            int itemX = sX + m_pItemList[i]->m_sX;
            int itemY = sY + m_pItemList[i]->m_sY;

            // Check if point is within item bounds (34x34)
            if (msX >= itemX && msX < itemX + 34 &&
                msY >= itemY && msY < itemY + 34) {
                return i;
            }
        }
    }
    return -1;  // No item found
}
```

### Item Order Management

```cpp
// m_cItemOrder tracks display priority
// -1 means slot is empty or item hidden
// 0-49 indicates display order

void CGame::_ReorderItems()
{
    // Reset order array
    for (int i = 0; i < DEF_MAXITEMS; i++) {
        m_cItemOrder[i] = -1;
    }

    // Assign order to existing items
    int order = 0;
    for (int i = 0; i < DEF_MAXITEMS; i++) {
        if (m_pItemList[i] != NULL && !m_bIsItemEquipped[i]) {
            m_cItemOrder[i] = order++;
        }
    }
}
```

---

## Network Messages

### Deposit to Bank

```cpp
// Message: MSGID_COMMAND_COMMON
// Command: DEF_COMMONTYPE_GIVEITEMTOCHAR

struct DepositMessage {
    DWORD dwMsgId;           // MSGID_COMMAND_COMMON
    WORD  wCommand;          // DEF_COMMONTYPE_GIVEITEMTOCHAR
    WORD  wInventorySlot;    // Source inventory slot
    WORD  wAmount;           // Quantity to deposit
    WORD  wNpcId;            // 1002 for bank NPC
    // Additional fields for NPC position
};
```

### Withdraw from Bank

```cpp
// Message: MSGID_REQUEST_RETRIEVEITEM

struct WithdrawMessage {
    DWORD dwMsgId;           // MSGID_REQUEST_RETRIEVEITEM
    WORD  wBankSlot;         // Bank slot index (0-120)
};
```

### Server Responses

```cpp
// Bank item update notifications
void CGame::NotifyMsg_ItemToBank(char * pData)
{
    // Parse updated bank contents
    // Refresh bank dialog display
}

// Inventory update after withdraw
void CGame::NotifyMsg_ItemLifeSpan(char * pData)
{
    // Item durability/state updates
}
```

---

## Integration Points

### Related Dialogs

| Dialog | Integration |
|--------|-------------|
| Character (Equipment) | Drag items to equip, shows equipped items |
| Skill | Assign consumables to quick slots |
| Shop | Purchase items to inventory |
| Sell List | Drag items to sell |
| Item Exchange | Trade items with other players |
| Item Upgrade | Use upgrade materials |
| Crafting | Use materials from inventory |

### System Dependencies

| System | Dependency |
|--------|------------|
| Network | All item operations require server validation |
| Equipment | Equipped items tracked separately |
| Weight | Inventory affects movement speed |
| Combat | Quick-use items during battle |

---

## Constants and Limits

```cpp
// Inventory limits
#define DEF_MAXITEMS          50    // Inventory capacity
#define DEF_MAXBANKITEMS     121    // Bank capacity (120 usable)

// Grid dimensions (implicit)
// Inventory: ~10 columns x 5 rows at 34px slots
// Bank list: 8-10 visible rows, scrollable

// Item position ranges (pixels within dialog)
// Inventory X: 0-170
// Inventory Y: -10 to 95

// Bank scrollbar
#define BANK_SCROLLBAR_HEIGHT 274   // Pixels
#define BANK_VISIBLE_ROWS      10   // Approximate

// Item slot size
#define ITEM_SLOT_SIZE         34   // Pixels (width and height)
```

---

## Known Issues / Technical Debt

1. **Free-form positioning**: Items store pixel positions rather than grid slots, making layout inconsistent
2. **No slot-based indexing**: Item lookup requires iterating all slots
3. **Mixed list/grid rendering**: Bank uses list view while inventory uses grid
4. **Hardcoded dimensions**: Slot sizes and positions are magic numbers throughout
5. **No item stacking merge**: Moving stackable items doesn't auto-merge with existing stacks
6. **Tight coupling**: Drag-drop logic spread across multiple `bItemDrop_*` functions
7. **Global state**: Item drag state stored in class members, not scoped to operation
8. **No undo**: Accidental drops cannot be reversed client-side

---

## Modernization Notes

### Recommended Changes

1. **Grid-based slot system**: Use explicit row/column indices instead of pixel positions
2. **Unified slot structure**: Both inventory and bank should use same `inventory_slot` type
3. **Stack merging**: Automatically combine compatible stackable items
4. **Drag-drop abstraction**: Single drag-drop manager handling all dialogs
5. **RAII for drag state**: Scoped drag operation object instead of global flags
6. **Event callbacks**: Replace switch statements with registered handlers
7. **Data-driven layout**: Load grid dimensions from configuration

### Preserved Behavior

- 50 inventory slots
- 121 bank slots (120 usable)
- Double-click to equip/use
- Right-click bank items to withdraw
- Shift-drag for batch operations
- Item color variations
- Quantity display for stackables
- Weight system integration
