# Character Movement

Exhaustive documentation of the tile-based movement system used for all entities in the modernized Helbreath client (`src/`).

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Coordinate Systems](#coordinate-systems)
3. [Tile Grid](#tile-grid)
4. [Direction System](#direction-system)
5. [Data Structures](#data-structures)
6. [Input Handling](#input-handling)
7. [Pathfinding](#pathfinding)
8. [Client-Side Prediction](#client-side-prediction)
9. [Movement Interpolation](#movement-interpolation)
10. [Server Reconciliation](#server-reconciliation)
11. [Action Queue System](#action-queue-system)
12. [Run Mode](#run-mode)
13. [Combat Mode Integration](#combat-mode-integration)
14. [Other Entity Movement](#other-entity-movement)
15. [Animation Integration](#animation-integration)
16. [Sound Integration](#sound-integration)
17. [Camera Following](#camera-following)
18. [Rendering](#rendering)
19. [Debug Visualization](#debug-visualization)
20. [Network Protocol](#network-protocol)
21. [Key Files Reference](#key-files-reference)

---

## Architecture Overview

Movement follows a **client-predicted, server-authoritative** model processed one tile at a time:

```
Input (mouse/keyboard)
  -> Pathfinding (one step toward destination)
  -> Client-side prediction (begin interpolation)
  -> Network request (send to server)
  -> Server response (accept/reject)
  -> Reconciliation (rubber-band or revert)
```

The local player moves tile-by-tile toward a destination. Each step is predicted locally, sent to the server for validation, and corrected if necessary. Remote entities receive position updates from the server and are either teleported or interpolated depending on the protocol.

---

## Coordinate Systems

Three coordinate spaces are used throughout the movement system:

### Tile Coordinates

Integer grid positions. All pathfinding, collision, and server communication operates in tile space.

- **Range**: 0 to 751 (map max is 752x752)
- **Stored in**: `transform_component::tile_x`, `tile_y`
- **Used by**: `map::is_walkable()`, `get_next_walkable_dir()`, network messages

### World Pixel Coordinates

Pixel positions derived from tile coordinates. Used for interpolation, rendering, and camera positioning.

- **Formula**: `world_x = tile_x * 32 + 16`, `world_y = tile_y * 32 + 16`
- **The `+16` centers** the entity at the middle of the 32x32 tile
- **Stored in**: `transform_component::x`, `y`
- **Updated every frame** during movement interpolation

### Screen Coordinates

Position relative to the viewport, computed by subtracting camera offset from world position.

- **Formula**: `screen_x = world_x - camera_x`, `screen_y = world_y - camera_y`
- **Zoom-aware**: When zoom is active, `screen_to_world()` applies the zoom transform:
  ```
  world_x = camera_x + (screen_width/2) + (screen_x - screen_width/2) * zoom_level
  ```

### Conversion Functions

| Function | File | Purpose |
|----------|------|---------|
| `world::screen_to_tile(sx, sy)` | `world/world.cpp:302` | Screen pixels to tile coordinates |
| `world::screen_to_world(sx, sy)` | `world/world.cpp:308` | Screen pixels to world pixels (zoom-aware) |
| `world::world_to_screen(wx, wy)` | `world/world.cpp:324` | World pixels to screen pixels (zoom-aware) |
| `map::world_to_tile_x(wx)` | `world/map.hpp:53` | `wx / tile_width` |
| `map::tile_to_world_x(tx)` | `world/map.hpp:55` | `tx * tile_width` |

---

## Tile Grid

**File**: `world/tile.hpp`

### Dimensions

```cpp
inline constexpr int32_t tile_width = 32;   // pixels
inline constexpr int32_t tile_height = 32;  // pixels
inline constexpr int32_t max_map_width = 752;   // tiles
inline constexpr int32_t max_map_height = 752;  // tiles
```

### Tile Flags

Each tile has a bitfield of `tile_flag` values that determine behavior:

| Flag | Bit | Movement Effect |
|------|-----|-----------------|
| `walkable` | 0 | Required for movement. Tile is passable |
| `water` | 1 | Visual only (currently) |
| `lava` | 2 | Visual only (currently) |
| `ice` | 3 | Visual only (currently) |
| `swamp` | 4 | Visual only (currently) |
| `teleport` | 5 | Triggers map transition when stepped on |
| `blocks_sight` | 6 | Used by `has_line_of_sight()` |
| `blocks_magic` | 7 | Not used by movement |
| `safe_zone` | 8 | PvP restriction (not movement) |
| `pvp_zone` | 9 | PvP permission (not movement) |
| `occupied` | 10 | Set when an entity stands on the tile |
| `item_present` | 11 | Not used by movement |

### Walkability Check

**File**: `world/map.cpp:186`

```cpp
bool map::is_walkable(int32_t x, int32_t y) const
{
    if (!is_valid_position(x, y)) return false;
    const auto& t = tiles_[tile_index(x, y)];
    return t.is_walkable() && !t.is_occupied();
}
```

A tile is walkable when:
1. Position is within map bounds (`is_valid_position`)
2. The `walkable` flag is set
3. The `occupied` flag is **not** set

### Nearest Walkable Search

**File**: `world/map.cpp:227`

`find_nearest_walkable(x, y, range)` performs a spiral outward search up to `range` tiles (default 3) from a position. Used when an entity needs to be placed near a blocked tile.

---

## Direction System

**File**: `core/game_enums.hpp:144`, `core/direction_utils.hpp`

### Direction Enum

8 cardinal/diagonal directions plus `none`:

```cpp
enum class direction : uint8_t {
    none       = 0,
    north      = 1,   // Up        (dy = -1)
    north_east = 2,   // Up-right  (dx = +1, dy = -1)
    east       = 3,   // Right     (dx = +1)
    south_east = 4,   // Down-right(dx = +1, dy = +1)
    south      = 5,   // Down      (dy = +1)
    south_west = 6,   // Down-left (dx = -1, dy = +1)
    west       = 7,   // Left      (dx = -1)
    north_west = 8,   // Up-left   (dx = -1, dy = -1)
};
```

### Protocol Direction Mapping

The internal enum uses 1-8 (0 = none). The network protocol uses 0-7 (0 = North). Conversion adds/subtracts 1:

| Function | Conversion |
|----------|------------|
| `direction_from_protocol(p)` | `static_cast<direction>(p + 1)` |
| `direction_to_protocol(d)` | `static_cast<int16_t>(d) - 1` |

### Direction Utility Functions

| Function | Signature | Purpose |
|----------|-----------|---------|
| `calculate_direction` | `(from_x, from_y, to_x, to_y) -> direction` | 8-way direction from source to target using slope thresholds (0.3333 and 3.0) |
| `direction_offset` | `(dir) -> {dx, dy}` | Returns the tile delta for a direction (e.g., north = {0, -1}) |
| `rotate_cw` | `(dir) -> direction` | One step clockwise (north -> north_east) |
| `rotate_ccw` | `(dir) -> direction` | One step counter-clockwise (north -> north_west) |
| `direction_to_sprite_index` | `(dir, default_dir) -> int32_t` | Returns 1-8 for sprite selection. Defaults to 5 (south) if invalid |

### Slope Thresholds in `calculate_direction`

```
slope = dy / dx

|slope| > 3.0          -> north or south (nearly vertical)
|slope| < 0.3333       -> east or west (nearly horizontal)
0.3333 <= slope <= 3.0 -> diagonal (NE, SE, SW, NW based on signs)
```

---

## Data Structures

### `transform_component` (always present on every entity)

**File**: `entity/components.hpp:13`

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| `x` | `int32_t` | 0 | World pixel X (interpolated during movement) |
| `y` | `int32_t` | 0 | World pixel Y (interpolated during movement) |
| `tile_x` | `int32_t` | 0 | Current authoritative tile X |
| `tile_y` | `int32_t` | 0 | Current authoritative tile Y |
| `direction` | `direction` | `south` | Current facing direction |
| `move_progress` | `float` | 0.0 | 0.0 to 1.0 lerp between tiles |
| `dest_tile_x` | `int32_t` | 0 | Destination tile X (next step) |
| `dest_tile_y` | `int32_t` | 0 | Destination tile Y (next step) |
| `moving` | `bool` | false | True while interpolating between tiles |

### `movement_component` (optional, present on players/characters/monsters)

**File**: `entity/components.hpp:275`

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| `speed` | `float` | 4.0 | Tiles per second (base walking speed) |
| `run_speed` | `float` | 8.0 | Tiles per second (running) |
| `running` | `bool` | false | Whether entity is running |
| `moving` | `bool` | false | Whether entity has a movement target |
| `can_move` | `bool` | true | Whether movement is allowed |
| `target_x` | `int32_t` | 0 | Final pathfinding destination tile X |
| `target_y` | `int32_t` | 0 | Final pathfinding destination tile Y |
| `path` | `vector<pair<int32_t, int32_t>>` | empty | Pre-computed waypoint sequence |
| `path_index` | `size_t` | 0 | Current index into `path` |

### Entity Types with Movement

Created in `entity_manager::create_entity_with_id()` (`entity/entity_manager.cpp:142`):

| Entity Type | Has `movement_component` | Movement Behavior |
|-------------|--------------------------|-------------------|
| `player` | Yes | Full input-driven + server-reconciled |
| `character` | Yes | Server broadcast positions |
| `monster` | Yes | Server broadcast positions |
| `npc` | **No** | Server teleport only |
| `item` | No | Static |
| `effect` | No | Static |

### Game State Movement Variables

**File**: `gameplay/game_state.hpp`

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `run_mode_enabled_` | `bool` | false | Persistent run toggle (Ctrl+R) |
| `move_dest_x_` | `int32_t` | -1 | Current pathfinding destination (-1 = none) |
| `move_dest_y_` | `int32_t` | -1 | Current pathfinding destination (-1 = none) |
| `player_turn_` | `bool` | false | Alternates CW/CCW obstacle avoidance |
| `prev_tile_x_` | `int32_t` | -1 | Previous tile for doubleback detection |
| `prev_tile_y_` | `int32_t` | -1 | Previous tile for doubleback detection |
| `combat_mode_` | `bool` | false | Attack stance (Tab toggles) |
| `blocked_movement_cooldown_` | `float` | 0.0 | Countdown preventing actions (250ms after collision) |
| `pending_action_` | `queued_action` | none | Action waiting for current animation to complete |

---

## Input Handling

**File**: `gameplay/game_state.cpp:1199` (`handle_movement_input`)

Input is processed in `update_playing()` which calls `handle_playing_input()` every frame. Movement input is checked only when no modal UI is open and cinematic drag is not active.

### Mouse Click-to-Move

#### Left Click (Initial Press) - `is_mouse_pressed`

1. Convert screen position to tile: `world_.screen_to_tile(mouse_x, mouse_y)`
2. Check if click lands on the player's own sprite: `is_point_in_entity_sprite()`
   - **Plain click on self**: Triggers pickup request at current tile
   - **Ctrl+click on self**: Attacks the entity on the tile directly north
   - Both check `can_perform_action()` and fall back to queuing if busy
   - Returns immediately (not treated as movement)

#### Left Click Held - `is_mouse_down`

1. Convert screen to tile
2. Skip if hovering own sprite
3. Skip if destination equals current tile
4. Set `move_dest_x_` / `move_dest_y_` to the mouse tile (continuously updated)
5. Reset `prev_tile_x_` / `prev_tile_y_` to `-1` when destination changes (prevents stale doubleback detection from blocking the new direction)

#### Right Click (Initial Press)

Queues a `stop` action to cancel active movement.

#### Right Click Held

Continuously calculates the direction from the player to the mouse tile using slope-based thresholds:

```
|dx| > |dy| * 2  -> horizontal (east/west)
|dy| > |dx| * 2  -> vertical (north/south)
otherwise         -> diagonal
```

Updates the facing direction locally for immediate visual feedback. Sends a `player_stop_request` to the server if `can_perform_action()` is true, otherwise queues a `face_direction` action.

Also clears `move_dest_x_` / `move_dest_y_` to interrupt pathfinding.

### Keyboard Movement (WASD / Arrow Keys)

**File**: `gameplay/game_state.cpp:1460`

1. Reads WASD and arrow key state into `dx` / `dy` (-1, 0, or 1 each)
2. Any keyboard input immediately clears mouse destination (`move_dest_x_ = -1`)
3. Maps `{dx, dy}` directly to a `direction` enum value
4. Checks walkability of target tile:
   - `map.is_walkable(next_x, next_y)`
   - No living entities blocking the tile (`get_entities_on_tile()`)
5. Applies client-side prediction and sends network request (same as mouse movement)

**Key difference from mouse**: Keyboard movement does not use the obstacle-avoidance pathfinding. If the direct tile is blocked, movement simply does not occur.

---

## Pathfinding

**File**: `gameplay/pathfinding.hpp`

The system uses a **greedy single-step** pathfinding approach - not A* or any full path algorithm. Each frame, it computes the next single tile to move toward.

### `get_next_move_dir(from_x, from_y, to_x, to_y)`

Simple 8-direction calculation with no obstacle awareness. Returns the direction that moves closest to the destination:

```
dx > 0, dy > 0 -> south_east
dx > 0, dy < 0 -> north_east
dx < 0, dy > 0 -> south_west
... etc
```

### `get_next_walkable_dir(from, to, player_turn, is_passable)`

Obstacle-avoidance pathfinding ported from the legacy `CGame::cGetNextMoveDir`. Algorithm:

1. Compute the **base direction** toward the destination via `get_next_move_dir()`
2. Try a **3-direction cone** in the preferred rotation:
   - Step 0: base direction (straight toward target)
   - Step 1: base + 1 rotation (45 degrees)
   - Step 2: base + 2 rotations (90 degrees)
   - Rotation direction is CW if `player_turn == false`, CCW if `true`
3. If the preferred cone is fully blocked, try the **opposite rotation** (steps 1 and 2 only, base was already checked)
4. Returns `std::nullopt` if all 5 candidates are blocked

### Zigzag Behavior

`player_turn_` alternates each step (`player_turn_ = !player_turn_`), causing the pathfinder to try CW one step then CCW the next. This creates a zigzag pattern around obstacles instead of always hugging one side.

### Passability Predicate

The lambda passed to `get_next_walkable_dir` in `handle_movement_input` checks both terrain and entity occupation:

```cpp
auto is_passable = [&](int32_t x, int32_t y) -> bool {
    if (!world_.current_map().is_walkable(x, y)) return false;
    for (auto* e : entities_.get_entities_on_tile(x, y)) {
        if (e && e->is_alive() && e != player) return false;
    }
    return true;
};
```

### Doubleback Detection

**File**: `gameplay/game_state.cpp:1401`

Prevents the player from oscillating between two tiles when stuck:

1. If the computed next tile equals `prev_tile_x_` / `prev_tile_y_` (where we just came from)
2. AND the user is NOT actively holding mouse or movement keys
3. THEN stop movement and clear destination

This check only triggers when the player releases input, allowing held-input movement to push through narrow passages.

#### Doubleback Tracking Resets

`prev_tile_x_` / `prev_tile_y_` are reset to `-1` in three situations to prevent stale tracking data from falsely blocking movement:

| Trigger | File Location | Why |
|---------|---------------|-----|
| Mouse destination changes | `game_state.cpp:1286` | New click direction should not be blocked by old doubleback state |
| Queued `move` action executes | `game_state.cpp:1140` | Deferred movement starts fresh without stale history |
| Game data cleared (`clear_game_data`) | `game_state.cpp:584` | Full movement state reset on re-entry |

---

## Client-Side Prediction

**File**: `gameplay/game_state.cpp:1436` (mouse), `1511` (keyboard)

When the pathfinder selects a valid next tile:

```cpp
// 1. Set destination on transform
t.dest_tile_x = next_x;
t.dest_tile_y = next_y;
t.direction = dir;
t.moving = true;
t.move_progress = 0.0f;

// 2. Set running flag from input
movement.running = should_run;

// 3. Start animation
player->set_action_with_combat_mode(base_action, combat_mode_);

// 4. Send to server
json msg = make_player_move_request(t.tile_x, t.tile_y, dir_protocol,
                                    should_run, move_dest_x_, move_dest_y_);
ws_connection_.send(msg);
```

The entity immediately begins interpolating toward the destination tile. If the server rejects the move, the prediction is reverted (see [Server Reconciliation](#server-reconciliation)).

---

## Movement Interpolation

**File**: `entity/entity_manager.cpp:550` (`update_movement`)

Called every frame for every entity with a `movement_component` while `transform.moving == true`.

### Timing

Movement speed is derived from legacy animation frame timing, not from `movement_component::speed`:

| Mode | Frames | Frame Duration | Total Time per Tile |
|------|--------|----------------|---------------------|
| Walk | 8 | 70ms | **560ms** |
| Run | 8 | 42ms | **336ms** |

```cpp
float move_time_ms = m.running ? 336.0f : 560.0f;
float move_time_sec = move_time_ms / 1000.0f;
t.move_progress += delta_time / move_time_sec;
```

### Interpolation Formula

While `move_progress < 1.0`:

```cpp
int32_t start_x = tile_x * 32 + 16;    // Source tile center
int32_t start_y = tile_y * 32 + 16;
int32_t end_x = dest_tile_x * 32 + 16;  // Destination tile center
int32_t end_y = dest_tile_y * 32 + 16;

x = start_x + (int32_t)((end_x - start_x) * move_progress);
y = start_y + (int32_t)((end_y - start_y) * move_progress);
```

### Arrival

When `move_progress >= 1.0`:

1. Snap tile coordinates to destination: `tile_x = dest_tile_x`
2. Snap world position to tile center: `x = tile_x * 32 + 16`
3. Reset: `move_progress = 0.0`, `moving = false`
4. Check for next waypoint in `movement.path`:
   - If path has more entries and next tile is walkable, continue
   - If next tile is blocked, clear path and return to idle
5. If no path remaining, check if final destination reached:
   - If `target_x/y` reached or invalid (-1), return to idle
   - Otherwise, stay stopped but keep the target (wait for next position update)

### Direction Update During Movement

```cpp
if (dest_tile_x != tile_x || dest_tile_y != tile_y) {
    t.direction = calculate_direction(tile_x, tile_y, dest_tile_x, dest_tile_y);
}
```

The entity always faces the tile it's moving toward.

---

## Server Reconciliation

### Move Response (`handle_player_move_response`)

**File**: `gameplay/game_state.cpp:2973`

#### Rejection

The server may reject a move for two reasons, identified by the `error` field:

| Error | Cause | Client Response |
|-------|-------|-----------------|
| `"blocked_occupied"` | Entity blocking the tile | Revert prediction, 250ms cooldown, hurt sound (C12/C13), clear destination |
| `"blocked_terrain"` | Tile not walkable on server | Revert prediction, 250ms cooldown, clear destination |

Revert sequence:
```cpp
t.moving = false;
t.move_progress = 0.0f;
t.dest_tile_x = t.tile_x;
t.dest_tile_y = t.tile_y;
// Correct to server position if provided
t.tile_x = data.x; t.tile_y = data.y;
t.x = data.x * 32 + 16; t.y = data.y * 32 + 16;
player->set_action_with_combat_mode(stop_peace, combat_mode_);
```

#### Acceptance - Position Matches

If `dest_tile == server_position`: do nothing. Let the client-side interpolation continue naturally.

#### Acceptance - Position Mismatch (Rubber-Band)

If the server reports a different position than the client predicted:

```cpp
t.tile_x = data.x; t.tile_y = data.y;
t.dest_tile_x = data.x; t.dest_tile_y = data.y;
t.x = data.x * 32 + 16; t.y = data.y * 32 + 16;
t.moving = false; t.move_progress = 0.0f;
blocked_movement_cooldown_ = 0.25f;  // Prevent spam
move_dest_x_ = -1; move_dest_y_ = -1;
```

### Stop Response (`handle_player_stop_response`)

**File**: `gameplay/game_state.cpp:2946`

Updates the local player's tile position and direction to match the server's authoritative state.

### Position Update Broadcast (`handle_player_position_update`)

**File**: `gameplay/game_state.cpp:2901`

For remote entities: teleports them to the new position. For the local player during movement: the update is **ignored** to avoid disrupting interpolation.

### Legacy Binary Protocol Rubber-Band

**File**: `network/handlers/motion_handlers.cpp:38`

The legacy handler uses a distance threshold:

```cpp
int32_t dx = std::abs(t.tile_x - server_x);
int32_t dy = std::abs(t.tile_y - server_y);
if (dx > 1 || dy > 1) {
    // Snap to server position
    t.tile_x = server_x; t.tile_y = server_y;
    t.x = server_x * 32 + 16;
    t.y = server_y * 32 + 32;  // Note: +32 (legacy inconsistency)
    t.move_progress = 0.0f;
}
```

---

## Action Queue System

**File**: `gameplay/game_state.hpp:222`, `gameplay/game_state.cpp:1073`

Prevents overlapping actions. New actions are queued while the player is busy.

### `can_perform_action()`

Returns `false` when any of these conditions hold:
1. `blocked_movement_cooldown_ > 0.0f` (after a collision)
2. `player->transform().moving == true` (mid-tile interpolation)
3. A non-looping animation is playing and not finished (attack, magic, damage, get_item, dying)

### Queue Behavior

```cpp
void queue_action(queued_action action);    // Store if can't execute now
void process_queued_action();               // Execute when free (called every frame)
```

### Queued Action Types

| Type | Deferred Action |
|------|-----------------|
| `move` | Sets `move_dest_x_` / `move_dest_y_`, resets `prev_tile_x_` / `prev_tile_y_` |
| `attack` | Calls `network_.request_attack()` |
| `pickup` | Calls `request_pickup()` |
| `magic` | Calls `network_.request_magic()` |
| `face_direction` | Sends stop request with new direction |
| `stop` | Clears destination, sends stop if was moving |

### Blocked Movement Cooldown

```cpp
static constexpr float blocked_movement_cooldown_duration = 0.25f;  // 250ms
```

Applied when:
- Server rejects movement due to `blocked_occupied`
- Server rejects movement due to `blocked_terrain`
- Server confirms movement but with a position mismatch

The cooldown prevents rapid retry spam and gives a brief "stunned" feel when walking into obstacles.

---

## Run Mode

Two mechanisms control whether the player runs:

### Persistent Toggle (Ctrl+R)

**File**: `gameplay/game_state.cpp:1715`

```cpp
run_mode_enabled_ = !run_mode_enabled_;
```

When enabled, all movement uses run speed until toggled off.

### Shift Override

**File**: `gameplay/game_state.cpp:1431`

Holding Left Shift or Right Shift forces running for the current frame, regardless of the toggle state.

```cpp
bool should_run = inp.is_key_down(sf::Keyboard::Key::LShift) ||
                  inp.is_key_down(sf::Keyboard::Key::RShift) ||
                  run_mode_enabled_;
```

### Run vs Walk Differences

| Aspect | Walk | Run |
|--------|------|-----|
| `object_action` | `move_peace` / `move_combat` | `run` (no combat variant) |
| Animation frame duration | 70ms | 42ms |
| Movement time per tile | 560ms | 336ms |
| Footstep sound | `character_sound::walk_step` | `character_sound::run_step` |
| Movement component | `running = false` | `running = true` |

---

## Combat Mode Integration

**File**: `gameplay/game_state.cpp:1756`

### Toggle

Tab key toggles `combat_mode_`. When toggled while idle, the player's action updates to the combat stance variant:

```cpp
player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
```

### Action Variants

`set_action_with_combat_mode()` maps base actions to their combat equivalents:

| Base Action | Combat=false | Combat=true |
|-------------|-------------|-------------|
| `stop_peace` | `stop_peace` (sprites 0-7) | `stop_combat` (sprites 8-15) |
| `move_peace` | `move_peace` (sprites 16-23) | `move_combat` (sprites 24-31) |
| `attack_peace` | `attack_peace` (sprites 40-47) | `attack_combat` (sprites 48-55) |
| `run` | `run` (sprites 32-39) | `run` (no combat variant) |

Combat mode affects sprite selection (which pose set) but not movement mechanics.

---

## Other Entity Movement

### Remote Players (`handle_player_position_update`)

**File**: `gameplay/game_state.cpp:2901`

- Position is **teleported** (no interpolation) to the tile from the broadcast
- `movement.running` and `movement.target_x/y` are set from broadcast data
- The `moving` flag on `transform_component` is not set (no client interpolation)

### NPCs (`handle_npc_move`)

**File**: `gameplay/game_state.cpp:3094`

- Position is teleported. NPCs do not have `movement_component` (not added during creation)
- If the NPC entity is unknown, an `entity_info_request` is sent to the server

### Monsters

- Created with `movement_component` by default
- Receive position updates like remote players
- Can interpolate movement when `transform.moving` is set by the legacy protocol

### Legacy Binary Protocol (Remote Entities)

**File**: `network/handlers/motion_handlers.cpp`

The legacy handler sets up proper interpolation for remote entities:

```cpp
// process_move (line 165):
t.dest_tile_x = *x; t.dest_tile_y = *y;
t.direction = static_cast<direction>(*dir);
t.moving = true; t.move_progress = 0.0f;
ent->set_action(object_action::move_peace);

// process_run (line 197):
// Same but with object_action::run
```

These entities then interpolate through `entity_manager::update_movement()` the same way the local player does.

---

## Animation Integration

**File**: `entity/entity_manager.cpp:484` (`update_animation`)

### Action-to-Animation Mapping

`entity::set_action()` maps `object_action` to `entity_anim_state`:

| `object_action` | `entity_anim_state` | Looping | Frame Count | Frame Duration |
|-----------------|---------------------|---------|-------------|----------------|
| `stop_peace/combat` | `stop` | Yes | 8 | 115ms |
| `move_peace/combat` | `move` | Yes | 8 | 70ms |
| `run` | `run` | Yes | 8 | 42ms |
| `attack_*` | `attack` | No | 8 | 78ms |
| `magic` | `magic` | No | 16 | 88ms |
| `get_item` | `get_item` | No | 4 | 150ms |
| `damage` | `damage` | No | 8 | 70ms |
| `dying` | `dying` | No | 13 | 80ms |

### Non-Looping Animation Completion

When a non-looping animation finishes:
- `attack`, `damage`, `magic`, `get_item` -> transition to `stop` (idle)
- `dying` -> transition to `dead`

These transitions happen in `update_animation()` and release the action lock, allowing `can_perform_action()` to return true again.

### Animation Continuity

`animation_component::set_state()` only resets frame/timer if the state actually changes:

```cpp
void set_state(entity_anim_state new_state) {
    if (state == new_state && !finished) return;  // No reset if same state
    // ... reset frame, timer, etc.
}
```

This means consecutive movement steps of the same type (walk-walk or run-run) continue the animation seamlessly without resetting to frame 0.

---

## Sound Integration

**File**: `entity/entity_manager.cpp:515` (footstep trigger), `936` (play function)

### Footstep Sounds

Played on animation frames 1 and 3 during walk/run animations:

```cpp
if (anim.state == entity_anim_state::move) {
    if (anim.current_frame == 1 || anim.current_frame == 3) {
        play_footstep_sound(e, false);  // walk_step
    }
}
else if (anim.state == entity_anim_state::run) {
    if (anim.current_frame == 1 || anim.current_frame == 3) {
        play_footstep_sound(e, true);   // run_step
    }
}
```

Only plays for `entity_type::player` and `entity_type::character`. Uses positional audio via `sounds_.play_character_sound_at(sound, x, y)`.

### Collision Sound

When movement is rejected with `blocked_occupied`, a hurt sound plays:

```cpp
// Male: sound C12, Female: sound C13
uint8_t gender = player->sprite().gender;
int sound_num = (gender == 2) ? 13 : 12;
sounds_.play_sound('C', sound_num, 0);
```

---

## Camera Following

**File**: `gameplay/game_state.cpp:693`, `world/world.cpp:146`

Every frame during `update_playing()`:

```cpp
world_.set_player_position(transform.x, transform.y);
sounds_.set_listener_position(transform.x, transform.y);
```

`set_player_position()` stores the target and (if not in cinematic mode) immediately centers the camera:

```cpp
void world::center_on_player() {
    double target_x = player_world_x_ - screen_width_ / 2.0;
    double target_y = player_world_y_ - screen_height_ / 2.0;
    // Clamp to map bounds
    target_x = clamp(target_x, 0.0, map_width * 32 - screen_width_);
    target_y = clamp(target_y, 0.0, map_height * 32 - screen_height_);
    camera_x_ = target_x;
    camera_y_ = target_y;
}
```

Because `transform.x` / `transform.y` are interpolated every frame, the camera follows smoothly during tile transitions.

---

## Rendering

**File**: `entity/entity_manager.cpp:629`

### Depth Sorting

Visible entities are sorted by Y position before rendering:

```cpp
std::sort(visible_entities.begin(), visible_entities.end(),
    [](const entity* a, const entity* b) {
        return a->transform().y < b->transform().y;
    });
```

Entities lower on screen (higher Y) render on top of entities above them.

### Screen Position

```cpp
int32_t screen_x = transform.x - camera_x;
int32_t screen_y = transform.y - camera_y;
```

### Sprite Selection

For player characters (`render_player_character`):

```cpp
int32_t dir = direction_to_sprite_index(t.direction);       // 1-8
int32_t action = static_cast<int32_t>(e.current_action());  // 0-11
int32_t frame_index = (dir - 1) * 8 + anim.current_frame;

// Body sprite: 500 + (owner_type-1) * 120 + action * 8 + (dir-1)
// Underwear:   base + color * 15 + action
// Hair:        base + style * 15 + action
```

For NPCs/monsters (`render_npc_or_monster`):

```cpp
// Sprite ID: 20000 + (type - 10) * 56 + action * 8 + (dir - 1)
```

### Visibility Culling

Entities outside the screen (with a 128-pixel margin) are not rendered. In global render mode (cinematic), all entities render regardless of distance.

---

## Debug Visualization

### Walkability Overlay (F6)

**File**: `gameplay/game_state.cpp:1536` (`update_pathfinding_trace`)

When enabled, renders a color-coded tile overlay:
- **Red**: Non-walkable tiles
- **Blue**: Teleport tiles
- **Cyan**: Pathfinding trace from player to destination

### Pathfinding Trace

Runs the same `get_next_walkable_dir()` algorithm from the player's current position (or `dest_tile` if moving) to `move_dest_x_/y_` for up to 200 steps. Detects cycles and stops if a step would backtrack.

```cpp
static constexpr int32_t max_trace_steps = 200;
```

The trace is passed to `map_renderer::set_pathfinding_trace()` and drawn as colored tile overlays.

### Tile Grid (F7)

Renders tile boundaries as a grid overlay.

---

## Network Protocol

### WebSocket JSON Messages

#### Move Request

**Builder**: `make_player_move_request()` (`network/messages.hpp:475`)

```json
{
    "type": "player_move_request",
    "data": {
        "x": 100,           // Current tile X
        "y": 200,           // Current tile Y
        "direction": 3,     // Protocol direction (0-7)
        "is_running": true,
        "dest_x": 110,      // Final destination tile X (optional)
        "dest_y": 205,      // Final destination tile Y (optional)
        "timestamp": 1700000000000
    }
}
```

#### Stop Request

**Builder**: `make_player_stop_request()` (`network/messages.hpp:494`)

```json
{
    "type": "player_stop_request",
    "data": {
        "x": 100,
        "y": 200,
        "direction": 5,
        "timestamp": 1700000000000
    }
}
```

#### Move Response (Server -> Client)

```json
{
    "type": "player_move_response",
    "data": {
        "success": false,
        "x": 100,
        "y": 200,
        "direction": 3,
        "error": "blocked_occupied"
    }
}
```

#### Position Update Broadcast (Server -> Client)

```json
{
    "type": "player_position_update",
    "data": {
        "entity_id": 42,
        "x": 101,
        "y": 200,
        "direction": 3,
        "is_running": false,
        "dest_x": 110,
        "dest_y": 205
    }
}
```

### Legacy Binary Protocol

| Message | Handler |
|---------|---------|
| `msg_command_motion` | `motion_handler::handle_motion_response` |
| Motion events | `motion_handler::handle_motion_event` -> dispatches to `process_stop`, `process_move`, `process_run`, etc. |
| Common events | `motion_handler::handle_common_event` -> `process_spawn_object`, `process_despawn_object` |

---

## Key Files Reference

| File | Purpose |
|------|---------|
| `entity/components.hpp` | `transform_component`, `movement_component`, `animation_component`, `entity_anim_state` |
| `entity/entity.hpp` | Entity class: `set_action()`, `set_move_target()`, `current_action()` |
| `entity/entity.cpp` | Action-to-animation mapping, combat mode variant selection |
| `entity/entity_manager.cpp` | `update_movement()` (interpolation), `update_animation()` (frame timing + footsteps) |
| `gameplay/game_state.hpp` | Movement state variables, action queue types, cooldown constants |
| `gameplay/game_state.cpp` | `handle_movement_input()`, `handle_player_move_response()`, `handle_player_stop_response()`, pathfinding trace |
| `gameplay/pathfinding.hpp` | `get_next_move_dir()`, `get_next_walkable_dir()` |
| `core/game_enums.hpp` | `direction`, `object_action`, `entity_anim_state` enums |
| `core/direction_utils.hpp` | `calculate_direction()`, `direction_offset()`, `rotate_cw/ccw()`, protocol conversion |
| `world/tile.hpp` | `tile_width/height`, `tile_flag`, `tile` struct |
| `world/map.hpp` | `map::is_walkable()`, `find_nearest_walkable()`, `has_line_of_sight()` |
| `world/map.cpp` | Walkability logic, Bresenham line-of-sight, spiral search |
| `world/world.hpp` | Camera, `screen_to_tile()`, cinematic mode, zoom |
| `world/world.cpp` | Coordinate conversion, camera following, camera shake |
| `network/messages.hpp` | `make_player_move_request()`, `make_player_stop_request()` |
| `network/handlers/motion_handlers.cpp` | Legacy binary protocol: move/run/stop/attack/damage for remote entities |
| `audio/sound_types.hpp` | `walk_step`, `run_step` sound enums |
