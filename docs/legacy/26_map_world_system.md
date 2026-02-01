# Map and World System

## Overview

The Helbreath client implements a tile-based map system supporting a 752x752 tile world divided into 40x35 tile viewport regions. The system manages both static map geometry (walkability, teleports) and dynamic entities (players, monsters, NPCs, items, effects). Two parallel data structures track static tile properties and dynamic object state, with viewport-relative caching for efficient client-side updates.

## Source Files

| File | Lines | Purpose |
|------|-------|---------|
| `MapData.h` | ~85 | Class declaration and data structures |
| `MapData.cpp` | ~3,665 | Core implementation: frame updates, entity management |
| `Tile.h` | ~75 | Individual tile data structure for dynamic entities |
| `Tile.cpp` | ~90 | Tile initialization and clearing |
| `TileSpr.h` | ~28 | Static tile sprite/terrain definition |
| `ActionID.h` | - | Action/animation type constants |
| `DynamicObjectID.h` | - | Dynamic object type definitions |
| `Game.h` | - | Integration point (m_pMapData member) |

## Key Data Structures

### CMapData Class

The central map data manager:

```cpp
class CMapData {
    // Dynamic entities - 40x35 viewport grid
    CTile m_pData[MAPDATASIZEX][MAPDATASIZEY];      // 40x35 = 1,400 tiles
    CTile m_pTmpData[MAPDATASIZEX][MAPDATASIZEY];   // Temp buffer for viewport shifting

    // Static terrain - full world grid
    CTileSpr m_tile[752][752];                       // ~565,504 tiles total

    // Map dimensions (loaded from file)
    short m_sMapSizeX, m_sMapSizeY;

    // Viewport pivot (top-left corner of visible 40x35 region)
    short m_sPivotX, m_sPivotY;
    short m_sRectX, m_sRectY;                        // Internal offset calculation

    // Object ID lookup cache (fast position lookup)
    int m_iObjectIDcacheLocX[30000];                 // Object ID -> world X
    int m_iObjectIDcacheLocY[30000];                 // Object ID -> world Y

    // Animation frame tables per character type
    struct {
        short m_sMaxFrame;                           // Frame count for this action
        short m_sFrameTime;                          // Milliseconds per frame
    } m_stFrame[DEF_TOTALCHARACTERS][DEF_TOTALACTION];  // 80 types x 15 actions

    // Timing
    DWORD m_dwFrameTime;                             // Entity animation timing
    DWORD m_dwDOframeTime;                           // Dynamic object animation timing
    DWORD m_dwFrameAdjustTime;                       // Lag compensation
};
```

### CTile Structure

Stores all dynamic entities at a single map location:

```cpp
class CTile {
    //=== Living Entities (players/NPCs/monsters) ===
    WORD  m_wObjectID;                    // Object identifier (1-30000)
    short m_sOwnerType;                   // Character type (1-69)
    char  m_cOwnerName[12];               // Character name
    char  m_cDir;                         // Direction (1-8, clockwise from north)
    char  m_cOwnerAction;                 // Action state (STOP, MOVE, ATTACK, etc)
    char  m_cOwnerFrame;                  // Current animation frame
    short m_sAppr1, m_sAppr2;             // Appearance slot 1-2 (armor, helm)
    short m_sAppr3, m_sAppr4;             // Appearance slot 3-4 (weapon, shield)
    int   m_iApprColor;                   // Color tint for appearance
    short m_sStatus;                      // Status flags (frozen, poisoned, etc)
    short m_sV1, m_sV2, m_sV3;            // Variant data (weapon type, spell target)
    DWORD m_dwOwnerTime;                  // Timestamp for frame timing

    //=== Dead Bodies (corpses) ===
    WORD  m_wDeadObjectID;
    short m_sDeadOwnerType;
    char  m_cDeadOwnerName[12];
    char  m_cDeadDir;
    char  m_cDeadOwnerFrame;              // -1 = initial, increments during decay
    short m_sDeadAppr1, m_sDeadAppr2;
    short m_sDeadAppr3, m_sDeadAppr4;
    int   m_iDeadApprColor;
    short m_sDeadStatus;
    DWORD m_dwDeadOwnerTime;

    //=== Dynamic Objects (spells, environmental) ===
    short m_sDynamicObjectType;           // DEF_DYNAMICOBJECT_* type
    char  m_cDynamicObjectFrame;          // Current animation frame
    char  m_cDynamicObjectData1;          // Type-specific data
    char  m_cDynamicObjectData2;
    char  m_cDynamicObjectData3;
    char  m_cDynamicObjectData4;
    DWORD m_dwDynamicObjectTime;

    //=== Visual Effects ===
    int   m_iEffectType;                  // Effect layer (1-2)
    int   m_iEffectFrame;                 // Current frame
    int   m_iEffectTotalFrame;            // Total animation frames
    DWORD m_dwEffectTime;

    //=== Dropped Items ===
    short m_sItemSprite;                  // Sprite ID
    short m_sItemSpriteFrame;             // Frame/palette index
    int   m_cItemColor;                   // Color variant

    //=== Chat Messages ===
    int   m_iChatMsg;                     // Index into CGame::m_pChatMsgList
    int   m_iDeadChatMsg;                 // Chat message on corpse
};
```

### CTileSpr Structure

Static terrain data per world tile:

```cpp
class CTileSpr {
    short m_sTileSprite;          // Base terrain sprite ID
    short m_sTileSpriteFrame;     // Terrain animation frame
    short m_sObjectSprite;        // Static object overlay (trees, walls, etc)
    short m_sObjectSpriteFrame;   // Object frame
    bool  m_bIsMoveAllowed;       // Walkability flag (bit 0x80 in map file)
    bool  m_bIsTeleport;          // Teleport location marker (bit 0x40)
};
```

## Core Functions

### Initialization & Loading

#### `Init()`
Resets all 40x35 tiles to empty state:
- Clears object ID cache (30,000 entries)
- Initializes animation frame tables for 80 character types
- Sets pivot point to (-1, -1) indicating uninitialized

#### `OpenMapDataFile(char* filename)`
Loads static map data from disk:
- Reads 256-byte header containing `MAPSIZEX=...` and `MAPSIZEY=...`
- Decodes map dimensions via `_bDecodeMapInfo()`
- Loads full 752x752 tile array (10 bytes per tile)
- Populates `m_tile[x][y]` with terrain, objects, walkability, teleport flags

### Viewport Management

#### `ShiftMapData(char cDir)`
Pans the 40x35 viewport when player moves to edge:
```
Direction codes:
1 = North       5 = South
2 = Northeast   6 = Southwest
3 = East        7 = West
4 = Southeast   8 = Northwest
```
- Copies overlapping tiles from `m_pData` into `m_pTmpData`
- Updates `m_sPivotX`/`m_sPivotY` pivot coordinates
- Handles 8-directional seamless scrolling

#### `bGetIsLocateable(short sX, short sY)`
Tests if a world coordinate is walkable:
- Checks bounds within current viewport
- Verifies `m_tile[sX][sY].m_bIsMoveAllowed == TRUE`
- Rejects occupied tiles (`m_sOwnerType != NULL`)
- Rejects mineral deposits blocking movement

#### `bIsTeleportLoc(short sX, short sY)`
Tests for teleport trigger:
- Returns `m_tile[sX][sY].m_bIsTeleport` flag

### Entity Management

#### `bSetOwner(...)`
Places or updates a living entity at a map location.

**Parameters (15 total):**
- Object ID, position (X, Y)
- Character type, direction
- Appearance slots (4)
- Color, status
- Name, action, animation frame
- Variant data (3)

**Behavior:**
- Maintains object ID cache for quick lookups
- Positive cache = entity alive
- Negative cache = entity dead (corpse)
- Handles entity relocation (clears old position)
- Syncs chat messages with entity movement

#### `bGetOwner(...)`
Retrieves entity data at a location.

**Two overloads:**
1. Full data: type, direction, appearance, color, status, name, action, frame, chat index, variants
2. Quick lookup: name, type, status, object ID only

#### `bSetDeadOwner(...)`
Marks an entity as dead (creates corpse):
- Stores corpse data in `m_cDead*` fields
- Initializes `m_cDeadOwnerFrame = -1` (special initial state)
- Sets negative object ID in cache
- Clears living entity data

#### `bGetDeadOwner(...)`
Retrieves corpse data and dropped items.

### Dynamic Objects

#### `bSetDynamicObject(short sX, short sY, WORD wID, short sType, BOOL bIsEvent)`
Spawns environmental effects at a location.

**Dynamic Object Types:**
| ID | Constant | Description |
|----|----------|-------------|
| 1 | `DEF_DYNAMICOBJECT_FIRE` | Firewall spell |
| 2 | `DEF_DYNAMICOBJECT_FISH` | Fishing spot |
| 3 | `DEF_DYNAMICOBJECT_FISHOBJECT` | Fish sprites |
| 4 | `DEF_DYNAMICOBJECT_MINERAL1` | Mine node (type 1) |
| 5 | `DEF_DYNAMICOBJECT_MINERAL2` | Mine node (type 2) |
| 8 | `DEF_DYNAMICOBJECT_ICESTORM` | Ice storm effect |
| 9 | `DEF_DYNAMICOBJECT_SPIKE` | Spike trap |
| 10 | `DEF_DYNAMICOBJECT_PCLOUD_BEGIN` | Poison cloud (start) |
| 11 | `DEF_DYNAMICOBJECT_PCLOUD_LOOP` | Poison cloud (active) |
| 12 | `DEF_DYNAMICOBJECT_PCLOUD_END` | Poison cloud (fade) |
| 13 | `DEF_DYNAMICOBJECT_FIRE2` | Firewall variant |

### Animation & Rendering

#### `iObjectFrameCounter(char* playerName, short viewpointX, short viewpointY)`
Updates all entity animations. Called every ~90ms.

**For each of 40x35 tiles:**

1. **Living entities**: Increment `m_cOwnerFrame` based on timing
   - Frame time varies by character type and action (40-300ms typical)
   - Frozen status reduces speed by 25%
   - Skips frames on lag (up to 3 frames)

2. **Dead bodies**: Increment decay frame (0-10), then remove

3. **Dynamic objects**: Update animation frames
   - Fire: 24 frames
   - Spike: 13 frames
   - Ice storm: 10 frames
   - Poison cloud: Auto-transitions through BEGIN/LOOP/END states

4. **Action completion**:
   - Dying action converts entity to corpse
   - Other actions return to STOP

5. **Sound effects**: Plays footsteps, combat, spells based on frame/type

6. **Particle effects**: Adds equipment glints, weapon trails, etc.

#### `GetOwnerStatusByObjectID(WORD objectID, ...)`
Linear search for object by ID:
- Scans all 40x35 tiles to find matching `m_wObjectID`
- Used when object location unknown (slow path)

### Chat Messages

#### `bSetChatMsgOwner(WORD objectID, short sX, short sY, int iIndex)`
Associates chat message with an entity's tile.

#### `ClearChatMsg(short sX, short sY)`
Removes chat message from tile, frees memory.

### Items

#### `bSetItem(short sX, short sY, short sItemSpr, short sItemSpriteFrame, char cItemColor, BOOL bDropEffect)`
Drops an item on the map:
- Stores sprite ID, frame, color tint
- Optionally plays drop animation effect

## Constants & Limits

### Viewport Constants
```cpp
#define MAPDATASIZEX    40      // Viewport width in tiles
#define MAPDATASIZEY    35      // Viewport height in tiles
// Total cached tiles: 40 x 35 = 1,400
```

### World Constants
```cpp
// Full world dimensions
CTileSpr m_tile[752][752];      // ~565,504 tiles
                                // ~18.5 MB per map (uncompressed)
```

### Entity Constants
```cpp
#define DEF_TOTALCHARACTERS  80  // Character types (1-80)
#define DEF_TOTALACTION      15  // Action types (0-14)

// Object ID cache
int m_iObjectIDcacheLocX[30000]; // Max 30,000 concurrent objects
int m_iObjectIDcacheLocY[30000];
```

### Action Types
```cpp
#define DEF_OBJECTSTOP          0
#define DEF_OBJECTMOVE          1
#define DEF_OBJECTRUN           2
#define DEF_OBJECTATTACK        3
#define DEF_OBJECTMAGIC         4
#define DEF_OBJECTGETITEM       5
#define DEF_OBJECTDAMAGE        6
#define DEF_OBJECTDAMAGEMOVE    7
#define DEF_OBJECTATTACKMOVE    8
#define DEF_OBJECTDYING         10
#define DEF_OBJECTNULLACTION    100
#define DEF_OBJECTDEAD          101
```

### Character Type Ranges
| Range | Description |
|-------|-------------|
| 1-6 | Player characters (human male/female variants) |
| 10-37 | Common monsters (Slime, Skeleton, Orc, etc.) |
| 40-42 | Structures (Mana Collector, etc.) |
| 43-70 | Advanced monsters/NPCs (Gargoyle, Wyvern, etc.) |
| 65 | Special (flying creature with unique effects) |

## Integration Points

### With CGame

```cpp
class CGame {
    CMapData* m_pMapData;                         // Pointer to map system

    short m_sViewPointX, m_sViewPointY;           // Camera center (world coords)
    short m_sVDL_X, m_sVDL_Y;                     // Download location for new data

    CMsg* m_pChatMsgList[DEF_MAXCHATMSGS];        // Chat messages (linked by tiles)

    // Called by CMapData for effects/sounds:
    void PlaySound(char type, int num, int dist, long pan);
    void bAddNewEffect(short type, int x, int y, ...);
};
```

### Map Loading Sequence

1. `CGame::_ReadMapData()` - Sends network request for map tile data
2. `CGame::InitDataResponseHandler()` - Receives map file from server
3. `CMapData::OpenMapDataFile()` - Parses static terrain into `m_tile[752][752]`
4. `CMapData::Init()` - Clears dynamic entities in `m_pData[40][35]`
5. `CGame::MotionEventHandler()` - Server sends initial positions, calls `bSetOwner()`

## State Management

### Coordinate Systems

**World Space** (absolute):
- Range: 0-751 in X and Y
- Represents full map grid

**Viewport Space** (relative):
- Range: 0-39 in X, 0-34 in Y
- Conversion: `dX = sX - m_sPivotX`, `dY = sY - m_sPivotY`
- Valid only if within bounds

**Pixel Space** (screen):
- Each tile = 32x32 pixels
- World pixel = world tile * 32

### Entity Lifecycle

```
1. Spawn     -> Server sends position, bSetOwner() called
2. Animate   -> iObjectFrameCounter() increments frame every ~90ms
3. Move      -> Server updates position, bSetOwner() at new location
4. Death     -> Server sets action = DEF_OBJECTDYING
5. Corpse    -> Frame counter detects dying complete, calls bSetDeadOwner()
6. Decay     -> Corpse frame increments (0-10), after 10+ frames cleared
7. Despawn   -> Tile reset to empty state via Clear()
```

### Timing Model

| Event | Interval |
|-------|----------|
| Frame updates | ~90ms (`m_dwFrameTime`) |
| Dynamic object updates | ~100ms (`m_dwDOframeTime`) |
| Corpse decay | ~1.5 seconds (10 frames x 150ms) |
| Lag frame skip | Up to 3 frames skipped |

## Known Issues / Technical Debt

### Object ID Cache Inconsistency
- Negative cache values track dead entities
- Full 40x35 scan triggered on cache miss
- No validation that cached coordinate matches actual entity
- Can cause double-cleanup if entity removed while off-screen

### Memory Hazards
- CTile stores 12-byte char arrays for names (not null-safe)
- `strcpy()` used without bounds checking
- `ZeroMemory()` usage inconsistent

### Animation Frame Timing
- Per-character frame timing hardcoded in constructor
- No runtime configuration
- Some character types have tuning errors (typos in indices)

### Viewport Shifting
- `ShiftMapData()` uses hardcoded offset math
- 9-tile border maintained for reasons not documented
- Complex 8-directional case handling

### Thread Safety
- No synchronization primitives
- Socket thread could corrupt tiles while rendering thread reads
- Race conditions possible during rapid position updates

### Map File Format
- Header parsing fragile (string-based)
- 10-byte per-tile encoding never documented
- No version checking or migration support

### Dynamic Object State Machine
- Poison cloud transitions hardcoded in `iObjectFrameCounter()`
- Fire wall spawning uses magic numbers (24 frames)
- No validation of type vs data layout

## Modernization Notes

### Recommended Refactoring

**1. Split Concerns**
- Separate viewport management from entity data
- Extract animation timing into dedicated system
- Move dynamic object state machine to separate class

**2. Improve Type Safety**
```cpp
// Replace char arrays
std::string m_ownerName;

// Use enum class
enum class CharacterType : short { ... };
enum class ActionType : char { ... };
enum class DynamicObjectType : short { ... };

// Define coordinate types
struct WorldPos { short x, y; };
struct ViewportPos { short x, y; };
```

**3. Fix Caching**
```cpp
// Replace array with map
std::unordered_map<uint16_t, WorldPos> m_objectPositionCache;
```

**4. Better Timing**
```cpp
// Use std::chrono
std::chrono::steady_clock::time_point m_lastFrameUpdate;
std::chrono::milliseconds m_frameDuration{90};
```

**5. Safety**
- Use `std::span` instead of raw pointers
- Add bounds checking to all array accesses
- Implement proper synchronization (`std::mutex`)

**6. Modern Patterns**
- Use `std::optional` for nullable returns
- Use `std::expected` for error handling
- Implement visitor pattern for tile updates
