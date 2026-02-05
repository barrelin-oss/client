# Camera System

Exhaustive reference for the camera, viewport, zoom, coordinate conversion, and rendering pipeline in the modern client (`src/`).

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Coordinate Systems](#coordinate-systems)
3. [Constants](#constants)
4. [Camera State](#camera-state)
5. [Camera Following (Normal Mode)](#camera-following-normal-mode)
6. [Map Bounds Clamping](#map-bounds-clamping)
7. [Camera Shake](#camera-shake)
8. [Cinematic Mode](#cinematic-mode)
9. [Drag-to-Pan](#drag-to-pan)
10. [Zoom System](#zoom-system)
11. [SFML View Management](#sfml-view-management)
12. [Visible Tile Range Calculation](#visible-tile-range-calculation)
13. [Chunk-Based Rendering](#chunk-based-rendering)
14. [Coordinate Conversion Functions](#coordinate-conversion-functions)
15. [Rendering Pipeline](#rendering-pipeline)
16. [Entity Visibility Culling](#entity-visibility-culling)
17. [Entity Screen Positioning](#entity-screen-positioning)
18. [Entity Click Detection](#entity-click-detection)
19. [Screen Resize & Resolution Change](#screen-resize--resolution-change)
20. [View Range Notification](#view-range-notification)
21. [Debug Stats Integration](#debug-stats-integration)
22. [Input Bindings](#input-bindings)
23. [Global Render Mode](#global-render-mode)
24. [Known Issues](#known-issues)
25. [Key Files Reference](#key-files-reference)
26. [Data Flow Diagrams](#data-flow-diagrams)

---

## Architecture Overview

The camera system is split across three layers:

| Layer | Class | Responsibility |
|-------|-------|----------------|
| **World** | `hb::world` | Camera state, position tracking, zoom logic, cinematic mode, shake, coordinate conversion |
| **Map Renderer** | `hb::map_renderer` | Visible tile range calculation, tile-to-screen mapping, chunk rendering, zoom-aware culling |
| **Renderer** | `hb::renderer` | SFML `sf::View` manipulation for GPU-side zoom scaling |

The `game_state_manager` orchestrates these by:
1. Feeding player position to `world` each frame
2. Processing input (zoom wheel, cinematic toggles, drag-to-pan)
3. Calling the rendering pipeline in the correct order (zoom view on, world render, entity render, zoom view off, UI render)

---

## Coordinate Systems

The codebase uses three coordinate spaces. All positions are integers except where noted.

### Tile Coordinates

Grid position on the map. Origin `(0, 0)` is the top-left tile.

- Range: `[0, map_width)` x `[0, map_height)`
- Maximum map size: 752 x 752 tiles
- Stored in `transform_component::tile_x`, `tile_y`

### World Coordinates (Pixels)

Absolute pixel position within the map. Origin `(0, 0)` is the top-left pixel of tile `(0, 0)`.

```
world_x = tile_x * 32
world_y = tile_y * 32
```

Entity positions (`transform_component::x`, `y`) are in world pixels, typically at tile center:

```
entity.x = tile_x * 32 + 16    // Tile center X
entity.y = tile_y * 32 + 16    // Tile center Y (feet position)
```

- Range: `[0, map_width * 32)` x `[0, map_height * 32)`
- Camera position (`camera_x_`, `camera_y_`) is in world pixels, representing the top-left corner of the viewport

### Screen Coordinates (Pixels)

Pixel position relative to the application window. Origin `(0, 0)` is the top-left of the window.

```
screen_x = world_x - camera_x
screen_y = world_y - camera_y
```

- Range: `[0, screen_width)` x `[0, screen_height)` for visible content
- Default screen size: 640 x 480 (configurable via settings)

### Conversion Summary

```
Tile  --> World:   world  = tile * 32
World --> Tile:    tile   = world / 32   (integer division)
World --> Screen:  screen = world - camera
Screen --> World:  world  = screen + camera
Screen --> Tile:   tile   = (screen + camera) / 32
```

---

## Constants

### Tile Dimensions (`src/world/tile.hpp:8-9`)

```cpp
inline constexpr int32_t tile_width = 32;
inline constexpr int32_t tile_height = 32;
```

### Default Screen Dimensions (`src/core/constants.hpp:9-10`)

```cpp
inline constexpr uint32_t screen_width = 640;
inline constexpr uint32_t screen_height = 480;
```

These are defaults; actual dimensions come from `config::instance().video()` and can be changed at runtime.

### Map Limits (`src/world/map.hpp:13-18`)

```cpp
inline constexpr int32_t max_map_width = 752;
inline constexpr int32_t max_map_height = 752;
inline constexpr int32_t visible_tiles_x = 21;  // ~640/32 + margin
inline constexpr int32_t visible_tiles_y = 16;  // ~480/32 + margin
```

Note: `visible_tiles_x` and `visible_tiles_y` are legacy constants. The actual visible tile count is computed dynamically by `map_renderer::calculate_visible_range()` using real screen dimensions and zoom level.

---

## Camera State

All camera state lives in `hb::world` (`src/world/world.hpp:136-190`).

### Core Position (`world.hpp:144-148`)

```cpp
double camera_x_ = 0.0;           // Current camera X in world pixels (double for zoom precision)
double camera_y_ = 0.0;           // Current camera Y in world pixels (double for zoom precision)
int32_t player_world_x_ = 0;      // Last known player position (world pixels)
int32_t player_world_y_ = 0;
bool cinematic_mode_ = false;      // If true, camera does NOT follow player
```

Camera position is stored as `double` to avoid jitter during zoom interpolation. It is cast to `int32_t` when consumed by rendering code.

### Effective Camera Position (`world.hpp:99-100`)

```cpp
int32_t camera_x() const { return static_cast<int32_t>(camera_x_) + shake_offset_x_; }
int32_t camera_y() const { return static_cast<int32_t>(camera_y_) + shake_offset_y_; }
```

All consumers (rendering, coordinate conversion, debug display) use these accessors, which add shake offset to the truncated position.

### Screen Dimensions (`world.hpp:152-153`)

```cpp
uint32_t screen_width_ = 640;
uint32_t screen_height_ = 480;
```

Updated via `set_screen_size()` when the window resolution changes. These drive camera centering and visible range calculations.

---

## Camera Following (Normal Mode)

In normal (non-cinematic) mode, the camera follows the local player.

### Update Flow

Each frame in `game_state_manager::update_playing()` (`game_state.cpp:687-692`):

```cpp
if (entity* player = local_player()) {
    const auto& transform = player->transform();
    world_.set_player_position(transform.x, transform.y);
}
```

### `set_player_position()` (`world.cpp:146-156`)

```cpp
void world::set_player_position(int32_t world_x, int32_t world_y)
{
    player_world_x_ = world_x;
    player_world_y_ = world_y;
    if (!cinematic_mode_)
    {
        center_on_player();
    }
}
```

Stores the player position and immediately re-centers the camera (unless in cinematic mode).

### `center_on_player()` (`world.cpp:158-176`)

```cpp
void world::center_on_player()
{
    double target_x = static_cast<double>(player_world_x_) - static_cast<double>(screen_width_) / 2.0;
    double target_y = static_cast<double>(player_world_y_) - static_cast<double>(screen_height_) / 2.0;

    // Clamp to map bounds
    if (current_map_.is_loaded())
    {
        double max_x = static_cast<double>(current_map_.width() * tile_width)
                       - static_cast<double>(screen_width_);
        double max_y = static_cast<double>(current_map_.height() * tile_height)
                       - static_cast<double>(screen_height_);
        target_x = std::clamp(target_x, 0.0, std::max(0.0, max_x));
        target_y = std::clamp(target_y, 0.0, std::max(0.0, max_y));
    }

    camera_x_ = target_x;
    camera_y_ = target_y;
    has_zoom_anchor_ = false;
}
```

The camera snaps instantly (no lerp/smoothing). The player is centered in the viewport, clamped so the camera never goes past map edges.

---

## Map Bounds Clamping

The camera is clamped to prevent showing areas outside the map:

```
min_x = 0
min_y = 0
max_x = map_width_pixels - screen_width
max_y = map_height_pixels - screen_height
```

Where `map_width_pixels = map.width() * 32`.

If the map is smaller than the screen, `max_x` or `max_y` would be negative; `std::max(0.0, ...)` prevents that.

**Note:** Clamping is only applied in `center_on_player()`. Direct camera positioning in cinematic mode (`set_camera_position()`, `move_camera()`, drag-to-pan) does NOT clamp to map bounds.

---

## Camera Shake

### State (`world.hpp:156-160`)

```cpp
int32_t shake_offset_x_ = 0;      // Current frame's random X offset
int32_t shake_offset_y_ = 0;      // Current frame's random Y offset
float shake_intensity_ = 0.0f;    // Initial intensity in pixels
float shake_duration_ = 0.0f;     // Total duration in seconds
float shake_timer_ = 0.0f;        // Elapsed time since shake started
```

### Trigger (`world.cpp:202-207`)

```cpp
void world::add_camera_shake(float intensity, float duration)
{
    shake_intensity_ = intensity;
    shake_duration_ = duration;
    shake_timer_ = 0.0f;
}
```

Calling again replaces the current shake (no accumulation).

### Per-Frame Update (`world.cpp:317-341`)

Called from `world::update_camera(delta_time)` every frame:

1. Increment `shake_timer_` by `delta_time`
2. If timer exceeds duration: clear offsets and reset state
3. Otherwise: compute linear decay and apply random offsets

```cpp
float progress = shake_timer_ / shake_duration_;
float current_intensity = shake_intensity_ * (1.0f - progress);

shake_offset_x_ = random_in_range(-current_intensity, +current_intensity);
shake_offset_y_ = random_in_range(-current_intensity, +current_intensity);
```

The random values use `rand() / RAND_MAX` (stdlib rand, not deterministic).

### How It Affects Rendering

Shake offsets are baked into the return value of `camera_x()` / `camera_y()`. This means:

- Map tiles shift by the shake offset
- Entities shift by the shake offset (they use `camera_x()` for screen position)
- UI elements are NOT affected (they use absolute screen coordinates)

---

## Cinematic Mode

Decouples the camera from the player for free-camera exploration.

### Toggle (`game_state.cpp:1541-1552`)

**Hotkey: F5**

```cpp
if (inp.is_key_pressed(sf::Keyboard::Key::F5)) {
    bool cinematic = !world_.is_cinematic_mode();
    world_.set_cinematic_mode(cinematic);
    // Auto-disable global render mode when exiting
    if (!cinematic && world_.is_global_render_mode()) {
        world_.set_global_render_mode(false);
        entities_.set_global_render_mode(false);
    }
}
```

### Enable/Disable (`world.cpp:209-218`)

```cpp
void world::set_cinematic_mode(bool enabled)
{
    cinematic_mode_ = enabled;
    if (!enabled)
    {
        center_on_player();  // Snap back to player
    }
}
```

### Direct Camera Control (Cinematic Only)

**Arrow Keys** (`game_state.cpp:1591-1610`):

```cpp
int32_t pan_amount = 32;  // 1 tile
if (shift_held) pan_amount = 32 * 5;  // 5 tiles

if (key_left)  world_.move_camera(-pan_amount, 0);
if (key_right) world_.move_camera(pan_amount, 0);
if (key_up)    world_.move_camera(0, -pan_amount);
if (key_down)  world_.move_camera(0, pan_amount);
```

**`move_camera()`** (`world.cpp:231-240`): Adds delta to `camera_x_`/`camera_y_` and clears zoom anchor.

**`set_camera_position()`** (`world.cpp:220-229`): Sets absolute camera position. Also clears zoom anchor.

### State Reset on Game Exit (`game_state.cpp:571-574`)

When clearing game data (map change, disconnect), camera state is fully reset:

```cpp
world_.set_cinematic_mode(false);
world_.set_global_render_mode(false);
world_.set_zoom_mode_enabled(false);
```

---

## Drag-to-Pan

Available only in cinematic mode when `camera_drag_locked_` is false.

### Controls

**Ctrl + Left Mouse Drag** (`game_state.cpp:1033-1054`)

### State (`world.hpp:186-190`)

```cpp
bool drag_active_ = false;
int32_t drag_start_mouse_x_ = 0;
int32_t drag_start_mouse_y_ = 0;
int32_t drag_start_camera_x_ = 0;
int32_t drag_start_camera_y_ = 0;
```

### Mechanics

1. **Start** (`world.cpp:242-251`): Records mouse position and current camera position
2. **Update** (`world.cpp:253-271`): Computes delta from drag start (inverted: drag right moves camera left). Delta is multiplied by `zoom_level_` so panning feels consistent at any zoom level.

```cpp
double dx = static_cast<double>(drag_start_mouse_x_ - mouse_x);
double dy = static_cast<double>(drag_start_mouse_y_ - mouse_y);

if (zoom_mode_enabled_ && zoom_level_ != 1.0)
{
    dx *= zoom_level_;
    dy *= zoom_level_;
}

camera_x_ = drag_start_camera_x_ + dx;
camera_y_ = drag_start_camera_y_ + dy;
```

3. **End** (`world.cpp:273-276`): Sets `drag_active_ = false`

### Drag Lock

`game_state_manager::camera_drag_locked_` (`game_state.hpp:309`) can be set via `set_camera_drag_locked(true)` to disable drag during scripted cinematic sequences. When locked, Ctrl+click drag is ignored.

---

## Zoom System

### Overview

Zoom is opt-in via `zoom_mode_enabled_`. It works by:
1. Setting an `sf::View` with a scale factor (GPU-side scaling)
2. Adjusting the visible tile range calculation to render more/fewer tiles
3. Adjusting camera position to maintain a zoom anchor point

### State (`world.hpp:170-180`)

```cpp
double zoom_level_ = 1.0;           // Current zoom (interpolates toward target)
double zoom_target_ = 1.0;          // Desired zoom level
bool zoom_mode_enabled_ = false;    // Must be true for zoom to work

// Anchor - keeps a world point fixed on screen during zoom
double zoom_anchor_world_x_ = 0.0;
double zoom_anchor_world_y_ = 0.0;
double zoom_anchor_screen_x_ = 0.0;
double zoom_anchor_screen_y_ = 0.0;
bool has_zoom_anchor_ = false;
```

### Zoom Level Semantics

| `zoom_level_` | Effect |
|-------------:|--------|
| 0.5 | 2x zoom in (everything appears 2x larger) |
| 1.0 | Normal view (no zoom) |
| 2.0 | 2x zoom out (everything appears half size) |
| 8.0 | 8x zoom out (maximum) |

Range: `[0.5, 8.0]`, enforced by `std::clamp` in `adjust_zoom()`.

### Enable/Disable (`world.cpp:388-398`)

**Hotkey: Ctrl+Q** (`game_state.cpp:1571-1576`)

```cpp
void world::set_zoom_mode_enabled(bool enabled)
{
    zoom_mode_enabled_ = enabled;
    if (!enabled)
    {
        zoom_level_ = 1.0;
        zoom_target_ = 1.0;
        has_zoom_anchor_ = false;
    }
}
```

Disabling zoom immediately resets to 1.0 (no smooth transition back).

### Input: Mouse Wheel (`game_state.cpp:664-676`)

```cpp
if (world_.is_zoom_mode_enabled()) {
    int32_t wheel = inp.wheel_delta();
    if (wheel != 0) {
        if (world_.is_cinematic_mode()) {
            // Zoom-to-cursor: pass mouse position for anchor
            world_.adjust_zoom(-wheel * 0.1f, inp.mouse_x(), inp.mouse_y());
        } else {
            // Zoom without cursor anchor
            world_.adjust_zoom(-wheel * 0.1f);
        }
    }
}
```

Scroll up = zoom in (negative delta to decrease `zoom_level_`). Step size: 0.1 per scroll tick.

Cursor-based anchoring is only used in cinematic mode.

### `adjust_zoom()` (`world.cpp:400-437`)

1. Clamps new target to `[0.5, 8.0]`
2. If cursor position provided (`cursor_x >= 0 && cursor_y >= 0`):
   - Computes the world point under the cursor using current zoom
   - Snaps to nearest tile center for stability
   - Records this as the zoom anchor (world position + screen position)
3. Sets `zoom_target_`

**World-point-under-cursor formula:**

```cpp
double cursor_from_center_x = cursor_x - screen_center_x;
double cursor_from_center_y = cursor_y - screen_center_y;

double world_x = camera_x_ + screen_center_x + cursor_from_center_x * zoom_level_;
double world_y = camera_y_ + screen_center_y + cursor_from_center_y * zoom_level_;
```

**Tile center snapping:**

```cpp
int32_t tile_x = static_cast<int32_t>(world_x) / tile_width;
int32_t tile_y = static_cast<int32_t>(world_y) / tile_height;
zoom_anchor_world_x_ = tile_x * tile_width + tile_width / 2;
zoom_anchor_world_y_ = tile_y * tile_height + tile_height / 2;
```

### Smooth Interpolation (`world.cpp:35-57`)

Called each frame from `world::update()`:

```cpp
constexpr double zoom_speed = 10.0;
double t = 1.0 - std::exp(-zoom_speed * delta_time);

zoom_level_ += (zoom_target_ - zoom_level_) * t;
```

This is exponential decay toward the target. At 60 FPS, `t ~ 0.154`, meaning ~15% of the remaining distance is covered each frame.

**Camera repositioning to maintain anchor:**

```cpp
camera_x_ = zoom_anchor_world_x_ - screen_center_x
             - (zoom_anchor_screen_x_ - screen_center_x) * zoom_level_;
camera_y_ = zoom_anchor_world_y_ - screen_center_y
             - (zoom_anchor_screen_y_ - screen_center_y) * zoom_level_;
```

This ensures the anchor world point remains at the same screen pixel throughout the zoom animation.

**Anchor clearing:**

```cpp
if (std::abs(zoom_level_ - zoom_target_) < 0.001)
{
    has_zoom_anchor_ = false;
}
```

---

## SFML View Management

### Renderer Methods (`src/graphics/renderer.cpp:199-208`)

```cpp
void renderer::set_zoom_view(float zoom_level, float center_x, float center_y)
{
    sf::View view = window_.getDefaultView();
    view.setCenter({center_x, center_y});
    view.zoom(zoom_level);
    window_.setView(view);
}

void renderer::reset_to_default_view()
{
    window_.setView(window_.getDefaultView());
}
```

`sf::View::zoom()` scales the view. A `zoom_level` of 2.0 makes the view cover 2x the pixels in each direction (everything appears half-size).

### World View Application (`world.cpp:74-92`)

```cpp
void world::apply_zoom_view(renderer& rend)
{
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        float screen_center_x = static_cast<float>(screen_width_) / 2.0f;
        float screen_center_y = static_cast<float>(screen_height_) / 2.0f;
        rend.set_zoom_view(static_cast<float>(zoom_level_), screen_center_x, screen_center_y);
    }
}

void world::reset_zoom_view(renderer& rend)
{
    if (zoom_mode_enabled_ && zoom_level_ != 1.0)
    {
        rend.reset_to_default_view();
    }
}
```

The view is always centered at the screen center. Camera position handles panning; the SFML view only handles scaling.

---

## Visible Tile Range Calculation

### `calculate_visible_range()` (`map_renderer.cpp:316-353`)

Determines which tiles to render based on camera position, screen size, and zoom level.

```cpp
visible_range map_renderer::calculate_visible_range(const map& m,
    int32_t camera_x, int32_t camera_y) const
```

**Step 1: Effective screen size**

```cpp
float effective_width = screen_width_ * zoom_level_;
float effective_height = screen_height_ * zoom_level_;
```

When zoomed out (zoom > 1), we see more of the world, so the effective size increases.

**Step 2: Tile count**

```cpp
int32_t tiles_x = static_cast<int32_t>(effective_width / tile_width) + 2;
int32_t tiles_y = static_cast<int32_t>(effective_height / tile_height) + 2;
```

**Step 3: Zoom padding**

```cpp
int32_t zoom_padding = static_cast<int32_t>(std::ceil(zoom_level_)) + 2;
```

Extra tiles rendered around the edges to prevent pop-in during zoom transitions.

**Step 4: Adjusted camera for centered view**

Since the SFML view is centered at the screen center, zooming extends the visible area symmetrically. The start position must be offset:

```cpp
float half_extra_width = (effective_width - screen_width_) / 2.0f;
float half_extra_height = (effective_height - screen_height_) / 2.0f;

int32_t adjusted_camera_x = camera_x - static_cast<int32_t>(half_extra_width);
int32_t adjusted_camera_y = camera_y - static_cast<int32_t>(half_extra_height);
```

**Step 5: Final range with clamping**

```cpp
int32_t base_start_x = adjusted_camera_x / tile_width;
int32_t base_start_y = adjusted_camera_y / tile_height;

range.start_x = std::max(0, base_start_x - zoom_padding);
range.start_y = std::max(0, base_start_y - zoom_padding);
range.end_x = std::min(m.width(), base_start_x + tiles_x + zoom_padding * 2);
range.end_y = std::min(m.height(), base_start_y + tiles_y + zoom_padding * 2);
```

### Return Type (`map_renderer.hpp:69-73`)

```cpp
struct visible_range
{
    int32_t start_x, start_y;
    int32_t end_x, end_y;
};
```

Start is inclusive, end is exclusive.

### Zoom Level Synchronization (`world.cpp:62-63`)

Before rendering, `world::render()` pushes the current zoom level to the map renderer:

```cpp
map_renderer_.set_zoom_level(zoom_mode_enabled_ ? static_cast<float>(zoom_level_) : 1.0f);
```

---

## Chunk-Based Rendering

An optimization for zoomed-out views. When zoomed out beyond a threshold, pre-rendered tile chunks are used instead of drawing individual tiles.

### Constants (`map_renderer.hpp:77-78`)

```cpp
static constexpr int32_t chunk_size_ = 16;              // 16x16 tiles per chunk
static constexpr float chunk_zoom_threshold_ = 2.0f;    // Threshold to switch to chunk mode
```

### When Chunks Are Used (`map_renderer.cpp:31-34`)

```cpp
bool map_renderer::should_use_chunks() const
{
    return zoom_level_ >= chunk_zoom_threshold_;
}
```

### Chunk Storage (`map_renderer.hpp:80-97`)

Each chunk is keyed by `(chunk_x, chunk_y)` and holds an `sf::RenderTexture`:

```cpp
struct chunk_data {
    std::unique_ptr<sf::RenderTexture> texture;
    bool valid = false;
};

std::unordered_map<chunk_key, chunk_data, chunk_key_hash> chunks_;
```

Chunk textures are `chunk_size_ * tile_width` = 512 x 512 pixels.

### Chunk Rendering (`map_renderer.cpp:59-108`)

Chunks render only the terrain layer. Objects are rendered separately on top for correct depth layering.

### Chunk Invalidation (`map_renderer.cpp:23-29`)

```cpp
void map_renderer::invalidate_chunks()
{
    for (auto& [key, chunk] : chunks_)
    {
        chunk.valid = false;
    }
}
```

Call when map data changes to force re-rendering.

### Render Path (`map_renderer.cpp:126-178`)

```
if (should_use_chunks()):
    1. Calculate visible chunk range from visible tile range
    2. For each visible chunk:
       a. Render to texture if not valid
       b. Draw chunk texture at (chunk_world_x - camera_x, chunk_world_y - camera_y)
    3. Render object layer separately (tile-by-tile)
else:
    1. Render terrain layer tile-by-tile
    2. Render object layer tile-by-tile
```

---

## Coordinate Conversion Functions

### World-Level (`src/world/world.cpp:302-315`)

```cpp
std::pair<int32_t, int32_t> world::screen_to_tile(int32_t screen_x, int32_t screen_y) const
{
    return map_renderer_.screen_to_tile(screen_x, screen_y, camera_x_, camera_y_);
}

std::pair<int32_t, int32_t> world::screen_to_world(int32_t screen_x, int32_t screen_y) const
{
    return {screen_x + static_cast<int32_t>(camera_x_),
            screen_y + static_cast<int32_t>(camera_y_)};
}

std::pair<int32_t, int32_t> world::world_to_screen(int32_t world_x, int32_t world_y) const
{
    return {world_x - static_cast<int32_t>(camera_x_),
            world_y - static_cast<int32_t>(camera_y_)};
}
```

Note: `screen_to_tile` and `screen_to_world` use the raw double `camera_x_`, NOT `camera_x()` (no shake offset). This is intentional for click targeting.

### Map Renderer Level (`map_renderer.cpp:361-373`)

```cpp
std::pair<int32_t, int32_t> map_renderer::screen_to_tile(
    int32_t screen_x, int32_t screen_y,
    int32_t camera_x, int32_t camera_y) const
{
    int32_t world_x = screen_x + camera_x;
    int32_t world_y = screen_y + camera_y;
    return {world_x / tile_width, world_y / tile_height};
}

std::pair<int32_t, int32_t> map_renderer::tile_to_screen(
    int32_t tile_x, int32_t tile_y,
    int32_t camera_x, int32_t camera_y) const
{
    return {tile_x * tile_width - camera_x, tile_y * tile_height - camera_y};
}
```

### Map Static Helpers (`map.hpp:53-56`)

```cpp
static int32_t world_to_tile_x(int32_t world_x) { return world_x / tile_width; }
static int32_t world_to_tile_y(int32_t world_y) { return world_y / tile_height; }
static int32_t tile_to_world_x(int32_t tile_x) { return tile_x * tile_width; }
static int32_t tile_to_world_y(int32_t tile_y) { return tile_y * tile_height; }
```

---

## Rendering Pipeline

The rendering order in `game_state_manager::render_playing()` (`game_state.cpp:832-850+`):

```
1.  world_.apply_zoom_view(rend)         -- Set SFML view with zoom scale
2.  world_.render(rend)                  -- Render map tiles (terrain + objects)
3.  entities_.render(rend, sprites_,     -- Render entities (sorted by Y for depth)
        world_.camera_x(),              --   Uses effective camera pos (with shake)
        world_.camera_y(),
        mouse_x_, mouse_y_)
4.  world_.reset_zoom_view(rend)         -- Restore default SFML view
5.  debug::debug_stats::render(rend)     -- Debug overlay (screen-space, unzoomed)
6.  status_log_.render(rend)             -- Status messages (screen-space, unzoomed)
7.  ui_.render(rend)                     -- All UI dialogs (screen-space, unzoomed)
```

**Key insight:** Steps 2-3 render in zoomed coordinates (world content scales with zoom). Steps 5-7 render in screen coordinates (UI is always at 1:1 pixel scale, unaffected by zoom).

### Map Render Details (`world.cpp:60-72`)

```cpp
void world::render(renderer& rend)
{
    map_renderer_.set_zoom_level(zoom_mode_enabled_ ? static_cast<float>(zoom_level_) : 1.0f);
    map_renderer_.render(rend, current_map_, camera_x_, camera_y_);
}
```

### Shake Discrepancy Between Map and Entities

There is a subtle difference in which camera value each system receives:

- **Map tiles** (`world.cpp:65`): `map_renderer_.render(rend, current_map_, camera_x_, camera_y_)` -- passes raw `camera_x_` (the `double` member, implicitly narrowed to `int32_t`). **No shake offset.**
- **Entities** (`game_state.cpp:840`): `entities_.render(rend, sprites_, world_.camera_x(), world_.camera_y(), ...)` -- passes `camera_x()` which includes `shake_offset_x_`. **Includes shake offset.**

This means during camera shake, map tiles stay fixed while entities jitter. Whether this is intentional (a stylistic screen-shake where only characters tremble) or a bug (shake should affect everything uniformly) depends on desired behavior. See [Known Issues](#known-issues) item 7.

---

## Entity Visibility Culling

### Normal Mode (`entity_manager.cpp:629-658`)

Entities are culled based on their screen position:

```cpp
static constexpr int32_t render_margin = 128;  // Pixels beyond screen edge

int32_t screen_x = transform.x - camera_x;
int32_t screen_y = transform.y - camera_y;

bool visible = (screen_x >= -render_margin && screen_x < scr_width + render_margin &&
                screen_y >= -render_margin && screen_y < scr_height + render_margin);
```

The 128-pixel margin prevents pop-in/pop-out for large sprites that extend beyond their anchor point.

### Global Render Mode

When enabled, ALL entities are rendered regardless of distance (no culling):

```cpp
if (global_render_mode_) {
    visible_entities.push_back(e.get());
    continue;
}
```

### Depth Sorting

Visible entities are sorted by Y position (ascending) so entities lower on screen render on top:

```cpp
std::sort(visible_entities.begin(), visible_entities.end(),
    [](const entity* a, const entity* b) {
        return a->transform().y < b->transform().y;
    });
```

---

## Entity Screen Positioning

### Transform to Screen (`entity_manager.cpp:672-678`)

```cpp
int32_t screen_x = t.x - camera_x;
int32_t screen_y = t.y - camera_y;
```

The `(screen_x, screen_y)` represents the entity's **feet position** on screen. Sprite drawing offsets from this point:

- **Items**: drawn at `(screen_x - 16, screen_y - 16)` (32x32 centered)
- **Players**: drawn at `(screen_x, screen_y)` using sprite metadata offsets (sprites anchor at feet)
- **NPCs/Monsters**: drawn at `(screen_x, screen_y)` using sprite metadata offsets

### Smooth Movement Interpolation (`entity_manager.cpp:617-626`)

During tile-to-tile movement, position is linearly interpolated:

```cpp
int32_t start_x = tile_x * tile_width + 16;     // Source tile center
int32_t start_y = tile_y * tile_height + 16;
int32_t end_x = dest_tile_x * tile_width + 16;  // Dest tile center
int32_t end_y = dest_tile_y * tile_height + 16;

t.x = start_x + (int32_t)((end_x - start_x) * move_progress);
t.y = start_y + (int32_t)((end_y - start_y) * move_progress);
```

This feeds smooth pixel positions to the camera follow system.

---

## Entity Click Detection

### Radius-Based (`entity_manager.cpp:289-316`)

`get_entity_at_screen_pos()` converts screen coordinates to world, then finds the closest entity within a 32-pixel radius:

```cpp
int32_t world_x = screen_x + camera_x;
int32_t world_y = screen_y + camera_y;

// For each entity:
int32_t dx = entity.x - world_x;
int32_t dy = entity.y - world_y;
int32_t dist_sq = dx * dx + dy * dy;

if (dist_sq < 32 * 32) { /* candidate */ }
```

### Sprite-Based (`entity_manager.cpp:318-391`)

`is_point_in_entity_sprite()` does pixel-precise detection using actual sprite bounds:

- **Items**: 32x32 centered at position
- **NPCs/Monsters**: sprite metadata bounds if available, else fallback 64x64
- **Players**: body sprite metadata bounds if available, else fallback 64x64

Both methods use raw `camera_x`/`camera_y` (with shake offset from `camera_x()`) for the conversion.

---

## Screen Resize & Resolution Change

### Flow (`game_state.cpp:3121-3193`)

`game_state_manager::change_resolution()`:

1. Close existing window
2. Create new window at new resolution (`renderer::set_resolution()`)
3. Update config (`video.screen_width`, `video.screen_height`)
4. Update world: `world_.set_screen_size(width, height)` -- this re-centers camera
5. Notify server: `send_view_range()`
6. Reposition UI elements (settings dialog, icon panel)

### World Handler (`world.cpp:178-192`)

```cpp
void world::set_screen_size(uint32_t width, uint32_t height)
{
    screen_width_ = width;
    screen_height_ = height;
    map_renderer_.set_screen_size(width, height);

    if (!cinematic_mode_)
    {
        center_on_player();
    }
}
```

Updates both `world` and `map_renderer` screen dimensions. Re-centers camera on player (the new screen size changes the centering calculation).

### Initial Screen Size (`game_state.cpp:224-225`)

Set during state initialization:

```cpp
const auto& video = config::instance().video();
world_.set_screen_size(video.screen_width, video.screen_height);
```

---

## View Range Notification

When entering the game or changing resolution, the client notifies the server of the viewport dimensions:

```cpp
void game_state_manager::send_view_range()
{
    const auto& video = config::instance().video();
    json msg = make_set_view_range_request(video.screen_width, video.screen_height);
    ws_connection_.send(msg);
}
```

This allows the server to send only entities within the client's visible range.

Called on:
- Game entry (`handle_enter_game_response`, `game_state.cpp:2718`)
- Resolution change (`change_resolution`, `game_state.cpp:3157-3158`)

---

## Debug Stats Integration

The debug overlay (`debug::debug_stats`, toggle with **Alt+\`**) displays camera-related information.

### Updated Per Frame (`game_state.cpp:706-759`)

```cpp
int32_t cam_x = world_.camera_x();
int32_t cam_y = world_.camera_y();

debug_stats.set_camera_bounds(
    cam_x, cam_y,
    cam_x + renderer_->width(),
    cam_y + renderer_->height()
);

debug_stats.set_player_position(tile_x, tile_y, world_x, world_y);

debug_stats.set_mouse_screen_pos(inp.mouse_x(), inp.mouse_y());
debug_stats.set_mouse_world_pos(inp.mouse_x() + cam_x, inp.mouse_y() + cam_y);
auto [tile_x, tile_y] = world_.screen_to_tile(inp.mouse_x(), inp.mouse_y());
debug_stats.set_mouse_tile_pos(tile_x, tile_y);
```

### Displayed Data (`debug_stats.hpp:34-61`)

| Metric | Method |
|--------|--------|
| Camera bounds (world px) | `set_camera_bounds(left, top, right, bottom)` |
| Player tile position | `set_player_position(tile_x, tile_y, world_x, world_y)` |
| Player world position | (same call) |
| Mouse screen position | `set_mouse_screen_pos(x, y)` |
| Mouse world position | `set_mouse_world_pos(x, y)` |
| Mouse tile position | `set_mouse_tile_pos(x, y)` |
| Hovered entity | `set_hovered_entity(info)` |
| Entity count | `set_entity_count(count)` |
| Map name | `set_map_name(name)` |

---

## Input Bindings

All camera-related input bindings:

| Input | Condition | Action |
|-------|-----------|--------|
| **Mouse Wheel** | Zoom mode enabled | Zoom in/out (0.1 per tick) |
| **Mouse Wheel** | Zoom mode + Cinematic | Zoom-to-cursor (anchored) |
| **Ctrl+Q** | Playing state | Toggle zoom mode on/off |
| **F5** | Playing state | Toggle cinematic mode |
| **Ctrl+G** | Cinematic mode | Toggle global render mode |
| **Arrow Keys** | Cinematic mode | Pan camera (1 tile = 32px) |
| **Shift+Arrow Keys** | Cinematic mode | Pan camera (5 tiles = 160px) |
| **Ctrl+Left Click Drag** | Cinematic mode, not drag-locked | Drag-to-pan |
| **Alt+\`** | Playing state | Toggle debug stats overlay |
| **Left Click** | Playing state | Movement target (uses `screen_to_tile`) |

---

## Global Render Mode

**Hotkey: Ctrl+G** (requires cinematic mode)

When enabled:
- Entity visibility culling is bypassed; ALL entities render regardless of screen distance
- Both `world` and `entity_manager` have independent `global_render_mode_` flags that must be set together
- Automatically disabled when exiting cinematic mode

Purpose: allows viewing the entire map's entities when zoomed out in cinematic mode.

---

## Known Issues

1. **`screen_to_tile()` ignores zoom** -- The conversion `tile = (screen + camera) / 32` does not account for SFML view scaling. At non-1.0 zoom, clicking on a tile targets the wrong coordinate. The fix would require mapping screen coordinates through the inverse of the SFML view transform.

2. **`get_entity_at_screen_pos()` ignores zoom** -- Same issue: screen-to-world conversion doesn't account for the zoomed SFML view, so entity selection is inaccurate when zoomed.

3. **`screen_to_world()` ignores zoom** -- All three coordinate conversion functions assume zoom = 1.0.

4. **Anchor clearing can cause camera jump** -- The zoom anchor is cleared when `|zoom_level_ - zoom_target_| < 0.001`. If the player is moving during a zoom animation, clearing the anchor allows `center_on_player()` to snap the camera on the next frame, potentially causing a visible jump.

5. **Cinematic mode has no map bounds clamping** -- `move_camera()`, `set_camera_position()`, and drag-to-pan do not clamp to map bounds. The camera can be panned outside the map.

6. **Shake uses stdlib `rand()`** -- The camera shake uses `rand()` which is not seeded explicitly, making shake patterns non-deterministic across runs. This is cosmetic only.

7. **Map tiles and entities use different camera values during shake** -- `world::render()` passes `camera_x_` (no shake) to the map renderer, while entity rendering uses `camera_x()` (with shake). During shake, entities jitter relative to tiles. This may be intentional (screen-shake effect) or a bug depending on desired behavior.

---

## Key Files Reference

| File | Lines | Contents |
|------|-------|----------|
| `src/world/world.hpp` | 44-193 | Camera state, zoom state, drag state, shake state, all accessors |
| `src/world/world.cpp` | 30-437 | Camera logic: update, center, shake, cinematic, drag, zoom, coordinate conversion |
| `src/world/map_renderer.hpp` | 26-106 | `visible_range` struct, chunk rendering types, `screen_to_tile`/`tile_to_screen` |
| `src/world/map_renderer.cpp` | 110-373 | Render pipeline, visible range calculation, tile-to-screen mapping, chunk rendering |
| `src/graphics/renderer.hpp` | 82-86 | `set_zoom_view()` / `reset_to_default_view()` declarations |
| `src/graphics/renderer.cpp` | 199-208 | SFML `sf::View` manipulation for zoom |
| `src/gameplay/game_state.hpp` | 308-314 | `camera_drag_locked_` state |
| `src/gameplay/game_state.cpp` | 664-676 | Mouse wheel zoom input |
| `src/gameplay/game_state.cpp` | 687-692 | Camera follow player update |
| `src/gameplay/game_state.cpp` | 706-759 | Debug stats camera/mouse reporting |
| `src/gameplay/game_state.cpp` | 832-843 | Render pipeline (zoom view bracket) |
| `src/gameplay/game_state.cpp` | 1033-1054 | Drag-to-pan input handling |
| `src/gameplay/game_state.cpp` | 1541-1610 | Cinematic mode, zoom mode, global render, arrow key pan hotkeys |
| `src/gameplay/game_state.cpp` | 3121-3193 | Resolution change handler |
| `src/entity/entity_manager.cpp` | 289-391 | Entity click detection (screen-to-world conversion) |
| `src/entity/entity_manager.cpp` | 629-670 | Entity visibility culling and depth sorting |
| `src/entity/entity_manager.cpp` | 672-678 | Entity screen position calculation |
| `src/entity/components.hpp` | 13-23 | `transform_component` (world position, tile position, movement) |
| `src/world/tile.hpp` | 8-9 | `tile_width`, `tile_height` constants |
| `src/world/map.hpp` | 13-18, 53-56 | Map limits, static coordinate conversion helpers |
| `src/core/constants.hpp` | 9-10 | Default `screen_width`, `screen_height` |
| `src/debug/debug_stats.hpp` | 14-137 | Debug overlay state and rendering |

---

## Data Flow Diagrams

### Per-Frame Camera Update

```
game_state_manager::update_playing(delta_time)
  |
  +-- handle_playing_input(inp)
  |     +-- [cinematic?] drag-to-pan handling
  |     +-- handle_movement_input  --> world_.screen_to_tile() for click targets
  |     +-- handle_hotkey_input    --> F5 (cinematic), Ctrl+Q (zoom), arrow keys (pan)
  |
  +-- [zoom enabled?] mouse wheel --> world_.adjust_zoom()
  |
  +-- world_.update(delta_time)
  |     +-- update_camera()        --> shake offset update
  |     +-- [zoom + anchor?]       --> smooth zoom interpolation + camera repositioning
  |
  +-- entities_.update(delta_time) --> movement interpolation (updates entity world positions)
  |
  +-- [has local player?] world_.set_player_position(px, py)
  |     +-- [not cinematic?] center_on_player() --> camera_x_/camera_y_ updated
  |
  +-- [debug visible?] update debug stats with camera bounds, mouse positions
```

### Per-Frame Render

```
game_state_manager::render_playing(rend)
  |
  +-- world_.apply_zoom_view(rend)           -- sf::View with zoom scale
  |
  +-- world_.render(rend)                    -- tiles in world coords (zoomed view)
  |     +-- map_renderer_.set_zoom_level()
  |     +-- map_renderer_.render()
  |           +-- calculate_visible_range()  -- zoom-aware tile culling
  |           +-- [chunks or tiles]          -- draw at (tile_world - camera) positions
  |
  +-- entities_.render(rend, ...,            -- entities in world coords (zoomed view)
  |       camera_x(), camera_y())            --   uses effective pos (with shake)
  |     +-- visibility culling
  |     +-- Y-sort for depth
  |     +-- for each: screen_pos = entity.pos - camera
  |
  +-- world_.reset_zoom_view(rend)           -- restore default sf::View
  |
  +-- debug_stats.render(rend)               -- screen-space (unzoomed)
  +-- status_log_.render(rend)               -- screen-space (unzoomed)
  +-- ui_.render(rend)                       -- screen-space (unzoomed)
```
