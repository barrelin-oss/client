# DirectInput 7 Input System

## Overview

The Helbreath client uses **DirectInput 7** exclusively for mouse input, while relying on standard **Windows message handling** for keyboard input. This hybrid approach was common in games of this era, as DirectInput provided more precise and lower-latency mouse control (especially important for games requiring pixel-accurate cursor positioning), while Windows messages were sufficient and easier to implement for keyboard handling.

The input system also includes a sophisticated text input subsystem supporting Asian Input Method Editors (IME) for Korean, Chinese, and Japanese character entry.

---

## File Structure

| File | Purpose |
|------|---------|
| `DXC_dinput.h` | DirectInput wrapper class declaration |
| `DXC_dinput.cpp` | DirectInput wrapper implementation |
| `MouseInterface.h` | Clickable rectangle UI interface declaration |
| `MouseInterface.cpp` | Clickable rectangle UI implementation |
| `ActionID.h` | Character action/motion constants |
| `Wmain.cpp` | Windows message loop and keyboard event routing |
| `Game.cpp` / `Game.h` | Input processing, command handling, text input |

---

## DirectInput Mouse Wrapper (DXC_dinput)

### Class Declaration

```cpp
class DXC_dinput
{
public:
    DXC_dinput();
    virtual ~DXC_dinput();
    void UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB);
    void SetAcquire(BOOL bFlag);
    BOOL bInit(HWND hWnd, HINSTANCE hInst);

    DIMOUSESTATE dims;           // DirectInput mouse state structure
    IDirectInput *       m_pDI;  // DirectInput interface pointer
    IDirectInputDevice * m_pMouse; // Mouse device interface
    short m_sX, m_sY, m_sZ;      // Accumulated mouse position
};
```

### Initialization (`bInit`)

```cpp
BOOL DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
    HRESULT hr;
    DIMOUSESTATE dims;
    POINT Point;

    // Get initial cursor position from Windows
    GetCursorPos(&Point);
    m_sX = (short)(Point.x);
    m_sY = (short)(Point.y);

    // Create DirectInput object (version 0x0700)
    hr = DirectInputCreate(hInst, DIRECTINPUT_VERSION, &m_pDI, NULL);
    if (hr != DI_OK) return FALSE;

    // Create mouse device using system mouse GUID
    hr = m_pDI->CreateDevice(GUID_SysMouse, &m_pMouse, NULL);
    if (hr != DI_OK) return FALSE;

    // Set mouse data format
    hr = m_pMouse->SetDataFormat(&c_dfDIMouse);
    if (hr != DI_OK) return FALSE;

    // Set cooperative level: EXCLUSIVE + FOREGROUND
    // - EXCLUSIVE: Only this application receives mouse input
    // - FOREGROUND: Only receive input when window is focused
    hr = m_pMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if (hr != DI_OK) return FALSE;

    // Initial device acquisition
    if (m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &dims) != DI_OK)
    {
        m_pMouse->Acquire();
    }

    return TRUE;
}
```

### Mouse State Update (`UpdateMouseState`)

```cpp
void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ,
                                   char * pLB, char * pRB)
{
    // Attempt to get device state; re-acquire if lost
    if (m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &dims) != DI_OK)
    {
        m_pMouse->Acquire();
        return;
    }

    // Accumulate relative mouse movement into absolute position
    m_sX += (short)dims.lX;  // X delta
    m_sY += (short)dims.lY;  // Y delta

    // Mouse wheel (Z axis) - only update if non-zero
    if ((short)dims.lZ != 0)
        m_sZ = (short)dims.lZ;

    // Clamp to screen bounds (640x480)
    if (m_sX < 0) m_sX = 0;
    if (m_sY < 0) m_sY = 0;
    if (m_sX > 639) m_sX = 639;
    if (m_sY > 479) m_sY = 479;

    // Output values
    *pX = m_sX;
    *pY = m_sY;
    *pZ = m_sZ;
    *pLB = (char)dims.rgbButtons[0];  // Left button state (0 or 0x80)
    *pRB = (char)dims.rgbButtons[1];  // Right button state (0 or 0x80)
}
```

### Device Acquisition Control (`SetAcquire`)

Called when the application gains/loses focus:

```cpp
void DXC_dinput::SetAcquire(BOOL bFlag)
{
    DIMOUSESTATE dims;

    if (m_pMouse == NULL) return;

    if (bFlag == TRUE) {
        m_pMouse->Acquire();
        m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &dims);
    }
    else {
        m_pMouse->Unacquire();
    }
}
```

### Cleanup (Destructor)

```cpp
DXC_dinput::~DXC_dinput()
{
    if (m_pMouse != NULL) {
        m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = NULL;
    }
    if (m_pDI != NULL) {
        m_pDI->Release();
        m_pDI = NULL;
    }
}
```

### Key Technical Details

| Aspect | Value/Setting |
|--------|---------------|
| DirectInput Version | 0x0700 (DirectInput 7) |
| Device | `GUID_SysMouse` (system mouse) |
| Data Format | `c_dfDIMouse` (standard mouse) |
| Cooperative Level | `DISCL_EXCLUSIVE | DISCL_FOREGROUND` |
| Screen Bounds | 640x480 (hardcoded) |
| Button Mapping | rgbButtons[0] = Left, rgbButtons[1] = Right |

---

## DIMOUSESTATE Structure

The `DIMOUSESTATE` structure used by DirectInput 7:

```cpp
typedef struct DIMOUSESTATE {
    LONG lX;              // X-axis relative movement
    LONG lY;              // Y-axis relative movement
    LONG lZ;              // Z-axis (mouse wheel) movement
    BYTE rgbButtons[4];   // Button states (0x80 = pressed, 0x00 = released)
} DIMOUSESTATE;
```

| Button Index | Mapping |
|--------------|---------|
| rgbButtons[0] | Left mouse button |
| rgbButtons[1] | Right mouse button |
| rgbButtons[2] | Middle mouse button (unused) |
| rgbButtons[3] | Extra button (unused) |

---

## Windows Keyboard Message Handling

Keyboard input is handled through standard Windows messages in the `WndProc` function (`Wmain.cpp`):

### Message Routing

```cpp
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // First, route to text input handler
    if(G_pGame->GetText(hWnd, message, wParam, lParam))
        return 0;

    switch (message) {
    case WM_KEYDOWN:
        G_pGame->OnKeyDown(wParam);
        return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_KEYUP:
        G_pGame->OnKeyUp(wParam);
        return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_SYSKEYDOWN:
        G_pGame->OnSysKeyDown(wParam);
        return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_SYSKEYUP:
        G_pGame->OnSysKeyUp(wParam);
        return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_ACTIVATEAPP:
        if(wParam == 0) {
            // Window deactivated
            G_pGame->m_bIsProgramActive = FALSE;
            G_pGame->m_DInput.SetAcquire(FALSE);
        }
        else {
            // Window activated
            G_pGame->m_bIsProgramActive = TRUE;
            G_pGame->m_DInput.SetAcquire(TRUE);
            G_pGame->m_bCtrlPressed = FALSE; // Reset modifier state
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_SETCURSOR:
        SetCursor(NULL);  // Hide Windows cursor
        return TRUE;
    // ...
    }
}
```

### Key Messages Handled

| Message | Handler | Purpose |
|---------|---------|---------|
| `WM_KEYDOWN` | `OnKeyDown()` | Regular key press |
| `WM_KEYUP` | `OnKeyUp()` | Regular key release |
| `WM_SYSKEYDOWN` | `OnSysKeyDown()` | Alt+key press |
| `WM_SYSKEYUP` | `OnSysKeyUp()` | Alt+key release |
| `WM_CHAR` | `GetText()` | Character input |
| `WM_IME_*` | `GetText()` | Asian IME input |
| `WM_ACTIVATEAPP` | WndProc | Focus change |

---

## Keyboard Handler Functions

### OnKeyDown (Game.cpp:29187)

Handles initial key press events. Most key actions are deferred to `OnKeyUp` to prevent repeat triggering.

```cpp
void CGame::OnKeyDown(WPARAM wParam)
{
    switch (wParam) {
    case VK_CONTROL:
        m_bCtrlPressed = TRUE;
        break;
    case VK_SHIFT:
        m_bShiftPressed = TRUE;
        break;

    // These keys are ignored on keydown (handled on keyup):
    case VK_INSERT:
    case VK_DELETE:
    case VK_TAB:
    case VK_RETURN:
    case VK_ESCAPE:
    case VK_END:
    case VK_HOME:
    case VK_F1: case VK_F2: case VK_F3: case VK_F4:
    case VK_F5: case VK_F6: case VK_F7: case VK_F8:
    case VK_F9: case VK_F10: case VK_F11: case VK_F12:
    case VK_PRIOR: case VK_NEXT:  // Page Up/Down
    case VK_LWIN: case VK_RWIN:
    case VK_MULTIPLY: case VK_ADD: case VK_SEPARATOR:
    case VK_SUBTRACT: case VK_DECIMAL: case VK_DIVIDE:
    case VK_NUMLOCK: case VK_SCROLL:
        break;  // No action on key down

    default:
        // Any other key in main game mode starts chat input
        if (m_cGameMode == DEF_GAMEMODE_ONMAINGAME) {
            if (m_bCtrlPressed) {
                // Ctrl+0-9: Magic circle shortcuts
                switch (wParam) {
                case 48: EnableDialogBox(3, NULL, NULL, NULL); m_stDialogBoxInfo[3].sView = 9; break; // 0
                case 49: EnableDialogBox(3, NULL, NULL, NULL); m_stDialogBoxInfo[3].sView = 0; break; // 1
                case 50: EnableDialogBox(3, NULL, NULL, NULL); m_stDialogBoxInfo[3].sView = 1; break; // 2
                // ... through 9
                }
            }
            else if ((m_bInputStatus == FALSE) && (GetAsyncKeyState(VK_MENU)>>15 == FALSE)) {
                // Start chat input mode on any regular key press
                StartInputString(10, 414, sizeof(m_cChatMsg), m_cChatMsg);
                ClearInputString();
            }
        }
        break;
    }
}
```

### OnKeyUp (Game.cpp:28610)

Handles the main key action logic (most game actions trigger on key release):

```cpp
void CGame::OnKeyUp(WPARAM wParam)
{
    int i = 0;
    DWORD dwTime = timeGetTime();

    switch (wParam) {
    case VK_SHIFT:
        m_bShiftPressed = FALSE;
        break;
    case VK_CONTROL:
        m_bCtrlPressed = FALSE;
        break;

    // Letter keys with Ctrl modifiers
    case 65: // 'A' - Toggle force attack mode
        if (m_bCtrlPressed && m_cGameMode == DEF_GAMEMODE_ONMAINGAME && (!m_bInputStatus)) {
            m_bForceAttack = !m_bForceAttack;
            AddEventList(m_bForceAttack ? DEF_MSG_FORCEATTACK_ON : DEF_MSG_FORCEATTACK_OFF, 10);
        }
        break;

    case 68: // 'D' - Cycle detail level
        if (m_bCtrlPressed == TRUE && m_cGameMode == DEF_GAMEMODE_ONMAINGAME && (!m_bInputStatus)) {
            m_cDetailLevel++;
            if (m_cDetailLevel > 2) m_cDetailLevel = 0;
            // 0=Low, 1=Medium, 2=High
        }
        break;

    case 77: // 'M' - Toggle minimap
        if (m_cGameMode == DEF_GAMEMODE_ONMAINGAME && m_bCtrlPressed) {
            if (m_bIsDialogEnabled[9] == TRUE) DisableDialogBox(9);
            else EnableDialogBox(9, 0, 0, 0, NULL);
        }
        break;

    case 82: // 'R' - Toggle run/walk mode
        if (m_bCtrlPressed == TRUE && m_cGameMode == DEF_GAMEMODE_ONMAINGAME && (!m_bInputStatus)) {
            m_bRunningMode = !m_bRunningMode;
        }
        break;

    case 83: // 'S' - Cycle sound settings (Music -> Sound -> Off -> Music)
        if (m_bCtrlPressed == TRUE && m_cGameMode == DEF_GAMEMODE_ONMAINGAME && (!m_bInputStatus)) {
            // Toggle: Music On -> Music Off, Sound On -> Sound Off -> Both On
        }
        break;

    case 84: // 'T' - Start whisper to targeted player
        if (m_bCtrlPressed == TRUE && m_cGameMode == DEF_GAMEMODE_ONMAINGAME && (!m_bInputStatus)) {
            // Opens whisper chat to player under cursor or from chat log
        }
        break;

    case 107: // '+' - Zoom map in
        if (m_bInputStatus == FALSE) m_bZoomMap = TRUE;
        break;

    case 109: // '-' - Zoom map out
        if (m_bInputStatus == FALSE) m_bZoomMap = FALSE;
        break;

    // Function keys
    case VK_F1: UseShortCut(0); break;  // or Help dialog
    case VK_F2: UseShortCut(1); break;
    case VK_F3: UseShortCut(2); break;
    case VK_F4: UseMagic(m_sMagicShortCut); break;
    case VK_F5: UseShortCut(3); break;  // or Character dialog
    case VK_F6: UseShortCut(4); break;  // or Inventory dialog
    case VK_F7: /* Magic or Character */ break;
    case VK_F8: /* Skills or Inventory */ break;
    case VK_F9: /* Magic or Chat */ break;
    case VK_F11: /* Chat or Toggle transparency */ break;
    case VK_F12: /* System menu */ break;

    // Quick item use
    case VK_INSERT:
        // Use red potion (HP recovery)
        // Searches inventory for sprite 6, frame 1 or 2
        break;

    case VK_DELETE:
        // Use blue potion (MP recovery)
        // Searches inventory for sprite 6, frame 3 or 4
        break;

    // Navigation
    case VK_END:
        // Restore previous chat message
        break;

    case VK_HOME:
        // Toggle safe attack mode
        bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_TOGGLESAFEATTACKMODE, ...);
        break;

    // Arrow keys (for whisper history navigation)
    case VK_UP:    m_cArrowPressed = 1; /* Previous whisper */ break;
    case VK_RIGHT: m_cArrowPressed = 2; break;
    case VK_DOWN:  m_cArrowPressed = 3; /* Next whisper */ break;
    case VK_LEFT:  m_cArrowPressed = 4; break;

    case VK_TAB:
        // Toggle combat mode
        bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_TOGGLECOMBATMODE, ...);
        break;

    case VK_RETURN:
        m_bEnterPressed = TRUE;
        break;

    case VK_ESCAPE:
        m_bEscPressed = TRUE;
        // Cancel logout countdown, cancel pointing mode, etc.
        break;

    case VK_SNAPSHOT:
        // Take screenshot
        CreateScreenShot();
        break;

    case 33: // Page Up - Activate special ability
        // Sends special ability activation request
        break;
    }
}
```

### OnSysKeyDown / OnSysKeyUp (Game.cpp:28564)

Handles Alt+key combinations (system keys):

```cpp
void CGame::OnSysKeyDown(WPARAM wParam)
{
    switch (wParam) {
    case VK_SHIFT:   m_bShiftPressed = TRUE;  break;
    case VK_CONTROL: m_bCtrlPressed = TRUE;   break;
    case VK_RETURN:  m_bEnterPressed = TRUE;  break;
    }
}

void CGame::OnSysKeyUp(WPARAM wParam)
{
    switch (wParam) {
    case VK_SHIFT:   m_bShiftPressed = FALSE;  break;
    case VK_CONTROL: m_bCtrlPressed = FALSE;   break;
    case VK_RETURN:
        m_bEnterPressed = FALSE;
        if (m_bToggleScreen == TRUE) {
            // Alt+Enter: Toggle fullscreen
            m_bIsRedrawPDBGS = TRUE;
            m_DDraw.ChangeDisplayMode(G_hWnd);
        }
        break;
    case VK_ESCAPE:
        m_bEscPressed = FALSE;
        break;
    case VK_F10:
        // Toggle skills dialog
        break;
    }
}
```

---

## Complete Key Binding Reference

### Modifier Keys

| Key | State Variable | Purpose |
|-----|----------------|---------|
| Shift | `m_bShiftPressed` | Run mode, reverse focus navigation |
| Control | `m_bCtrlPressed` | Shortcut modifier, magic circles |
| Alt | (via `GetAsyncKeyState`) | Super attack mode, fullscreen toggle |

### Function Keys

| Key | Normal Action | With Ctrl |
|-----|---------------|-----------|
| F1 | Shortcut slot 0 / Help | - |
| F2 | Shortcut slot 1 | - |
| F3 | Shortcut slot 2 | - |
| F4 | Cast magic shortcut | - |
| F5 | Shortcut slot 3 / Character dialog | - |
| F6 | Shortcut slot 4 / Inventory dialog | - |
| F7 | Character dialog | - |
| F8 | Inventory dialog | - |
| F9 | Magic dialog / Chat | - |
| F10 | Skills dialog (SysKey) | - |
| F11 | Chat dialog | Toggle dialog transparency |
| F12 | System menu | - |

### Special Keys

| Key | Action |
|-----|--------|
| Tab | Toggle combat mode |
| Enter | Submit text / confirm |
| Escape | Cancel current action / close dialogs |
| Insert | Use red (HP) potion |
| Delete | Use blue (MP) potion |
| Home | Toggle safe attack mode |
| End | Recall previous chat message |
| Page Up | Activate special ability |
| Print Screen | Take screenshot |
| + (Numpad) | Zoom map in |
| - (Numpad) | Zoom map out |

### Ctrl+Letter Combinations

| Combination | Action |
|-------------|--------|
| Ctrl+A | Toggle force attack mode |
| Ctrl+D | Cycle graphics detail level |
| Ctrl+H | Open help dialog |
| Ctrl+M | Toggle minimap |
| Ctrl+R | Toggle run/walk mode |
| Ctrl+S | Cycle sound settings |
| Ctrl+T | Start whisper to target |
| Ctrl+0-9 | Open magic dialog to specific circle |
| Ctrl+F1-F6 | Assign current item/magic to shortcut |

### Alt Combinations

| Combination | Action |
|-------------|--------|
| Alt+Enter | Toggle fullscreen mode |
| Alt+Click | Super attack mode (while held) |

### Arrow Keys

| Key | Action |
|-----|--------|
| Up | Scroll to previous whisper recipient |
| Down | Scroll to next whisper recipient |
| Left/Right | Arrow direction tracking (movement AI) |

---

## Shortcut System (UseShortCut)

The shortcut system allows binding items or spells to F-keys:

```cpp
// num: 0=F1, 1=F2, 2=F3, 3=F5, 4=F6
void CGame::UseShortCut(int num)
{
    int index;
    if (num < 3) index = num + 1;  // F1=1, F2=2, F3=3
    else index = num + 2;          // F5=5, F6=6

    if (m_cGameMode != DEF_GAMEMODE_ONMAINGAME) return;

    if (m_bCtrlPressed == TRUE) {
        // Ctrl+Fn: Assign recent item/magic to shortcut
        if (m_sRecentShortCut == -1) {
            AddEventList(MSG_SHORTCUT1, 10); // "No item/magic selected"
        }
        else {
            m_sShortCut[num] = m_sRecentShortCut;
            // Values 0-99: Item indices
            // Values 100+: Magic indices (subtract 100)
        }
    }
    else {
        // Fn: Use assigned shortcut
        if (m_sShortCut[num] == -1) {
            AddEventList(MSG_SHORTCUT1, 10); // "Nothing assigned"
        }
        else if (m_sShortCut[num] < 100) {
            // Use/equip item
            ItemEquipHandler((char)m_sShortCut[num]);
        }
        else if (m_sShortCut[num] >= 100) {
            // Cast magic
            UseMagic(m_sShortCut[num] - 100);
        }
    }
}
```

### Shortcut Storage

| Variable | Type | Purpose |
|----------|------|---------|
| `m_sShortCut[5]` | short[5] | Shortcut bindings (F1, F2, F3, F5, F6) |
| `m_sRecentShortCut` | short | Most recently used item/magic (-1 if none) |
| `m_sMagicShortCut` | short | F4 magic shortcut |

---

## Mouse Cursor State Machine

The game implements a cursor state machine for handling clicks, drags, and selections:

### Cursor States

```cpp
#define DEF_CURSORSTATUS_NULL      0  // Idle state
#define DEF_CURSORSTATUS_PRESSED   1  // Button held down
#define DEF_CURSORSTATUS_SELECTED  2  // Object selected (dialog/item)
#define DEF_CURSORSTATUS_DRAGGING  3  // Dragging operation in progress
```

### Selected Object Types

```cpp
#define DEF_SELECTEDOBJTYPE_DLGBOX  1  // Dialog box selected
#define DEF_SELECTEDOBJTYPE_ITEM    2  // Item selected
```

### Cursor Structure (m_stMCursor)

```cpp
struct {
    short sX;                    // Current cursor X position
    short sY;                    // Current cursor Y position
    short sCursorFrame;          // Current cursor sprite frame
    char  cPrevStatus;           // Previous cursor state
    char  cSelectedObjectType;   // Type of selected object
    short sSelectedObjectID;     // ID of selected object
    short sPrevX, sPrevY;        // Previous position (for drag detection)
    short sDistX, sDistY;        // Drag offset from object origin
    DWORD dwSelectClickTime;     // Time of last click (double-click detection)
    short sClickX, sClickY;      // Position of last click
} m_stMCursor;
```

### Cursor Frames

| Frame | Meaning |
|-------|---------|
| 0 | Default cursor |
| 3 | Attack cursor (enemy target) |
| 4 | Movement arrow cursor |
| 5 | Movement arrow with direction indicator |
| 6 | Friendly target cursor |
| 8 | Hourglass/wait cursor |
| 10 | Fishing cursor |

### State Transitions

```
NULL ──[LB pressed on dialog]──> SELECTED
NULL ──[LB pressed elsewhere]──> PRESSED
PRESSED ──[LB released]──> NULL
SELECTED ──[LB released]──> NULL (triggers click/double-click)
SELECTED ──[mouse moved]──> DRAGGING
DRAGGING ──[LB released]──> NULL (triggers drop)
```

### Double-Click Detection

```cpp
#define DEF_DOUBLECLICKTIME  300  // 300ms window for double-click

// In CommandProcessor
if (((dwTime - m_stMCursor.dwSelectClickTime) < DEF_DOUBLECLICKTIME) &&
    (msX == m_stMCursor.sClickX) && (msY == m_stMCursor.sClickY)) {
    // Double click detected
    _bCheckDlgBoxDoubleClick(msX, msY);
}
else {
    // Single click
    _bCheckDlgBoxClick(msX, msY);
}
```

---

## Clickable Rectangle System (CMouseInterface)

A helper class for managing clickable UI regions:

### Class Declaration

```cpp
#define DEF_MAXRECTS         30  // Maximum clickable regions
#define DEF_MIRESULT_NONE    0   // No interaction
#define DEF_MIRESULT_PRESS   1   // Button held in region
#define DEF_MIRESULT_CLICK   2   // Click completed in region

class CMouseInterface
{
public:
    int iGetStatus(int msX, int msY, char cLB, char * pResult);
    void AddRect(long sx, long sy, long dx, long dy);
    CMouseInterface();
    virtual ~CMouseInterface();

    RECT * m_pRect[DEF_MAXRECTS];  // Array of clickable regions
    char   m_cPrevPress;           // Index of currently pressed region
    DWORD  m_dwTime;               // Timestamp for timing
};
```

### Usage Pattern

```cpp
// Create interface with button regions
static class CMouseInterface * pMI;
pMI = new class CMouseInterface;
pMI->AddRect(200, 244, 200+74, 244+20);  // Button 1
pMI->AddRect(370, 244, 370+74, 244+20);  // Button 2

// Check for interactions
char cMIresult;
int iMIbuttonNum = pMI->iGetStatus(msX, msY, cLB, &cMIresult);

if (cMIresult == DEF_MIRESULT_CLICK) {
    switch (iMIbuttonNum) {
    case 1: /* Button 1 clicked */ break;
    case 2: /* Button 2 clicked */ break;
    }
}
```

### Click Detection Logic

```cpp
int CMouseInterface::iGetStatus(int msX, int msY, char cLB, char * pResult)
{
    if (cLB != 0) {
        // Button pressed - check if in any region
        for (int i = 1; i < DEF_MAXRECTS; i++) {
            if (m_pRect[i] != NULL) {
                if ((m_pRect[i]->left < msX) && (m_pRect[i]->right > msX) &&
                    (m_pRect[i]->top < msY)  && (m_pRect[i]->bottom > msY)) {
                    m_cPrevPress = i;
                    *pResult = DEF_MIRESULT_PRESS;
                    return i;
                }
            }
        }
    }

    if ((m_cPrevPress != 0) && (cLB == 0)) {
        // Button was pressed and now released - check for click
        if ((m_pRect[m_cPrevPress]->left < msX) && (m_pRect[m_cPrevPress]->right > msX) &&
            (m_pRect[m_cPrevPress]->top < msY)  && (m_pRect[m_cPrevPress]->bottom > msY)) {
            int iRet = m_cPrevPress;
            m_cPrevPress = 0;
            *pResult = DEF_MIRESULT_CLICK;
            return iRet;
        }
    }

    *pResult = DEF_MIRESULT_NONE;
    return 0;
}
```

---

## Command Processor (Game.cpp:35701)

The `CommandProcessor` function translates mouse input into game commands:

### Function Signature

```cpp
void CGame::CommandProcessor(short msX, short msY,    // Screen coordinates
                             short indexX, short indexY, // Tile coordinates
                             char cLB, char cRB)       // Button states
```

### Command Processing Flow

1. **Observer Mode Panning** - Edge scrolling for spectators
2. **Super Attack Mode Check** - Alt key held
3. **Cursor State Machine** - Process click/drag states
4. **Teleport Check** - Handle teleport locations
5. **Item Pickup** - Check for items at player location
6. **Target Selection** - Determine action based on target type
7. **Movement/Attack** - Generate appropriate command

### Game Commands Generated

| Command | Value | Meaning |
|---------|-------|---------|
| `DEF_OBJECTSTOP` | 0 | Stop moving |
| `DEF_OBJECTMOVE` | 1 | Walk to destination |
| `DEF_OBJECTRUN` | 2 | Run to destination |
| `DEF_OBJECTATTACK` | 3 | Attack target |
| `DEF_OBJECTMAGIC` | 4 | Cast magic |
| `DEF_OBJECTGETITEM` | 5 | Pick up item |
| `DEF_OBJECTDAMAGE` | 6 | Take damage (reaction) |
| `DEF_OBJECTDAMAGEMOVE` | 7 | Knockback movement |
| `DEF_OBJECTATTACKMOVE` | 8 | Dash attack |
| `DEF_OBJECTDYING` | 10 | Death animation |
| `DEF_OBJECTNULLACTION` | 100 | No action |
| `DEF_OBJECTDEAD` | 101 | Dead state |

---

## Text Input System

### Input State Variables

```cpp
BOOL m_bInputStatus;           // TRUE when accepting text input
char * m_pInputBuffer;         // Pointer to destination buffer
int m_iInputX, m_iInputY;      // Display position
unsigned char m_cInputMaxLen;  // Maximum input length
char m_cEdit[4];               // IME composition buffer (non-Win IME)
HWND G_hEditWnd;               // RichEdit control handle (Win IME)
```

### StartInputString

Begins text input mode:

```cpp
void CGame::StartInputString(int sX, int sY, unsigned char iLen,
                             char * pBuffer, BOOL bIsHide)
{
    m_bInputStatus = TRUE;
    m_iInputX = sX;
    m_iInputY = sY;
    m_pInputBuffer = pBuffer;
    ZeroMemory(m_cEdit, sizeof(m_cEdit));
    m_cInputMaxLen = iLen;

#ifdef DEF_USING_WIN_IME
    // Create RichEdit control for IME support
    if (bIsHide == FALSE)
        G_hEditWnd = CreateWindow(RICHEDIT_CLASS, NULL,
            WS_POPUP | ES_SELFIME, sX-5, sY-1, iLen*12, 16,
            G_hWnd, (HMENU)0, G_hInstance, NULL);
    else
        G_hEditWnd = CreateWindow(RICHEDIT_CLASS, NULL,
            WS_POPUP | ES_PASSWORD | ES_SELFIME, sX-5, sY-1, iLen*12, 16,
            G_hWnd, (HMENU)0, G_hInstance, NULL);

    SetWindowText(G_hEditWnd, m_pInputBuffer);
    SendMessage(G_hEditWnd, EM_EXLIMITTEXT, 0, iLen-1);

    // Set IME composition window position
    COMPOSITIONFORM composform;
    composform.dwStyle = CFS_POINT;
    composform.ptCurrentPos.x = sX;
    composform.ptCurrentPos.y = sY;
    HIMC hImc = ImmGetContext(G_hWnd);
    ImmSetCompositionWindow(hImc, &composform);
#endif
}
```

### EndInputString

Finishes text input mode:

```cpp
void CGame::EndInputString()
{
    m_bInputStatus = FALSE;

#ifdef DEF_USING_WIN_IME
    if (G_hEditWnd != NULL) {
        GetWindowText(G_hEditWnd, m_pInputBuffer, (int)m_cInputMaxLen);
        DestroyWindow(G_hEditWnd);
        G_hEditWnd = NULL;
    }
#else
    // Append any partial IME characters
    int len = strlen(m_cEdit);
    if (len > 0) {
        m_cEdit[len] = 0;
        strcpy(m_pInputBuffer + strlen(m_pInputBuffer), m_cEdit);
        ZeroMemory(m_cEdit, sizeof(m_cEdit));
    }
#endif
}
```

### GetText (IME Handler)

Two implementations exist:
1. **DEF_USING_WIN_IME** - Uses Windows RichEdit control
2. **Native IME** - Manual IME message handling

#### Windows RichEdit Version

```cpp
bool CGame::GetText(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (m_pInputBuffer == NULL) return FALSE;
    if (G_hEditWnd == NULL) return FALSE;

    switch (msg) {
    case WM_CHAR:
        // Filter special characters
        if ((wparam == 22) || (wparam == 3) || (wparam == 9) || (wparam == 13))
            return TRUE;
        if (strlen(m_pInputBuffer) < m_cInputMaxLen-1)
            SendMessage(G_hEditWnd, msg, wparam, lparam);
        return TRUE;

    case WM_IME_COMPOSITION:
        if (strlen(m_pInputBuffer) < (m_cInputMaxLen-2))
            SendMessage(G_hEditWnd, msg, wparam, lparam);
        return TRUE;

    case WM_IME_CHAR:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CONTROL:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_NOTIFY:
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
    case WM_IME_SELECT:
        SendMessage(G_hEditWnd, msg, wparam, lparam);
        return TRUE;

    case WM_KEYDOWN:
        if (wparam == 8) // Backspace
            SendMessage(G_hEditWnd, msg, wparam, lparam);
        return FALSE;
    }
    return FALSE;
}
```

#### Native IME Version

```cpp
bool CGame::GetText(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int len;
    HIMC hIMC = NULL;

    if (m_pInputBuffer == NULL) return FALSE;

    switch (msg) {
    case WM_IME_COMPOSITION:
        ZeroMemory(m_cEdit, sizeof(m_cEdit));
        if (lparam & GCS_RESULTSTR) {
            // Completed character
            hIMC = ImmGetContext(hWnd);
            len = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
            if (len > 4) len = 4;
            ImmGetCompositionString(hIMC, GCS_RESULTSTR, m_cEdit, len);
            ImmReleaseContext(hWnd, hIMC);

            len = strlen(m_pInputBuffer) + strlen(m_cEdit);
            if (len < m_cInputMaxLen)
                strcpy(m_pInputBuffer + strlen(m_pInputBuffer), m_cEdit);
            ZeroMemory(m_cEdit, sizeof(m_cEdit));
        }
        else if (lparam & GCS_COMPSTR) {
            // Composition in progress
            hIMC = ImmGetContext(hWnd);
            len = ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0);
            if (len > 4) len = 4;
            ImmGetCompositionString(hIMC, GCS_COMPSTR, m_cEdit, len);
            ImmReleaseContext(hWnd, hIMC);

            len = strlen(m_pInputBuffer) + strlen(m_cEdit);
            if (len >= m_cInputMaxLen)
                ZeroMemory(m_cEdit, sizeof(m_cEdit));
        }
        return TRUE;

    case WM_CHAR:
        if (wparam == 8) {
            // Backspace - handle multi-byte characters
            if (strlen(m_pInputBuffer) > 0) {
                len = strlen(m_pInputBuffer);
                switch (GetCharKind(m_pInputBuffer, len-1)) {
                case 1: // ASCII
                    m_pInputBuffer[len-1] = NULL;
                    break;
                case 2: // First byte of double-byte
                case 3: // Second byte of double-byte
                    m_pInputBuffer[len-2] = NULL;
                    m_pInputBuffer[len-1] = NULL;
                    break;
                }
                ZeroMemory(m_cEdit, sizeof(m_cEdit));
            }
        }
        else if ((wparam != 9) && (wparam != 13) && (wparam != 27)) {
            // Regular character
            len = strlen(m_pInputBuffer);
            if (len >= m_cInputMaxLen-1) return FALSE;
            m_pInputBuffer[len] = wparam & 0xff;
            m_pInputBuffer[len+1] = 0;
        }
        return TRUE;
    }
    return FALSE;
}
```

### Character Kind Detection (Multi-byte Support)

```cpp
// Determines if character at index is:
// 1 = ASCII single byte
// 2 = First byte of double-byte character
// 3 = Second byte of double-byte character
int CGame::GetCharKind(char *str, int index)
{
    int kind = 1;
    do {
        if (kind == 2) kind = 3;
        else {
            if ((unsigned char)*str < 128) kind = 1;
            else kind = 2;
        }
        str++;
        index--;
    } while (index >= 0);
    return kind;
}
```

---

## Action ID Constants (ActionID.h)

Character action states used for animation and command processing:

```cpp
#define DEF_TOTALCHARACTERS     80   // Maximum character types
#define DEF_TOTALACTION         15   // Action types per character

// Action IDs
#define DEF_OBJECTSTOP          0    // Standing still
#define DEF_OBJECTMOVE          1    // Walking
#define DEF_OBJECTRUN           2    // Running
#define DEF_OBJECTATTACK        3    // Attacking
#define DEF_OBJECTMAGIC         4    // Casting magic
#define DEF_OBJECTGETITEM       5    // Picking up item
#define DEF_OBJECTDAMAGE        6    // Taking damage
#define DEF_OBJECTDAMAGEMOVE    7    // Knockback
#define DEF_OBJECTATTACKMOVE    8    // Dash attack
#define DEF_OBJECTDYING         10   // Dying animation
#define DEF_OBJECTNULLACTION    100  // No action
#define DEF_OBJECTDEAD          101  // Dead state

// Motion confirmation from server
#define DEF_OBJECTMOVE_CONFIRM          1001
#define DEF_OBJECTMOVE_REJECT           1010
#define DEF_OBJECTMOTION_CONFIRM        1020
#define DEF_OBJECTMOTION_ATTACK_CONFIRM 1030
#define DEF_OBJECTMOTION_REJECT         1040
```

---

## Input State Flags Summary

### Boolean State Variables

| Variable | Purpose |
|----------|---------|
| `m_bIsProgramActive` | Window has focus |
| `m_bInputStatus` | Text input mode active |
| `m_bEnterPressed` | Enter key state |
| `m_bEscPressed` | Escape key state |
| `m_bCtrlPressed` | Control key state |
| `m_bShiftPressed` | Shift key state |
| `m_bRunningMode` | Run mode (vs walk) |
| `m_bIsCombatMode` | Combat mode active |
| `m_bIsSafeAttackMode` | Safe attack mode |
| `m_bSuperAttackMode` | Super attack (Alt held) |
| `m_bForceAttack` | Force attack mode (Ctrl+A) |
| `m_bIsGetPointingMode` | Waiting for point target |
| `m_bCommandAvailable` | Can accept new commands |

### Direction/Arrow State

```cpp
char m_cArrowPressed;  // Arrow key state: 1=Up, 2=Right, 3=Down, 4=Left
char m_cPlayerDir;     // Player facing direction
char m_cPlayerTurn;    // Direction player should turn to
```

---

## Integration with Game Loop

The input system integrates into the main game loop:

```cpp
// In EventLoop() - Wmain.cpp
while (1) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        if (!GetMessage(&msg, NULL, 0, 0)) return;
        TranslateMessage(&msg);  // Generate WM_CHAR from WM_KEYDOWN
        DispatchMessage(&msg);   // Route to WndProc
    }
    else if (G_pGame->m_bIsProgramActive) {
        G_pGame->UpdateScreen();  // Main game update
    }
    else {
        WaitMessage();  // Sleep when inactive
    }
}

// In UpdateScreen_OnGame()
m_DInput.UpdateMouseState(&msX, &msY, &msZ, &cLB, &cRB);
// ... render game world ...
CommandProcessor(msX, msY, indexX, indexY, cLB, cRB);
// ... draw cursor sprite ...
m_pSprite[DEF_SPRID_MOUSECURSOR]->PutSpriteFast(msX, msY, m_stMCursor.sCursorFrame, dwTime);
```

---

## Platform-Specific Considerations

### Focus Handling

When the application loses/gains focus:

```cpp
// WM_ACTIVATEAPP handler
if (wParam == 0) {
    // Lost focus
    G_pGame->m_bIsProgramActive = FALSE;
    G_pGame->m_DInput.SetAcquire(FALSE);  // Release mouse
}
else {
    // Gained focus
    G_pGame->m_bIsProgramActive = TRUE;
    G_pGame->m_DInput.SetAcquire(TRUE);   // Reacquire mouse
    G_pGame->m_bCtrlPressed = FALSE;      // Reset modifier states

    // Security checks on reactivation
    bCheckImportantFile();
    __FindHackingDll__("CRCCHECK");
}
```

### Cursor Hiding

```cpp
// WM_SETCURSOR handler
case WM_SETCURSOR:
    SetCursor(NULL);  // Hide Windows cursor
    return TRUE;      // Prevent Windows from setting cursor
```

### Screen Bounds

The mouse position is clamped to 640x480 (the game's native resolution):

```cpp
if (m_sX < 0) m_sX = 0;
if (m_sY < 0) m_sY = 0;
if (m_sX > 639) m_sX = 639;
if (m_sY > 479) m_sY = 479;
```

---

## Limitations and Quirks

1. **Hardcoded Resolution**: Mouse bounds fixed at 640x480
2. **No Keyboard DirectInput**: Only mouse uses DirectInput
3. **Exclusive Mouse Mode**: Other applications cannot access mouse
4. **IME Complexity**: Two separate implementations for IME support
5. **State Reset Issues**: Modifier keys can get "stuck" after Alt+Tab
6. **No Gamepad Support**: Only keyboard and mouse supported
7. **Button Polling**: Button states are polled, not event-driven
8. **Index Starting at 1**: CMouseInterface rects array starts at index 1

---

## Dependencies

| Dependency | Purpose |
|------------|---------|
| DirectInput 7 SDK | `dinput.h`, `dinput.lib` |
| DirectInput GUIDs | `dxguid.lib` |
| Windows IME | `imm32.lib` |
| RichEdit | `Riched20.dll` (for DEF_USING_WIN_IME) |

---

## See Also

- [01_cgame_monolithic_class.md](01_cgame_monolithic_class.md) - CGame class overview
- [03_game_state_machine.md](03_game_state_machine.md) - Game state handling
- [DirectDraw 7 Documentation](04_directdraw7_graphics.md) - Graphics system
- [Dialog System Documentation](13_dialog_system.md) - UI interaction handling
