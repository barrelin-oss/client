# Helbreath Client C++20 Modernization - Phase 3 Plan

> **Phase 1 Status**: COMPLETE - Infrastructure (SFML, CMake, subsystems)
> **Phase 2 Status**: COMPLETE - Core game logic (17,623 LOC, 93 files, 0 warnings)
> **Phase 3 Goal**: Complete porting, enable server testing, final polish

## Current State Summary
- **Modern code**: 93 files, 17,623 lines of C++20
- **Legacy code**: 68 files (~2.7 MB), Game.cpp = 48,500 lines
- **Build**: Release builds with zero warnings

## What's Implemented
| System | Status | Notes |
|--------|--------|-------|
| Network protocol | ✅ Complete | 100+ handlers, encryption, dual connections |
| Combat system | ✅ Complete | Stat formulas, weapon skills, super attacks |
| Magic system | ✅ Complete | 100+ spells, casting state machine |
| Inventory/Equipment | ✅ Complete | 50 slots + 15 equipped + 120 bank |
| Entity system | ✅ Complete | Component-based architecture |
| UI framework | ✅ Complete | 15 dialogs implemented |
| World/Map | ✅ Complete | Tiles, camera, rendering |
| PAK loading | ⚠️ Partial | File reading works, bitmap decoding incomplete |
| Skills | ⚠️ Partial | Interface complete, implementation needs verification |
| Audio | ⚠️ Partial | SFML interface exists |

## What's Missing
| System | Gap | Priority |
|--------|-----|----------|
| UI Dialogs | 26 of 41 missing | Critical |
| PAK bitmap decoding | 16-bit DIB → SFML texture | Critical |
| Login/character flow | Create account, select char, create char | Critical |
| Effects system | Spell effects, damage numbers | High |
| Guild system | Full management logic | Medium |
| Quest system | Not implemented | Medium |
| Crusade/War | Not implemented | Low |

---

## NEW: Dialog Manager System Architecture

> **Goal**: Create a flexible dialog system supporting both classic sprite-based and modern programmatic rendering, with fluent API for code + YAML for rapid iteration.

### Design Requirements (User-Specified)
- **Migration Strategy**: Gradual - existing dialogs remain, new system available for new dialogs
- **Layout Definition**: Fluent API in C++ + YAML files for tweaking values during development
- **Custom Logic**: Both event callbacks (simple dialogs) AND subclassing (complex dialogs)

### Core Components

#### 1. `dialog_definition` - Layout Data Structure
```cpp
// src/ui/dialog_definition.hpp
namespace hb {

enum class element_type {
    label, button, text_input, image, progress_bar,
    checkbox, slider, list_box, grid, sprite
};

struct element_def {
    std::string id;
    element_type type;
    rect bounds;                           // Position & size
    std::string text;                      // For labels/buttons
    std::string sprite_pak;                // For sprite elements
    int32_t sprite_index = 0;
    int32_t sprite_frame = 0;
    std::optional<sf::Color> color;
    std::optional<sf::Color> hover_color;
    std::map<std::string, std::string> properties;  // Extensible
};

struct dialog_definition {
    std::string id;
    std::string title;
    rect bounds;
    bool modal = false;
    bool draggable = true;
    bool closeable = true;
    bool has_border = true;
    std::optional<sf::Color> background_color;
    std::string background_sprite_pak;     // Classic mode
    int32_t background_sprite_index = 0;
    int32_t background_sprite_frame = 0;
    std::vector<element_def> elements;
};

} // namespace hb
```

#### 2. `dialog_builder` - Fluent API
```cpp
// src/ui/dialog_builder.hpp
namespace hb {

class dialog_builder {
public:
    static dialog_builder create(std::string_view id);

    // Dialog properties
    dialog_builder& title(std::string_view title);
    dialog_builder& bounds(int32_t x, int32_t y, int32_t w, int32_t h);
    dialog_builder& modal(bool m = true);
    dialog_builder& draggable(bool d = true);
    dialog_builder& closeable(bool c = true);
    dialog_builder& background(sf::Color color);
    dialog_builder& background_sprite(std::string_view pak, int32_t idx, int32_t frame = 0);

    // Elements
    dialog_builder& label(std::string_view id, int32_t x, int32_t y,
                          std::string_view text, sf::Color color = sf::Color::White);
    dialog_builder& button(std::string_view id, int32_t x, int32_t y,
                           int32_t w, int32_t h, std::string_view text);
    dialog_builder& text_input(std::string_view id, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t max_chars = 32);
    dialog_builder& image(std::string_view id, int32_t x, int32_t y,
                          std::string_view pak, int32_t idx, int32_t frame = 0);
    dialog_builder& progress_bar(std::string_view id, int32_t x, int32_t y,
                                 int32_t w, int32_t h);
    dialog_builder& checkbox(std::string_view id, int32_t x, int32_t y,
                             std::string_view label_text);
    dialog_builder& grid(std::string_view id, int32_t x, int32_t y,
                         int32_t cols, int32_t rows, int32_t cell_w, int32_t cell_h);

    // Build
    dialog_definition build();

private:
    dialog_definition def_;
};

} // namespace hb
```

#### 3. `render_strategy` - Dual Rendering Support
```cpp
// src/ui/render_strategy.hpp
namespace hb {

class render_strategy {
public:
    virtual ~render_strategy() = default;
    virtual void render_background(renderer& rend, sprite_manager& sprites,
                                   const dialog_definition& def, const rect& bounds) = 0;
    virtual void render_element(renderer& rend, sprite_manager& sprites,
                                const element_def& elem, const element_state& state) = 0;
};

class classic_render_strategy : public render_strategy {
    // Renders using PAK sprites (original game look)
};

class modern_render_strategy : public render_strategy {
    // Renders using programmatic drawing (new modern style)
};

} // namespace hb
```

#### 4. `managed_dialog` - Data-Driven Dialog
```cpp
// src/ui/managed_dialog.hpp
namespace hb {

class managed_dialog : public dialog {
public:
    explicit managed_dialog(dialog_definition def);

    // Event callbacks (for simple dialogs)
    void on_button_click(std::string_view element_id, std::function<void()> callback);
    void on_checkbox_change(std::string_view element_id, std::function<void(bool)> callback);
    void on_text_change(std::string_view element_id, std::function<void(std::string_view)> callback);
    void on_grid_select(std::string_view element_id, std::function<void(int32_t, int32_t)> callback);

    // Element access
    void set_label_text(std::string_view element_id, std::string_view text);
    void set_progress(std::string_view element_id, float value);  // 0.0-1.0
    void set_checkbox_checked(std::string_view element_id, bool checked);
    void set_element_visible(std::string_view element_id, bool visible);
    void set_element_enabled(std::string_view element_id, bool enabled);

    std::string_view get_text_input(std::string_view element_id) const;
    bool get_checkbox_checked(std::string_view element_id) const;

    // Rendering mode
    void set_render_strategy(std::unique_ptr<render_strategy> strategy);

protected:
    // Override points for subclassing (complex dialogs)
    virtual void on_open_impl() {}
    virtual void on_close_impl() {}
    virtual void on_update_impl(float delta_time) {}
    virtual bool on_custom_render(renderer& rend, sprite_manager& sprites) { return false; }

private:
    dialog_definition definition_;
    std::unique_ptr<render_strategy> render_strategy_;
    std::map<std::string, element_state> element_states_;
    std::map<std::string, std::function<void()>> button_callbacks_;
    // ... other callback maps
};

} // namespace hb
```

#### 5. `dialog_manager` - Central Coordinator
```cpp
// src/ui/dialog_manager.hpp
namespace hb {

class dialog_manager {
public:
    void initialize(sprite_manager* sprites);

    // Definition loading
    void load_definitions_from_yaml(const std::filesystem::path& yaml_path);
    void register_definition(dialog_definition def);

    // Dialog creation
    managed_dialog* create_dialog(std::string_view definition_id);

    template<typename T, typename... Args>
    T* create_custom_dialog(std::string_view definition_id, Args&&... args);

    // Global render mode
    void set_default_render_mode(render_mode mode);  // classic or modern

    // Hot reload (development)
    void reload_definitions();  // Re-reads YAML files

private:
    std::map<std::string, dialog_definition> definitions_;
    std::vector<std::unique_ptr<managed_dialog>> dialogs_;
    sprite_manager* sprites_ = nullptr;
    render_mode default_mode_ = render_mode::classic;
};

} // namespace hb
```

### YAML Schema for Dialog Definitions
```yaml
# assets/ui/dialogs/example_dialog.yaml
dialogs:
  - id: "confirm_dialog"
    title: "Confirm"
    bounds: { x: 200, y: 150, w: 240, h: 120 }
    modal: true
    draggable: true
    closeable: true
    background_color: { r: 32, g: 32, b: 48, a: 240 }

    elements:
      - id: "message"
        type: label
        bounds: { x: 20, y: 30 }
        text: "Are you sure?"
        color: { r: 255, g: 255, b: 200 }

      - id: "btn_yes"
        type: button
        bounds: { x: 30, y: 70, w: 80, h: 30 }
        text: "Yes"

      - id: "btn_no"
        type: button
        bounds: { x: 130, y: 70, w: 80, h: 30 }
        text: "No"
```

### Usage Examples

#### Simple Dialog (Callbacks Only)
```cpp
// No subclassing needed for simple confirm/alert dialogs
auto* dlg = dialog_manager.create_dialog("confirm_dialog");
dlg->set_label_text("message", "Delete this item?");
dlg->on_button_click("btn_yes", [this]() {
    delete_item();
    dlg->close();
});
dlg->on_button_click("btn_no", [this]() {
    dlg->close();
});
dlg->open();
```

#### Complex Dialog (Subclassing)
```cpp
// For dialogs needing custom logic (inventory, spellbook, etc.)
class inventory_dialog : public managed_dialog {
public:
    inventory_dialog(dialog_definition def, inventory& inv)
        : managed_dialog(std::move(def)), inventory_(inv) {}

protected:
    void on_open_impl() override {
        refresh_item_grid();
    }

    void on_update_impl(float dt) override {
        // Handle drag & drop logic, tooltips, etc.
    }

    bool on_custom_render(renderer& rend, sprite_manager& sprites) override {
        // Render item icons in grid cells
        render_items(rend, sprites);
        return true;  // We handled custom rendering
    }

private:
    inventory& inventory_;
};

// Create using template method
auto* inv_dlg = dialog_manager.create_custom_dialog<inventory_dialog>(
    "inventory_layout", player_inventory);
```

### Integration with Existing `ui_system`
```cpp
// ui_system owns dialog_manager internally
class ui_system {
public:
    dialog_manager& dialogs() { return dialog_manager_; }

    // Existing dialogs (gradual migration)
    chat_dialog& chat() { return *chat_dialog_; }
    // ...

private:
    dialog_manager dialog_manager_;

    // Legacy dialogs (keep working)
    std::unique_ptr<chat_dialog> chat_dialog_;
    std::unique_ptr<inventory_dialog> inventory_dialog_;
    // ...
};
```

### Files to Create
| File | Purpose |
|------|---------|
| `src/ui/dialog_definition.hpp` | Layout data structures |
| `src/ui/dialog_builder.hpp/cpp` | Fluent API for building definitions |
| `src/ui/render_strategy.hpp/cpp` | Classic and modern rendering |
| `src/ui/managed_dialog.hpp/cpp` | Data-driven dialog base |
| `src/ui/dialog_manager.hpp/cpp` | Central coordinator |
| `src/ui/yaml_loader.hpp/cpp` | YAML parsing for definitions |

### Implementation Phases

#### Phase 1: Core Infrastructure
- Create `dialog_definition` data structure
- Create `dialog_builder` fluent API
- Create `managed_dialog` base class (programmatic rendering only)

#### Phase 2: Rendering Strategies
- Create `render_strategy` interface
- Implement `modern_render_strategy` (programmatic)
- Implement `classic_render_strategy` (sprite-based)

#### Phase 3: Dialog Manager
- Create `dialog_manager` class
- Integrate with existing `ui_system`
- Basic dialog lifecycle (create, open, close, destroy)

#### Phase 4: YAML Support
- Create YAML loader using nlohmann/json (or yaml-cpp)
- Hot-reload support for development iteration
- Validation and error reporting

#### Phase 5: Migration
- Convert one existing dialog as proof-of-concept
- Document migration pattern for remaining dialogs
- Support both old and new dialogs coexisting

---

## Phase 3A: Critical Path (Enable Server Testing)

### Task 3A.1: PAK Sprite Bitmap Decoding
**Goal**: Render actual sprites from legacy PAK files

**Reference**: `Sprite.cpp` lines 26-97, `Mydib.cpp`

**Modify**: `src/assets/pak_file.cpp`, `src/assets/sprite.cpp`

**Implementation**:
```cpp
// Legacy DIB format: 16-bit RGB565
// Byte layout in PAK:
//   - Header at offset 24 + (index * 8)
//   - stBrush frame data: 12 bytes per frame
//   - Bitmap data: 16-bit pixels (RGB565)
// Convert: RGB565 → RGBA8888 for SFML
```

**Verification**: Load interface2.pak, render sprite on screen

---

### Task 3A.2: Login/Character Flow
**Goal**: Complete login → character select → create → enter game

**Reference**: `Game.cpp` - `UpdateScreen_OnLogin()`, `UpdateScreen_OnSelectCharacter()`

**Create**:
- `src/ui/dialogs/login_dialog.hpp/cpp`
- `src/ui/dialogs/character_select_dialog.hpp/cpp`
- `src/ui/dialogs/character_create_dialog.hpp/cpp`

**Modify**: `src/gameplay/game_state.cpp` (add state handlers)

**Verification**: Successfully login to server, see character list

---

### Task 3A.3: Critical UI Dialogs
**Goal**: Essential gameplay dialogs

| Dialog | Legacy Function | File to Create |
|--------|----------------|----------------|
| Icon Panel | `DrawDialogBox_IconPannel` | `icon_panel_dialog.hpp/cpp` |
| Gauge Panel | `DrawDialogBox_GaugePannel` | `gauge_panel_dialog.hpp/cpp` |
| System Menu | `DrawDialogBox_SysMenu` | `system_menu_dialog.hpp/cpp` |
| Level Up | `DrawDialogBox_LevelUpSetting` | `levelup_dialog.hpp/cpp` |

---

## Phase 3B: Visual Systems

### Task 3B.1: Effects System
**Reference**: `Effect.h/cpp`, `DynamicObjectID.h`

**Create**: `src/graphics/effect_system.hpp/cpp`

**Functionality**:
- Spell visual effects
- Damage number popups
- Frame-based animation
- Alpha blending (70%, 50%, 25%)

---

### Task 3B.2: Weather System
**Reference**: `m_stWhetherObject[]`, `DrawWhetherEffects()`

**Create**: `src/graphics/weather_system.hpp/cpp`

**Functionality**: Rain, snow, time-of-day lighting

---

### Task 3B.3: Entity Rendering
**Modify**: `src/entity/entity_manager.cpp`, `src/world/map_renderer.cpp`

**Functionality**:
- Animation state machine (stop, move, attack, damage, etc.)
- Equipment appearance rendering
- Name/HP bars
- Shadows

---

## Phase 3C: Remaining UI Dialogs (26 total)

### Gameplay Dialogs
| Dialog | Function | Priority |
|--------|----------|----------|
| Quest | `DrawDialogBox_Quest` | High |
| Guild Menu | `DrawDialogBox_GuildMenu` | High |
| Guild Operation | `DrawDialogBox_GuildOperation` | High |
| Exchange | `DrawDialogBox_Exchange` | High |
| Magic Shop | `DrawDialogBox_MagicShop` | Medium |
| Skill Dialog | `DrawDialogBox_SkillDlg` | Medium |
| Sell List | `DrawDialogBox_SellList` | Medium |

### War/Crusade Dialogs
| Dialog | Function |
|--------|----------|
| Crusade Job | `DrawDialogBox_CrusadeJob` |
| Commander | `DrawDialogBox_Commander` |
| Constructor | `DrawDialogBox_Constructor` |
| Soldier | `DrawDialogBox_Soldier` |

### Utility Dialogs
| Dialog | Function |
|--------|----------|
| Text | `DrawDialogBox_Text` |
| NPC Action | `DrawDialogBox_NpcActionQuery` |
| Warning Msg | `DrawDialogBox_WarningMsg` |
| Item Drop | `DrawDialogBox_ItemDrop` |
| Query Amount | `DrawDialogBox_QueryDropItemAmount` |
| Fishing | `DrawDialogBox_Fishing` |
| City Hall | `DrawDialogBox_CityHallMenu` |

---

## Phase 3D: Gameplay Systems

### Task 3D.1: Guild System
**Create**: `src/gameplay/guild.hpp/cpp`
- Member management
- Rank system
- Guild warehouse

### Task 3D.2: Quest System
**Create**: `src/gameplay/quest.hpp/cpp`
- Quest tracking
- Progress/rewards

### Task 3D.3: Crusade System
**Create**: `src/gameplay/crusade.hpp/cpp`
- War state tracking
- Structure building
- Commander commands

---

## Phase 3E: Audio & Polish

### Task 3E.1: Sound Effect Mapping
**Reference**: `SoundID.h` (110+ sounds)

**Create**: `src/audio/sound_ids.hpp`

**Functionality**:
- Map legacy IDs to files
- Positional audio
- Background music

---

## Phase 3F: Integration Testing

### Server Protocol Verification
- Capture legacy client packets
- Compare with modern client
- Fix any protocol mismatches

### Full Game Loop Checklist
- [ ] Login flow
- [ ] Character creation
- [ ] Movement (8 directions)
- [ ] Combat (melee, ranged, magic)
- [ ] Inventory operations
- [ ] NPC interaction
- [ ] Chat (all modes)
- [ ] Party/Guild

---

## Implementation Priority (Server Testing Focus)

### Sprint 1: Enable Server Connection (CRITICAL PATH)
| # | Task | Files | Outcome |
|---|------|-------|---------|
| 1 | PAK bitmap decoding | `pak_file.cpp`, `sprite.cpp` | Sprites render |
| 2 | Login dialog | `login_dialog.hpp/cpp` | Account/password input |
| 3 | Character select | `character_select_dialog.hpp/cpp` | See character list |
| 4 | Character create | `character_create_dialog.hpp/cpp` | Create new character |
| 5 | Enter game flow | `game_state.cpp` | Load into world |

### Sprint 2: Playable State
| # | Task | Files | Outcome |
|---|------|-------|---------|
| 6 | Icon panel | `icon_panel_dialog.hpp/cpp` | Quick action buttons |
| 7 | Gauge panel | `gauge_panel_dialog.hpp/cpp` | HP/MP/SP/EXP bars |
| 8 | System menu | `system_menu_dialog.hpp/cpp` | Options, logout |
| 9 | Level up dialog | `levelup_dialog.hpp/cpp` | Allocate stat points |

### Sprint 3: Server Protocol Verification
| # | Task | Method |
|---|------|--------|
| 10 | Test login packet | Compare with legacy client |
| 11 | Test movement sync | Walk, run, direction changes |
| 12 | Test combat | Attack, damage, death |
| 13 | Test chat | Normal, whisper, guild, party |

### Future Sprints (On-Demand)
- Remaining 22 dialogs (add as needed during testing)
- Effects system (when combat testing)
- Weather system (low priority)
- Guild/Quest/Crusade (when testing those features)

---

## Verification Plan

### Milestone 1: Sprites Render
- Load `interface2.pak` or `Hitem2.pak`
- Display any sprite frame on screen
- Verify color key transparency works

### Milestone 2: Server Login
- Connect to your Helbreath server
- Send login packet with encryption
- Receive character list response

### Milestone 3: Enter Game World
- Select/create character
- Receive initial player data
- Render player on map

### Milestone 4: Basic Gameplay
- Movement works (8 directions)
- Can attack monsters/players
- HP/MP/SP bars update
- Chat messages display

---

## Asset Locations
Game assets in `C:\code\vibe\helbreath\client\assets\`:
```
assets/
├── sprites/         # PAK files (interface.pak, interface2.pak, *.pak)
├── fonts/           # Font files
├── data/
│   ├── mapdata/     # Map tile data
│   ├── music/       # Background music
│   ├── shops/       # Shop configurations
│   └── sounds/      # Sound effects
└── strings/         # Localization JSON
```

**Key PAK files for testing**: `assets/sprites/interface.pak`, `assets/sprites/interface2.pak`

## Reference Files Location
Legacy source files in `C:\code\vibe\helbreath\client\` (same directory as `src/`)

---

## Phase 2 Tasks Overview

| Priority | Task | Reference Files | Lines Est. |
|----------|------|-----------------|------------|
| 1 | Network Protocol | NetMessages.h, XSocket.cpp | ~800 |
| 2 | Packet Handlers | Game.cpp (message handlers) | ~2000 |
| 3 | Combat System | Game.cpp (_iGetAttackType, etc.) | ~500 |
| 4 | Magic System | Magic.cpp/h, Game.cpp | ~600 |
| 5 | Skill System | Skill.cpp/h | ~400 |
| 6 | Item/Equipment | Item.cpp/h | ~800 |
| 7 | Entity Animation | Game.cpp (DrawObject_OnXXX) | ~1500 |
| 8 | Map/PAK Loading | Sprite.cpp, MapData.cpp | ~1000 |
| 9 | UI Dialogs (41) | Game.cpp (DrawDialogBox_XXX) | ~8000 |

---

## Task 1: Network Protocol Implementation

**Goal**: Implement the packet format, encryption, and base message handling.

**Reference**: `XSocket.cpp`, `NetMessages.h`

### Packet Structure
```
Byte 0:       Key (0 = no encryption)
Bytes 1-2:    Packet size (little-endian, includes 3-byte header)
Bytes 3+:     Payload (encrypted if key != 0)
```

### Encryption (XOR cipher)
```cpp
// Encrypt (XSocket.cpp:486-490)
for (int i = 0; i < size; i++) {
    payload[i] = (payload[i] + (i ^ key)) ^ (key ^ (size - i));
}
// Decrypt is reverse operation
```

### Files to Modify
- `src/network/packet.hpp/cpp` - Add encryption/decryption
- `src/network/protocol.hpp` - Add all message IDs from NetMessages.h

### Key Message IDs to Add
```cpp
// Login (0x0FC94xxx)
constexpr uint32_t msg_request_login = 0x0FC94201;
constexpr uint32_t msg_response_login = 0x0FC94203;
constexpr uint32_t msg_request_enter_game = 0x0FC94205;

// Gameplay (0x0FA314xx)
constexpr uint32_t msg_command_motion = 0x0FA314D5;
constexpr uint32_t msg_response_motion = 0x0FA314D6;
constexpr uint32_t msg_event_motion = 0x0FA314D7;
constexpr uint32_t msg_notify = 0x0FA314D0;

// Common (0x0Axx subtypes)
constexpr uint16_t common_itemdrop = 0x0A01;
constexpr uint16_t common_magic = 0x0A0D;
constexpr uint16_t common_useitem = 0x0A11;
// ... 50+ more
```

---

## Task 2: Packet Handlers Implementation

**Goal**: Implement the 60+ NotifyMsg handlers and response handlers.

**Reference**: `Game.cpp` - search for `NotifyMsg_`, `ResponseHandler`, `EventHandler`

### Handler Categories

**Status Notifications (0x0Bxx)**:
| ID | Handler | Purpose |
|----|---------|---------|
| 0x0B07 | notify_hp | HP changed |
| 0x0B14 | notify_mp | MP changed |
| 0x0B15 | notify_sp | SP changed |
| 0x0B16 | notify_levelup | Level up |
| 0x0B01 | notify_item_obtained | Got item |
| 0x0B10 | notify_magic_success | Learned spell |
| 0x0B12 | notify_skill_success | Trained skill |

### Files to Create/Modify
- `src/network/handlers/notify_handlers.hpp/cpp`
- `src/network/handlers/response_handlers.hpp/cpp`
- `src/network/handlers/event_handlers.hpp/cpp`

### Implementation Pattern
```cpp
// In network_system.cpp
void network_system::dispatch_notify(uint16_t type, packet_reader& reader) {
    switch (type) {
        case notify_type::hp: handle_notify_hp(reader); break;
        case notify_type::mp: handle_notify_mp(reader); break;
        // ... 60+ cases
    }
}
```

---

## Task 3: Combat System

**Goal**: Port damage formulas, attack types, and combat mechanics.

**Reference**: `Game.cpp` lines 23713-23850 (`_iGetAttackType`, `_iGetWeaponSkillType`)

### Stat Formulas
```cpp
// From Game.cpp line 20154+
max_hp = (vitality * 3) + (level * 2) + (strength / 2);
max_mp = (magic * 2) + (level * 2) + (intelligence / 2);
max_sp = (strength * 2) + (level * 2);
```

### Weapon Skill Mapping
```cpp
enum class weapon_skill : uint8_t {
    fist = 5,        // Weapon types 0
    archery = 6,     // Weapon types 40+
    two_hand = 7,    // Weapon types 1-2
    sword = 8,       // Weapon types 3-6, 8-19
    spear = 9,       // Weapon type 7
    axe = 10,        // Weapon types 20-29
    shield = 14,     // Weapon types 30-34
    staff = 21       // Weapon types 35-39
};
```

### Attack Types
- Type 1: Normal melee
- Type 2: Ranged (bow)
- Types 20-27: Super attacks (require 100% skill mastery)

### Files to Create/Modify
- `src/gameplay/combat.hpp/cpp` - New file for combat system
- `src/gameplay/stats.hpp` - Stat calculation formulas

---

## Task 4: Magic System Enhancement

**Goal**: Port spell casting mechanics, mana costs, and spell effects.

**Reference**: `Magic.cpp/h`, `Game.cpp` line 48061 (`UseMagic`)

### Magic Types (23+)
```cpp
enum class magic_type : uint8_t {
    damage_spot = 1,      // Single target damage
    hpup_spot = 2,        // Single target heal
    damage_area = 3,      // AoE damage
    spdown_spot = 4,      // SP drain
    teleport = 8,         // Teleportation
    summon = 9,           // Monster summon
    protect = 11,         // Protection buff
    invisibility = 13,    // Stealth
    poison = 17,          // DoT
    polymorph = 20,       // Transform
    ice = 23              // Freeze effect
};
```

### Mana Cost Calculation
```cpp
int32_t calculate_mana_cost(spell_id spell, const player_stats& stats) {
    auto& info = get_spell_info(spell);
    int32_t base_cost = info.mana_cost;
    // Modify by intelligence and mastery
    return base_cost - (stats.intelligence / 10);
}
```

### Files to Modify
- `src/gameplay/magic.hpp/cpp` - Add casting logic, effect application

---

## Task 5: Skill System Enhancement

**Goal**: Port skill experience, mastery tracking, and skill usage.

**Reference**: `Skill.cpp/h`, `Game.cpp`

### Skill Mastery
- 60 skills tracked in `m_cSkillMastery[60]` array
- Range: 0-100%
- Super attacks unlock at 100% mastery

### Files to Modify
- `src/gameplay/skills.hpp/cpp` - Add experience gain, mastery calculation

---

## Task 6: Item/Equipment System

**Goal**: Port item effects, equipment bonuses, and inventory management.

**Reference**: `Item.cpp/h`, `Game.cpp` line 34584-34722

### Equipment Slots (15)
Already defined in `src/core/game_enums.hpp`

### Item Effects
```cpp
// Attribute bit unpacking from m_dwAttribute
uint8_t effect_type = (attribute >> 16) & 0x0F;  // bits 16-19
uint8_t effect_value = (attribute >> 8) & 0x0F;  // bits 8-11
uint8_t bonus = (attribute >> 28) & 0x0F;        // bits 28-31
```

### Common Effects
| Type | Effect |
|------|--------|
| 1 | Super attack bonus +N |
| 6 | Fire resistance +N×4% |
| 8 | Ice resistance +N×7% |
| 10 | Spell accuracy +N×3% |
| 11 | Poison resistance N% |

### Files to Modify
- `src/gameplay/inventory.hpp/cpp` - Add item effect application
- `src/gameplay/item.hpp` - Add attribute unpacking

---

## Task 7: Entity Animation States

**Goal**: Port the 12 DrawObject_OnXXX animation state handlers.

**Reference**: `Game.cpp` - search for `DrawObject_On`

### Animation States
```cpp
enum class entity_state : uint8_t {
    stop,          // DrawObject_OnStop
    move,          // DrawObject_OnMove
    run,           // DrawObject_OnRun
    attack,        // DrawObject_OnAttack
    attack_move,   // DrawObject_OnAttackMove
    damage,        // DrawObject_OnDamage
    damage_move,   // DrawObject_OnDamageMove
    magic,         // DrawObject_OnMagic
    get_item,      // DrawObject_OnGetItem
    dying,         // DrawObject_OnDying
    dead           // DrawObject_OnDead
};
```

### Files to Modify
- `src/entity/components.hpp` - Add animation state component
- `src/entity/entity_manager.cpp` - Add state-based rendering

---

## Task 8: PAK File & Sprite Loading

**Goal**: Actually parse and render sprites from PAK files.

**Reference**: `Sprite.cpp` constructor (lines 50-150), `Sprite.h` (stBrush struct)

### PAK Format
```cpp
struct pak_header {
    uint32_t entry_count;
    uint32_t offsets[];  // Array of file offsets
};

struct sprite_header {
    int16_t pivot_x, pivot_y;
    uint16_t width, height;
    // Followed by RLE-compressed bitmap data
};
```

### Color Key
- Color key is the pixel at position (0, 0) of each sprite
- Not all sprites have transparency (e.g., background tiles don't use color keys)
- Must check sprite type to determine if transparency applies

### Files to Modify
- `src/assets/pak_file.cpp` - Implement actual PAK parsing
- `src/assets/sprite.cpp` - Convert to SFML textures

---

## Task 9: UI Dialogs (41 types)

**Goal**: Port the major game dialogs.

**Reference**: `Game.cpp` - search for `DrawDialogBox_`, `DlgBoxClick_`

### Priority Dialogs
| ID | Name | Function |
|----|------|----------|
| 1 | Character | DrawDialogBox_Character |
| 2 | Inventory | DrawDialogBox_Inventory |
| 3 | Magic/Spellbook | DrawDialogBox_Magic |
| 10 | Chat | DrawDialogBox_Chat |
| 11 | Shop | DrawDialogBox_Shop |
| 14 | Bank | DrawDialogBox_Bank |
| 15 | Skills | DrawDialogBox_Skill |

### Implementation Pattern
Each dialog needs:
1. `draw()` method - Render the dialog
2. `handle_click(x, y)` method - Process mouse clicks
3. State tracking (open/closed, scroll position, etc.)

### Files to Create
- `src/ui/dialogs/character_dialog.hpp/cpp`
- `src/ui/dialogs/inventory_dialog.hpp/cpp`
- `src/ui/dialogs/magic_dialog.hpp/cpp`
- etc.

---

## Implementation Order

```
Week 1-2:  Tasks 1-2 (Network protocol + handlers)
Week 3:    Task 3 (Combat system)
Week 4:    Tasks 4-5 (Magic + Skills)
Week 5:    Task 6 (Items/Equipment)
Week 6:    Task 7 (Entity animation)
Week 7:    Task 8 (PAK loading)
Week 8-10: Task 9 (UI dialogs)
```

---

## Verification Milestones

### Milestone 1: Network Connection
- Connect to login server
- Send login packet with encryption
- Receive and parse response

### Milestone 2: Enter Game World
- Complete login flow
- Receive player data
- Load into map

### Milestone 3: Movement Works
- Send movement commands
- See other players move
- Proper animation states

### Milestone 4: Combat Works
- Attack entities
- See damage numbers
- Death/respawn cycle

### Milestone 5: Full Gameplay
- Inventory management
- Spell casting
- NPC interaction
- Guild/party systems

---

# Helbreath Client C++20 Modernization - Phase 1 Plan (COMPLETED)

## Strategy Summary

- **Graphics/Audio/Input**: DirectX 7 → SFML (immediate replacement)
- **Asset Format**: Preserve PAK files, load into SFML textures at runtime
- **Game Logic**: Full rewrite using CGame as reference documentation

## Code Style

- **snake_case** for everything (functions, variables, types, files)
- **No Hungarian notation** (no `bFlag`, `iCount`, `pPointer`)
- **No member prefixes** (no `m_variable`, just `variable`)
- **stdlib types** directly (`int32_t`, `uint8_t`, not custom aliases)
- **Namespaces**: `hb::` for all project code

---

## Phase 1 Tasks

### Task 1: CMake Build System

**Files to create:**
- `CMakeLists.txt` (root)
- `CMakePresets.json`
- `src/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(helbreath_client VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML CONFIG REQUIRED COMPONENTS graphics window system audio)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(src)
```

---

### Task 2: Core Constants Module

**Files to create:**
- `src/core/constants.hpp`
- `src/core/game_enums.hpp`

**constants.hpp** - Extract from Game.h lines 58-141:
```cpp
#pragma once
#include <cstdint>
#include <cstddef>

namespace hb {

inline constexpr uint32_t screen_width = 640;
inline constexpr uint32_t screen_height = 480;
inline constexpr std::size_t max_sprites = 20000;
inline constexpr std::size_t max_items = 50;
inline constexpr std::size_t max_bank_items = 121;
inline constexpr std::size_t max_chat_messages = 500;
inline constexpr std::size_t max_effects = 300;
inline constexpr std::size_t max_sound_effects = 110;
inline constexpr uint32_t double_click_time = 300;
// ... all other DEF_* constants

} // namespace hb
```

**game_enums.hpp** - Type-safe enums:
```cpp
#pragma once
#include <cstdint>

namespace hb {

enum class game_state : int8_t {
    main_menu,
    login,
    character_select,
    loading,
    playing,
    quit
};

enum class equip_slot : uint8_t {
    none = 0,
    head = 1,
    body = 2,
    arms = 3,
    pants = 4,
    boots = 5,
    neck = 6,
    left_hand = 7,
    right_hand = 8,
    two_hand = 9,
    right_finger = 10,
    left_finger = 11,
    back = 12,
    full_body = 13
};

enum class item_type : uint8_t {
    none = 0,
    equip = 1,
    consumable = 2,
    material = 3,
    arrow = 6
};

} // namespace hb
```

---

### Task 3: SFML Renderer

**Files to create:**
- `src/graphics/renderer.hpp`
- `src/graphics/renderer.cpp`

**Replaces:** `DXC_ddraw.h/cpp`

```cpp
#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string_view>

namespace hb {

class sprite; // forward declaration

class renderer {
public:
    bool initialize(uint32_t width, uint32_t height, bool fullscreen);
    void shutdown();

    void begin_frame();
    void end_frame();

    // sprite drawing
    void draw_sprite(const sprite& spr, int32_t x, int32_t y, uint32_t frame = 0);
    void draw_sprite_alpha(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);

    // text
    void draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color);

    // primitives
    void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, sf::Color color, bool filled = true);
    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, sf::Color color);

    sf::RenderWindow& window() { return window_; }
    bool is_open() const { return window_.isOpen(); }

private:
    sf::RenderWindow window_;
    sf::Font font_;
};

} // namespace hb
```

---

### Task 4: SFML Input

**Files to create:**
- `src/input/input.hpp`
- `src/input/input.cpp`

**Replaces:** `DXC_dinput.h/cpp`

```cpp
#pragma once
#include <SFML/Window.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace hb {

class input {
public:
    void update(sf::RenderWindow& window);
    void reset();

    // mouse
    int32_t mouse_x() const { return mouse_x_; }
    int32_t mouse_y() const { return mouse_y_; }
    int32_t wheel_delta() const { return wheel_delta_; }
    bool is_mouse_down(sf::Mouse::Button btn) const;
    bool is_mouse_pressed(sf::Mouse::Button btn) const;
    bool is_mouse_released(sf::Mouse::Button btn) const;

    // keyboard
    bool is_key_down(sf::Keyboard::Key key) const;
    bool is_key_pressed(sf::Keyboard::Key key) const;
    bool is_key_released(sf::Keyboard::Key key) const;

    // text input
    std::string_view text_input() const { return text_input_; }
    void clear_text_input() { text_input_.clear(); }

private:
    int32_t mouse_x_ = 0;
    int32_t mouse_y_ = 0;
    int32_t wheel_delta_ = 0;

    std::array<bool, sf::Mouse::ButtonCount> mouse_down_{};
    std::array<bool, sf::Mouse::ButtonCount> mouse_pressed_{};
    std::array<bool, sf::Mouse::ButtonCount> mouse_released_{};

    std::array<bool, sf::Keyboard::KeyCount> key_down_{};
    std::array<bool, sf::Keyboard::KeyCount> key_pressed_{};
    std::array<bool, sf::Keyboard::KeyCount> key_released_{};

    std::string text_input_;
};

} // namespace hb
```

---

### Task 5: SFML Audio

**Files to create:**
- `src/audio/audio.hpp`
- `src/audio/audio.cpp`

**Replaces:** `YWSound.h/cpp`, `SoundBuffer.h/cpp`

```cpp
#pragma once
#include <SFML/Audio.hpp>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hb {

using sound_id = uint16_t;

class audio {
public:
    bool initialize();
    void shutdown();
    void update();

    // sound effects
    sound_id load_sound(std::string_view path);
    void play_sound(sound_id id, float volume = 1.0f, float pan = 0.0f);
    void stop_sound(sound_id id);

    // music
    bool play_music(std::string_view path, bool loop = true);
    void stop_music();
    void set_music_volume(float volume);

    // global
    void set_master_volume(float volume);
    void set_muted(bool muted);

private:
    std::unordered_map<sound_id, sf::SoundBuffer> buffers_;
    std::vector<sf::Sound> active_sounds_;
    sf::Music music_;

    float master_volume_ = 1.0f;
    bool muted_ = false;
    sound_id next_id_ = 1;
};

} // namespace hb
```

---

### Task 6: PAK File Reader

**Files to create:**
- `src/assets/pak_file.hpp`
- `src/assets/pak_file.cpp`

**Purpose:** Read existing PAK sprite archives, extract bitmap data.

```cpp
#pragma once
#include <cstdint>
#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

namespace hb {

struct pak_entry {
    std::vector<uint8_t> data;
    uint32_t width;
    uint32_t height;
};

class pak_file {
public:
    bool open(std::string_view path);
    void close();

    uint32_t entry_count() const { return static_cast<uint32_t>(offsets_.size()); }
    std::optional<pak_entry> read_entry(uint32_t index);

private:
    std::ifstream file_;
    std::vector<uint32_t> offsets_;
};

} // namespace hb
```

**Reference:** Study `Sprite.cpp` constructor to understand PAK format.

---

### Task 7: Sprite System

**Files to create:**
- `src/assets/sprite.hpp`
- `src/assets/sprite.cpp`

**Purpose:** Convert PAK bitmap data to SFML textures with frame support.

```cpp
#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <vector>

namespace hb {

class pak_file;

struct sprite_frame {
    sf::IntRect rect;
    int16_t pivot_x;
    int16_t pivot_y;
};

class sprite {
public:
    bool load_from_pak(pak_file& pak, uint32_t index);

    void draw(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame = 0) const;
    void draw_alpha(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame, float alpha) const;

    uint32_t frame_count() const { return static_cast<uint32_t>(frames_.size()); }
    const sprite_frame& get_frame(uint32_t idx) const { return frames_[idx]; }

private:
    sf::Texture texture_;
    std::vector<sprite_frame> frames_;
};

} // namespace hb
```

---

### Task 8: Application Shell

**Files to create:**
- `src/application.hpp`
- `src/application.cpp`
- `src/main.cpp`

```cpp
// application.hpp
#pragma once
#include "core/game_enums.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"

namespace hb {

class application {
public:
    int run();

private:
    bool initialize();
    void shutdown();
    void main_loop();

    void process_events();
    void update(float dt);
    void render();

    renderer renderer_;
    input input_;
    audio audio_;

    game_state state_ = game_state::main_menu;
    bool running_ = false;
};

} // namespace hb
```

```cpp
// main.cpp
#include "application.hpp"

int main() {
    hb::application app;
    return app.run();
}
```

---

## File Structure

```
client/
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── application.hpp
│   ├── application.cpp
│   ├── core/
│   │   ├── constants.hpp
│   │   └── game_enums.hpp
│   ├── graphics/
│   │   ├── renderer.hpp
│   │   └── renderer.cpp
│   ├── input/
│   │   ├── input.hpp
│   │   └── input.cpp
│   ├── audio/
│   │   ├── audio.hpp
│   │   └── audio.cpp
│   └── assets/
│       ├── pak_file.hpp
│       ├── pak_file.cpp
│       ├── sprite.hpp
│       └── sprite.cpp
├── reference/                    # Old code for reference only
│   ├── Game.h
│   ├── Game.cpp
│   └── ...
└── assets/
    └── sprites/*.pak
```

---

## Implementation Order

1. `CMakeLists.txt` + `CMakePresets.json` → verify SFML links
2. `core/constants.hpp` → extract from Game.h
3. `core/game_enums.hpp` → type-safe enums
4. `graphics/renderer` → open window, clear/display
5. `input/input` → mouse and keyboard
6. `audio/audio` → basic sound playback
7. `assets/pak_file` → read PAK archives
8. `assets/sprite` → convert to SFML textures
9. `application` → main loop, tie it together

---

## Verification

### Build Test
```bash
cmake --preset=default
cmake --build build
./build/helbreath_client
```

### Milestone 1: Window Opens
- SFML window displays at 640x480
- Window closes on X button or Escape key

### Milestone 2: Sprite Renders
- Load one PAK file (e.g., `sprites/interface2.pak`)
- Display a sprite on screen

### Milestone 3: Input Works
- Print mouse coordinates to console
- Detect mouse clicks and key presses

### Milestone 4: Audio Plays
- Load and play a WAV sound effect
- Background music streams and loops

---

## Reference Files (from legacy code)

| What | Where to Look |
|------|---------------|
| PAK format | `Sprite.cpp` constructor, lines 50-150 |
| Sprite frames/brush | `Sprite.h` stBrush struct |
| Color key value | `DXC_ddraw.cpp` - usually magenta (255,0,255) |
| Alpha blend tables | `DXC_ddraw.cpp` G_lTransG* arrays |
| Game constants | `Game.h` lines 58-141 |
| Game modes | `Game.h` lines 104-126 |
| Sound loading | `SoundBuffer.cpp` _LoadWavFile() |

---

## Code Style Summary

| Element | Style | Example |
|---------|-------|---------|
| Namespaces | snake_case | `hb::graphics` |
| Classes | snake_case | `class renderer` |
| Functions | snake_case | `void draw_sprite()` |
| Variables | snake_case | `int32_t mouse_x` |
| Member variables | trailing `_` | `sf::Font font_` |
| Constants | snake_case | `max_sprites` |
| Enums | snake_case | `enum class game_state` |
| Enum values | snake_case | `game_state::main_menu` |
| File names | snake_case | `pak_file.hpp` |
| Types | stdlib directly | `int32_t`, `uint8_t`, `float` |

**No custom type aliases** - use `int32_t`, `uint8_t`, `float` directly
**No Hungarian notation** - don't prefix with type indicators
**No m_ prefix** - use trailing underscore `_` for private members
