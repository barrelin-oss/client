# Equipment System

## Overview

The equipment system manages wearable items that modify character stats, appearance, and abilities. The legacy implementation supports 15 equipment slots with various restriction types including gender, level, weight/strength, and durability requirements. Equipment changes are reflected visually on the character sprite and communicated to the server via network messages.

## Source Files

- `Item.h` - CItem class definition, equipment slot constants, item type definitions
- `Item.cpp` - Item initialization and helper functions
- `Game.h` - Equipment arrays, appearance tracking, equipment status
- `Game.cpp` - Equipment handlers (~lines 48396-48530), rendering, validation
- `CharInfo.h` - Character appearance storage (m_sAppr1-4)
- `SpriteID.h` - Equipment sprite ID constants

## Key Data Structures

### Equipment Slot Constants

```cpp
#define DEF_MAXITEMEQUIPPOS     15

#define DEF_EQUIPPOS_NONE       0   // Not equippable
#define DEF_EQUIPPOS_HEAD       1   // Helmet/Hat
#define DEF_EQUIPPOS_BODY       2   // Chest armor
#define DEF_EQUIPPOS_ARMS       3   // Gauntlets/Gloves
#define DEF_EQUIPPOS_PANTS      4   // Leg armor
#define DEF_EQUIPPOS_BOOTS      5   // Footwear
#define DEF_EQUIPPOS_NECK       6   // Amulet/Necklace
#define DEF_EQUIPPOS_LHAND      7   // Left hand (Shield/Off-hand)
#define DEF_EQUIPPOS_RHAND      8   // Right hand (Main weapon)
#define DEF_EQUIPPOS_TWOHAND    9   // Two-handed weapon
#define DEF_EQUIPPOS_RFINGER    10  // Right finger ring
#define DEF_EQUIPPOS_LFINGER    11  // Left finger ring
#define DEF_EQUIPPOS_BACK       12  // Cape/Mantle
#define DEF_EQUIPPOS_FULLBODY   13  // Full body armor (mutually exclusive with individual pieces)
```

### CItem Class (Equipment-Related Fields)

```cpp
class CItem {
public:
    char  m_cName[21];              // Item name (20 chars + null)
    char  m_cItemType;              // Item type (DEF_ITEMTYPE_EQUIP = 1 for equipment)
    char  m_cEquipPos;              // Equipment slot (0-13)
    char  m_cItemColor;             // Color variation index
    char  m_cSpeed;                 // Attack speed modifier (weapons)
    char  m_cGenderLimit;           // Gender restriction: 0=any, 1=male, 2=female

    short m_sLevelLimit;            // Minimum level requirement
    short m_sSprite;                // Inventory icon sprite ID
    short m_sSpriteFrame;           // Inventory icon frame index
    short m_sX, m_sY;               // Position in inventory grid

    // Special effect values (elemental, resistances, etc.)
    short m_sItemSpecEffectValue1;
    short m_sItemSpecEffectValue2;
    short m_sItemSpecEffectValue3;

    // Standard effect values (damage, defense, bonuses)
    short m_sItemEffectValue1;      // Min damage (weapons) or defense (armor)
    short m_sItemEffectValue2;      // Max damage (weapons)
    short m_sItemEffectValue3;      // Additional effect
    short m_sItemEffectValue4;      // Additional effect
    short m_sItemEffectValue5;      // Additional effect
    short m_sItemEffectValue6;      // Additional effect

    WORD  m_wCurLifeSpan;           // Current durability
    WORD  m_wMaxLifeSpan;           // Maximum durability
    WORD  m_wPrice;                 // Base sale price
    WORD  m_wWeight;                // Weight (in units of 0.01)

    DWORD m_dwCount;                // Stack count (usually 1 for equipment)
    DWORD m_dwAttribute;            // Encoded attribute flags (see below)
};
```

### Equipment Status Arrays in CGame

```cpp
class CGame {
    // Inventory storage
    CItem * m_pItemList[DEF_MAXITEMS];       // 50 inventory slots
    CItem * m_pBankList[DEF_MAXBANKITEMS];   // 121 bank slots

    // Equipment tracking
    CInt m_bIsItemEquipped[DEF_MAXITEMS];    // TRUE if item in slot is equipped
    CInt m_bIsItemDisabled[DEF_MAXITEMS];    // TRUE if item is disabled
    short m_sItemEquipmentStatus[15];        // Maps slot -> inventory index (-1 if empty)

    // Character appearance (reflects equipped items)
    short m_sAppr1;                          // Upper body appearance
    short m_sAppr2;                          // Lower body appearance
    short m_sAppr3;                          // Special equipment
    short m_sAppr4;                          // Weapon/shield appearance
};
```

### Attribute Encoding (m_dwAttribute)

The 32-bit attribute field encodes special properties and bonuses:

```
Bit Layout:
  Bit 0:        Custom/upgraded item flag (1 = custom item, bypasses level check)
  Bits 8-11:    Effect value 2
  Bits 12-15:   Effect type 2
  Bits 16-19:   Effect value 1
  Bits 20-23:   Effect type 1
  Bits 28-31:   Damage/stat bonus (displayed as "+N" in item name)
```

**Attribute Unpacking:**
```cpp
dwType1  = (dwAttribute & 0x00F00000) >> 20;   // Effect type 1
dwValue1 = (dwAttribute & 0x000F0000) >> 16;   // Effect value 1
dwType2  = (dwAttribute & 0x0000F000) >> 12;   // Effect type 2
dwValue2 = (dwAttribute & 0x00000F00) >> 8;    // Effect value 2
dwBonus  = (dwAttribute & 0xF0000000) >> 28;   // +N bonus
```

**Effect Type Mappings:**
| Type | Effect | Multiplier |
|------|--------|------------|
| 0 | None | - |
| 1 | Super Attack Bonus | +N damage |
| 2 | Experience Bonus | +N% |
| 3 | Gold Bonus | +N% |
| 4 | HP Recovery | +N per tick |
| 5 | MP Recovery | +N per tick |
| 6 | Fire Resistance | +N * 4% |
| 7 | Water Resistance | +N * 5% |
| 8 | Ice Resistance | +N * 7% |
| 9 | Earth Resistance | +N * 3% |
| 10 | Spell Accuracy | +N * 3% |
| 11 | Poison Resistance | +N * 10% |
| 12 | Critical Bonus | +N% |
| 13 | Physical Absorption | N% |
| 14 | Magic Absorption | N% |
| 15 | Skill Bonus | +N to skill |

## Core Functions

### ItemEquipHandler

```cpp
void CGame::ItemEquipHandler(char cItemID)
```

Handles equipping an item from inventory. Located around Game.cpp:48407-48530.

**Validation Sequence:**
1. Check item is not already equipped (`m_bIsItemEquipped[cItemID] == FALSE`)
2. Check item has valid equipment slot (`m_cEquipPos != DEF_EQUIPPOS_NONE`)
3. Check durability (`m_wCurLifeSpan > 0`)
4. Check weight vs STR (`m_wWeight / 100 <= m_iStr`)
5. Check level requirement (`m_sLevelLimit <= m_iLevel` or custom item)
6. Check not casting skill (`m_bSkillUsingStatus == FALSE`)
7. Check gender restriction matches player

**Mutual Exclusion Logic:**
```cpp
// Two-hand weapon conflicts
if (equipPos == DEF_EQUIPPOS_TWOHAND) {
    // Auto-unequip left hand and right hand items
    if (m_sItemEquipmentStatus[DEF_EQUIPPOS_LHAND] >= 0)
        ReleaseEquipHandler(DEF_EQUIPPOS_LHAND);
    if (m_sItemEquipmentStatus[DEF_EQUIPPOS_RHAND] >= 0)
        ReleaseEquipHandler(DEF_EQUIPPOS_RHAND);
}

// One-hand weapon conflicts with two-hand
if (equipPos == DEF_EQUIPPOS_RHAND || equipPos == DEF_EQUIPPOS_LHAND) {
    if (m_sItemEquipmentStatus[DEF_EQUIPPOS_TWOHAND] >= 0)
        ReleaseEquipHandler(DEF_EQUIPPOS_TWOHAND);
}

// Full body armor conflicts
if (equipPos == DEF_EQUIPPOS_FULLBODY) {
    // Auto-unequip: head, body, arms, pants, boots, back
    for (int slot : {HEAD, BODY, ARMS, PANTS, BOOTS, BACK}) {
        if (m_sItemEquipmentStatus[slot] >= 0)
            ReleaseEquipHandler(slot);
    }
}

// Individual armor conflicts with full body
if (equipPos in {HEAD, BODY, ARMS, PANTS, BOOTS, BACK}) {
    if (m_sItemEquipmentStatus[DEF_EQUIPPOS_FULLBODY] >= 0)
        ReleaseEquipHandler(DEF_EQUIPPOS_FULLBODY);
}
```

**Network Message:**
```cpp
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_EQUIPITEM,
             NULL, cItemID, NULL, NULL, NULL);
```

### ReleaseEquipHandler

```cpp
void CGame::ReleaseEquipHandler(char cEquipPos)
```

Unequips an item from the specified equipment slot. Located around Game.cpp:48396-48405.

```cpp
void CGame::ReleaseEquipHandler(char cEquipPos)
{
    if (m_sItemEquipmentStatus[cEquipPos] < 0) return;  // Slot already empty

    char cItemID = m_sItemEquipmentStatus[cEquipPos];
    m_bIsItemEquipped[cItemID] = FALSE;
    m_sItemEquipmentStatus[cEquipPos] = -1;

    AddEventList("Equipment released: [item name]", 10);
}
```

### Equipment Validation Helpers

```cpp
// Check if item can be equipped
BOOL CGame::bCheckItemEquipRequirements(short sItemID)
{
    CItem* pItem = m_pItemList[sItemID];
    if (pItem == NULL) return FALSE;

    // Type check
    if (pItem->m_cItemType != DEF_ITEMTYPE_EQUIP) return FALSE;

    // Slot check
    if (pItem->m_cEquipPos == DEF_EQUIPPOS_NONE) return FALSE;

    // Durability check
    if (pItem->m_wCurLifeSpan == 0) return FALSE;

    // Weight/STR check
    if ((pItem->m_wWeight / 100) > m_iStr) return FALSE;

    // Level check (bypass for custom items)
    if (((pItem->m_dwAttribute & 0x00000001) == 0) &&
        (pItem->m_sLevelLimit > m_iLevel)) return FALSE;

    // Gender check
    if (pItem->m_cGenderLimit != 0) {
        // Male characters: type 1, 2, 3
        // Female characters: type 4, 5, 6
        BOOL bIsMale = (m_sPlayerType <= 3);
        if (pItem->m_cGenderLimit == 1 && !bIsMale) return FALSE;
        if (pItem->m_cGenderLimit == 2 && bIsMale) return FALSE;
    }

    return TRUE;
}
```

## Constants & Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_MAXITEMS` | 50 | Inventory capacity |
| `DEF_MAXBANKITEMS` | 121 | Bank storage capacity |
| `DEF_MAXITEMEQUIPPOS` | 15 | Number of equipment slots |
| `DEF_ITEMTYPE_EQUIP` | 1 | Item type for equippable items |
| `DEF_SPRID_ITEMEQUIP_PIVOTPOINT` | 200 | Base sprite ID for equipment visuals |

**Weight System:**
- Weight stored in units of 0.01 (centiunits)
- Actual weight = `m_wWeight / 100`
- Character must have `STR >= weight` to equip

**Gender Values:**
| Value | Restriction |
|-------|-------------|
| 0 | Any gender |
| 1 | Male only (player types 1-3) |
| 2 | Female only (player types 4-6) |

## Integration Points

### With Inventory System
- Equipment slots map to inventory indices via `m_sItemEquipmentStatus[]`
- `m_bIsItemEquipped[]` tracks which inventory items are currently worn
- Unequipping moves item back to available inventory slot

### With Character Stats
- Equipped items modify: HP, MP, SP, STR, DEX, INT, MAG, VIT, CHR
- Defense values from armor pieces
- Attack values from weapons
- Special effects (resistances, bonuses) from attribute encoding

### With Rendering System
- Appearance values (`m_sAppr1` through `m_sAppr4`) encode visual equipment
- Equipment sprites loaded from PAK files
- Male sprites: `DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0` to `+ 39`
- Female sprites: `DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 40` to `+ 79`

### With Network System
- Equipment changes sent via `MSGID_COMMAND_COMMON` with `DEF_COMMONTYPE_EQUIPITEM`
- Server validates and confirms equipment changes
- Appearance updates broadcast to nearby players

## State Management

### Equipment Status Array

```cpp
short m_sItemEquipmentStatus[15];
// Index = equipment slot (0-14, though 0 is NONE and 14 unused)
// Value = inventory index (0-49) or -1 if slot empty

// Example state:
m_sItemEquipmentStatus[DEF_EQUIPPOS_HEAD] = 5;   // Inventory slot 5 equipped as helmet
m_sItemEquipmentStatus[DEF_EQUIPPOS_BODY] = 12;  // Inventory slot 12 equipped as armor
m_sItemEquipmentStatus[DEF_EQUIPPOS_RHAND] = 3;  // Inventory slot 3 equipped as weapon
m_sItemEquipmentStatus[DEF_EQUIPPOS_LHAND] = -1; // No shield equipped
```

### Appearance Encoding

The `m_sAppr` values encode visual equipment for rendering:

```cpp
// m_sAppr1 - Upper body equipment
Upper nibble (bits 8-11): Color index for RGB tinting
Lower nibble (bits 0-3):  Equipment sprite variation

// m_sAppr4 - Weapon/Shield
Upper byte: Weapon color index
Lower byte: Weapon sprite index
```

**Rendering with appearance:**
```cpp
// Draw equipment with color tinting
m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + spriteIndex]
    ->PutSpriteRGB(sX, sY,
                   (m_sAppr1 & 0x0F00) >> 8,  // Color index
                   iR, iG, iB, dwTime);
```

## Visual Representation

### Equipment Dialog Slot Positions

The equipment dialog displays a character silhouette with clickable equipment slots:

```
Slot Layout (approximate pixel coordinates):

        [HEAD: 72,135]
            |
    [NECK: 35,120]--[BACK: 41,137]
            |
[LHAND: 90,170]--[BODY: 171,290]--[RHAND/2H: 57,186]
            |
        [ARMS: 171,290]
            |
        [PANTS: 171,290]
            |
        [BOOTS: 171,290]

[LFINGER: left side]    [RFINGER: 32,193]
```

### Collision Detection for Slot Clicks

```cpp
// Check if mouse click hits equipment slot
if (cEquipPosStatus[DEF_EQUIPPOS_HEAD] != -1) {
    sSprH = m_pItemList[cEquipPosStatus[DEF_EQUIPPOS_HEAD]]->m_sSprite;
    sFrame = m_pItemList[cEquipPosStatus[DEF_EQUIPPOS_HEAD]]->m_sSpriteFrame;

    if (m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH]
        ->_bCheckCollison(sX + 72, sY + 135, sFrame, msX, msY)) {
        // Head slot clicked - handle interaction
    }
}
```

## Known Issues / Technical Debt

1. **Hardcoded Slot Positions** - Equipment dialog slot coordinates are magic numbers scattered throughout Game.cpp
2. **No Slot Validation** - Equipment slot array index 0 (NONE) and 14 are unused but array sized for 15
3. **Gender Check Coupling** - Gender restriction tied to player type numbers (1-3 male, 4-6 female)
4. **Attribute Bit Packing** - Complex bit manipulation for attributes is error-prone and hard to debug
5. **No Equipment Sets** - No support for matching set bonuses
6. **Weight as Integer** - Weight stored as integer centiunits requires division
7. **Durability Display** - No visual indicator of durability in equipment slots
8. **Mixed Responsibility** - ItemEquipHandler does validation, state update, and network send

## Modernization Notes

### Recommended Changes

1. **Strong Typing** - Replace magic numbers with `enum class equip_slot`
2. **Equipment Component** - Separate equipment state from CGame into dedicated class
3. **Validation Service** - Extract equipment validation into testable service class
4. **Event System** - Use events for equipment changes instead of direct coupling
5. **Attribute Struct** - Replace bit-packed attribute with structured data
6. **Equipment Slots Map** - Use `std::array<std::optional<item>, 15>` for type safety

### Data-Driven Approach

```cpp
// Modern equipment slot definition
struct equipment_slot_def {
    equip_slot id;
    std::string_view name;
    vec2i dialog_position;
    std::vector<equip_slot> conflicts_with;
    bool is_armor_piece;
};

// Conflict rules as data, not code
const equipment_slot_def slots[] = {
    {equip_slot::two_hand, "Two-Handed", {57,186},
     {equip_slot::left_hand, equip_slot::right_hand}, false},
    {equip_slot::full_body, "Full Body", {171,290},
     {equip_slot::head, equip_slot::body, equip_slot::arms,
      equip_slot::pants, equip_slot::boots, equip_slot::back}, true},
    // ...
};
```

### Network Protocol Preservation

The equipment message format must remain unchanged for server compatibility:

```cpp
// Legacy format - must preserve
bSendCommand(MSGID_COMMAND_COMMON,    // 0x0FA314DC
             DEF_COMMONTYPE_EQUIPITEM, // 0x0A02
             NULL,
             cItemID,                  // Inventory slot (0-49)
             NULL, NULL, NULL);
```
