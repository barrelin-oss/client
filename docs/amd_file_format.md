# AMD Map File Format Specification

**Version:** 1.0
**Game:** Helbreath (circa 2002-2003)
**File Extension:** `.amd` (Assumed Map Data)

---

## Overview

The `.amd` file format is the binary map data format used by Helbreath to store tile-based world geometry. Each map in the game (cities, dungeons, wilderness areas) is stored as a separate `.amd` file containing static terrain information, walkability data, and teleport locations.

### Key Characteristics

| Property | Value |
|----------|-------|
| Header size | 256 bytes |
| Tile data size | 10 bytes per tile |
| Byte order | Little-endian |
| Maximum dimensions | 752 x 752 tiles (client) |
| Typical dimensions | Variable per map |

---

## File Structure

```
+------------------+
|     Header       |  256 bytes - Text-based configuration
+------------------+
|    Tile Data     |  (MapSizeX * MapSizeY * 10) bytes
+------------------+
```

---

## Header Format (256 bytes)

The header is a text-based configuration block containing key-value pairs. The format uses simple `KEY = VALUE` syntax with whitespace, commas, tabs, or newlines as delimiters.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `MAPSIZEX` | Integer | Map width in tiles |
| `MAPSIZEY` | Integer | Map height in tiles |

### Optional Fields (Server-side)

| Field | Type | Description |
|-------|------|-------------|
| `TILESIZE` | Integer | Bytes per tile record (typically 10) |

### Header Parsing

The header parser:
1. Reads exactly 256 bytes from file start
2. Replaces NULL bytes with spaces for tokenization
3. Tokenizes on `=`, `,`, `\t`, `\n`, and space characters
4. Extracts `MAPSIZEX` and `MAPSIZEY` values

### Example Header

```
MAPSIZEX = 752
MAPSIZEY = 752
TILESIZE = 10
```

**Note:** Remaining bytes in the 256-byte header block may contain padding, additional metadata, or be unused. The parser ignores unrecognized tokens.

---

## Tile Data Format

Tile data immediately follows the 256-byte header. Tiles are stored in **row-major order** (Y-axis outer loop, X-axis inner loop):

```cpp
for (int y = 0; y < MapSizeY; y++)
{
    for (int x = 0; x < MapSizeX; x++)
    {
        // Read 10-byte tile record
    }
}
```

### Tile Record Structure (10 bytes)

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | int16 | `TileSprite` | Ground terrain sprite ID |
| 2 | 2 | int16 | `TileSpriteFrame` | Ground terrain animation frame |
| 4 | 2 | int16 | `ObjectSprite` | Static object overlay sprite ID |
| 6 | 2 | int16 | `ObjectSpriteFrame` | Static object animation frame |
| 8 | 1 | uint8 | `Flags` | Tile property flags (see below) |
| 9 | 1 | uint8 | `Reserved` | Unused padding byte |

### C Structure Definition

```cpp
#pragma pack(push, 1)
struct amd_tile_record
{
    int16_t  tile_sprite;           // Ground terrain sprite ID
    int16_t  tile_sprite_frame;     // Ground terrain frame
    int16_t  object_sprite;         // Object overlay sprite ID
    int16_t  object_sprite_frame;   // Object overlay frame
    uint8_t  flags;                 // Property flags
    uint8_t  reserved;              // Padding (unused)
};
#pragma pack(pop)

static_assert(sizeof(amd_tile_record) == 10, "Tile record must be 10 bytes");
```

---

## Tile Flags (Byte 8)

The flags byte contains bitwise properties for tile behavior:

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 7 | `0x80` | `MOVE_BLOCKED` | Tile is not walkable (walls, water, obstacles) |
| 6 | `0x40` | `TELEPORT` | Tile triggers teleportation |
| 5 | `0x20` | `FARM` | Tile is farmable land (agriculture system) |
| 4-0 | `0x1F` | Reserved | Unused, should be 0 |

### Flag Interpretation

```cpp
bool is_blocked  = (flags & 0x80) != 0;
bool is_teleport = (flags & 0x40) != 0;
bool is_farm     = (flags & 0x20) != 0;  // Server-side only
```

### Client vs Server Parsing

**Client** reads flags into boolean fields:
```cpp
if ((flags & 0x80) != 0)
    tile.m_bIsMoveAllowed = FALSE;
else
    tile.m_bIsMoveAllowed = TRUE;

if ((flags & 0x40) != 0)
    tile.m_bIsTeleport = TRUE;
else
    tile.m_bIsTeleport = FALSE;
```

**Server** additionally checks for farm tiles and water detection:
```cpp
if ((flags & 0x80) != 0) tile.m_bIsMoveAllowed = FALSE;
if ((flags & 0x40) != 0) tile.m_bIsTeleport = TRUE;
if ((flags & 0x20) != 0) tile.m_bIsFarm = TRUE;

// Special case: Sprite ID 19 = Water
if (tile_sprite == 19)
    tile.m_bIsWater = TRUE;
```

---

## Sprite System

### Ground Terrain Sprites (`TileSprite`)

Ground sprites define the base terrain appearance:

| Sprite ID | Description |
|-----------|-------------|
| 0 | Default/empty |
| 19 | Water (special - triggers water flag on server) |
| Others | Various terrain types (grass, stone, sand, etc.) |

### Object Overlay Sprites (`ObjectSprite`)

Object sprites are rendered on top of ground terrain:

| Type | Description |
|------|-------------|
| 0 | No object overlay |
| > 0 | Static objects (trees, buildings, walls, decorations) |

### Sprite Frames

Both ground and object sprites can have multiple animation frames. The frame value selects which frame of a sprite sheet to display. A value of 0 typically indicates the default/first frame.

---

## Memory Layout

### Total File Size Calculation

```
FileSize = 256 + (MapSizeX * MapSizeY * 10) bytes
```

### Example Sizes

| Map Size | Tile Count | Data Size | Total File Size |
|----------|------------|-----------|-----------------|
| 100x100 | 10,000 | 100,000 bytes | ~97 KB |
| 256x256 | 65,536 | 655,360 bytes | ~640 KB |
| 512x512 | 262,144 | 2,621,440 bytes | ~2.5 MB |
| 752x752 | 565,504 | 5,655,040 bytes | ~5.4 MB |

---

## Client Memory Structure

The client loads the entire map into a 752x752 array regardless of actual map size:

```cpp
class CMapData
{
    // Static terrain (full map)
    CTileSpr m_tile[752][752];

    // Map dimensions (from header)
    short m_sMapSizeX, m_sMapSizeY;
};

class CTileSpr
{
    short m_sTileSprite;        // Ground sprite ID
    short m_sTileSpriteFrame;   // Ground frame
    short m_sObjectSprite;      // Object sprite ID
    short m_sObjectSpriteFrame; // Object frame
    bool  m_bIsMoveAllowed;     // Walkability
    bool  m_bIsTeleport;        // Teleport marker
};
```

---

## Server Memory Structure

The server uses a similar but extended tile structure:

```cpp
class CTile
{
    // Static properties (from .amd file)
    bool m_bIsMoveAllowed;      // Can walk here
    bool m_bIsTeleport;         // Is teleport location
    bool m_bIsWater;            // Is water tile
    bool m_bIsFarm;             // Is farmable
    bool m_bIsTempMoveAllowed;  // Runtime toggle (doors)

    // Dynamic properties (runtime only)
    short m_sOwner;             // Current occupant
    char  m_cOwnerClass;        // Owner type (player/NPC)
    // ... additional runtime state
};
```

---

## Coordinate Systems

### World Coordinates

- Origin: Top-left corner (0, 0)
- X-axis: Increases to the right
- Y-axis: Increases downward
- Unit: Tiles (32x32 pixels each)

### Array Indexing

**File storage:** Row-major, Y outer loop
```cpp
// File position calculation
file_offset = 256 + (y * map_size_x + x) * 10;
```

**Client array:** Standard 2D array
```cpp
tile = m_tile[x][y];
```

**Server array (unusual):** Uses Y as stride
```cpp
// Server accesses tiles with Y-stride formula
tile_ptr = m_pTile + x + y * m_sSizeY;
```

**Note:** The server uses an unusual indexing formula (`x + y * sizeY`) rather than standard row-major (`y * sizeX + x`). This is a legacy quirk.

---

## Teleportation Integration

Tiles marked with the `TELEPORT` flag (0x40) indicate teleport trigger locations. The actual teleport destinations are defined in separate `.txt` configuration files, not in the `.amd` file:

```
# Example from mapdata/aresden.txt
teleport-loc = 296 590 default 350 350 5
```

Format: `teleport-loc = srcX srcY destMap destX destY direction`

---

## Loading Sequence

### Client Loading

```cpp
void CMapData::OpenMapDataFile(char* filename)
{
    // 1. Open file
    HANDLE hFile = CreateFile(filename, ...);

    // 2. Read 256-byte header
    char header[256];
    ReadFile(hFile, header, 256, ...);

    // 3. Parse header for dimensions
    _bDecodeMapInfo(header);  // Sets m_sMapSizeX, m_sMapSizeY

    // 4. Allocate and read tile data
    char* mapData = new char[m_sMapSizeX * m_sMapSizeY * 10];
    ReadFile(hFile, mapData, m_sMapSizeX * m_sMapSizeY * 10, ...);

    // 5. Parse tile records
    char* cp = mapData;
    for (int y = 0; y < m_sMapSizeY; y++)
    {
        for (int x = 0; x < m_sMapSizeX; x++)
        {
            short* sp = (short*)cp;
            m_tile[x][y].m_sTileSprite        = sp[0];
            m_tile[x][y].m_sTileSpriteFrame   = sp[1];
            m_tile[x][y].m_sObjectSprite      = sp[2];
            m_tile[x][y].m_sObjectSpriteFrame = sp[3];

            uint8_t flags = cp[8];
            m_tile[x][y].m_bIsMoveAllowed = (flags & 0x80) == 0;
            m_tile[x][y].m_bIsTeleport    = (flags & 0x40) != 0;

            cp += 10;
        }
    }

    delete[] mapData;
    CloseHandle(hFile);
}
```

### Server Loading

```cpp
BOOL CMap::_bDecodeMapDataFileContents()
{
    // 1. Read header
    char header[256];
    ReadFile(hFile, header, 256, ...);

    // 2. Parse dimensions and tile size
    // MAPSIZEX, MAPSIZEY, TILESIZE

    // 3. Allocate tile array
    m_pTile = new CTile[m_sSizeX * m_sSizeY];

    // 4. Read and parse tiles
    for (int y = 0; y < m_sSizeY; y++)
    {
        for (int x = 0; x < m_sSizeX; x++)
        {
            char tileData[10];
            ReadFile(hFile, tileData, m_sTileDataSize, ...);

            CTile* pTile = m_pTile + x + y * m_sSizeY;

            // Movement flag (bit 7)
            pTile->m_bIsMoveAllowed = (tileData[8] & 0x80) == 0;

            // Teleport flag (bit 6)
            pTile->m_bIsTeleport = (tileData[8] & 0x40) != 0;

            // Farm flag (bit 5)
            pTile->m_bIsFarm = (tileData[8] & 0x20) != 0;

            // Water detection (sprite ID 19)
            short* sp = (short*)tileData;
            pTile->m_bIsWater = (*sp == 19);
        }
    }
}
```

---

## File Validation

When reading `.amd` files, validate:

1. **File size:** Must be at least 256 bytes (header)
2. **Header parsing:** `MAPSIZEX` and `MAPSIZEY` must be positive integers
3. **Data size:** File must contain `256 + (MapSizeX * MapSizeY * 10)` bytes
4. **Dimension limits:** Client expects max 752x752

### Validation Example

```cpp
bool validate_amd_file(const char* filename)
{
    // Check file size
    size_t file_size = get_file_size(filename);
    if (file_size < 256)
        return false;  // Header missing

    // Parse header
    int map_size_x, map_size_y;
    if (!parse_header(filename, &map_size_x, &map_size_y))
        return false;

    // Validate dimensions
    if (map_size_x <= 0 || map_size_x > 752)
        return false;
    if (map_size_y <= 0 || map_size_y > 752)
        return false;

    // Check data size
    size_t expected = 256 + (map_size_x * map_size_y * 10);
    if (file_size < expected)
        return false;

    return true;
}
```

---

## Map Editor Considerations

When creating or modifying `.amd` files:

1. **Header format:** Keep the text format simple with `KEY = VALUE` pairs
2. **Padding:** Zero-fill unused header bytes
3. **Tile order:** Write tiles in row-major order (Y outer, X inner)
4. **Flags consistency:** Ensure blocked tiles have proper collision sprites
5. **Teleports:** Mark teleport tiles in `.amd` and define destinations in `.txt`

---

## Related Files

Each map typically has associated configuration files:

| File | Purpose |
|------|---------|
| `mapdata/{name}.amd` | Binary tile geometry (this format) |
| `mapdata/{name}.txt` | Text configuration (NPCs, teleports, spawns) |

### Configuration File Keywords

The `.txt` file contains additional map configuration:

- `teleport-loc` - Teleport definitions
- `waypoint` - NPC movement waypoints
- `npc` - NPC spawn definitions
- `initial-point` - Player spawn points
- `noattack-rect` - Safe zones
- `smgr` - Spot mob generators
- `mgar` - Mob generation avoid rectangles
- `fish-point` - Fishing locations
- `mineral-point` - Mining locations

---

## Version History

| Version | Changes |
|---------|---------|
| 1.0 | Original Helbreath format (circa 2002) |

---

## Appendix A: Complete Tile Record Diagram

```
Byte offset:  0   1   2   3   4   5   6   7   8   9
            +---+---+---+---+---+---+---+---+---+---+
            |TileSprt |TileFrm |ObjSprt  |ObjFrm   |Flg|Rsv|
            +---+---+---+---+---+---+---+---+---+---+
            |<--16-->|<--16-->|<--16--->|<--16--->|8  |8  |

TileSprt  = Ground terrain sprite ID (int16, little-endian)
TileFrm   = Ground terrain frame (int16, little-endian)
ObjSprt   = Object overlay sprite ID (int16, little-endian)
ObjFrm    = Object overlay frame (int16, little-endian)
Flg       = Flags byte (bit 7=blocked, bit 6=teleport, bit 5=farm)
Rsv       = Reserved/padding byte
```

---

## Appendix B: Flags Byte Diagram

```
Bit:     7     6     5     4     3     2     1     0
       +-----+-----+-----+-----+-----+-----+-----+-----+
       |BLOCK|TELE |FARM | Rsv | Rsv | Rsv | Rsv | Rsv |
       +-----+-----+-----+-----+-----+-----+-----+-----+

BLOCK = 0x80 = Movement blocked (walls, obstacles)
TELE  = 0x40 = Teleport trigger location
FARM  = 0x20 = Farmable tile (server-side agriculture)
Rsv   = Reserved (must be 0)
```

---

## Appendix C: Example Hex Dump

A sample 10-byte tile record:

```
Offset: 00 01 02 03 04 05 06 07 08 09
Data:   05 00 00 00 0A 00 02 00 00 00

Parsed:
  TileSprite       = 0x0005 = 5    (grass terrain)
  TileSpriteFrame  = 0x0000 = 0    (first frame)
  ObjectSprite     = 0x000A = 10   (tree overlay)
  ObjectSpriteFrame= 0x0002 = 2    (third frame)
  Flags            = 0x00          (walkable, not teleport)
  Reserved         = 0x00          (unused)
```

Blocked tile with teleport:

```
Offset: 00 01 02 03 04 05 06 07 08 09
Data:   13 00 00 00 00 00 00 00 C0 00

Parsed:
  TileSprite       = 0x0013 = 19   (water - special!)
  TileSpriteFrame  = 0x0000 = 0
  ObjectSprite     = 0x0000 = 0    (no object)
  ObjectSpriteFrame= 0x0000 = 0
  Flags            = 0xC0          (blocked + teleport)
  Reserved         = 0x00
```
