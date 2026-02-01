# Entity Management System

## Overview

The entity management system handles all dynamic objects in the game world: player characters, NPCs, monsters, items on the ground, and environmental objects. Entities are stored in a tile-based spatial structure with efficient ID-based lookup caching. The system supports 80 distinct entity types with up to 15 animation states each.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` / `Game.h` | Entity rendering, spawning logic, player tracking |
| `MapData.cpp` / `MapData.h` | CTile structure, entity storage, spatial queries |
| `Tile.h` | CTile class definition with entity data fields |

## Key Data Structures

### CTile - Entity Container

Each map tile stores data for live entities, dead entities (corpses), items, and dynamic objects:

```cpp
class CTile
{
public:
    // === LIVE ENTITY (OWNER) ===
    WORD  m_wObjectID;              // Unique entity ID (0 = empty)
    short m_sOwnerType;             // Entity type (1-80)
    short m_sAppr1;                 // Appearance: Head/Helmet
    short m_sAppr2;                 // Appearance: Body/Armor
    short m_sAppr3;                 // Appearance: Weapon/Shield
    short m_sAppr4;                 // Appearance: Special effects
    short m_sStatus;                // Status flags (poisoned, invisible, etc.)
    int   m_iApprColor;             // RGB color tint
    char  m_cOwnerName[12];         // Entity name
    char  m_cOwnerAction;           // Current action (DEF_OBJECTSTOP, etc.)
    char  m_cOwnerFrame;            // Current animation frame
    char  m_cDir;                   // Direction (0-7)
    short m_sV1, m_sV2, m_sV3;      // Extra values (context-dependent)
    DWORD m_dwOwnerTime;            // Timestamp for timing calculations

    // === DEAD ENTITY (CORPSE) ===
    WORD  m_wDeadObjectID;          // Dead entity ID
    short m_sDeadOwnerType;         // Entity type when dead
    short m_sDeadAppr1, m_sDeadAppr2, m_sDeadAppr3, m_sDeadAppr4;
    short m_sDeadStatus;            // Dead status flags
    int   m_iDeadApprColor;         // Dead appearance color
    char  m_cDeadOwnerName[12];     // Dead entity name
    char  m_cDeadOwnerFrame;        // Dead animation frame (-1 = initial)
    char  m_cDeadDir;               // Direction when died
    DWORD m_dwDeadOwnerTime;        // Corpse timestamp

    // === ITEMS ON GROUND ===
    short m_sItemSprite;            // Item sprite ID
    short m_sItemSpriteFrame;       // Item sprite frame
    int   m_cItemColor;             // Item color variant

    // === DYNAMIC OBJECTS ===
    short m_sDynamicObjectType;     // Dynamic object type (fire, fish, etc.)
    char  m_cDynamicObjectFrame;    // Animation frame
    char  m_cDynamicObjectData1;    // Custom data field 1
    char  m_cDynamicObjectData2;    // Custom data field 2
    char  m_cDynamicObjectData3;    // Custom data field 3
    char  m_cDynamicObjectData4;    // Custom data field 4
    DWORD m_dwDynamicObjectTime;    // Dynamic object timer

    // === EFFECTS & CHAT ===
    int   m_iEffectType;            // Visual effect type
    int   m_iEffectFrame;           // Effect frame counter
    int   m_iEffectTotalFrame;      // Total effect frames
    DWORD m_dwEffectTime;           // Effect timer
    int   m_iChatMsg;               // Chat message index (-1 = none)
    int   m_iDeadChatMsg;           // Chat message over corpse
};
```

### CMapData - Spatial Container

```cpp
class CMapData
{
public:
    CTile m_pData[MAPDATASIZEX][MAPDATASIZEY];  // 40x35 tile viewport grid

    // Animation frame data per entity type and action
    struct {
        short m_sMaxFrame;      // Total frames for this action
        short m_sFrameTime;     // Milliseconds per frame
    } m_stFrame[DEF_TOTALCHARACTERS][DEF_TOTALACTION];
};
```

### Object ID Cache (in CGame)

```cpp
// Fast entity lookup by ID
int m_iObjectIDcacheLocX[30000];  // X position cache
int m_iObjectIDcacheLocY[30000];  // Y position cache
// Positive = alive at location
// Negative (absolute value) = dead at location
// Zero = not cached
```

## Entity Types

### Character Type Constants

| Range | Type | Description |
|-------|------|-------------|
| 1-6 | Player Characters | 6 different player skins/classes |
| 7-9 | Reserved | Unused |
| 10 | Slime | Basic monster |
| 11 | Skeleton | Undead monster |
| 12 | Stone-Golem | Golem monster |
| 13 | Cyclops | Giant monster |
| 14 | Orc | Humanoid monster |
| 15 | ShopKeeper-W | NPC vendor |
| 16 | Giant Ant | Insect monster |
| 17 | Scorpion | Insect monster |
| 18 | Zombie | Undead monster |
| 19-26 | Named NPCs | Gandlf, Howard, Guard, Tom, William, Kenedy, etc. |
| 27 | Hellbound | Demon monster |
| 28 | Troll | Large monster |
| 29 | Ogre | Large monster |
| 30 | Liche | Undead caster |
| 31 | Demon | Boss monster |
| 32 | Unicorn | Rare creature |
| 33 | WereWolf | Lycanthrope |
| 34 | Dummy | Training target |
| 35 | Energy-Ball | Summoned object (special transparency) |
| 36-42 | Crusade Structures | Towers, collectors, generators (static) |
| 43-62+ | Additional Monsters | Beetles, knights, stalkers, gargoyles, etc. |

**Total: 80 entity types** (`DEF_TOTALCHARACTERS = 80`)

### Dynamic Object Types

```cpp
#define DEF_DYNAMICOBJECT_FIRE          1   // Burning fire effect
#define DEF_DYNAMICOBJECT_FISH          2   // Fishing spot indicator
#define DEF_DYNAMICOBJECT_FISHOBJECT    3   // Active fishing object
#define DEF_DYNAMICOBJECT_MINERAL1      4   // Mineral deposit type 1
#define DEF_DYNAMICOBJECT_MINERAL2      5   // Mineral deposit type 2
#define DEF_DYNAMICOBJECT_ICESTORM      8   // Ice storm effect
#define DEF_DYNAMICOBJECT_SPIKE         9   // Spike trap
#define DEF_DYNAMICOBJECT_PCLOUD_BEGIN  10  // Poison cloud start
#define DEF_DYNAMICOBJECT_PCLOUD_LOOP   11  // Poison cloud active
#define DEF_DYNAMICOBJECT_PCLOUD_END    12  // Poison cloud dissipate
#define DEF_DYNAMICOBJECT_FIRE2         13  // Alternate fire effect
```

## Core Functions

### Entity Spawning

```cpp
// Add or update an entity at a tile position
BOOL CMapData::bSetOwner(
    WORD wObjectID,         // Unique entity ID
    int sX, int sY,         // Tile coordinates (-1,-1 to remove)
    int sType,              // Entity type (1-80)
    int cDir,               // Direction (0-7)
    short sAppr1,           // Head/Helmet appearance
    short sAppr2,           // Body/Armor appearance
    short sAppr3,           // Weapon/Shield appearance
    short sAppr4,           // Special effects
    int iApprColor,         // RGB color tint
    short sStatus,          // Status flags
    char* pName,            // Entity name (max 11 chars)
    short sAction,          // Initial action state
    short sV1, sV2, sV3,    // Extra data values
    int iPreLoc,            // Previous location (for movement)
    int iFrame              // Initial animation frame
);
```

### Entity Retrieval

```cpp
// Get entity data at a tile position
BOOL CMapData::bGetOwner(
    short sX, short sY,
    short* pOwnerType,
    char* pDir,
    short* pAppr1, short* pAppr2, short* pAppr3, short* pAppr4,
    int* pApprColor,
    short* pStatus,
    char* pName,
    char* pAction,
    char* pFrame,
    int* pChatIndex,
    short* pV1, short* pV2
);

// Get entity by Object ID (uses cache for O(1) lookup)
void CMapData::GetOwnerStatusByObjectID(
    WORD wObjectID,
    char* pOwnerType,
    char* pDir,
    short* pAppr1, short* pAppr2, short* pAppr3, short* pAppr4,
    short* pStatus,
    int* pColor,
    char* pName
);
```

### Dead Entity (Corpse) Handling

```cpp
// Create a corpse at a tile position
BOOL CMapData::bSetDeadOwner(
    WORD wObjectID,
    short sX, short sY,
    short sType,
    char cDir,
    short sAppr1, short sAppr2, short sAppr3, short sAppr4,
    int iApprColor,
    short sStatus,
    char* pName
);

// Retrieve corpse data
BOOL CMapData::bGetDeadOwner(
    short sX, short sY,
    short* pOwnerType,
    char* pDir,
    short* pAppr1, short* pAppr2, short* pAppr3, short* pAppr4,
    int* pApprColor,
    short* pStatus,
    char* pName
);
```

### Dynamic Objects and Items

```cpp
// Place a dynamic object (fire, fishing spot, etc.)
BOOL CMapData::bSetDynamicObject(
    short sX, short sY,
    WORD wID,
    short sType,
    BOOL bIsEvent
);

// Place an item on the ground
BOOL CMapData::bSetItem(
    short sX, short sY,
    short sItemSpr,
    short sItemSprFrame,
    char cItemColor,
    BOOL bDropEffect
);
```

## Action States

Entities have 15 possible action states that determine animation:

| Constant | Value | Description | Typical Frames |
|----------|-------|-------------|----------------|
| `DEF_OBJECTSTOP` | 0 | Standing idle | 14 (players) |
| `DEF_OBJECTMOVE` | 1 | Walking | 7 |
| `DEF_OBJECTRUN` | 2 | Running | 7 |
| `DEF_OBJECTATTACK` | 3 | Melee attack | 7 |
| `DEF_OBJECTMAGIC` | 4 | Casting spell | 15 |
| `DEF_OBJECTGETITEM` | 5 | Picking up item | 3 |
| `DEF_OBJECTDAMAGE` | 6 | Taking damage (knockback) | 7 |
| `DEF_OBJECTDAMAGEMOVE` | 7 | Damaged while moving | 3 |
| `DEF_OBJECTATTACKMOVE` | 8 | Attack while moving | 12 |
| `DEF_OBJECTDYING` | 10 | Death animation | 12 |
| `DEF_OBJECTDEAD` | 101 | Corpse (static) | 1 |

## Entity Rendering

### Main Render Loop

```cpp
void CGame::DrawObjects(
    short sPivotX, short sPivotY,   // Camera center position
    short sDivX, short sDivY,       // Viewport division
    short sModX, short sModY,       // Pixel offset
    short msX, short msY            // Mouse position
);
```

### Action-Specific Render Functions

```cpp
void DrawObject_OnStop();           // Idle animation
void DrawObject_OnMove();           // Walk animation
void DrawObject_OnRun();            // Run animation
void DrawObject_OnAttack();         // Attack animation
void DrawObject_OnMagic();          // Cast animation
void DrawObject_OnGetItem();        // Pickup animation
void DrawObject_OnDamage();         // Knockback animation
void DrawObject_OnDamageMove();     // Damaged while moving
void DrawObject_OnAttackMove();     // Attack while moving
void DrawObject_OnDying();          // Death sequence
void DrawObject_OnDead();           // Corpse rendering
```

### Sprite ID Calculation

```cpp
// Player characters (types 1-6)
SpriteID = 500 + (OwnerType - 1) * 8 * 15 + (ActionIndex * 8) + Direction;

// Monsters (types 10+)
SpriteID = 1220 + (OwnerType - 10) * 8 * 7 + (ActionIndex * 8) + Direction;
```

### Special Rendering Cases

- **Energy-Ball (Type 35) & Wyvern (Type 66)**: Always rendered with transparency (`bInv = TRUE`)
- **Crusade Structures (Types 36-42, 51)**: Static display, no movement frames
- **Player Names**: Only rendered for types 1-6
- **Corpses**: Rendered in a separate pass below living entities

## Animation System

### Frame Data Structure

```cpp
// Pre-computed for each entity type and action
m_stFrame[EntityType][ActionIndex].m_sMaxFrame;   // Total frames
m_stFrame[EntityType][ActionIndex].m_sFrameTime;  // MS per frame
```

### Frame Advancement

```cpp
// Calculate current frame from elapsed time
ElapsedTime = CurrentTime - m_dwOwnerTime;
FrameTime = m_stFrame[Type][Action].m_sFrameTime;
MaxFrames = m_stFrame[Type][Action].m_sMaxFrame;
CurrentFrame = (ElapsedTime / FrameTime) % MaxFrames;
```

### Animation Timing Examples

| Entity Type | Action | Frames | Frame Time | Total Duration |
|-------------|--------|--------|------------|----------------|
| Player | STOP | 14 | 60ms | 840ms |
| Player | MOVE | 7 | 70ms | 490ms |
| Player | ATTACK | 7 | 60ms | 420ms |
| Slime | STOP | 4 | 240ms | 960ms |
| Slime | MOVE | 4 | 120ms | 480ms |
| Troll | ATTACK | 5 | 60ms | 300ms |

## Direction System

Entities face one of 8 directions:

```
    0 (North)
  7   1
6   X   2
  5   3
    4 (South)
```

| Value | Direction |
|-------|-----------|
| 0 | North |
| 1 | Northeast |
| 2 | East |
| 3 | Southeast |
| 4 | South |
| 5 | Southwest |
| 6 | West |
| 7 | Northwest |

## Appearance System

### Equipment Visual Layers

| Field | Purpose | Examples |
|-------|---------|----------|
| `m_sAppr1` | Head/Helmet | Hair style, helmet type |
| `m_sAppr2` | Body/Armor | Clothing, armor type |
| `m_sAppr3` | Weapon/Shield | Weapon sprite, shield type |
| `m_sAppr4` | Special Effects | Invisibility, blessing glow |

### Status Flags (m_sStatus)

Bit flags indicating entity conditions:

- Poisoned
- Paralyzed
- Invisible
- Blessed
- Cursed
- PvP mode enabled
- Combat mode active
- Guild faction indicator

## Constants & Limits

```cpp
#define DEF_TOTALCHARACTERS     80      // Max entity types
#define DEF_TOTALACTION         15      // Max animation actions
#define MAPDATASIZEX            40      // Viewport width in tiles
#define MAPDATASIZEY            35      // Viewport height in tiles

// Object ID ranges
// 0          = No entity
// 1-29999    = Live entities (players, monsters, NPCs)
// 30000+     = Dynamic objects (items, environmental)

// Cache arrays
int m_iObjectIDcacheLocX[30000];
int m_iObjectIDcacheLocY[30000];
```

## Entity Lifecycle

### 1. Spawn
Server sends `MotionEventHandler` packet with entity data.

```cpp
bSetOwner(wObjectID, sX, sY, sType, cDir,
          sAppr1, sAppr2, sAppr3, sAppr4,
          iApprColor, sStatus, pName,
          sAction, sV1, sV2, sV3, iPreLoc, iFrame);
```

### 2. Cache Update
Location cached for fast ID-based lookup.

```cpp
m_iObjectIDcacheLocX[wObjectID] = sX;
m_iObjectIDcacheLocY[wObjectID] = sY;
```

### 3. Render Loop
Each frame, viewport is scanned and entities drawn.

```cpp
DrawObjects(sPivotX, sPivotY, ...);
// Finds entity in CTile, calls appropriate DrawObject_On*()
```

### 4. Animation
Frame advances based on elapsed time and frame rate.

### 5. Movement/Update
Server sends new position/action, entity data updated.

### 6. Death
Death action triggered, death animation plays.

```cpp
// Action changes to DEF_OBJECTDYING
DrawObject_OnDying();  // 12-frame death sequence
```

### 7. Corpse
After death animation, converted to dead entity.

```cpp
bSetDeadOwner(wObjectID, sX, sY, sType, ...);
// Stored in m_wDeadObjectID instead of m_wObjectID
```

### 8. Removal
Server signals removal, entity cleared.

```cpp
bSetOwner(wObjectID, -1, -1, ...);  // Position -1,-1 = remove
m_iObjectIDcacheLocX[wObjectID] = 0;
m_iObjectIDcacheLocY[wObjectID] = 0;
```

## Integration Points

| System | Integration |
|--------|-------------|
| **Network** | Receives spawn/update/remove packets via message handlers |
| **Rendering** | DrawObjects() iterates viewport, renders each entity |
| **Map System** | CTile stores entity data, CMapData manages spatial grid |
| **Combat** | Attack actions, damage animations, death handling |
| **Effects** | Visual effects attached to entity positions |
| **Chat** | Chat bubbles associated with entity tiles |

## Known Issues / Technical Debt

1. **Monolithic Storage**: All entity data packed into CTile structure
2. **Fixed Array Sizes**: 30,000 entity ID limit, 40x35 viewport
3. **No Entity Pooling**: New/delete for each spawn potentially
4. **Tight Coupling**: Entity rendering mixed with game logic in CGame
5. **Magic Numbers**: Sprite ID calculations use hardcoded offsets
6. **Redundant Data**: Live and dead entity fields duplicated in CTile
7. **Global Cache**: Object ID cache is global, not encapsulated

## Modernization Notes

### Recommended C++20 Approach

1. **Entity Component System (ECS)**
   - Separate components: Transform, Sprite, Animation, Combat, Movement
   - EntityManager with typed queries

2. **Smart Pointers**
   - `std::unique_ptr` for owned entities
   - `std::weak_ptr` for references

3. **Type-Safe IDs**
   - `enum class EntityId : uint32_t` instead of raw WORD
   - Separate ID types for different entity categories

4. **Spatial Partitioning**
   - Replace fixed tile array with spatial hash or quadtree
   - Support variable viewport sizes

5. **Animation System**
   - Data-driven animation definitions
   - State machine for action transitions

6. **Render Batching**
   - Group entities by sprite sheet
   - Instanced rendering for same-type entities

### Example Modern Interface

```cpp
namespace hb::entity {
    using EntityId = uint32_t;

    class Entity {
    public:
        [[nodiscard]] EntityId id() const noexcept;
        [[nodiscard]] EntityType type() const noexcept;

        template<typename T>
        T* getComponent();
    };

    class EntityManager {
    public:
        Entity& spawn(EntityType type, Vec2 position);
        void destroy(EntityId id);
        Entity* find(EntityId id);

        void update(float deltaTime);
        void render(gfx::IRenderer& renderer);
    };
}
```
