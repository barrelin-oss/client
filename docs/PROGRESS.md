# Client Progress

## Core Systems

- [x] CMake build system with vcpkg
- [x] SFML rendering backend
- [x] Application lifecycle
- [x] Configuration system

## Graphics & Rendering

- [x] Sprite/PAK asset loading
- [x] Tile map rendering
- [x] Camera system with zoom
- [x] Day/night overlay
- [x] Weather particle system
- [x] Walk-behind tree transparency
- [x] Hair color tinting
- [x] Additive blending for effects
- [x] RGB tint shader for effects
- [ ] Minimap rendering
- [ ] Full equipment rendering

## Entity System

- [x] Entity component system
- [x] Player rendering and animation
- [x] NPC rendering and animation
- [x] Monster rendering and animation
- [x] Character movement (origin/destination model)
- [x] Monster/NPC sounds
- [x] Entity info debug overlay
- [ ] Dead body / corpse rendering
- [ ] Mount rendering

## Combat

- [x] End-to-end combat system
- [x] Dash attacks
- [ ] Critical hit effects
- [ ] Full damage type display

## Magic & Effects

- [x] Spell effect system
- [x] Projectile effects
- [x] Composite/child effects
- [x] Thunder rendering
- [x] Effect test tool
- [ ] All 100+ spells verified

## UI System

- [x] Dialog framework (YAML + C++)
- [x] Login screen
- [x] Character select screen
- [x] Chat system (tabbed, search, channel switching)
- [x] Chat bubbles
- [x] Settings dialog (tabbed)
- [x] Skills dialog
- [x] Fishing dialog
- [x] Guild dialog
- [ ] Inventory dialog
- [ ] Shop/trade dialogs
- [ ] Party dialog
- [ ] Quest dialogs
- [ ] Remaining dialog types (~30)

## Networking

- [x] Legacy binary packet protocol
- [x] WebSocket JSON protocol
- [x] Launcher authentication
- [x] Reconnect handling
- [x] Server-authoritative hostility/faction
- [ ] Full message handler coverage

## Audio

- [x] Sound effect playback
- [x] Ambient audio
- [x] Monster hurt/death sounds on animation frames
- [ ] Music system
- [ ] Full sound coverage

## Other

- [x] Flatpak packaging
- [x] Fishing system
- [x] Guild system (create, invite, promote/demote, MOTD)
- [ ] Localization
- [ ] Skill training
- [ ] Party system
- [ ] Trade system
- [ ] Crusade/Heldenian events

---

## Recent Changes

### 2026-02-13: Guild system and rendering fixes
- Add guild system with create, invite, promote/demote, and MOTD support
- Add entity info debug overlay with pin-on-click support
- Replace numeric nation with server-authoritative hostility and faction strings
- Add monster sounds and move hurt/death sounds to animation frames
- Fix map loading: case-insensitive AMD lookup and teleport reload
- Remove action_queue subsystem, inline into input_handler
- Fix movement arrival, effect physics, and input handling
- Replace duplicate no-colorkey textures with shader, add RGB tint to effects
- Fix sprite loading, add hair color tinting, and improve entity overlays

### 2026-02-12: Skills overhaul and Flatpak packaging
- Refactor skills system: simplify to mastery-based model with YAML config
- Add Flatpak packaging and fix compiler warnings
- Add dash attacks, faction name colors, and skills dialog overhaul
- Refactor movement to use origin/destination model and improve combat targeting

### 2026-02-11: Fishing and NPC fixes
- Add fishing system with dialog, network messages, and fish node effects
- Fix NPC rendering, positioning, and animation handling
- Fix chill wind effect: correct sprite index, frame count, and child count
- Add additive blending for effects, effect test tool, and definition fixes

### 2026-02-10: Spell effect overhaul
- Overhaul spell effects, thunder rendering, and message handling

### 2026-02-09: Spell effect fixes
- Fix spell effect definitions and correct spell-to-effect mappings

### 2026-02-08: Combat and magic systems
- Implement end-to-end combat system
- Add magic system, ambient audio, weather/tint settings, and reconnect
- Fix spell effect system: projectiles, composites, and spell ID handling
- Implement weather particle system and day/night overlay

### 2026-02-07: Chat, UI, and view system
- Add launcher auth, reconnect screen, and combat message handlers
- Replace system menu with tabbed settings dialog
- Overhaul chat system with tabbed dialog, search, and unified colors
- Add chat bubbles over player heads
- Implement chat input overlay with channel switching and whisper support
- Decouple view radius from internal resolution
- Support separate X/Y view radius for non-square viewports
