# Helbreath Client

> **See also:** [`../CLAUDE.md`](../CLAUDE.md) for shared coding standards, C++20 guidelines, and memory safety patterns.

## Project Overview

The Helbreath game client - a 2D MMORPG client originally developed circa 2002-2003, being modernized to C++20 with SFML.

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

**Ask questions frequently.** Use AskUserQuestion liberally to ensure proper guidance:

- Before starting non-trivial tasks, clarify requirements and approach
- When multiple implementation options exist, ask for preference
- If requirements are ambiguous, ask rather than assume
- When making architectural decisions, get confirmation first
- If unsure about existing patterns or conventions, ask

It's better to ask too many questions than to implement the wrong thing.
