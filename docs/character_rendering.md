# Character Rendering

Exhaustive reference for how player characters, NPCs, monsters, items, and effects are rendered in the modern system (`src/`). Covers the sprite rendering pipeline from asset loading through final draw calls. For animation state machines and frame timing, see `character_animation.md`.

---

## Table of Contents

1. [Render Pipeline Overview](#render-pipeline-overview)
2. [Sprite Loading and Storage](#sprite-loading-and-storage)
3. [Player Character Rendering](#player-character-rendering)
4. [NPC and Monster Rendering](#npc-and-monster-rendering)
5. [Item and Effect Rendering](#item-and-effect-rendering)
6. [Sprite ID Formulas](#sprite-id-formulas)
7. [Frame Index Calculation](#frame-index-calculation)
8. [Direction System](#direction-system)
9. [Menu Character Renderer](#menu-character-renderer)
10. [Sprite Draw Methods](#sprite-draw-methods)
11. [Color Key Transparency](#color-key-transparency)
12. [Alpha Transparency](#alpha-transparency)
13. [Camera and World-to-Screen Transform](#camera-and-world-to-screen-transform)
14. [Zoom System](#zoom-system)
15. [Depth Sorting](#depth-sorting)
16. [Frustum Culling](#frustum-culling)
17. [Entity Overlays](#entity-overlays)
18. [Hit Testing](#hit-testing)
19. [Sprite Memory Management](#sprite-memory-management)
20. [Not Yet Implemented](#not-yet-implemented)
21. [Key Files Reference](#key-files-reference)

---

## Render Pipeline Overview

The main render loop for the playing state is orchestrated by `game_state_manager::render_playing()` (`gameplay/game_state.cpp:832`):

```
1. world_.apply_zoom_view(rend)         -- Set SFML view for zoom
2. world_.render(rend)                  -- Map terrain + objects
3. entities_.render(rend, sprites_, ...) -- All entities (Y-sorted)
4. world_.reset_zoom_view(rend)         -- Reset to default view
5. debug overlay                        -- Debug stats (no zoom)
6. status_log_.render(rend, ...)        -- Status messages (no zoom)
7. ui_.render(rend)                     -- UI layer (called from parent render())
```

Steps 1-3 render under the zoom view. Steps 4-7 render at native screen resolution. The UI layer always draws on top of everything.

### Entity Render Call Chain

```
entity_manager::render()                    [entity_manager.cpp:629]
  -> Collect visible entities (frustum cull) [lines 631-658]
  -> Sort by Y position (depth ordering)     [lines 660-664]
  -> For each visible entity:
       render_entity()                       [line 672]
         -> Items: draw body_sprite           [line 685]
         -> Effects: draw effect_sprite       [line 690]
         -> NPCs/Monsters: render_npc_or_monster() [line 696]
         -> Players: render_player_character() [line 700]
         -> render_entity_name()             [line 704]
         -> render_entity_health_bar()       [line 712]
```

---

## Sprite Loading and Storage

### sprite_manager Architecture

`sprite_manager` (`assets/sprite_manager.hpp`) manages all sprite assets through three storage layers:

| Layer | Type | Key | Purpose |
|-------|------|-----|---------|
| `pak_files_` | `unordered_map<string, unique_ptr<pak_file>>` | PAK name | Open PAK file handles |
| `sprite_cache_` | `unordered_map<sprite_key, unique_ptr<sprite>>` | `{pak_name, index}` | Loaded sprite objects |
| `id_sprites_` | `unordered_map<uint16_t, sprite*>` | Global sprite ID | O(1) lookup by numeric ID |

**sprite_key** hashes on `{pak_name, sprite_index}`. The `id_sprites_` map stores raw pointers into `sprite_cache_`, mirroring the original game's `m_pSprite[ID]` array.

### PAK File Format

PAK files (`assets/pak_file.hpp`) are Helbreath's proprietary sprite archive format. Each PAK contains multiple sprites, each comprising:

- **Frame metadata** (`sprite_frame_info`): source rect in the bitmap, pivot offsets
- **Bitmap data**: BMP or raw RGBA pixel data

Key structures:
```cpp
struct sprite_frame_info {
    int16_t source_x, source_y;   // Position in bitmap atlas
    int16_t width, height;        // Frame dimensions
    int16_t pivot_x, pivot_y;     // Draw offset from entity position
};
```

### Two-Tier Loading

Sprites support two independent loading phases:

1. **Metadata-only** (`sprite::load_metadata_from_pak()`): Loads frame definitions, bitmap dimensions, and pivot offsets. No GPU memory allocated. Fast startup.

2. **Bitmap loading** (`sprite::load_bitmap()`): Reads pixel data from PAK, creates two SFML textures (with and without color key). Allocates GPU memory.

On-demand loading happens transparently during rendering via `sprite::ensure_loaded()` (`assets/sprite.cpp:284`). If a sprite has metadata but no bitmap, `ensure_loaded()` triggers `load_bitmap()` before the draw call proceeds.

### Loading Paths

**Menu screens** use `menu_character_renderer::initialize()` (`graphics/menu_character_renderer.cpp:264`), which iterates PAK loading tables (`pak_load_entry` arrays) and calls `load_pak_at_offset()` for each entry. This does full loading (metadata + bitmap) and registers each sprite at a global ID via `sprites.store_sprite_at_id()`.

**In-game** uses `entity_manager::load_character_sprites()` (`entity/entity_manager.cpp:892`), which calculates sprite IDs from appearance data and looks them up via `sprites.get_sprite_by_id()`. The sprites must already be loaded (from menu initialization or preloading).

### PAK Name Conventions

Common PAK file names used by `sprite_manager`:

| Constant | PAK name | Content |
|----------|----------|---------|
| `pak_tiles` | `"tiles"` | Terrain tiles |
| `pak_effects` | `"effects"` | Visual effects |
| `pak_interface` | `"interface"` | UI elements |
| `pak_objects` | `"objects"` | Map decorations |
| `pak_malehu` | `"malehu"` | Male human sprites |
| `pak_femalehu` | `"femalehu"` | Female human sprites |
| `pak_monsters` | `"monsters"` | Monster sprites |

Equipment PAKs use short names like `"Bm"`, `"Wm"`, `"Ym"` (body), `"Mpt"` (male underwear), `"MLArmor"` (male leather armor), etc. See `menu_character_renderer.cpp` lines 17-236 for the complete mapping.

---

## Player Character Rendering

Player characters use multi-layer sprite compositing. Each layer is an independent sprite drawn at the same screen position.

### In-Game Layers (Current Implementation)

`entity_manager::render_player_character()` (`entity/entity_manager.cpp:718`) draws these layers in order:

| Order | Layer | Sprite Source | Frame Index | Condition |
|-------|-------|---------------|-------------|-----------|
| 1 | Underwear | `calculate_underwear_sprite_id()` | `(dir-1)*8 + current_frame` | Always |
| 2 | Body | `calculate_body_sprite_id()` | `current_frame` only | Always |
| 3 | Hair | `calculate_hair_sprite_id()` | `(dir-1)*8 + current_frame` | No `helm_sprite` |
| 4 | Effect overlay | `sprite_component.effect_sprite` | `current_frame` | If set |

Each layer respects the entity's `alpha` value. When `alpha < 1.0`, layers use `draw_sprite_alpha()` instead of `draw_sprite()`.

**Important difference**: Body sprites encode direction in the sprite ID itself (one sprite per action+direction combination), so only `current_frame` is needed as the frame index. Underwear and hair pack all 8 directions into a single sprite per action, so they use the compound frame index `(dir-1)*8 + current_frame`.

### sprite_component Fields

Defined in `entity/components.hpp:26`:

```cpp
struct sprite_component {
    // Layer pointers (null = not equipped/not loaded)
    const sprite* body_sprite = nullptr;
    const sprite* armor_sprite = nullptr;
    const sprite* weapon_sprite = nullptr;
    const sprite* shield_sprite = nullptr;
    const sprite* helm_sprite = nullptr;
    const sprite* hair_sprite = nullptr;
    const sprite* underwear_sprite = nullptr;
    const sprite* effect_sprite = nullptr;

    // Per-layer frame indices
    uint8_t body_frame = 0;
    uint8_t armor_frame = 0;
    uint8_t weapon_frame = 0;

    // Appearance data (drives sprite ID calculation)
    uint8_t skin_color = 0;       // 1-3
    uint8_t hair_style = 0;       // 0-7
    uint8_t hair_color = 0;       // 0-15 (not yet used in rendering)
    uint8_t underwear_color = 0;  // 0-7
    uint8_t gender = 1;           // 1=male, 2=female

    // Rendering modifiers
    float alpha = 1.0f;           // 0.0=invisible, 1.0=opaque
    bool visible = true;          // false = skip rendering entirely
    bool flipped = false;         // Horizontal flip (not yet used)
};
```

### Owner Type Calculation

`calculate_owner_type()` (`entity/entity_manager.cpp:43`) maps gender + skin to a 1-6 index:

| Gender | Skin Color | Owner Type |
|--------|-----------|------------|
| Male (1) | 1 (Black) | 1 |
| Male (1) | 2 (White) | 2 |
| Male (1) | 3 (Yellow) | 3 |
| Female (2) | 1 (Black) | 4 |
| Female (2) | 2 (White) | 5 |
| Female (2) | 3 (Yellow) | 6 |

Both gender and skin_color are clamped to valid ranges.

---

## NPC and Monster Rendering

NPCs and monsters use single-sprite rendering (no layering).

`entity_manager::render_npc_or_monster()` (`entity/entity_manager.cpp:773`):

1. Get direction index via `direction_to_sprite_index()`
2. Map `object_action` to NPC action index (0-6) via `action_to_npc_action_index()`
3. Resolve `visual_type` from the entity's `visual_type()`, falling back to `npc_component.npc_type` or `monster_component.monster_type`
4. Default to type 10 (Slime) if visual type < 10
5. Calculate sprite ID via `calculate_npc_sprite_id()`
6. Look up sprite via `sprites.get_sprite_by_id()`
7. Draw with `draw_sprite()` or `draw_sprite_alpha()` based on `alpha`
8. Draw `effect_sprite` overlay if present

### NPC Action Index Mapping

`action_to_npc_action_index()` (`entity/entity_manager.cpp:83`) collapses the 12 player action types into 7 NPC actions:

| NPC Index | NPC Action | Mapped From |
|-----------|-----------|-------------|
| 0 | Stop/idle | `stop_peace`, `stop_combat` |
| 1 | Move | `move_peace`, `move_combat`, `run` |
| 2 | Attack | `attack_peace`, `attack_combat`, `attack_combat_bow` |
| 3 | Damage | `damage` |
| 4 | Dying | `dying` |
| 5 | Dead | (not directly mapped, held at dying frame) |
| 6 | Magic | `magic` |

`get_item` and any unrecognized actions default to index 0 (stop).

---

## Item and Effect Rendering

### Items

Items (`entity_type::item`) render a single sprite at a fixed offset from their position (`entity_manager.cpp:685`):

```cpp
rend.draw_sprite(*s.body_sprite, screen_x - 16, screen_y - 16, s.body_frame);
```

The -16 offset centers the 32x32 item sprite on the tile center.

### Effects

Effects (`entity_type::effect`) render from their `effect_component` (`entity_manager.cpp:690`):

```cpp
rend.draw_sprite(*eff.effect_sprite, screen_x + eff.offset_x, screen_y + eff.offset_y, eff.effect_frame);
```

Effects have configurable `offset_x`/`offset_y` for positioning relative to the entity, and a `effect_timer`/`effect_duration` for auto-removal.

---

## Sprite ID Formulas

### Character Sprite Constants

Defined in `entity/entity_manager.cpp:18` and `graphics/menu_character_renderer.hpp:101`:

```cpp
struct character_sprite_constants {
    static constexpr uint16_t body_base = 500;
    static constexpr uint16_t body_stride = 120;         // 15 actions * 8 directions
    static constexpr uint16_t male_underwear_base = 4580;
    static constexpr uint16_t female_underwear_base = 14580;
    static constexpr uint16_t underwear_stride = 15;
    static constexpr uint16_t male_hair_base = 4820;
    static constexpr uint16_t female_hair_base = 14820;
    static constexpr uint16_t hair_stride = 15;
};
```

### Body Sprite ID

`calculate_body_sprite_id()` (`entity/entity_manager.cpp:52`):

```
sprite_id = 500 + (owner_type - 1) * 120 + action * 8 + (direction - 1)
```

- `owner_type`: 1-6 (see Owner Type table above)
- `action`: 0-14 (from `object_action` enum value)
- `direction`: 1-8 (sprite direction index)
- Each owner type occupies 120 sprite IDs (15 actions x 8 directions)
- Each sprite contains multiple animation frames internally

**Example**: Female, skin 2, walking peace mode (action=2), facing east (dir=3):
```
owner_type = 3 + 2 = 5
sprite_id = 500 + (5-1)*120 + 2*8 + (3-1) = 500 + 480 + 16 + 2 = 998
```

### Underwear Sprite ID

`calculate_underwear_sprite_id()` (`entity/entity_manager.cpp:62`):

```
sprite_id = base + color * 15 + action
```

- `base`: 4580 (male) or 14580 (female)
- `color`: 0-7 (clamped)
- `action`: 0-14
- Each color occupies 15 sprite IDs (one per action)
- Direction is encoded in the frame index, not the sprite ID

### Hair Sprite ID

`calculate_hair_sprite_id()` (`entity/entity_manager.cpp:72`):

```
sprite_id = base + style * 15 + action
```

- `base`: 4820 (male) or 14820 (female)
- `style`: 0-7 (clamped)
- `action`: 0-14

### NPC/Monster Sprite ID

`calculate_npc_sprite_id()` (`entity/entity_manager.cpp:112`):

```
sprite_id = 1220 + (visual_type - 10) * 56 + npc_action * 8 + (direction - 1)
```

- `visual_type`: 10+ (10=Slime, 11=Skeleton, etc.)
- `npc_action`: 0-6 (NPC action index)
- `direction`: 1-8 (clamped)
- Each monster type occupies 56 sprite IDs (7 actions x 8 directions)

### Equipment Sprite IDs (Menu Renderer Only)

Full equipment sprite ID formulas from `menu_character_renderer.hpp`:

| Equipment | Formula | Base (M/F) | Stride |
|-----------|---------|------------|--------|
| Body armor | `base + type * 15 + action` | 5060 / 15060 | 15 per type |
| Arm armor | `base + type * 15 + action` | 5300 / 15300 | 15 per type |
| Pants | `base + type * 15 + action` | 5540 / 15540 | 15 per type |
| Boots | `base + type * 15 + action` | 5780 / 15780 | 15 per type |
| Weapons | `base + type * 64 + action * 8 + (dir-1)` | 6020 / 16020 | 64 per type |
| Shields | `base + type * 8 + action` | 9100 / 19100 | 8 per type |
| Mantles | `base + type * 15 + action` | 9230 / 19230 | 15 per type |
| Helmets | `base + type * 15 + action` | 9300 / 19300 | 15 per type |

Note: Weapons have a larger stride (64 = 8 actions x 8 directions) because they encode direction in the sprite ID, similar to body sprites. All other equipment types encode direction in the frame index.

---

## Frame Index Calculation

Two different frame indexing schemes are used depending on whether direction is encoded in the sprite ID:

### Direction-in-Sprite-ID (Body, Weapons, NPC/Monster)

The sprite ID already selects a specific direction, so the frame index is just the animation frame:

```cpp
frame_index = current_frame  // 0-7 typically
```

Used by: `render_player_character()` for body sprites (line 754), `render_npc_or_monster()` (line 802).

### Direction-in-Frame-Index (Underwear, Hair, Armor, etc.)

A single sprite per action contains all 8 directions. Each direction occupies 8 frame slots:

```cpp
frame_index = (direction - 1) * 8 + current_frame
```

- `direction`: 1-8
- `current_frame`: 0-7
- Total frame count per sprite: up to 64 (8 dirs x 8 frames)

Used by: `render_player_character()` for underwear/hair (line 728), all menu renderer equipment draws.

### calc_frame() Helper

The menu renderer provides a helper (`menu_character_renderer.cpp:319`):

```cpp
int32_t calc_frame(int32_t direction, int32_t frame) const {
    return (direction - 1) * 8 + frame;
}
```

---

## Direction System

### direction Enum

Defined in `core/game_enums.hpp:144`:

| Value | Direction | Sprite Index |
|-------|-----------|-------------|
| 0 | none | defaults to 5 (south) |
| 1 | north | 1 |
| 2 | north_east | 2 |
| 3 | east | 3 |
| 4 | south_east | 4 |
| 5 | south | 5 |
| 6 | south_west | 6 |
| 7 | west | 7 |
| 8 | north_west | 8 |

### Conversion Functions

`core/direction_utils.hpp`:

| Function | Purpose |
|----------|---------|
| `direction_to_sprite_index(dir, default=5)` | Enum to sprite index (1-8), defaults to south |
| `direction_from_protocol(int16_t)` | Network protocol (0-7) to enum (1-8) |
| `direction_to_protocol(dir)` | Enum (1-8) to protocol (0-7) |
| `calculate_direction(from_x, from_y, to_x, to_y)` | Compute direction between two tiles |

`direction_to_sprite_index()` (`core/direction_utils.hpp:37`) passes through the enum's integer value (1-8) directly, since the enum values match sprite direction indices. Returns `default_dir` (5 = south) for invalid inputs.

---

## Menu Character Renderer

`menu_character_renderer` (`graphics/menu_character_renderer.hpp`) provides full equipment layering for character previews on menu screens (character select/create). This is the reference implementation for the complete draw order.

### character_appearance Struct

```cpp
struct character_appearance {
    uint8_t gender = 1;           // 1=male, 2=female
    uint8_t skin_color = 1;       // 1-3
    uint8_t hair_style = 0;       // 0-7
    uint8_t hair_color = 0;       // 0-15 (not rendered)
    uint8_t underwear_color = 0;  // 0-7
    uint8_t body_armor = 0;       // 1-15 (0=none)
    uint8_t arm_armor = 0;        // 1-15
    uint8_t pants = 0;            // 1-15 (1=skirt for female)
    uint8_t boots = 0;            // 1-15
    uint8_t helmet = 0;           // 1-15
    uint8_t mantle = 0;           // 1-15
    uint8_t weapon = 0;           // 1-255
    uint8_t shield = 0;           // 1-9
    uint8_t weapon_glow = 0;      // 0-3 (not rendered)
    uint8_t shield_glow = 0;      // 0-3 (not rendered)
};
```

### Direction-Dependent Draw Order

Two static arrays control layer ordering based on facing direction:

```cpp
// Weapon: 0 = drawn LAST (in front), 1 = drawn FIRST (behind body)
static constexpr int8_t drawing_order[9] = {0, 1, 0, 0, 0, 0, 0, 1, 1};

// Mantle: 0 = early (before underwear), 1 = after shield, 2 = between armor and shield
static constexpr int8_t mantle_drawing_order[9] = {0, 1, 1, 1, 0, 0, 0, 2, 2};
```

Index 0 is unused (directions are 1-8).

| Direction | Weapon Position | Mantle Position |
|-----------|----------------|-----------------|
| 1 (N) | Behind | After shield |
| 2 (NE) | In front | After shield |
| 3 (E) | In front | After shield |
| 4 (SE) | In front | Early |
| 5 (S) | In front | Early |
| 6 (SW) | In front | Early |
| 7 (W) | Behind | Between armor/shield |
| 8 (NW) | Behind | Between armor/shield |

### Full Draw Order

`menu_character_renderer::draw()` (`graphics/menu_character_renderer.cpp:323`):

```
 1. [if weapon_behind] Weapon
 2. Body (skin)
 3. [if mantle_order==0] Mantle (early)
 4. Underwear
 5. [if no helmet] Hair
 6. [if skirt] Boots (early - under skirt)
 7. Pants/Skirt
 8. Arm armor (shirts/sleeves)
 9. [if not skirt] Boots (late - over pants)
10. Body armor (chest plate/robe)
11. Helmet (replaces hair)
12. [if mantle_order==2] Mantle (between)
13. Shield
14. [if mantle_order==1] Mantle (after shield)
15. [if !weapon_behind] Weapon
```

### Equipment PAK Loading Tables

`menu_character_renderer.cpp` lines 17-236 define `pak_load_entry` arrays mapping PAK filenames to global sprite ID ranges. Each entry specifies:

```cpp
struct pak_load_entry {
    const char* pak_name;       // PAK filename (without .pak extension)
    uint32_t sprite_id;         // Starting global sprite ID
    uint32_t sprite_count;      // Number of sprites to register
};
```

**Body PAKs** (6 total, 120 sprites each):
- Male: `"Bm"` (skin 1), `"Wm"` (skin 2), `"Ym"` (skin 3) at IDs 500-859
- Female: `"Bw"` (skin 1), `"Ww"` (skin 2), `"Yw"` (skin 3) at IDs 860-1219

**Weapon PAKs** (categories by weapon class):
- Swords: `"Msw"`, `"Mswx"`, `"Msw2"` / `"Wsw"`, `"Wswx"`, `"Wsw2"` (14 types each)
- Axes: `"MAxe1"`-`"MAxe6"`, `"MPickAxe1"`, `"Mhoe"` (8 types)
- Hammers: `"MHammer"`, `"MBHammer"` (2 types)
- Staffs: `"Mstaff1"`, `"Mstaff2"` (2 types)
- Bows: `"Mbo"` (2 types)

**Shadow rendering**: `draw_with_shadow()` (`line 411`) is defined but currently just calls `draw()` without any shadow effect.

---

## Sprite Draw Methods

### sprite Class Draw API

Defined in `assets/sprite.hpp:74` and implemented in `assets/sprite.cpp:324`:

| Method | Texture Used | Alpha | Purpose |
|--------|-------------|-------|---------|
| `draw()` | `texture_` (color key) | Opaque | Standard character/object rendering |
| `draw_alpha()` | `texture_` (color key) | Variable | Transparent entities (invisibility, etc.) |
| `draw_no_color_key()` | `texture_no_colorkey_` | Opaque | Background/terrain rendering |
| `draw_alpha_no_color_key()` | `texture_no_colorkey_` | Variable | Semi-transparent backgrounds |
| `draw_width()` | `texture_` (color key) | Opaque | Partial-width rendering (HP/MP bars) |

### Draw Implementation Detail

Every draw method:

1. Calls `ensure_loaded()` to trigger on-demand bitmap loading
2. Validates the frame index against `frames_.size()`
3. Looks up the `sprite_frame` for pivot offsets and source rect
4. Creates an `sf::Sprite` with the appropriate texture and source rect
5. Positions at `(x + pivot_x, y + pivot_y)` -- pivots offset the sprite so the entity position corresponds to the character's feet
6. Submits to the SFML `RenderTarget`

For alpha draws, an additional step sets the sprite color to `sf::Color(255, 255, 255, alpha_byte)` where `alpha_byte = clamp(alpha * 255, 0, 255)`.

### renderer Wrapper Methods

`renderer` (`graphics/renderer.hpp:21`) delegates directly to `sprite`:

```cpp
void draw_sprite(const sprite& spr, int32_t x, int32_t y, uint32_t frame);
void draw_sprite_alpha(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);
void draw_sprite_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame);
void draw_sprite_alpha_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);
```

These pass the SFML `window_` as the render target to the corresponding `sprite` methods (`graphics/renderer.cpp:92-106`).

---

## Color Key Transparency

Every sprite loaded from a PAK file uses **color key masking** for transparency.

### How It Works

1. During `sprite::load_from_data()` (`assets/sprite.cpp:174-191`):
   - The pixel at position (0,0) is captured as `color_key_`
   - `texture_no_colorkey_` is created from the original image
   - `image.createMaskFromColor(color_key_)` converts all matching pixels to alpha=0
   - `texture_` is created from the masked image

2. Same process occurs in `sprite::load_bitmap()` (`assets/sprite.cpp:248-264`) for on-demand loading.

3. Character/entity sprites use `texture_` (with color key), ensuring the background color is transparent. Terrain/background sprites use `texture_no_colorkey_` to preserve all colors.

### Dual Texture Storage

Each loaded sprite maintains two textures:
- `texture_`: Color key applied (character rendering)
- `texture_no_colorkey_`: Original colors (background rendering)

This doubles GPU memory per sprite: `2 * width * height * 4 bytes`.

---

## Alpha Transparency

`sprite_component.alpha` (default 1.0) controls per-entity transparency.

### Rendering Path Selection

In `render_player_character()` (`entity/entity_manager.cpp:744`):

```cpp
if (s.alpha < 1.0f) {
    rend.draw_sprite_alpha(*underwear_spr, screen_x, screen_y, frame_index, s.alpha);
} else {
    rend.draw_sprite(*underwear_spr, screen_x, screen_y, frame_index);
}
```

The same pattern applies to body, NPC/monster sprites, and all other layers. When `alpha == 1.0`, the faster non-alpha path is used.

### Alpha Byte Conversion

In `sprite::draw_alpha()` (`assets/sprite.cpp:352`):

```cpp
uint8_t alpha_byte = static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
spr.setColor(sf::Color(255, 255, 255, alpha_byte));
```

The SFML sprite color modulates the texture: `(255,255,255,alpha)` preserves original RGB while applying uniform transparency.

---

## Camera and World-to-Screen Transform

### Camera Position

The `world` class (`world/world.hpp:99`) provides the camera position including shake offset:

```cpp
int32_t camera_x() const { return static_cast<int32_t>(camera_x_) + shake_offset_x_; }
int32_t camera_y() const { return static_cast<int32_t>(camera_y_) + shake_offset_y_; }
```

Camera position is stored as `double` internally for precision during zoom interpolation, cast to `int32_t` for rendering.

### World-to-Screen Transform

Each entity's screen position is computed in `render_entity()` (`entity/entity_manager.cpp:677`):

```cpp
int32_t screen_x = t.x - camera_x;
int32_t screen_y = t.y - camera_y;
```

### Entity World Position

Entity position (`transform_component.x`, `transform_component.y`) represents the **tile center** in world pixels:

```cpp
t.x = tile_x * 32 + 16;   // tile_width=32, center offset=16
t.y = tile_y * 32 + 16;    // tile_height=32, center offset=16
```

Tile constants from `world/tile.hpp:8`:
```cpp
inline constexpr int32_t tile_width = 32;
inline constexpr int32_t tile_height = 32;
```

This means the entity position corresponds to the character's **feet** at the center of their tile.

### Smooth Movement Interpolation

During tile transitions (`entity_manager::update_movement()`, line 617):

```cpp
int32_t start_x = tile_x * tile_width + 16;
int32_t start_y = tile_y * tile_height + 16;
int32_t end_x = dest_tile_x * tile_width + 16;
int32_t end_y = dest_tile_y * tile_height + 16;

t.x = start_x + (int32_t)((end_x - start_x) * t.move_progress);
t.y = start_y + (int32_t)((end_y - start_y) * t.move_progress);
```

`move_progress` interpolates from 0.0 to 1.0 over the movement duration:
- Walking: 560ms (8 frames x 70ms)
- Running: 336ms (8 frames x 42ms)

### Camera Following

`world::set_player_position()` centers the camera on the player:

```cpp
target_x = player_world_x - screen_width / 2;
target_y = player_world_y - screen_height / 2;
```

Clamped to map boundaries to prevent viewing outside the map.

### Camera Shake

`world::add_camera_shake()` adds a decaying random offset to `shake_offset_x_`/`shake_offset_y_`, affecting all rendered content (map, entities, objects). The shake decays over `shake_duration_` seconds.

---

## Zoom System

### How Zoom Works

When zoom is enabled (`world::zoom_mode_enabled_`), the world applies an SFML view transform before entity rendering:

`world::apply_zoom_view()` (`world/world.cpp:74`):
```cpp
rend.set_zoom_view(zoom_level, screen_center_x, screen_center_y);
```

`renderer::set_zoom_view()` (`graphics/renderer.cpp:199`):
```cpp
sf::View view = window_.getDefaultView();
view.setCenter({center_x, center_y});
view.zoom(zoom_level);
window_.setView(view);
```

After entities render, `reset_zoom_view()` restores the default view for UI rendering.

### Zoom Range and Interpolation

- Range: 0.5x (zoomed in) to 8.0x (zoomed out)
- `zoom_level_` interpolates smoothly toward `zoom_target_` each frame
- Anchor point: zoom centers on the cursor position for natural zoom-to-cursor behavior
- Internal representation uses `double` precision to prevent jitter at extreme zoom levels

---

## Depth Sorting

### Y-Sort (Painter's Algorithm)

`entity_manager::render()` (`entity/entity_manager.cpp:660`):

```cpp
std::sort(visible_entities.begin(), visible_entities.end(),
    [](const entity* a, const entity* b) {
        return a->transform().y < b->transform().y;
    });
```

- Entities with smaller Y (higher on screen, farther from viewer) draw first
- Entities with larger Y (lower on screen, closer to viewer) draw on top
- This produces correct visual overlap for a 2D top-down perspective

### Complete Visual Stack (Back to Front)

1. **Map terrain** (layer 0) -- `map_renderer`
2. **Map objects** (layer 1) -- `map_renderer`
3. **Entities** (Y-sorted within group) -- `entity_manager`
4. **Debug overlay** -- `debug_stats`
5. **Status log** -- `status_log_`
6. **UI dialogs and widgets** -- `ui_system`

Steps 1-3 render under the zoom view. Steps 4-6 render at native resolution.

### Intra-Entity Layer Order

Within a single character, layers draw back-to-front as described in [Player Character Rendering](#player-character-rendering) (in-game) or [Menu Character Renderer](#menu-character-renderer) (full equipment). The direction-dependent weapon/mantle ordering ensures correct overlap when characters face different directions.

---

## Frustum Culling

`entity_manager::render()` (`entity/entity_manager.cpp:631-658`):

```cpp
static constexpr int32_t render_margin = 128;
int32_t scr_width = static_cast<int32_t>(rend.width());
int32_t scr_height = static_cast<int32_t>(rend.height());

// Skip entities outside screen + margin
if (screen_x >= -render_margin && screen_x < scr_width + render_margin &&
    screen_y >= -render_margin && screen_y < scr_height + render_margin)
```

- **Margin**: 128 pixels on all sides prevents sprite pop-in/pop-out at screen edges (character sprites can extend well beyond their position point due to pivot offsets)
- **Invisible entities**: `sprite().visible == false` entities are skipped before the bounds check (line 641)
- **Global render mode**: When `global_render_mode_` is true (cinematic mode), all entities are included regardless of screen position (line 644)

---

## Entity Overlays

### Entity Name

`render_entity_name()` (`entity/entity_manager.cpp:814`):

- **Shown on hover only** -- `is_hovered` must be true (determined by hit testing)
- **Color coding** by entity type:

| Entity Type | Condition | Color |
|-------------|-----------|-------|
| Player/Character | Default | White |
| Player/Character | PK count > 0 | Red |
| NPC | -- | Yellow |
| Monster | Default | Orange `(255, 128, 0)` |
| Monster | `is_boss == true` | Red |

- **Positioned** below entity feet: `screen_x - name_length * 4`, `screen_y + 10`
- **Guild name** rendered below character name in green `(100, 200, 100)` if present

### Health Bar

`render_entity_health_bar()` (`entity/entity_manager.cpp:859`):

- **Shown for**: other characters (`entity_type::character`) and monsters only
- **Not shown for**: the local player, NPCs, items, effects
- **Position**: `screen_x - 20`, `screen_y - 70` (above the character)
- **Dimensions**: 40x4 pixels
- **Background**: dark gray `(40, 40, 40)` filled rect
- **Fill color** by HP ratio:

| HP Ratio | Color |
|----------|-------|
| >= 60% | Green |
| 30%-60% | Yellow |
| < 30% | Red |

- **Border**: gray `(100, 100, 100)` outline rect

### Chat Bubble

Rendered in `render_entity_name()` (`entity/entity_manager.cpp:847`), independent of hover state:

- **Background**: black with 70% opacity `(0, 0, 0, 180)`
- **Position**: `screen_x - message_length * 3`, `screen_y - 80`
- **Size**: `message_length * 6 + 8` wide, 16 tall
- **Text**: white, standard font size
- **Lifetime**: controlled by `name_component.chat_timer`, auto-clears when timer reaches 0

---

## Hit Testing

`entity_manager::is_point_in_entity_sprite()` (`entity/entity_manager.cpp:318`) determines if a mouse position overlaps an entity's rendered sprite. Used for hover detection and click targeting.

### Hit Test by Entity Type

| Entity Type | Method |
|-------------|--------|
| Items | Simple 32x32 box centered at position |
| Effects | Not clickable (always returns false) |
| NPCs/Monsters | `sprite::get_bounds()` from NPC sprite, fallback to 64x64 box |
| Players | `sprite::get_bounds()` from body sprite, fallback to 64x64 box |

### sprite::get_bounds()

`assets/sprite.cpp:418`:

```cpp
sf::IntRect get_bounds(int32_t x, int32_t y, uint32_t frame) const {
    const auto& f = frames_[frame];
    return sf::IntRect(
        {x + f.pivot_x, y + f.pivot_y},
        {f.source_rect.size.x, f.source_rect.size.y}
    );
}
```

Returns the actual rendered rectangle for a given frame, accounting for pivot offsets. This is more accurate than a fixed bounding box since sprite frames vary in size.

### Click Targeting

`entity_manager::get_entity_at_screen_pos()` (`entity/entity_manager.cpp:289`) uses a simpler distance-based check for click targeting:

- Converts screen to world coordinates
- Finds closest entity within 32-pixel radius
- Skips effects
- Returns the nearest match

---

## Sprite Memory Management

### Overview

`sprite_manager` provides automatic memory management to balance GPU memory usage against loading latency.

### Eviction System

`sprite_manager::update_memory()` (`assets/sprite_manager.cpp:221`):

- Runs every **5 seconds** (`eviction_check_interval_`)
- Checks `seconds_since_use()` for every loaded sprite
- Unloads bitmaps for sprites unused for **30 seconds** (`eviction_timeout_`)
- `sprite::unload_bitmap()` replaces textures with 1x1 placeholders, freeing GPU memory
- Metadata is preserved -- the sprite can be re-loaded on next draw via `ensure_loaded()`

### Memory Tracking

- `sprite::touch()` updates `last_used_` timestamp on every draw
- `sprite::memory_usage()`: returns `2 * width * height * 4` bytes (two textures)
- `sprite_manager::gpu_memory_usage()`: sums across all cached sprites
- `sprite_manager::loaded_bitmap_count()`: count of sprites with active bitmaps

### LRU Cache

`sprite_manager::lru_order_` tracks sprite access order. `shrink_cache()` removes the oldest entries when the cache exceeds a target size. This removes both metadata and bitmap, unlike eviction which only removes bitmaps.

### Lifecycle Summary

```
PAK opened -> Metadata preloaded (fast, no GPU memory)
                -> First draw triggers bitmap load (GPU memory allocated)
                     -> Sprite touched on every draw
                     -> 30s without draw -> bitmap evicted (GPU memory freed)
                          -> Next draw triggers bitmap reload
```

---

## Not Yet Implemented

The following rendering features exist in the menu renderer or as data fields but are not yet active in the in-game rendering pipeline:

| Feature | Status | Location |
|---------|--------|----------|
| Equipment layers (armor, weapon, shield, helmet, mantle, boots, pants) | Fields in `sprite_component`, full implementation in `menu_character_renderer`, TODO at `entity_manager.cpp:765` | `entity/entity_manager.cpp` |
| Direction-dependent weapon/mantle draw order | Implemented in menu renderer only | `graphics/menu_character_renderer.cpp:335` |
| Damage flash (color modulation on hit) | Not present | -- |
| Shadow rendering | `draw_with_shadow()` defined but is a no-op | `graphics/menu_character_renderer.cpp:411` |
| Weapon glow effects | `character_appearance.weapon_glow` field exists, never rendered | `graphics/menu_character_renderer.hpp:30` |
| Shield glow effects | `character_appearance.shield_glow` field exists, never rendered | `graphics/menu_character_renderer.hpp:31` |
| Hair color tinting | `sprite_component.hair_color` stored, never applied during draw | `entity/components.hpp:42` |
| Horizontal flip | `sprite_component.flipped` stored, never applied during draw | `entity/components.hpp:48` |
| Death/corpse transparency | Dying entities hold last frame, no fade-out | -- |
| Palette swapping | No runtime color manipulation for equipment variants | -- |

Equipment color variation is achieved entirely through **separate PAK files** per equipment type (e.g., `"MLArmor"`, `"MCMail"`, `"MSMail"` are distinct armors with pre-colored sprites).

---

## Key Files Reference

| File | Lines | Purpose |
|------|-------|---------|
| `entity/entity_manager.cpp` | 949 | Sprite ID calculation, render pipeline, depth sorting, frustum culling, hit testing, movement interpolation |
| `entity/entity_manager.hpp` | 122 | Entity manager public API, render/update signatures |
| `entity/entity.hpp` | 149 | Entity class, component accessors, visual_type, current_action |
| `entity/components.hpp` | 344 | All component structs: transform, sprite, animation, stats, combat, name, npc, monster, item, effect |
| `graphics/renderer.hpp` | 105 | SFML renderer API: sprite/text draw calls, zoom view, scissor |
| `graphics/renderer.cpp` | 512 | Renderer implementation, SFML draw delegation |
| `graphics/menu_character_renderer.hpp` | 168 | character_appearance struct, equipment sprite constants, draw order arrays |
| `graphics/menu_character_renderer.cpp` | 591 | Full equipment layering, PAK loading tables, per-layer draw functions |
| `assets/sprite.hpp` | 125 | Sprite class: frames, textures, two-tier loading, draw methods |
| `assets/sprite.cpp` | 435 | Sprite loading from PAK/file/data, color key masking, draw implementations, bounds calculation |
| `assets/sprite_manager.hpp` | 159 | Sprite cache, PAK management, ID-based lookup, memory eviction config |
| `assets/sprite_manager.cpp` | 313 | Cache implementation, LRU, eviction, metadata preloading |
| `assets/pak_file.hpp` | 81 | PAK file format reader, sprite_frame_info, pak_sprite_data/metadata structs |
| `core/game_enums.hpp` | 189 | `object_action`, `direction`, `entity_anim_state` enums |
| `core/direction_utils.hpp` | 127 | Direction conversions, calculate_direction, direction_offset |
| `world/world.hpp` | 193 | Camera position, zoom state, shake, coordinate helpers |
| `world/tile.hpp` | 66 | `tile_width`/`tile_height` constants (32x32) |
| `gameplay/game_state.cpp` | ~1000 | `render_playing()` orchestration at line 832 |
