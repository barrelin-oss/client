# Inventory System

## Overview

The legacy Helbreath inventory system manages all item storage, manipulation, and display for the player. It uses a flat array-based design with 50 inventory slots, supports 12 different item types, and communicates with the server via binary packet synchronization. Items have complex attribute encoding packed into 32-bit values for effects, bonuses, and special properties.

## Source Files

| File | Purpose |
|------|---------|
| `Item.h` | CItem class definition, item type constants, equipment position constants |
| `Item.cpp` | CItem constructor/destructor implementation |
| `ItemName.h` | CItemName class for item name database |
| `ItemName.cpp` | CItemName implementation |
| `BuildItem.h` | CBuildItem class for crafting recipes |
| `BuildItem.cpp` | CBuildItem implementation |
| `Game.h` | Inventory arrays, item management declarations |
| `Game.cpp` | All inventory logic, rendering, packet handling |

---

## Key Data Structures

### CItem Class

The core item data structure (from `Item.h`):

```cpp
class CItem {
public:
    CItem();
    virtual ~CItem();

    // Identity
    char  m_cName[21];              // Item name (max 20 characters + null)

    // Classification
    char  m_cItemType;              // Item type (1 of 12 types)
    char  m_cEquipPos;              // Equipment slot (1 of 15 positions)
    char  m_cItemColor;             // Color/rarity code (0-15)

    // Combat Properties
    char  m_cSpeed;                 // Attack speed modifier

    // Requirements
    char  m_cGenderLimit;           // Gender restriction (0=any, 1=male, 2=female)
    short m_sLevelLimit;            // Minimum level requirement

    // Visual Properties
    short m_sSprite;                // Sprite ID in sprite list
    short m_sSpriteFrame;           // Frame index within sprite
    short m_sX, m_sY;               // Position in inventory dialog (pixels)

    // Special Effect Values (3 slots)
    short m_sItemSpecEffectValue1;
    short m_sItemSpecEffectValue2;
    short m_sItemSpecEffectValue3;

    // General Effect Values (6 slots)
    short m_sItemEffectValue1;      // Primary effect (damage, defense, etc.)
    short m_sItemEffectValue2;      // Secondary effect
    short m_sItemEffectValue3;      // Tertiary effect
    short m_sItemEffectValue4;      // Quaternary effect
    short m_sItemEffectValue5;      // Quinary effect
    short m_sItemEffectValue6;      // Senary effect

    // Durability
    WORD  m_wCurLifeSpan;           // Current durability (game ticks remaining)
    WORD  m_wMaxLifeSpan;           // Maximum durability

    // Economy
    WORD  m_wPrice;                 // Buy/sell price in gold
    WORD  m_wWeight;                // Weight in units

    // Quantity
    DWORD m_dwCount;                // Stack count (for stackable items)

    // Packed Attributes
    DWORD m_dwAttribute;            // 32-bit packed effect/bonus flags
};
```

### CItemName Class

Item name database entry (from `ItemName.h`):

```cpp
class CItemName {
public:
    CItemName();
    virtual ~CItemName();

    char m_cOriginName[21];         // Original/internal item name (20 chars)
    char m_cName[34];               // Localized display name (33 chars)
};
```

### CBuildItem Class

Crafting recipe definition (from `BuildItem.h`):

```cpp
class CBuildItem {
public:
    BOOL  m_bBuildEnabled;          // Recipe is craftable
    char  m_cName[21];              // Recipe/result name
    int   m_iSkillLimit;            // Minimum skill level required
    int   m_iMaxSkill;              // Maximum skill level for recipe
    int   m_iSprH;                  // Sprite handle for display
    int   m_iSprFrame;              // Sprite frame for display

    char  m_cElementName[7][21];    // Ingredient names (index 0 unused, 1-6 used)
    DWORD m_iElementCount[7];       // Ingredient quantities required
    BOOL  m_bElementFlag[7];        // Ingredient availability flags
};
```

---

## Constants & Limits

### Inventory Size Constants

```cpp
#define DEF_MAXITEMS            50      // Player inventory capacity
#define DEF_MAXBANKITEMS        121     // Bank storage capacity (120 + 1)
#define DEF_MAXITEMNAMES        1000    // Item name database size
#define DEF_MAXBUILDITEMS       100     // Craftable recipe count
#define DEF_MAXMENUITEMS        140     // Shop item list capacity
```

### Item Type Constants

```cpp
#define DEF_ITEMTYPE_NONE                    0   // Invalid/no type
#define DEF_ITEMTYPE_EQUIP                   1   // Equipable (armor/weapon)
#define DEF_ITEMTYPE_APPLY                   2   // Apply effect to target
#define DEF_ITEMTYPE_USE_DEPLETE             3   // Use once, then deplete
#define DEF_ITEMTYPE_INSTALL                 4   // Install/place item
#define DEF_ITEMTYPE_CONSUME                 5   // Consumable item
#define DEF_ITEMTYPE_ARROW                   6   // Arrow/projectile ammo
#define DEF_ITEMTYPE_EAT                     7   // Food item
#define DEF_ITEMTYPE_USE_SKILL               8   // Activates a skill
#define DEF_ITEMTYPE_USE_PERM                9   // Permanent effect
#define DEF_ITEMTYPE_USE_SKILL_ENABLEDIALOGBOX 10 // Skill with dialog UI
#define DEF_ITEMTYPE_USE_DEPLETE_DEST        11  // Deplete with destination
#define DEF_ITEMTYPE_MATERIAL                12  // Crafting material
```

### Equipment Position Constants

```cpp
#define DEF_MAXITEMEQUIPPOS     15      // Total equipment slots

#define DEF_EQUIPPOS_NONE       0       // Not equipped / no slot
#define DEF_EQUIPPOS_HEAD       1       // Helmet/headgear
#define DEF_EQUIPPOS_BODY       2       // Chest armor/shirt
#define DEF_EQUIPPOS_ARMS       3       // Gloves/arm guards
#define DEF_EQUIPPOS_PANTS      4       // Leg armor/pants
#define DEF_EQUIPPOS_BOOTS      5       // Footwear
#define DEF_EQUIPPOS_NECK       6       // Necklace/amulet
#define DEF_EQUIPPOS_LHAND      7       // Left hand (shield/offhand)
#define DEF_EQUIPPOS_RHAND      8       // Right hand (main weapon)
#define DEF_EQUIPPOS_TWOHAND    9       // Two-handed weapon
#define DEF_EQUIPPOS_RFINGER    10      // Right ring
#define DEF_EQUIPPOS_LFINGER    11      // Left ring
#define DEF_EQUIPPOS_BACK       12      // Cape/cloak/backpack
#define DEF_EQUIPPOS_FULLBODY   13      // Full body robe/armor
// Position 14 reserved
```

### Gender Restriction Values

```cpp
m_cGenderLimit:
    0 = No restriction (any gender)
    1 = Male only
    2 = Female only
```

---

## Item Color System

The `m_cItemColor` field (0-15) indicates item rarity/quality and determines visual tinting:

| Color Code | Color | Typical Meaning |
|------------|-------|-----------------|
| 0 | White | Normal/common quality |
| 1 | Red | Rare/epic quality |
| 2 | Blue | Uncommon/magic quality |
| 3 | Green | Uncommon/enchanted |
| 4 | Yellow | Special/unique |
| 5 | Cyan | Special effect |
| 6-15 | Various | Custom/special rarities |

Color is applied via RGB delta from the color lookup tables:
```cpp
// In CGame class
WORD m_wR[16], m_wG[16], m_wB[16];  // Color lookup tables

// Applied as:
deltaR = m_wR[color] - m_wR[0];
deltaG = m_wG[color] - m_wG[0];
deltaB = m_wB[color] - m_wB[0];
```

---

## Item Attribute Encoding

The `m_dwAttribute` field is a 32-bit packed structure containing multiple effects and bonuses:

### Bit Layout

```
Bits 31-28: Bonus stat type 2 (4 bits)
Bits 27-24: Reserved
Bits 23-20: Bonus value 1 (4 bits)
Bits 19-16: Effect type 1 (4 bits)
Bits 15-12: Effect type 2 (4 bits)
Bits 11-8:  Effect value (8 bits)
Bits 7-4:   Reserved
Bits 3-1:   Reserved
Bit 0:      Binding flag (0=tradeable, 1=soulbound)
```

### Unpacking Logic (from Game.cpp)

```cpp
// Check if item has special attributes
if ((pItem->m_dwAttribute & 0x00F0F000) != 0) {
    // Extract effect type 1 (bits 20-23)
    dwType1  = (pItem->m_dwAttribute & 0x00F00000) >> 20;

    // Extract effect value 1 (bits 16-19)
    dwValue1 = (pItem->m_dwAttribute & 0x000F0000) >> 16;

    // Extract effect type 2 (bits 12-15)
    dwType2  = (pItem->m_dwAttribute & 0x0000F000) >> 12;

    // Extract effect value 2 (bits 8-11)
    dwValue2 = (pItem->m_dwAttribute & 0x00000F00) >> 8;
}

// Extract bonus stat type (bits 28-31)
dwValue3 = (pItem->m_dwAttribute & 0xF0000000) >> 28;
```

### Effect Types

| Value | Effect | Formula |
|-------|--------|---------|
| 0 | None | No effect |
| 1 | Super Attack Bonus | +N damage to super attacks |
| 2 | Experience Bonus | +N% experience gain |
| 3 | Gold Bonus | +N% gold drops |
| 4 | HP Recovery | +N HP per tick |
| 5 | MP Recovery | +N MP per tick |
| 6 | Fire Resistance | +N×4% fire resist |
| 7 | Water Resistance | +N×5% water resist |
| 8 | Ice Resistance | +N×7% ice resist |
| 9 | Earth Resistance | +N×3% earth resist |
| 10 | Spell Accuracy | +N×3% spell hit |
| 11 | Poison Resistance | +N% poison resist |
| 12 | Critical Bonus | +N% critical damage |
| 13 | Physical Absorption | Absorb N% physical |
| 14 | Magic Absorption | Absorb N% magic |
| 15 | Skill Bonus | +N to specific skill |

---

## Inventory Management

### CGame Member Variables

```cpp
class CGame {
    // Inventory storage
    CItem * m_pItemList[DEF_MAXITEMS];           // 50 inventory slots
    CItem * m_pBankList[DEF_MAXBANKITEMS];       // 121 bank slots
    CItem * m_pItemForSaleList[DEF_MAXMENUITEMS]; // Shop items

    // Item name database
    CItemName * m_pItemNameList[DEF_MAXITEMNAMES]; // 1000 names

    // Crafting recipes
    CBuildItem * m_pBuildItemList[DEF_MAXBUILDITEMS]; // 100 recipes

    // Inventory state
    char  m_cItemOrder[DEF_MAXITEMS];            // Slot ordering
    short m_sItemEquipmentStatus[DEF_MAXITEMEQUIPPOS]; // Equipment status
    BOOL  m_bIsItemEquipped[DEF_MAXITEMS];       // Per-slot equipped flag
    BOOL  m_bIsItemDisabled[DEF_MAXITEMS];       // Per-slot disabled flag

    // Drag state
    int   m_iPointCommandCount;                   // Command counter
    BOOL  m_bIsItemDisable;                       // Item interaction disabled
};
```

### Slot System

Inventory uses a 0-indexed array with 50 slots:
- Slots 0-49: Player inventory
- Each slot is a `CItem*` pointer (NULL if empty)
- Items track their visual position via `m_sX`, `m_sY` (dialog coordinates)

---

## Core Functions

### Initialization

**InitItemList(char* pData)** - `Game.cpp:17689`
```cpp
// Parses server packet to populate inventory
// Format: [item_count][item_data × count][bank_count][bank_data × count]
// Also initializes magic and skill mastery arrays
void CGame::InitItemList(char* pData);
```

### Rendering

**DrawDialogBox_Inventory(int msX, int msY)** - `Game.cpp:21005`
```cpp
// Renders the inventory dialog
// - Draws 50 item slots in grid layout
// - Applies color tinting based on m_cItemColor
// - Shows stack counts for stackable items
// - Highlights equipped items
// - Displays item stats on hover
void CGame::DrawDialogBox_Inventory(int msX, int msY);
```

### Input Handling

**DlgBoxClick_Inventory(short msX, short msY)** - `Game.cpp:22644`
```cpp
// Handles clicks on inventory dialog
// - Item selection
// - Double-click to use/equip
// - Right-click context menu
// - Drag initiation
void CGame::DlgBoxClick_Inventory(short msX, short msY);
```

### Drag & Drop

**bItemDrop_Inventory(short msX, short msY)** - `Game.cpp:41970`
```cpp
// Handles dropping dragged item in inventory
// - Validates drop position
// - Swaps items if slot occupied
// - Updates m_sX, m_sY positions
// - Sends MSGID_REQUEST_SETITEMPOS to server
BOOL CGame::bItemDrop_Inventory(short msX, short msY);
```

Additional drop handlers:
- `bItemDrop_Bank()` - Drop in bank dialog
- `bItemDrop_SellList()` - Drop in sell dialog
- `bItemDrop_ExchangeDialog()` - Drop in trade dialog
- `bItemDrop_ItemUpgrade()` - Drop in upgrade dialog
- `bItemDrop_Character()` - Drop on character (equip)
- `bItemDrop_IconPannel()` - Drop in quickslot

### Item Manipulation

**EraseItem(char cItemID)** - `Game.cpp:31988`
```cpp
// Removes item from inventory slot
// - Deletes CItem object
// - Sets slot to NULL
// - Updates equipment status if needed
void CGame::EraseItem(char cItemID);
```

**SetItemCount(char* pItemName, DWORD dwCount)** - `Game.cpp:5783`
```cpp
// Updates stack count for named item
// - Finds item by name
// - Sets m_dwCount
// - Triggers UI refresh
void CGame::SetItemCount(char* pItemName, DWORD dwCount);
```

---

## Server Communication

### Item Packet Format

When receiving inventory data from server (`InitItemList`):

| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | Item count (0-50) |
| 1 | 20 | Item name (char[20]) |
| 21 | 4 | Stack count (DWORD) |
| 25 | 1 | Item type |
| 26 | 1 | Equipment position |
| 27 | 1 | Equipped status (0/1) |
| 28 | 2 | Level limit (short) |
| 30 | 1 | Gender limit |
| 31 | 2 | Current lifespan (WORD) |
| 33 | 2 | Weight (WORD) |
| 35 | 2 | Sprite ID (short) |
| 37 | 2 | Sprite frame (short) |
| 39 | 1 | Color code |
| 40 | 1 | Special effect value 2 |
| 41 | 4 | Attribute (DWORD) |
| ... | ... | Repeat for each item |

### Notification Messages

Server-to-client item notifications:

| Message | Purpose |
|---------|---------|
| `NotifyMsg_SetItemCount` | Update item stack count |
| `NotifyMsg_ItemColorChange` | Change item color/rarity |
| `NotifyMsg_ItemObtained` | New item acquired |
| `NotifyMsg_DropItemFin_EraseItem` | Item dropped on ground |
| `NotifyMsg_GiveItemFin_EraseItem` | Item given to player |
| `NotifyMsg_ItemDepleted_EraseItem` | Consumable used up |
| `NotifyMsg_ItemReleased` | Equipment unequipped |
| `NotifyMsg_ItemLifeSpanEnd` | Item durability expired |

### Request Messages

Client-to-server item requests:

| Message ID | Purpose |
|------------|---------|
| `MSGID_REQUEST_SETITEMPOS` | Move item in inventory |
| `MSGID_REQUEST_DROPITEM` | Drop item on ground |
| `MSGID_REQUEST_USEITEM` | Use consumable/skill item |
| `MSGID_REQUEST_EQUIPITEM` | Equip item to slot |
| `MSGID_REQUEST_SELLITEM` | Sell item to shop |
| `MSGID_REQUEST_EXCHANGEITEM` | Trade with player |

---

## Integration Points

### With Equipment System
- Items with `m_cEquipPos > 0` can be equipped
- `m_sItemEquipmentStatus[]` tracks what's in each slot
- `m_bIsItemEquipped[]` flags equipped inventory items

### With Combat System
- `m_sItemEffectValue1-6` provide combat bonuses
- `m_cSpeed` affects attack timing
- `m_dwAttribute` effects applied during combat calculations

### With Crafting System
- `CBuildItem` references items by name
- `m_cElementName[]` matches `CItem::m_cName`
- Crafting consumes inventory items

### With UI System
- Inventory dialog reads `m_pItemList[]`
- `m_sX`, `m_sY` control visual positioning
- Bank dialog reads `m_pBankList[]`
- Shop dialog reads `m_pItemForSaleList[]`

### With Network System
- All item changes synchronized via packets
- Server authoritative for item state
- Client sends requests, server confirms

---

## State Management

### Item Lifecycle

1. **Creation**: Server sends item data, client creates `CItem` via `new`
2. **Storage**: Pointer stored in `m_pItemList[slot]`
3. **Display**: Rendered based on `m_sSprite`, `m_sSpriteFrame`, `m_cItemColor`
4. **Interaction**: User clicks/drags, client sends request
5. **Modification**: Server confirms, client updates state
6. **Deletion**: `EraseItem()` calls `delete` and NULLs pointer

### Memory Management

```cpp
// Allocation (in packet handlers)
m_pItemList[slot] = new CItem;

// Deallocation (in EraseItem)
delete m_pItemList[cItemID];
m_pItemList[cItemID] = NULL;
```

No smart pointers - raw `new`/`delete` with manual NULL checks.

---

## Known Issues / Technical Debt

### Memory Safety
- Raw pointer arrays with manual lifetime management
- No bounds checking on slot indices
- Potential memory leaks if packets handled incorrectly

### Magic Numbers
- Item packet structure hardcoded with byte offsets
- Effect type values embedded throughout code
- Color codes not enumerated

### Coupling
- Item rendering logic embedded in dialog code
- Network packet parsing mixed with game logic
- No separation between item model and view

### Limitations
- Fixed 50-slot inventory (compile-time constant)
- Fixed 121-slot bank (compile-time constant)
- No dynamic resizing possible
- Color limited to 16 values (4-bit)

### Data Redundancy
- `m_cName` duplicated in `CItem` and `CItemName`
- Equipment status tracked in multiple arrays
- Position stored both logically and visually

---

## Modernization Notes

### Recommended C++20 Improvements

1. **Smart Pointers**
   ```cpp
   std::array<std::unique_ptr<Item>, 50> inventory_;
   ```

2. **Strong Typing**
   ```cpp
   enum class ItemType : uint8_t { None, Equip, Apply, ... };
   enum class EquipSlot : uint8_t { None, Head, Body, ... };
   enum class ItemColor : uint8_t { White, Red, Blue, ... };
   ```

3. **Attribute Unpacking**
   ```cpp
   struct ItemAttribute {
       static ItemAttribute from_packed(uint32_t attr);
       EffectType effect1;
       uint8_t effect_value1;
       EffectType effect2;
       uint8_t effect_value2;
       bool is_bound;
   };
   ```

4. **Separation of Concerns**
   - `Item` class for data model
   - `InventoryManager` for storage/manipulation
   - `InventoryRenderer` for UI display
   - `ItemSerializer` for network packets

5. **Container Safety**
   ```cpp
   std::optional<Item&> get_item(size_t slot);
   std::expected<void, Error> move_item(size_t from, size_t to);
   ```

6. **Event-Driven Updates**
   - Publish item changes via EventBus
   - UI subscribes to inventory events
   - Decouples storage from display

### Protocol Compatibility

The modernized system must maintain exact binary compatibility with the legacy packet format for server communication. The attribute encoding must be preserved exactly as documented.
