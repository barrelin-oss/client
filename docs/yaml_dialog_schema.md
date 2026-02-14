# YAML Dialog System Schema

Complete specification for the data-driven dialog system. Dialogs are defined in YAML (or JSON) files and loaded at runtime with optional hot-reload.

---

## File Structure

```yaml
# Root level: array of dialog definitions
dialogs:
  - id: "my_dialog"
    # ... dialog properties
    elements:
      - id: "my_element"
        # ... element properties

# Optional: custom top-level keys for specialized dialog subclasses
# (e.g., help_config, help_topics, icon_panel_config)
```

Files are loaded from `assets/ui/dialogs/` with extensions `.yaml`, `.yml`, or `.json`.

---

## Dialog Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `id` | string | - | **yes** | Unique identifier, used for lookup and targeting |
| `title` | string | `""` | no | Title bar text |
| `bounds` | bounds | `{0,0,200,150}` | no | Position and size |
| `modal` | bool | `false` | no | Blocks input to dialogs behind it |
| `draggable` | bool | `true` | no | Can be dragged by title bar |
| `closeable` | bool | `true` | no | Shows close button in title bar |
| `has_border` | bool | `true` | no | Draws border around dialog |
| `has_title_bar` | bool | `true` | no | Shows title bar at top |
| `centered` | bool | `false` | no | Centers on screen when opened (ignores bounds x,y) |
| `always_on_top` | bool | `false` | no | Renders above non-always-on-top dialogs |
| `background_color` | color | none | no | Solid background fill |
| `border_color` | color | none | no | Border color |
| `title_bar_color` | color | none | no | Title bar background |
| `title_text_color` | color | none | no | Title bar text color |
| `background_sprite` | sprite_ref | none | no | Sprite-based background (classic mode) |
| `elements` | element[] | `[]` | no | Child element definitions |

### Background Sprite

```yaml
background_sprite:
  pak: "GameDialog"    # PAK file name
  index: 6             # Sprite index within PAK
  frame: 14            # Frame number within sprite
```

---

## Common Types

### Bounds

Position relative to dialog origin, size in pixels.

```yaml
bounds: { x: 20, y: 10, w: 200, h: 30 }
```

| Field | Type | Description |
|-------|------|-------------|
| `x` | int32 | Horizontal offset from dialog left edge |
| `y` | int32 | Vertical offset from dialog top edge |
| `w` | int32 | Width in pixels |
| `h` | int32 | Height in pixels |

Negative values are allowed (e.g., `y: -5` to position above the dialog).

### Color

RGBA components, 0-255 each. Alpha defaults to 255 if omitted.

```yaml
text_color: { r: 255, g: 200, b: 100, a: 255 }
fill_color: { r: 200, g: 50, b: 50 }  # a defaults to 255
```

| Field | Type | Default | Range |
|-------|------|---------|-------|
| `r` | int | 255 | 0-255 |
| `g` | int | 255 | 0-255 |
| `b` | int | 255 | 0-255 |
| `a` | int | 255 | 0-255 |

---

## Element Types

Every element requires `id` (string) and `type` (string). All elements accept `bounds`.

### Universal Element Properties

These are parsed for **every** element type, regardless of whether the type uses them:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | - | **Required.** Unique within the dialog. |
| `type` | string | `"label"` | Element type (see types below) |
| `bounds` | bounds | `{0,0,0,0}` | Position and size relative to dialog |
| `text` | string | `""` | Text content |
| `text_color` | color | none | Text color |
| `font_size` | uint32 | none | Font size in pixels (uses default if unset) |
| `background_color` | color | none | Background fill |
| `border_color` | color | none | Border/outline |
| `hover_color` | color | none | Background when hovered |
| `pressed_color` | color | none | Background when pressed |
| `fill_color` | color | none | Fill color (progress bars, sliders) |
| `sprite_pak` | string | `""` | PAK file name for sprite rendering |
| `sprite_index` | int32 | 0 | Sprite index within PAK |
| `sprite_frame` | int32 | 0 | Frame number within sprite |
| `tooltip` | string | `""` | Text shown on hover |
| `properties` | map | `{}` | Arbitrary string key-value pairs |
| `action` | action_type | `"none"` | Button action (see Actions) |
| `action_target` | string | `""` | Target dialog ID for action |
| `max_chars` | int32 | 256 | Max characters (text_input) |
| `password_mode` | bool | `false` | Mask input as dots (text_input) |
| `grid_cols` | int32 | 1 | Column count (grid) |
| `grid_rows` | int32 | 1 | Row count (grid) |
| `cell_width` | int32 | 32 | Cell width in pixels (grid) |
| `cell_height` | int32 | 32 | Cell height in pixels (grid) |
| `cell_padding` | int32 | 2 | Cell spacing in pixels (grid) |
| `show_value_text` | bool | `false` | Show percentage text (progress_bar) |
| `min_value` | float | 0.0 | Minimum value (slider) |
| `max_value` | float | 1.0 | Maximum value (slider) |
| `step` | float | 0.01 | Value increment (slider) |

> **Note:** All properties are parsed into a flat `element_def` struct. Properties irrelevant to the element type are simply ignored at render time.

---

### `label` - Static Text

Displays non-interactive text.

```yaml
- id: "lbl_title"
  type: label
  bounds: { x: 20, y: 10, w: 200, h: 20 }
  text: "Hello World"
  text_color: { r: 255, g: 255, b: 200 }
  font_size: 14
```

**Relevant properties:** `text`, `text_color`, `font_size`

---

### `button` - Clickable Button

Interactive button with hover/press states. Supports both solid-color and sprite-based rendering.

```yaml
# Solid-color button
- id: "btn_ok"
  type: button
  bounds: { x: 100, y: 150, w: 80, h: 28 }
  text: "OK"
  background_color: { r: 60, g: 100, b: 60 }
  hover_color: { r: 80, g: 130, b: 80 }
  pressed_color: { r: 50, g: 80, b: 50 }
  tooltip: "Confirm action"
  action: close_self

# Sprite-based button
- id: "btn_inventory"
  type: button
  bounds: { x: 449, y: 7, w: 37, h: 41 }
  text: "I"
  tooltip: "Inventory (F6)"
  sprite_pak: "GameDialog"
  sprite_index: 6
  sprite_frame: 7
  properties:
    hover_frame: "16"
    pressed_frame: "2"
    hotkey: "F6"
    button_index: "1"
```

**Relevant properties:** `text`, `text_color`, `background_color`, `hover_color`, `pressed_color`, `disabled_color`, `tooltip`, `action`, `action_target`, `sprite_pak`, `sprite_index`, `sprite_frame`

**Custom properties:**
| Key | Description |
|-----|-------------|
| `hover_frame` | Sprite frame to show on hover |
| `pressed_frame` | Sprite frame to show when pressed |
| `hotkey` | Keyboard shortcut label (display only) |
| `button_index` | Numeric index for code-based identification |

---

### `text_input` - Text Entry Field

Editable single-line text field.

```yaml
- id: "txt_username"
  type: text_input
  bounds: { x: 80, y: 50, w: 150, h: 24 }
  text: "default value"
  text_color: { r: 255, g: 255, b: 255 }
  background_color: { r: 30, g: 30, b: 40 }
  max_chars: 20
  password_mode: false
```

**Relevant properties:** `text`, `text_color`, `background_color`, `max_chars`, `password_mode`

---

### `image` - Static Image/Sprite

Displays a sprite from a PAK file (functionally same as `sprite`).

```yaml
- id: "img_background"
  type: image
  bounds: { x: 0, y: 0, w: 300, h: 200 }
  sprite_pak: "GameDialog"
  sprite_index: 10
  sprite_frame: 0
```

**Relevant properties:** `sprite_pak`, `sprite_index`, `sprite_frame`

---

### `sprite` - PAK Sprite

Displays a sprite from a PAK file.

```yaml
- id: "icon_gold"
  type: sprite
  bounds: { x: 200, y: 20, w: 32, h: 32 }
  sprite_pak: "Hitem"
  sprite_index: 100
  sprite_frame: 0
```

**Relevant properties:** `sprite_pak`, `sprite_index`, `sprite_frame`

---

### `progress_bar` - Gauge/Progress Bar

Horizontal fill bar showing a 0.0-1.0 value.

```yaml
- id: "bar_hp"
  type: progress_bar
  bounds: { x: 20, y: 80, w: 200, h: 16 }
  fill_color: { r: 200, g: 50, b: 50 }
  background_color: { r: 60, g: 20, b: 20 }
  show_value_text: true
```

**Relevant properties:** `fill_color`, `background_color`, `show_value_text`

---

### `checkbox` - Toggle Checkbox

Checkbox with adjacent label text.

```yaml
- id: "chk_sound"
  type: checkbox
  bounds: { x: 20, y: 100, w: 16, h: 16 }
  text: "Enable Sound"
  text_color: { r: 200, g: 200, b: 220 }
```

**Relevant properties:** `text`, `text_color`

---

### `slider` - Value Slider

Horizontal slider for float values.

```yaml
- id: "slider_volume"
  type: slider
  bounds: { x: 120, y: 70, w: 180, h: 20 }
  min_value: 0.0
  max_value: 1.0
  step: 0.05
  background_color: { r: 60, g: 60, b: 80 }
  fill_color: { r: 100, g: 100, b: 200 }
```

**Relevant properties:** `min_value`, `max_value`, `step`, `background_color`, `fill_color`

---

### `list_box` - Scrollable List

Scrollable list of selectable items. Items are populated from code.

```yaml
- id: "quest_list"
  type: list_box
  bounds: { x: 10, y: 35, w: 330, h: 180 }
  background_color: { r: 25, g: 25, b: 38, a: 200 }
  border_color: { r: 60, g: 60, b: 80 }
```

**Relevant properties:** `background_color`, `border_color`

---

### `grid` - Cell Grid

Grid of cells (used for inventory, skill slots, etc.). Content populated from code.

```yaml
- id: "inventory_grid"
  type: grid
  bounds: { x: 10, y: 40, w: 280, h: 160 }
  grid_cols: 5
  grid_rows: 4
  cell_width: 52
  cell_height: 36
  cell_padding: 4
  background_color: { r: 25, g: 25, b: 35 }
  border_color: { r: 60, g: 60, b: 80 }
```

**Relevant properties:** `grid_cols`, `grid_rows`, `cell_width`, `cell_height`, `cell_padding`, `background_color`, `border_color`

---

### `panel` - Container Panel

Visual container with background. Does not manage child layout - children are separate elements in the flat `elements` array.

```yaml
- id: "panel_stats"
  type: panel
  bounds: { x: 10, y: 30, w: 180, h: 100 }
  background_color: { r: 30, g: 30, b: 40, a: 200 }
  border_color: { r: 60, g: 60, b: 80 }
```

**Relevant properties:** `background_color`, `border_color`

---

### `separator` - Visual Line

Horizontal or vertical divider line.

```yaml
- id: "sep_1"
  type: separator
  bounds: { x: 10, y: 120, w: 280, h: 1 }
```

**Relevant properties:** `background_color` (line color; uses default if unset)

---

### `custom` - Generic Custom Element

Placeholder positioned by YAML, rendered entirely by code subclass.

```yaml
- id: "custom_widget"
  type: custom
  bounds: { x: 100, y: 100, w: 200, h: 150 }
  properties:
    custom_data: "value"
```

---

## HUD Element Types

These are specialized element types rendered by code. YAML defines their position and configuration; rendering logic is in C++ subclasses.

### `hp_bar` - Health Bar

```yaml
- id: "hp_bar"
  type: hp_bar
  bounds: { x: 23, y: 9, w: 101, h: 18 }
  fill_color: { r: 200, g: 50, b: 50 }
  background_color: { r: 60, g: 20, b: 20 }
  properties:
    classic_sprite_frame: "12"
```

### `mp_bar` - Mana Bar

```yaml
- id: "mp_bar"
  type: mp_bar
  bounds: { x: 23, y: 30, w: 101, h: 18 }
  fill_color: { r: 50, g: 100, b: 200 }
  background_color: { r: 20, g: 40, b: 80 }
  properties:
    classic_sprite_frame: "12"
```

### `sp_bar` - Stamina Bar

```yaml
- id: "sp_bar"
  type: sp_bar
  bounds: { x: 147, y: 9, w: 167, h: 12 }
  fill_color: { r: 200, g: 180, b: 50 }
  background_color: { r: 80, g: 70, b: 20 }
  properties:
    classic_sprite_frame: "13"
```

### `exp_bar` - Experience Bar

```yaml
- id: "exp_bar"
  type: exp_bar
  bounds: { x: 0, y: -5, w: 640, h: 5 }
  fill_color: { r: 100, g: 200, b: 100 }
  background_color: { r: 30, g: 60, b: 30 }
```

### `location_text` - Map/Coordinates Display

```yaml
- id: "location_text"
  type: location_text
  bounds: { x: 140, y: 16, w: 183, h: 16 }
  text_color: { r: 200, g: 200, b: 120 }
  font_size: 11
```

### `combat_indicator` - Safe/PK Mode

```yaml
- id: "combat_indicator"
  type: combat_indicator
  bounds: { x: 368, y: 13, w: 30, h: 30 }
  properties:
    safe_frame: "4"
    pk_frame: "5"
    animation_frame: "3"
```

### `super_attack` - Super Attack Counter

```yaml
- id: "super_attack"
  type: super_attack
  bounds: { x: 362, y: 0, w: 40, h: 40 }
```

---

## Button Actions

Buttons can trigger automatic dialog operations without code callbacks.

```yaml
action: close_self
# or
action: open_dialog
action_target: "help_dialog"
```

| Action | Description |
|--------|-------------|
| `none` | No automatic action (default). Use code callback. |
| `open_dialog` | Open the dialog specified by `action_target` |
| `close_dialog` | Close the dialog specified by `action_target` |
| `close_self` | Close the dialog containing this button |
| `toggle_dialog` | Toggle open/closed the dialog specified by `action_target` |

---

## Custom Properties

Any element can carry arbitrary string key-value pairs via `properties`. These are accessible from code but have no built-in behavior.

```yaml
properties:
  skill_id: "123"
  cooldown_seconds: "5"
  custom_key: "custom_value"
```

All values are stored as strings. Code accesses them via `element_def::properties["key"]`.

---

## Specialized Top-Level Keys

Some YAML files include extra top-level keys consumed by dialog subclasses. These are **not** part of the standard `dialogs` array.

### `help_config`

Configuration for the help dialog subclass.

```yaml
help_config:
  visible_topics: 8
  visible_content_lines: 12
  topic_row_height: 20
  content_line_height: 16
  categories:
    - name: "All"
      id: -1
    - name: "Basics"
      id: 0
```

### `help_topics`

Content for the help dialog.

```yaml
help_topics:
  - title: "Getting Started"
    category: 0
    content:
      - "#Heading text (rendered in tan)"
      - "Normal text (light gray)"
      - "* Bullet point (green)"
      - "! Warning text (red)"
      - "> Tip/info text (blue)"
      - "- Sub-item (dimmer)"
      - "@ Keyboard shortcut (cyan)"
```

Content line prefixes:
| Prefix | Color | Purpose |
|--------|-------|---------|
| `#` | Tan | Section heading |
| `*` | Green | Bullet point |
| `!` | Red | Warning/important |
| `>` | Blue | Tip/info |
| `-` | Dim gray | Sub-bullet/secondary |
| `@` | Cyan | Keyboard shortcut |
| (none) | Light gray | Normal text |

### `icon_panel_config`

Layout constants for the icon panel (reference only, consumed by code).

```yaml
icon_panel_config:
  panel_height: 53
  panel_y: 434
  classic_sprites:
    pak_name: "GameDialog"
    sprite_index: 6
    panel_background: 14
    hover_highlight: 16
```

---

## Hot Reload

When enabled (debug builds by default), the dialog manager polls YAML files for changes every 200ms and reloads modified definitions. Hot reload preserves:

- Dialog open/closed state
- Element values (text, progress, selections)
- Code-registered callbacks

Only layout and visual properties are updated.

---

## Complete Example

```yaml
dialogs:
  - id: "my_dialog"
    title: "Example Dialog"
    bounds: { x: 0, y: 0, w: 300, h: 250 }
    centered: true
    modal: true
    draggable: true
    closeable: true
    has_border: true
    has_title_bar: true
    background_color: { r: 32, g: 32, b: 48, a: 240 }
    border_color: { r: 80, g: 80, b: 100 }
    title_bar_color: { r: 50, g: 50, b: 70 }

    elements:
      - id: "lbl_prompt"
        type: label
        bounds: { x: 20, y: 35, w: 260, h: 20 }
        text: "Enter your name:"
        text_color: { r: 200, g: 200, b: 220 }

      - id: "txt_name"
        type: text_input
        bounds: { x: 20, y: 60, w: 260, h: 28 }
        max_chars: 32

      - id: "chk_remember"
        type: checkbox
        bounds: { x: 20, y: 100, w: 16, h: 16 }
        text: "Remember me"
        text_color: { r: 180, g: 180, b: 200 }

      - id: "slider_opacity"
        type: slider
        bounds: { x: 20, y: 130, w: 200, h: 20 }
        min_value: 0.0
        max_value: 1.0
        step: 0.1

      - id: "sep"
        type: separator
        bounds: { x: 15, y: 165, w: 270, h: 1 }

      - id: "btn_ok"
        type: button
        bounds: { x: 60, y: 180, w: 80, h: 30 }
        text: "OK"
        background_color: { r: 60, g: 100, b: 60 }
        hover_color: { r: 80, g: 130, b: 80 }
        action: close_self

      - id: "btn_cancel"
        type: button
        bounds: { x: 160, y: 180, w: 80, h: 30 }
        text: "Cancel"
        background_color: { r: 100, g: 60, b: 60 }
        hover_color: { r: 130, g: 80, b: 80 }
        action: close_self
```
