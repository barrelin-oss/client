# Character Animation

Exhaustive reference for the sprite animation system in `src/`.

---

## Architecture Overview

The animation system is spread across four subsystems:

| Subsystem | Key Files | Responsibility |
|-----------|-----------|----------------|
| **Entity** | `entity/components.hpp`, `entity/entity.cpp`, `entity/entity_manager.cpp` | State machine, frame advancement, movement sync, rendering dispatch |
| **Assets** | `assets/sprite.hpp`, `assets/sprite_manager.hpp`, `assets/pak_file.hpp` | Sprite storage, PAK loading, on-demand bitmap loading, memory eviction |
| **Graphics** | `graphics/menu_character_renderer.hpp/.cpp`, `graphics/renderer.hpp` | Equipment layering, draw order, SFML draw calls |
| **Core** | `core/game_enums.hpp`, `core/direction_utils.hpp` | Enums (`object_action`, `direction`), direction math |

---

## Enums and Types

### `object_action` (`core/game_enums.hpp:42-57`)

Defines 12 action values. Each value maps to a group of 8 direction sprites in the PAK layout:

| Value | Name | Sprite Range | Description |
|-------|------|-------------|-------------|
| 0 | `stop_peace` | 0-7 | Idle (peace mode) |
| 1 | `stop_combat` | 8-15 | Idle (combat mode) |
| 2 | `move_peace` | 16-23 | Walk (peace mode) |
| 3 | `move_combat` | 24-31 | Walk (combat mode) |
| 4 | `run` | 32-39 | Run (shared) |
| 5 | `attack_peace` | 40-47 | Attack (peace mode) |
| 6 | `attack_combat` | 48-55 | Attack (combat mode) |
| 7 | `attack_combat_bow` | 56-63 | Bow attack |
| 8 | `magic` | 64-71 | Casting spell |
| 9 | `get_item` | 72-79 | Pick up item |
| 10 | `damage` | 80-87 | Taking damage |
| 11 | `dying` | 88-95 | Death animation |
| 100 | `null_action` | -- | Sentinel |

Legacy aliases at `game_enums.hpp:60-65` provide backward compatibility for the network protocol.

### `entity_anim_state` (`entity/components.hpp:52-65`)

The 12 animation state machine states:

| Value | Name | Legacy Handler |
|-------|------|----------------|
| 0 | `stop` | `DrawObject_OnStop` |
| 1 | `move` | `DrawObject_OnMove` |
| 2 | `run` | `DrawObject_OnRun` |
| 3 | `attack` | `DrawObject_OnAttack` |
| 4 | `attack_move` | `DrawObject_OnAttackMove` |
| 5 | `damage` | `DrawObject_OnDamage` |
| 6 | `damage_move` | `DrawObject_OnDamageMove` |
| 7 | `magic` | `DrawObject_OnMagic` |
| 8 | `get_item` | `DrawObject_OnGetItem` |
| 9 | `dying` | `DrawObject_OnDying` |
| 10 | `dead` | `DrawObject_OnDead` |
| 11 | `magic_attack` | `DrawObject_OnMagicAttack` |

### `animation_component::anim_type` (`entity/components.hpp:70-80`)

Legacy sub-enum for backward compatibility (not actively used by the state machine):

```
idle=0, walk=1, run=2, attack=3, attack_spell=4, damage=5, dying=6, dead=7, special=8
```

### `direction` (`core/game_enums.hpp:144-153`)

8-way direction enum with values 1-8. Use `std::optional<direction>` where "no direction" is needed (e.g. when pathfinding is blocked or source equals destination). The protocol uses 0-7 for directions; conversion functions in `direction_utils.hpp` handle the +1/-1 offset.

| Value | Direction |
|-------|-----------|
| 1 | `north` |
| 2 | `north_east` |
| 3 | `east` |
| 4 | `south_east` |
| 5 | `south` (default) |
| 6 | `south_west` |
| 7 | `west` |
| 8 | `north_west` |

---

## The `animation_component` (`entity/components.hpp:68-188`)

Every entity has an `animation_component` (always present, not optional).

### Fields

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| `current_anim` | `anim_type` | `idle` | Legacy animation type (compat) |
| `state` | `entity_anim_state` | `stop` | Current state machine state |
| `current_frame` | `uint8_t` | `0` | Current frame index within the animation |
| `frame_count` | `uint8_t` | `8` | Total frames for the current state |
| `frame_timer` | `float` | `0.0` | Accumulated time since last frame advance (seconds) |
| `frame_duration` | `float` | `0.115` | Duration per frame in seconds |
| `looping` | `bool` | `true` | Whether the current state loops |
| `finished` | `bool` | `false` | Whether a one-shot animation has completed |
| `attack_frame` | `uint8_t` | `0` | Frame at which attack damage fires |
| `attack_triggered` | `bool` | `false` | Whether the attack callback has fired this cycle |

### Frame Count Constants (`components.hpp:96-106`)

| Constant | Value | Legacy |
|----------|-------|--------|
| `frames_idle` | 8 | `DEF_OBJECTSTOP` maxFrame=7 |
| `frames_walk` | 8 | `DEF_OBJECTMOVE` maxFrame=7 |
| `frames_run` | 8 | `DEF_OBJECTRUN` maxFrame=7 |
| `frames_attack` | 8 | `DEF_OBJECTATTACK` maxFrame=7 |
| `frames_attack_move` | 13 | `DEF_OBJECTATTACKMOVE` maxFrame=12 |
| `frames_damage` | 8 | `DEF_OBJECTDAMAGE` maxFrame=7 |
| `frames_damage_move` | 4 | `DEF_OBJECTDAMAGEMOVE` maxFrame=3 |
| `frames_dying` | 13 | `DEF_OBJECTDYING` maxFrame=12 |
| `frames_dead` | 1 | Corpse (static) |
| `frames_magic` | 16 | `DEF_OBJECTMAGIC` maxFrame=15 |
| `frames_get_item` | 4 | `DEF_OBJECTGETITEM` maxFrame=3 |

### Frame Duration Table (`components.hpp:128-152`)

`get_state_frame_duration()` returns:

| State | Per-Frame (sec) | Per-Frame (ms) | Total Cycle Time |
|-------|-----------------|----------------|------------------|
| `stop` | 0.115 | 115 | 920ms (8 frames) |
| `move` | 0.070 | 70 | 560ms (8 frames) |
| `run` | 0.042 | 42 | 336ms (8 frames) |
| `attack` | 0.078 | 78 | 624ms (8 frames) |
| `attack_move` | 0.078 | 78 | 1014ms (13 frames) |
| `damage` | 0.070 | 70 | 560ms (8 frames) |
| `damage_move` | 0.070 | 70 | 280ms (4 frames) |
| `magic` | 0.088 | 88 | 1408ms (16 frames) |
| `magic_attack` | 0.088 | 88 | 1408ms (16 frames) |
| `dying` | 0.080 | 80 | 1040ms (13 frames) |
| `get_item` | 0.150 | 150 | 600ms (4 frames) |
| default | 0.060 | 60 | -- |

### Loop Behavior (`components.hpp:155-173`)

`should_loop()`:

- **Looping** (`true`): `stop`, `move`, `run`, `dead`
- **One-shot** (`false`): `attack`, `attack_move`, `damage`, `damage_move`, `dying`, `magic`, `magic_attack`, `get_item`

### `set_state()` (`components.hpp:177-187`)

Resets the animation when the state changes:

```cpp
void set_state(entity_anim_state new_state)
{
    if (state == new_state && !finished) return;  // Already in this state
    state = new_state;
    current_frame = 0;
    frame_timer = 0.0f;
    frame_count = get_state_frame_count();
    frame_duration = get_state_frame_duration();
    looping = should_loop();
    finished = false;
    attack_triggered = false;
}
```

The early-return prevents animation restarts when the same state is set continuously (e.g. server re-sending "move" while already moving).

---

## Action Mapping

### `entity::set_action()` (`entity/entity.cpp:9-49`)

Maps network/gameplay `object_action` to `entity_anim_state`:

| `object_action` | `entity_anim_state` |
|-----------------|---------------------|
| `stop_peace`, `stop_combat` | `stop` |
| `move_peace`, `move_combat` | `move` |
| `run` | `run` |
| `attack_peace`, `attack_combat`, `attack_combat_bow` | `attack` |
| `magic` | `magic` |
| `get_item` | `get_item` |
| `damage` | `damage` |
| `dying` | `dying` |
| default | `stop` |

Stores `current_action_` (the raw `object_action`) for sprite ID calculation, then calls `animation_.set_state()`.

### `entity::set_action_with_combat_mode()` (`entity/entity.cpp:52-74`)

Wraps `set_action()` with combat mode awareness:

| Base Action | + Combat Mode | Result |
|-------------|---------------|--------|
| `stop_peace` | `true` | `stop_combat` |
| `move_peace` | `true` | `move_combat` |
| `attack_peace` | `true` | `attack_combat` |
| anything else | any | pass-through |

This matters for sprite selection: combat/peace actions reference different sprite groups.

### NPC Action Mapping (`entity_manager.cpp:83-108`)

`action_to_npc_action_index()` compresses `object_action` into 7 NPC action indices:

| `object_action` | NPC Index | Meaning |
|-----------------|-----------|---------|
| `stop_peace`, `stop_combat` | 0 | Stop |
| `move_peace`, `move_combat`, `run` | 1 | Move |
| `attack_peace`, `attack_combat`, `attack_combat_bow` | 2 | Attack |
| `damage` | 3 | Damage |
| `dying` | 4 | Dying |
| `magic` | 6 | Magic |
| `get_item`, default | 0 | Stop |

Note: index 5 (dead) is never returned here -- the dead state uses `dying` action index 4 with `dead` anim state.

---

## Frame Advancement (`entity_manager.cpp:484-548`)

`update_animation()` is called for every entity each frame from `entity_manager::update()`.

### Algorithm

```
1. If finished && !looping:
   - Transition to next state (see table below)
   - Return early

2. frame_timer += delta_time

3. If frame_timer >= frame_duration:
   a. frame_timer -= frame_duration  (preserves leftover for smooth timing)
   b. current_frame++
   c. Sound triggers (see below)
   d. Boundary check:
      - Looping: wrap current_frame to 0
      - One-shot: clamp to frame_count-1, set finished=true
```

### State Transitions on Finish

| Finished State | Transitions To |
|----------------|----------------|
| `attack` | `stop` |
| `attack_move` | `stop` |
| `damage` | `stop` |
| `damage_move` | `stop` |
| `magic` | `stop` |
| `magic_attack` | `stop` |
| `get_item` | `stop` |
| `dying` | `dead` |

### Sound Triggers

Sounds fire at specific frame indices after `current_frame++`:

- **Footsteps** (frames 1 and 3): during `move` and `run` states. Calls `play_footstep_sound()` which dispatches to `sound_manager::play_character_sound_at()` with `walk_step` or `run_step`.
- **Attack hit** (frame 4): during `attack`, `attack_move`, and `magic_attack` states. Sets `attack_triggered = true`.

---

## Movement-Animation Synchronization (`entity_manager.cpp:550-627`)

Movement is directly tied to animation timing:

```cpp
float move_time_ms = m.running ? 336.0f : 560.0f;
t.move_progress += delta_time / (move_time_ms / 1000.0f);
```

This ensures the entity traverses exactly one tile per walk/run animation cycle:
- **Walk**: 8 frames x 70ms = 560ms per tile
- **Run**: 8 frames x 42ms = 336ms per tile

Position is linearly interpolated between tile centers:

```cpp
t.x = start_x + (end_x - start_x) * move_progress;
t.y = start_y + (end_y - start_y) * move_progress;
```

When `move_progress >= 1.0`, the entity snaps to the destination tile and either follows the next waypoint or returns to idle.

---

## Direction Handling (`core/direction_utils.hpp`)

### Conversion Functions

| Function | Input | Output | Purpose |
|----------|-------|--------|---------|
| `direction_from_protocol(int16_t)` | 0-7 | `std::optional<direction>` | Network protocol to internal (nullopt if out of range) |
| `direction_to_protocol(direction)` | 1-8 | 0-7 | Internal to network protocol |
| `direction_to_sprite_index(direction)` | `direction` | 1-8 | For sprite ID calculations |
| `direction_to_sprite_index(optional<direction>, default=5)` | `optional<direction>` | 1-8 (or default) | For sprite ID calculations with fallback |
| `calculate_direction(from_x, from_y, to_x, to_y)` | tile coords | `std::optional<direction>` | Movement direction (nullopt when from==to) |
| `direction_offset(direction)` | `direction` | `{dx, dy}` | Tile offset for a direction |
| `rotate_cw(direction)` | `direction` | `direction` | One step clockwise |
| `rotate_ccw(direction)` | `direction` | `direction` | One step counter-clockwise |

### `calculate_direction()` Algorithm

Uses slope-based classification with thresholds:

| Slope Range | dx Sign | Direction |
|-------------|---------|-----------|
| `abs(slope) > 3.0` | -- | north / south |
| `abs(slope) < 0.333` | -- | east / west |
| `slope in (0.333, 3.0)` | negative | north_west |
| `slope in (0.333, 3.0)` | positive | south_east |
| `slope in (-3.0, -0.333)` | negative | south_west |
| `slope in (-3.0, -0.333)` | positive | north_east |

---

## Two Frame Indexing Schemes

The system uses two fundamentally different approaches for mapping `(direction, frame)` to a sprite frame index:

### Scheme A: Direction-in-Sprite-ID

Used for **body sprites** and **NPC/monster sprites**.

- The sprite ID itself encodes the direction (e.g., `body_base + action*8 + (dir-1)`)
- Each sprite contains only frames for one direction
- Frame index = `current_frame` (0 to frame_count-1)

Example: body facing east (dir=3) doing `move_peace` (action=2):
```
sprite_id = 500 + (owner_type-1)*120 + 2*8 + (3-1) = 500 + owner*120 + 18
frame = current_frame  (0-7)
```

### Scheme B: Direction-in-Frame-Index

Used for **underwear, hair, armor, pants, boots, helmet, mantle** layers.

- One sprite per action contains all 8 directions interleaved
- Frame index = `(direction - 1) * 8 + current_frame`
- Up to 64 frames per sprite (8 dirs x 8 frames)

Example: underwear facing east (dir=3) at frame 5:
```
sprite_id = underwear_base + color*15 + action
frame = (3-1)*8 + 5 = 21
```

### Scheme for Weapons

Weapons use a hybrid: `weapon_stride = 64` (8 actions x 8 directions), and the sprite is indexed by action and direction, but frames are indexed as `current_frame` only. Each weapon type gets 64 sprites total.

### Scheme for Shields

Shields use `shield_stride = 8` (8 direction sprites per shield type). Frame index = `current_frame`.

---

## Sprite ID Calculation Formulas

### Character Body (`entity_manager.cpp:52-59`)

```
body_id = 500 + (owner_type - 1) * 120 + action * 8 + (direction - 1)
```

Where `owner_type` = 1-3 male (skin 1-3), 4-6 female (skin 1-3):
```cpp
owner_type = is_female ? (3 + skin_color) : skin_color;  // skin_color clamped to 1-3
```

### Character Underwear (`entity_manager.cpp:62-69`)

```
underwear_id = base + color * 15 + action
```

| Gender | Base |
|--------|------|
| Male | 4580 |
| Female | 14580 |

`color` clamped to 0-7. Frame index = `(dir-1)*8 + current_frame`.

### Character Hair (`entity_manager.cpp:72-79`)

```
hair_id = base + style * 15 + action
```

| Gender | Base |
|--------|------|
| Male | 4820 |
| Female | 14820 |

`style` clamped to 0-7. Frame index = `(dir-1)*8 + current_frame`.

### NPC/Monster (`entity_manager.cpp:112-123`)

```
npc_id = 20000 + (visual_type - 10) * 56 + npc_action * 8 + (direction - 1)
```

- `visual_type` starts at 10 (Slime = 10, Skeleton = 11, ...)
- `56 = 7 actions * 8 directions` per NPC type
- `npc_action` = 0-6 (stop, move, attack, damage, dying, dead, magic)
- Frame index = `current_frame`

### Equipment Sprite Constants (`menu_character_renderer.hpp:101-152`)

All equipment uses the same pattern: `base + (type-1) * stride + action`

| Category | Male Base | Female Base | Stride | Notes |
|----------|-----------|-------------|--------|-------|
| Body | 500 | 500+3*120=860 | 120 | 15 actions x 8 dirs per body |
| Underwear | 4580 | 14580 | 15 | Per color (0-7) |
| Hair | 4820 | 14820 | 15 | Per style (0-7) |
| Body armor | 5060 | 15060 | 15 | Per type (1-15) |
| Arm armor | 5300 | 15300 | 15 | Per type (1-15) |
| Pants | 5540 | 15540 | 15 | Per type (1-15) |
| Boots | 5780 | 15780 | 15 | Per type (1-15) |
| Weapons | 6020 | 16020 | 64 | 8 actions x 8 dirs per weapon |
| Shields | 9100 | 19100 | 8 | Per type (1-9) |
| Mantles | 9600 | 19600 | 15 | Per type |
| Helmets | 9300 | 19300 | 15 | Per type |

---

## Sprite Loading Pipeline

### PAK File Format (`assets/pak_file.hpp`)

Helbreath's proprietary sprite archive:

```
[0..23]           File header (24 bytes)
[24..]            Offset table: N entries of 8 bytes (4-byte offset + 4-byte padding)
[offsets..]       Sprite data blocks
```

Each sprite data block:
```
[+0..+99]         "Sprite Confirm" header (100 bytes, skipped)
[+100..+103]      Frame count (int32)
[+104..+104+12*N] Frame definitions (N x sprite_frame_info)
[after frames]    BMP bitmap data (embedded Windows BMP)
```

### `sprite_frame_info` (`pak_file.hpp:14-21`)

Per-frame definition (12 bytes, matches legacy `stBrush` struct):

| Field | Type | Purpose |
|-------|------|---------|
| `source_x` | `int16_t` | X position in bitmap atlas |
| `source_y` | `int16_t` | Y position in bitmap atlas |
| `width` | `int16_t` | Frame width in pixels |
| `height` | `int16_t` | Frame height in pixels |
| `pivot_x` | `int16_t` | X draw offset (positions character feet) |
| `pivot_y` | `int16_t` | Y draw offset (positions character feet) |

### Two-Tier Loading

**Tier 1 - Metadata only** (`sprite::load_metadata_from_pak()`):
- Reads frame definitions (`sprite_frame_info` array) and bitmap dimensions
- No pixel data loaded, no GPU memory allocated
- Stores `pak_source_` pointer for later bitmap loading

**Tier 2 - Bitmap loading** (`sprite::load_bitmap()`):
- Reads pixel data from PAK via stored `pak_source_` pointer
- Creates two SFML textures:
  - `texture_`: Color key applied (pixel at 0,0 becomes transparent)
  - `texture_no_colorkey_`: Original colors preserved
- GPU memory cost: `2 * width * height * 4` bytes
- BMP conversion supports 8-bit indexed, 16-bit (RGB555/RGB565), 24-bit (BGR), and 32-bit (BGRA)

**On-demand loading** (`sprite::ensure_loaded()`):
- Called before every draw operation
- If bitmap not loaded, triggers `load_bitmap()` via const-cast
- Updates `last_used_` timestamp via `touch()`

### `sprite_frame` (`assets/sprite.hpp:16-20`)

Runtime frame representation after loading:

```cpp
struct sprite_frame {
    sf::IntRect source_rect;  // Region in texture atlas
    int16_t pivot_x;          // X offset for drawing
    int16_t pivot_y;          // Y offset for drawing
};
```

---

## Sprite Manager (`assets/sprite_manager.hpp`)

### Storage Architecture

Three layers:

| Layer | Type | Key | Purpose |
|-------|------|-----|---------|
| `pak_files_` | `map<string, unique_ptr<pak_file>>` | PAK name | Open file handles |
| `sprite_cache_` | `map<sprite_key, unique_ptr<sprite>>` | `{pak_name, index}` | Loaded sprite objects |
| `id_sprites_` | `map<uint16_t, sprite*>` | Global sprite ID | O(1) numeric lookup |

### Loading Flow

1. **Initialization**: `menu_character_renderer::initialize()` iterates PAK loading tables (~100 `pak_load_entry` arrays), calling `load_pak_at_offset()` for each entry
2. **Storage**: `sprites.store_sprite_at_id(global_id, pak_name, index)` loads the sprite into the cache and creates the ID-based shortcut
3. **Runtime lookup**: `get_sprite_by_id(id)` does a direct O(1) hash map lookup into `id_sprites_`

### LRU Cache (`sprite_manager.cpp:82-121`)

On each `get_sprite()` call, the `sprite_key` is moved to the back of `lru_order_`. `shrink_cache()` evicts entries from the front.

### Memory Eviction (`sprite_manager.cpp:221-249`)

`update_memory(delta_time)`:
- Runs every **5 seconds** (`eviction_check_interval_`)
- Scans all cached sprites; unloads bitmaps unused for **30 seconds** (`eviction_timeout_`)
- `sprite::unload_bitmap()` replaces textures with 1x1 placeholders, freeing GPU memory
- Metadata is preserved; sprites reload on next draw via `ensure_loaded()`

### Usage Tracking (`sprite.cpp:296-304`)

- `touch()`: Updates `last_used_` to current steady_clock time
- `seconds_since_use()`: Returns elapsed seconds since last touch
- `memory_usage()`: Returns `2 * bitmap_width * bitmap_height * 4`

---

## Render Pipeline

### Frame-Level Flow (`gameplay/game_state.cpp:838`)

```
render_playing():
  1. world_.apply_zoom_view(rend)       -- Set SFML zoom view
  2. world_.render(rend)                -- Map terrain + objects
  3. entities_.render(rend, sprites...) -- All entities (Y-sorted)
  4. floating_text_.render(...)         -- Damage numbers
  5. world_.reset_zoom_view(rend)       -- Reset view
  6. UI layer (debug, status log, dialogs)
```

### Visibility Culling (`entity_manager.cpp:629-658`)

For each entity:
- Skip if `should_remove()` or `!sprite().visible`
- Calculate screen position: `screen_x = world_x - camera_x`
- Include if within `[-128, screen_width+128]` x `[-128, screen_height+128]`
- **Global render mode** bypasses culling entirely

### Depth Sorting (`entity_manager.cpp:660-664`)

```cpp
std::sort(visible_entities.begin(), visible_entities.end(),
    [](const entity* a, const entity* b) {
        return a->transform().y < b->transform().y;
    });
```

Entities with smaller Y (higher on screen = farther away) render first. Entities lower on screen render on top.

### Entity Render Dispatch (`entity_manager.cpp:672-716`)

| Entity Type | Renderer | Drawing |
|-------------|----------|---------|
| `item` | Direct | `body_sprite` at `(screen_x-16, screen_y-16)` with `body_frame` |
| `effect` | Direct | `effect_sprite` at `(screen_x+offset_x, screen_y+offset_y)` |
| `npc`, `monster` | `render_npc_or_monster()` | Single sprite + effect overlay |
| `player`, `character` | `render_player_character()` | Multi-layer compositing |

After sprite layers: name overlay (on hover), health bar, chat bubble.

---

## Player Character Rendering (`entity_manager.cpp:718-771`)

### In-Game Layers (current implementation)

Three base layers are rendered:

```
1. Underwear:  sprite_id = base + color*15 + action,     frame = (dir-1)*8 + current_frame
2. Body:       sprite_id = 500 + (owner-1)*120 + action*8 + (dir-1), frame = current_frame
3. Hair:       sprite_id = base + style*15 + action,     frame = (dir-1)*8 + current_frame
   (only if no helmet equipped)
4. Effect:     if present, draws at current_frame
```

Alpha transparency: when `sprite.alpha < 1.0`, uses `draw_sprite_alpha()` with alpha byte `clamp(alpha * 255, 0, 255)`.

> **Note**: Armor, weapon, shield, helmet, mantle, and boots layers are TODO for in-game rendering (`entity_manager.cpp:765`). They are fully implemented in the menu character renderer.

---

## NPC/Monster Rendering (`entity_manager.cpp:773-812`)

Single sprite per NPC/monster:

```
sprite_id = 20000 + (visual_type - 10) * 56 + npc_action * 8 + (direction - 1)
frame = current_frame
```

Fallback: if `visual_type < 10`, defaults to 10 (Slime).

Effect overlay drawn on top if `effect_sprite` is set.

---

## Menu Character Rendering (`graphics/menu_character_renderer.cpp:323-409`)

Full equipment layering with up to 15 draw calls per character. Used by character select and create screens.

### Draw Order

The order depends on facing direction, controlled by two arrays (`menu_character_renderer.hpp:161-163`):

```cpp
// Weapon: 0 = drawn LAST (in front), 1 = drawn FIRST (behind body)
static constexpr int8_t drawing_order[9] = {0, 1, 0, 0, 0, 0, 0, 1, 1};
//                                          idx: 0  1  2  3  4  5  6  7  8
//                                          dir: -  N  NE E  SE S  SW W  NW

// Mantle position: 0=early, 1=after shield, 2=between armor and shield
static constexpr int8_t mantle_drawing_order[9] = {0, 1, 1, 1, 0, 0, 0, 2, 2};
```

When facing **N** (1), **W** (7), or **NW** (8): weapon is drawn first (behind body).
When facing other directions: weapon is drawn last (in front).

### Full Layer Sequence

```
 1. [if weapon_behind]           Weapon
 2.                              Body (skin)
 3. [if mantle_order == 0]       Mantle (early)
 4.                              Underwear
 5. [if no helmet]               Hair
 6. [if skirt]                   Boots (early, under skirt)
 7.                              Pants / Skirt
 8.                              Arm armor
 9. [if not skirt]               Boots (late, over pants)
10.                              Body armor
11.                              Helmet
12. [if mantle_order == 2]       Mantle (between armor and shield)
13.                              Shield
14. [if mantle_order == 1]       Mantle (after shield)
15. [if !weapon_behind]          Weapon
```

Skirt detection: `pants == 1 && gender == female`.

### Menu Animation Timer

Character select/create screens use a simpler 100ms-per-frame timer:

```cpp
frame_timer_ += delta_time;
if (frame_timer_ >= 0.1f) {
    frame_timer_ = 0.0f;
    // advance frame 0-7...
}
```

---

## Sprite Draw Methods (`assets/sprite.cpp:324-435`)

All draw methods follow the same pattern:

1. `ensure_loaded()` -- triggers on-demand bitmap loading if needed
2. Validate frame index against `frames_.size()`
3. Look up `sprite_frame` for pivot offsets and source rect
4. Create `sf::Sprite` with texture and source rect
5. Position at `(x + pivot_x, y + pivot_y)` -- pivots align character feet to position
6. Submit to SFML `RenderTarget`

### Available Draw Methods

| Method | Texture Used | Alpha | Notes |
|--------|-------------|-------|-------|
| `draw()` | `texture_` (color-keyed) | 1.0 | Standard character/object draw |
| `draw_alpha()` | `texture_` | 0.0-1.0 | Semi-transparent (invisibility, fade) |
| `draw_width()` | `texture_` | 1.0 | Partial-width render (HP/MP bar fills) |
| `draw_no_color_key()` | `texture_no_colorkey_` | 1.0 | Backgrounds (no transparency) |
| `draw_alpha_no_color_key()` | `texture_no_colorkey_` | 0.0-1.0 | Semi-transparent backgrounds |

### Bounds Query

`get_bounds(x, y, frame)` returns `sf::IntRect` at `(x + pivot_x, y + pivot_y)` with the frame's source rect dimensions. Used for mouse-over hit testing in `entity_manager::is_point_in_entity_sprite()`.

---

## `character_appearance` Struct (`menu_character_renderer.hpp:11-38`)

Full appearance data for rendering a character preview:

| Field | Type | Range | Default |
|-------|------|-------|---------|
| `gender` | `uint8_t` | 1-2 | 1 (male) |
| `skin_color` | `uint8_t` | 1-3 | 1 |
| `hair_style` | `uint8_t` | 0-7 | 0 |
| `hair_color` | `uint8_t` | 0-15 | 0 |
| `underwear_color` | `uint8_t` | 0-7 | 0 |
| `body_armor` | `uint8_t` | 0-15 | 0 (none) |
| `arm_armor` | `uint8_t` | 0-15 | 0 |
| `pants` | `uint8_t` | 0-15 | 0 |
| `boots` | `uint8_t` | 0-15 | 0 |
| `helmet` | `uint8_t` | 0-15 | 0 |
| `mantle` | `uint8_t` | 0-15 | 0 |
| `weapon` | `uint8_t` | 0-255 | 0 |
| `shield` | `uint8_t` | 0-9 | 0 |
| `weapon_glow` | `uint8_t` | 0-3 | 0 (none) |
| `shield_glow` | `uint8_t` | 0-3 | 0 (none) |

---

## `sprite_component` (`entity/components.hpp:26-49`)

Per-entity sprite state:

| Field | Type | Purpose |
|-------|------|---------|
| `body_sprite` | `const sprite*` | Body sprite pointer |
| `armor_sprite` | `const sprite*` | Armor sprite pointer |
| `weapon_sprite` | `const sprite*` | Weapon sprite pointer |
| `shield_sprite` | `const sprite*` | Shield sprite pointer |
| `helm_sprite` | `const sprite*` | Helmet sprite pointer |
| `hair_sprite` | `const sprite*` | Hair sprite pointer |
| `underwear_sprite` | `const sprite*` | Underwear sprite pointer |
| `effect_sprite` | `const sprite*` | Effect overlay pointer |
| `body_frame` | `uint8_t` | Explicit body frame override |
| `armor_frame` | `uint8_t` | Explicit armor frame override |
| `weapon_frame` | `uint8_t` | Explicit weapon frame override |
| `skin_color` | `uint8_t` | Skin color (1-3) |
| `hair_style` | `uint8_t` | Hair style (0-7) |
| `hair_color` | `uint8_t` | Hair color (0-15) |
| `underwear_color` | `uint8_t` | Underwear color (0-7) |
| `gender` | `uint8_t` | 1=male, 2=female |
| `alpha` | `float` | Transparency (0.0-1.0) |
| `visible` | `bool` | Visibility flag |
| `flipped` | `bool` | Horizontal flip (not yet applied) |

---

## Entity Component Architecture

### Always-Present Components (`entity.hpp:124-126`)

Every entity has these three components unconditionally:

```cpp
transform_component transform_;   // Position, direction, movement interpolation
sprite_component sprite_;          // Layer pointers, appearance data, alpha
animation_component animation_;    // State machine, frame counter, timing
```

### Optional Components (`entity.hpp:129-136`)

Added on-demand via `add_X()` methods, checked via `has_X()`:

- `stats_component` -- HP, MP, SP, level, attributes
- `combat_component` -- Combat mode, target, status effects
- `name_component` -- Display name, guild, chat bubble
- `movement_component` -- Speed, path, waypoints
- `npc_component` -- NPC type, shop/quest flags
- `monster_component` -- Monster type, boss/summon flags
- `item_component` -- Item type, amount, owner
- `effect_component` -- Visual effect type, duration, offset

---

## Not Yet Implemented

Features with data structures defined but not active in the in-game renderer:

| Feature | Status | Notes |
|---------|--------|-------|
| Equipment layers in-game | TODO at `entity_manager.cpp:765` | Only rendered in menu screens |
| Damage flash / color modulation | Not present | No shader or color tinting |
| Shadow rendering | No-op | `draw_with_shadow()` calls `draw()` |
| Weapon/shield glow effects | Fields exist in `character_appearance` | Never rendered |
| Hair color tinting | `hair_color` stored | Never applied |
| Horizontal sprite flip | `flipped` field in `sprite_component` | Never applied |
| Death fade-out transparency | Not implemented | Dead entities stay at full alpha |
| Palette swapping | Not implemented | Legacy used palette manipulation |

---

## Key Files Reference

| File | Purpose |
|------|---------|
| `entity/components.hpp` | `entity_anim_state`, `animation_component` with all frame constants and timing |
| `entity/entity.hpp` | Entity class with component accessors |
| `entity/entity.cpp` | `set_action()` and `set_action_with_combat_mode()` |
| `entity/entity_manager.hpp` | Entity manager API |
| `entity/entity_manager.cpp` | Frame advancement, state transitions, sprite ID calculation, render pipeline, movement sync |
| `core/game_enums.hpp` | `object_action`, `direction` enums |
| `core/direction_utils.hpp` | Direction conversion, calculation, rotation |
| `assets/sprite.hpp` | `sprite_frame`, `sprite` class with two-tier loading |
| `assets/sprite.cpp` | PAK loading, color key masking, all draw methods |
| `assets/sprite_manager.hpp` | Sprite cache, PAK management, ID-based lookup |
| `assets/sprite_manager.cpp` | LRU cache, memory eviction, `store_sprite_at_id()` |
| `assets/pak_file.hpp` | PAK file format structs (`sprite_frame_info`, `pak_sprite_data`) |
| `assets/pak_file.cpp` | PAK reader, BMP conversion (8/16/24/32-bit to RGBA) |
| `graphics/menu_character_renderer.hpp` | Equipment sprite constants, draw order arrays, `character_appearance` |
| `graphics/menu_character_renderer.cpp` | PAK loading tables, 15-layer draw order, per-equipment draw functions |
| `graphics/renderer.hpp` | SFML renderer wrapper |
| `network/handlers/motion_handlers.cpp` | Network-driven animation state changes |
| `gameplay/game_state.cpp` | Render pipeline orchestration |
