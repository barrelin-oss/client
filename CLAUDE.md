# Helbreath Client

## Project Overview

**Helbreath** is a classic late-1990s/early-2000s 2D MMORPG being modernized to C++20. This is the game client, using SFML for rendering, UI, and networking.

### Current State

| Aspect | Original | Modernized |
|--------|----------|------------|
| Language | C++98 | C++20 |
| Graphics | DirectX 7 | SFML |
| Audio | DirectSound 7 | SFML Audio |
| Input | DirectInput 7 | SFML Window |
| Networking | WinSock2 | SFML Network + WebSocket |
| Architecture | Monolithic (~48K lines) | Subsystem-based |

### Key Statistics

| Component | Count |
|-----------|-------|
| Dialogue box types | 41 |
| Magic spell types | 100+ |
| Skill types | 60 |
| Supported languages | 5 |

---

## Building the Project

### Prerequisites

- **CMake 3.20+**
- **Visual Studio 2022** (or MSVC 19.29+)
- **vcpkg** installed at `C:/code/vcpkg`

### Build Commands

```bash
# Configure and build (Debug)
cmake --preset=default
cmake --build --preset=default

# Configure and build (Release)
cmake --preset=release
cmake --build --preset=release

# Clean rebuild
rm -rf build
cmake --preset=default
cmake --build --preset=default
```

### Output

All binaries and DLLs output to `bin/`:
- `bin/helbreath_client.exe` - Main executable
- `bin/icon_panel_demo.exe` - Demo executable
- DLLs automatically copied after build

### Visual Studio

Solution: `build/helbreath_client.sln`
Debugger working directory: Configured to `bin/` for F5 debugging.

### Dependencies (via vcpkg)

| Package | Purpose |
|---------|---------|
| **sfml** | Graphics, audio, input, window |
| **spdlog** | Logging |
| **nlohmann-json** | JSON parsing |
| **yaml-cpp** | Dialog YAML definitions |
| **ixwebsocket** | WebSocket client |
| **libsodium** | Cryptography |
| **openssl**, **zlib**, **lz4** | Networking/compression |

---

## Coding Style

### Naming Convention

**All new code must follow stdlib-style snake_case naming:**

| Element | Convention | Example |
|---------|------------|---------|
| Types (classes, structs, enums) | snake_case | `player_state`, `damage_type` |
| Variables | snake_case | `health`, `player_name` |
| Functions/Methods | snake_case | `calculate_damage()`, `get_player()` |
| Constants | snake_case with constexpr | `max_inventory_slots` |
| Files | snake_case | `player_state.h`, `combat_system.cpp` |
| Namespace | `hb` | `namespace hb { }` |

**What to avoid:**
- **No Hungarian notation**: No `bVar`, `iCount`, `szString` prefixes
- **No member prefixes**: No `m_` prefix for class members
- **No C-prefix**: No `CGame`, `CClient` - just `game`, `client`
- **No SCREAMING_SNAKE**: Use `constexpr` lowercase instead

### Formatting

| Rule | Convention |
|------|------------|
| Brace style | **Allman** (opening brace on its own line) |
| Indentation | 4 spaces (no tabs) |
| Line length | 120 characters max |
| Pointer/reference | `int* ptr` not `int *ptr` |

Formatting is enforced by `.clang-format` at the project root. Run `clang-format -i src/**/*.cpp src/**/*.hpp` to format.

```cpp
// GOOD (stdlib-like style with Allman braces)
class player_state
{
    int32_t health;
    std::string name;
    bool is_active;
};

void calculate_damage(int raw_damage, int armor)
{
    if (raw_damage > 0)
    {
        // ...
    }
}

inline constexpr auto max_level = 180;

// BAD (legacy style to avoid)
class CPlayerState {              // No C-prefix, use Allman braces
    int m_iHealth;                // No m_ prefix, no Hungarian notation
    char* m_szName;               // No sz prefix
    BOOL m_bIsActive;             // No BOOL, no m_b prefix
};
#define MAX_LEVEL 180             // Use constexpr instead
```

---

## C++20 Guidelines

### Preferred Language Features

```cpp
// Use concepts for generic constraints
template<typename T>
concept entity = requires(T t)
{
    { t.get_id() } -> std::convertible_to<uint32_t>;
};

// Use std::span instead of raw pointer + size
void process_items(std::span<const item> items);

// Use std::optional for nullable returns
auto find_player(uint32_t id) -> std::optional<player_state&>;

// Use std::expected (C++23) or result<T,E> for error handling
auto create_item(item_template tmpl) -> std::expected<item, std::string>;

// Use std::string_view for non-owning string parameters
void send_message(client_id id, std::string_view message);

// Use designated initializers
auto config = server_config
{
    .port = 2848,
    .max_clients = 2000,
};

// Use structured bindings
auto [success, player] = player_manager.authenticate(credentials);

// Use ranges for collection operations
auto active = clients | std::views::filter(&client::is_active);

// Use constexpr for compile-time computation
inline constexpr auto max_inventory_slots = 50;

// Use enum class with underlying type
enum class damage_type : uint8_t
{
    physical = 0,
    magic = 1,
};
```

### Memory Management

```cpp
// Use smart pointers
std::unique_ptr<T>  // Sole ownership
std::shared_ptr<T>  // Shared ownership (use sparingly)

// Use containers instead of raw arrays
std::vector<T>              // Dynamic array
std::array<T, N>            // Fixed-size array
std::unordered_map<K, V>    // Hash map

// RAII for all resources - no manual new/delete
```

---

## Memory Safety Patterns

### Container Removal Order

When an object is owned by one container (e.g., `unique_ptr` in a map) and referenced by another (e.g., raw pointers in a vector), **always remove references before deleting the owner**:

```cpp
// WRONG - use-after-free!
dialogs_.erase(type);  // Deletes the object
dialog_order_.erase(   // Dereferences deleted pointer
    std::remove_if(..., [](dialog* d) { return d->type() == type; }), ...);

// CORRECT - remove reference first, then delete
if (auto it = dialogs_.find(type); it != dialogs_.end())
{
    dialog* ptr = it->second.get();  // Get pointer while object is alive
    dialog_order_.erase(
        std::remove(dialog_order_.begin(), dialog_order_.end(), ptr),
        dialog_order_.end()
    );
    dialogs_.erase(it);  // Now safe to delete
}
```

### Thread Safety

Never modify shared containers from background threads. The ixwebsocket library runs callbacks on background threads. Use one of these patterns:

1. **Polling**: Don't set callbacks; poll for messages on the main thread:
   ```cpp
   // In update() on main thread:
   while (auto msg = connection.receive())
   {
       handle_message(*msg);  // Safe to modify state here
   }
   ```

2. **Deferred actions**: Queue events for main thread processing:
   ```cpp
   // Background thread - just set a flag
   {
       std::lock_guard<std::mutex> lock(mutex_);
       pending_event_ = event;
   }
   has_pending_event_.store(true);

   // Main thread - process the queued event
   if (has_pending_event_.exchange(false))
   {
       Event event;
       {
           std::lock_guard<std::mutex> lock(mutex_);
           event = std::move(pending_event_);
       }
       process_event(event);  // Safe to modify state
   }
   ```

---

## Error Handling

```cpp
// Prefer std::expected or Result<T,E> for recoverable errors
std::expected<result, error_code> try_operation();

// Use exceptions only for truly exceptional/unrecoverable situations
// No exceptions in hot paths

// Use assertions for programmer errors (debug only)
assert(ptr != nullptr && "Pointer must not be null");

// Log errors with structured logging (spdlog)
spdlog::error("Failed to load player {}: {}", player_id, error.message());
```

---

## File Organization

```cpp
// Header file structure
#pragma once

#include <standard_library>     // Standard library first
#include <third_party/lib.h>    // Third-party second
#include "project/header.h"     // Project headers last

namespace hb::subsystem
{

// Forward declarations
class other_class;

// Type aliases
using player_id = uint32_t;

// Class declaration
class my_class
{
public:
    my_class();
    ~my_class();

    void public_method();

private:
    int32_t member_;  // Trailing underscore for private members (optional)
};

} // namespace hb::subsystem
```

---

## Architecture

### Directory Structure

```
src/
├── main.cpp                    # Entry point
├── application.cpp/h           # Application lifecycle
├── core/                       # Utilities (types, config, timer)
├── math/                       # Vec2, Rect, Color
├── platform/                   # Window, filesystem
├── graphics/                   # Renderer, sprites, text
├── audio/                      # Sound, music
├── input/                      # Input system
├── network/                    # Sockets, packets, WebSocket
├── assets/                     # Asset manager, PAK files
├── world/                      # Maps, tiles
├── entity/                     # Players, NPCs, monsters
├── gameplay/                   # Combat, magic, skills, inventory
├── ui/                         # UI system, widgets, dialogs
├── chat/                       # Chat system
└── localization/               # String localization
```

### Key Subsystems

| Subsystem | Files | Purpose |
|-----------|-------|---------|
| **Graphics** | `src/graphics/` | SFML rendering, sprites, text |
| **UI System** | `src/ui/` | Dialogs, widgets, screens |
| **Network** | `src/network/` | Packet protocol, WebSocket |
| **Game State** | `src/gameplay/game_state.*` | State machine, subsystem orchestration |
| **Entity** | `src/entity/` | Players, NPCs, components |
| **World** | `src/world/` | Maps, tiles, spatial queries |

---

## UI System

The UI uses a hybrid approach:

1. **Legacy Dialogs** (`src/ui/dialogs/`) - C++ dialog classes
2. **Data-Driven Dialogs** (`src/ui/dialog_manager.*`) - YAML-defined dialogs
3. **Screens** (`src/ui/screens/`) - Full-screen sprite-based UI (login, menus)

### Dialog Documentation

- `docs/dialog_system.md` - Comprehensive dialog system docs
- `docs/dialog_quick_reference.md` - Quick reference cheat sheet

---

## System Documentation

| System | Documentation |
|--------|--------------|
| Character Movement | `docs/character_movement.md` |
| Character Animation | `docs/character_animation.md` |
| Character Rendering | `docs/character_rendering.md` |
| Sound & Music | `docs/sound_system.md` |
| Map Rendering | `docs/map_rendering.md` |
| Camera & Zoom | `docs/camera_system.md` |
| Character Sounds | `docs/character_sounds.md` |
| Dialog System | `docs/dialog_system.md` |
| AMD File Format | `docs/amd_file_format.md` |

---

## Network Protocol

The client supports two protocols:

1. **Legacy Binary** - Original Helbreath packet format for game server
2. **WebSocket JSON** - Modern JSON protocol for auth server

### Key Files

| File | Purpose |
|------|---------|
| `src/network/network_system.*` | Legacy packet handling |
| `src/network/websocket_connection.*` | WebSocket client |
| `src/network/messages.h` | JSON message types |
| `src/gameplay/game_state.cpp` | Message routing |

### Protocol Documentation

The JSON protocol specification for WebSocket communication is documented in `../server/docs/JSON_PROTOCOL.md`.

---

## Legacy Exclusions

### Equilibrium System

The legacy Helbreath codebase contains an "Equilibrium" system (also referred to as "EQ", "balance mode", or similar). **Do not implement or port equilibrium-related code by default.** When you encounter anything related to equilibrium during research or implementation — whether in legacy code references, oracle search results, or design docs — **always ask the user** whether it should be included before proceeding. In most cases the answer will be no, but occasionally it may be desired.

---

## Migration Strategy

### Completed
- [x] CMake build system with vcpkg
- [x] SFML rendering backend
- [x] Basic UI system with dialogs
- [x] WebSocket authentication
- [x] Login and character select flow

### In Progress
- [ ] Full game state implementation
- [ ] Entity rendering and animation
- [ ] Combat and magic systems

### Planned
- [ ] Complete all 41 dialog types
- [ ] Full network protocol support
- [ ] Audio system
- [ ] Localization

---

## Client-Specific Notes

When working on the client:

1. **UI thread safety** - Never modify `dialog_order_` from network callbacks
2. **Sprite memory** - Use `sprite_manager` for automatic memory management
3. **Dialog creation** - Prefer YAML definitions over C++ dialog classes
4. **Screen rendering** - Login/menu screens use sprite-based rendering
5. **Input routing** - Check modal dialogs before processing game input

---

## Documentation Requirements

### Protocol Changes

Any change to the client/server protocol **MUST** be documented in:
- `docs/JSON_PROTOCOL.md` - Message format specification
- `docs/protocol/` - Detailed protocol documentation

### Progress Tracking

- Major features **MUST** be checked off in `docs/PROGRESS.md`
- All changes **SHOULD** be logged in `docs/PROGRESS.md` under `## Recent Changes` using:

```
### YYYY-MM-DD: Summary
- Individual items
- Individual items
...
```

---

## AI Assistant Guidelines

When working on this codebase:

1. **Always use C++20 features** - Prefer modern alternatives to legacy patterns
2. **Follow the coding style strictly** - snake_case everywhere, no Hungarian notation, Allman braces
3. **Check existing code first** - Patterns may already exist
4. **Use RAII** - Never use raw `new`/`delete`
5. **Handle errors gracefully** - Use `std::expected` or result types
6. **Keep functions small** - Each function should do one thing well
7. **Log important operations** - But avoid excessive logging in hot paths
8. **Test edge cases** - Especially around combat, items, and networking
9. **Preserve game behavior** - Modernized code should behave identically to original
10. **Update documentation** - Keep CLAUDE.md and docs/ in sync with changes
11. **Translate Korean comments** - When encountered, translate to English inline
12. **Update PROGRESS.md** - After completing a feature, update the relevant `docs/PROGRESS.md` to mark items as done and add a dated changelog entry
13. **Ask questions frequently** - Use AskUserQuestion liberally to ensure proper guidance. Before starting non-trivial tasks, clarify requirements and approach. When multiple implementation options exist, ask for preference. If requirements are ambiguous, ask rather than assume.
