# Legacy Helbreath Client Documentation Index

This index tracks the comprehensive documentation effort for the legacy Helbreath client codebase (circa 2002-2003). The goal is to fully document all systems before modernization to C++20.

**Original Codebase Stats:**
- ~48,500 lines in Game.cpp alone
- 41 header files, 27 source files
- DirectX 7 (DirectDraw, DirectInput, DirectSound)
- WinSock2 networking
- C++98 era code

---

## Documentation Progress

| # | Filename | Description | Status |
|---|----------|-------------|--------|
| 01 | `01_cgame_monolithic_class.md` | The 48,500+ line CGame class structure, responsibilities, and member organization | Done |
| 02 | `02_global_definitions_and_constants.md` | GlobalDef.h, GlobalVal.h, macros, resource limits, and compile-time constants | Done |
| 03 | `03_game_state_machine.md` | 21 game modes/states, transitions, and mode-specific logic | Done |
| 04 | `04_pak_file_system.md` | PAK archive format, asset extraction, and resource file management | Done |
| 05 | `05_directdraw7_wrapper.md` | DXC_ddraw surface management, 16-bit color, alpha blending levels | Done |
| 06 | `06_sprite_system.md` | Sprite.h/cpp, frame animation, rendering modes, collision detection | Done |
| 07 | `07_directsound7_audio.md` | YWSound, SoundBuffer, sound categories, 110 concurrent sound limit | Done |
| 08 | `08_directinput7_input.md` | DXC_dinput keyboard/mouse handling, 80 action ID mappings | Done |
| 09 | `09_xsocket_networking.md` | WinSock2 wrapper, async events, message buffering, connection states | Done |
| 10 | `10_network_protocol.md` | NetMessages.h packet types, request/response/notify message definitions | Done |
| 11 | `11_character_system.md` | CharInfo.h/cpp, stats, appearance, creation, player properties | Done |
| 12 | `12_combat_system.md` | Attack handling, damage calculation, combat modes, safe/force attack | Done |
| 13 | `13_magic_system.md` | 100+ spells, 23 magic types, mana costs, mastery, spell learning | Done |
| 14 | `14_skill_system.md` | 60 skills, training mechanics, mastery levels, skill-based abilities | Done |
| 15 | `15_inventory_system.md` | Item.h/cpp, 12 item types, 50 inventory slots, item effects/attributes | Done |
| 16 | `16_equipment_system.md` | 15 equipment slots, stat bonuses, gender/level restrictions, durability | Done |
| 17 | `17_crafting_system.md` | BuildItem.h/cpp, 100 recipes, 6 ingredient slots, skill requirements | Done |
| 18 | `18_dialog_system_overview.md` | 41 dialog types, common patterns, rendering, input handling, data binding | Done |
| 19 | `19_dialog_character_stats.md` | Character info dialog, stat display, level-up point allocation | Done |
| 20 | `20_dialog_inventory_bank.md` | Inventory dialog, bank dialog, item grid management, drag-drop | Done |
| 21 | `21_dialog_magic_skills.md` | Spellbook dialog, skills dialog, magic shop, spell/skill selection | Done |
| 22 | `22_dialog_social.md` | Guild menu, guild operations, party list, chat window dialogs | Done |
| 23 | `23_dialog_commerce.md` | Shop dialog, sell/repair, item exchange, pricing display | Done |
| 24 | `24_dialog_navigation.md` | Mini-map, world guide, teleportation selection dialogs | Done |
| 25 | `25_dialog_system_misc.md` | System menu, help, warnings, shutdown, age verification dialogs | Done |
| 26 | `26_map_world_system.md` | MapData.h/cpp, 752x752 tile grid, walkability, dynamic objects | Done |
| 27 | `27_tile_rendering.md` | Tile.h, TileSpr.h, tile properties, object frames, map drawing | Done |
| 28 | `28_entity_management.md` | Player/NPC/monster entities, entity IDs, position tracking, spawning | Done |
| 29 | `29_effects_system.md` | Effect.h/cpp, 300 concurrent effects, spell visuals, particles | Done |
| 30 | `30_weather_environment.md` | 7 weather types, day/night cycle, environmental hazards | Done |
| 31 | `31_party_system.md` | 8-member parties, status tracking, party abilities, join/leave | Done |
| 32 | `32_guild_system.md` | 32-member guilds, ranks, permissions, guild operations | Done |
| 33 | `33_chat_system.md` | Message types, 500 message history, whispers, bad word filtering | Done |
| 34 | `34_crusade_war_system.md` | 300 structures, duties (Commander/Constructor/Soldier), rewards | Done |
| 35 | `35_quest_system.md` | Quest acceptance, tracking, completion, contribution, rewards | Done |
| 36 | `36_fishing_system.md` | Fishing minigame mechanics, success/failure, rewards | Done |
| 37 | `37_teleportation_system.md` | Teleport locations, costs, restrictions, UI flow | Done |
| 38 | `38_pk_rating_system.md` | Player killing rules, reputation/rating, penalties, criminal status | Done |
| 39 | `39_localization_system.md` | 5 languages (EN/KO/JA/ZH-CN/ZH-TW), compile-time string defines | Done |
| 40 | `40_configuration_persistence.md` | login.cfg, game settings, item name database, saved preferences | Done |
| 41 | `41_message_handlers.md` | All packet processing handlers in CGame, server message dispatch | Done |

---

## Documentation Standards

Each document should follow this structure:

```markdown
# [System Name]

## Overview
Brief description of what this system does and its role in the game.

## Source Files
- `filename.h` - Description
- `filename.cpp` - Description

## Key Data Structures
Document structs, classes, enums, and important typedefs.

## Core Functions
Document the main functions with signatures and explanations.

## Constants & Limits
List important constants, magic numbers, and resource limits.

## Integration Points
How this system interacts with other systems.

## State Management
How state is stored and modified (if applicable).

## Known Issues / Technical Debt
Legacy quirks, hardcoded values, areas needing modernization.

## Modernization Notes
Suggestions for the C++20 port.
```

---

## Quick Reference

### File Locations (Legacy)
| File | Purpose |
|------|---------|
| `Game.cpp` / `Game.h` | Main CGame class (48,500+ lines) |
| `GlobalDef.h` | Global definitions and macros |
| `GlobalVal.h` | Global variables |
| `DXC_ddraw.h/.cpp` | DirectDraw 7 wrapper |
| `DXC_dinput.h/.cpp` | DirectInput 7 wrapper |
| `YWSound.h/.cpp` | DirectSound 7 wrapper |
| `SoundBuffer.h/.cpp` | Sound effect management |
| `Sprite.h/.cpp` | Sprite loading and rendering |
| `Tile.h` / `TileSpr.h` | Tile definitions |
| `MapData.h/.cpp` | Map and world data |
| `XSocket.h/.cpp` | WinSock2 networking |
| `NetMessages.h` | Network protocol definitions |
| `Item.h/.cpp` | Item properties |
| `CharInfo.h/.cpp` | Character information |
| `Magic.h/.cpp` | Magic/spell definitions |
| `Skill.h/.cpp` | Skill definitions |
| `Effect.h/.cpp` | Visual effects |
| `BuildItem.h/.cpp` | Crafting system |
| `Msg.h/.cpp` | Chat message wrapper |
| `ItemName.h/.cpp` | Item name database |
| `ActionID.h` | Input action mappings |
| `SoundID.h` | Sound effect IDs |
| `lan_*.h` | Localization strings (5 files) |

### Key Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| Max Sprites | 20,000 | Sprite resource limit |
| Max Sounds | 110 | Concurrent sound effects |
| Max Effects | 300 | Concurrent visual effects |
| Map Size | 752x752 | Tile grid dimensions |
| Viewport | 25x19 | Visible tiles (800x600 @ 32px) |
| Inventory Slots | 50 | Player inventory capacity |
| Bank Slots | 121 | Bank storage capacity |
| Equipment Slots | 15 | Wearable item slots |
| Party Members | 8 | Maximum party size |
| Guild Members | 32 | Maximum guild size |
| Chat History | 500 | Stored chat messages |
| Magic Types | 100+ | Spell definitions |
| Skill Types | 60 | Skill definitions |
| Dialog Types | 41 | UI dialog boxes |
| Game Modes | 21 | State machine states |

---

## Completion Summary

- **Total Systems:** 41
- **Documented:** 41
- **Pending:** 0
- **Progress:** 100%

---

## Notes

- Documentation is for the **legacy codebase only**, not the modern C++20 port
- Focus on understanding existing behavior for accurate modernization
- Protocol documentation is critical for server compatibility
- UI dialog documentation helps preserve exact game feel
