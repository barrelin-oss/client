# Spell & Effect System Reference

Complete technical reference for the magic casting and visual effects pipeline. Written to document every lesson learned during the Energy Strike debugging process so the same mistakes are never repeated.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [The Coordinate Problem (And How We Fixed It)](#the-coordinate-problem)
3. [Spell Definitions](#spell-definitions)
4. [Effect Definitions](#effect-definitions)
5. [Effect Behaviors](#effect-behaviors)
6. [The Three Casting Paths](#the-three-casting-paths)
7. [Effect System API](#effect-system-api)
8. [The Height Offset Mechanism](#the-height-offset-mechanism)
9. [Composite Effects & Child Spawning](#composite-effects--child-spawning)
10. [Complete Spell → Effect Mapping](#complete-spell--effect-mapping)
11. [Complete Effect Definition Table](#complete-effect-definition-table)
12. [Network Protocol](#network-protocol)
13. [Common Pitfalls](#common-pitfalls)

---

## Architecture Overview

```
Player clicks to cast
       │
       ▼
 ┌─────────────┐     ┌──────────────────┐
 │input_handler │────▶│ ws_message_handler│──── JSON ──▶ Server
 │  (direct)    │     │  .request_magic() │
 └──────┬───────┘     └──────────────────┘
        │                                         Server broadcasts
        │ Local effects                           combat_effect to
        │ (immediate)                             all nearby players
        ▼                                                │
 ┌──────────────┐                              ┌─────────▼──────────┐
 │ effect_system │◀────────────────────────────│handle_combat_effect │
 │              │                              │ (other players'     │
 │ add_effect_world()                          │  spell casts)       │
 │ add_effect_at_pixel()                       └────────────────────┘
 └──────────────┘
        │
 ┌──────────────┐
 │ action_queue  │  Queued casts (if player was busy when spell was clicked)
 │  (deferred)   │  Also uses add_effect_world()
 └──────────────┘
```

**Key principle:** The local player sees spell effects *immediately* on click. The server response (`player_magic_response`) only updates authoritative data (MP, damage numbers, target HP). Other players' spell effects come via `combat_effect` broadcast.

---

## The Coordinate Problem

### The Bug

When casting Energy Strike, the child projectiles (scattered bolts from caster to target area) were landing ~3 tiles above where the player clicked. The offset was consistent and always in the Y direction.

### Root Cause

The effect system has a `height_offset` field (default: **-40 pixels**) that shifts all effect Y-coordinates upward so projectiles fly "head-to-head" rather than "feet-to-feet". This offset is applied inside `init_effect()`:

```cpp
// init_effect() always does this:
float h = static_cast<float>(def.height_offset);  // -40
eff.src_y = src_y + h;    // shifts up by 40px
eff.dest_y = dest_y + h;  // shifts up by 40px
```

When using tile-based `add_effect()`, the coordinates are first converted to pixel positions at tile center, then shifted by -40. For a single projectile this looks fine — both source and destination are shifted equally, so the flight path is parallel.

**But for composite effects with children:** The parent's `dest_y` (already shifted by -40) becomes the center point for random scatter (±50 in Energy Strike's case). This means the scatter is centered on `target_y - 40`, not `target_y`. The children then get their *own* height_offset applied on top, compounding the error.

The total Y offset could be: **parent height_offset (-40) + random_range (±50) + child height_offset (-40)** = up to 130 pixels off from click position. That's ~4 tiles.

### The Fix

Created `add_effect_world()` which takes **exact world pixel coordinates** and **pre-compensates** for height_offset:

```cpp
void effect_system::add_effect_world(effect_type_id type_id,
                                      float src_x, float src_y,
                                      float dest_x, float dest_y)
{
    float h = static_cast<float>(def->height_offset);
    // Subtract h so that init_effect's addition cancels it out:
    // init_effect does: eff.y = (y - h) + h = y  (exact position)
    init_effect(eff, *def, src_x, src_y - h, dest_x, dest_y - h, 0, 1);
}
```

All three casting paths now use `add_effect_world()` with the player's actual pixel position (`transform().x/y`) and the target's pixel position, bypassing tile quantization entirely.

### The Coordinate Chain (Traced)

For `add_effect_world(type, src_x, src_y, dest_x, dest_y)`:

1. **add_effect_world** pre-compensates: `init_effect(src_x, src_y - h, dest_x, dest_y - h)`
2. **init_effect** adds h back: `eff.src_y = (src_y - h) + h = src_y` ✓
3. **eff.dest_y** = `(dest_y - h) + h = dest_y` ✓
4. **spawn_children** reads `parent.dest_y` which = exact target Y ✓
5. Random scatter: `dest_y ± random_range` — now symmetric around actual target ✓
6. Child pre-compensation: `init_effect(child, src_y - child_h, dest_y - child_h)` → child adds `child_h` back → exact positions ✓

---

## Spell Definitions

**File:** `src/gameplay/magic.cpp` (lines 657-1117)
**Struct:** `src/gameplay/magic.hpp`

### Spell Struct Fields

```cpp
struct spell {
    uint16_t id;                    // Unique spell ID (0-99)
    std::string name;               // Display name
    std::string description;        // Tooltip

    spell_category category;        // attack/defense/healing/buff/debuff/summon/utility
    spell_target target_type;       // self/single/area/ground
    magic_type type;                // Server-side magic type enum
    magic_element element;          // none/fire/ice/lightning/earth/holy/dark

    // Requirements
    uint16_t level_req, magic_req, int_req;

    // Cost
    uint16_t mp_cost, sp_cost, hp_cost;

    // Combat stats
    int32_t base_damage, base_healing;
    float duration;                 // Buff/debuff duration in seconds
    float cooldown = 1.0f;
    int32_t range = 8;              // Tiles
    int32_t aoe_radius = 0;        // Tiles (0 = single target)

    // === VISUAL EFFECT FIELDS (the ones we care about) ===
    uint16_t projectile_effect;     // Effect type ID for projectile (0 = none)
    uint16_t effect_sprite;         // Effect type ID for impact (0 = none)
    float cast_time = 1.0f;

    // Progression (from server)
    bool learned;
    uint8_t mastery_level;
    uint32_t experience;
    int32_t total_casts;
};
```

### How Spells Connect to Effects

Each spell has two effect slots:

| Field | Purpose | When Spawned |
|-------|---------|-------------|
| `projectile_effect` | Projectile from caster to target | On cast click (locally) or on broadcast |
| `effect_sprite` | Impact/aura at target location | On cast if no projectile, OR on projectile arrival via `impact_effect` chain |

**Rule:** If `projectile_effect != 0`, spawn the projectile and it handles impact on arrival. If `projectile_effect == 0 && effect_sprite != 0`, spawn the static effect directly at the target. These are mutually exclusive in code (`if/else if`).

### Spell Target Types

```cpp
enum class spell_target {
    self,       // Cast on self (target = local player position)
    single,     // Cast on clicked tile; include entity ID if clicking on one
    area,       // Same as single for client purposes (server handles AoE)
    ground,     // Same as single for client purposes
};
```

**All target types** resolve to a tile coordinate from the click position. The only difference is `self` which forces the target to the player's own position. If clicking directly on an entity, we include their ID for the server.

---

## Effect Definitions

**File:** `src/gameplay/effect_definition.cpp` (93 definitions)
**Struct:** `src/gameplay/effect_definition.hpp`

### Effect Definition Fields

```cpp
struct effect_definition {
    effect_type_id type_id;
    effect_behavior behavior;           // static_anim, projectile, physics, composite
    effect_render_mode render_mode;     // transparent, alpha_25/50/70, fade, normal
    effect_detail detail_level;         // all, high_only (particles skipped at low detail)

    uint8_t sprite_pak_index;           // Index into effect_sprites_[] (255 = invisible)
    uint8_t start_frame;
    uint8_t max_frames;                 // Last frame index (NOT frame count)
    uint16_t frame_time_ms;             // Milliseconds between frames

    int8_t sound_id;                    // 'E' category sound (-1 = none)
    int8_t shake_intensity;             // Camera shake (0 = none)
    int8_t shake_multiplier;            // Shake multiplier for powerful effects

    bool emits_light;
    uint8_t light_radius;

    int8_t gravity;                     // Physics particles: positive = down
    bool uses_tile_coords = true;       // Whether add_effect() converts tiles→pixels
    int16_t height_offset = -40;        // Y-offset applied in init_effect (DEFAULT -40!)

    bool directional;                   // 8-direction sprite based on src→dest angle

    // Projectile fields
    uint8_t projectile_speed;           // Bresenham steps per frame (50=fast, 4=slow)
    effect_type_id impact_effect;       // Spawned at destination on arrival
    effect_type_id trail_effect;        // Spawned each frame while traveling
    uint8_t trail_count;                // How many trail particles per frame
    int16_t trail_random_range;         // Pixel scatter for trail particles

    // Composite children
    child_effect_spec children[8];
    uint8_t child_count;
};
```

### Child Effect Spec

```cpp
struct child_effect_spec {
    effect_type_id type;            // Child effect to spawn
    int8_t trigger_frame;           // -1 = every frame, N = spawn at frame N
    int8_t count;                   // How many to spawn
    int16_t offset_x, offset_y;    // Pixel offset from parent position
    bool random_offset;             // Add random scatter
    int16_t random_range;           // ±range pixels for scatter
    bool as_projectile;             // If true: projectile from parent.src to parent.dest
};
```

---

## Effect Behaviors

### static_anim
Simple animation at a fixed position. Advances frames based on `frame_time_ms`. Dies when `current_frame > max_frame`. Used for impacts, auras, buffs.

### projectile
Bresenham line movement from source to destination.
- `projectile_speed` = steps per Bresenham tick (higher = faster)
- Spawns `trail_effect` particles each frame while moving
- Spawns `impact_effect` at destination on arrival
- Can be `directional` (8 sprite variants based on travel angle)
- Dies on arrival or after 10s safety timeout

### physics
Particle with velocity and gravity.
- Random initial velocity: `vx ∈ [-5, 5], vy ∈ [-10, -2]`
- Each frame: `velocity_y += gravity`
- `uses_tile_coords = false` (physics particles always use pixel coords)
- Used for burst effects (fire_burst, burst_physics, blood_burst)

### composite
Parent animation that spawns child effects at specific frames.
- Advances frames like static_anim
- Calls `spawn_children()` on each frame advance
- Can have a visible sprite (most composites) or be invisible (`sprite_pak_index = 255`, e.g., Energy Strike spawner)
- Children can be `as_projectile = true` (fire from parent.src to parent.dest with scatter) or `as_projectile = false` (spawn at parent.pos with offset)

---

## The Three Casting Paths

### Path 1: Direct Cast (input_handler.cpp)

**Trigger:** Player clicks during spell targeting mode.
**File:** `input_handler::handle_spell_targeting()` (line ~570)

```
1. Determine target tile and entity from click position
2. Send request_magic() to server
3. Trigger cooldown locally
4. Compute world pixel coords:
   - Source: player->transform().x/y (actual pixel position)
   - Dest: target entity x/y, or screen_to_world(mouse) for ground
5. Call add_effect_world() for projectile, or add_effect_at_pixel() for static impact
6. Clear pending spell, suppress input until mouse release
```

### Path 2: Queued Cast (action_queue.cpp)

**Trigger:** Spell was queued while player was busy (moving/attacking), now executing.
**File:** `action_queue::process_pending()`, `queued_action_type::magic` case (line ~128)

```
1. Send request_magic() to server
2. Set player action to magic animation
3. Trigger cooldown
4. Compute world pixel coords:
   - Source: player->transform().x/y
   - Dest: target entity x/y, or tile center (tile * 32 + 16)
5. Call add_effect_world() / add_effect_at_pixel()
```

**Note:** Queued casts don't have mouse position available, so they fall back to tile center (`tile * 32 + 16`) for ground targets. This is acceptable since the tile was already determined at queue time.

### Path 3: Broadcast (ws_message_handler.cpp)

**Trigger:** Server sends `combat_effect` message for another player's spell cast.
**File:** `ws_message_handler::handle_combat_effect()` (line ~1470)

```
1. Skip if source_id == local_player_id (we already handled our own cast)
2. Set caster entity to magic animation
3. Show spell name as red chat bubble on caster
4. Compute world pixel coords:
   - Source: caster entity x/y, or tile center
   - Dest: target entity x/y, or tile center from combat_effect data
5. Call add_effect_world() / add_effect_at_pixel()
6. Show floating text (damage/heal/miss/etc)
7. Update target HP from broadcast
```

### What Each Path Handles

| Concern | Path 1 (Direct) | Path 2 (Queued) | Path 3 (Broadcast) |
|---------|-----------------|-----------------|---------------------|
| Send network request | ✓ | ✓ | N/A (received) |
| Magic animation | Already playing (readied) | Set here | Set here |
| Cooldown | ✓ | ✓ | N/A |
| Projectile/impact effects | ✓ | ✓ | ✓ |
| Spell name bubble | Already showing (readied) | Not shown | ✓ |
| MP update | Via player_magic_response | Via response | N/A |
| Floating text | Via response/broadcast | Via response | ✓ |
| Target HP | Via response/broadcast | Via response | ✓ |

---

## Effect System API

**File:** `src/gameplay/effect_system.hpp`

### Public Methods

```cpp
// TILE COORDINATES - converts tile→pixel internally
// Use for: server-initiated effects where you only have tile coords
void add_effect(effect_type_id type_id,
                int32_t src_x, int32_t src_y,       // source tile
                int32_t dest_x, int32_t dest_y,     // destination tile
                int8_t start_frame = 0, int32_t value = 1);

// WORLD PIXEL COORDINATES - pre-compensates for height_offset
// Use for: ALL spell casting effects (player casts, broadcasts)
// This is the correct API for spell effects because it preserves exact positions
void add_effect_world(effect_type_id type_id,
                       float src_x, float src_y,     // source world pixels
                       float dest_x, float dest_y);  // destination world pixels

// TILE COORDINATES - single position (src == dest)
// Use for: static effects at a tile (server-initiated)
void add_effect_at(effect_type_id type_id, int32_t tile_x, int32_t tile_y);

// WORLD PIXEL COORDINATES - single position
// Use for: static impact effects at exact pixel position (spell impacts)
void add_effect_at_pixel(effect_type_id type_id, float world_x, float world_y);
```

### When to Use Which

| Scenario | API to Use |
|----------|-----------|
| Spell projectile (local or broadcast) | `add_effect_world()` |
| Spell impact (no projectile) | `add_effect_at_pixel()` |
| Server tells us to show effect at tile | `add_effect()` or `add_effect_at()` |
| Non-spell effects (arrows, gold drops) | `add_effect()` |

**Rule of thumb:** If you have entity pixel positions available, use the `_world` / `_at_pixel` variants. If you only have tile coordinates, use the tile-based variants.

---

## The Height Offset Mechanism

### What It Does

Every effect has a `height_offset` field (default: **-40 pixels**). This shifts the effect's Y position upward by 40 pixels so effects appear at "head height" rather than at the entity's feet.

### Where It's Applied

In `init_effect()` (effect_system.cpp):

```cpp
float h = static_cast<float>(def.height_offset);  // typically -40
eff.src_y = src_y + h;       // -40 shifts up
eff.dest_y = dest_y + h;
eff.pos_y = eff.src_y;       // starts at shifted source
```

### Why It Matters

- **Projectiles** fly at head level between source and destination
- **Impact effects** appear at head level on the target
- **Composite children** inherit the parent's already-shifted positions
- **The danger:** If you pass world pixel positions directly to `add_effect()` (tile API), the height offset gets applied on top, pushing the effect 40px above where you intended

### How add_effect_world() Fixes It

```cpp
// add_effect_world pre-compensates:
init_effect(eff, *def, src_x, src_y - h, dest_x, dest_y - h, 0, 1);
// init_effect then adds h back:
// eff.src_y = (src_y - h) + h = src_y  ← exact position
```

### Composite Child Height Handling

In `spawn_children()`, when spawning child projectiles:

```cpp
float child_h = static_cast<float>(child_def->height_offset);
init_effect(eff, *child_def,
            parent.src_x, parent.src_y - child_h,   // parent.src_y is already correct
            dest_x, dest_y - child_h, 0, 1);
// child's init_effect adds child_h back:
// eff.src_y = (parent.src_y - child_h) + child_h = parent.src_y  ✓
```

---

## Composite Effects & Child Spawning

### How spawn_children() Works

Called from `update_composite()` on every frame advance.

```
For each child_spec in parent's children[]:
    If trigger_frame == -1 OR trigger_frame == current_frame:
        For count times:
            If as_projectile:
                dest = parent.dest + random_scatter(±random_range)
                Spawn projectile from parent.src to dest
                Pre-compensate child's height_offset
            Else:
                pos = parent.pos + offset + random_scatter
                Spawn static effect at pos via add_effect_at_pixel()
```

### Notable Composite Effects

| Type ID | Name | Children | Behavior |
|---------|------|----------|----------|
| 5 | fire_explosion | 5× fire_burst at frame 1, ±50px | Fireball impact |
| 6 | energy_bolt_burst | 5× burst_physics at frame 1, ±40px | Energy bolt impact |
| 7 | magic_missile_exp | 3× burst_stationary at frame 1, ±30px | Magic missile impact |
| 19 | energy_strike_impact | 5× burst_physics at frame 0, ±20px | Energy strike bolt arrival |
| 61 | meteor_strike_explosion | 8× fire_burst at frame 1, ±60px | Meteor impact |
| 114 | spell_celebrating_light | 6× fire_burst at frame 2, ±80px | Healing light |
| 138 | spell_tremor | 14× dust_cloud at frame 1, ±100px | Earth tremor |
| **160** | **spell_energy_strike** | **1× energy_strike_proj every frame (-1), ±50px, as_projectile** | **Invisible spawner** |

### The Energy Strike Chain (Most Complex)

```
Energy Strike spell (ID 60, projectile_effect=160)
    │
    ▼
spell_energy_strike (type 160) — COMPOSITE, invisible (sprite 255)
    │  Spawns 1 child every frame for 7 frames
    │  as_projectile=true, random_offset=true, random_range=50
    │
    ▼
energy_strike_proj (type 16) — PROJECTILE
    │  speed=40, trail=1× burst_stationary, ±10px
    │  impact_effect = energy_strike_impact
    │
    ▼
energy_strike_impact (type 19) — COMPOSITE
    │  ground_shake sprite, 5× burst_physics at frame 0, ±20px
    │
    ▼
burst_physics (type 9) — PHYSICS
    gravity=1, random velocity, sprite 11
```

That's **4 levels deep**: composite → projectile → composite → physics.

---

## Complete Spell → Effect Mapping

### Projectile Spells (projectile_effect != 0)

| Spell ID | Name | Projectile Effect | Impact (via chain) |
|----------|------|-------------------|-------------------|
| 0 | Magic Missile | 100 (spell_magic_missile) | → 7 (magic_missile_exp) |
| 10 | Energy Bolt | 110 (spell_energy_bolt) | → 6 (energy_bolt_burst) |
| 20 | Fire Ball | 120 (spell_fire_ball) | → 5 (fire_explosion) |
| 30 | Fire Strike | 120 (spell_fire_ball) | → 5 (fire_explosion) |
| 37 | Lightning Arrow | 137 (spell_lightning_arrow) | → 10 (lightning_arrow_exp) |
| 43 | Lightning | 137 | → 10 |
| 47 | Triple Energy Bolt | 110 | → 6 |
| 51 | Lightning Bolt | 137 | → 10 |
| 56 | Mass Lightning Arrow | 137 | → 10 |
| 60 | Energy Strike | 160 (spell_energy_strike) | → 16 → 19 → 9 |
| 61 | Mass Fire Strike | 120 | → 5 |
| 82 | Mass Magic Missile | 100 | → 7 |

### Static Impact Spells (effect_sprite only, no projectile)

| Spell ID | Name | Effect ID | Effect Name |
|----------|------|-----------|-------------|
| 1 | Heal | 101 | spell_heal |
| 2 | Create Food | 114 | spell_celebrating_light |
| 11 | Stamina Drain | 62 | dark_cloud |
| 12 | Recall | 112 | spell_recall_1 |
| 13 | Defense Shield | 113 | spell_defense_shield |
| 14 | Celebrating Light | 114 | spell_celebrating_light |
| 21 | Great Heal | 121 | spell_great_heal |
| 23 | Stamina Recovery | 114 | spell_celebrating_light |
| 24 | Protection from Arrow | 52 | protection_ring |
| 25 | Hold Person | 135 | spell_paralyze (triggers hold_twist) |
| 26 | Possession | 62 | dark_cloud |
| 27 | Poison | 62 | dark_cloud |
| 28 | Great Stamina Recovery | 114 | spell_celebrating_light |
| 31 | Summon Creature | 62 | dark_cloud |
| 32 | Invisibility | 132 | spell_invisibility (fade) |
| 33 | Protection from Magic | 52 | protection_ring |
| 34 | Detect Invisibility | 113 | spell_defense_shield |
| 35 | Paralyze | 135 | spell_paralyze |
| 36 | Cure | 136 | spell_cure |
| 38 | Tremor | 62 | dark_cloud |
| 40 | Fire Wall | 5 | fire_explosion |
| 41 | Fire Field | 5 | fire_explosion |
| 42 | Confuse Language | 62 | dark_cloud |
| 44 | Great Defense Shield | 113 | spell_defense_shield |
| 45 | Chill Wind | 40 | chill_wind |
| 46 | Poison Cloud | 62 | dark_cloud |
| 50 | Berserk | 150 | spell_berserk |
| 53 | Mass Poison | 62 | dark_cloud |
| 54 | Spike Field | 62 | dark_cloud |
| 55 | Ice Storm | 40 | chill_wind |
| 57 | Ice Strike | 40 | chill_wind |
| 62 | Confusion | 62 | dark_cloud |
| 63 | Mass Chill Wind | 40 | chill_wind |
| 64 | Earthworm Strike | 62 | dark_cloud |
| 65 | Absolute Magic Protection | 52 | protection_ring |
| 66 | Armor Break | 62 | dark_cloud |
| 70 | Bloody Shock Wave | 6 | energy_bolt_burst |
| 71 | Mass Confusion | 62 | dark_cloud |
| 72 | Mass Ice Strike | 40 | chill_wind |
| 73 | Cloud Kill | 62 | dark_cloud |
| 74 | Lightning Strike | 63 | lightning_strike |
| 76 | Cancellation | 62 | dark_cloud |
| 77 | Illusion Movement | 62 | dark_cloud |
| 80 | Illusion | 62 | dark_cloud |
| 81 | Meteor Strike | 61 | meteor_strike_explosion |
| 83 | Inhibition Casting | 62 | dark_cloud |
| 90 | Mass Illusion | 62 | dark_cloud |
| 91 | Blizzard | 72 | blizzard_large_impact |
| 94 | Resurrection | 121 | spell_great_heal |
| 95 | Mass Illusion Movement | 62 | dark_cloud |
| 96 | Earth Shock Wave | 62 | dark_cloud |

### Placeholder Effects (effect_sprite = 62)

**27 spells** use `dark_cloud` (type 62) as a placeholder. These are debuffs, utility spells, and effects that need proper unique visuals. They all show a semi-transparent dark cloud animation.

---

## Complete Effect Definition Table

### Behavior Legend
- **S** = static_anim (play at position)
- **P** = projectile (travel src → dest)
- **Ph** = physics (particle with gravity)
- **C** = composite (spawns children)

### Render Mode Legend
- **T** = transparent (color-key)
- **A25/A50/A70** = alpha blend 25%/50%/70%
- **F** = fade

| ID | Name | Beh | Render | Sprite | Frames | FTime | Speed | Impact | Trail | Children | Notes |
|----|------|-----|--------|--------|--------|-------|-------|--------|-------|----------|-------|
| 1 | sword_slash | S | T | 8 | 2 | 10 | | | | | Melee trail |
| 2 | arrow | P | T | 7 | 0 | 10 | 70 | 14 | | | Directional |
| 4 | gold_drop | S | T | 1 | 12 | 100 | | | | | Sound 12 |
| 5 | fire_explosion | C | T | 3 | 11 | 10 | | | | 5× fire_burst@1 ±50 | Sound 4, light |
| 6 | energy_bolt_burst | C | T | 6 | 14 | 10 | | | | 5× burst_physics@1 ±40 | Sound 2, light |
| 7 | magic_missile_exp | C | T | 0 | 5 | 50 | | | | 3× burst_stationary@1 ±30 | Sound 3, light |
| 8 | burst_stationary | S | T | 11 | 4 | 30 | | | | | HIGH_ONLY |
| 9 | burst_physics | Ph | T | 11 | 14 | 30 | | | | | HIGH_ONLY, gravity=1 |
| 10 | lightning_arrow_exp | C | T | 6 | 14 | 10 | | | | 5× burst_physics@1 ±40 | Light |
| 11 | blood_burst | Ph | T | 11 | 8 | 30 | | | | | HIGH_ONLY, gravity=1 |
| 12 | fire_burst | Ph | T | 11 | 10 | 30 | | | | | HIGH_ONLY, gravity=1, light |
| 13 | smoke_rising | S | A50 | 13 | 18 | 20 | | | | | |
| 14 | dust_cloud | S | A50 | 12 | 4 | 100 | | | | | HIGH_ONLY |
| 15 | fire_trail | S | T | 11 | 16 | 80 | | | | | HIGH_ONLY, light |
| 16 | energy_strike_proj | P | T | 0 | 0 | 20 | 40 | 19 | 8×1 ±10 | | Light |
| 17 | ice_storm_fragment | Ph | T | 11 | 12 | 20 | | | | | Gravity=1 |
| 18 | ground_shake | S | A70 | 18 | 10 | 50 | | | | | Shake=3 |
| 19 | energy_strike_impact | C | A70 | 18 | 10 | 50 | | | | 5× burst_physics@0 ±20 | Sound 1, shake=3 |
| 20-27 | magic_projectile_N | P | T | 0 | 0 | 10 | 50 | | 8×2 ±10 | | Directional, light |
| 30 | mass_fire_strike_main | S | T | 14 | 9 | 30 | | | | | Shake=3×2, light |
| 31 | mass_fire_strike_sec | S | T | 15 | 8 | 30 | | | | | Shake=2, light |
| 32 | breaking_effect | S | T | 11 | 4 | 30 | | | | | |
| 33 | mass_magic_attack | S | T | 6 | 16 | 30 | | | | | Light |
| 34 | moving_ice_bolt | P | T | 20 | 0 | 10 | 50 | | | | Directional, shake=2 |
| 40 | chill_wind | S | A50 | 20 | 15 | 30 | | | | | Sound 45 |
| 41-44 | meteor_large_N | S | T | 31-34 | 14 | 30 | | | | | Shake=3, light |
| 45-46 | meteor_small_N | S | T | 35 | 14 | 30 | | | | | Light |
| 47-49 | blizzard_ice_N | S | T | 46-48 | 12 | 30 | | | | | Sound 46 (ice1 only) |
| 50 | meteor_impact | S | T | 31 | 12 | 30 | | | | | Shake=4, light |
| 51 | chill_wind_aftermath | S | A50 | 21 | 9 | 30 | | | | | |
| 52 | protection_ring | S | A50 | 24 | 15 | 80 | | | | | Light |
| 53 | hold_twist | S | A50 | 25 | 15 | 80 | | | | | |
| 54-55 | star_twinkle_N | S | T | 28-29 | 10 | 30 | | | | | Light |
| 56 | mass_chill_wind | S | A50 | 22 | 14 | 30 | | | | | Sound 45 |
| 57 | casting_effect | S | T | 4 | 16 | 80 | | | | | Sound 5, light |
| 60 | meteor_strike_prepare | S | T | 31 | 10 | 50 | | | | | Light |
| 61 | meteor_strike_explosion | C | T | 31 | 16 | 30 | | | | 8× fire_burst@1 ±60 | Shake=5×2, light |
| 62 | dark_cloud | S | A70 | 38 | 6 | 50 | | | | | PLACEHOLDER |
| 63 | lightning_strike | S | T | 6 | 16 | 20 | | | | | Shake=3, light |
| 64 | resurrection_effect | S | A50 | 24 | 15 | 30 | | | | | Light |
| 65 | moving_dark_cloud | P | A70 | 38 | 30 | 30 | 50 | | | | |
| 66 | earthquake | S | T | 35 | 14 | 30 | | | | | Shake=5×2 |
| 67 | fire_pillar | S | T | 39 | 27 | 30 | | | | | Sound 42, light |
| 68 | worm_bite | S | T | 40 | 17 | 30 | | | | | |
| 69-70 | surface_fire_N | S | T | 18-19 | 11 | 30 | | | | | Sound 42 (fire1), light |
| 71 | moving_ice_bolt_2 | P | T | 20 | 0 | 10 | 50 | | | | Directional |
| 72 | blizzard_large_impact | S | T | 49 | 15 | 30 | | | | | Sound 47, shake=2 |
| 73-74 | light_effect_N | S | A50 | 50-51 | 15/19 | 30 | | | | | Light |
| 75-77 | directional_effect_N | S | T | 74-76 | 16 | 30 | | | | | Directional |
| 100 | spell_magic_missile | P | T | 0 | 0 | 20 | 50 | 7 | 8×1 ±10 | | Sound 1, dir, light |
| 101 | spell_heal | S | A50 | 4 | 14 | 80 | | | | | Light |
| 102 | spell_create_food | S | T | 4 | 13 | 120 | | | | | |
| 110 | spell_energy_bolt | P | T | 6 | 0 | 20 | 50 | 6 | 8×2 ±10 | | Sound 2, dir, light |
| 111 | spell_stamina_drain | S | A50 | 4 | 14 | 80 | | | | | |
| 112 | spell_recall_1 | S | A50 | 4 | 12 | 80 | | | | | Light |
| 113 | spell_defense_shield | S | A50 | 24 | 12 | 120 | | | | | Light |
| 114 | spell_celebrating_light | C | T | 4 | 14 | 30 | | | | 6× fire_burst@2 ±80 | Light |
| 120 | spell_fire_ball | P | T | 3 | 0 | 20 | 50 | 5 | | | Dir, light |
| 121 | spell_great_heal | S | A50 | 4 | 14 | 80 | | | | | Light |
| 122 | spell_recall_2 | S | A50 | 4 | 13 | 120 | | | | | Light |
| 124 | spell_protection_nm | C | A50 | 4 | 14 | 30 | | | | 1× protection_ring@0 | |
| 125 | spell_hold_person | C | A50 | 4 | 14 | 30 | | | | 1× hold_twist@0 | |
| 130 | spell_fire_strike | P | T | 11 | 0 | 20 | 50 | 5 | | | Dir, light |
| 131 | spell_summon | S | T | 4 | 12 | 80 | | | | | |
| 132 | spell_invisibility | S | F | 4 | 12 | 80 | | | | | Fade render |
| 133 | spell_protection_magic | C | A50 | 4 | 14 | 30 | | | | 1× protection_ring@0 | |
| 135 | spell_paralyze | C | A50 | 4 | 14 | 30 | | | | 1× hold_twist@0 | |
| 136 | spell_cure | S | A50 | 4 | 13 | 120 | | | | | Light |
| 137 | spell_lightning_arrow | P | T | 6 | 0 | 20 | 50 | 10 | 8×3 ±10 | | Dir, light |
| 138 | spell_tremor | C | T | 35 | 14 | 30 | | | | 14× dust_cloud@1 ±100 | Shake=5×3 |
| 150 | spell_berserk | S | T | 4 | 11 | 100 | | | | | Light |
| 160 | spell_energy_strike | C | T | **255** | 7 | 80 | | | | 1× energy_strike_proj@**-1** ±50 **as_proj** | Sound 1, **INVISIBLE** |
| 180 | spell_illusion | S | F | 60 | 11 | 100 | | | | | Fade |
| 181 | spell_special_meteor | S | T | 6 | 16 | 20 | | | | | Shake=4, light |
| 190 | spell_mass_illusion | S | F | 61 | 11 | 100 | | | | | Fade |

---

## Network Protocol

### Sending a Spell Cast (Client → Server)

**Message type:** `player_magic_request`
**Builder:** `make_player_magic_request()` in messages.hpp

```json
{
  "type": "player_magic_request",
  "data": {
    "x": 125,               // Caster tile X
    "y": 40,                // Caster tile Y
    "direction": 3,         // Caster facing
    "spell_id": 60,         // Spell ID
    "target_type": "npc",   // "none", "player", "npc", "ground"
    "target_id": 42,        // Entity ID if clicking on one (0 = ground)
    "target_x": 135,        // Target tile X
    "target_y": 40,         // Target tile Y
    "timestamp": 1707350400 // Client timestamp
  }
}
```

**target_type resolution:**
- Entity under cursor with `entity_type::monster` → `"npc"`
- Entity under cursor with `entity_type::character` → `"player"`
- No entity under cursor → `"ground"`
- target_id == 0 → `"none"`

### Server Response (Server → Caster)

**Message type:** `player_magic_response`

```json
{
  "type": "player_magic_response",
  "data": {
    "result": {
      "success": true,
      "spell_id": 60,
      "mana_cost": 65,
      "damage": 80,
      "heal": 0,
      "target_id": 42,
      "caster_mp": 135
    }
  }
}
```

**What the handler does with the response:**
- Updates local player MP: `player->stats().mp = data.caster_mp`
- Shows floating damage/heal text at target position
- Shows status log message (e.g., "Energy Strike hits for 80 damage")
- Updates target entity HP
- Does **NOT** spawn effects (already done locally on click)

### Broadcast to Nearby Players (Server → Others)

**Message type:** `combat_effect`

```json
{
  "type": "combat_effect",
  "data": {
    "source_id": 1001,
    "target_id": 42,
    "effect_type": "damage",
    "value": 80,
    "damage_type": "lightning",
    "spell_id": 60,
    "is_critical": false,
    "target_x": 135,
    "target_y": 40
  }
}
```

**What the handler does:**
- Skip if `source_id == local_player_id` (prevents double effects)
- Set caster animation to magic
- Show spell name bubble on caster
- Spawn projectile/impact effects via `add_effect_world()`
- Show floating text
- Update target HP

---

## Common Pitfalls

### 1. Using add_effect() for spell visuals
**Wrong:** `add_effect(type, caster_tile_x, caster_tile_y, target_tile_x, target_tile_y)`
**Right:** `add_effect_world(type, caster_pixel_x, caster_pixel_y, target_pixel_x, target_pixel_y)`

Tile-based API quantizes positions to tile centers and then applies height_offset, causing composite child effects to scatter around the wrong center point.

### 2. Spawning both projectile AND impact
**Wrong:**
```cpp
if (sp->projectile_effect != 0)
    effects.add_effect_world(projectile_effect, ...);
if (sp->effect_sprite != 0)
    effects.add_effect_at_pixel(effect_sprite, ...);
```

**Right:**
```cpp
if (sp->projectile_effect != 0)
    effects.add_effect_world(projectile_effect, ...);
else if (sp->effect_sprite != 0)
    effects.add_effect_at_pixel(effect_sprite, ...);
```

Projectiles handle their own impact via the `impact_effect` field in the effect definition. Spawning both would double-up the impact.

### 3. Forgetting to skip local player in combat_effect
The local player gets spell effects from the direct cast path (or queued cast path). The `combat_effect` broadcast also arrives for the local player's own casts. Without the skip check, effects would appear twice:

```cpp
if (data.source_id != 0 && data.source_id == entities.local_player_id())
{
    return;  // Already handled locally
}
```

### 4. Getting world pixel coords for ground targets
When there's no target entity:
- **Direct cast (input_handler):** Use `world.screen_to_world(mouse_x_, mouse_y_)` — exact click position
- **Queued cast (action_queue):** Use `tile * 32 + 16` — tile center (mouse position unavailable)
- **Broadcast (ws_handler):** Use `data.target_x * 32 + 16` — tile center from server

### 5. Not suppressing input after cast
After casting, `suppress_until_release_ = true` prevents the held mouse button from being interpreted as a movement click on the next frame.

### 6. The effect_sprite field naming is misleading
Despite being called `effect_sprite`, it's actually an **effect_type_id** that maps to a complete effect definition (which may itself be a composite with children). It's not a raw sprite index.
