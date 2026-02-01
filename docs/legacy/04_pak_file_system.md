# PAK File System - Legacy Documentation

## Overview

The PAK (Package) file system is Helbreath's proprietary archive format for storing and managing sprite assets. Each PAK file contains multiple sprite images (referred to as "ASD" data internally) along with metadata about each frame's position, size, and pivot points. The system was designed for efficient loading of 2D sprite data on late 1990s/early 2000s hardware.

---

## File Format Specification

### PAK File Structure

A PAK file is a binary archive with the following overall structure:

```
+------------------+
| PAK Header       |  (24 bytes minimum)
+------------------+
| Entry Offset     |  (8 bytes per entry)
| Table            |
+------------------+
| ASD Entry 0      |  (Variable size)
+------------------+
| ASD Entry 1      |  (Variable size)
+------------------+
| ...              |
+------------------+
| ASD Entry N      |  (Variable size)
+------------------+
```

### PAK Header (Bytes 0-23)

The PAK header occupies the first 24 bytes of the file:

| Offset | Size | Type    | Description                                    |
|--------|------|---------|------------------------------------------------|
| 0      | 20   | char[]  | Reserved/signature data                        |
| 20     | 4    | int32   | Total number of images (sprites) in this PAK   |

**Code Reference** (`Game.cpp:3550`):
```cpp
SetFilePointer(m_hPakFile, 20, NULL, FILE_BEGIN);
ReadFile(m_hPakFile, (char *)&iTotalimage, 4, &nCount, NULL);
```

### Entry Offset Table (Bytes 24+)

Starting at byte 24, there is an array of 8-byte entries, one for each sprite:

| Offset in Table | Size | Type   | Description                                |
|-----------------|------|--------|--------------------------------------------|
| 0               | 4    | int32  | Absolute file offset to ASD entry start    |
| 4               | 4    | int32  | Reserved/unused (possibly original size)   |

**Code Reference** (`Sprite.cpp:41-42`):
```cpp
SetFilePointer(hPakFile, 24 + sNthFile*8, NULL, FILE_BEGIN);
ReadFile(hPakFile, &iASDstart, 4, &nCount, NULL);
```

The formula to find the offset for sprite index `N` is:
```
offset_to_sprite_N = 24 + (N * 8)
```

### ASD Entry Structure (Animation/Sprite Data)

Each ASD entry contains metadata about a sprite followed by its bitmap data:

#### ASD Header

| Offset | Size              | Type         | Description                              |
|--------|-------------------|--------------|------------------------------------------|
| 0      | 100               | char[]       | Sprite confirmation/signature data       |
| 100    | 4                 | int32        | Total number of frames in this sprite    |
| 104    | 12 * frame_count  | stBrush[]    | Frame metadata array (brush data)        |
| 104 + (12 * frame_count) | variable | BMP data | Embedded Windows BMP bitmap data    |

**Code Reference** (`Sprite.cpp:44-52`):
```cpp
SetFilePointer(hPakFile, iASDstart+100, NULL, FILE_BEGIN);
ReadFile(hPakFile, &m_iTotalFrame, 4, &nCount, NULL);
m_dwBitmapFileStartLoc = iASDstart + (108 + (12*m_iTotalFrame));
m_stBrush = new stBrush[m_iTotalFrame];
ReadFile(hPakFile, m_stBrush, 12*m_iTotalFrame, &nCount, NULL);
```

### stBrush Structure (Frame Metadata)

Each frame in a sprite has 12 bytes of metadata:

```cpp
typedef struct stBrushtag {
    short sx;   // Source X position within the bitmap
    short sy;   // Source Y position within the bitmap
    short szx;  // Width of this frame
    short szy;  // Height of this frame
    short pvx;  // Pivot X offset (hotspot X)
    short pvy;  // Pivot Y offset (hotspot Y)
} stBrush;
```

| Field | Size | Type   | Description                                           |
|-------|------|--------|-------------------------------------------------------|
| sx    | 2    | int16  | X coordinate in sprite sheet where frame starts       |
| sy    | 2    | int16  | Y coordinate in sprite sheet where frame starts       |
| szx   | 2    | int16  | Width of the frame in pixels                          |
| szy   | 2    | int16  | Height of the frame in pixels                         |
| pvx   | 2    | int16  | Pivot/origin X offset for positioning                 |
| pvy   | 2    | int16  | Pivot/origin Y offset for positioning                 |

**Total: 12 bytes per frame**

The pivot points (pvx, pvy) define the "anchor" point for the sprite. When drawing a sprite at position (X, Y), the actual screen position is calculated as:
```cpp
dX = sX + pvx;
dY = sY + pvy;
```

### Embedded Bitmap Data

After the frame metadata array, the ASD entry contains a standard Windows BMP bitmap. This bitmap contains all frames packed into a single image (sprite sheet).

The bitmap uses:
- **BITMAPFILEHEADER** (14 bytes) - Standard BMP file header
- **BITMAPINFOHEADER** (40 bytes) - Bitmap info with dimensions and color depth
- **Color palette** (if applicable) - For 8-bit or lower color depths
- **Pixel data** - The actual image data

**Code Reference** (`Mydib.cpp:9-36`):
```cpp
CMyDib::CMyDib(char *szFilename, unsigned long dwFilePointer)
{
    // Opens PAK file and reads BMP data starting at dwFilePointer
    wsprintf(PathName, "sprites\\%s.pak", szFilename);
    hFileRead = CreateFile(PathName, GENERIC_READ, ...);
    SetFilePointer(hFileRead, dwFilePointer, NULL, FILE_BEGIN);
    ReadFile(hFileRead, (char *)&fh, 14, &nCount, NULL);  // BMP header
    m_lpDib = (LPSTR)new char[fh.bfSize-14];
    ReadFile(hFileRead, (char *)m_lpDib, fh.bfSize-14, &nCount, NULL);
    // ... extract dimensions, color depth, etc.
}
```

Supported bitmap formats:
- 24-bit color (no palette)
- 8-bit indexed color (256 color palette)
- 4-bit indexed color (16 color palette)
- 1-bit monochrome (2 color palette)

---

## Complete Binary Layout Example

For a PAK file containing 3 sprites:

```
Offset 0x0000:  [PAK Header - 20 bytes reserved]
Offset 0x0014:  [int32: 3]                    // Total images = 3
Offset 0x0018:  [int32: offset_to_ASD_0]      // Entry 0 offset
Offset 0x001C:  [int32: reserved]
Offset 0x0020:  [int32: offset_to_ASD_1]      // Entry 1 offset
Offset 0x0024:  [int32: reserved]
Offset 0x0028:  [int32: offset_to_ASD_2]      // Entry 2 offset
Offset 0x002C:  [int32: reserved]

// ASD Entry 0 starts here
Offset 0x0030:  [100 bytes: sprite signature/confirmation]
Offset 0x0094:  [int32: frame_count]          // e.g., 8 frames
Offset 0x0098:  [stBrush array: 12 * 8 = 96 bytes]
Offset 0x00F8:  [BMP data starts - BITMAPFILEHEADER]
                [BITMAPINFOHEADER]
                [Pixel data...]

// ASD Entry 1 starts at offset_to_ASD_1
// ...
```

---

## Loading Process

### Step 1: Open PAK File

```cpp
HANDLE m_hPakFile = CreateFile("sprites\\interface.pak",
                               GENERIC_READ, NULL, NULL,
                               OPEN_EXISTING, NULL, NULL);
```

### Step 2: Read Total Image Count

```cpp
int iTotalimage;
DWORD nCount;
SetFilePointer(m_hPakFile, 20, NULL, FILE_BEGIN);
ReadFile(m_hPakFile, (char *)&iTotalimage, 4, &nCount, NULL);
```

### Step 3: Create CSprite Objects

```cpp
for (short i = 0; i < sCount; i++) {
    if (i < iTotalimage) {
        m_pSprite[i+sStart] = new class CSprite(m_hPakFile, &m_DDraw,
                                                 FileName, i, bAlphaEffect);
    }
}
```

### Step 4: CSprite Constructor Reads Frame Data

```cpp
CSprite::CSprite(HANDLE hPakFile, DXC_ddraw *pDDraw,
                 char *cPakFileName, short sNthFile, bool bAlphaEffect)
{
    // Find ASD entry offset
    SetFilePointer(hPakFile, 24 + sNthFile*8, NULL, FILE_BEGIN);
    ReadFile(hPakFile, &iASDstart, 4, &nCount, NULL);

    // Read frame count
    SetFilePointer(hPakFile, iASDstart+100, NULL, FILE_BEGIN);
    ReadFile(hPakFile, &m_iTotalFrame, 4, &nCount, NULL);

    // Calculate bitmap location
    m_dwBitmapFileStartLoc = iASDstart + (108 + (12*m_iTotalFrame));

    // Read frame metadata
    m_stBrush = new stBrush[m_iTotalFrame];
    ReadFile(hPakFile, m_stBrush, 12*m_iTotalFrame, &nCount, NULL);

    // Store PAK filename for lazy bitmap loading
    memcpy(m_cPakFileName, cPakFileName, strlen(cPakFileName));
}
```

### Step 5: Lazy Bitmap Loading

The actual bitmap data is NOT loaded during construction. It's loaded on-demand when the sprite is first drawn:

```cpp
bool CSprite::_iOpenSprite()
{
    if (m_lpSurface != NULL) return FALSE;

    // Load bitmap and create DirectDraw surface
    m_lpSurface = _pMakeSpriteSurface();
    if (m_lpSurface == NULL) return FALSE;

    // Set color key for transparency
    m_pDDraw->iSetColorKey(m_lpSurface, m_wColorKey);
    m_bIsSurfaceEmpty = FALSE;

    // Lock surface to get memory address
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = 124;
    m_lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    m_pSurfaceAddr = (WORD *)ddsd.lpSurface;
    m_sPitch = (short)ddsd.lPitch >> 1;
    m_lpSurface->Unlock(NULL);

    return TRUE;
}
```

### Step 6: Surface Creation

```cpp
IDirectDrawSurface7 * CSprite::_pMakeSpriteSurface()
{
    // Create CMyDib to load BMP from PAK
    CMyDib mydib(m_cPakFileName, m_dwBitmapFileStartLoc);
    m_wBitmapSizeX = mydib.m_wWidthX;
    m_wBitmapSizeY = mydib.m_wWidthY;

    // Create DirectDraw offscreen surface
    pdds4 = m_pDDraw->pCreateOffScreenSurface(m_wBitmapSizeX, m_wBitmapSizeY);

    // Paint BMP to surface using GDI
    pdds4->GetDC(&hDC);
    mydib.PaintImage(hDC);
    pdds4->ReleaseDC(hDC);

    // Extract color key from top-left pixel
    ddsd.dwSize = 124;
    pdds4->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    pdds4->Unlock(NULL);
    wp = (WORD *)ddsd.lpSurface;
    m_wColorKey = *wp;  // Top-left pixel is the transparent color

    return pdds4;
}
```

---

## Memory Management

### Surface Lifecycle

Sprites use lazy loading and can be unloaded to free video memory:

| Method | Purpose |
|--------|---------|
| `_iOpenSprite()` | Load bitmap into DirectDraw surface on first use |
| `_iCloseSprite()` | Release DirectDraw surface to free VRAM |
| `iRestore()` | Restore surface after device loss (Alt+Tab) |

```cpp
void CSprite::_iCloseSprite()
{
    if (m_stBrush == NULL) return;
    if (m_lpSurface == NULL) return;
    if (m_lpSurface->IsLost() != DD_OK) return;

    m_lpSurface->Release();
    m_lpSurface = NULL;
    m_bIsSurfaceEmpty = TRUE;
    m_cAlphaDegree = 1;
}
```

### Reference Time Tracking

Each sprite tracks when it was last used via `m_dwRefTime`. This could be used for LRU cache eviction (though implementation details are unclear in the legacy code):

```cpp
m_dwRefTime = dwTime;  // Updated on each draw call
```

---

## Sprite Arrays and Allocation

### Array Limits

The game maintains three primary sprite arrays with fixed limits:

```cpp
#define DEF_MAXSPRITES      20000  // General sprites
#define DEF_MAXTILES        500    // Tile sprites
#define DEF_MAXEFFECTSPR    100    // Effect sprites

class CSprite * m_pSprite[DEF_MAXSPRITES];
class CSprite * m_pTileSpr[DEF_MAXTILES];
class CSprite * m_pEffectSpr[DEF_MAXEFFECTSPR];
```

### Sprite ID Ranges

The `m_pSprite[]` array uses a structured ID scheme:

| Range | Purpose | Notes |
|-------|---------|-------|
| 0 | Mouse cursor | Special handling |
| 22-31 | Interface fonts | Font sprites |
| 35-39 | Map images | Mini-map sprites |
| 40-46 | Feedback UI | China-specific |
| 50-73 | UI dialogs | Login, menus, etc. |
| 100-199 | Ground items | `DEF_SPRID_ITEMGROUND_PIVOTPOINT` |
| 200-299 | Equipped items | `DEF_SPRID_ITEMEQUIP_PIVOTPOINT` |
| 300-399 | Inventory items | `DEF_SPRID_ITEMPACK_PIVOTPOINT` |
| 400-499 | Dynamic items | `DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT` |
| 500+ | Characters/Monsters | Complex offset calculations |

### Sprite ID Definitions (`SpriteID.h`)

```cpp
#define DEF_SPRID_MOUSECURSOR               0
#define DEF_SPRID_INTERFACE_SPRFONTS        22
#define DEF_SPRID_INTERFACE_ADDINTERFACE    27
#define DEF_SPRID_INTERFACE_SPRFONTS2       28
#define DEF_SPRID_INTERFACE_F1HELPWINDOWS   29
#define DEF_SPRID_INTERFACE_FONT1           30
#define DEF_SPRID_INTERFACE_FONT2           31
#define DEF_SPRID_INTERFACE_NEWMAPS1        35
#define DEF_SPRID_INTERFACE_NEWMAPS2        36
#define DEF_SPRID_INTERFACE_NEWMAPS3        37
#define DEF_SPRID_INTERFACE_NEWMAPS4        38
#define DEF_SPRID_INTERFACE_NEWMAPS5        39
#define DEF_SPRID_INTERFACE_FEEDBACK1       40
#define DEF_SPRID_INTERFACE_FEEDBACK2       41
#define DEF_SPRID_INTERFACE_FEEDBACK3       42
#define DEF_SPRID_INTERFACE_FEEDBACK4       43
#define DEF_SPRID_INTERFACE_FEEDBACK5       44
#define DEF_SPRID_INTERFACE_FEEDBACK6       45
#define DEF_SPRID_INTERFACE_FEEDBACK7       46
#define DEF_SPRID_INTERFACE_MONSTER         50
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
#define DEF_SPRID_ITEMGROUND_PIVOTPOINT     100
#define DEF_SPRID_ITEMEQUIP_PIVOTPOINT      200
#define DEF_SPRID_ITEMPACK_PIVOTPOINT       300
#define DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT    400
```

---

## Loading Functions

### MakeSprite()

Loads multiple sprites from a single PAK file into `m_pSprite[]`:

```cpp
void CGame::MakeSprite(char* FileName, short sStart, short sCount, bool bAlphaEffect)
{
    int iTotalimage;
    DWORD nCount;
    char PathName[64];

    wsprintf(PathName, "sprites\\%s.pak", FileName);
    HANDLE m_hPakFile = CreateFile(PathName, GENERIC_READ, NULL, NULL,
                                   OPEN_EXISTING, NULL, NULL);
    if (m_hPakFile == INVALID_HANDLE_VALUE) return;

    SetFilePointer(m_hPakFile, 20, NULL, FILE_BEGIN);
    ReadFile(m_hPakFile, (char *)&iTotalimage, 4, &nCount, NULL);

    for (short i = 0; i < sCount; i++) {
        if (i < iTotalimage) {
            m_pSprite[i+sStart] = new class CSprite(m_hPakFile, &m_DDraw,
                                                     FileName, i, bAlphaEffect);
        }
    }
    CloseHandle(m_hPakFile);
}
```

**Parameters:**
- `FileName`: Base name of PAK file (without path/extension)
- `sStart`: Starting index in `m_pSprite[]` array
- `sCount`: Number of sprites to load
- `bAlphaEffect`: Whether sprite supports alpha blending effects

### MakeTileSpr()

Identical to `MakeSprite()` but loads into `m_pTileSpr[]`:

```cpp
void CGame::MakeTileSpr(char* FileName, short sStart, short sCount, bool bAlphaEffect)
{
    // Same logic as MakeSprite but uses m_pTileSpr[] array
    m_pTileSpr[i+sStart] = new class CSprite(m_hPakFile, &m_DDraw,
                                              FileName, i, bAlphaEffect);
}
```

### MakeEffectSpr()

Identical to `MakeSprite()` but loads into `m_pEffectSpr[]`:

```cpp
void CGame::MakeEffectSpr(char* FileName, short sStart, short sCount, bool bAlphaEffect)
{
    // Same logic as MakeSprite but uses m_pEffectSpr[] array
    m_pEffectSpr[i+sStart] = new class CSprite(m_hPakFile, &m_DDraw,
                                                FileName, i, bAlphaEffect);
}
```

---

## PAK File Inventory

### UI/Interface PAK Files

| PAK File | Contents | Sprites |
|----------|----------|---------|
| `interface.pak` | Mouse cursor, font sprites | 2 |
| `interface2.pak` | Additional UI, help windows | 3 |
| `sprfonts.pak` | Font type 1 and 2 | 2 |
| `New-Dialog.pak` | Loading screen, main menu, quit | 3 |
| `LoginDialog.pak` | Login, new account, agreement | 3 |
| `GameDialog.pak` | Game UI panels (11 sprites) | 11 |
| `DialogText.pak` | Text and button sprites | 2 |
| `Newmaps.pak` | Mini-map images | 5 |
| `Telescope.pak` | Guide/area map | 26 |
| `FeedBack.pak` | Feedback cards (China) | 7 |

### Character PAK Files

| PAK File | Contents |
|----------|----------|
| `Bm.pak` | Black Male character body |
| `Wm.pak` | White Male character body |
| `Ym.pak` | Yellow Male character body |
| `Bw.pak` | Black Female character body |
| `Ww.pak` | White Female character body |
| `Yw.pak` | Yellow Female character body |
| `Mpt.pak` | Male pants |
| `Wpt.pak` | Female pants |
| `Mhr.pak` | Male hair |
| `Whr.pak` | Female hair |
| `M*.pak` | Male equipment (Msw, Msh, Mbo, etc.) |
| `W*.pak` | Female equipment (Wsw, Wsh, Wbo, etc.) |

### Monster/NPC PAK Files

| PAK File | Monster Type |
|----------|--------------|
| `Slime.pak` | Slime |
| `Skeleton.pak` | Skeleton |
| `Zombie.pak` | Zombie |
| `Cyclops.pak` | Cyclops |
| `Orge.pak` | Ogre |
| `Troll.pak` | Troll |
| `Werewolf.pak` | Werewolf |
| `Demon.pak` | Demon |
| `Liche.pak` | Lich |
| `Barlog.pak` | Barlog |
| ... | (100+ monster types) |

### Item PAK Files

| PAK File | Contents |
|----------|----------|
| `item-pack.pak` | Inventory item icons |
| `item-ground.pak` | Ground/dropped item sprites |
| `item-equipM.pak` | Male equipped item sprites |
| `item-equipW.pak` | Female equipped item sprites |
| `item-dynamic.pak` | Dynamic/animated items |

### Effect PAK Files

| PAK File | Contents |
|----------|----------|
| `effect4.pak` - `effect13.pak` | Spell/combat effects |
| `CruEffect1.pak` | Crusade effects |

### Map Tile PAK Files

| PAK File | Contents |
|----------|----------|
| `maptiles1.pak` - `maptiles6.pak` | Terrain tiles |
| `Trees1.pak` | Tree sprites |
| `TreeShadows.pak` | Tree shadow sprites |
| `Sinside1.pak` | Indoor building tiles |
| `objects1.pak` | Map objects |
| `structures1.pak` | Structure tiles |

---

## File Locations

PAK files are expected in the `sprites\` subdirectory relative to the game executable:

```
game.exe
sprites\
  interface.pak
  interface2.pak
  Bm.pak
  ...
```

The path is constructed as:
```cpp
wsprintf(PathName, "sprites\\%s.pak", FileName);
```

---

## CMyDib Class (BMP Loading)

The `CMyDib` class handles reading embedded BMP data from PAK files:

### Header File (`Mydib.h`)

```cpp
class CMyDib
{
public:
    CMyDib(char *szFilename, unsigned long dwFilePointer);
    ~CMyDib();
    void PaintImage(HDC hDC);

    WORD m_wWidthX;       // Bitmap width
    WORD m_wWidthY;       // Bitmap height
    WORD m_wColorNums;    // Number of colors in palette
    LPSTR m_lpDib;        // DIB data buffer
    LPBITMAPINFO m_bmpInfo;  // Bitmap info pointer
};
```

### Implementation (`Mydib.cpp`)

```cpp
CMyDib::CMyDib(char *szFilename, unsigned long dwFilePointer)
{
    BITMAPFILEHEADER fh;
    m_lpDib = NULL;
    HANDLE hFileRead;
    DWORD nCount;
    char PathName[28];

    wsprintf(PathName, "sprites\\%s.pak", szFilename);
    hFileRead = CreateFile(PathName, GENERIC_READ, NULL, NULL,
                           OPEN_EXISTING, NULL, NULL);
    SetFilePointer(hFileRead, dwFilePointer, NULL, FILE_BEGIN);

    // Read BMP file header (14 bytes)
    ReadFile(hFileRead, (char *)&fh, 14, &nCount, NULL);

    // Allocate buffer for rest of BMP data
    m_lpDib = (LPSTR)new char[fh.bfSize-14];
    ReadFile(hFileRead, (char *)m_lpDib, fh.bfSize-14, &nCount, NULL);
    CloseHandle(hFileRead);

    // Extract bitmap info
    LPBITMAPINFOHEADER bmpInfoHeader = (LPBITMAPINFOHEADER)m_lpDib;
    m_bmpInfo = (LPBITMAPINFO)m_lpDib;
    m_wWidthX = (WORD)(bmpInfoHeader->biWidth);
    m_wWidthY = (WORD)(bmpInfoHeader->biHeight);

    // Determine color count
    if (bmpInfoHeader->biClrUsed == 0) {
        if (bmpInfoHeader->biBitCount == 24) m_wColorNums = 0;
        else if (bmpInfoHeader->biBitCount == 8) m_wColorNums = 256;
        else if (bmpInfoHeader->biBitCount == 1) m_wColorNums = 2;
        else if (bmpInfoHeader->biBitCount == 4) m_wColorNums = 16;
        else m_wColorNums = 0;
    }
    else {
        m_wColorNums = (WORD)(bmpInfoHeader->biClrUsed);
    }
}

void CMyDib::PaintImage(HDC hDC)
{
    if (m_lpDib == NULL) return;

    // Paint DIB to device context
    SetDIBitsToDevice(hDC, 0, 0, m_wWidthX, m_wWidthY,
                      0, 0, 0, m_wWidthY,
                      m_lpDib + *(LPDWORD)m_lpDib + 4*m_wColorNums,
                      m_bmpInfo, DIB_RGB_COLORS);
}
```

---

## CSprite Class Interface

### Public Methods

| Method | Description |
|--------|-------------|
| `PutSpriteFast()` | Draw sprite with color key transparency |
| `PutSpriteFastNoColorKey()` | Draw sprite without transparency |
| `PutTransSprite()` | Draw with variable alpha transparency |
| `PutTransSprite70()` | Draw at 70% opacity |
| `PutTransSprite50()` | Draw at 50% opacity |
| `PutTransSprite25()` | Draw at 25% opacity |
| `PutShadowSprite()` | Draw shadow (darkened version) |
| `PutSpriteRGB()` | Draw with color tinting |
| `PutTransSpriteRGB()` | Draw with transparency and color tint |
| `PutFadeSprite()` | Draw with fade effect |
| `PutRevTransSprite()` | Draw with reverse transparency |
| `_bCheckCollison()` | Test if point collides with sprite |
| `_GetSpriteRect()` | Get bounding rectangle |
| `iRestore()` | Restore surface after device loss |

### Member Variables

| Variable | Type | Description |
|----------|------|-------------|
| `m_lpSurface` | `LPDIRECTDRAWSURFACE7` | DirectDraw surface for sprite |
| `m_pSurfaceAddr` | `WORD*` | Pointer to surface pixel data |
| `m_sPitch` | `short` | Surface pitch (stride) in WORDs |
| `m_stBrush` | `stBrush*` | Frame metadata array |
| `m_iTotalFrame` | `int` | Total number of frames |
| `m_dwBitmapFileStartLoc` | `DWORD` | Offset to BMP data in PAK |
| `m_cPakFileName` | `char[16]` | PAK file name (lazy loading) |
| `m_wBitmapSizeX` | `WORD` | Bitmap width |
| `m_wBitmapSizeY` | `WORD` | Bitmap height |
| `m_wColorKey` | `WORD` | Transparent color value |
| `m_bIsSurfaceEmpty` | `bool` | Whether surface is loaded |
| `m_bAlphaEffect` | `bool` | Whether alpha effects apply |
| `m_cAlphaDegree` | `char` | Current alpha degree setting |
| `m_rcBound` | `RECT` | Last drawn bounding rectangle |
| `m_dwRefTime` | `DWORD` | Last reference time (LRU) |
| `m_pDDraw` | `DXC_ddraw*` | DirectDraw wrapper pointer |
| `m_sPivotX/Y` | `short` | Sprite pivot points |
| `m_bOnCriticalSection` | `bool` | Thread safety flag |

---

## Color Key Transparency

The transparent color is automatically determined from the top-left pixel (0,0) of each sprite bitmap:

```cpp
IDirectDrawSurface7 * CSprite::_pMakeSpriteSurface()
{
    // ... load BMP to surface ...

    // Lock surface to read pixel
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = 124;
    pdds4->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    pdds4->Unlock(NULL);

    // First pixel is the color key
    wp = (WORD *)ddsd.lpSurface;
    m_wColorKey = *wp;

    return pdds4;
}
```

When rendering, pixels matching `m_wColorKey` are not drawn (transparent).

---

## Alpha Blending Support

The sprite system supports 5 levels of alpha blending:

| Level | Opacity | Method |
|-------|---------|--------|
| 100% | Full | `PutSpriteFast()` (no blend) |
| 70% | 70% opaque | `PutTransSprite70()` |
| 50% | 50% opaque | `PutTransSprite50()` |
| 25% | 25% opaque | `PutTransSprite25()` |
| 2% | Nearly invisible | `PutRevTransSprite()` |

These use pre-computed lookup tables for performance:
```cpp
extern long G_lTransG100[64][64], G_lTransRB100[64][64];
extern long G_lTransG70[64][64], G_lTransRB70[64][64];
extern long G_lTransG50[64][64], G_lTransRB50[64][64];
extern long G_lTransG25[64][64], G_lTransRB25[64][64];
extern long G_lTransG2[64][64], G_lTransRB2[64][64];
```

---

## Loading Sequence During Game Start

The game loads sprites in phases during the loading screen (`UpdateScreen_OnLoading()`):

### Phase 1 (m_cLoading = 1-4)
- Interface sprites
- Font sprites
- Login dialogs

### Phase 2 (m_cLoading = 8-16)
- Map tiles
- Item sprites (ground, equipped, inventory)

### Phase 3 (m_cLoading = 20-36)
- Character bodies (male/female, all skin colors)
- Character equipment sprites

### Phase 4 (m_cLoading = 40-76)
- Weapon sprites
- Armor sprites
- Shield sprites

### Phase 5 (m_cLoading = 80-100)
- Effect sprites
- Monster sprites

The loading progress is displayed as a percentage based on `m_cLoading` value.

---

## Source File References

| File | Purpose |
|------|---------|
| `Sprite.h` | CSprite class declaration |
| `Sprite.cpp` | CSprite implementation (3000+ lines) |
| `Mydib.h` | CMyDib class declaration |
| `Mydib.cpp` | BMP loading from PAK files |
| `SpriteID.h` | Sprite ID constant definitions |
| `Game.h` | Sprite array declarations (lines 640-642) |
| `Game.cpp` | Sprite loading functions (lines 3548-4300+) |

---

## Known Limitations

1. **Fixed Array Sizes**: Maximum of 20,000 general sprites, 500 tiles, 100 effects
2. **16-bit Color Only**: Assumes RGB555 or RGB565 pixel format
3. **No Compression**: BMP data is stored uncompressed
4. **Single File Handle**: File is opened/closed per loading operation
5. **No Streaming**: Entire BMP is loaded into memory at once
6. **Hardcoded Paths**: Sprites must be in `sprites\` directory
7. **Case Sensitivity**: PAK filenames may be case-sensitive depending on filesystem

---

## Modernization Considerations

When modernizing this system, consider:

1. **Replace PAK format** with standard formats (PNG atlas with JSON metadata)
2. **Add compression** (zlib, LZ4) for reduced file sizes
3. **Implement virtual filesystem** for flexible asset locations
4. **Use texture atlases** for batched rendering
5. **Add async loading** with proper threading
6. **Implement proper resource management** with reference counting
7. **Support modern color depths** (32-bit RGBA)
8. **Remove DirectDraw dependency** in favor of modern graphics APIs
