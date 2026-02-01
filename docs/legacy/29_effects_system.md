# Effects System

## Overview

The Effects System manages all visual effects in the Helbreath client, including spell explosions, projectiles, particles, environmental effects, and combat feedback. Effects are short-lived animated objects that provide visual feedback for game events like spellcasting, attacks, and item interactions.

The system supports up to **300 concurrent effects** (`DEF_MAXEFFECTS`) using **100 effect sprite types** (`DEF_MAXEFFECTSPR`). Effects are frame-based animations with support for:
- Projectile movement (arrows, magic missiles, fireballs)
- Stationary explosions and bursts
- Particle systems (blood, sparks, smoke)
- Camera shaking
- Light emission
- Transparency/fade effects

## Source Files

| File | Purpose |
|------|---------|
| `Effect.h` | CEffect class definition - effect data structure |
| `Effect.cpp` | CEffect constructor/destructor |
| `Game.h` | Effect array declarations, constants |
| `Game.cpp` | All effect logic: creation, update, rendering, cleanup |

**Note**: The `CEffect` class is a simple data container. All logic resides in `CGame` methods.

## Key Data Structures

### CEffect Class

```cpp
class CEffect {
public:
    CEffect();
    virtual ~CEffect();

    short m_sType;              // Effect type ID (1-200+)
    char  m_cFrame;             // Current animation frame (-1 = not started)
    char  m_cMaxFrame;          // Maximum frame count
    char  m_cDir;               // Direction (for projectiles, 1-8)
    DWORD m_dwTime;             // Last update timestamp
    DWORD m_dwFrameTime;        // Milliseconds per frame

    // Position tracking
    int   m_sX, m_sY;           // Source tile coordinates
    int   m_dX, m_dY;           // Destination tile coordinates
    int   m_mX, m_mY;           // Current pixel position
    int   m_mX2, m_mY2;         // Previous pixel position (for trails)
    int   m_mX3, m_mY3;         // Additional position data

    // Physics/movement
    int   m_iErr;               // Bresenham error accumulator
    int   m_rX, m_rY;           // Velocity/random offset
    int   m_iV1;                // Generic value (damage, distance, etc.)
};
```

### Effect Storage in CGame

```cpp
// Game.h
#define DEF_MAXEFFECTSPR    100     // Maximum effect sprite types
#define DEF_MAXEFFECTS      300     // Maximum concurrent effects (was 600)

class CGame {
    class CSprite  * m_pEffectSpr[DEF_MAXEFFECTSPR];  // Effect sprite resources
    class CEffect * m_pEffectList[DEF_MAXEFFECTS];     // Active effect instances

    int m_iCameraShakingDegree;  // Current camera shake intensity
};
```

## Effect Types Catalog

### Basic Effects (1-19)

| Type | Name | Description | Frames | Frame Time |
|------|------|-------------|--------|------------|
| 1 | Sword Slash | Melee weapon trail | 2 | 10ms |
| 2 | Arrow | Flying arrow projectile | 0 (continuous) | 10ms |
| 4 | Gold Drop | Coin drop animation | 12 | 100ms |
| 5 | Fire Explosion | Fireball impact | 11 | 10ms |
| 6 | Energy Bolt | Lightning burst | 14 | 10ms |
| 7 | Magic Missile Exp | Magic missile impact | 5 | 50ms |
| 8 | Burst Type 1 | Particle burst (stationary) | 4 | 30ms |
| 9 | Burst Type 2 | Particle burst (physics) | 14 | 30ms |
| 10 | Lightning Arrow | Lightning projectile impact | 14 | 10ms |
| 11 | Blood Burst | Blood splatter particles | 8 | 30ms |
| 12 | Fire Burst | Flame particles | 10 | 30ms |
| 13 | Smoke Rising | Smoke column | 18 | 20ms |
| 14 | Dust Cloud | Ground dust | 4 | 100ms |
| 15 | Fire Trail | Flame afterimage | 16 | 80ms |
| 16 | Fire Strike | Moving fire projectile | 0 (continuous) | 20ms |
| 17 | Ice Storm Fragment | Ice shard particle | - | 20ms |
| 18 | Ground Shake | Tremor visual | 10 | 50ms |

### Projectile Effects (20-27)

| Type | Name | Description |
|------|------|-------------|
| 20-27 | Magic Projectiles | Various flying magical attacks |

All share: `m_cMaxFrame = 0`, `m_dwFrameTime = 10ms`, calculate direction via `cCalcDirection()`

### Spell Effects (30-77)

| Type | Name | Frames | Notes |
|------|------|--------|-------|
| 30 | Mass-Fire-Strike Main | 9 | Camera shake x2 |
| 31 | Mass-Fire-Strike Secondary | 8 | Camera shake |
| 32 | Breaking Effect | 4 | When armor breaks |
| 33 | Mass Magic Attack | 16 | RGB tinting |
| 34 | Moving Ice Bolt | 0 | Projectile with camera shake |
| 40 | Chill Wind | 15 | Ice effect |
| 41-44 | Meteor Large 1-4 | 14 | Shadow + falling animation |
| 45-46 | Meteor Small 1-2 | 14 | Smaller meteors |
| 47-49 | Blizzard Ice 1-3 | 12 | Ice crystals falling |
| 50 | Meteor Impact | 12 | Ground explosion |
| 51 | Chill Wind Aftermath | 9 | Ice ground effect |
| 52 | Protection Ring | 15 | Protective aura |
| 53 | Hold Twist | 15 | Paralysis visual |
| 54-55 | Star Twinkle | 10 | Sparkle effect |
| 56 | Mass Chill Wind | 14 | Area ice |
| 57 | Casting Effect | 16 | Generic spell cast |
| 60 | Meteor Strike Prepare | 10 | Warning animation |
| 61 | Meteor Strike Explosion | 16 | Main impact |
| 62 | Dark Cloud | 6 | Darkness effect |
| 63 | Lightning Strike | 16 | Thunder visual |
| 64 | Resurrection Effect | 15 | Revival glow |
| 65 | Moving Dark Cloud | 30 | Traveling darkness |
| 66 | Earth Quake | 14 | Ground shake visual |
| 67 | Fire Pillar | 27 | Vertical flame |
| 68 | Worm Bite | 17 | Monster attack |
| 69-70 | Surface Fire 1-2 | 11 | Ground flames |
| 71 | Moving Ice Bolt | 0 | Ice projectile |
| 72 | Blizzard Large Impact | 15 | Major ice explosion |
| 73-74 | Light Effects | 15-19 | Glow animations |
| 75-77 | Directional Effects | 16 | Use m_dX/m_dY for direction |

### Magic Spell Effects (100-200+)

These map to spell IDs and often trigger other effects:

| Type | Spell | Description |
|------|-------|-------------|
| 100 | Magic Missile | Flying light projectile |
| 101 | Heal | Healing glow (14 frames) |
| 102 | Create Food | Sparkle effect (13 frames) |
| 110 | Energy Bolt | Electric projectile |
| 111 | Stamina Drain | Energy transfer (14 frames) |
| 112 | Recall | Teleport visual (12 frames) |
| 113 | Defense Shield | Protective barrier (12 frames) |
| 114 | Celebrating Light | Spawns multiple fire effects |
| 120 | Fire Ball | Directional fire projectile |
| 121 | Great Heal | Stronger healing (14 frames) |
| 122 | Recall | Teleport (13 frames) |
| 123-128 | Stamina Recovery | Various recovery spells |
| 124 | Protection from NM | Triggers type 52 |
| 125 | Hold Person | Triggers type 53 |
| 130 | Fire Strike | Fire projectile |
| 131 | Summon | Summoning circle (12 frames) |
| 132 | Invisibility | Fade effect (12 frames) |
| 133 | Protection Magic | Triggers type 52 |
| 134 | Detect Invisibility | Reveal effect |
| 135 | Paralyze | Triggers type 53 |
| 136 | Cure | Healing (13 frames) |
| 137 | Lightning Arrow | Electric projectile |
| 138 | Tremor | Camera shake + 14 dust effects |
| 142 | Confuse Language | Confusion visual |
| 144 | Great Defense Shield | Large barrier |
| 150 | Berserk | Rage aura (11 frames) |
| 152-153 | Mass Poison | Area poison |
| 162 | Confusion | Mind effect |
| 165 | Special Effect | v2.16 addition |
| 171 | Mass Confusion | Area confusion |
| 180 | Illusion | Transform visual (11 frames) |
| 181 | Special Meteor | Lightning strike variant |
| 190 | Mass Illusion | Area illusion |

## Core Functions

### Effect Creation

```cpp
void CGame::bAddNewEffect(
    short sType,        // Effect type ID
    int sX, int sY,     // Source position (tile or pixel depending on type)
    int dX, int dY,     // Destination position
    char cStartFrame,   // Starting frame (often 0 or negative for delay)
    int iV1 = 1         // Generic value (attacker height, count, etc.)
);
```

**Key behaviors:**
- Finds first NULL slot in `m_pEffectList[]`
- Creates new `CEffect` instance
- Initializes type-specific properties (position, frames, timing)
- Calculates direction for projectiles via `m_Misc.cCalcDirection()`
- Plays associated sound effects via `PlaySound()`
- Triggers camera shake via `SetCameraShakingEffect()`
- **Detail level filtering**: At low detail (`m_cDetailLevel == 0`), skips particle effects (types 8, 9, 11, 12, 14, 15)

### Effect Rendering

```cpp
void CGame::DrawEffects();       // Main effect rendering
void CGame::DrawEffectLights();  // Light/glow overlay pass
```

**DrawEffects()** iterates through `m_pEffectList[]` and renders each active effect:
- Calculates screen position: `dX = m_mX - m_sViewPointX`
- Selects appropriate sprite from `m_pEffectSpr[]`
- Chooses render method based on effect type:
  - `PutSpriteFast()` - No transparency
  - `PutTransSprite_NoColorKey()` - Standard transparency
  - `PutTransSprite25/50/70_NoColorKey()` - Partial alpha
  - `PutTransSpriteRGB()` - Color tinting (for fade-out)
  - `PutFadeSprite()` - Shadows
  - `PutRevTransSprite()` - Reversed/flipped

**DrawEffectLights()** adds glow effects for light-emitting types:
- Renders light halos at effect positions
- Creates ambient lighting for fire/magic effects

### Effect Update Loop

Located around line 15893 in Game.cpp:

```cpp
for (i = 0; i < DEF_MAXEFFECTS; i++)
if (m_pEffectList[i] != NULL) {
    // Check if frame time elapsed
    if ((dwTime - m_pEffectList[i]->m_dwTime) > m_pEffectList[i]->m_dwFrameTime) {
        m_pEffectList[i]->m_dwTime = dwTime;
        m_pEffectList[i]->m_cFrame++;

        // Store previous position for trails
        m_pEffectList[i]->m_mX2 = m_pEffectList[i]->m_mX;
        m_pEffectList[i]->m_mY2 = m_pEffectList[i]->m_mY;

        // Type-specific update logic...
    }
}
```

**Update behaviors by type:**

1. **Projectiles** (types 2, 16, 20-27, 34, 71, 100, 110, 120, 130, 137):
   - Use `m_Misc.GetPoint()` for Bresenham line movement
   - Delete when reaching destination
   - May spawn child effects on impact

2. **Physics particles** (types 9, 11, 12):
   - Apply velocity: `m_mX += m_rX`, `m_mY += m_rY`
   - Apply gravity: `m_rY++`
   - Delete when exceeding max frame

3. **Static animations** (most types):
   - Simply increment frame
   - Delete when `m_cFrame > m_cMaxFrame`

4. **Composite effects** (types 1, 5, 6, 7, 10, 114, 138):
   - Spawn child particle effects at specific frames
   - Example: Fire explosion spawns 5 fire bursts at frame 1

### Effect Cleanup

```cpp
// In ResetGameData() and state transitions
for (i = 0; i < DEF_MAXEFFECTS; i++) {
    if (m_pEffectList[i] != NULL) delete m_pEffectList[i];
    m_pEffectList[i] = NULL;
}
```

### Camera Shake

```cpp
void CGame::SetCameraShakingEffect(short sDist, int iMul = 0);
```

- Calculates shake intensity based on distance: `iDegree = (5 - sDist) * 2`
- Applies multiplier for powerful effects
- Sets `m_iCameraShakingDegree` which affects view offset during rendering
- Minimum threshold of 2 to avoid imperceptible shakes

## Effect Sprite Resources

Effect sprites are stored in PAK files and loaded into `m_pEffectSpr[]`:

| Index | PAK File | Contents |
|-------|----------|----------|
| 0 | Effect0.pak | Light orbs, glows |
| 1 | Effect1.pak | Gold/item drop |
| 3 | Effect3.pak | Fire explosion |
| 4 | Effect4.pak | Spell casting |
| 6 | Effect6.pak | Energy/lightning |
| 7 | Effect7.pak | Arrow sprites |
| 8 | Effect8.pak | Sword slash |
| 11 | Effect11.pak | Particles (blood, fire, smoke) |
| 14-15 | Effect14-15.pak | Mass fire effects |
| 18-19 | Effect18-19.pak | Ground effects |
| 20-22 | Effect20-22.pak | Ice/cold effects |
| 24-25 | Effect24-25.pak | Protection rings |
| 28-29 | Effect28-29.pak | Star/chill effects |
| 31-35 | Effect31-35.pak | Meteor/earthquake |
| 38-44 | Effect38-44.pak | Monster attacks |
| 46-51 | Effect46-51.pak | Blizzard effects |
| 60-61 | Effect60-61.pak | Illusion effects |
| 74-76 | Effect74-76.pak | Special directional |

## Audio Integration

Effects trigger sounds via `PlaySound()`:

```cpp
PlaySound('E', soundId, sDist, lPan);
```

| Sound ID | Effect |
|----------|--------|
| 1 | Magic missile launch |
| 2 | Energy bolt |
| 3 | Magic missile impact |
| 4 | Fire explosion |
| 5 | Spell casting |
| 12 | Gold drop |
| 42 | Surface fire |
| 45 | Chill wind |
| 46 | Ice fall |
| 47 | Blizzard impact |

Sound parameters:
- `sDist`: Distance attenuation (0-10 scale)
- `lPan`: Stereo panning (-10000 to 10000)

## Constants & Limits

```cpp
#define DEF_MAXEFFECTSPR    100     // Effect sprite types
#define DEF_MAXEFFECTS      300     // Concurrent effects (was 600)
```

**Timing constants:**
- Minimum frame time: 10ms
- Maximum frame time: 120ms
- Typical particle: 30ms
- Slow effects: 80-100ms

**Position scaling:**
- Tile to pixel: multiply by 32
- Character height offset: `_iAttackerHeight[iV1]`
- Standard offset: -40 pixels (head height)

## Integration Points

### With Combat System
- `bAddNewEffect(1, ...)` called on melee attacks
- `bAddNewEffect(2, ...)` for arrow shots
- Blood effects spawned on damage

### With Magic System
- Spell types 100-200+ directly map to magic IDs
- `NotifyMsg_Magic()` triggers appropriate effects
- Some spells cascade into multiple effect types

### With Network
- Server sends effect triggers via `CYCLEMSG_EFFECT` packets
- Client creates local effects based on received data
- Damage numbers shown separately from effects

### With Map System
- Effects use tile coordinates for positioning
- Camera viewport used for screen-space rendering
- Some effects interact with terrain (ground dust, fire spread)

## State Management

**Effect lifecycle:**
1. **Creation**: Allocated in first free slot, initialized
2. **Active**: Frame advances each `m_dwFrameTime` ms
3. **Deletion**: When `m_cFrame > m_cMaxFrame` or reaching destination

**Memory management:**
- Raw `new`/`delete` for CEffect instances
- No pooling or recycling
- Manual cleanup on state transitions

## Known Issues / Technical Debt

1. **Hardcoded type IDs**: 77+ effect types identified only by magic numbers
2. **Giant switch statements**: bAddNewEffect has 100+ cases
3. **Duplicated logic**: Similar effects have copy-pasted initialization
4. **Mixed coordinate systems**: Some use tile coords, some pixel coords
5. **No effect pooling**: Constant allocation/deallocation
6. **Limited composability**: Complex effects require manual child spawning
7. **Language-specific code**: `#if DEF_LANGUAGE != 3` disables blood for certain regions
8. **Magic numbers**: Frame counts, timing, offsets embedded in code

## Modernization Notes

### Recommended Architecture

```cpp
// Effect type enumeration
enum class EffectType : uint16_t {
    SwordSlash = 1,
    Arrow = 2,
    GoldDrop = 4,
    // ...
};

// Data-driven effect definition
struct EffectDefinition {
    EffectType type;
    std::string_view spritePak;
    int spriteIndex;
    int maxFrames;
    std::chrono::milliseconds frameTime;
    bool hasPhysics;
    bool emitsLight;
    std::optional<SoundId> sound;
    std::vector<EffectType> childEffects;
};

// Modern effect class
class Effect {
public:
    void update(Duration deltaTime);
    void render(Renderer& renderer, Vec2 viewOffset);
    bool isFinished() const;

private:
    const EffectDefinition* m_definition;
    Vec2 m_position;
    Vec2 m_velocity;
    int m_currentFrame;
    Duration m_frameAccumulator;
};

// Effect system with pooling
class EffectSystem {
public:
    EffectHandle spawn(EffectType type, Vec2 position, Vec2 target);
    void update(Duration deltaTime);
    void render(Renderer& renderer);

private:
    ObjectPool<Effect> m_effectPool;
    std::vector<Effect*> m_activeEffects;
};
```

### Key Improvements
1. **Data-driven definitions**: Load from config files
2. **Object pooling**: Reuse effect instances
3. **Component-based**: Separate physics, rendering, audio
4. **Strong typing**: Enums instead of magic numbers
5. **Cleaner lifecycle**: RAII, no manual delete
6. **Batched rendering**: Group by sprite for efficiency
