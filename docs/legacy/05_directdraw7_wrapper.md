# Legacy Documentation: DirectDraw 7 Wrapper (DXC_ddraw)

## Overview

The `DXC_ddraw` class is a wrapper around Microsoft DirectDraw 7, providing the core graphics rendering infrastructure for the Helbreath client. This class handles display mode management, surface creation, page flipping, text rendering, color format detection, and transparency lookup table generation.

**Source Files:**
- `DXC_ddraw.h` - Class declaration
- `DXC_ddraw.cpp` - Implementation

**Dependencies:**
- `ddraw.h` - DirectDraw 7 SDK header
- `Misc.h` - Utility functions
- `GlobalDef.h` - Language and configuration definitions

---

## Class Declaration

```cpp
class DXC_ddraw
{
public:
    // Custom memory allocation using Windows heap
    void * operator new (size_t size);
    void operator delete(void * mem);

    DXC_ddraw();
    virtual ~DXC_ddraw();

    // Initialization and mode switching
    BOOL bInit(HWND hWnd);
    void ChangeDisplayMode(HWND hWnd);
    HRESULT InitFlipToGDI(HWND hWnd);

    // Surface operations
    IDirectDrawSurface7 * pCreateOffScreenSurface(WORD iSzX, WORD iSzY);
    HRESULT iFlip();
    void ClearBackB4();

    // Color key operations
    HRESULT iSetColorKey(IDirectDrawSurface7 * pdds4, WORD wColorKey);
    HRESULT iSetColorKey(IDirectDrawSurface7 * pdds4, COLORREF rgb);
    DWORD _dwColorMatch(IDirectDrawSurface7 * pdds4, WORD wColorKey);
    DWORD _dwColorMatch(IDirectDrawSurface7 * pdds4, COLORREF rgb);

    // Pixel format detection
    void _TestPixelFormat();
    void ColorTransferRGB(COLORREF fcolor, int * iR, int * iG, int * iB);

    // Drawing operations
    void PutPixel(short sX, short sY, WORD wR, WORD wG, WORD wB);
    void DrawShadowBox(short sX, short sY, short dX, short dY, int iType = 0);

    // Text rendering
    void _GetBackBufferDC();
    void _ReleaseBackBufferDC();
    void TextOut(int x, int y, char * cStr, COLORREF rgb);
    void DrawText(LPRECT pRect, char * pString, COLORREF rgb);

    // Transparency lookup table generation
    long _CalcMinValue(int iS, int iD, char cMode);
    long _CalcMaxValue(int iS, int iD, char cMode, char cMethod, double dAlpha);

    // Screenshot
    bool Screenshot(LPCTSTR FileName, LPDIRECTDRAWSURFACE7 lpDDS);
};
```

---

## Member Variables

### DirectDraw Objects

| Variable | Type | Description |
|----------|------|-------------|
| `m_lpDD4` | `LPDIRECTDRAW7` | Main DirectDraw 7 interface object |
| `m_lpFrontB4` | `LPDIRECTDRAWSURFACE7` | Primary (front) buffer surface |
| `m_lpBackB4` | `LPDIRECTDRAWSURFACE7` | Back buffer surface for rendering |
| `m_lpBackB4flip` | `LPDIRECTDRAWSURFACE7` | Flip chain back buffer (fullscreen only) |
| `m_lpPDBGS` | `LPDIRECTDRAWSURFACE7` | Pre-Draw Background Surface (672x512 pixels) |

### Surface Memory Access

| Variable | Type | Description |
|----------|------|-------------|
| `m_pBackB4Addr` | `WORD *` | Direct pointer to back buffer pixel data |
| `m_sBackB4Pitch` | `short` | Pitch of back buffer in WORDs (not bytes) |
| `m_hDC` | `HDC` | Device context for GDI text rendering |
| `m_hFontInUse` | `HFONT` | Currently selected font handle |

### Display Configuration

| Variable | Type | Description |
|----------|------|-------------|
| `m_bFullMode` | `BOOL` | TRUE = fullscreen exclusive, FALSE = windowed |
| `m_cPixelFormat` | `char` | Detected pixel format (1=RGB565, 2=RGB555, 3=BGR565) |
| `m_rcClipArea` | `RECT` | Clipping rectangle for rendering (default: 0,0,640,480) |
| `m_rcFlipping` | `RECT` | Rectangle used for flip/blit operations |

### Transparency Lookup Tables

The class contains six pairs of pre-computed lookup tables for fast alpha blending:

| Table Pair | Alpha Level | Description |
|------------|-------------|-------------|
| `m_lTransG100`, `m_lTransRB100` | 100% | Full additive blending |
| `m_lTransG70`, `m_lTransRB70` | 70% | 70% source opacity |
| `m_lTransG50`, `m_lTransRB50` | 50% | 50% source opacity |
| `m_lTransG25`, `m_lTransRB25` | 25% | 25% source opacity |
| `m_lTransG2`, `m_lTransRB2` | Average | Average blend (method 2) |
| `m_lFadeG`, `m_lFadeRB` | Fade | Subtractive fade effect |

Each table is a 64x64 array indexed by source and destination color channel values:
```cpp
long m_lTransG100[64][64];   // Green channel (0-63 for RGB565)
long m_lTransRB100[64][64];  // Red/Blue channels (0-31 for 5-bit)
```

---

## Initialization

### `bInit(HWND hWnd)`

Initializes the DirectDraw subsystem. This is the primary initialization function.

**Process:**

1. **Set default clip area:**
   ```cpp
   SetRect(&m_rcClipArea, 0, 0, 640, 480);
   ```

2. **Create DirectDraw object:**
   ```cpp
   DirectDrawCreateEx(NULL, (VOID**)&m_lpDD4, IID_IDirectDraw7, NULL);
   ```

3. **Mode-dependent initialization:**

   **Fullscreen Mode (`m_bFullMode == TRUE`):**
   ```cpp
   m_lpDD4->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
   m_lpDD4->SetDisplayMode(640, 480, 16, 0, 0);
   // Create flipping chain with 1 back buffer
   ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
   ddsd.dwBackBufferCount = 1;
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
   ```

   **Windowed Mode (`m_bFullMode == FALSE`):**
   ```cpp
   m_lpDD4->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
   // Center window on screen
   SetWindowPos(hWnd, HWND_TOP, cx-320, cy-240, 640, 480, SWP_SHOWWINDOW);
   // Create primary surface only (no flip chain)
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
   ```

4. **Create rendering surfaces:**
   ```cpp
   m_lpBackB4 = pCreateOffScreenSurface(640, 480);      // Main back buffer
   m_lpPDBGS = pCreateOffScreenSurface(640+32, 480+32); // Pre-draw BG (672x512)
   ```

5. **Lock back buffer to get memory address:**
   ```cpp
   m_lpBackB4->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
   m_pBackB4Addr = (WORD *)ddsd.lpSurface;
   m_sBackB4Pitch = (short)ddsd.lPitch >> 1;  // Convert bytes to WORDs
   m_lpBackB4->Unlock(NULL);
   ```

6. **Detect pixel format and build lookup tables:**
   ```cpp
   _TestPixelFormat();
   // Build all transparency tables for both member and global arrays
   for (iS = 0; iS < 64; iS++)
   for (iD = 0; iD < 64; iD++) {
       m_lTransRB100[iD][iS] = _CalcMaxValue(iS, iD, 'R', 1, 1.0f);
       m_lTransG100[iD][iS]  = _CalcMaxValue(iS, iD, 'G', 1, 1.0f);
       // ... all other tables
   }
   ```

7. **Create language-specific font:**
   ```cpp
   // Font selection based on DEF_LANGUAGE compile-time constant
   #if DEF_LANGUAGE == 1    // Taiwan
       CreateFont(12, ..., CHINESEBIG5_CHARSET, ..., "MingLiu");
   #elif DEF_LANGUAGE == 2  // China
       CreateFont(12, ..., GB2312_CHARSET, ..., "MingLiu");
   #elif DEF_LANGUAGE == 3  // Korea
       CreateFont(12, ..., HANGUL_CHARSET, ..., "굴림");
   #elif DEF_LANGUAGE == 4  // English
       CreateFont(16, ..., ANSI_CHARSET, ..., "Comic Sans MS");
   #elif DEF_LANGUAGE == 5  // Japan
       CreateFont(12, ..., SHIFTJIS_CHARSET, ..., "MS PGothic");
   #endif
   ```

**Return Value:** `TRUE` on success, `FALSE` on failure.

**Default Mode:**
- Debug builds (`_DEBUG`): Windowed mode
- Release builds: Fullscreen mode

---

## Pixel Formats

### `_TestPixelFormat()`

Detects the pixel format of the display adapter by examining the red channel bit mask.

**Detected Formats:**

| `m_cPixelFormat` | Format | R Mask | G Mask | B Mask | Description |
|------------------|--------|--------|--------|--------|-------------|
| 1 | RGB 5:6:5 | `0xF800` | `0x07E0` | `0x001F` | Most common 16-bit format |
| 2 | RGB 5:5:5 | `0x7C00` | `0x03E0` | `0x001F` | 15-bit with unused high bit |
| 3 | BGR 5:6:5 | `0x001F` | `0x07E0` | `0xF800` | Reversed byte order |

**Implementation:**
```cpp
void DXC_ddraw::_TestPixelFormat()
{
    DDSURFACEDESC2 ddSurfaceDesc2;
    ZeroMemory(&ddSurfaceDesc2, sizeof(DDSURFACEDESC2));
    ddSurfaceDesc2.dwSize = sizeof(ddSurfaceDesc2);
    ddSurfaceDesc2.dwFlags = DDSD_PIXELFORMAT;
    m_lpBackB4->GetSurfaceDesc(&ddSurfaceDesc2);

    if (ddSurfaceDesc2.ddpfPixelFormat.dwRBitMask == 0x0000F800)
        m_cPixelFormat = 1;  // RGB 5:6:5
    if (ddSurfaceDesc2.ddpfPixelFormat.dwRBitMask == 0x00007C00)
        m_cPixelFormat = 2;  // RGB 5:5:5
    if (ddSurfaceDesc2.ddpfPixelFormat.dwRBitMask == 0x0000001F)
        m_cPixelFormat = 3;  // BGR 5:6:5
}
```

### `ColorTransferRGB(COLORREF fcolor, int *iR, int *iG, int *iB)`

Converts a Windows COLORREF (24-bit RGB) to the current 16-bit pixel format components.

**For RGB 5:6:5 (m_cPixelFormat == 1):**
```cpp
wR = (WORD)((fcolor & 0x000000f8) >> 3);   // 5-bit red
wG = (WORD)((fcolor & 0x0000fc00) >> 10);  // 6-bit green
wB = (WORD)((fcolor & 0x00f80000) >> 19);  // 5-bit blue
```

**For RGB 5:5:5 (m_cPixelFormat == 2):**
```cpp
wR = (WORD)((fcolor & 0x000000f8) >> 3);   // 5-bit red
wG = (WORD)((fcolor & 0x0000f800) >> 11);  // 5-bit green
wB = (WORD)((fcolor & 0x00f80000) >> 19);  // 5-bit blue
```

---

## Surface Management

### `pCreateOffScreenSurface(WORD wSzX, WORD wSzY)`

Creates an offscreen surface in system memory.

**Parameters:**
- `wSzX` - Width in pixels (auto-aligned to 4-pixel boundary)
- `wSzY` - Height in pixels

**Implementation:**
```cpp
IDirectDrawSurface7 * DXC_ddraw::pCreateOffScreenSurface(WORD wSzX, WORD wSzY)
{
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface7 * pdds4;

    ZeroMemory(&ddsd, sizeof(ddsd));
    // Ensure width is 4-pixel aligned for optimal memory access
    if ((wSzX % 4) != 0) wSzX += 4 - (wSzX % 4);

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth = (DWORD)wSzX;
    ddsd.dwHeight = (DWORD)wSzY;

    if (m_lpDD4->CreateSurface(&ddsd, &pdds4, NULL) != DD_OK)
        return NULL;
    return pdds4;
}
```

**Notes:**
- Uses system memory (`DDSCAPS_SYSTEMMEMORY`) for CPU access
- Width alignment improves memory access patterns
- Caller is responsible for releasing the surface

### Surface Dimensions Used

| Surface | Dimensions | Purpose |
|---------|------------|---------|
| `m_lpBackB4` | 640 x 480 | Main rendering back buffer |
| `m_lpPDBGS` | 672 x 512 | Pre-drawn background (includes 32px border for scrolling) |

---

## Page Flipping

### `iFlip()`

Presents the back buffer to the screen.

**Fullscreen Behavior:**

Without IME or HTML overlays:
```cpp
// Fast path: BltFast to flip buffer, then hardware flip
m_lpBackB4flip->BltFast(0, 0, m_lpBackB4, &m_rcFlipping, DDBLTFAST_NOCOLORKEY);
m_lpFrontB4->Flip(m_lpBackB4flip, DDFLIP_WAIT);
```

With Windows IME enabled (`DEF_USING_WIN_IME`):
```cpp
// Must use Blt for GDI compatibility
m_lpFrontB4->Blt(NULL, m_lpBackB4, NULL, DDBLT_WAIT, NULL);
if (G_hEditWnd != NULL) {
    m_lpDD4->FlipToGDISurface();
}
```

**Windowed Behavior:**
```cpp
// Blt to positioned rectangle on primary surface
m_lpFrontB4->Blt(&m_rcFlipping, m_lpBackB4, NULL, DDBLT_WAIT, NULL);
```

**Surface Lost Handling:**
```cpp
if (ddVal == DDERR_SURFACELOST) {
    m_lpFrontB4->Restore();
    m_lpBackB4->Restore();
    // Re-acquire back buffer address
    m_lpBackB4->Lock(NULL, &ddsd2, DDLOCK_WAIT, NULL);
    m_pBackB4Addr = (WORD *)ddsd2.lpSurface;
    m_lpBackB4->Unlock(NULL);
    return DDERR_SURFACELOST;
}
```

**Return Values:**
- `DD_OK` - Success
- `DDERR_SURFACELOST` - Surfaces were lost and restored (caller should reload sprites)

---

## Display Mode Switching

### `ChangeDisplayMode(HWND hWnd)`

Toggles between fullscreen and windowed mode at runtime.

**Process:**
1. Release flip buffer (if exists)
2. Release back and front buffers
3. Restore original display mode (if switching from fullscreen)
4. Set new cooperative level
5. Create new surface chain
6. Recreate offscreen surfaces
7. Re-acquire back buffer address

**Switching TO Windowed:**
```cpp
m_lpDD4->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
SetWindowPos(hWnd, NULL, cx-320, cy-240, 640, 480, SWP_SHOWWINDOW);
// Create simple primary surface
ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
m_bFullMode = FALSE;
```

**Switching TO Fullscreen:**
```cpp
m_lpDD4->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
m_lpDD4->SetDisplayMode(640, 480, 16, 0, 0);
// Create flipping chain
ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
m_bFullMode = TRUE;
```

---

## Color Key Operations

### `iSetColorKey(IDirectDrawSurface7 *pdds4, COLORREF rgb)`
### `iSetColorKey(IDirectDrawSurface7 *pdds4, WORD wColorKey)`

Sets the source color key for a surface, enabling transparency during blit operations.

**Implementation (COLORREF version):**
```cpp
HRESULT DXC_ddraw::iSetColorKey(IDirectDrawSurface7 * pdds4, COLORREF rgb)
{
    DDCOLORKEY ddck;
    ddck.dwColorSpaceLowValue = _dwColorMatch(pdds4, rgb);
    ddck.dwColorSpaceHighValue = ddck.dwColorSpaceLowValue;
    return pdds4->SetColorKey(DDCKEY_SRCBLT, &ddck);
}
```

### `_dwColorMatch(IDirectDrawSurface7 *pdds4, COLORREF rgb)`

Converts a COLORREF to the surface's native pixel format by drawing and reading back.

**Process:**
1. Get device context from surface
2. Save pixel at (0,0)
3. Draw the target color at (0,0) using GDI SetPixel
4. Release DC and lock surface
5. Read the native format value at (0,0)
6. Mask to appropriate bit depth
7. Restore original pixel

```cpp
DWORD DXC_ddraw::_dwColorMatch(IDirectDrawSurface7 * pdds4, COLORREF rgb)
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;

    if (rgb != CLR_INVALID && pdds4->GetDC(&hdc) == DD_OK) {
        rgbT = GetPixel(hdc, 0, 0);      // Save original
        SetPixel(hdc, 0, 0, rgb);         // Draw target color
        pdds4->ReleaseDC(hdc);
    }

    // Lock and read native format
    while ((hres = pdds4->Lock(NULL, &ddsd2, 0, NULL)) == DDERR_WASSTILLDRAWING);
    if (hres == DD_OK) {
        dw = *(DWORD *)ddsd2.lpSurface;
        dw &= (1 << ddsd2.ddpfPixelFormat.dwRGBBitCount) - 1;
        pdds4->Unlock(NULL);
    }

    // Restore original pixel
    if (rgb != CLR_INVALID && pdds4->GetDC(&hdc) == DD_OK) {
        SetPixel(hdc, 0, 0, rgbT);
        pdds4->ReleaseDC(hdc);
    }

    return dw;
}
```

---

## Transparency System

### Overview

The Helbreath client uses pre-computed lookup tables for fast software alpha blending. Since DirectDraw 7 doesn't support hardware alpha blending, all transparency effects are done via per-pixel CPU operations using these tables.

### Lookup Table Generation

#### `_CalcMaxValue(int iS, int iD, char cMode, char cMethod, double dAlpha)`

Generates blending values for additive/alpha operations.

**Parameters:**
- `iS` - Source color channel value (0-63)
- `iD` - Destination color channel value (0-63)
- `cMode` - 'G' for green channel, other for red/blue
- `cMethod` - 1 = alpha blend, 2 = average
- `dAlpha` - Alpha multiplier (0.0 to 1.0)

**Method 1 (Alpha Blend):**
```cpp
// result = (source * alpha) + destination, clamped
dTmp = (double)iS * dAlpha;
iS = (int)dTmp;
Sum = iS + iD;
if (Sum < iD) Sum = iD;  // Overflow protection
```

**Method 2 (Average):**
```cpp
// result = (source + destination) / 2
Sum = (iS + iD) / 2;
```

**Clamping:**
```cpp
switch (cMode) {
case 'G':
    // Green has 6 bits in RGB565
    if (m_cPixelFormat == 1 && Sum >= 64) Sum = 63;
    // Green has 5 bits in RGB555
    if (m_cPixelFormat == 2 && Sum >= 32) Sum = 31;
    break;
default:
    // Red/Blue always have 5 bits
    if (Sum >= 32) Sum = 31;
    break;
}
```

#### `_CalcMinValue(int iS, int iD, char cMode)`

Generates values for subtractive fade operations.

```cpp
// result = destination - source, clamped to 0
Sum = iD - iS;
if (Sum < 0) Sum = 0;
// Same clamping as _CalcMaxValue
```

### Alpha Blending Levels

The system supports five pre-defined transparency levels:

| Level | Alpha | Use Case |
|-------|-------|----------|
| 100% | 1.0 | Full additive blend (magic effects, glows) |
| 70% | 0.7 | Semi-transparent objects |
| 50% | 0.5 | 50% transparency (ghosts, invisibility) |
| 25% | 0.25 | Highly transparent effects |
| Average | N/A | Smooth blending (method 2) |
| Fade | N/A | Subtractive darkening effect |

### Usage in Sprite Rendering

The `CSprite` class uses these tables for transparency rendering:

**RGB 5:6:5 Format:**
```cpp
// Extract channels, lookup, and recombine
pDst[ix] = (WORD)(
    (G_lTransRB100[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) |  // Red
    (G_lTransG100[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) |         // Green
    G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]                        // Blue
);
```

**RGB 5:5:5 Format:**
```cpp
pDst[ix] = (WORD)(
    (G_lTransRB100[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) |  // Red
    (G_lTransG100[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) |         // Green
    G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]                        // Blue
);
```

---

## Drawing Operations

### `ClearBackB4()`

Clears the back buffer to black (zeroes all pixels).

```cpp
void DXC_ddraw::ClearBackB4()
{
    DDSURFACEDESC2 ddsd2;
    ddsd2.dwSize = sizeof(ddsd2);
    if (m_lpBackB4->Lock(NULL, &ddsd2, DDLOCK_WAIT, NULL) != DD_OK) return;
    memset((char *)ddsd2.lpSurface, 0, ddsd2.lPitch * 480);
    m_lpBackB4->Unlock(NULL);
}
```

### `DrawShadowBox(short sX, short sY, short dX, short dY, int iType)`

Draws a filled rectangle with various darkening effects.

**Type 0 - 50% Darken (bit shift):**
```cpp
// For each pixel: (pixel & mask) >> 1
// RGB565 mask: 0xF7DE (preserves structure while halving values)
// RGB555 mask: 0x7BDE
for (iy = 0; iy <= (dY - sY); iy++) {
    for (ix = 0; ix <= (dX - sX); ix++)
        pDst[ix] = (pDst[ix] & 0xf7de) >> 1;
    pDst += m_sBackB4Pitch;
}
```

**Type 1 - Fixed Dark Gray:**
```cpp
// RGB565: 0x38E7 (approximately RGB(28, 28, 28) * 8)
// RGB555: 0x1CE7
wValue = (m_cPixelFormat == 1) ? 0x38e7 : 0x1ce7;
```

**Type 2 - Fixed Darker Gray:**
```cpp
// RGB565: 0x1863 (approximately RGB(12, 12, 12) * 4)
// RGB555: 0x0C63
wValue = (m_cPixelFormat == 1) ? 0x1863 : 0xc63;
```

### `PutPixel(short sX, short sY, WORD wR, WORD wG, WORD wB)`

Draws a single pixel to the back buffer.

**Bounds Checking:**
```cpp
if ((sX < 0) || (sY < 0) || (sX > 639) || (sY > 479)) return;
```

**Pixel Assembly:**
```cpp
pDst = (WORD *)m_pBackB4Addr + sX + (sY * m_sBackB4Pitch);

switch (m_cPixelFormat) {
case 1:  // RGB 5:6:5
    *pDst = (WORD)(((wR>>3)<<11) | ((wG>>2)<<5) | (wB>>3));
    break;
case 2:  // RGB 5:5:5
    *pDst = (WORD)(((wR>>3)<<10) | ((wG>>3)<<5) | (wB>>3));
    break;
}
```

**Note:** Input R, G, B values are 8-bit (0-255) and are shifted down to fit the 5/6-bit channels.

---

## Text Rendering

Text is rendered using GDI functions through a device context obtained from the back buffer surface.

### `_GetBackBufferDC()` / `_ReleaseBackBufferDC()`

Acquires/releases the GDI device context for the back buffer.

```cpp
void DXC_ddraw::_GetBackBufferDC()
{
    m_lpBackB4->GetDC(&m_hDC);
    SelectObject(m_hDC, m_hFontInUse);
    SetBkMode(m_hDC, TRANSPARENT);
    SetBkColor(m_hDC, RGB(0,0,0));
}

void DXC_ddraw::_ReleaseBackBufferDC()
{
    m_lpBackB4->ReleaseDC(m_hDC);
}
```

**Important:** The surface is locked while the DC is held. Minimize time between Get/Release.

### `TextOut(int x, int y, char *cStr, COLORREF rgb)`

Renders a single-line text string.

```cpp
void DXC_ddraw::TextOut(int x, int y, char * cStr, COLORREF rgb)
{
    SetTextColor(m_hDC, rgb);
    ::TextOut(m_hDC, x, y, cStr, strlen(cStr));
}
```

### `DrawText(LPRECT pRect, char *pString, COLORREF rgb)`

Renders multi-line text within a rectangle with word wrapping.

```cpp
void DXC_ddraw::DrawText(LPRECT pRect, char *pString, COLORREF rgb)
{
    SetTextColor(m_hDC, rgb);
    ::DrawText(m_hDC, pString, strlen(pString), pRect,
               DT_CENTER | DT_NOCLIP | DT_WORDBREAK | DT_NOPREFIX);
}
```

**Flags Used:**
- `DT_CENTER` - Center text horizontally
- `DT_NOCLIP` - Don't clip to rectangle
- `DT_WORDBREAK` - Break lines at word boundaries
- `DT_NOPREFIX` - Don't interpret '&' as underline prefix

### Language-Specific Fonts

| Language | DEF_LANGUAGE | Font | Size | Charset |
|----------|--------------|------|------|---------|
| Taiwan | 1 | MingLiu | 12pt | CHINESEBIG5_CHARSET |
| China | 2 | MingLiu | 12pt | GB2312_CHARSET |
| Korea | 3 | 굴림 (Gulim) | 12pt | HANGUL_CHARSET |
| English | 4 | Comic Sans MS | 16pt | ANSI_CHARSET |
| Japan | 5 | MS PGothic | 12pt | SHIFTJIS_CHARSET |

**Font Characteristics:**
- Weight: `FW_NORMAL`
- Quality: `NONANTIALIASED_QUALITY` (for pixel-perfect rendering)
- Pitch: `VARIABLE_PITCH`

---

## Screenshot System

### `Screenshot(LPCTSTR FileName, LPDIRECTDRAWSURFACE7 lpDDS)`

Captures a surface to a BMP file.

**Process:**
1. Get surface dimensions from description
2. Get GDI-compatible DC from surface
3. Create compatible bitmap
4. BitBlt surface contents to bitmap
5. Use GetDIBits to convert to device-independent bitmap
6. Write BMP file header
7. Write BITMAPINFOHEADER
8. Write palette (if applicable)
9. Write pixel data
10. Update file header with correct sizes

**BMP File Structure Written:**
```
[BITMAPFILEHEADER - 14 bytes]
[BITMAPINFOHEADER - 40 bytes]
[Palette - variable, typically 0 for 16-bit]
[Pixel Data - biSizeImage bytes]
```

**Error Handling:**
Uses exception-based error handling with numeric error codes:
- 0: GetSurfaceDesc failed
- 1: GetDC failed
- 2: CreateCompatibleBitmap failed
- 3: CreateCompatibleDC failed
- 4: Memory allocation for BITMAPINFO failed
- 5-7: GetDIBits failed
- 8-13: File operations failed

---

## GDI Interoperability

### `InitFlipToGDI(HWND hWnd)`

Sets up a clipper for GDI rendering on the primary surface.

**Purpose:** Allows GDI content (like IME windows, HTML dialogs) to render correctly when overlaid on DirectDraw content.

```cpp
HRESULT DXC_ddraw::InitFlipToGDI(HWND hWnd)
{
    DDCAPS ddcaps;
    ZeroMemory(&ddcaps, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(ddcaps);
    m_lpDD4->GetCaps(&ddcaps, NULL);

    // Check if windowed rendering is supported
    if ((ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) == 0)
        return E_FAIL;

    // Create and attach clipper
    LPDIRECTDRAWCLIPPER pClipper;
    m_lpDD4->CreateClipper(0, &pClipper, NULL);
    pClipper->SetHWnd(0, hWnd);
    m_lpFrontB4->SetClipper(pClipper);
    pClipper->Release();  // Surface maintains ref count

    return S_OK;
}
```

---

## Memory Management

### Custom Operators

The class overrides `new` and `delete` to use the Windows heap directly:

```cpp
void * operator new (size_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
};

void operator delete(void * mem)
{
    HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, mem);
};
```

**Rationale:**
- `HEAP_ZERO_MEMORY` ensures all members are initialized to 0/NULL
- `HEAP_NO_SERIALIZE` improves performance (single-threaded access assumed)
- Direct heap usage avoids potential CRT memory issues

### Destructor

```cpp
DXC_ddraw::~DXC_ddraw()
{
    if (m_hFontInUse != NULL) DeleteObject(m_hFontInUse);
    if (m_lpBackB4flip != NULL) m_lpBackB4flip->Release();
    if (m_lpBackB4 != NULL) m_lpBackB4->Release();
    if (m_lpFrontB4 != NULL) m_lpFrontB4->Release();
    if (m_bFullMode == TRUE) {
        if (m_lpDD4 != NULL) m_lpDD4->RestoreDisplayMode();
    }
    if (m_lpDD4 != NULL) m_lpDD4->Release();
}
```

**Order Matters:**
1. GDI objects (font)
2. Back buffers
3. Front buffer
4. Display mode restoration
5. DirectDraw object

---

## Integration with CSprite

The `CSprite` class holds a pointer to the `DXC_ddraw` instance and uses it extensively:

### Surface Access
```cpp
// From CSprite, creating sprite surface:
pdds4 = m_pDDraw->pCreateOffScreenSurface(m_wBitmapSizeX, m_wBitmapSizeY);
```

### Clip Region
```cpp
// Sprites check clip area before rendering:
if (dX < m_pDDraw->m_rcClipArea.left) { /* clip */ }
if (dX+szx > m_pDDraw->m_rcClipArea.right) { /* clip */ }
```

### Direct Memory Access
```cpp
// Transparency rendering uses direct pixel access:
pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + (dY * m_pDDraw->m_sBackB4Pitch);
```

### Blit Operations
```cpp
// Fast sprite rendering uses BltFast:
m_pDDraw->m_lpBackB4->BltFast(dX, dY, m_lpSurface, &rcRect,
                              DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT);
```

---

## Integration with CGame

The `CGame` class contains a single instance of `DXC_ddraw`:

```cpp
class CGame {
    // ...
    class DXC_ddraw m_DDraw;
    // ...
};
```

### Initialization
```cpp
if (m_DDraw.bInit(m_hWnd) == FALSE) {
    // Handle initialization failure
}
```

### Frame Rendering
```cpp
m_DDraw.ClearBackB4();
// ... render sprites, UI, effects ...
if (m_DDraw.iFlip() == DDERR_SURFACELOST) {
    RestoreSprites();
}
```

### Text Rendering
```cpp
m_DDraw._GetBackBufferDC();
m_DDraw.TextOut(x, y, text, color);
m_DDraw._ReleaseBackBufferDC();
```

### Background Tile Rendering
```cpp
// Temporarily expand clip area for pre-drawn background
SetRect(&m_DDraw.m_rcClipArea, 0, 0, 640+32, 480+32);
// Render tiles to m_DDraw.m_lpPDBGS
// Restore clip area
SetRect(&m_DDraw.m_rcClipArea, 0, 0, 640, 480);
// Copy pre-drawn BG to back buffer
m_DDraw.m_lpBackB4->BltFast(0, 0, m_DDraw.m_lpPDBGS, &rcRect, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
```

---

## Global Transparency Tables

In addition to member variables, there are global versions of the transparency tables used by `CSprite`:

```cpp
// Declared in DXC_ddraw.cpp as extern:
extern long G_lTransG100[64][64], G_lTransRB100[64][64];
extern long G_lTransG70[64][64], G_lTransRB70[64][64];
extern long G_lTransG50[64][64], G_lTransRB50[64][64];
extern long G_lTransG25[64][64], G_lTransRB25[64][64];
extern long G_lTransG2[64][64], G_lTransRB2[64][64];
```

These are initialized in `bInit()` alongside the member tables:
```cpp
G_lTransRB100[iD][iS] = _CalcMaxValue(iS, iD, 'R', 1, 1.0f);
G_lTransG100[iD][iS]  = _CalcMaxValue(iS, iD, 'G', 1, 1.0f);
// ... etc
```

**Purpose:** Allows `CSprite` methods to access tables without going through the `DXC_ddraw` pointer, potentially improving cache locality during intensive sprite rendering.

---

## Conditional Compilation Flags

| Flag | Effect |
|------|--------|
| `_DEBUG` | Default to windowed mode |
| `DEF_USING_WIN_IME` | Use Blt instead of Flip for GDI compatibility |
| `DEF_HTMLCOMMOM` | Support HTML dialog overlay rendering |
| `DEF_LANGUAGE` | Select font and charset (1-5) |

---

## Constants and Limits

| Constant | Value | Description |
|----------|-------|-------------|
| Display Width | 640 | Fixed display width in pixels |
| Display Height | 480 | Fixed display height in pixels |
| Color Depth | 16 | Bits per pixel |
| Back Buffer Count | 1 | Single back buffer for flip chain |
| Pre-draw BG Width | 672 | 640 + 32 for scrolling margin |
| Pre-draw BG Height | 512 | 480 + 32 for scrolling margin |

---

## Known Limitations

1. **Fixed Resolution:** Hardcoded to 640x480 @ 16bpp
2. **No Hardware Alpha:** All transparency is software-rendered
3. **Single Thread:** No synchronization primitives
4. **Surface Lost:** Caller must handle DDERR_SURFACELOST
5. **Limited Color Depth:** Only supports 16-bit color modes
6. **No Stretching:** Surfaces must be rendered at 1:1 scale

---

## Modernization Considerations

When porting to modern graphics APIs:

1. **Replace DirectDraw with:**
   - Direct3D 11/12 (Windows)
   - SDL2 + SDL_Renderer (Cross-platform)
   - SFML (Cross-platform)

2. **Transparency tables can be replaced with:**
   - GPU shader-based alpha blending
   - Pre-multiplied alpha textures
   - Hardware blend states

3. **Text rendering can use:**
   - DirectWrite (Windows)
   - FreeType + texture atlas
   - SDL_ttf

4. **Consider:**
   - Resolution independence
   - Aspect ratio correction
   - VSync and frame pacing
   - High-DPI support
