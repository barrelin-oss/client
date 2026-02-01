# Commerce Dialog Systems

## Overview

The Helbreath client implements four primary commerce-related dialog systems for all economic transactions in the game. These dialogs handle NPC shop interactions, item selling/repair, player-to-player trading, and public item auctions. All commerce dialogs are implemented within the monolithic `CGame` class in Game.cpp.

**Dialog Box IDs:**
| ID | Name | Purpose |
|----|------|---------|
| 11 | Shop | Buying items from NPCs |
| 23 | Sell/Repair | Selling items or repairing equipment |
| 27 | Exchange | Player-to-player trading |
| 31 | Sell List | Public item auction/storefront |

---

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | All dialog drawing, click handling, and state management (~48,500 lines) |
| `Game.h` | Dialog structure definitions, member declarations |
| `Item.h` / `Item.cpp` | CItem class with pricing, attributes, durability |
| `NetMessages.h` | Commerce-related packet type definitions |
| `lan_eng.h` | English localization strings for commerce UI |

---

## 1. Shop Dialog (ID: 11)

### Purpose
Allows players to purchase items from NPC merchants. Each shop type loads different inventory from content files.

### Key Functions
```cpp
void DrawDialogBox_Shop(short msX, short msY, short msZ, char cLB);
void DlgBoxClick_Shop(short msX, short msY);
void _LoadShopMenuContents(char cType);
BOOL __bDecodeContentsAndBuildItemForSaleList(char* pBuffer);
```

### Constants
```cpp
#define DEF_MAXMENUITEMS     140    // Maximum items per shop
#define DEF_MAXITEMNAME      21     // Item name length
```

### Data Structure
```cpp
// Dialog state in m_stDialogBoxInfo[11]
struct {
    short sV1;      // Shop type/ID (determines content file)
    short sV3;      // Purchase quantity (1-50)
    char  cMode;    // 0=list view, >0=item detail (cMode-1 = selected index)
    short sView;    // Scroll position in item list
    BOOL  bFlag;    // Enable state
    BOOL  bIsScrollSelected;  // Scroll interaction flag
};

// Shop inventory storage
CItem* m_pItemForSaleList[DEF_MAXMENUITEMS];  // 140 items max
```

### CItem Structure (relevant fields)
```cpp
class CItem {
    char  m_cName[21];      // Item name
    char  m_cItemType;      // Item category (1-12)
    short m_sSprite;        // Sprite ID for display
    short m_sSpriteFrame;   // Animation frame
    WORD  m_wPrice;         // Base purchase price
    WORD  m_wWeight;        // Weight in stones
    WORD  m_wCurLifeSpan;   // Current durability
    WORD  m_wMaxLifeSpan;   // Maximum durability
    DWORD m_dwAttribute;    // Attribute bitmask
    short m_sItemEffectValue[6];  // Effect values
    char  m_cEquipPos;      // Equipment slot (0-14)
    char  m_cItemColor;     // Dye color index
    int   m_iCount;         // Stack quantity
};
```

### Price Calculation Formula
```cpp
// Discount based on player charisma
int iDiscountRatio = (m_iCharisma - 10) / 4;  // 1% per 4 CHR above 10

// NPC-specific discount/markup
int iBaseCost = (int)(m_wPrice * ((100 + m_cDiscount) / 100.0f));

// Apply charisma discount
int iDiscountAmount = (int)(m_wPrice * (iDiscountRatio / 100.0f));
int iFinalCost = iBaseCost - iDiscountAmount;

// Minimum price floor (prevents negative/zero prices)
if (iFinalCost < (m_wPrice / 2 - 1)) {
    iFinalCost = m_wPrice / 2 - 1;
}
```

### UI Layout
```
+------------------------------------------+
|  ITEM                           PRICE    |  Y=35
+------------------------------------------+
|  [Item 1 Name]                  [Cost]   |  Y=65, row height=18
|  [Item 2 Name]                  [Cost]   |
|  [Item 3 Name]                  [Cost]   |
|  ... (13 visible items)                  |
|  [Item 13 Name]                 [Cost]   |  Y=281
+------------------------------------------+  Scrollbar: X=235-260
|                                          |
|  [Item Sprite]   [Item Name]             |  Detail view (cMode>0)
|                  Weight: X Stone         |
|                  Price: X Gold           |
|                                          |
|  [-10][-1] Qty: XX [+1][+10]             |  Quantity controls
|                                          |
|  [  PURCHASE  ]    [  CANCEL  ]          |  Y=292
+------------------------------------------+
```

### Coordinate Definitions
| Element | X Range | Y Range |
|---------|---------|---------|
| Item list | 20-220 | 65-281 |
| Price column | 148-260 | 65-281 |
| Scrollbar | 235-260 | 10-330 |
| Item sprite (detail) | 27-90 | 74-120 |
| Quantity -10 | 145-162 | - |
| Quantity -1 | 163-180 | - |
| Quantity +1 | 181-198 | - |
| Quantity +10 | 199-216 | - |
| Purchase button | sX+30 | sY+292 |
| Cancel button | sX+154 | sY+292 |

### Button Sprites
| Button | Normal | Hover |
|--------|--------|-------|
| Purchase | 38 | 39 |
| Cancel | 16 | 17 |

### Shop Content Loading
```cpp
// Content files: contents\contents{cType}.txt
void _LoadShopMenuContents(char cType) {
    // Clear existing items
    for (int i = 0; i < DEF_MAXMENUITEMS; i++) {
        if (m_pItemForSaleList[i] != NULL) {
            delete m_pItemForSaleList[i];
            m_pItemForSaleList[i] = NULL;
        }
    }

    // Load file contents{cType}.txt
    // Parse with __bDecodeContentsAndBuildItemForSaleList()
}
```

### Content File Format
```
item-name,item-type,equip-pos,effect1,effect2,effect3,effect4,effect5,effect6,
value1,value2,value3,value4,value5,value6,
price,weight,sprite,spriteframe,color,gender,level
```

### Network Protocol

**Purchase Request:**
```cpp
bSendCommand(MSGID_COMMAND_COMMON,
             DEF_COMMONTYPE_REQ_PURCHASEITEM,
             NULL,                    // No position
             m_stDialogBoxInfo[11].sV3,  // Quantity
             m_pItemForSaleList[iIndex]->m_cName,  // Item name
             NULL);
```

**Server Responses:**
| Packet | ID | Description |
|--------|-----|-------------|
| DEF_NOTIFY_ITEMPURCHASED | 0x0B06 | Purchase successful |
| DEF_NOTIFY_NOTENOUGHGOLD | 0x0B08 | Insufficient gold |

---

## 2. Sell/Repair Dialog (ID: 23)

### Purpose
Handles selling items to NPCs and repairing damaged equipment at blacksmiths.

### Key Functions
```cpp
void DrawDialogBox_SellorRepairItem(short msX, short msY);
void DlgBoxClick_ItemSellorRepair(short msX, short msY);
void NotifyMsg_SellItemPrice(char* pData);
void NotifyMsg_RepairItemPrice(char* pData);
void NotifyMsg_ItemRepaired(char* pData);
```

### Data Structure
```cpp
// Dialog state in m_stDialogBoxInfo[23]
struct {
    char  cMode;    // 1=sell confirm, 2=repair confirm, 3=sell processing, 4=repair processing
    short sV1;      // Inventory slot ID (0-49)
    short sV2;      // Current durability
    short sV3;      // Value (sell price) or repair cost
    short sV4;      // Item quantity/count
    short sX, sY;   // Dialog position
};
```

### State Machine
```
                    +------------------+
                    |   Dialog Open    |
                    +--------+---------+
                             |
              +--------------+--------------+
              |                             |
     +--------v--------+           +--------v--------+
     | cMode=1: SELL   |           | cMode=2: REPAIR |
     | Confirm screen  |           | Confirm screen  |
     +--------+--------+           +--------+--------+
              |                             |
              | Click "Sell"                | Click "Repair"
              v                             v
     +--------+--------+           +--------+--------+
     | cMode=3: SELLING|           | cMode=4: REPAIR |
     | Processing...   |           | Processing...   |
     +--------+--------+           +--------+--------+
              |                             |
              | Server response             | Server response
              v                             v
     +--------+--------+           +--------+--------+
     | Success/Fail    |           | Success/Fail    |
     | Close dialog    |           | Close dialog    |
     +--------+--------+           +--------+--------+
```

### UI Layout (Sell Mode)
```
+------------------------------------------+
|                                          |
|         [Item Sprite]                    |  Y=114
|                                          |
|         Item Name (x Quantity)           |  Y=60
|                                          |
|         Endurance: XX                    |  Y=113
|         Value: XX Gold                   |  Y=128
|                                          |
|         Do you want to sell?             |
|                                          |
|  [   SELL   ]      [  CANCEL  ]          |  Y=292
+------------------------------------------+
```

### UI Layout (Repair Mode)
```
+------------------------------------------+
|                                          |
|         [Item Sprite]                    |  Y=114
|                                          |
|         Item Name                        |  Y=60
|                                          |
|         Endurance: XX                    |  Y=113
|         Cost: XX Gold                    |  Y=128
|                                          |
|         Do you want to repair?           |
|                                          |
|  [  REPAIR  ]      [  CANCEL  ]          |  Y=292
+------------------------------------------+
```

### Processing States Display
```cpp
// cMode == 3 (Sell Processing)
"Processing the item sale..."
"You cannot give or sell until this finishes."

// cMode == 4 (Repair Processing)
"Processing the item repair..."
"You cannot give or sell until this finishes."
```

### Item Locking
```cpp
// When entering dialog
m_bIsItemDisabled[sV1] = TRUE;   // Lock item

// On cancel or completion
m_bIsItemDisabled[sV1] = FALSE;  // Unlock item
```

### Network Protocol

**Sell Flow:**
```cpp
// 1. Player initiates sell (server sends price quote)
// Incoming: DEF_NOTIFY_SELLITEMPRICE (0x0B2D)
struct SellPriceData {
    DWORD dwItemID;       // Inventory slot
    DWORD dwDurability;   // Current durability
    DWORD dwPrice;        // Offered price
    DWORD dwUnknown;      // Reserved
    char  cName[20];      // Item name
};

// 2. Player confirms sale
bSendCommand(MSGID_COMMAND_COMMON,
             DEF_COMMONTYPE_REQ_SELLITEMCONFIRM,
             NULL,
             sV1,           // Item ID
             sV4,           // Count
             sV3,           // Price
             m_pItemList[sV1]->m_cName);

// 3. Server responses
DEF_NOTIFY_ITEMSOLD (0x0B31)        // Success
DEF_NOTIFY_CANNOTSELLITEM (0x0B2C) // Failure
```

**Repair Flow:**
```cpp
// 1. Player initiates repair (server sends cost quote)
// Incoming: DEF_NOTIFY_REPAIRITEMPRICE (0x0B2F)
// Same structure as SellPriceData

// 2. Player confirms repair
bSendCommand(MSGID_COMMAND_COMMON,
             DEF_COMMONTYPE_REQ_REPAIRITEMCONFIRM,
             NULL,
             sV1,           // Item ID
             m_pItemList[sV1]->m_cName);

// 3. Server responses
DEF_NOTIFY_ITEMREPAIRED (0x0B30)     // Success
DEF_NOTIFY_CANNOTREPAIRITEM (0x0B2E) // Failure
```

### Sell Error Codes
| Code | Message |
|------|---------|
| 1 | "Cannot sell this item" |
| 2 | "Bounded items cannot be sold" |
| 3 | "Cannot sell" + "Special attributes" |
| 4 | "Cannot sell sealed item" + "Try different item" |

### Repair Error Codes
| Code | Message |
|------|---------|
| 1 | "Repair is not required" |
| 2 | "Cannot be repaired by this smith" |

### Item Color Rendering
```cpp
// Dyed items use palette-based color transformation
// Palette arrays: m_wR[], m_wG[], m_wB[] (base)
//                m_wWR[], m_wWG[], m_wWB[] (white variant)

// Sprite types 1, 2, 3, 15 use white palette
// All others use base palette
int iR = m_wR[cItemColor] - m_wR[0];  // Color offset
int iG = m_wG[cItemColor] - m_wG[0];
int iB = m_wB[cItemColor] - m_wB[0];
```

---

## 3. Exchange Dialog (ID: 27)

### Purpose
Enables secure player-to-player item and gold trading with dual confirmation.

### Key Functions
```cpp
void DrawDialogBox_Exchange(short msX, short msY);
void DlgBoxClick_Exchange(short msX, short msY);
void NotifyMsg_OpenExchangeWindow(char* pData);
void NotifyMsg_SetExchangeItem(char* pData);
void NotifyMsg_CancelExchangeItem(char* pData);
```

### Data Structure
```cpp
// Dialog state in m_stDialogBoxInfo[27]
struct {
    // My side
    short sV1;      // My item sprite ID (-1 if empty)
    short sV2;      // My item frame
    short sV3;      // My gold amount
    short sV4;      // My item color
    short sV9;      // My item current durability
    short sV10;     // My item max durability

    // Partner side
    short sV5;      // Partner item sprite ID (-1 if empty)
    short sV6;      // Partner item frame
    short sV7;      // Partner gold amount
    short sV8;      // Partner item color
    short sV11;     // Partner completion percentage

    // Names and attributes
    char  cStr[32];   // My item name
    char  cStr3[32];  // Partner item name
    char  cStr4[32];  // Partner character name
    DWORD dwV1;       // My item attributes
    DWORD dwV2;       // Partner item attributes

    // State
    char  cMode;    // 1=selecting, 2=confirmed, 3=exchanging
    short sView;    // Selection index
};
```

### State Machine
```
    +-------------------+
    |  Exchange Request |
    +--------+----------+
             |
             v
    +--------+----------+
    | cMode=1: SELECTING|<--------+
    | Both players add  |         |
    | items/gold        |         |
    +--------+----------+         |
             |                    |
             | Click "Exchange"   | Cancel
             v                    |
    +--------+----------+         |
    | cMode=2: WAITING  |---------+
    | For partner       |
    | confirmation      |
    +--------+----------+
             |
             | Both confirmed
             v
    +--------+----------+
    | cMode=3: COMPLETE |
    | Items exchanged   |
    +-------------------+
```

### UI Layout
```
+------------------------------------------------------------------+
|                                                                  |
|  [My Name]                              [Partner Name]           |  Y=42
|                                                                  |
|  +------------------+                  +------------------+      |
|  |                  |                  |                  |      |
|  | [My Item Sprite] |                  | [Partner Sprite] |      |  Y=100
|  |                  |                  |                  |      |
|  +------------------+                  +------------------+      |
|                                                                  |
|  My Item Name                          Partner Item Name         |  Y=145
|  Attribute 1                           Attribute 1               |  Y=160+
|  Attribute 2                           Attribute 2               |
|  Gold: XXXX                            Gold: XXXX                |
|                                                                  |
|  Quantity: X                           Completion: XX%           |
|  Endurance: X/X                                                  |
|                                                                  |
|            [  EXCHANGE  ]       [  CANCEL  ]                     |  Y=310
+------------------------------------------------------------------+
```

### Coordinate Definitions
| Element | X Position | Y Position |
|---------|------------|------------|
| My name | 80-180 | 42 |
| Partner name | 250-540 | 42 |
| My item sprite | 130 | 100 |
| Partner item sprite | 400 | 100 |
| My item name | 15-250 | 145 |
| Partner item name | 270-520 | 145 |
| Exchange button | 220 | 310 |
| Cancel button | 450 | 310 |

### Exchange Button Conditions
```cpp
// Exchange button only enabled when:
// - Both sides have items (sV1 != -1 AND sV5 != -1)
// - OR both sides have gold (sV3 > 0 AND sV7 > 0)
// - AND cMode == 1 (selection phase)
BOOL bCanExchange = (sV1 != -1 && sV5 != -1) && (cMode == 1);
```

### Network Protocol

**Incoming Packets:**
```cpp
// Exchange window opened
DEF_NOTIFY_OPENEXCHANGEWINDOW (0x0B5E)
// Initializes dialog, sets partner name

// Partner set their item
DEF_NOTIFY_SETEXCHANGEITEM (0x0B5F)
struct ExchangeItemData {
    short sSprite;
    short sFrame;
    short sColor;
    int   iGold;
    short sCurDur;
    short sMaxDur;
    int   iCompletion;
    char  cItemName[32];
    DWORD dwAttributes;
};

// Partner cancelled
DEF_NOTIFY_CANCELEXCHANGEITEM (0x0B60)
// Closes dialog, returns items
```

**Outgoing Commands:**
```cpp
// Set my item for trade
DEF_COMMONTYPE_SETEXCHANGEITEM (0x0A1F)

// Confirm exchange
DEF_COMMONTYPE_CONFIRMEXCHANGEITEM (0x0A20)

// Cancel exchange
DEF_COMMONTYPE_CANCELEXCHANGEITEM (0x0A21)
```

### Gold Display
```cpp
// Large gold amounts formatted for readability
void DisplayGold(int iAmount) {
    // Stores formatted string in G_cTxt[]
    // Example: 1,234,567 Gold
}
```

---

## 4. Sell List Dialog (ID: 31)

### Purpose
Allows players to set up a personal storefront with up to 12 items for public sale.

### Key Functions
```cpp
void DrawDialogBox_SellList(short msX, short msY);
void DlgBoxClick_SellList(short msX, short msY);
```

### Constants
```cpp
#define DEF_MAXSELLLIST  12  // Maximum items in sell list
```

### Data Structure
```cpp
// Global sell list array
struct SellListItem {
    int iIndex;     // Inventory item ID (-1 if empty)
    int iAmount;    // Quantity for sale
} m_stSellItemList[DEF_MAXSELLLIST];

// Dialog state in m_stDialogBoxInfo[31]
struct {
    short sX, sY;    // Dialog position
    short sSizeX;    // Width for text alignment
    char  cMode;     // Reserved (currently 0)
    short sView;     // Scroll offset
};
```

### UI Layout
```
+------------------------------------------+
|  SELL LIST                               |
+------------------------------------------+
|                                          |
|  1. Item Name (attribute)                |  Y=55, row height=15
|  2. 5x Healing Potion                    |
|  3. Iron Sword (SM+3)                    |
|  4. ...                                  |
|  ...                                     |
|  12. [Empty slot]                        |
|                                          |
+------------------------------------------+
|  Instructions:                           |
|  - Drag items from inventory             |
|  - Click item name to remove             |
|  - Up to 12 items allowed                |
|                                          |
|  [   SELL   ]      [  CANCEL  ]          |  Y=292
+------------------------------------------+
```

### Item Display Formats
```cpp
// With quantity > 1
sprintf(cTxt, "%d %s", iAmount, cItemName);  // "5 Healing Potion"

// With attributes (short)
sprintf(cTxt, "%s (%s)", cItemName, cAttr1); // "Iron Sword (SM+3)"

// With attributes (long, >36 chars total)
// Line 1: Item name
// Line 2: (attribute1, attribute2)
```

### Text Colors
| Condition | RGB Color |
|-----------|-----------|
| Hover | (255, 255, 255) White |
| Normal | (45, 25, 25) Dark brown |
| Special item | (0, 255, 50) Green |

### Click Handling
```cpp
void DlgBoxClick_SellList(short msX, short msY) {
    // Detect which item row was clicked
    for (int i = 0; i < DEF_MAXSELLLIST; i++) {
        if (msY >= sY + 55 + i*15 && msY <= sY + 69 + i*15) {
            if (m_stSellItemList[i].iIndex != -1) {
                // Remove item from list
                int iSlot = m_stSellItemList[i].iIndex;
                m_bIsItemDisabled[iSlot] = FALSE;  // Unlock
                m_stSellItemList[i].iIndex = -1;
                m_stSellItemList[i].iAmount = 0;

                // Compact array
                CompactSellList();

                // Play click sound
                PlaySound('E', 14, 5);
            }
        }
    }
}
```

### Array Compaction
```cpp
void CompactSellList() {
    for (int x = 0; x < DEF_MAXSELLLIST - 1; x++) {
        if (m_stSellItemList[x].iIndex == -1) {
            // Shift next item up
            m_stSellItemList[x].iIndex = m_stSellItemList[x+1].iIndex;
            m_stSellItemList[x].iAmount = m_stSellItemList[x+1].iAmount;
            m_stSellItemList[x+1].iIndex = -1;
            m_stSellItemList[x+1].iAmount = 0;
        }
    }
}
```

### Item Restrictions
Cannot add to sell list:
- Exhausted items (`m_wCurLifeSpan == 0`)
- Bounded/locked items
- Items with certain special attributes
- Sealed items

### Network Protocol
```cpp
// Submit sell list to server
bSendCommand(MSGID_REQUEST_SELLITEMLIST, ...);
// Sends entire m_stSellItemList array
// Server validates and creates public storefront
```

### Help Text (when list is full)
```
1. "Drag the item that you want to sell from your bag."
2. "You can sell up to 12 items simultaneously."
3. "You cannot sell exhausted items."
4. "If you want to remove selected item from the list,"
5. "click the name of the item."
6. "* Drop the items to sell here! *"
```

---

## Commerce System Globals

### Player Stats Affecting Commerce
```cpp
int  m_iCharisma;      // Charisma stat (affects discounts)
char m_cDiscount;      // NPC-specific discount modifier (-20 to +20)
```

### Inventory Management
```cpp
CItem* m_pItemList[50];           // Player inventory (50 slots)
CItem* m_pBankList[121];          // Bank storage (121 slots)
BOOL   m_bIsItemDisabled[50];     // Per-slot lock flags
CItem* m_pItemForSaleList[140];   // Current shop inventory
```

### Gold
```cpp
DWORD m_dwGold;  // Player's gold amount
```

---

## Helper Functions

### GetItemName
```cpp
// Extract item name and attributes for display
void GetItemName(CItem* pItem, char* pName1, char* pName2, char* pName3);
void GetItemName(char* pName, DWORD dwAttr, char* pName1, char* pName2, char* pName3);
```

### Inventory Helpers
```cpp
int _iGetTotalItemNum();           // Count items in inventory
int _iGetBankItemNum();            // Count items in bank
BOOL _bCheckItemByType(int type);  // Check for item type
```

### Dialog Control
```cpp
void EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char* pString);
void DisableDialogBox(int iBoxID);
BOOL bIsDialogEnabled(int iBoxID);
```

### Audio Feedback
```cpp
void PlaySound(char cType, int iNum, int iDist, long lPan = 0);
// cType='E' for effects, iNum=14 for click sound
```

### Event Messages
```cpp
void AddEventList(char* pMsg, unsigned char iLastSec);
// Display message in event log (iLastSec = display duration)
```

---

## Localization Strings

### Shop Dialog (lan_eng.h)
```cpp
#define DRAW_DIALOGBOX_SHOP1    "ITEM"
#define DRAW_DIALOGBOX_SHOP3    "PRICE"
#define DRAW_DIALOGBOX_SHOP6    "Weight"
#define DRAW_DIALOGBOX_SHOP7    ": %d Gold"
#define DRAW_DIALOGBOX_SHOP8    ": %d Stone"
```

### Sell/Repair Dialog
```cpp
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1   "%d %s"
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM2   "Endurance: %d"
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM3   "Value: %d Gold"
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM4   "Do you want to sell?"
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM6   "Cost: %d Gold"
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM7   "Do you want to repair?"
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM8   " Processing the item sale.."
#define DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM11  " Processing the item repair..."
```

### Exchange Dialog
```cpp
#define DRAW_DIALOGBOX_EXCHANGE1  "My Item"
#define DRAW_DIALOGBOX_EXCHANGE2  "Quantity: %d"
#define DRAW_DIALOGBOX_EXCHANGE3  "Endurance: %d/%d"
#define DRAW_DIALOGBOX_EXCHANGE4  "Completion: %d%%"
#define DRAW_DIALOGBOX_EXCHANGE5  "%s's item"
```

### Notification Messages
```cpp
#define NOTIFYMSG_ITEMPURCHASED       "Bought %s for %d Gold"
#define NOTIFYMSG_ITEMREPAIRED1       "%s: has been repaired."
#define NOTIFYMSG_CANNOT_REPAIR_ITEM1 "Repair not required."
#define NOTIFYMSG_CANNOT_REPAIR_ITEM2 "Cannot be repaired by this smith."
#define NOTIFYMSG_CANNOT_SELL_ITEM1   "Cannot sell this item"
#define NOTIFYMSG_CANNOT_SELL_ITEM2   "Bounded items cannot be sold"
```

---

## Integration Points

### With Inventory System
- Items dragged from inventory to commerce dialogs
- `m_bIsItemDisabled[]` prevents item use during transactions
- Inventory updates on successful transactions

### With Network System
- All transactions require server validation
- Asynchronous request/response pattern
- Price quotes from server before confirmation

### With Input System
- Mouse click detection for buttons and item selection
- Drag-and-drop for adding items to sell list/exchange
- Scroll wheel for shop list navigation

### With Audio System
- Click sounds on button presses (Sound ID 14)
- Success/failure notification sounds

### With Event System
- Transaction results displayed in event log
- Error messages shown to player

---

## Known Issues / Technical Debt

1. **Hardcoded Coordinates** - All UI positions are magic numbers
2. **No Input Validation** - Client trusts server price quotes
3. **Global State** - Commerce state scattered across multiple arrays
4. **Tight Coupling** - Drawing/logic mixed in single functions
5. **Memory Management** - Manual `new`/`delete` for shop items
6. **No Transaction Rollback** - Failed transactions may leave items locked
7. **Fixed Resolution** - UI assumes 800x600 display

---

## Modernization Notes

### Recommended Improvements
1. **Data-Driven UI** - Define layouts in YAML/JSON
2. **State Pattern** - Proper state machine for transaction flows
3. **RAII** - Smart pointers for item ownership
4. **Event-Based** - Decouple UI from network responses
5. **Validation** - Client-side validation before server requests
6. **Responsive Layout** - Resolution-independent coordinates
7. **Localization** - Runtime string loading instead of compile-time defines

### Modern Class Structure
```cpp
namespace hb::ui {
    class ShopDialog : public Dialog { ... };
    class SellRepairDialog : public Dialog { ... };
    class ExchangeDialog : public Dialog { ... };
    class SellListDialog : public Dialog { ... };
}

namespace hb::commerce {
    class TransactionManager { ... };
    class PriceCalculator { ... };
    class ItemValidator { ... };
}
```
