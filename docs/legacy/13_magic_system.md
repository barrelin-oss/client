# Magic System

## Overview

The Helbreath magic system provides 99 spells organized into 9 circles (tiers) of progression. Spells are categorized into 23 distinct magic types covering damage, healing, buffs, debuffs, teleportation, summoning, and utility effects. The system integrates deeply with the combat, targeting, and network subsystems.

## Source Files

| File | Purpose |
|------|---------|
| `Magic.h` | CMagic class definition |
| `Magic.cpp` | CMagic implementation |
| `Game.h` | Magic arrays, mastery tracking, casting state |
| `Game.cpp` | Spell casting logic, UI handling, network communication |
| `magiccfg.txt` | Spell definitions (99 spells) |
| `NetMessages.h` | Magic-related packet definitions |
| `Effect.h` | Spell visual effect definitions |

---

## Key Data Structures

### CMagic Class

```cpp
class CMagic {
public:
    CMagic();
    ~CMagic();

    char  m_cName[31];      // Spell name (max 30 characters + null)
    int   m_sValue1;        // Mana (MP) cost
    int   m_sValue2;        // Required Intelligence stat
    int   m_sValue3;        // Additional cost/effect modifier
    bool  m_bIsVisible;     // Whether spell appears in spellbook (0=hidden, 1=visible)
};
```

### CGame Magic Members

```cpp
// In Game.h - CGame class members

// Spell configuration array
CMagic * m_pMagicCfgList[DEF_MAXMAGICTYPE];  // DEF_MAXMAGICTYPE = 100

// Player mastery tracking
char m_cMagicMastery[100];   // Mastery level per spell (0=not learned, 1-255=mastery)

// Casting state
short m_sMagicShortCut;      // Quickbound spell ID (-1 = none)
int   m_iCastingMagicType;   // Currently casting spell ID
int   m_iPointCommandType;   // Target command type (magic ID + 100)

// Targeting mode
bool  m_bIsGetPointingMode;  // Active during ground-target spell selection

// Related stats
int   m_iMP;                 // Current mana points
int   m_iMaxMP;              // Maximum mana points
int   m_iInt;                // Intelligence stat (affects spell requirements)
```

---

## Magic Types (23 Total)

The magic type determines the spell's fundamental behavior and targeting rules.

| ID | Constant | Description |
|----|----------|-------------|
| 1 | `DEF_MAGICTYPE_DAMAGE_SPOT` | Single-target damage spell |
| 2 | `DEF_MAGICTYPE_HPUP_SPOT` | Single-target healing spell |
| 3 | `DEF_MAGICTYPE_DAMAGE_AREA` | Area-of-effect damage spell |
| 4 | `DEF_MAGICTYPE_SPDOWN_SPOT` | Single-target MP drain |
| 5 | `DEF_MAGICTYPE_SPDOWN_AREA` | Area-of-effect MP drain |
| 6 | `DEF_MAGICTYPE_SPUP_SPOT` | Single-target MP restoration |
| 7 | `DEF_MAGICTYPE_SPUP_AREA` | Area-of-effect MP restoration |
| 8 | `DEF_MAGICTYPE_TELEPORT` | Teleportation spell |
| 9 | `DEF_MAGICTYPE_SUMMON` | Summon creature |
| 10 | `DEF_MAGICTYPE_CREATE` | Create object or apply buff |
| 11 | `DEF_MAGICTYPE_PROTECT` | Protection/defense buff |
| 12 | `DEF_MAGICTYPE_HOLDOBJECT` | Paralyze/freeze target |
| 13 | `DEF_MAGICTYPE_INVISIBILITY` | Invisibility effect |
| 14 | `DEF_MAGICTYPE_CREATE_DYNAMIC` | Create dynamic world object |
| 15 | `DEF_MAGICTYPE_POSSESSION` | Mind control/charm |
| 16 | `DEF_MAGICTYPE_CONFUSE` | Confusion or debuff cure |
| 17 | `DEF_MAGICTYPE_POISON` | Damage-over-time poison |
| 18 | `DEF_MAGICTYPE_BERSERK` | Berserk combat buff |
| 20 | `DEF_MAGICTYPE_POLYMORPH` | Transform/shapeshift |
| 21 | `DEF_MAGICTYPE_DAMAGE_AREA_NOSPOT` | Area damage without spot marker |
| 22 | `DEF_MAGICTYPE_TREMOR` | Earthquake/tremor effect |
| 23 | `DEF_MAGICTYPE_ICE` | Ice/freeze attack |

**Note:** Type 19 is unused/skipped in the enumeration.

---

## Complete Spell List (99 Spells)

Spells are defined in `magiccfg.txt` with the format:
```
magic <ID> <NAME> <MANA_COST> <INT_REQ> <VALUE3> <UNUSED> <UNUSED> <UNUSED> <VISIBLE>
```

### Circle 1 - Apprentice Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 0 | Magic-Missile | 8 | 18 | Yes | Basic single-target damage |
| 1 | Heal | 15 | 20 | Yes | Basic single-target healing |
| 2 | Create-Food | 18 | 18 | Yes | Create consumable food |
| 3 | GM-Kill | 10 | 10 | **No** | Admin instant-kill command |

### Circle 2 - Novice Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 10 | Energy-Bolt | 15 | 24 | Yes | Energy damage projectile |
| 11 | Stamina-Drain | 14 | 22 | Yes | Drain target's stamina |
| 12 | Recall | 15 | 10 | Yes | Teleport to town |
| 13 | Defense-Shield | 19 | 26 | Yes | Defensive buff |
| 14 | Celebrating-Light | 20 | 25 | Yes | Light/visibility spell |

### Circle 3 - Journeyman Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 20 | Fire-Ball | 27 | 26 | Yes | Fire damage projectile |
| 21 | Great-Heal | 28 | 28 | Yes | Improved healing |
| 23 | Stamina-Recovery | 20 | 20 | Yes | Restore stamina |
| 24 | Protection-From-Arrow | 22 | 20 | Yes | Arrow defense buff |
| 25 | Hold-Person | 24 | 26 | Yes | Paralyze humanoid target |
| 26 | Possession | 25 | 26 | Yes | Mind control |
| 27 | Poison | 28 | 29 | Yes | Apply poison DoT |
| 28 | Great-Stamina-Recovery | 45 | 30 | Yes | Major stamina restore |

### Circle 4 - Adept Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 30 | Fire-Strike | 36 | 34 | Yes | Fire damage strike |
| 31 | Summon-Creature | 35 | 38 | Yes | Summon allied creature |
| 32 | Invisibility | 31 | 30 | Yes | Become invisible |
| 33 | Protection-From-Magic | 35 | 32 | Yes | Magic resistance buff |
| 34 | Detect-Invisibility | 33 | 30 | Yes | See invisible targets |
| 35 | Paralyze | 35 | 36 | Yes | Freeze target in place |
| 36 | Cure | 32 | 35 | Yes | Remove poison/debuffs |
| 37 | Lightning-Arrow | 32 | 38 | Yes | Lightning projectile |
| 38 | Tremor | 34 | 33 | Yes | Ground-shaking attack |

### Circle 5 - Expert Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 40 | Fire-Wall | 42 | 45 | Yes | Create wall of fire |
| 41 | Fire-Field | 48 | 48 | Yes | Area fire damage zone |
| 42 | Confuse-Language | 40 | 42 | Yes | Confusion debuff |
| 43 | Lightning | 44 | 47 | Yes | Lightning damage |
| 44 | Great-Defense-Shield | 45 | 46 | Yes | Superior defense buff |
| 45 | Chill-Wind | 48 | 50 | Yes | Ice wind damage |
| 46 | Poison-Cloud | 48 | 49 | Yes | Area poison (radius 4) |
| 47 | Triple-Energy-Bolt | 40 | 45 | Yes | Multi-hit energy attack |

### Circle 6 - Master Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 50 | Berserk | 57 | 59 | Yes | Combat damage buff |
| 51 | Lightning-Bolt | 58 | 58 | Yes | Powerful lightning |
| 53 | Mass-Poison | 54 | 52 | Yes | Area poison spread |
| 54 | Spike-Field | 56 | 56 | Yes | Ground spike hazard |
| 55 | Ice-Storm | 58 | 159 | **No** | Restricted ice storm |
| 56 | Mass-Lightning-Arrow | 55 | 53 | Yes | Multi-target lightning |
| 57 | Ice-Strike | 59 | 60 | Yes | Ice damage strike |

### Circle 7 - Grandmaster Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 60 | Energy-Strike | 65 | 67 | Yes | Energy damage strike |
| 61 | Mass-Fire-Strike | 80 | 85 | Yes | Multi-target fire |
| 62 | Confusion | 78 | 75 | Yes | Mass confusion |
| 63 | Mass-Chill-Wind | 90 | 93 | Yes | Area ice wind |
| 64 | Earthworm-Strike | 80 | 97 | Yes | Ground-based attack |
| 65 | Absolute-Magic-Protect | 250 | 300 | Yes | Ultimate magic defense |
| 66 | Armor-Break | 290 | 350 | Yes | Destroy target armor |
| 67 | Scan | 50 | 150 | **No** | Detection spell |
| 68 | Greater-Berserk | 103 | 93 | **No** | Enhanced berserk |

### Circle 8 - Archmage Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 70 | Bloody-Shock-Wave | 120 | 250 | **No** | PvP shock wave |
| 71 | Mass-Confusion | 125 | 130 | Yes | Area confusion |
| 72 | Mass-Ice-Strike | 120 | 133 | Yes | Multi-target ice |
| 73 | Cloud-Kill | 130 | 120 | Yes | Deadly poison cloud |
| 74 | Lightning-Strike | 60 | 123 | Yes | Lightning strike |
| 76 | Cancellation | 120 | 450 | Yes | Remove all buffs |
| 77 | Illusion-Movement | 120 | 350 | Yes | Teleport illusion |
| 78 | Ancient-Berserk | 255 | 450 | **No** | Ultimate berserk |

### Circle 9 - Legendary Spells

| ID | Name | MP Cost | INT Req | Visible | Description |
|----|------|---------|---------|---------|-------------|
| 80 | Illusion | 143 | 150 | Yes | Create illusion |
| 81 | Meteor-Strike | 60 | 200 | Yes | Meteor impact |
| 82 | Mass-Magic-Missile | 160 | 250 | Yes | Multi-target missiles |
| 83 | Inhibition-Casting | 180 | 450 | **No** | Silence target |
| 84 | Magic-Shield | 220 | 400 | **No** | Magic absorption |
| 85 | Greatest-Heal | 250 | 350 | **No** | Ultimate healing |
| 87 | Strike-Of-The-Ghosts | 500 | 525 | **No** | Ghost damage |
| 88 | Lightning-Clash | 525 | 550 | **No** | Ultimate lightning |
| 90 | Mass-Illusion | 200 | 280 | Yes | Area illusion |
| 91 | Blizzard | 220 | 200 | Yes | Ice storm |
| 92 | Call-Of-The-Gods | 400 | 500 | **No** | Guild war spell |
| 93 | Wind-Blast | 240 | 400 | Yes | Wind knockback |
| 94 | Resurrection | 200 | 0 | **No** | Revive dead player |
| 95 | Mass-Illusion-Movement | 200 | 450 | Yes | Group teleport illusion |
| 96 | Earth-Shock-Wave | 240 | 350 | Yes | Earthquake wave |
| 97 | Fiery-Shock-Wave | 280 | 450 | **No** | Fire shock wave |
| 98 | Mass-Blizzard | 320 | 400 | **No** | Ultimate blizzard |

---

## Core Functions

### Spell Configuration Loading

```cpp
// Game.cpp
bool CGame::bInitMagicCfgList()
```

Loads `magiccfg.txt` and populates `m_pMagicCfgList[]` array.

**Parsing State Machine:**
- `cReadModeA = 1` - Inside magic section
- `cReadModeB = 1-9` - Field sequence:
  1. Magic ID (0-99)
  2. Magic Name (max 30 chars)
  3. m_sValue1 (Mana cost)
  4. m_sValue2 (Required INT)
  5. m_sValue3 (Additional value)
  6-8. Unused fields
  9. m_bIsVisible (0 or 1)

### Mana Cost Calculation

```cpp
// Game.cpp
int CGame::iGetManaCost(int iMagicNo)
```

Calculates actual MP cost for a spell:
- Base cost from `m_pMagicCfgList[iMagicNo]->m_sValue1`
- Reduced by mastery level
- Minimum cost is 1 MP

### Spell Casting

```cpp
// Game.cpp
void CGame::UseMagic(int iMagicNo)
```

**Pre-cast Validation:**
1. `iMagicNo` within range 0-99
2. Player has learned spell: `m_cMagicMastery[iMagicNo] != 0`
3. Player is alive: `m_iHP > 0`
4. Not already in targeting mode: `!m_bIsGetPointingMode`
5. Sufficient mana: `iGetManaCost(iMagicNo) <= m_iMP`
6. No item equipped in hand: `!_bIsItemOnHand()`
7. Not using a skill: `!m_bSkillUsingStatus`

**Cast Initiation:**
```cpp
// Toggle combat mode if needed
if (m_bIsCombatMode == false) {
    bSendCommand(CYCLESTART_COMBAT, NULL, NULL, NULL, NULL, NULL, NULL);
}

// Set casting state
m_cCommand = DEF_OBJECTMAGIC;        // 4
m_iCastingMagicType = iMagicNo;
m_iPointCommandType = iMagicNo + 100;

// Enter targeting mode for ground-target spells
m_bIsGetPointingMode = true;
```

### Target Selection

```cpp
// Game.cpp - Mouse click handler
void CGame::OnLButtonDown_PointMode(int x, int y)
```

When player clicks to select a target:
1. Convert screen coordinates to world tile coordinates
2. Validate target is within spell range
3. Send cast command to server

---

## Network Protocol

### Outgoing Messages (Client → Server)

**Cast Spell Motion:**
```cpp
// MSGID_COMMAND_MOTION (0x0FA314D5)
bSendCommand(MSGID_COMMAND_MOTION,
             DEF_OBJECTMAGIC,      // Command type = 4
             m_cPlayerDir,         // Facing direction
             m_iCastingMagicType,  // Spell ID
             targetX, targetY,     // Target coordinates
             ...);
```

**Cast Spell Common:**
```cpp
// MSGID_COMMAND_COMMON (0x0FA314DC)
// Sub-type: DEF_COMMONTYPE_MAGIC (0x0A0D)
bSendCommand(MSGID_COMMAND_COMMON,
             DEF_COMMONTYPE_MAGIC,
             targetX, targetY,
             m_iPointCommandType,  // Spell ID + 100
             ...);
```

**Learn Spell Request:**
```cpp
// MSGID_COMMAND_COMMON (0x0FA314DC)
// Sub-type: DEF_COMMONTYPE_REQ_STUDYMAGIC (0x0A0E)
bSendCommand(MSGID_COMMAND_COMMON,
             DEF_COMMONTYPE_REQ_STUDYMAGIC,
             spellId,
             ...);
```

### Incoming Messages (Server → Client)

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_NOTIFY_MAGICSTUDYSUCCESS` | 0x0B10 | Spell successfully learned |
| `DEF_NOTIFY_MAGICSTUDYFAIL` | 0x0B11 | Spell learning failed |
| `DEF_NOTIFY_MAGICEFFECTON` | 0x0B27 | Spell effect applied to entity |
| `DEF_NOTIFY_MAGICEFFECTOFF` | 0x0B28 | Spell effect removed from entity |
| `DEF_NOTIFY_GRANDMAGICRESULT` | 0x0B9D | Area spell damage results |

**Grand Magic Result Packet Format:**
```
WORD    casterEntityId
WORD    magicId
WORD    damageValue
char[10] mapName
WORD    targetEntityId
WORD[]  affectedHPValues  // Variable-length array
```

---

## Magic Mastery System

### Mastery Array

```cpp
char m_cMagicMastery[100];  // Index = spell ID
```

| Value | Meaning |
|-------|---------|
| 0 | Spell not learned |
| 1-255 | Mastery level |

### Learning Spells

Players learn spells through:
1. Magic Circle NPCs (trainers)
2. Quest rewards
3. Special events

**Network Flow:**
1. Client sends `DEF_COMMONTYPE_REQ_STUDYMAGIC` with spell ID
2. Server validates requirements (INT, gold, prerequisites)
3. Server responds with `DEF_NOTIFY_MAGICSTUDYSUCCESS` or `DEF_NOTIFY_MAGICSTUDYFAIL`
4. On success, client updates `m_cMagicMastery[spellId] = 1`

### Mastery Benefits

Higher mastery levels provide:
- Reduced mana cost
- Increased spell effectiveness (damage/healing)
- Faster cast times (some spells)

Mastery increases through repeated use of the spell.

---

## UI Integration

### Spellbook Dialog (F7 Key)

```cpp
// Dialog index
m_bIsDialogEnabled[3]  // Magic circle dialog

// Layout constants
const int SPELLS_PER_ROW = 5;
const int SLOT_WIDTH = 40;
const int SLOT_HEIGHT = 40;
const int SLOT_PADDING = 4;
```

**Features:**
- Organized by circle (1-9 tabs)
- Grid display of learned spells
- Spell icons with mastery indicator
- Double-click to cast (enters targeting mode)
- Right-click to set hotkey

### Quick Cast System

```cpp
short m_sMagicShortCut;  // Currently bound spell (-1 = none)
```

**Hotkey Binding:**
- F4 key triggers quick cast of `m_sMagicShortCut`
- Stored in Windows Registry: `HKEY_CURRENT_USER\Software\Siementech\Helbreath\Magic`
- Value encoding: `spell_id + 1` (0 = no spell bound)

### Targeting Cursor

When `m_bIsGetPointingMode == true`:
- Cursor changes to targeting reticle
- Left-click selects target location
- Right-click cancels casting
- ESC key cancels casting

---

## Spell Visual Effects

Spell effects are rendered through the Effect system. Common effect mappings:

| Spell Type | Effect ID | Description |
|------------|-----------|-------------|
| Magic-Missile | 25 | Explosion on impact |
| Meteor-Strike | 60 | Large impact explosion |
| Fire spells | Various | Fire/flame animations |
| Ice spells | Various | Ice crystal animations |
| Lightning | Various | Electrical discharge |
| Poison | Various | Green cloud particles |
| Healing | Various | Golden glow |

Effect rendering is handled by `DrawEffects()` in Game.cpp using the Effect subsystem.

---

## Constants & Limits

### Array Sizes

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEF_MAXMAGICTYPE` | 100 | Maximum spell definitions |
| Spell ID range | 0-99 | Valid spell ID range |
| Mastery range | 0-255 | Mastery level range |
| Spell name length | 30 | Maximum characters |

### Spell Ranges

| Statistic | Min | Max |
|-----------|-----|-----|
| MP Cost | 8 | 525 |
| INT Requirement | 0 | 550 |
| Circles | 1 | 9 |
| Hidden spells | - | 16 |

### Hidden Spells (Visible = 0)

These spells don't appear in the normal spellbook:
- GM-Kill (3) - Admin command
- Ice-Storm (55) - Restricted
- Scan (67) - Detection
- Greater-Berserk (68) - Boss/special
- Bloody-Shock-Wave (70) - PvP restricted
- Ancient-Berserk (78) - Special
- Inhibition-Casting (83) - Silence
- Magic-Shield (84) - Restricted
- Greatest-Heal (85) - High-level
- Strike-Of-The-Ghosts (87) - Restricted
- Lightning-Clash (88) - Restricted
- Call-Of-The-Gods (92) - Guild war
- Resurrection (94) - Revival
- Fiery-Shock-Wave (97) - Restricted
- Mass-Blizzard (98) - Restricted

---

## Integration Points

### Combat System
- Spell damage integrates with combat calculations
- Magic attacks trigger hit detection
- Spell buffs modify combat stats

### Character System
- INT stat determines learnable spells
- MP pool limits casting frequency
- Mastery tracked per character

### Network System
- All spell casts validated server-side
- Results broadcast to nearby clients
- Spell effects synchronized

### Effect System
- Visual effects spawned for each spell
- Effect duration matches spell duration
- Particle systems for area spells

### Input System
- F7 opens spellbook
- F4 quick-casts bound spell
- Mouse targeting for ground spells
- Hotkey bindings stored in registry

---

## State Management

### Casting State Machine

```
IDLE
  │
  ├─(UseMagic)──► TARGETING (m_bIsGetPointingMode = true)
  │                    │
  │                    ├─(Click target)──► CASTING
  │                    │                       │
  │                    ├─(Cancel)──► IDLE     │
  │                    │                       │
  │                    └─(ESC)──► IDLE        │
  │                                           │
  │◄───────────(Spell complete)───────────────┘
```

### State Variables

```cpp
bool m_bIsGetPointingMode;   // In targeting mode
int  m_iCastingMagicType;    // Spell being cast
int  m_iPointCommandType;    // Command type (spell ID + 100)
char m_cCommand;             // DEF_OBJECTMAGIC during cast
```

---

## Known Issues / Technical Debt

1. **Hardcoded spell IDs** - Spell behavior often checked by magic number instead of type
2. **Monolithic spell handling** - All 99 spells handled in same function with giant switch
3. **No spell inheritance** - Similar spells duplicate code
4. **Registry dependency** - Hotkey storage uses Windows Registry
5. **Magic numbers** - `+100` offset for point command type is unexplained
6. **Limited mastery feedback** - No visual indicator of mastery progression
7. **Fixed spell slots** - Array size of 100 limits expansion
8. **Tightly coupled** - Spell logic intertwined with rendering, networking, UI

---

## Modernization Notes

### Recommended Changes

1. **Data-driven spells** - Move all spell data to JSON/YAML configuration
2. **Spell class hierarchy** - Base spell class with type-specific subclasses
3. **Event-based casting** - Decouple cast/result through event bus
4. **Type-safe IDs** - Use strongly-typed spell ID instead of raw int
5. **Separate mastery system** - Extract to dedicated progression module
6. **Modern targeting** - Replace bool flag with proper state machine
7. **Cross-platform storage** - Replace registry with config file
8. **Spell effect system** - Proper component-based buff/debuff tracking

### C++20 Features to Apply

- `std::expected` for spell cast results
- Concepts for spell type constraints
- `std::span` for spell list views
- `std::format` for spell descriptions
- Coroutines for async cast sequences
- `constexpr` spell constant calculations
