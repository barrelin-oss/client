# Dialog System Quick Reference

## YAML Cheat Sheet

### Minimal Dialog

```yaml
dialogs:
  - id: "my_dialog"
    title: "Title"
    bounds: { x: 100, y: 100, w: 200, h: 150 }
    elements:
      - id: "btn_ok"
        type: button
        bounds: { x: 60, y: 100, w: 80, h: 28 }
        text: "OK"
```

### Dialog Properties

```yaml
modal: false          # Block input to other dialogs
draggable: true       # Drag by title bar
closeable: true       # Show close button
has_border: true      # Draw border
has_title_bar: true   # Show title bar
centered: false       # Center on open
always_on_top: false  # Render above others
```

### Color Format

```yaml
color: { r: 255, g: 200, b: 100, a: 255 }
```

### Element Types

| Type | Key Properties |
|------|----------------|
| `label` | `text`, `text_color`, `font_size` |
| `button` | `text`, `tooltip`, `action`, `action_target` |
| `text_input` | `max_chars`, `password_mode` |
| `progress_bar` | `fill_color`, `background_color`, `show_value_text` |
| `checkbox` | `text` |
| `slider` | `min_value`, `max_value`, `step` |
| `grid` | `grid_cols`, `grid_rows`, `cell_width`, `cell_height` |
| `sprite` | `sprite_pak`, `sprite_index`, `sprite_frame` |
| `panel` | `background_color` |

### Button Actions

```yaml
action: close_self          # Close this dialog
action: toggle_dialog       # Toggle another dialog
action_target: "dialog_id"
action: open_dialog         # Open another dialog
action: close_dialog        # Close another dialog
```

### Sprite Button (Classic Mode)

```yaml
- id: "btn_attack"
  type: button
  bounds: { x: 10, y: 10, w: 40, h: 40 }
  sprite_pak: "GameDialog"
  sprite_index: 6
  sprite_frame: 0
  properties:
    hover_frame: "1"
    pressed_frame: "2"
```

### Custom HUD Elements

```yaml
- id: "hp_bar"
  type: hp_bar  # Rendered by code
  bounds: { x: 20, y: 5, w: 100, h: 15 }
  fill_color: { r: 200, g: 50, b: 50 }
```

Custom types: `hp_bar`, `mp_bar`, `sp_bar`, `exp_bar`, `location_text`, `combat_indicator`, `super_attack`, `custom`

---

## C++ Cheat Sheet

### Create Dialog from YAML

```cpp
auto* dlg = dialog_manager.create_dialog("dialog_id");
dlg->on_button_click("btn_ok", []() { /* handler */ });
dlg->open();
```

### Create Dialog with Builder

```cpp
auto def = dialog_builder::create("my_dialog")
    .title("Title")
    .bounds(100, 100, 200, 150)
    .button("btn_ok", 60, 100, 80, 28, "OK")
    .build();

dialog_manager.register_definition(std::move(def));
```

### Element Access

```cpp
dlg->set_label_text("lbl_id", "New text");
dlg->set_progress("bar_id", 0.75f);
dlg->set_checkbox_checked("chk_id", true);
dlg->set_text_input("txt_id", "value");
dlg->set_slider_value("slider_id", 0.5f);
dlg->set_element_visible("elem_id", false);
dlg->set_element_enabled("elem_id", true);
```

### Callbacks

```cpp
dlg->on_button_click("btn_id", []() { /* click */ });
dlg->on_checkbox_change("chk_id", [](bool v) { /* change */ });
dlg->on_text_change("txt_id", [](std::string_view v) { /* change */ });
dlg->on_slider_change("slider_id", [](float v) { /* change */ });
dlg->on_grid_select("grid_id", [](int32_t r, int32_t c) { /* select */ });
```

### Custom Dialog Subclass

```cpp
class my_dialog : public managed_dialog {
public:
    my_dialog(dialog_definition def) : managed_dialog(std::move(def)) {}

protected:
    void on_open_impl() override { /* on open */ }
    void on_close_impl() override { /* on close */ }
    void on_update_impl(float dt) override { /* every frame */ }
    bool on_custom_render(renderer& r) override {
        // Return true to handle all rendering
        // Return false to let base render standard elements
        return false;
    }
};

// Create
auto* dlg = dialog_manager.create_custom_dialog<my_dialog>("def_id");
```

### Hot Reload

```cpp
dialog_manager.set_hot_reload_enabled(true);
dialog_manager.set_hot_reload_interval(std::chrono::milliseconds(200));
```

### Render Mode

```cpp
dialog_manager.set_default_render_mode(render_mode::modern);
dialog_manager.set_default_render_mode(render_mode::classic);
```

---

## Common Patterns

### Modal Confirmation Dialog

```yaml
dialogs:
  - id: "confirm"
    title: "Confirm"
    bounds: { x: 0, y: 0, w: 260, h: 120 }
    modal: true
    centered: true
    elements:
      - id: "message"
        type: label
        bounds: { x: 20, y: 35, w: 220, h: 40 }
        text: "Are you sure?"
      - id: "btn_yes"
        type: button
        bounds: { x: 30, y: 80, w: 80, h: 28 }
        text: "Yes"
      - id: "btn_no"
        type: button
        bounds: { x: 150, y: 80, w: 80, h: 28 }
        text: "No"
```

```cpp
auto* dlg = dialog_manager.open_dialog("confirm");
dlg->set_label_text("message", "Delete item?");
dlg->on_button_click("btn_yes", [this]() {
    delete_item();
    dialog_manager.close_dialog("confirm");
});
dlg->on_button_click("btn_no", [this]() {
    dialog_manager.close_dialog("confirm");
});
```

### HUD Element (Always on Top)

```yaml
dialogs:
  - id: "minimap"
    title: ""
    bounds: { x: 500, y: 10, w: 130, h: 130 }
    modal: false
    draggable: false
    closeable: false
    has_border: true
    has_title_bar: false
    always_on_top: true
    elements:
      - id: "map_display"
        type: custom
        bounds: { x: 5, y: 5, w: 120, h: 120 }
```

### Form Dialog

```yaml
dialogs:
  - id: "login"
    title: "Login"
    bounds: { x: 0, y: 0, w: 280, h: 180 }
    modal: true
    centered: true
    elements:
      - id: "lbl_user"
        type: label
        bounds: { x: 20, y: 40, w: 80, h: 20 }
        text: "Username:"
      - id: "txt_user"
        type: text_input
        bounds: { x: 100, y: 38, w: 160, h: 24 }
        max_chars: 20
      - id: "lbl_pass"
        type: label
        bounds: { x: 20, y: 75, w: 80, h: 20 }
        text: "Password:"
      - id: "txt_pass"
        type: text_input
        bounds: { x: 100, y: 73, w: 160, h: 24 }
        max_chars: 20
        password_mode: true
      - id: "chk_remember"
        type: checkbox
        bounds: { x: 20, y: 110, w: 150, h: 20 }
        text: "Remember me"
      - id: "btn_login"
        type: button
        bounds: { x: 100, y: 140, w: 80, h: 28 }
        text: "Login"
```
