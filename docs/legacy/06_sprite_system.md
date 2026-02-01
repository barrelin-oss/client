# Legacy Sprite System Documentation

## Overview

The sprite system is the core rendering mechanism for all 2D graphics in the Helbreath client. It handles loading sprite data from PAK files, managing DirectDraw surfaces, and providing numerous rendering methods with various blending and transparency effects.

**Primary Files:**
- `Sprite.h` / `Sprite.cpp` - Main sprite class (~3,260 lines)
- `SpriteID.h` - Sprite index constants and pivot points
- `TileSpr.h` / `TileSpr.cpp` - Tile sprite data container
- `Tile.h` / `Tile.cpp` - Tile state with sprite references
- `Mydib.h` / `Mydib.cpp` - BMP image loader for sprite surfaces
- `DXC_ddraw.h` / `DXC_ddraw.cpp` - DirectDraw wrapper (rendering backend)

---

## Architecture

### Class Hierarchy

```
CSprite                    Main sprite class - handles loading, surfaces, rendering
  |
  +-- Uses: DXC_ddraw      DirectDraw 7 wrapper for surface management
  +-- Uses: CMyDib         BMP image loader from PAK files
  +-- Uses: stBrush        Frame metadata structure

CTileSpr                   Tile sprite data container (references to sprite indices)
CTile                      Runtime tile state including sprite references
```

### Sprite Storage in CGame

```cpp
// Game.h - Sprite arrays
#define DEF_MAXSPRITES      20000   // Maximum general sprites
#define DEF_MAXTILES        500     // Maximum tile sprites
#define DEF_MAXEFFECTSPR    100     // Maximum effect sprites

class CSprite * m_pSprite[DEF_MAXSPRITES];      // General sprites (UI, items, characters, etc.)
class CSprite * m_pTileSpr[DEF_MAXTILES];       // Map tile sprites
class CSprite * m_pEffectSpr[DEF_MAXEFFECTSPR]; // Visual effect sprites
```

---

## CSprite Class

### Data Structures

#### stBrush - Frame Metadata Structure

```cpp
typedef struct stBrushtag {
    short sx;    // Source X position in sprite sheet
    short sy;    // Source Y position in sprite sheet
    short szx;   // Frame width in pixels
    short szy;   // Frame height in pixels
    short pvx;   // Pivot X offset (horizontal anchor point)
    short pvy;   // Pivot Y offset (vertical anchor point)
} stBrush;
```

The pivot point defines where the sprite is anchored relative to the drawing position. For characters, this is typically at their feet. For items, it may be centered.

### Member Variables

```cpp
class CSprite {
public:
    // Memory Management - Custom allocators using Windows Heap
    void * operator new (size_t size);    // HeapAlloc with HEAP_ZERO_MEMORY
    void operator delete(void * mem);      // HeapFree with HEAP_NO_SERIALIZE

    // Frame and Animation Data
    int         m_iTotalFrame;             // Total number of frames in this sprite
    stBrush*    m_stBrush;                 // Array of frame metadata (size = m_iTotalFrame)

    // DirectDraw Surface
    LPDIRECTDRAWSURFACE7 m_lpSurface;      // The DirectDraw surface containing sprite bitmap
    bool        m_bIsSurfaceEmpty;         // TRUE if surface not yet loaded (lazy loading)
    WORD*       m_pSurfaceAddr;            // Pointer to locked surface pixel data
    short       m_sPitch;                  // Surface pitch in WORDs (stride / 2)

    // Bitmap Information
    WORD        m_wBitmapSizeX;            // Sprite sheet width
    WORD        m_wBitmapSizeY;            // Sprite sheet height
    WORD        m_wColorKey;               // Transparent color (magenta typically)
    DWORD       m_dwBitmapFileStartLoc;    // Byte offset in PAK file to bitmap data

    // PAK File Reference
    char        m_cPakFileName[16];        // PAK filename (without path/extension)

    // Rendering State
    RECT        m_rcBound;                 // Last rendered bounding rectangle
    short       m_sPivotX, m_sPivotY;      // Cached pivot for last frame
    DWORD       m_dwRefTime;               // Last access time (for surface eviction)

    // Alpha/Transparency
    char        m_cAlphaDegree;            // Current alpha degree (1 or 2)
    bool        m_bAlphaEffect;            // Whether alpha effects are enabled
    bool        m_bOnCriticalSection;      // Thread safety flag (unused in practice)

    // DirectDraw Context
    class DXC_ddraw * m_pDDraw;            // Reference to DirectDraw wrapper
};
```

### Constructor

```cpp
CSprite::CSprite(
    HANDLE hPakFile,           // Open file handle to .pak file
    DXC_ddraw *pDDraw,         // DirectDraw context
    char *cPakFileName,        // PAK filename (without extension)
    short sNthFile,            // Index of sprite within PAK (0-based)
    bool bAlphaEffect = TRUE   // Enable alpha effect processing
);
```

**Constructor Flow:**
1. Initialize all member variables to safe defaults
2. Read sprite header from PAK file:
   - Seek to offset `24 + sNthFile * 8` to find ASD (sprite data) location
   - Read ASD start offset
3. Seek to `ASD + 100` and read total frame count
4. Calculate bitmap data offset: `ASD + 108 + (12 * frameCount)`
5. Allocate and read frame metadata array (`stBrush[m_iTotalFrame]`)
6. Store PAK filename for lazy loading
7. **Note:** Bitmap is NOT loaded at construction time (lazy loading)

### Destructor

```cpp
CSprite::~CSprite() {
    if (m_stBrush != NULL) delete[] m_stBrush;
    if (m_lpSurface != NULL) m_lpSurface->Release();
}
```

---

## Lazy Loading System

Sprites use lazy loading to conserve video memory. The bitmap data is only loaded when the sprite is first rendered.

### _iOpenSprite() - Load Sprite Surface

```cpp
bool CSprite::_iOpenSprite();
```

**Behavior:**
1. Check if surface already exists (return FALSE if so)
2. Create DirectDraw surface via `_pMakeSpriteSurface()`
3. Set color key for transparency
4. Lock surface to get pixel address and pitch
5. Apply alpha degree adjustments if needed
6. Mark surface as loaded (`m_bIsSurfaceEmpty = FALSE`)

### _iCloseSprite() - Release Sprite Surface

```cpp
void CSprite::_iCloseSprite();
```

**Behavior:**
1. Validate state (return if brush/surface is NULL)
2. Check surface is not lost
3. Release DirectDraw surface
4. Reset state flags

### _pMakeSpriteSurface() - Create Surface from PAK

```cpp
IDirectDrawSurface7 * CSprite::_pMakeSpriteSurface();
```

**Behavior:**
1. Create `CMyDib` to load BMP from PAK file at stored offset
2. Create off-screen DirectDraw surface of bitmap dimensions
3. Get device context and paint DIB to surface
4. Lock surface briefly to read color key from pixel (0,0)
5. Return the created surface

### iRestore() - Restore Lost Surface

```cpp
void CSprite::iRestore();
```

Called when DirectDraw surfaces are lost (e.g., Alt-Tab). Restores the surface and reloads bitmap data.

---

## Rendering Methods

### Rendering Method Categories

| Category | Methods | Description |
|----------|---------|-------------|
| **Fast Blit** | `PutSpriteFast*` | Hardware-accelerated BltFast with color key |
| **Transparent** | `PutTransSprite*` | Software alpha blending (100%, 70%, 50%, 25%, 2%) |
| **Shadow** | `PutShadowSprite*` | Darkened projection rendering |
| **RGB Tint** | `PutSpriteRGB*` | Color channel adjustment |
| **Fade** | `PutFadeSprite*` | Darkening effect |
| **Reverse Trans** | `PutRevTransSprite` | Reverse alpha blend with depth |
| **Shifted** | `PutShiftSprite*` | Offset within sprite sheet (for scrolling) |

### Fast Blit Methods

#### PutSpriteFast

```cpp
void CSprite::PutSpriteFast(int sX, int sY, int sFrame, DWORD dwTime);
```

**Parameters:**
- `sX, sY` - Screen position to draw at
- `sFrame` - Frame index to render (0 to m_iTotalFrame-1)
- `dwTime` - Current time (for surface reference tracking)

**Behavior:**
1. Validate frame index and brush data
2. Get frame metadata (source rect, size, pivot)
3. Calculate destination position: `dX = sX + pvx`, `dY = sY + pvy`
4. Clip against `m_pDDraw->m_rcClipArea`
5. Lazy load surface if needed
6. Check/apply alpha degree changes
7. Use DirectDraw `BltFast` with `DDBLTFAST_SRCCOLORKEY`
8. Store bounding rect in `m_rcBound`

#### Variants

```cpp
// Render to front buffer (immediate display)
void PutSpriteFastFrontBuffer(int sX, int sY, int sFrame, DWORD dwTime);

// Render to specific destination surface
void PutSpriteFastDst(LPDIRECTDRAWSURFACE7 lpDstS, int sX, int sY, int sFrame, DWORD dwTime);

// Render without color key (opaque)
void PutSpriteFastNoColorKey(int sX, int sY, int sFrame, DWORD dwTime);
void PutSpriteFastNoColorKeyDst(LPDIRECTDRAWSURFACE7 lpDstS, int sX, int sY, int sFrame, DWORD dwTime);

// Render with width limit (for progress bars)
void PutSpriteFastWidth(int sX, int sY, int sFrame, int sWidth, DWORD dwTime);
```

### Transparency Methods

All transparency methods perform **software pixel blending** using pre-computed lookup tables.

#### Transparency Levels

| Method | Alpha | Visual Effect |
|--------|-------|---------------|
| `PutTransSprite` | 100% | Full sprite color (overlay blend) |
| `PutTransSprite70` | 70% | 70% sprite, 30% background |
| `PutTransSprite50` | 50% | Equal blend |
| `PutTransSprite25` | 25% | 25% sprite, 75% background |
| `PutTransSprite2` | ~2% | Near-invisible glow |

#### PutTransSprite (100% Alpha Blend)

```cpp
void CSprite::PutTransSprite(int sX, int sY, int sFrame, DWORD dwTime, int alphaDepth = 30);
```

**Pixel Format Handling:**
```cpp
switch (m_pDDraw->m_cPixelFormat) {
case 1:  // RGB565 (16-bit, 5-6-5)
    // Red: bits 15-11 (5 bits, 0-31)
    // Green: bits 10-5 (6 bits, 0-63)
    // Blue: bits 4-0 (5 bits, 0-31)
    pDst[ix] = (WORD)(
        (G_lTransRB100[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) |
        (G_lTransG100[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) |
        G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]
    );
    break;

case 2:  // RGB555 (16-bit, 5-5-5)
    // Red: bits 14-10 (5 bits, 0-31)
    // Green: bits 9-5 (5 bits, 0-31)
    // Blue: bits 4-0 (5 bits, 0-31)
    pDst[ix] = (WORD)(
        (G_lTransRB100[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) |
        (G_lTransG100[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) |
        G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]
    );
    break;
}
```

#### NoColorKey Variants

Methods ending in `_NoColorKey` blend ALL pixels, including the transparent color:
```cpp
void PutTransSprite_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime, int alphaDepth = 0);
void PutTransSprite70_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime);
void PutTransSprite50_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime);
void PutTransSprite25_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime);
```

### Shadow Rendering

#### PutShadowSprite

```cpp
void CSprite::PutShadowSprite(int sX, int sY, int sFrame, DWORD dwTime);
```

Renders a flattened, darkened projection of the sprite. Used for character/object shadows.

**Algorithm:**
1. Sample every 3rd row of sprite (szy / 3 iterations)
2. For each non-transparent pixel:
   - Calculate shadow position with perspective skew
   - Darken destination pixel by right-shifting and masking:
     - RGB565: `pDst = (pDst & 0xE79C) >> 2`
     - RGB555: `pDst = (pDst & 0x739C) >> 2`

#### PutShadowSpriteClip

```cpp
void CSprite::PutShadowSpriteClip(int sX, int sY, int sFrame, DWORD dwTime);
```

Same as `PutShadowSprite` but with proper clipping against screen boundaries.

### RGB Tinting Methods

#### PutSpriteRGB

```cpp
void CSprite::PutSpriteRGB(int sX, int sY, int sFrame,
                            int sRed, int sGreen, int sBlue, DWORD dwTime);
```

Renders sprite with per-channel color adjustment. Used for equipment coloring, status effects, etc.

**Parameters:**
- `sRed, sGreen, sBlue` - Color adjustments (-255 to +255)

**Algorithm:**
```cpp
// Pre-compute offset indices
iRedPlus255   = sRed + 255;
iGreenPlus255 = sGreen + 255;
iBluePlus255  = sBlue + 255;

// For each pixel (RGB565 example):
pDst[ix] = (WORD)(
    (G_iAddTable31[(pSrc[ix]&0xF800)>>11][iRedPlus255] << 11) |
    (G_iAddTable63[(pSrc[ix]&0x7E0)>>5][iGreenPlus255] << 5) |
    G_iAddTable31[(pSrc[ix]&0x1F)][iBluePlus255]
);
```

#### PutTransSpriteRGB

```cpp
void CSprite::PutTransSpriteRGB(int sX, int sY, int sFrame,
                                 int sRed, int sGreen, int sBlue, DWORD dwTime);
```

Combines transparency blending with RGB tinting. Used for glowing effects, fire, magic, etc.

### Fade Effect

#### PutFadeSprite

```cpp
void CSprite::PutFadeSprite(short sX, short sY, short sFrame, DWORD dwTime);
```

Darkens the background where the sprite is rendered (silhouette effect).

**Algorithm:**
```cpp
// Darken destination by 75% (shift right 2, mask out overflow bits)
// RGB565:
if (pSrc[ix] != m_wColorKey)
    pDst[ix] = ((pDst[ix] & 0xE79C) >> 2);

// RGB555:
if (pSrc[ix] != m_wColorKey)
    pDst[ix] = ((pDst[ix] & 0x739C) >> 2);
```

#### PutFadeSpriteDst

```cpp
void CSprite::PutFadeSpriteDst(WORD * pDstAddr, short sPitch,
                                short sX, short sY, short sFrame, DWORD dwTime);
```

Same as `PutFadeSprite` but renders to arbitrary pixel buffer.

### Reverse Transparency

#### PutRevTransSprite

```cpp
void CSprite::PutRevTransSprite(int sX, int sY, int sFrame, DWORD dwTime, int alphaDepth = 0);
```

Reverse alpha blend using fade tables. The `alphaDepth` parameter controls blend intensity.

**Uses:** `m_pDDraw->m_lFadeRB` and `m_pDDraw->m_lFadeG` lookup tables.

### Shifted Sprite Methods

For rendering portions of large sprite sheets (e.g., scrolling backgrounds):

```cpp
// Fast blit with source offset (fixed 128x128 size)
void PutShiftSpriteFast(int sX, int sY, int shX, int shY, int sFrame, DWORD dwTime);

// Transparency blend with source offset (fixed 128x128 size)
void PutShiftTransSprite2(int sX, int sY, int shX, int shY, int sFrame, DWORD dwTime);
```

**Note:** These methods use a hardcoded 128x128 pixel size regardless of frame metadata.

---

## Utility Methods

### _GetSpriteRect

```cpp
void CSprite::_GetSpriteRect(int sX, int sY, int sFrame);
```

Calculates the screen bounding rectangle for a frame without rendering. Stores result in `m_rcBound` and pivot in `m_sPivotX`/`m_sPivotY`.

### _bCheckCollision

```cpp
BOOL CSprite::_bCheckCollison(int sX, int sY, short sFrame, int msX, int msY);
```

Pixel-perfect collision detection between mouse position and sprite.

**Algorithm:**
1. Calculate frame bounds
2. Early-exit if mouse outside frame rect
3. Check 7x7 pixel area centered on mouse position
4. Return TRUE if any non-transparent pixel found

**Usage:** Used for clicking on characters, items, and interactive objects.

### _SetAlphaDegree

```cpp
void CSprite::_SetAlphaDegree();
```

Applies global alpha degree adjustment to loaded sprite surface.

**Alpha Degrees:**
- Degree 1: No adjustment (sRed=0, sGreen=0, sBlue=0)
- Degree 2: Night/indoor effect (sRed=-3, sGreen=-3, sBlue=+2)

Modifies the actual surface pixels in-place when alpha degree changes.

---

## Lookup Tables

### Global Blending Tables

Defined as extern in `Sprite.cpp`:

```cpp
// Addition tables for RGB channel adjustment
extern int G_iAddTable31[64][510];     // 5-bit channels (0-31)
extern int G_iAddTable63[64][510];     // 6-bit channels (0-63)
extern int G_iAddTransTable31[510][64];
extern int G_iAddTransTable63[510][64];

// Transparency blend tables at different alpha levels
extern long G_lTransG100[64][64], G_lTransRB100[64][64];  // 100% blend
extern long G_lTransG70[64][64], G_lTransRB70[64][64];    // 70% blend
extern long G_lTransG50[64][64], G_lTransRB50[64][64];    // 50% blend
extern long G_lTransG25[64][64], G_lTransRB25[64][64];    // 25% blend
extern long G_lTransG2[64][64], G_lTransRB2[64][64];      // 2% blend
```

### DXC_ddraw Instance Tables

```cpp
class DXC_ddraw {
    // Blending tables (instance-specific, same as globals)
    long m_lTransG100[64][64], m_lTransRB100[64][64];
    long m_lTransG70[64][64], m_lTransRB70[64][64];
    long m_lTransG50[64][64], m_lTransRB50[64][64];
    long m_lTransG25[64][64], m_lTransRB25[64][64];
    long m_lTransG2[64][64], m_lTransRB2[64][64];

    // Fade tables for reverse transparency
    long m_lFadeG[64][64], m_lFadeRB[64][64];
};
```

---

## Pixel Formats

The sprite system supports two 16-bit pixel formats:

### Format 1: RGB565 (Default)

```
Bit layout: RRRRRGGGGGGBBBBB
- Red:   5 bits (0-31), bits 15-11, mask 0xF800
- Green: 6 bits (0-63), bits 10-5,  mask 0x07E0
- Blue:  5 bits (0-31), bits 4-0,   mask 0x001F
```

### Format 2: RGB555

```
Bit layout: XRRRRRGGGGGBBBBB
- X:     1 bit (unused), bit 15
- Red:   5 bits (0-31), bits 14-10, mask 0x7C00
- Green: 5 bits (0-31), bits 9-5,   mask 0x03E0
- Blue:  5 bits (0-31), bits 4-0,   mask 0x001F
```

Format detection happens in `DXC_ddraw::_TestPixelFormat()` and is stored in `m_cPixelFormat`.

---

## CTileSpr Class

Simple container for tile sprite references:

```cpp
class CTileSpr {
public:
    CTileSpr();
    virtual ~CTileSpr();

    short m_sTileSprite;        // Index into m_pTileSpr array
    short m_sTileSpriteFrame;   // Frame number for tile
    short m_sObjectSprite;      // Index for object on tile
    short m_sObjectSpriteFrame; // Frame number for object
    bool  m_bIsMoveAllowed;     // Can characters walk here
    bool  m_bIsTeleport;        // Is this a teleport tile
};
```

**Default Values:**
```cpp
CTileSpr::CTileSpr() {
    m_sTileSprite = 0;
    m_sTileSpriteFrame = 0;
    m_sObjectSprite = 0;
    m_sObjectSpriteFrame = 0;
    m_bIsMoveAllowed = TRUE;
    m_bIsTeleport = FALSE;
}
```

---

## CTile Class

Runtime tile state for the visible map area:

```cpp
class CTile {
public:
    // Timing
    DWORD m_dwOwnerTime;           // Time entity entered tile
    DWORD m_dwEffectTime;          // Effect animation time
    DWORD m_dwDeadOwnerTime;       // Time dead entity placed
    DWORD m_dwDynamicObjectTime;   // Dynamic object time

    // Dropped Items
    short m_sItemSprite;           // Item sprite index
    short m_sItemSpriteFrame;      // Item sprite frame
    int   m_cItemColor;            // Item color tint

    // Dynamic Objects (fire, fish, minerals, etc.)
    short m_sDynamicObjectType;
    char  m_cDynamicObjectFrame;
    char  m_cDynamicObjectData1;
    char  m_cDynamicObjectData2;
    char  m_cDynamicObjectData3;
    char  m_cDynamicObjectData4;

    // Tile Effects
    int   m_iEffectType;
    int   m_iEffectFrame;
    int   m_iEffectTotalFrame;

    // Living Entity
    short m_sOwnerType;            // Entity type ID
    char  m_cOwnerAction;          // Current action state
    char  m_cOwnerFrame;           // Animation frame
    char  m_cDir;                  // Facing direction
    short m_sAppr1, m_sAppr2;      // Appearance data
    short m_sAppr3, m_sAppr4;
    int   m_iApprColor;            // Appearance color
    short m_sStatus;               // Status flags
    WORD  m_wObjectID;             // Unique entity ID
    char  m_cOwnerName[12];        // Entity name
    int   m_iChatMsg;              // Chat message index

    // Dead Entity (corpse)
    short m_sDeadOwnerType;
    char  m_cDeadOwnerFrame;       // -1 = no corpse, 0-8 = decay frame
    char  m_cDeadDir;
    // ... similar fields for dead entity

    void Clear();  // Reset all fields
};
```

---

## CMyDib Class

BMP loader for extracting sprite bitmaps from PAK files:

```cpp
class CMyDib {
public:
    CMyDib(char *szFilename, unsigned long dwFilePointer);
    ~CMyDib();
    void PaintImage(HDC hDC);

    WORD m_wWidthX;          // Bitmap width
    WORD m_wWidthY;          // Bitmap height
    WORD m_wColorNums;       // Color table entries (0, 2, 16, or 256)
    LPSTR m_lpDib;           // Raw DIB data
    LPBITMAPINFO m_bmpInfo;  // Bitmap header info
};
```

**Constructor Flow:**
```cpp
CMyDib::CMyDib(char *szFilename, unsigned long dwFilePointer) {
    // Build path: "sprites\\{filename}.pak"
    char PathName[28];
    wsprintf(PathName, "sprites\\%s.pak", szFilename);

    // Open file and seek to bitmap
    HANDLE hFileRead = CreateFile(PathName, GENERIC_READ, ...);
    SetFilePointer(hFileRead, dwFilePointer, NULL, FILE_BEGIN);

    // Read BMP file header (14 bytes)
    BITMAPFILEHEADER fh;
    ReadFile(hFileRead, &fh, 14, ...);

    // Allocate and read DIB data
    m_lpDib = new char[fh.bfSize - 14];
    ReadFile(hFileRead, m_lpDib, fh.bfSize - 14, ...);

    // Extract dimensions from info header
    LPBITMAPINFOHEADER bmpInfoHeader = (LPBITMAPINFOHEADER)m_lpDib;
    m_wWidthX = bmpInfoHeader->biWidth;
    m_wWidthY = bmpInfoHeader->biHeight;

    // Determine color count based on bit depth
    // 24-bit: 0 colors, 8-bit: 256, 4-bit: 16, 1-bit: 2
}
```

---

## Sprite ID Constants (SpriteID.h)

### Interface Sprites

```cpp
#define DEF_SPRID_MOUSECURSOR               0

// Fonts
#define DEF_SPRID_INTERFACE_SPRFONTS        22
#define DEF_SPRID_INTERFACE_SPRFONTS2       28
#define DEF_SPRID_INTERFACE_FONT1           30
#define DEF_SPRID_INTERFACE_FONT2           31

// UI Dialogs
#define DEF_SPRID_INTERFACE_ND_LOADING      51
#define DEF_SPRID_INTERFACE_ND_MAINMENU     52
#define DEF_SPRID_INTERFACE_ND_LOGIN        53
#define DEF_SPRID_INTERFACE_ND_NEWACCOUNT   54
#define DEF_SPRID_INTERFACE_ND_QUIT         55
#define DEF_SPRID_INTERFACE_ND_AGREEMENT    56
#define DEF_SPRID_INTERFACE_ND_SELECTCHAR   57
#define DEF_SPRID_INTERFACE_ND_NEWCHAR      58
#define DEF_SPRID_INTERFACE_ND_NEWEXCHANGE  59
#define DEF_SPRID_INTERFACE_ND_GAME1        60
#define DEF_SPRID_INTERFACE_ND_GAME2        61
#define DEF_SPRID_INTERFACE_ND_GAME3        62
#define DEF_SPRID_INTERFACE_ND_GAME4        63
#define DEF_SPRID_INTERFACE_ND_ICONPANNEL   64
#define DEF_SPRID_INTERFACE_ND_INVENTORY    67
#define DEF_SPRID_INTERFACE_ND_TEXT         70
#define DEF_SPRID_INTERFACE_ND_BUTTON       71
#define DEF_SPRID_INTERFACE_ND_CRUSADE      72
#define DEF_SPRID_INTERFACE_GUIDEMAP        73

// Maps
#define DEF_SPRID_INTERFACE_NEWMAPS1        35
#define DEF_SPRID_INTERFACE_NEWMAPS2        36
#define DEF_SPRID_INTERFACE_NEWMAPS3        37
#define DEF_SPRID_INTERFACE_NEWMAPS4        38
#define DEF_SPRID_INTERFACE_NEWMAPS5        39
```

### Item Sprite Pivot Points

```cpp
#define DEF_SPRID_ITEMGROUND_PIVOTPOINT     100  // Items on ground
#define DEF_SPRID_ITEMEQUIP_PIVOTPOINT      200  // Equipped items
#define DEF_SPRID_ITEMPACK_PIVOTPOINT       300  // Inventory items
#define DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT    400  // Dynamic items
```

These are base indices. The actual sprite index is calculated as:
`base + itemType`

---

## Usage Patterns

### Loading Sprites

```cpp
// Open PAK file
m_hPakFile = CreateFile("sprites\\New-Dialog.pak", GENERIC_READ, NULL, NULL,
                         OPEN_EXISTING, NULL, NULL);

// Create sprite from PAK (index 0 = first sprite in PAK)
m_pSprite[DEF_SPRID_INTERFACE_ND_LOADING] = new class CSprite(
    m_hPakFile,      // File handle
    &m_DDraw,        // DirectDraw context
    "New-Dialog",    // PAK name (for lazy reload)
    0,               // Sprite index in PAK
    FALSE            // Disable alpha effects
);

// Close file handle (sprite keeps PAK name for lazy loading)
CloseHandle(m_hPakFile);
```

### Helper Functions in CGame

```cpp
// Load multiple sprites from a PAK file
void CGame::_LoadSprite(short sSprite, char *FileName, int sStart, short sCount);

// Load tile sprites
void CGame::_LoadTileSpr(short sSprite, char *FileName, int sStart, short sCount);

// Load effect sprites
void CGame::_LoadEffectSpr(short sSprite, char *FileName, int sStart, short sCount);
```

### Rendering Examples

```cpp
DWORD dwTime = timeGetTime();

// Simple UI sprite
m_pSprite[DEF_SPRID_MOUSECURSOR]->PutSpriteFast(msX, msY, 0, dwTime);

// Item on ground with color tint
m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + sItemSprite]->PutSpriteRGB(
    ix, iy, sItemSpriteFrame,
    m_wR[cItemColor] - m_wR[0],  // Red adjustment
    m_wG[cItemColor] - m_wG[0],  // Green adjustment
    m_wB[cItemColor] - m_wB[0],  // Blue adjustment
    dwTime
);

// Character shadow
m_pTileSpr[sObjSpr]->PutShadowSprite(ix - 16, iy - 16, sObjSprFrame, dwTime);

// Transparent effect (fire glow)
m_pEffectSpr[23]->PutTransSprite50_NoColorKey(
    ix + (rand() % 2),  // Slight random offset
    iy + (rand() % 2),
    sDynamicObjectFrame,
    dwTime
);

// Magic effect with RGB tint
m_pEffectSpr[0]->PutTransSpriteRGB(
    ix, iy, 1,
    iDvalue,    // Dynamic brightness
    iDvalue,
    iDvalue,
    dwTime
);
```

---

## Memory Management

### Surface Lifecycle

1. **Construction:** Only frame metadata loaded, surface is NULL
2. **First Render:** Surface created via lazy loading
3. **Subsequent Renders:** Surface reused, `m_dwRefTime` updated
4. **Alt-Tab:** Surfaces lost, `iRestore()` called on next render
5. **Destruction:** Surface released in destructor

### Eviction Strategy

The `m_dwRefTime` member tracks last access time. While the legacy code doesn't implement automatic eviction, this timestamp could be used to release unused surfaces under memory pressure.

### Thread Safety

The `m_bOnCriticalSection` flag exists but is not used with actual synchronization primitives. The rendering system is effectively single-threaded.

---

## Performance Considerations

### Software Blending Bottleneck

All transparency and RGB effects are software-rendered pixel-by-pixel. This was acceptable for 2002-era 800x600 displays but scales poorly.

### Lookup Table Optimization

Pre-computed blend tables avoid per-pixel arithmetic:
- Color channel extraction: bitwise AND + shift
- Blend calculation: single table lookup
- Result assembly: shifts + OR

### Clipping Optimization

All rendering methods perform manual clipping against `m_rcClipArea` before any pixel operations, avoiding partial writes.

---

## Modernization Notes

### Direct Replacement Candidates

| Legacy | Modern Equivalent |
|--------|-------------------|
| `LPDIRECTDRAWSURFACE7` | Texture/Sprite objects |
| Manual pixel blending | GPU alpha blending |
| Color key transparency | Alpha channel |
| Lookup tables | Shader operations |
| `stBrush` frame data | Sprite atlas + UV coordinates |
| `CMyDib` BMP loading | Image library (stb_image, SDL_image) |

### Architectural Changes

1. **Sprite Atlas:** Combine related sprites into texture atlases
2. **Batch Rendering:** Group draw calls by texture/blend state
3. **GPU Blending:** Use hardware alpha blending modes
4. **Async Loading:** Load sprites on background thread
5. **Reference Counting:** Proper shared ownership of textures

### Preserved Behaviors

- Frame metadata format (stBrush) for animation compatibility
- PAK file format for asset compatibility
- Color key value at (0,0) convention
- Pivot point semantics
- 5 alpha blend levels (100%, 70%, 50%, 25%, 2%)
