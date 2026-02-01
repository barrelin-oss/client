# Tile Rendering System

## Overview

The Helbreath client uses a classic 2D tile-based rendering system built on DirectX 7 (DirectDraw). Tiles are 32x32 pixel sprites rendered in a grid pattern with multiple layers: terrain/background tiles at the base, followed by object sprites, entities, items, and effects. The system supports dynamic objects, animation frames, and per-tile entity tracking.

**Key Characteristics:**
- Tile size: 32x32 pixels
- Map grid: 752x752 tiles (maximum world size)
- Display resolution: 640x480 pixels
- Viewport coverage: ~20x15 visible tiles (with overflow for tall sprites)
- Dual-layer rendering: Background tiles + Object/Entity sprites
- Per-tile state management via `CTile` and `CTileSpr` classes

---

## Source Files

| File | Purpose |
|------|---------|
| `Tile.h` | CTile class definition - dynamic tile state (entities, items, dead bodies) |
| `Tile.cpp` | CTile implementation |
| `TileSpr.h` | CTileSpr class definition - static tile sprite data from map files |
| `TileSpr.cpp` | CTileSpr implementation |
| `MapData.h` | CMapData class - map grid management, frame timing tables |
| `MapData.cpp` | Map initialization, animation timing configuration |
| `Game.h` / `Game.cpp` | Core rendering functions: `DrawBackground()`, `DrawObjects()` |
| `Sprite.h` / `Sprite.cpp` | CSprite class - DirectDraw sprite rendering methods |
| `ActionID.h` | Animation action type enums (stop, move, attack, etc.) |

---

## Key Data Structures

### CTile Class

The `CTile` class represents the **dynamic state** of a single map tile. It tracks all entities and temporary data present on that tile position.

```cpp
class CTile {
public:
    // Living entity on this tile
    DWORD m_dwOwnerTime;           // Timestamp when entity was placed
    int   m_iChatMsg;              // Chat message ID (if entity is talking)
    int   m_iApprColor;            // Appearance color modifier
    int   m_iEffectType;           // Visual effect type on entity
    int   m_iEffectFrame;          // Current effect animation frame
    int   m_iEffectTotalFrame;     // Total effect animation frames

    WORD  m_wObjectID;             // Unique entity ID (server-assigned)
    short m_sOwnerType;            // Character type (1-6 = players, 10+ = monsters)
    short m_sAppr1, m_sAppr2;      // Appearance data (equipment visuals)
    short m_sAppr3, m_sAppr4;      // Additional appearance slots
    short m_sStatus;               // Status flags (buffs, debuffs)

    char  m_cOwnerAction;          // Current action (from ActionID.h)
    char  m_cOwnerFrame;           // Animation frame within action
    char  m_cDir;                  // Direction facing (0-7, 8 directions)
    char  m_cOwnerName[12];        // Character name (11 chars + null)

    // Dead entity on this tile (corpse layer)
    DWORD m_dwDeadOwnerTime;
    WORD  m_wDeadObjectID;
    short m_sDeadOwnerType;
    short m_sDeadAppr1, m_sDeadAppr2, m_sDeadAppr3, m_sDeadAppr4;
    short m_sDeadStatus;
    char  m_cDeadOwnerFrame;       // -1 = not initialized, 0+ = death frame
    char  m_cDeadDir;
    char  m_cDeadOwnerName[12];

    // Item on ground at this tile
    short m_sItemSprite;           // Item sprite index
    short m_sItemSpriteFrame;      // Item animation frame
    int   m_cItemColor;            // Item color tint index

    // Dynamic objects (doors, chests, construction sites)
    short m_sDynamicObjectType;    // Type of dynamic object
    char  m_cDynamicObjectFrame;   // Animation frame
    char  m_cDynamicObjectData1;   // Object-specific data
    char  m_cDynamicObjectData2;
    char  m_cDynamicObjectData3;
    char  m_cDynamicObjectData4;

    void Clear();  // Reset all fields to empty state
};
```

### CTileSpr Class

The `CTileSpr` class represents **static sprite data** for a tile, loaded from map files.

```cpp
class CTileSpr {
public:
    short m_sTileSprite;           // Background/terrain sprite ID
    short m_sTileSpriteFrame;      // Terrain sprite frame number
    short m_sObjectSprite;         // Object/static sprite ID (trees, rocks, buildings)
    short m_sObjectSpriteFrame;    // Object sprite frame number
    bool  m_bIsMoveAllowed;        // Can entities walk on this tile?
    bool  m_bIsTeleport;           // Is this a teleport exit location?
};
```

### CMapData Class

The `CMapData` class manages the entire map's tile data and provides the interface between rendering and game logic.

```cpp
class CMapData {
public:
    // Dynamic tile data (viewport window)
    CTile m_pData[MAPDATASIZEX][MAPDATASIZEY];     // 40x35 active viewport
    CTile m_pTmpData[MAPDATASIZEX][MAPDATASIZEY];  // Temporary buffer

    // Static tile data (full world map)
    CTileSpr m_tile[752][752];                     // Full world sprite data

    // Map positioning and scrolling
    short m_sMapSizeX, m_sMapSizeY;   // Total map dimensions in tiles
    short m_sRectX, m_sRectY;         // Current viewport position (world coords)
    short m_sPivotX, m_sPivotY;       // Data array pivot point (viewport center)

    // Animation frame timing tables
    struct {
        short m_sMaxFrame;     // Number of frames in animation
        short m_sFrameTime;    // Milliseconds per frame
    } m_stFrame[DEF_TOTALCHARACTERS][DEF_TOTALACTION];  // 80 types x 15 actions

    // Timing state
    DWORD m_dwFrameTime;           // Current frame timestamp
    DWORD m_dwDOframeTime;         // Dynamic object frame time
    DWORD m_dwFrameCheckTime;      // Last frame timing check
    DWORD m_dwFrameAdjustTime;     // Frame adjustment offset

    // Object ID location cache (for fast entity lookups)
    int m_iObjectIDcacheLocX[30000];  // X position by object ID
    int m_iObjectIDcacheLocY[30000];  // Y position by object ID
};
```

---

## Tile Properties & Flags

### Tile Property Summary

| Property | Source | Purpose |
|----------|--------|---------|
| Terrain Sprite | `CTileSpr::m_sTileSprite` | Visual appearance of ground |
| Object Sprite | `CTileSpr::m_sObjectSprite` | Static objects (trees, walls) |
| Walkable | `CTileSpr::m_bIsMoveAllowed` | Movement collision |
| Teleport | `CTileSpr::m_bIsTeleport` | Teleport destination marker |
| Entity | `CTile::m_cOwnerName[0] != 0` | Living entity present |
| Corpse | `CTile::m_wDeadObjectID != 0` | Dead body present |
| Item | `CTile::m_sItemSprite != 0` | Ground item present |
| Dynamic Object | `CTile::m_sDynamicObjectType != 0` | Interactive object present |

### Entity Types (m_sOwnerType)

| Range | Entity Type |
|-------|-------------|
| 1-6 | Player characters (different gender/class combinations) |
| 10+ | Monsters and NPCs |

### Direction Values (m_cDir)

```
    0
  7   1
6   X   2
  5   3
    4
```

8-direction system: 0=North, 1=NE, 2=East, 3=SE, 4=South, 5=SW, 6=West, 7=NW

---

## Animation Action Types

Defined in `ActionID.h`:

```cpp
#define DEF_OBJECTSTOP        0     // Standing idle
#define DEF_OBJECTMOVE        1     // Walking
#define DEF_OBJECTRUN         2     // Running
#define DEF_OBJECTATTACK      3     // Melee attack
#define DEF_OBJECTMAGIC       4     // Casting spell
#define DEF_OBJECTGETITEM     5     // Picking up item
#define DEF_OBJECTDAMAGE      6     // Taking damage (hit reaction)
#define DEF_OBJECTDAMAGEMOVE  7     // Moving while damaged
#define DEF_OBJECTATTACKMOVE  8     // Moving while attacking
#define DEF_OBJECTDYING       10    // Death animation
#define DEF_OBJECTNULLACTION  100   // No action (placeholder)
#define DEF_OBJECTDEAD        101   // Dead body state
```

### Frame Timing Configuration

Set in `CMapData` constructor for each character type and action:

```cpp
// Player characters (types 1-6)
m_stFrame[1-6][DEF_OBJECTSTOP].m_sMaxFrame = 14;
m_stFrame[1-6][DEF_OBJECTSTOP].m_sFrameTime = 60;     // 60ms per frame

m_stFrame[1-6][DEF_OBJECTMOVE].m_sMaxFrame = 7;
m_stFrame[1-6][DEF_OBJECTMOVE].m_sFrameTime = 70;     // 70ms per frame

m_stFrame[1-6][DEF_OBJECTRUN].m_sMaxFrame = 7;
m_stFrame[1-6][DEF_OBJECTRUN].m_sFrameTime = 42;      // 42ms per frame (faster)

m_stFrame[1-6][DEF_OBJECTATTACK].m_sMaxFrame = 7;
m_stFrame[1-6][DEF_OBJECTATTACK].m_sFrameTime = 78;

m_stFrame[1-6][DEF_OBJECTMAGIC].m_sMaxFrame = 15;
m_stFrame[1-6][DEF_OBJECTMAGIC].m_sFrameTime = 88;

m_stFrame[1-6][DEF_OBJECTGETITEM].m_sMaxFrame = 3;
m_stFrame[1-6][DEF_OBJECTGETITEM].m_sFrameTime = 150;

m_stFrame[1-6][DEF_OBJECTDAMAGE].m_sMaxFrame = 7;
m_stFrame[1-6][DEF_OBJECTDAMAGE].m_sFrameTime = 70;

m_stFrame[1-6][DEF_OBJECTDYING].m_sMaxFrame = 12;
m_stFrame[1-6][DEF_OBJECTDYING].m_sFrameTime = 80;

// Monsters have different timing (examples)
m_stFrame[10][DEF_OBJECTMOVE].m_sFrameTime = 120;  // Slime - slow
m_stFrame[11][DEF_OBJECTMOVE].m_sFrameTime = 90;   // Skeleton - medium
```

**Animation Loop Durations:**
| Action | Frames | Time/Frame | Total Duration |
|--------|--------|------------|----------------|
| Idle | 14 | 60ms | 840ms |
| Walk | 7 | 70ms | 490ms |
| Run | 7 | 42ms | 294ms |
| Attack | 7 | 78ms | 546ms |
| Magic | 15 | 88ms | 1320ms |
| Damage | 7 | 70ms | 490ms |
| Dying | 12 | 80ms | 960ms |

---

## Map Drawing Pipeline

### Overview

The rendering pipeline has two main phases:
1. **DrawBackground()** - Renders terrain tiles to a cached surface
2. **DrawObjects()** - Renders entities, items, objects, and effects

### DrawBackground() Function

Located in `Game.cpp` (~line 15841). Renders terrain tiles with caching optimization.

```cpp
void CGame::DrawBackground(short sDivX, short sModX,
                          short sDivY, short sModY) {

    if (sDivX < 0 || sDivY < 0) return;

    // Check if we need to redraw the cached background
    if ((m_bIsRedrawPDBGS == TRUE) ||
        (m_iPDBGSdivX != sDivX) ||
        (m_iPDBGSdivY != sDivY)) {

        m_bIsRedrawPDBGS = FALSE;
        m_iPDBGSdivX = sDivX;
        m_iPDBGSdivY = sDivY;

        // Set clip area for pre-draw surface (slightly larger than screen)
        SetRect(&m_DDraw.m_rcClipArea, 0, 0, 640+32, 480+32);

        // Iterate through visible tiles
        indexY = sDivY + m_pMapData->m_sPivotY;
        for (iy = -sModY; iy < 427+48; iy += 32) {

            indexX = sDivX + m_pMapData->m_sPivotX;
            for (ix = -sModX; ix < 640+48; ix += 32) {

                // Get terrain sprite for this world position
                sSpr      = m_pMapData->m_tile[indexX][indexY].m_sTileSprite;
                sSprFrame = m_pMapData->m_tile[indexX][indexY].m_sTileSpriteFrame;

                // Render to cached background surface
                m_pTileSpr[sSpr]->PutSpriteFastNoColorKeyDst(
                    m_DDraw.m_lpPDBGS,           // Pre-Draw BackGround Surface
                    ix - 16 + sModX,
                    iy - 16 + sModY,
                    sSprFrame,
                    m_dwCurTime
                );

                indexX++;
            }
            indexY++;
        }

        SetRect(&m_DDraw.m_rcClipArea, 0, 0, 640, 480);
    }

    // Copy cached background to back buffer
    RECT rcRect;
    SetRect(&rcRect, sModX, sModY, 640+sModX, 480+sModY);
    m_DDraw.m_lpBackB4->BltFast(0, 0, m_DDraw.m_lpPDBGS,
                                &rcRect, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
}
```

**Key Optimization:** The Pre-Draw Background Surface (PDBS) caches the terrain. It's only redrawn when the viewport moves to a new tile boundary (when `sDivX` or `sDivY` changes). This avoids redrawing static terrain every frame.

### DrawObjects() Function

Located in `Game.cpp` (~line 2278). Renders all dynamic content.

```cpp
void CGame::DrawObjects(short sPivotX, short sPivotY,
                       short sDivX, short sDivY,
                       short sModX, short sModY,
                       short msX, short msY) {

    if (sDivY < 0 || sDivX < 0) return;

    // Extended bounds for tall sprites (characters can be 100+ pixels tall)
    // Vertical: -224 to 779 (covers sprites above viewport)
    // Horizontal: -128 to 768 (covers sprites at edges)

    indexY = sDivY + sPivotY - 7;
    for (iy = -sModY - 224; iy <= 427 + 352; iy += 32) {

        indexX = sDivX + sPivotX - 4;
        for (ix = -sModX - 128; ix <= 640 + 128; ix += 32) {

            // Skip if completely outside visible area
            if ((ix < -sModX) || (ix > 640+16) ||
                (iy < -sModY) || (iy > 427+32+16)) {
                indexX++;
                continue;
            }

            // Bounds check against loaded map data
            if ((indexX < m_pMapData->m_sPivotX) ||
                (indexX > m_pMapData->m_sPivotX + MAPDATASIZEX) ||
                (indexY < m_pMapData->m_sPivotY) ||
                (indexY > m_pMapData->m_sPivotY + MAPDATASIZEY)) {
                indexX++;
                continue;
            }

            // Convert world coords to viewport array indices
            dX = indexX - m_pMapData->m_sPivotX;
            dY = indexY - m_pMapData->m_sPivotY;

            // Layer 1: Dead bodies (corpses rendered first, underneath living)
            if (m_pMapData->m_pData[dX][dY].m_wDeadObjectID != 0) {
                DrawObject_OnDead(indexX, indexY, ix, iy, FALSE, dwTime, msX, msY);
            }

            // Layer 2: Living entities
            if (strlen(m_pMapData->m_pData[dX][dY].m_cOwnerName) > 0) {

                char cAction = m_pMapData->m_pData[dX][dY].m_cOwnerAction;

                // Dispatch to action-specific rendering
                switch (cAction) {
                case DEF_OBJECTSTOP:
                    DrawObject_OnStop(...);
                    break;
                case DEF_OBJECTMOVE:
                    DrawObject_OnMove(...);
                    break;
                case DEF_OBJECTRUN:
                    DrawObject_OnRun(...);
                    break;
                case DEF_OBJECTATTACK:
                    DrawObject_OnAttack(...);
                    break;
                case DEF_OBJECTMAGIC:
                    DrawObject_OnMagic(...);
                    break;
                // ... other actions
                }
            }

            // Layer 3: Ground items
            if (m_pMapData->m_pData[dX][dY].m_sItemSprite != 0) {
                // Render item sprite at tile center
            }

            // Layer 4: Static object sprites (trees, buildings)
            sObjSpr = m_pMapData->m_tile[indexX][indexY].m_sObjectSprite;
            if (sObjSpr != 0) {
                // Render object with potential transparency for player visibility
            }

            indexX++;
        }
        indexY++;
    }
}
```

---

## Object Rendering Details

### Static Object Sprites

Objects defined in `CTileSpr` (trees, walls, buildings) are rendered after entities:

```cpp
sObjSpr      = m_pMapData->m_tile[indexX][indexY].m_sObjectSprite;
sObjSprFrame = m_pMapData->m_tile[indexX][indexY].m_sObjectSpriteFrame;

if (sObjSpr != 0) {
    // Check if player is behind this object
    if ((bIsPlayerDrawed == TRUE) &&
        (m_pTileSpr[sObjSpr]->m_rcBound.top <= m_rcPlayerRect.top)) {

        // Render shadow version (sprites at offset +50)
        m_pTileSpr[sObjSpr + 50]->PutFadeSprite(ix, iy, sObjSprFrame, dwTime);

        // Render object with transparency so player is visible
        m_pTileSpr[sObjSpr]->PutTransSprite2(ix - 16, iy - 16, sObjSprFrame, dwTime);
    } else {
        // Normal opaque rendering
        m_pTileSpr[sObjSpr]->PutSpriteFast(ix - 16, iy - 16, sObjSprFrame, dwTime);
    }
}
```

### Ground Items

Items dropped on the ground are rendered with optional color tinting:

```cpp
if (sItemSprite != 0) {
    if (cItemColor == 0) {
        // Standard item rendering
        m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + sItemSprite]
            ->PutSpriteFast(ix, iy, sItemSpriteFrame, dwTime);
    } else {
        // Color-tinted rendering (for enchanted/special items)
        m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + sItemSprite]
            ->PutSpriteRGB(ix, iy, sItemSpriteFrame,
                          colorR, colorG, colorB, dwTime);
    }

    // Highlight on mouse hover (blinking effect)
    if ((ix - 13 < msX) && (ix + 13 > msX) &&
        (iy - 13 < msY) && (iy + 13 > msY)) {
        // Set cursor to pickup mode
    }
}
```

### Dynamic Objects

Interactive objects like doors, chests, and construction sites:

```cpp
sDynamicObject = m_pMapData->m_pData[dX][dY].m_sDynamicObjectType;
if (sDynamicObject != 0) {
    sDynamicObjectFrame = (short)m_pMapData->m_pData[dX][dY].m_cDynamicObjectFrame;
    // Render based on object type
    // Data1-4 fields contain state information
}
```

---

## Sprite Rendering Methods

The `CSprite` class provides multiple rendering methods used by the tile system:

| Method | Purpose | Usage |
|--------|---------|-------|
| `PutSpriteFast()` | Direct rendering, no transparency | Terrain tiles |
| `PutSpriteFastNoColorKey()` | No color key (solid fill) | Background surface |
| `PutSpriteFastNoColorKeyDst()` | Render to specified surface | PDBS caching |
| `PutTransSprite()` | Alpha-blended transparency | See-through effects |
| `PutTransSprite70()` | 70% opacity | Partial visibility |
| `PutTransSprite50()` | 50% opacity | Ghost/fade effects |
| `PutTransSprite25()` | 25% opacity | Near-invisible |
| `PutShadowSprite()` | Dark shadow rendering | Entity shadows |
| `PutSpriteRGB()` | Color-tinted rendering | Colored items |
| `PutFadeSprite()` | Faded overlay | Occluded objects |

---

## Constants & Limits

### Screen and Viewport

```cpp
#define SCREEN_WIDTH        640    // Display width in pixels
#define SCREEN_HEIGHT       480    // Display height in pixels

// Calculated viewport coverage
Tiles visible X = 640 / 32 = 20 tiles
Tiles visible Y = 480 / 32 = 15 tiles

// Extended rendering bounds (for tall sprites)
Vertical range:   -224 to 779  (~31 tiles)
Horizontal range: -128 to 768  (~28 tiles)
```

### Map Dimensions

```cpp
#define MAPDATASIZEX        40     // Viewport window width in tiles
#define MAPDATASIZEY        35     // Viewport window height in tiles

// Full world map (static array)
m_tile[752][752]                   // 752x752 maximum world (~24,064x24,064 pixels)

// Memory: 752 * 752 * sizeof(CTileSpr) = ~5.6 MB for sparse map
```

### Sprite Limits

```cpp
#define DEF_MAXTILES        500    // Maximum tile sprites
#define DEF_MAXSPRITES      20000  // Maximum total sprites
#define DEF_TOTALCHARACTERS 80     // Character type definitions
#define DEF_TOTALACTION     15     // Animation action types
```

### Object Cache

```cpp
m_iObjectIDcacheLocX[30000]        // X position cache by object ID
m_iObjectIDcacheLocY[30000]        // Y position cache by object ID
```

---

## Integration Points

### Map Data Integration

- **Static sprites**: Accessed via `m_pMapData->m_tile[x][y]`
- **Dynamic entities**: Accessed via `m_pMapData->m_pData[x][y]`
- **Viewport position**: `m_pMapData->m_sPivotX/Y`
- **Frame timing**: `m_pMapData->m_stFrame[type][action]`

### Sprite System Integration

- Tile sprites stored in `m_pTileSpr[DEF_MAXTILES]` array
- Entity sprites in `m_pSprite[DEF_MAXSPRITES]` array
- Rendering via `CSprite::Put*()` methods

### Network Integration

Server packets update tile data:
- `bSetOwner()` - Add/update entity on tile
- `bSetDeadOwner()` - Add corpse
- `bSetItem()` - Add ground item
- `bSetDynamicObject()` - Add interactive object
- `bGetOwner()` - Query entity info
- `bGetIsLocateable()` - Check walkability

### Input Integration

- Mouse position (`msX`, `msY`) passed to `DrawObjects()`
- Collision detection against sprite bounds
- Item hover highlighting
- Entity selection

---

## Known Issues / Technical Debt

### Hard-coded Values
- Tile size (32 pixels) hard-coded throughout rendering loops
- Screen resolution (640x480) fixed
- Viewport offsets (-16, +32, etc.) magic numbers scattered in code

### Memory Inefficiency
- `m_tile[752][752]` always fully allocated (~5.6 MB)
- No sparse map support for smaller maps
- `m_pData[40][35]` static array even when smaller viewport would suffice

### Coordinate Confusion
- World coordinates (`indexX/Y`)
- Screen coordinates (`ix/iy`)
- Array indices (`dX/dY`)
- No clear transformation functions

### Animation Rigidity
- Frame timing hard-coded per character type
- Adding new character types requires code changes
- No data-driven animation configuration

### Rendering Limitations
- No sprite batching (each sprite is a separate draw call)
- No dirty rectangle optimization for objects
- Full object layer redrawn every frame
- PDBS optimization only helps terrain

---

## Modernization Notes

### Target Architecture

```cpp
namespace hb::world {
    struct tile {
        uint16_t terrain_id;
        uint16_t terrain_frame;
        uint16_t object_id;
        uint16_t object_frame;
        tile_flags flags;  // walkable, teleport, etc.
        uint8_t light_level;
    };

    class map {
        std::vector<tile> tiles_;
        int32_t width_, height_;

    public:
        const tile& get_tile(int32_t x, int32_t y) const;
        void set_tile(int32_t x, int32_t y, const tile& t);
        bool is_walkable(int32_t x, int32_t y) const;
    };

    class map_renderer {
    public:
        void render(renderer& r, const map& m, const viewport& vp);

    private:
        void render_terrain_layer(renderer& r, const map& m, const viewport& vp);
        void render_object_layer(renderer& r, const map& m, const viewport& vp);
    };
}
```

### Key Improvements

1. **Configurable tile size** - No hard-coded 32 pixel values
2. **Dynamic map allocation** - `std::vector` instead of fixed arrays
3. **Clear coordinate transforms** - Explicit world/screen/tile conversion
4. **Data-driven animations** - Load frame timing from configuration
5. **Sprite batching** - Collect draw calls, render in single pass
6. **Modern rendering API** - Abstract away DirectDraw specifics

### Migration Priority

1. Extract tile data structures (low risk)
2. Create coordinate transformation utilities
3. Implement modern map class
4. Abstract rendering behind interface
5. Port rendering logic incrementally
