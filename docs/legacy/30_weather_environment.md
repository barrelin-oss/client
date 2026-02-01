# Weather and Environment System

## Overview

The Weather and Environment system handles atmospheric effects (rain, snow), day/night cycle lighting, and environmental gameplay impacts. Weather is server-controlled and broadcast to all players on a map simultaneously. The system supports 7 weather types and 3 lighting states, with up to 600 concurrent weather particle objects.

Weather is purely visual except for a magic accuracy penalty that scales with weather severity.

## Source Files

| File | Purpose |
|------|---------|
| `Game.h` | Weather object array, state variables, constants |
| `Game.cpp` | `DrawWhetherEffects()`, `SetWhetherStatus()`, weather handlers |
| `NetMessages.h` | Weather/time change notification message IDs |
| `Sprite.cpp` | Day/night alpha blending application |

## Key Data Structures

### Weather Object Array

```cpp
// Game.h, line 593
struct {
    short sX, sY;    // World-space coordinates
    char cStep;      // Animation step counter
} m_stWhetherObject[DEF_MAXWHETHEROBJECTS];
```

Each weather object represents a single particle (raindrop or snowflake). The `cStep` field controls:
- **Rain**: Values -40 to 25 (falling: 0-20, splash: 20-25)
- **Snow**: Values -10 to 80 (falling: 0-80, then reset)

Negative values provide random initialization delays to stagger particle spawns.

### Weather State Variables

```cpp
// Game.h
char m_cWhetherStatus;        // Line 873 - Current weather type (0-6)
char m_cWhetherEffectType;    // Line 872 - Active effect type for rendering
BOOL m_bIsWhetherEffect;      // Line 715 - Weather currently active flag
DWORD m_dwWOFtime;            // Line 680 - Last weather frame update timestamp
```

### Global Lighting Variable

```cpp
// Game.cpp, line 33
extern char G_cSpriteAlphaDegree;  // 0=day, 1=dawn/dusk, 2=night
```

This global affects sprite rendering throughout the entire codebase (50+ references in Sprite.cpp).

## Weather Types

| Value | Type | Particles | Description |
|-------|------|-----------|-------------|
| 0 | Clear | 0 | No weather effects |
| 1 | Light Rain | 120 | Sparse rainfall |
| 2 | Medium Rain | 300 | Moderate rainfall |
| 3 | Heavy Rain | 600 | Dense rainfall with sound |
| 4 | Light Snow | 120 | Sparse snowfall |
| 5 | Medium Snow | 300 | Moderate snowfall |
| 6 | Heavy Snow | 600 | Dense snowfall |

**Particle Count Formula:**
- Light (1, 4): `DEF_MAXWHETHEROBJECTS / 5` = 120
- Medium (2, 5): `DEF_MAXWHETHEROBJECTS / 2` = 300
- Heavy (3, 6): `DEF_MAXWHETHEROBJECTS` = 600

## Day/Night Cycle

| Value | State | Visual Effect |
|-------|-------|---------------|
| 0 | Day | Full brightness (100% opacity) |
| 1 | Dawn/Dusk | Transition lighting |
| 2 | Night | Reduced brightness via alpha blending |

The server sends time change notifications; the client applies global alpha to all sprites.

## Core Functions

### DrawWhetherEffects()

```cpp
// Game.cpp, line 23085
void CGame::DrawWhetherEffects()
```

Main rendering function called every frame when weather is active.

**Rain Rendering Logic (Types 1-3):**
```cpp
// Game.cpp, lines 23098-23118, 23190-23219
if (cStep >= 0 && cStep < 20) {
    // Falling animation - frames 16-19
    iFrame = 16 + (cStep % 4);
    // Position update: fall down-left
    sY += (40 - cStep);
    sX -= 1;
}
else if (cStep >= 20 && cStep <= 25) {
    // Splash animation - frames 20-25
    iFrame = cStep;
}
```

**Snow Rendering Logic (Types 4-6):**
```cpp
// Game.cpp, lines 23124-23170, 23225-23256
if (cStep >= 0 && cStep < 80) {
    // Falling animation - frames 39-50
    iFrame = 39 + (cStep % 12);
    // Position update: slow drift with horizontal waver
    sY += (80 - cStep) / 10;
    sX += 1 - (rand() % 3);  // -1, 0, or +1 horizontal drift
}
```

**Sprite Asset:**
- Effect sprite index 11 (`m_pEffectSpr[11]`)
- Loaded from "effect4" PAK file
- Rain frames: 16-25
- Snow frames: 39-50

### SetWhetherStatus()

```cpp
// Game.cpp, lines 23258-23285
void CGame::SetWhetherStatus(BOOL bStart, char cType)
```

Activates or deactivates weather effects.

**Activation (bStart == TRUE):**
1. Sets `m_bIsWhetherEffect = TRUE`
2. Sets `m_cWhetherEffectType = cType`
3. Starts rain sound loop for types 1-3: `m_pESound[38]->Play(TRUE)`
4. Initializes all 600 weather objects with random delays
5. Starts background music for snow types 4-6

**Deactivation (bStart == FALSE):**
1. Sets `m_bIsWhetherEffect = FALSE`
2. Clears `m_cWhetherEffectType`
3. Stops rain sound: `m_pESound[38]->bStop()`

### NotifyMsg_WhetherChange()

```cpp
// Game.cpp, lines 25152-25164
void CGame::NotifyMsg_WhetherChange(char * pData)
{
    m_cWhetherStatus = *(pData + DEF_INDEX2_MSGTYPE + 2);

    if (m_cWhetherStatus != NULL)
        SetWhetherStatus(TRUE, m_cWhetherStatus);
    else
        SetWhetherStatus(FALSE, NULL);
}
```

Handles server weather change notifications. Extracts 1-byte weather type from packet.

### NotifyMsg_TimeChange()

```cpp
// Game.cpp, lines 25166-25181
void CGame::NotifyMsg_TimeChange(char * pData)
{
    G_cSpriteAlphaDegree = *(pData + DEF_INDEX2_MSGTYPE + 2);

    switch (G_cSpriteAlphaDegree) {
    case 1:  PlaySound('E', 32, 0); break;  // Dusk sound
    case 2:  PlaySound('E', 31, 0); break;  // Night sound
    }

    m_cGameModeCount = 1;
    m_bIsRedrawPDBGS = TRUE;  // Force full screen redraw
}
```

Handles day/night transitions. Updates global lighting and plays ambient sounds.

## Constants & Limits

```cpp
// Game.h, line 89
#define DEF_MAXWHETHEROBJECTS 600    // Max weather particles

// NetMessages.h
#define DEF_NOTIFY_WHETHERCHANGE 0x0B4D   // Weather change notification
#define DEF_NOTIFY_TIMECHANGE    0x0B41   // Day/night change notification

// Sound indices
#define SOUND_RAIN_LOOP    38    // Looping rain ambient
#define SOUND_NIGHT        31    // Night transition
#define SOUND_DUSK         32    // Dusk transition

// Effect sprite index
#define EFFECT_WEATHER_SPRITE  11   // m_pEffectSpr[11] - "effect4" PAK
```

**Animation Frame Ranges:**
| Effect | Start Frame | End Frame | Cycle Length |
|--------|-------------|-----------|--------------|
| Rain falling | 16 | 19 | 4 frames |
| Rain splash | 20 | 25 | 6 frames |
| Snow falling | 39 | 50 | 12 frames |

**Timing:**
- Weather update interval: 30ms minimum (`m_dwWOFtime` throttle)
- Rain cycle: ~25 steps total (20 falling + 5 splash)
- Snow cycle: 80 steps total

## Weather Object Spawning

**Initial Position Calculation:**
```cpp
// X position (Game.cpp)
sX = (m_pMapData->m_sPivotX * 32) + ((rand() % 940) - 200) + 300;

// Y position - Rain
sY = (m_pMapData->m_sPivotY * 32) + ((rand() % 800) - 600) + 240;

// Y position - Snow
sY = (m_pMapData->m_sPivotY * 32) + ((rand() % 800) - 600) + 600;
```

Objects spawn above the visible area and fall through the viewport. When a particle completes its cycle, it respawns at a new random position.

## Gameplay Integration

### Magic Accuracy Penalty

Weather reduces spell casting accuracy:

```cpp
// Game.cpp, lines 39519-39524
switch (m_cWhetherStatus) {
case 0: break;                      // Clear: no penalty
case 1: iResult -= (iResult / 24);  // Light: ~4% penalty
case 2: iResult -= (iResult / 12);  // Medium: ~8% penalty
case 3: iResult -= (iResult / 5);   // Heavy: ~20% penalty
}
```

**Note:** Only rain types (1-3) apply penalties in this code. Snow types (4-6) would fall through without penalty unless handled elsewhere.

### Equipment Mitigation

Special equipment attributes can reduce weather penalties:
- Attribute bits `0x00F00000` - `0x000F0000` indicate weather resistance

## Integration Points

### Network Protocol

| Message | ID | Direction | Payload |
|---------|-----|-----------|---------|
| Weather Change | 0x0B4D | Server → Client | 1 byte weather type |
| Time Change | 0x0B41 | Server → Client | 1 byte time value |

Weather is entirely server-controlled. All players on the same map see identical weather.

### Rendering Pipeline

```
Main Game Loop (Game.cpp:35118)
    └── DrawWhetherEffects()
            ├── Check m_bIsWhetherEffect flag
            ├── Loop through m_stWhetherObject[600]
            ├── Calculate screen position relative to viewport
            ├── Select animation frame based on cStep
            └── Draw sprite with m_pEffectSpr[11]
```

Weather renders after terrain/objects but before UI overlay.

### Audio Integration

| Sound Index | Trigger | Loop |
|-------------|---------|------|
| 38 | Rain types 1-3 active | Yes |
| 31 | Night transition | No |
| 32 | Dusk transition | No |

Sound respects global `m_bSoundStat` and `m_bSoundFlag` settings.

## State Management

### Initialization

```cpp
// Game.cpp, line 5066 (constructor)
// Weather objects zeroed
m_cWhetherStatus = NULL;  // No weather on startup
```

### Cleanup / Logout

```cpp
// Game.cpp, lines 35338, 43536
m_pESound[38]->bStop();   // Stop rain sound
m_bIsWhetherEffect = FALSE;
```

Weather effects are stopped when disconnecting or logging out.

## Christmas Special Mode

When compiled with `DEF_XMAS` flag:

```cpp
// Game.cpp, lines 23130-23168
if (sY == 425) {
    // Snow accumulates at bottom of screen
    // Special particle tracking with up to 1000 particles
    // Particles remain visible after "landing"
}
```

This creates an accumulating snow effect at the bottom of the screen during holiday events.

## Known Issues / Technical Debt

1. **Sparse Particle Density**: 600 particles may appear sparse on modern high-resolution displays
2. **No Localized Weather**: Weather is always fullscreen; no support for weather zones
3. **No Wind Simulation**: Only basic horizontal drift for snow
4. **Hardcoded Christmas Mode**: Requires recompilation to enable/disable
5. **Integer Division**: Weather penalty calculations use integer math, losing precision
6. **Snow Penalty Missing**: Only rain types apply magic accuracy penalty in visible code
7. **Global State**: `G_cSpriteAlphaDegree` is a global variable affecting all rendering
8. **No Weather Transitions**: Weather changes instantly, no gradual fade in/out

## Modernization Notes

### Recommended C++20 Approach

```cpp
namespace hb::world {

enum class WeatherType : uint8_t {
    Clear = 0,
    LightRain = 1,
    MediumRain = 2,
    HeavyRain = 3,
    LightSnow = 4,
    MediumSnow = 5,
    HeavySnow = 6
};

enum class TimeOfDay : uint8_t {
    Day = 0,
    Twilight = 1,
    Night = 2
};

struct WeatherParticle {
    Vec2 position;
    float animationProgress;  // 0.0 to 1.0
    bool active;
};

class WeatherSystem {
public:
    void setWeather(WeatherType type);
    void setTimeOfDay(TimeOfDay time);
    void update(float deltaTime);
    void render(gfx::IRenderer& renderer);

    [[nodiscard]] float getMagicAccuracyModifier() const;

private:
    std::vector<WeatherParticle> m_particles;
    WeatherType m_currentWeather = WeatherType::Clear;
    TimeOfDay m_timeOfDay = TimeOfDay::Day;
    float m_transitionProgress = 1.0f;  // For smooth transitions
};

}
```

### Improvements to Consider

1. **Particle Pooling**: Use object pool for weather particles
2. **GPU Particles**: Move particle simulation to compute shader
3. **Weather Transitions**: Gradual fade between weather states
4. **Localized Weather**: Support weather zones on maps
5. **Wind System**: Add wind direction/speed affecting particles
6. **Weather Events**: Thunderstorms with lightning, blizzards with visibility reduction
7. **Configuration**: Runtime particle density settings
8. **Separate Lighting System**: Decouple day/night from weather system
