# Map Rendering

The background tile rendering system. Draws the ground layer of the game world: terrain tiles, object overlays, lighting, and debug visualizations.

---

## Architecture Overview

Map rendering is split across three layers:

```
game_state_manager::render_playing()
    └─ world::render(renderer&)
           ├─ map_renderer::set_zoom_level()
           └─ map_renderer::render(renderer&, map&, camera_x, camera_y)
                  ├─ calculate_visible_range()
                  ├─ Terrain layer (layer 0)
                  ├─ Object layer (layer 1)
                  └─ Debug overlays
```

| Class | Responsibility |
|-------|----------------|
| `world` | Camera, zoom, lighting, map lifecycle |
| `map` | Tile storage, AMD loading, spatial queries |
| `map_renderer` | Visible range, tile drawing, chunk caching |
| `tile_sprite_registry` | Sprite ID to PAK file mapping and lazy loading |
| `sprite_manager` | PAK file management and bitmap caching |
| `sprite` | Texture management, two-tier loading, draw calls |

---

## Tile Data

### `tile` struct (`src/world/tile.hpp:41-56`)

Each tile stores six fields:

```cpp
struct tile
{
    int16_t terrain_id = 0;         // Base terrain sprite ID (0 is valid)
    int16_t terrain_frame = 0;      // Terrain animation frame
    int16_t object_id = 0;          // Object/decoration sprite ID (0 = none)
    int16_t object_frame = 0;       // Object animation frame
    uint16_t roof_id = 0;           // Roof sprite (loaded separately)
    tile_flag flags = tile_flag::walkable;
    uint8_t light_level = 255;      // 0 = dark, 255 = full bright
};
```

Helper methods: `is_walkable()`, `is_water()`, `is_teleport()`, `is_safe_zone()`, `blocks_sight()`, `is_occupied()`.

### `tile_flag` enum (`src/world/tile.hpp:12-26`)

Bitfield flags stored as `uint16_t`:

| Flag | Bit | Description |
|------|-----|-------------|
| `walkable` | 0 | Entity can traverse |
| `water` | 1 | Water terrain |
| `lava` | 2 | Lava terrain |
| `ice` | 3 | Ice terrain |
| `swamp` | 4 | Swamp terrain |
| `teleport` | 5 | Teleport trigger |
| `blocks_sight` | 6 | Vision blocking |
| `blocks_magic` | 7 | Magic blocking |
| `safe_zone` | 8 | PvP disabled |
| `pvp_zone` | 9 | PvP enabled |
| `occupied` | 10 | Entity standing here |
| `item_present` | 11 | Item on ground |

Bitwise operators `|`, `&`, and `has_flag()` are defined for combining and testing flags.

### `tile_animation` struct (`src/world/tile.hpp:59-64`)

```cpp
struct tile_animation
{
    uint16_t base_sprite_id;
    uint8_t frame_count;
    uint8_t frame_delay;   // In ticks
    uint8_t current_frame;
};
```

Defined but not currently driven by the renderer. Animation frames come from `tile.terrain_frame` and `tile.object_frame` as stored in the AMD file.

---

## Tile Grid

### Dimensions (`src/world/tile.hpp:8-9`)

```cpp
inline constexpr int32_t tile_width = 32;   // pixels
inline constexpr int32_t tile_height = 32;  // pixels
```

### Map Constants (`src/world/map.hpp:13-18`)

```cpp
inline constexpr int32_t max_map_width = 752;
inline constexpr int32_t max_map_height = 752;
inline constexpr int32_t visible_tiles_x = 21;  // ~640/32 + margin
inline constexpr int32_t visible_tiles_y = 16;  // ~480/32 + margin
```

### Projection

Orthogonal (not isometric). Each tile maps to a 32x32 pixel square in world space. No rotation or skew is applied.

### Coordinate Systems

Three coordinate spaces:

| Space | Unit | Origin |
|-------|------|--------|
| **Tile** | Grid cells | (0, 0) top-left of map |
| **World** | Pixels | (0, 0) top-left of map |
| **Screen** | Pixels | (0, 0) top-left of window |

### Conversions

**Tile <-> World** (`src/world/map.hpp:53-56`):

```cpp
static int32_t world_to_tile_x(int32_t world_x) { return world_x / tile_width; }
static int32_t world_to_tile_y(int32_t world_y) { return world_y / tile_height; }
static int32_t tile_to_world_x(int32_t tile_x) { return tile_x * tile_width; }
static int32_t tile_to_world_y(int32_t tile_y) { return tile_y * tile_height; }
```

**Screen <-> World** (`src/world/world.cpp:307-315`):

```cpp
// screen_to_world:
world_x = screen_x + camera_x
world_y = screen_y + camera_y

// world_to_screen:
screen_x = world_x - camera_x
screen_y = world_y - camera_y
```

**Screen -> Tile** (`src/world/map_renderer.cpp:361-367`):

```cpp
std::pair<int32_t, int32_t> screen_to_tile(int32_t screen_x, int32_t screen_y,
                                            int32_t camera_x, int32_t camera_y) const
{
    int32_t world_x = screen_x + camera_x;
    int32_t world_y = screen_y + camera_y;
    return {world_x / tile_width, world_y / tile_height};
}
```

**Tile -> Screen** (`src/world/map_renderer.cpp:369-373`):

```cpp
std::pair<int32_t, int32_t> tile_to_screen(int32_t tile_x, int32_t tile_y,
                                            int32_t camera_x, int32_t camera_y) const
{
    return {tile_x * tile_width - camera_x, tile_y * tile_height - camera_y};
}
```

### Storage (`src/world/map.hpp:69, 73-74`)

Tiles are stored in a flat `std::vector<tile>` in row-major order:

```cpp
std::vector<tile> tiles_;

size_t tile_index(int32_t x, int32_t y) const
{
    return static_cast<size_t>(y * width_ + x);
}
```

Out-of-bounds access returns a static `null_tile_` with default values (`src/world/map.cpp:9, 149-153`).

---

## AMD File Loading

See `docs/amd_file_format.md` for the full binary format specification.

### Loading entry points

| Method | Location | Purpose |
|--------|----------|---------|
| `map::load(path)` | `src/world/map.cpp:11-33` | Load from file on disk |
| `map::load_from_memory(data, size)` | `src/world/map.cpp:35-140` | Parse raw bytes |
| `world::load_map(map_name)` | `src/world/world.cpp:94-121` | High-level: converts name to path, loads |

### Parsing steps (`src/world/map.cpp:35-140`)

1. **Header** (256 bytes): NULL bytes replaced with spaces, then `MAPSIZEX` and `MAPSIZEY` extracted via substring search and integer parsing (lines 48-79).

2. **Validation**: Dimensions must be in range `(0, max_map_width]` x `(0, max_map_height]`. File must contain at least `256 + width * height * 10` bytes (lines 81-91).

3. **Tile records**: 10 bytes each, row-major (Y outer, X inner). Read as little-endian `int16_t` pairs (lines 93-136):

   | Offset | Bytes | Field |
   |--------|-------|-------|
   | 0 | 2 | `terrain_id` |
   | 2 | 2 | `terrain_frame` |
   | 4 | 2 | `object_id` |
   | 6 | 2 | `object_frame` |
   | 8 | 1 | AMD flags byte |
   | 9 | 1 | Reserved |

4. **Flag conversion** (lines 113-134):
   - Bit 7 (`0x80`) clear -> `tile_flag::walkable`
   - Bit 6 (`0x40`) set -> `tile_flag::teleport`
   - `terrain_id == 19` -> `tile_flag::water`
   - `light_level` initialized to 255 (full bright)

### Map name resolution (`src/world/world.cpp:94-102`)

Map name is lowercased and resolved to `./assets/data/mapdata/{name}.amd`.

---

## Tile Sprite Loading

### Tile Sprite Registry (`src/assets/tile_sprite_registry.hpp`, `src/assets/tile_sprite_registry.cpp`)

Maps legacy sprite IDs (int16 from AMD files) to PAK files. Each mapping is a `(pak_name, pak_index)` pair:

```cpp
struct sprite_source
{
    std::string pak_name;  // PAK file name without extension
    uint32_t pak_index;    // Index within that PAK
};

std::unordered_map<int16_t, sprite_source> id_map_;
std::unordered_map<int16_t, std::unique_ptr<sprite>> cache_;
```

### Initialization (`src/assets/tile_sprite_registry.cpp:11-33`)

Two phases:

1. **Core PAK registration** (`register_core_paks()`, lines 122-162): Hardcoded mappings from the original game:

   | ID Range | PAK File | Count | Content |
   |----------|----------|-------|---------|
   | 0-31 | `maptiles1` | 32 | Base terrain |
   | 70-96 | `Sinside1` | 27 | Cave interiors |
   | 100-145 | `Trees1` | 46 | Trees |
   | 150-195 | `TreeShadows` | 46 | Tree shadows |
   | 200-207 | `Objects1` | 8 | Objects |
   | 211-215 | `Objects2` | 5 | Objects |
   | 216-219 | `Objects3` | 4 | Objects |
   | 220 | `objects4` | 1 | Objects |
   | 300-314 | `maptiles2` | 15 | Additional terrain |
   | 320-329 | `maptiles4` | 10 | Additional terrain |
   | 330-348 | `maptiles5` | 19 | Additional terrain |
   | 349-352 | `maptiles6` | 4 | Additional terrain |
   | 353-361 | `maptiles353-361` | 9 | Additional terrain |

2. **Auto-discovery** (`discover_tile_paks()`, lines 164-233): Scans `assets/sprites/` for files matching `Tile###-###.pak` (case-insensitive regex: `tile(\d+)-(\d+)\.pak`). For each match:
   - Extracts start/end IDs from filename
   - Skips if core PAKs already cover that range
   - Loads PAK to verify actual sprite count
   - Registers the range (capped to actual count in file)

### `register_range()` (`src/assets/tile_sprite_registry.cpp:111-120`)

For a range starting at `start_id` with `count` sprites from `pak_name`:

```cpp
for (uint32_t i = 0; i < count; ++i)
{
    id_map_[start_id + i] = sprite_source{pak_name, i};
}
```

Each sprite ID maps to index `i` within that PAK.

### Sprite lookup (`src/assets/tile_sprite_registry.cpp:35-99`)

`get_sprite(int16_t id)`:

1. Check `cache_` for already-loaded sprite -> return immediately.
2. Look up `id_map_` for the mapping -> if missing, cache nullptr and return.
3. Get PAK from `sprite_manager::get_pak()`. If not loaded, try on-demand load via `sprite_manager::load_pak()` (path: `sprites_path_ + pak_name + ".pak"`).
4. Create `std::make_unique<sprite>()`, call `load_from_pak(pak, pak_index)`.
5. Cache and return pointer. On failure, cache nullptr to avoid repeated lookup.

### PAK Files (`src/assets/pak_file.hpp`)

PAK files are the original game's sprite archive format. Key structures:

```cpp
struct sprite_frame_info
{
    int16_t source_x, source_y;  // Region position in bitmap
    int16_t width, height;       // Frame size
    int16_t pivot_x, pivot_y;    // Drawing offset
};

struct pak_sprite_data
{
    std::vector<uint8_t> bitmap_data;      // RGBA pixel data
    std::vector<sprite_frame_info> frames;
    uint32_t bitmap_width, bitmap_height;
};
```

The `pak_file` class keeps the file open and maintains an offset table (`sprite_offsets_`) for random access. It supports three read modes:
- `read_sprite()` - full load (metadata + bitmap)
- `read_sprite_metadata()` - headers only (fast)
- `read_sprite_bitmap()` - bitmap only (for deferred loading)

### Sprite Manager (`src/assets/sprite_manager.hpp`)

Central PAK file and sprite cache manager:

- Maintains open PAK files in `pak_files_` (`unordered_map<string, unique_ptr<pak_file>>`)
- Caches sprites by `(pak_name, index)` key in `sprite_cache_`
- Supports LRU tracking for cache eviction
- Memory management: periodically evicts unused bitmaps (configurable timeout, default 30s, checked every 5s)

### Two-Tier Sprite Loading (`src/assets/sprite.hpp:24-123`)

Sprites support loading metadata separately from bitmap data:

| Tier | What's loaded | Memory | Speed |
|------|--------------|--------|-------|
| Metadata only | Frame rects, pivots, dimensions | Small | Fast |
| Full (metadata + bitmap) | Metadata + SFML textures | Large (GPU) | Slower |

Each sprite maintains two SFML textures:
- `texture_` - with color key applied (transparent background)
- `texture_no_colorkey_` - original pixels (opaque)

`ensure_loaded()` triggers on-demand bitmap loading before any draw call. Sprites track their last-use time for LRU eviction.

---

## Camera System

### State (`src/world/world.hpp:144-190`)

```cpp
double camera_x_ = 0.0;        // Camera position (double for zoom precision)
double camera_y_ = 0.0;
int32_t player_world_x_ = 0;   // Player position to follow
int32_t player_world_y_ = 0;
bool cinematic_mode_ = false;

uint32_t screen_width_ = 640;
uint32_t screen_height_ = 480;

// Shake
int32_t shake_offset_x_ = 0, shake_offset_y_ = 0;
float shake_intensity_ = 0.0f;
float shake_duration_ = 0.0f;
```

Effective camera position includes shake: `camera_x() = int32_t(camera_x_) + shake_offset_x_` (line 99).

### Normal mode

Camera follows the player, centered on screen. `set_player_position()` calls `center_on_player()` immediately (`src/world/world.cpp:146-156`).

### `center_on_player()` (`src/world/world.cpp:158-176`)

```
target_x = player_world_x - screen_width / 2
target_y = player_world_y - screen_height / 2
clamp target to [0, map_pixels - screen_size]
camera = target
```

### Cinematic mode

Camera detaches from player. Supports:
- Direct positioning: `set_camera_position(x, y)`
- Relative movement: `move_camera(dx, dy)`
- Drag-to-pan: `start_drag()`, `update_drag()`, `end_drag()` with zoom-scaled delta

### Camera shake (`src/world/world.cpp:202-207, 317-341`)

- `add_camera_shake(intensity, duration)` starts a shake
- Each frame: random offset within `[-intensity * (1-progress), +intensity * (1-progress)]`
- Offset linearly decays to zero over `duration` seconds
- Applied additively via `shake_offset_x_/y_`

### Resolution changes (`src/world/world.cpp:178-192`)

When screen size changes: update `screen_width_/height_`, propagate to `map_renderer_.set_screen_size()`, re-center camera.

---

## Zoom System

### State (`src/world/world.hpp:170-180`)

```cpp
double zoom_level_ = 1.0;         // Current (interpolated toward target)
double zoom_target_ = 1.0;        // Target zoom
bool zoom_mode_enabled_ = false;  // Server-controlled flag

// Anchor for zoom-to-cursor effect
double zoom_anchor_world_x_, zoom_anchor_world_y_;
double zoom_anchor_screen_x_, zoom_anchor_screen_y_;
bool has_zoom_anchor_ = false;
```

Zoom range: `0.5` (2x zoom in) to `8.0` (8x zoom out).

### `adjust_zoom()` (`src/world/world.cpp:400-437`)

When the user scrolls the mouse wheel:
1. Clamp new target to `[0.5, 8.0]`
2. If cursor position provided, compute the world point under cursor and snap it to tile center for stability
3. Record anchor in both world and screen space
4. Set `zoom_target_`

### Zoom interpolation (`src/world/world.cpp:36-57`)

Each frame in `update()`:

```cpp
constexpr double zoom_speed = 10.0;
double t = 1.0 - std::exp(-zoom_speed * delta_time);
zoom_level_ += (zoom_target_ - zoom_level_) * t;
```

Camera is recalculated to keep the anchor point fixed on screen:

```cpp
camera_x = anchor_world_x - screen_center_x
         - (anchor_screen_x - screen_center_x) * zoom_level;
camera_y = anchor_world_y - screen_center_y
         - (anchor_screen_y - screen_center_y) * zoom_level;
```

Anchor is cleared when zoom settles (delta < 0.001).

### SFML view zoom (`src/world/world.cpp:74-92`)

Before world rendering, `apply_zoom_view()` sets an SFML view centered on screen with the zoom factor. After world and entity rendering, `reset_zoom_view()` restores the default view for UI rendering.

---

## Visible Range Calculation

### `calculate_visible_range()` (`src/world/map_renderer.cpp:316-353`)

Returns a `visible_range { start_x, start_y, end_x, end_y }` defining which tiles to draw.

**Algorithm:**

```
effective_width  = screen_width  * zoom_level
effective_height = screen_height * zoom_level

tiles_x = int(effective_width  / 32) + 2
tiles_y = int(effective_height / 32) + 2

zoom_padding = ceil(zoom_level) + 2

// Offset camera for centered zoom
half_extra_w = (effective_width  - screen_width)  / 2
half_extra_h = (effective_height - screen_height) / 2
adjusted_camera_x = camera_x - half_extra_w
adjusted_camera_y = camera_y - half_extra_h

base_start_x = adjusted_camera_x / 32
base_start_y = adjusted_camera_y / 32

start_x = max(0, base_start_x - zoom_padding)
start_y = max(0, base_start_y - zoom_padding)
end_x   = min(map_width,  base_start_x + tiles_x + zoom_padding * 2)
end_y   = min(map_height, base_start_y + tiles_y + zoom_padding * 2)
```

The `zoom_padding` prevents pop-in at edges when zoomed out. At zoom 1.0, padding is 3 tiles on each side. At zoom 4.0, padding is 6 tiles.

---

## Rendering Pipeline

### Entry point (`src/world/map_renderer.cpp:110-230`)

`map_renderer::render()` is called once per frame by `world::render()`. It receives the renderer, map reference, and camera position.

### Rendering modes

Two rendering modes selected by zoom level:

| Mode | Condition | Method |
|------|-----------|--------|
| Tile-by-tile | `zoom_level < 2.0` | Draw each visible tile individually |
| Chunk-based | `zoom_level >= 2.0` | Pre-render 16x16 tile chunks to textures |

### Render config (`src/world/map_renderer.hpp:16-24`)

```cpp
struct map_render_config
{
    bool show_terrain = true;
    bool show_objects = true;
    bool show_roofs = true;
    bool show_grid = false;
    bool show_walkability = false;
    float light_level = 1.0f;      // Global light multiplier
};
```

### Tile-by-tile rendering (`src/world/map_renderer.cpp:166-178`)

When `!should_use_chunks()`:

```
1. If show_terrain: render_layer(renderer, map, camera_x, camera_y, 0)
2. If show_objects:  render_layer(renderer, map, camera_x, camera_y, 1)
```

### `render_layer()` (`src/world/map_renderer.cpp:232-299`)

Iterates over visible range (Y outer, X inner). For each tile:

**Layer 0 - Terrain** (lines 260-283):

1. Look up `tile.terrain_id` via `get_tile_sprite()` -> `tile_sprite_registry::get_sprite()`
2. Get frame index from `tile.terrain_frame`. Skip if frame >= sprite's frame count.
3. Calculate screen position: `tile_to_screen(x, y, camera_x, camera_y)`
4. Compute lighting alpha: `config_.light_level * (tile.light_level / 255.0f)`
5. If alpha >= 1.0: `rend.draw_sprite_no_color_key(sprite, sx, sy, frame)` (opaque, no transparency)
6. If alpha < 1.0: `rend.draw_sprite_alpha_no_color_key(sprite, sx, sy, frame, alpha)`

Note: `terrain_id = 0` is valid (first sprite in `maptiles1.pak`), not a sentinel.

**Layer 1 - Objects** (lines 285-296):

1. Skip if `tile.object_id == 0` (no object)
2. Look up `tile.object_id` via `get_tile_sprite()`
3. Draw with color key transparency: `rend.draw_sprite(sprite, sx, sy, 0)` (always frame 0)

Objects use color-keyed sprites (transparent background) while terrain uses non-color-keyed sprites (solid, no transparency).

### Sprite draw methods (`src/assets/sprite.cpp:384-416`)

**`draw_no_color_key()`** (terrain):
```cpp
// Uses texture_no_colorkey_ (original pixels, fully opaque)
sf::Sprite spr(texture_no_colorkey_, frame.source_rect);
spr.setPosition({x + frame.pivot_x, y + frame.pivot_y});
target.draw(spr);
```

**`draw_alpha_no_color_key()`** (dimmed terrain):
```cpp
// Same texture, but with alpha modulation
sf::Sprite spr(texture_no_colorkey_, frame.source_rect);
spr.setPosition({x + pivot_x, y + pivot_y});
spr.setColor(sf::Color(255, 255, 255, alpha * 255));
target.draw(spr);
```

**`draw()`** (objects):
```cpp
// Uses texture_ (color key applied, transparent background)
sf::Sprite spr(texture_, frame.source_rect);
spr.setPosition({x + frame.pivot_x, y + frame.pivot_y});
target.draw(spr);
```

All draw methods call `ensure_loaded()` first, which triggers on-demand bitmap loading if only metadata was loaded.

### Renderer delegation (`src/graphics/renderer.cpp:92-118`)

The `renderer` class delegates sprite drawing to the sprite's own methods, passing the SFML `RenderWindow`:

```cpp
void renderer::draw_sprite_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame)
{
    spr.draw_no_color_key(window_, x, y, frame);
}
```

For chunk textures, `draw_texture()` creates an `sf::Sprite` from the texture and draws it at the given screen position (line 108-112).

---

## Chunk-Based Rendering

### Configuration (`src/world/map_renderer.hpp:77-78`)

```cpp
static constexpr int32_t chunk_size_ = 16;          // 16x16 tiles per chunk
static constexpr float chunk_zoom_threshold_ = 2.0f; // Use chunks when zoom >= 2.0
```

Each chunk renders to a 512x512 pixel `sf::RenderTexture` (16 tiles * 32 px).

### Data structures (`src/world/map_renderer.hpp:80-93`)

```cpp
struct chunk_key { int32_t cx, cy; };  // Chunk grid coordinates
struct chunk_data
{
    std::unique_ptr<sf::RenderTexture> texture;
    bool valid = false;                 // False = needs re-rendering
};

std::unordered_map<chunk_key, chunk_data, chunk_key_hash> chunks_;
```

### Chunk rendering flow (`src/world/map_renderer.cpp:126-164`)

When `should_use_chunks()`:

1. Convert visible tile range to chunk range: `chunk_x = tile_x / 16`
2. For each visible chunk:
   - `get_or_create_chunk()`: creates texture if new (line 36-57)
   - If `!chunk.valid`: call `render_chunk()` to fill the texture
   - Draw chunk texture at world position: `screen_x = cx * 16 * 32 - camera_x`
3. After all chunks: render object layer separately via `render_layer(rend, m, camera_x, camera_y, 1)`

Objects are NOT included in chunk textures. They are always drawn on top individually to maintain correct layering.

### `render_chunk()` (`src/world/map_renderer.cpp:59-108`)

Renders terrain only to the chunk's `sf::RenderTexture`:

1. Clear to transparent
2. For each tile in the 16x16 chunk area (clamped to map bounds):
   - Look up terrain sprite
   - Calculate local position within chunk: `(x - start_x) * 32, (y - start_y) * 32`
   - Apply lighting alpha
   - Draw using `spr->draw_no_color_key()` or `spr->draw_alpha_no_color_key()` directly to the render texture
3. Call `texture->display()` and mark `valid = true`

### Invalidation (`src/world/map_renderer.cpp:23-29`)

`invalidate_chunks()` marks all chunks as `valid = false` without destroying textures. Called when map data changes. Chunks are re-rendered on next draw.

---

## Lighting

### Per-tile light level

Each tile stores `light_level` (0-255). Set to 255 on load from AMD files. Can be modified at runtime.

### Global light level

`map_render_config::light_level` is a float multiplier (0.0 to 1.0). Updated by `world::update_lighting()` (`src/world/world.cpp:347-386`) based on time of day and weather:

| Time | Light Level |
|------|-------------|
| Dawn | 0.6 |
| Morning | 0.8 |
| Noon | 1.0 |
| Afternoon | 0.9 |
| Dusk | 0.5 |
| Night | 0.3 |
| Midnight | 0.2 |

Rain or storm weather applies an additional 0.8x multiplier.

### Combined lighting in rendering

```cpp
float alpha = config_.light_level * (tile.light_level / 255.0f);
```

If `alpha >= 1.0`: draw at full brightness (no alpha blending overhead).
If `alpha < 1.0`: draw with alpha modulation via `sf::Color(255, 255, 255, alpha * 255)`.

---

## Debug Overlays

Always drawn directly (not through chunks) so they appear on top of everything. Controlled by `map_render_config` flags, toggled via keyboard:

| Flag | Key | Overlay |
|------|-----|---------|
| `show_grid` | F7 | Black grid lines (2px) at tile boundaries |
| `show_walkability` | F6 | Color-coded tile overlay |

### Grid overlay (`src/world/map_renderer.cpp:181-201`)

Draws horizontal and vertical lines across the visible range using `rend.draw_rect()` with color `(0, 0, 0, 200)`.

### Walkability overlay (`src/world/map_renderer.cpp:203-229`)

For each visible tile, draws a filled rectangle:

| Condition | Color |
|-----------|-------|
| Not walkable | Red `(255, 0, 0, 100)` |
| Teleport | Green `(0, 255, 0, 100)` |
| Occupied | Yellow `(255, 255, 0, 100)` |

---

## Full Frame Rendering Sequence

```
game_state_manager::render_playing()                   [game_state.cpp:832]
  │
  ├─ world_.apply_zoom_view(rend)                      [world.cpp:74-84]
  │     └─ rend.set_zoom_view(zoom, center_x, center_y)
  │           └─ SFML view zoom centered on screen
  │
  ├─ world_.render(rend)                               [world.cpp:60-72]
  │     ├─ map_renderer_.set_zoom_level(zoom)
  │     └─ map_renderer_.render(rend, map, cam_x, cam_y)
  │           │
  │           ├─ calculate_visible_range()              [map_renderer.cpp:316-353]
  │           │
  │           ├─ [If zoom >= 2.0] Chunk path           [map_renderer.cpp:126-164]
  │           │     ├─ For each visible chunk:
  │           │     │     ├─ get_or_create_chunk()
  │           │     │     ├─ render_chunk() if invalid
  │           │     │     │     └─ Terrain sprites -> sf::RenderTexture
  │           │     │     └─ rend.draw_texture(chunk, screen_pos)
  │           │     └─ render_layer(1) - objects on top
  │           │
  │           ├─ [If zoom < 2.0] Tile-by-tile path     [map_renderer.cpp:166-178]
  │           │     ├─ render_layer(0) - terrain
  │           │     │     └─ For each tile: get_tile_sprite() -> draw_no_color_key()
  │           │     └─ render_layer(1) - objects
  │           │           └─ For each tile: get_tile_sprite() -> draw()
  │           │
  │           └─ Debug overlays (grid, walkability)     [map_renderer.cpp:180-229]
  │
  ├─ entities_.render(...)                             (not part of map rendering)
  │
  └─ world_.reset_zoom_view(rend)                      [world.cpp:86-92]
        └─ rend.reset_to_default_view()
```

### Sprite data flow

```
AMD file on disk
  │  load_from_memory()
  ▼
tile.terrain_id / tile.object_id  (int16_t)
  │  tile_sprite_registry::get_sprite(id)
  ▼
sprite_source { pak_name, pak_index }
  │  sprite_manager::get_pak() + sprite::load_from_pak()
  ▼
PAK file -> pak_sprite_data { bitmap_data, frames }
  │  create sf::Texture
  ▼
sprite { texture_, texture_no_colorkey_, frames_ }
  │  draw_no_color_key() / draw()
  ▼
sf::RenderWindow (pixels on screen)
```

---

## Key Files

| File | Purpose |
|------|---------|
| `src/world/tile.hpp` | `tile` struct, `tile_flag` enum, tile dimensions |
| `src/world/map.hpp` | `map` class, constants, coordinate helpers |
| `src/world/map.cpp` | AMD loading, tile access, pathfinding helpers |
| `src/world/map_renderer.hpp` | `map_renderer` class, `map_render_config`, chunk types |
| `src/world/map_renderer.cpp` | Rendering logic, visible range, chunk system, overlays |
| `src/world/world.hpp` | `world` class, camera, zoom, weather/time enums |
| `src/world/world.cpp` | Camera updates, zoom interpolation, lighting, map lifecycle |
| `src/assets/tile_sprite_registry.hpp` | Sprite ID mapping declarations |
| `src/assets/tile_sprite_registry.cpp` | Core PAK mappings, auto-discovery, lazy sprite loading |
| `src/assets/sprite.hpp` | `sprite` class, two-tier loading, draw methods |
| `src/assets/sprite.cpp` | Sprite draw implementations (color key, no color key, alpha) |
| `src/assets/sprite_manager.hpp` | PAK management, sprite caching, memory eviction |
| `src/assets/pak_file.hpp` | PAK file format, frame info structs |
| `src/graphics/renderer.hpp` | `renderer` class, sprite/texture draw delegation |
| `src/graphics/renderer.cpp` | SFML window, draw methods, zoom view |
| `src/gameplay/game_state.cpp` | `render_playing()` orchestrates world + entity + UI rendering |
| `docs/amd_file_format.md` | Detailed AMD binary format specification |
