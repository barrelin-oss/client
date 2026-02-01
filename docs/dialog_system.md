# Helbreath Client Dialog System Documentation

> A flexible, data-driven dialog system supporting both YAML definitions and C++ code configuration, with dual rendering modes (modern programmatic and classic sprite-based).

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [YAML Dialog Definitions](#yaml-dialog-definitions)
4. [Code-Based Dialog Creation](#code-based-dialog-creation)
5. [Render Modes](#render-modes)
6. [Custom Dialog Subclasses](#custom-dialog-subclasses)
7. [Hot Reload](#hot-reload)
8. [Dialog Properties Reference](#dialog-properties-reference)
9. [Element Types Reference](#element-types-reference)
10. [Best Practices](#best-practices)

---

## Overview

The dialog system provides a hybrid approach to UI creation:

- **YAML files** define layout, positions, colors, and static properties
- **C++ code** handles dynamic behavior, callbacks, and complex rendering
- **Hot reload** enables real-time iteration during development
- **Dual render modes** support both modern programmatic and classic sprite-based rendering

### Key Classes

| Class | Purpose |
|-------|---------|
| `dialog_manager` | Central coordinator - loads definitions, creates dialogs |
| `dialog_definition` | Data structure holding a dialog's layout |
| `dialog_builder` | Fluent API for building definitions in C++ |
| `managed_dialog` | Base class for data-driven dialogs |
| `render_strategy` | Interface for rendering (modern vs classic) |

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                       ui_system                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                   dialog_manager                        │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │ │
│  │  │ definitions │  │  dialogs_   │  │dialog_order_│      │ │
│  │  │ (map<id>)   │  │ (map<id>)   │  │ (z-order)   │      │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘      │ │
│  └─────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│                     managed_dialog                           │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────┐   │
│  │ definition_   │  │element_states_│  │render_strategy_ │   │
│  │(layout data)  │  │(runtime state)│  │(modern/classic) │   │
│  └───────────────┘  └───────────────┘  └─────────────────┘   │
│                                                              │
│  Callbacks: button_callbacks_, checkbox_callbacks_, etc.     │
└──────────────────────────────────────────────────────────────┘
```

---

## YAML Dialog Definitions

### File Location

Place YAML files in `assets/ui/dialogs/`. Files are loaded via:

```cpp
dialog_manager.load_definitions_from_directory("assets/ui/dialogs");
```

### Basic Structure

```yaml
dialogs:
  - id: "my_dialog"
    title: "My Dialog Title"
    bounds: { x: 100, y: 100, w: 300, h: 200 }

    # Behavior flags
    modal: false
    draggable: true
    closeable: true
    has_border: true
    has_title_bar: true
    centered: false
    always_on_top: false

    # Visual styling
    background_color: { r: 40, g: 40, b: 50, a: 240 }
    border_color: { r: 100, g: 100, b: 120, a: 255 }
    title_bar_color: { r: 60, g: 60, b: 80, a: 255 }
    title_text_color: { r: 255, g: 255, b: 255, a: 255 }

    # Elements list
    elements:
      - id: "label_1"
        type: label
        bounds: { x: 20, y: 40, w: 260, h: 20 }
        text: "Hello World"
        text_color: { r: 255, g: 255, b: 200 }
```

### Color Format

Colors use RGBA components (0-255):

```yaml
# Full format
text_color: { r: 255, g: 200, b: 100, a: 255 }

# Alpha defaults to 255 if omitted
text_color: { r: 255, g: 200, b: 100 }
```

### Bounds Format

```yaml
bounds: { x: 100, y: 100, w: 300, h: 200 }
# x, y = position (from top-left)
# w, h = width, height
```

---

## Code-Based Dialog Creation

### Using dialog_builder (Fluent API)

```cpp
#include "ui/dialog_builder.hpp"

auto def = dialog_builder::create("confirm_dialog")
    .title("Confirm Action")
    .bounds(200, 150, 280, 140)
    .modal(true)
    .draggable(true)
    .closeable(true)
    .background(sf::Color(40, 40, 50, 240))

    // Add elements
    .label("lbl_message", 20, 40, "Are you sure?", sf::Color::White)
    .button("btn_yes", 40, 90, 80, 28, "Yes")
    .button("btn_no", 160, 90, 80, 28, "No")

    .build();

// Register with dialog_manager
dialog_manager.register_definition(std::move(def));
```

### Creating and Using Dialogs

```cpp
// Create dialog from definition
auto* dlg = dialog_manager.create_dialog("confirm_dialog");

// Set up callbacks
dlg->on_button_click("btn_yes", [this]() {
    perform_action();
    dialog_manager.close_dialog("confirm_dialog");
});

dlg->on_button_click("btn_no", [this]() {
    dialog_manager.close_dialog("confirm_dialog");
});

// Open the dialog
dlg->open();
```

### Element Access Methods

```cpp
// Labels
dlg->set_label_text("lbl_status", "Loading...");

// Progress bars
dlg->set_progress("progress_bar", 0.75f);  // 0.0 to 1.0

// Checkboxes
dlg->set_checkbox_checked("chk_option", true);
bool checked = dlg->get_checkbox_checked("chk_option");

// Text inputs
dlg->set_text_input("txt_name", "Default value");
std::string_view text = dlg->get_text_input("txt_name");

// Sliders
dlg->set_slider_value("slider_volume", 0.5f);
float value = dlg->get_slider_value("slider_volume");

// Visibility and enabled state
dlg->set_element_visible("panel_advanced", false);
dlg->set_element_enabled("btn_submit", true);
```

---

## Render Modes

The dialog system supports two rendering modes:

### Modern Mode (Programmatic)

Uses SFML primitives and solid colors. Good for development and modern UI aesthetics.

```cpp
dialog_manager.set_default_render_mode(render_mode::modern);
```

### Classic Mode (Sprite-Based)

Uses original Helbreath PAK sprites for authentic appearance.

```cpp
dialog_manager.set_default_render_mode(render_mode::classic);
```

### Sprite References in YAML

For classic mode, elements can reference PAK sprites:

```yaml
elements:
  - id: "btn_ok"
    type: button
    bounds: { x: 100, y: 50, w: 80, h: 30 }
    text: "OK"
    sprite_pak: "GameDialog"      # PAK file name
    sprite_index: 6               # Sprite index in PAK
    sprite_frame: 0               # Frame for normal state
    properties:
      hover_frame: "1"            # Frame for hover state
      pressed_frame: "2"          # Frame for pressed state
```

### Dialog Background Sprites

```yaml
dialogs:
  - id: "inventory"
    # ... other properties ...

    background_sprite:
      pak: "GameDialog"
      index: 10
      frame: 0
```

---

## Custom Dialog Subclasses

For complex dialogs requiring custom rendering or logic, subclass `managed_dialog`:

### Header

```cpp
#pragma once
#include "ui/managed_dialog.hpp"

namespace hb {

class my_custom_dialog : public managed_dialog {
public:
    explicit my_custom_dialog(dialog_definition def);

    // Public API for this dialog
    void set_player_stats(int hp, int mp);

protected:
    // Override points
    void on_open_impl() override;
    void on_close_impl() override;
    void on_update_impl(float delta_time) override;
    bool on_custom_render(renderer& rend) override;
    bool on_custom_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

private:
    int hp_ = 100;
    int mp_ = 50;
};

} // namespace hb
```

### Implementation

```cpp
#include "my_custom_dialog.hpp"

namespace hb {

my_custom_dialog::my_custom_dialog(dialog_definition def)
    : managed_dialog(std::move(def))
{
    // Initialize callbacks for YAML-defined buttons
    on_button_click("btn_close", [this]() { close(); });
}

void my_custom_dialog::on_open_impl() {
    // Called when dialog opens
    refresh_data();
}

void my_custom_dialog::on_update_impl(float delta_time) {
    // Called every frame
    update_animations(delta_time);
}

bool my_custom_dialog::on_custom_render(renderer& rend) {
    // Custom rendering for complex elements
    // Return true if you handle ALL rendering
    // Return false to let base class render standard elements

    // Get bounds from YAML-defined element for positioning
    if (auto* elem = definition().find_element("hp_display")) {
        auto bounds = get_absolute_bounds(*elem);
        render_custom_hp_bar(rend, bounds);
    }

    return false;  // Let base class render other elements
}

} // namespace hb
```

### Creating Custom Dialogs

```cpp
// Use template method to create custom dialog
auto* dlg = dialog_manager.create_custom_dialog<my_custom_dialog>("my_dialog_def");
dlg->set_player_stats(100, 50);
dlg->open();
```

### Custom HUD Element Types

For complex HUD elements, define custom element types in YAML that are rendered by code:

```yaml
elements:
  - id: "hp_bar"
    type: hp_bar           # Custom type - rendered by subclass
    bounds: { x: 23, y: 3, w: 101, h: 18 }
    fill_color: { r: 200, g: 50, b: 50 }
    properties:
      show_poison_effect: "true"
```

Available custom element types:
- `custom` - Generic custom element
- `hp_bar` - HP bar with poison support
- `mp_bar` - MP bar
- `sp_bar` - Stamina bar
- `exp_bar` - Experience bar
- `location_text` - Map name and coordinates
- `combat_indicator` - Safe/PK mode indicator
- `super_attack` - Super attack counter

---

## Hot Reload

During development, enable hot reload to see YAML changes in real-time:

### Enable Hot Reload

```cpp
dialog_manager.set_hot_reload_enabled(true);
dialog_manager.set_hot_reload_interval(std::chrono::milliseconds(200));
```

### How It Works

1. The dialog manager polls YAML files for changes
2. When a file modification is detected, definitions are reloaded
3. Existing dialogs receive updated definitions via `update_definition()`
4. Dialog state (callbacks, values) is preserved during hot reload

### Manual Reload

```cpp
dialog_manager.reload_definitions();
```

### Important Notes

- Hot reload preserves dialog state (HP values, checkbox states, etc.)
- Callbacks registered in code are NOT lost during hot reload
- Only layout and visual properties are updated
- Position changes take effect immediately

---

## Dialog Properties Reference

### Behavior Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `modal` | bool | false | Blocks input to dialogs behind it |
| `draggable` | bool | true | Can be dragged by title bar |
| `closeable` | bool | true | Shows close button in title bar |
| `has_border` | bool | true | Draws border around dialog |
| `has_title_bar` | bool | true | Shows title bar at top |
| `centered` | bool | false | Centers dialog on screen when opened |
| `always_on_top` | bool | false | Always renders above other dialogs |

### always_on_top Behavior

When `always_on_top: true`:
- Dialog renders after all non-always-on-top dialogs
- Multiple always-on-top dialogs maintain their relative z-order
- Useful for HUD elements (icon panel, gauges, chat)
- Still respects modal dialogs if one is open

```yaml
dialogs:
  - id: "icon_panel"
    always_on_top: true
    # HUD elements should typically be always on top
```

### centered Behavior

When `centered: true`:
- Dialog is centered on screen when `open()` is called
- The `bounds.x` and `bounds.y` in YAML are ignored
- Only `bounds.w` and `bounds.h` are used

```yaml
dialogs:
  - id: "error_message"
    centered: true
    bounds: { x: 0, y: 0, w: 300, h: 150 }  # x,y ignored
```

---

## Element Types Reference

### label

Static text display.

```yaml
- id: "lbl_title"
  type: label
  bounds: { x: 20, y: 10, w: 200, h: 20 }
  text: "Welcome!"
  text_color: { r: 255, g: 255, b: 200 }
  font_size: 14
```

### button

Clickable button with hover/press states.

```yaml
- id: "btn_ok"
  type: button
  bounds: { x: 100, y: 150, w: 80, h: 28 }
  text: "OK"
  tooltip: "Click to confirm"

  # Optional action (alternative to code callbacks)
  action: close_self
  # or
  action: toggle_dialog
  action_target: "other_dialog_id"
```

Button actions:
- `none` - No action (use code callback)
- `open_dialog` - Opens `action_target` dialog
- `close_dialog` - Closes `action_target` dialog
- `close_self` - Closes the containing dialog
- `toggle_dialog` - Toggles `action_target` open/closed

### text_input

Text entry field.

```yaml
- id: "txt_username"
  type: text_input
  bounds: { x: 80, y: 50, w: 150, h: 24 }
  max_chars: 20
  password_mode: false
```

### progress_bar

Progress/gauge bar.

```yaml
- id: "progress_hp"
  type: progress_bar
  bounds: { x: 20, y: 80, w: 200, h: 16 }
  fill_color: { r: 200, g: 50, b: 50 }
  background_color: { r: 60, g: 20, b: 20 }
  show_value_text: true
```

### checkbox

Toggle checkbox with label.

```yaml
- id: "chk_sound"
  type: checkbox
  bounds: { x: 20, y: 100, w: 150, h: 20 }
  text: "Enable Sound"
```

### slider

Value slider.

```yaml
- id: "slider_volume"
  type: slider
  bounds: { x: 20, y: 130, w: 200, h: 20 }
  min_value: 0.0
  max_value: 1.0
  step: 0.05
```

### grid

Grid of cells (for inventory-style layouts).

```yaml
- id: "inventory_grid"
  type: grid
  bounds: { x: 10, y: 40, w: 280, h: 160 }
  grid_cols: 5
  grid_rows: 4
  cell_width: 52
  cell_height: 36
  cell_padding: 4
```

### sprite

Static sprite from PAK file.

```yaml
- id: "icon_gold"
  type: sprite
  bounds: { x: 200, y: 20, w: 32, h: 32 }
  sprite_pak: "Hitem"
  sprite_index: 100
  sprite_frame: 0
```

### panel

Container panel with background.

```yaml
- id: "panel_stats"
  type: panel
  bounds: { x: 10, y: 30, w: 180, h: 100 }
  background_color: { r: 30, g: 30, b: 40, a: 200 }
```

### separator

Visual separator line.

```yaml
- id: "sep_1"
  type: separator
  bounds: { x: 10, y: 120, w: 280, h: 2 }
```

---

## Best Practices

### 1. Use YAML for Layout, Code for Logic

```yaml
# YAML defines WHERE things are
elements:
  - id: "btn_attack"
    type: button
    bounds: { x: 100, y: 50, w: 80, h: 30 }
```

```cpp
// Code defines WHAT happens
dlg->on_button_click("btn_attack", [this]() {
    perform_attack();
});
```

### 2. Use Custom Element Types for Complex HUD

When standard element types aren't enough, use custom types:

```yaml
- id: "minimap"
  type: custom  # Rendered entirely by code
  bounds: { x: 500, y: 10, w: 120, h: 120 }
```

### 3. Organize YAML Files by Feature

```
assets/ui/dialogs/
├── common/
│   ├── message_box.yaml
│   └── confirm_dialog.yaml
├── gameplay/
│   ├── inventory.yaml
│   ├── spellbook.yaml
│   └── skills.yaml
├── hud/
│   ├── icon_panel.yaml
│   └── gauge_panel.yaml
└── menus/
    ├── main_menu.yaml
    └── options.yaml
```

### 4. Use Properties for Custom Data

For data that doesn't fit standard fields, use `properties`:

```yaml
- id: "btn_skill"
  type: button
  bounds: { x: 10, y: 10, w: 40, h: 40 }
  properties:
    skill_id: "123"
    cooldown_seconds: "5"
    tier: "advanced"
```

Access in code:
```cpp
auto* elem = definition().find_element("btn_skill");
std::string skill_id = elem->properties.at("skill_id");
```

### 5. Use always_on_top Sparingly

Only use `always_on_top` for:
- Main HUD elements (icon panel, gauges)
- Chat window
- Critical notifications

Avoid for:
- Normal game dialogs
- Temporary popups

### 6. Test Both Render Modes

If supporting classic mode, test with both:

```cpp
// Toggle during development
if (input.is_key_pressed(sf::Keyboard::F11)) {
    auto mode = dialog_manager.default_render_mode();
    dialog_manager.set_default_render_mode(
        mode == render_mode::modern ? render_mode::classic : render_mode::modern
    );
}
```

---

## Integration with ui_system

The dialog manager integrates with the existing `ui_system`:

```cpp
class ui_system {
public:
    // Access dialog manager
    dialog_manager& dialogs();

    // Load YAML definitions
    void load_dialog_definitions(const std::filesystem::path& path);
    void load_dialog_definitions_from_directory(const std::filesystem::path& dir);

private:
    std::unique_ptr<dialog_manager> dialog_manager_;
};
```

### Initialization Example

```cpp
void game::initialize() {
    // Initialize UI system
    ui_system_.initialize();

    // Set up dialog manager
    ui_system_.dialogs().initialize(sprite_manager_);
    ui_system_.dialogs().set_default_render_mode(render_mode::classic);
    ui_system_.dialogs().set_hot_reload_enabled(true);

    // Load dialog definitions
    ui_system_.load_dialog_definitions_from_directory("assets/ui/dialogs");

    // Create HUD dialogs
    ui_system_.create_icon_panel_dialog();
}
```

---

## Complete YAML Example

```yaml
# assets/ui/dialogs/example_dialog.yaml

dialogs:
  - id: "shop_dialog"
    title: "General Store"
    bounds: { x: 150, y: 100, w: 340, h: 280 }
    modal: true
    draggable: true
    closeable: true
    has_border: true
    has_title_bar: true
    centered: false
    always_on_top: false

    background_color: { r: 35, g: 35, b: 45, a: 245 }
    border_color: { r: 80, g: 80, b: 100 }
    title_bar_color: { r: 50, g: 50, b: 70 }

    # For classic mode
    background_sprite:
      pak: "GameDialog"
      index: 15
      frame: 0

    elements:
      # Header
      - id: "lbl_gold"
        type: label
        bounds: { x: 240, y: 8, w: 90, h: 16 }
        text: "Gold: 0"
        text_color: { r: 255, g: 215, b: 0 }
        font_size: 12

      # Item grid
      - id: "shop_grid"
        type: grid
        bounds: { x: 10, y: 40, w: 320, h: 180 }
        grid_cols: 5
        grid_rows: 4
        cell_width: 60
        cell_height: 42
        cell_padding: 4
        background_color: { r: 25, g: 25, b: 35 }

      # Selected item info
      - id: "panel_info"
        type: panel
        bounds: { x: 10, y: 225, w: 220, h: 45 }
        background_color: { r: 45, g: 45, b: 55 }

      - id: "lbl_item_name"
        type: label
        bounds: { x: 15, y: 230, w: 210, h: 16 }
        text: "Select an item"
        text_color: { r: 200, g: 200, b: 220 }

      - id: "lbl_item_price"
        type: label
        bounds: { x: 15, y: 250, w: 210, h: 16 }
        text: ""
        text_color: { r: 255, g: 215, b: 0 }

      # Buttons
      - id: "btn_buy"
        type: button
        bounds: { x: 240, y: 230, w: 90, h: 32 }
        text: "Buy"
        tooltip: "Purchase selected item"

      - id: "btn_close"
        type: button
        bounds: { x: 240, y: 265, w: 90, h: 24 }
        text: "Close"
        action: close_self
```
