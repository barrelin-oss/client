# Sound & Music System

Comprehensive documentation of the two-layer audio architecture in the modern client.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Source Files](#source-files)
- [Types and Enums](#types-and-enums)
- [Low-Level Audio Layer (`audio`)](#low-level-audio-layer-audio)
- [Game-Level Sound Layer (`sound_manager`)](#game-level-sound-layer-sound_manager)
- [Sound Categories](#sound-categories)
- [Spatial Audio](#spatial-audio)
- [Volume System](#volume-system)
- [Enable/Disable Controls](#enabledisable-controls)
- [Music (BGM)](#music-bgm)
- [Sound Loading and Caching](#sound-loading-and-caching)
- [Character Sounds](#character-sounds)
- [UI Sounds](#ui-sounds)
- [System Integration](#system-integration)
- [Configuration and Persistence](#configuration-and-persistence)
- [Settings UI](#settings-ui)
- [Asset Files](#asset-files)
- [Error Handling](#error-handling)
- [Known Gaps](#known-gaps)

---

## Architecture Overview

The sound system has two layers:

1. **`hb::audio`** (`audio/audio.hpp` / `audio/audio.cpp`) -- Low-level SFML wrapper that owns sound buffers, manages active sound instances, and streams music.
2. **`hb::sound_manager`** (`audio/sound_manager.hpp` / `audio/sound_manager.cpp`) -- Game-level logic for spatial audio, sound categories (C/E/M), BGM track selection, and enable/disable toggles.

A third file, **`audio/sound_types.hpp`**, defines the `character_sound` enum and helper functions for gender-based and weapon-based sound selection.

```
application                    game_state_manager
    |                               |
    v                               v
  audio <-------- sound_manager ----+----> combat_system
    |                  |                        |
    |                  v                        v
    |            entity_manager           (triggers sounds
    |                  |                  via sound_manager)
    v                  v
  SFML Audio     (triggers footsteps
                  via sound_manager)
```

**Ownership:**
- `application` owns the `audio` instance, calls `audio::update()` each frame, and applies config volumes.
- `game_state_manager` owns the `sound_manager` instance, initializes it with a reference to `audio`, and wires it to `combat_system` and `entity_manager`.

---

## Source Files

### Core Audio Files

| File | Purpose |
|------|---------|
| `src/audio/audio.hpp` | `hb::audio` class declaration, `sound_id` type alias |
| `src/audio/audio.cpp` | SFML audio wrapper implementation |
| `src/audio/sound_manager.hpp` | `hb::sound_manager` class declaration, `max_sound_distance` and `tile_size` constants |
| `src/audio/sound_manager.cpp` | Game-level sound logic, BGM selection, spatial audio |
| `src/audio/sound_types.hpp` | `character_sound` enum, gender/weapon helper functions |

### Integration Points

| File | Integration |
|------|-------------|
| `src/application.cpp` | Owns `audio`, calls `update()` each frame, applies config |
| `src/gameplay/game_state.cpp` | Owns `sound_manager`, wires callbacks, triggers BGM |
| `src/gameplay/combat.cpp` | Calls `sound_manager` for attack/hurt/death/critical/level-up sounds |
| `src/entity/entity_manager.cpp` | Calls `sound_manager` for footstep sounds |
| `src/core/config.hpp` | Defines `audio_config` struct |
| `src/core/config.cpp` | Loads/saves audio settings from/to JSON |
| `src/ui/dialogs/system_menu_dialog.hpp/cpp` | Settings dialog with music/sound volume sliders |
| `src/ui/screens/screen_base.hpp` | Base class with `sound_callback` for button click sounds |
| `src/ui/screens/main_menu_screen.cpp` | Calls `play_button_sound()` |
| `src/ui/screens/login_screen.cpp` | Calls `play_button_sound()` |
| `src/ui/screens/character_select_screen.cpp` | Calls `play_button_sound()` |
| `src/ui/screens/character_create_screen.cpp` | Calls `play_button_sound()` |
| `src/ui/dialogs/icon_panel_dialog.hpp/cpp` | Plays button sound on icon panel click |
| `src/ui/dialogs/yaml_icon_panel_dialog.hpp/cpp` | Plays button sound on YAML icon panel click |

---

## Types and Enums

### `hb::sound_id` (audio.hpp)

```cpp
using sound_id = uint16_t;
inline constexpr sound_id invalid_sound_id = 0;
```

Monotonically incrementing ID assigned by `audio::load_sound()`. Starts at 1; 0 is the sentinel for "no sound loaded."

### `hb::character_sound` (sound_types.hpp)

```cpp
enum class character_sound : uint8_t
{
    small_sword_swing = 1,   // C1
    large_sword_swing = 2,   // C2
    bow_attack        = 3,   // C3
    arrow_release     = 4,   // C4
    punch             = 5,   // C5
    sword_impact      = 6,   // C6
    arrow_impact      = 7,   // C7
    walk_step         = 8,   // C8
    monster_step      = 9,   // C9
    run_step          = 10,  // C10
    weapon_impact     = 11,  // C11
    male_hurt         = 12,  // C12
    female_hurt       = 13,  // C13
    male_death        = 14,  // C14
    female_death      = 15,  // C15
    magic_cast        = 16,  // C16
    magic_failed      = 17,  // C17
    mace_swing        = 18,  // C18
    female_eat        = 19,  // C19
    male_eat          = 20,  // C20
    male_level_up     = 21,  // C21
    female_level_up   = 22,  // C22
    male_critical     = 23,  // C23
    female_critical   = 24,  // C24
};
```

### `hb::audio_config` (config.hpp)

```cpp
struct audio_config
{
    float master_volume = 1.0f;
    float music_volume  = 0.7f;
    float sfx_volume    = 1.0f;
    bool muted          = false;
    bool music_enabled  = true;
    bool sfx_enabled    = true;
};
```

### Constants

| Constant | Location | Value | Purpose |
|----------|----------|-------|---------|
| `invalid_sound_id` | `audio.hpp` | `0` | Sentinel for "no sound loaded" |
| `max_sound_distance` | `sound_manager.hpp` | `14` | Max audible distance in tiles |
| `tile_size` | `sound_manager.hpp` | `32` | Pixels per tile for position-to-tile conversion |

---

## Low-Level Audio Layer (`audio`)

### Class Members

```cpp
// Active sound wrapper that tracks intended volume for mute/unmute restoration
struct active_sound
{
    std::unique_ptr<sf::Sound> sound;
    float intended_volume = 1.0f;  // volume * sound_volume_ at play time
};

std::unordered_map<sound_id, sf::SoundBuffer> buffers_;    // Loaded sound buffers keyed by ID
std::vector<active_sound> active_sounds_;                    // Currently playing sound instances
sf::Music music_;                                            // Single streaming music track

float master_volume_ = 1.0f;
float sound_volume_  = 1.0f;
float music_volume_  = 1.0f;
bool muted_          = false;

// Music fade state
enum class fade_state : uint8_t { none, fading_out, fading_in };
fade_state fade_state_ = fade_state::none;
float fade_timer_ = 0.0f;
float fade_duration_ = 1.0f;
std::string pending_music_path_;
bool pending_music_loop_ = true;

sound_id next_id_ = 1;   // Auto-incrementing ID generator
```

### Public API

| Method | Signature | Description |
|--------|-----------|-------------|
| `initialize` | `bool initialize()` | Logs init message. Returns true. No SFML setup required beyond default construction. |
| `shutdown` | `void shutdown()` | Stops all sounds and music, clears all buffers and active sounds. |
| `update` | `void update(float delta_time)` | Called each frame. Removes finished sounds from `active_sounds_` and processes music fade state machine. |
| `load_sound` | `sound_id load_sound(std::string_view path)` | Loads a WAV file into an `sf::SoundBuffer`, stores it in `buffers_`. Returns `invalid_sound_id` on failure. |
| `unload_sound` | `void unload_sound(sound_id id)` | Stops all instances of the sound, then erases its buffer. |
| `play_sound` | `void play_sound(sound_id id, float volume = 1.0f, float pan = 0.0f)` | Creates a new `sf::Sound` from the buffer, applies volume and 3D panning, plays it, and adds it to `active_sounds_`. |
| `stop_sound` | `void stop_sound(sound_id id)` | Stops all playing instances that reference the given buffer. |
| `stop_all_sounds` | `void stop_all_sounds()` | Stops and clears all active sounds. |
| `play_music` | `bool play_music(std::string_view path, bool loop = true)` | Opens and streams music from file via `sf::Music`. Sets loop flag and volume. Returns false on failure. |
| `stop_music` | `void stop_music()` | Stops the current music stream. |
| `pause_music` | `void pause_music()` | Pauses the current music stream. |
| `resume_music` | `void resume_music()` | Resumes playback only if currently paused. |
| `is_music_playing` | `bool is_music_playing() const` | Returns true if music status is `Playing`. |
| `crossfade_music` | `void crossfade_music(std::string_view path, bool loop = true, float fade_duration = 1.0f)` | Crossfades to a new music track. If music is playing, fades out then fades in the new track. If no music is playing, starts the new track with a fade-in. |
| `set_music_volume` | `void set_music_volume(float volume)` | Clamps 0-1, updates `music_volume_`. Only applies to `sf::Music` immediately if no fade is active. |
| `set_master_volume` | `void set_master_volume(float volume)` | Clamps 0-1, updates `master_volume_`, recalculates volume on music and all active sounds. |
| `set_sound_volume` | `void set_sound_volume(float volume)` | Clamps 0-1, stores `sound_volume_` (applied to future `play_sound` calls). |
| `set_muted` | `void set_muted(bool muted)` | When muting, sets all volumes to 0. When unmuting, restores both music volume and all active sound volumes using stored `intended_volume`. |
| `master_volume` | `float master_volume() const` | Returns current master volume. |
| `sound_volume` | `float sound_volume() const` | Returns current SFX volume. |
| `is_muted` | `bool is_muted() const` | Returns muted state. |

### Private Methods

| Method | Description |
|--------|-------------|
| `effective_volume(float base_volume)` | Returns `0.0f` if muted, else `base_volume * master_volume_`. |

### Volume Calculation

When `play_sound` is called, the intended volume is computed and stored:
```cpp
float intended = volume * sound_volume_;
sound->setVolume(effective_volume(intended) * 100.0f);
```
- `intended` = `volume * sound_volume_` (distance-attenuated volume times SFX category volume)
- `effective_volume(intended)` = `intended * master_volume_` (or 0 if muted)
- Multiplied by `100.0f` to convert from 0.0-1.0 range to SFML's 0-100 scale
- The `intended_volume` is stored in the `active_sound` struct for later restoration on unmute or master volume change

### Stereo Panning

Panning uses SFML's 3D audio positioning with `setRelativeToListener(true)`:
```cpp
sound->setPosition({pan * 10.0f, 0.0f, 0.0f});
```
- `pan` ranges from -1.0 (left) to +1.0 (right)
- The `10.0f` multiplier spaces the position along the X axis for audible stereo separation

### Sound Instance Lifecycle

Each call to `play_sound` creates a new `sf::Sound` wrapped in an `active_sound` struct (which also stores the `intended_volume`). Instances are stored in `active_sounds_` and cleaned up by `update()` once their status becomes `Stopped`. There is no limit on concurrent sounds.

---

## Game-Level Sound Layer (`sound_manager`)

### Class Members

```cpp
audio* audio_ = nullptr;

std::array<sound_id, 25> character_sounds_{};         // C1-C24 (index 0 unused)
std::array<sound_id, 60> effect_sounds_{};            // E1-E53+ (index 0 unused, room for expansion)
std::unordered_map<int, sound_id> monster_sounds_;    // M1-M156 (sparse)

std::string current_bgm_track_;
int32_t listener_x_ = 0;
int32_t listener_y_ = 0;
bool sfx_enabled_   = true;
bool music_enabled_  = true;
std::string sound_dir_;   // "assets/data/sounds/"
std::string music_dir_;   // "assets/data/music/"
```

### Public API

| Method | Signature | Description |
|--------|-----------|-------------|
| `initialize` | `bool initialize(audio& audio_system)` | Stores audio reference, sets paths, fills arrays with `invalid_sound_id`, calls `load_sounds()`. |
| `shutdown` | `void shutdown()` | Stops BGM, unloads all buffers from `audio`, clears all containers. |
| `play_sound` | `void play_sound(char type, int num, int tile_distance, float pan = 0.0f)` | Main spatial sound function. Checks `sfx_enabled_`, distance <= 14, looks up `sound_id`, calculates attenuated volume, delegates to `audio::play_sound`. |
| `play_ui_sound` | `void play_ui_sound(int effect_num)` | Convenience wrapper: `play_sound('E', effect_num, 0, 0.0f)` -- full volume, centered. |
| `play_sound_at` | `void play_sound_at(char type, int num, int32_t world_x, int32_t world_y)` | Converts pixel coords to tiles, computes Chebyshev distance and pan from listener, calls `play_sound()`. |
| `play_character_sound` | `void play_character_sound(character_sound sound, int tile_distance = 0, float pan = 0.0f)` | Type-safe wrapper: `play_sound('C', static_cast<int>(sound), ...)`. |
| `play_character_sound_at` | `void play_character_sound_at(character_sound sound, int32_t world_x, int32_t world_y)` | Type-safe wrapper: `play_sound_at('C', static_cast<int>(sound), ...)`. |
| `set_listener_position` | `void set_listener_position(int32_t world_x, int32_t world_y)` | Sets `listener_x_` and `listener_y_` used by `play_sound_at` for distance/pan calculations. |
| `start_bgm` | `void start_bgm(std::string_view map_name, int weather_type = 0)` | Selects track via `select_bgm_track()`, skips if same track already playing, crossfades to new track. |
| `play_bgm_track` | `void play_bgm_track(std::string_view track, bool loop = true)` | Plays a named track file directly (skips map-name-to-track selection). Uses `crossfade_music()` internally. |
| `stop_bgm` | `void stop_bgm()` | Stops music, clears `current_bgm_track_`. |
| `is_bgm_playing` | `bool is_bgm_playing() const` | Delegates to `audio::is_music_playing()`. |
| `set_sfx_enabled` | `void set_sfx_enabled(bool enabled)` | Sets flag. If disabled, calls `audio::stop_all_sounds()`. |
| `set_music_enabled` | `void set_music_enabled(bool enabled)` | Sets flag. If disabled, stops music and clears track. Re-enabling does not auto-restart BGM. |
| `is_sfx_enabled` | `bool is_sfx_enabled() const` | Returns `sfx_enabled_`. |
| `is_music_enabled` | `bool is_music_enabled() const` | Returns `music_enabled_`. |

### Private Methods

| Method | Description |
|--------|-------------|
| `load_sounds()` | Scans `sound_dir_` for `.wav` files matching `[CEM]\d+.wav`, parses prefix/number, loads into `audio`, stores IDs. |
| `find_and_load_sound(char prefix, int num)` | Fallback on-demand loader (currently unused). Scans directory for matching filename. |
| `get_sound_id(char type, int num) const` | Looks up `sound_id` from the appropriate container (`character_sounds_`, `effect_sounds_`, or `monster_sounds_`). |
| `select_bgm_track(std::string_view map_name, int weather_type) const` | Map-to-music mapping logic (see [Music (BGM)](#music-bgm)). |

---

## Sound Categories

Sounds are loaded from `assets/data/sounds/` and identified by a letter prefix + number.

| Category | Prefix | Range | Storage | Count on Disk |
|----------|--------|-------|---------|---------------|
| Character | `C` | C1-C24 | `std::array<sound_id, 25>` (index 0 unused) | 24 files |
| Effect | `E` | E1-E53 | `std::array<sound_id, 60>` (index 0 unused) | 52 files (E48 missing) |
| Monster | `M` | M1-M156 | `std::unordered_map<int, sound_id>` (sparse) | ~155 files |

All sounds are WAV files loaded entirely into memory at startup via `sound_manager::load_sounds()`.

---

## Spatial Audio

### Distance Attenuation

Sounds played via `play_sound()` or `play_sound_at()` are attenuated by distance from the listener:

- **Distance metric:** Chebyshev distance -- `max(|dx|, |dy|)` in tile units
- **Max audible distance:** 14 tiles (`max_sound_distance`)
- **Attenuation:** Linear -- `volume = 1.0 - (tile_distance / 14.0)`
- **Beyond max distance:** Sound is not played at all (early return)
- **Distance 0:** Full volume (1.0)

```
Volume
1.0 |*
    | *
    |  *
0.5 |   *
    |    *
    |     *
0.0 +------*------
    0  7  14  tiles
```

### Stereo Panning

For `play_sound_at()`, panning is calculated from the horizontal tile offset:

```cpp
float pan = clamp(dx / max_sound_distance, -1.0, 1.0);
```

- `-1.0` = full left
- `0.0` = center
- `+1.0` = full right

Where `dx = sound_tile_x - listener_tile_x`.

### Listener Position

The listener position is stored as world pixel coordinates (`listener_x_`, `listener_y_`), set via `set_listener_position()`. The `play_sound_at()` method converts both the listener and sound positions to tile coordinates (dividing by `tile_size = 32`) before computing distance and pan.

---

## Volume System

### Volume Hierarchy

Final volume for a sound effect:

```
effective_volume = base_volume * master_volume * sound_volume * 100
                   ~~~~~~~~~~~~   ~~~~~~~~~~~~~   ~~~~~~~~~~~~   ~~~
                   distance-based  global master   SFX category   SFML scale
```

Where `base_volume = 1.0 - (tile_distance / 14.0)`.

For music:

```
effective_volume = music_volume * master_volume * 100
```

### Three Independent Volume Controls

| Control | Range | Default | Set By | Affects |
|---------|-------|---------|--------|---------|
| `master_volume_` | 0.0-1.0 | 1.0 | `audio::set_master_volume()` | Everything (SFX and music) |
| `sound_volume_` | 0.0-1.0 | 1.0 | `audio::set_sound_volume()` | SFX only |
| `music_volume_` | 0.0-1.0 | 0.7 (config default) | `audio::set_music_volume()` | Music only |

### Master Volume Updates

When `set_master_volume()` is called, it immediately:
1. Recalculates and applies the music volume to `sf::Music`
2. Iterates all `active_sounds_` and recalculates their volumes

### Mute Behavior

When `set_muted(true)`:
- Music volume set to 0
- All active sound volumes set to 0

When `set_muted(false)`:
- Music volume is restored
- All active sound volumes are restored using their stored `intended_volume`

---

## Enable/Disable Controls

Two independent toggles exist at the `sound_manager` level:

| Toggle | Member | Effect When Disabled |
|--------|--------|---------------------|
| SFX | `sfx_enabled_` | All `play_sound*()` methods return immediately; `audio::stop_all_sounds()` is called |
| Music | `music_enabled_` | `start_bgm()` returns immediately; current music is stopped and track name cleared |

These are separate from the `muted_` flag on `audio`, which silences everything at the SFML level.

Re-enabling music does **not** auto-restart BGM -- the caller must call `start_bgm()` again.

---

## Music (BGM)

### Streaming Playback

Music uses `sf::Music` for streaming playback (reads from disk, not loaded into memory). There is a single `sf::Music` member in `audio`, so only one music track can play at a time.

All BGM loops by default (`loop = true`).

### Track Selection

The `select_bgm_track()` method maps the current map name and weather type to a music file. Map name comparison is case-insensitive.

| Priority | Condition | Track |
|----------|-----------|-------|
| 1 | Christmas weather (types 4-6) | `carol.ogg` |
| 2 | Map name contains "aresden" | `aresden.ogg` |
| 3 | Map name contains "elvine" | `elvine.ogg` |
| 4 | Map name starts with "dglv" or "middled1" | `dungeon.ogg` |
| 5 | Map name contains "middleland" | `middleland.ogg` |
| 6 | Map name contains "abaddon" | `abaddon.ogg` |
| 7 | Default (no match) | `maintm.ogg` |

### BGM Triggers

BGM is started and stopped at these points in `game_state.cpp`:

| Event | Action | Location |
|-------|--------|----------|
| Enter "main_menu" state | `sounds_.play_bgm_track("title-screen.ogg")` | `game_state.cpp:enter_state()` |
| Enter "playing" state | `sounds_.start_bgm(map_name, weather)` | `game_state.cpp:enter_state()` |
| Map change (world event) | `sounds_.start_bgm(new_map, weather)` | `game_state.cpp` |
| Weather change to Christmas (types 4-6) | `sounds_.start_bgm(map_name, weather)` | `game_state.cpp` |
| Exit "playing" state | `sounds_.stop_bgm()` | `game_state.cpp:exit_state()` |
| Shutdown | `sounds_.shutdown()` (stops BGM internally) | `game_state.cpp` |

Title screen music persists across menu screens (login, character select, character create) and crossfades naturally when entering the `playing` state.

### Track Change Behavior

- If the same track is already playing, `start_bgm()` and `play_bgm_track()` return early without restarting.
- Track changes use crossfading via `audio::crossfade_music()`:
  - If music is currently playing, the old track fades out linearly over `fade_duration` (default 1.0s), then the new track fades in over the same duration.
  - If no music is playing, the new track starts with a fade-in.
  - The fade state machine runs inside `audio::update(delta_time)` each frame.

---

## Sound Loading and Caching

### Loading Flow

1. **`application::initialize()`** creates the `audio` instance, calls `audio::initialize()`, and applies config volumes.
2. **`game_state_manager::initialize()`** calls `sounds_.initialize(*audio_)` which:
   - Sets paths: `sound_dir_ = "assets/data/sounds/"`, `music_dir_ = "assets/data/music/"`
   - Fills `character_sounds_` and `effect_sounds_` arrays with `invalid_sound_id`
   - Calls `load_sounds()`
3. **`load_sounds()`** iterates `assets/data/sounds/` via `std::filesystem::directory_iterator`:
   - Filters for `.wav` files only (case-insensitive extension check)
   - Parses filename prefix (`C`, `E`, or `M`) and number (via `std::stoi`)
   - Skips malformed filenames (try-catch around `std::stoi`)
   - Calls `audio::load_sound(path)` which loads the entire WAV into an `sf::SoundBuffer`
   - Stores the returned `sound_id` in the appropriate container

### Caching Strategy

- **Eager loading:** All sounds are loaded into memory at startup. No lazy loading in the hot path.
- **No eviction:** Sound buffers persist for the application lifetime until `shutdown()`.
- **`find_and_load_sound()`** exists as a fallback on-demand loader but is currently unused.

### Playback Flow

```
Caller (combat/entity/UI)
  |
  v
sound_manager::play_sound()  or  play_sound_at()  or  play_character_sound_at()
  |
  +-- Check sfx_enabled_ flag (early return if false)
  +-- Check distance <= max_sound_distance (early return if beyond)
  +-- Look up sound_id via get_sound_id(type, num)
  +-- Calculate volume: 1.0 - (distance / 14.0)
  +-- Clamp pan to [-1.0, 1.0]
  |
  v
audio::play_sound(id, volume, pan)
  |
  +-- Check muted_ flag (early return if true)
  +-- Look up sf::SoundBuffer from buffers_ map
  +-- Create new sf::Sound (std::make_unique)
  +-- Compute intended_volume = volume * sound_volume_
  +-- Set volume: effective_volume(intended_volume) * 100.0
  +-- Set position: {pan * 10.0, 0, 0} relative to listener
  +-- Call sound->play()
  +-- Push active_sound{sound, intended_volume} to active_sounds_
```

### Cleanup

`audio::update(delta_time)` is called each frame from `application::main_loop()` (`application.cpp:209`). It removes finished sounds and processes the music fade state machine:

```cpp
active_sounds_.erase(
    std::remove_if(active_sounds_.begin(), active_sounds_.end(),
        [](const active_sound& s) {
            return s.sound->getStatus() == sf::Sound::Status::Stopped;
        }),
    active_sounds_.end()
);
```

---

## Character Sounds

Character sounds (C1-C24) cover combat, movement, damage, magic, consumables, and level-up events.

For a complete character sound reference table with detailed descriptions, see `docs/character_sounds.md`.

### Combat Sounds

| Sound | Enum | Trigger | Where Triggered |
|-------|------|---------|-----------------|
| C1 | `small_sword_swing` | Attack with weapon types 1-2 | `combat.cpp:play_attack_sound()` |
| C2 | `large_sword_swing` | Attack with weapon types 3-19 | `combat.cpp:play_attack_sound()` |
| C3 | `bow_attack` | Attack with weapon types 40-59 | `combat.cpp:play_attack_sound()` |
| C18 | `mace_swing` | Attack with weapon types 20-39 | `combat.cpp:play_attack_sound()` |
| C23 | `male_critical` | Male super attack (type >= `super_1`) | `combat.cpp:play_critical_sound()` |
| C24 | `female_critical` | Female super attack (type >= `super_1`) | `combat.cpp:play_critical_sound()` |

### Damage/Death Sounds

| Sound | Enum | Trigger | Where Triggered |
|-------|------|---------|-----------------|
| C12 | `male_hurt` | Male player takes damage | `combat.cpp:play_hurt_sound()` |
| C13 | `female_hurt` | Female player takes damage | `combat.cpp:play_hurt_sound()` |
| C14 | `male_death` | Male player dies | `combat.cpp:play_death_sound()` |
| C15 | `female_death` | Female player dies | `combat.cpp:play_death_sound()` |

Hurt sounds are also played directly in `game_state.cpp:2914` when the server rejects movement with "blocked_occupied" -- `sounds_.play_sound('C', sound_num, 0)` where `sound_num` is 12 (male) or 13 (female) based on `sprite.gender`.

### Movement Sounds

| Sound | Enum | Trigger | Where Triggered |
|-------|------|---------|-----------------|
| C8 | `walk_step` | Walking animation frames 1 and 3 | `entity_manager.cpp:play_footstep_sound()` |
| C10 | `run_step` | Running animation frames 1 and 3 | `entity_manager.cpp:play_footstep_sound()` |

Footstep sounds are only played for entities of type `player` or `character` (not monsters or NPCs).

### Level-Up Sounds

| Sound | Enum | Trigger | Where Triggered |
|-------|------|---------|-----------------|
| C21 | `male_level_up` | Male gains a level | `combat.cpp:play_level_up_sound()` |
| C22 | `female_level_up` | Female gains a level | `combat.cpp:play_level_up_sound()` |

### Currently Unused Character Sounds

These sounds have enum entries and WAV files on disk but are not triggered by any code:

| Sound | Enum | Intended Use |
|-------|------|--------------|
| C4 | `arrow_release` | Ranged projectile fired |
| C5 | `punch` | Unarmed attack (weapon type 0 -- mapped in `get_weapon_swing_sound` but weapon_type currently defaults to 0/unarmed which produces this) |
| C6 | `sword_impact` | Melee damage impact on target |
| C7 | `arrow_impact` | Ranged damage impact on target |
| C9 | `monster_step` | Monster footstep |
| C11 | `weapon_impact` | Generic weapon impact / Rudolph mount attack |
| C16 | `magic_cast` | Spell casting begins |
| C17 | `magic_failed` | Spell casting fails |
| C19 | `female_eat` | Female consumes food item |
| C20 | `male_eat` | Male consumes food item |

### Weapon Type to Sound Mapping

Defined in `get_weapon_swing_sound()` in `sound_types.hpp`:

| Weapon Type Range | Sound | Category |
|-------------------|-------|----------|
| 0 | C5 (punch) | Unarmed |
| 1-2 | C1 (small sword swing) | Two-hand |
| 3-19 | C2 (large sword swing) | Swords |
| 20-39 | C18 (mace swing) | Axes/Maces/Hammers |
| 40-59 | C3 (bow attack) | Bows/Crossbows |
| Other | C5 (fallback) | Unknown |

### Gender Detection

Player types determine gendered sounds. Defined in `sound_types.hpp`:

| Player Type | Gender | Race |
|-------------|--------|------|
| 1 | Male | Human |
| 2 | Male | Elf |
| 3 | Male | Dark Elf |
| 4 | Female | Human |
| 5 | Female | Elf |
| 6 | Female | Dark Elf |

In `combat.cpp:get_player_type()`, gender is derived from `entity::sprite().gender`:
- `gender <= 1` -> player_type 1 (male)
- `gender == 2` -> player_type 4 (female)

### Helper Functions (sound_types.hpp)

| Function | Returns |
|----------|---------|
| `is_male_player_type(int)` | `true` for types 1-3 |
| `is_female_player_type(int)` | `true` for types 4-6 |
| `get_hurt_sound(int player_type)` | `male_hurt` or `female_hurt` |
| `get_death_sound(int player_type)` | `male_death` or `female_death` |
| `get_level_up_sound(int player_type)` | `male_level_up` or `female_level_up` |
| `get_eat_sound(int player_type)` | `male_eat` or `female_eat` |
| `get_critical_sound(int player_type)` | `male_critical` or `female_critical` |
| `get_weapon_swing_sound(int weapon_type)` | Weapon-appropriate swing sound |
| `is_ranged_weapon(int weapon_type)` | `true` for types 40-59 |

---

## UI Sounds

### Button Click Sound

The only UI sound currently in use is **E14** (`E14.WAV`), played at full volume with no spatial attenuation:

```cpp
sounds_.play_ui_sound(14);
```

### Where Button Sounds Are Triggered

**Menu screens** (via `screen_base::play_button_sound()`):
- `main_menu_screen` -- on every button click and key press
- `login_screen` -- on button clicks, Enter, and Escape
- `character_select_screen` -- on button clicks and Enter
- `character_create_screen` -- on button clicks and Enter

**Icon panels** (via `sound_callback on_button_sound_`):
- `icon_panel_dialog` -- on every icon button click
- `yaml_icon_panel_dialog` -- on every icon button click

### Callback Wiring

All screen and icon panel button sound callbacks are wired in `game_state.cpp:163-167` and `game_state.cpp:2065/2153`:

```cpp
auto play_ui_sound = [this]() { sounds_.play_ui_sound(14); };
screens_.get_main_menu_screen().set_on_button_sound(play_ui_sound);
screens_.get_login_screen().set_on_button_sound(play_ui_sound);
screens_.get_character_select_screen().set_on_button_sound(play_ui_sound);
screens_.get_character_create_screen().set_on_button_sound(play_ui_sound);
```

There are no other UI sounds (no hover, error, dialog open/close, or confirmation sounds).

---

## System Integration

### Application Layer

```
application::initialize()
    |-- audio_.initialize()
    |-- audio_.set_master_volume(config.master_volume)
    |-- audio_.set_music_volume(config.music_volume)
    |-- audio_.set_sound_volume(config.sfx_volume)
    `-- audio_.set_muted(config.muted)

application::main_loop()
    `-- audio_.update(delta_time)   // Each frame: clean up finished sounds, process fades

application::apply_config()
    |-- audio_.set_master_volume(...)
    |-- audio_.set_music_volume(...)
    |-- audio_.set_sound_volume(...)
    `-- audio_.set_muted(...)

application::shutdown()
    `-- audio_.shutdown()
```

### Game State Manager

```
game_state_manager::initialize()
    |-- sounds_.initialize(*audio_)
    |-- sounds_.set_sfx_enabled(config.sfx_enabled)
    |-- sounds_.set_music_enabled(config.music_enabled)
    |-- entities_.set_sound_manager(&sounds_)
    |-- combat_.initialize(..., &sounds_)
    `-- Wire UI sound callbacks

game_state_manager state transitions:
    |-- Enter "main_menu" -> sounds_.play_bgm_track("title-screen.ogg")
    |-- Enter "playing"   -> sounds_.start_bgm(map, weather)
    `-- Exit "playing"    -> sounds_.stop_bgm()

World events:
    |-- on_map_changed    -> sounds_.start_bgm(new_map, weather)
    `-- on_weather_changed -> sounds_.start_bgm(...) if Christmas weather

game_state_manager::shutdown()
    `-- sounds_.shutdown()
```

### Combat System

The `combat_system` holds a `sound_manager*` pointer and an `inventory_system*` pointer (both set during `initialize()`). The weapon type for attack sounds is determined from the equipped weapon in the `right_hand` slot (two-handed weapons also use `right_hand`). Five private methods handle sound playback:

| Method | Trigger | Sound Played |
|--------|---------|--------------|
| `play_attack_sound(entity_id, weapon_type)` | `start_attack()` | Weapon swing at entity position |
| `play_critical_sound(entity_id)` | `start_attack()` for super attacks | C23/C24 at entity position |
| `play_hurt_sound(entity_id)` | `apply_damage()` | C12/C13 at target position |
| `play_death_sound(entity_id)` | `kill_entity()` | C14/C15 at target position |
| `play_level_up_sound(entity_id)` | `check_level_up()` | C21/C22 at target position |

All combat sound methods:
- Check for null `sounds_` and `entities_` pointers
- Verify the entity exists
- For hurt/death/critical, only play for `entity_type::player` or `entity_type::character` (not monsters)
- Use `play_character_sound_at()` for spatial positioning

### Entity Manager

The `entity_manager` holds a `sound_manager*` pointer (set via `set_sound_manager()`). It has one private method:

```cpp
void play_footstep_sound(const entity& e, bool running);
```

Called during `update_animation()` when walk/run animations hit frames 1 or 3. Only plays for `entity_type::player` and `entity_type::character`.

---

## Configuration and Persistence

### Config File

Audio settings are stored in `config.json` under the `"audio"` key:

```json
{
    "audio": {
        "master_volume": 1.0,
        "music_volume": 0.7,
        "sfx_volume": 1.0,
        "muted": false,
        "music_enabled": true,
        "sfx_enabled": true
    }
}
```

### Load Path

1. `config::load()` reads JSON, populates `audio_config` struct (`config.cpp:76-81`)
2. `application::initialize()` applies volumes to `audio` (`application.cpp:81-84`)
3. `game_state_manager::initialize()` applies enable/disable flags to `sound_manager` (`game_state.cpp:235-236`)

### Save Path

1. Settings dialog slider changes update `config::instance().audio()` in real-time
2. Settings dialog "Apply" button calls `config::instance().save()` to persist to disk
3. `config::save()` serializes `audio_config` fields to JSON (`config.cpp:176-181`)

---

## Settings UI

The `settings_dialog` class (`system_menu_dialog.hpp/cpp`) provides two audio sliders:

| Element | Constant | Label | Range | Description |
|---------|----------|-------|-------|-------------|
| Music slider | `elem_music_slider = 6` | "Music" | 0-100% | Controls `music_volume_` |
| Sound slider | `elem_sound_slider = 7` | "Sound" | 0-100% | Controls `sound_volume_` |

### Slider Behavior

- **Click to drag:** Clicking on a slider starts dragging; releasing the mouse button applies the value
- **Slider dimensions:** 150px wide, 16px tall, positioned at x-offset 120 from dialog bounds
- **Visual elements:** Background track, colored fill, draggable handle, percentage text label
- **Real-time feedback:** Volume changes are applied immediately via callbacks

### Callback Wiring (game_state.cpp:2237-2251)

```cpp
settings_dlg->set_on_music_volume_change([this](float volume) {
    if (audio_) audio_->set_music_volume(volume);
    config::instance().audio().music_volume = volume;
});

settings_dlg->set_on_sound_volume_change([this](float volume) {
    if (audio_) audio_->set_sound_volume(volume);
    config::instance().audio().sfx_volume = volume;
});
```

Volume changes are applied to both the live `audio` instance and the `config` (for persistence on Apply).

---

## Asset Files

### Sound Effects

- **Format:** WAV
- **Directory:** `assets/data/sounds/` (relative to working directory `bin/`)
- **Naming:** `[C|E|M]<number>.WAV` (case-insensitive extension)
- **File counts:**
  - Character (C prefix): 24 files (C1-C24)
  - Effect (E prefix): 52 files (E1-E53, E48 missing)
  - Monster (M prefix): ~155 files (M1-M156, sparse with gaps)
  - **Total:** ~231 WAV files

### Music

- **Format:** OGG Vorbis (streamed from disk)
- **Directory:** `assets/data/music/` (relative to working directory `bin/`)

**Tracks used by `select_bgm_track()`:**

| Track File | Usage |
|------------|-------|
| `title-screen.ogg` | Main menu / title screen |
| `carol.ogg` | Christmas weather (types 4-6) |
| `aresden.ogg` | Aresden maps |
| `elvine.ogg` | Elvine maps |
| `dungeon.ogg` | Dungeon maps (dglv*, middled1*) |
| `middleland.ogg` | Middleland maps |
| `abaddon.ogg` | Abaddon maps |
| `maintm.ogg` | Default / main theme |

**Available but unused tracks:**

`battle1.ogg` through `battle10.ogg`, `boss.ogg`, `carol0.ogg` through `carol2.ogg`, `cityunderattack.ogg`, `druncncity.ogg`, `fire.ogg`, `ice.ogg`, `iceandfire.ogg`, `magic.ogg`, `magicalforest.ogg`, `peace.ogg`

---

## Error Handling

All audio failures are **non-fatal** -- the game continues without sound.

### `audio` class

| Operation | On Failure |
|-----------|-----------|
| `load_sound()` | `sf::SoundBuffer::loadFromFile()` fails: logs `spdlog::error`, returns `invalid_sound_id` |
| `play_sound()` | Muted: returns silently. Unknown ID: logs `spdlog::warn`, returns |
| `play_music()` | `sf::Music::openFromFile()` fails: logs `spdlog::error`, returns `false` |

### `sound_manager` class

| Operation | On Failure |
|-----------|-----------|
| `load_sounds()` | Sound directory missing: logs `spdlog::warn`, returns. Individual file failures: logs `spdlog::warn` |
| `play_sound()` | SFX disabled, null audio, beyond max distance, or invalid sound_id: silent early return |
| `start_bgm()` | Music disabled or null audio: silent early return. Play fails: logs `spdlog::warn`, clears track |
| Filename parsing | `std::stoi` exception: silently skips malformed filenames |

### Application level

| Operation | On Failure |
|-----------|-----------|
| `audio::initialize()` | Logs `spdlog::warn`, continues without sound |
| `sounds_.initialize()` | Logs `spdlog::warn`, continues without sound |

---

## Sound Wiring Status

### Working

| Sound | Trigger | Location |
|-------|---------|----------|
| C8 walk footstep | Frames 1, 3 of walk animation (players only) | `entity_manager::update_animation` |
| C10 run footstep | Frames 1, 3 of run animation (players only) | `entity_manager::update_animation` |
| C12/C13 hurt (male/female) | Frame 5 of damage/damage_move (players) | `entity_manager::update_animation` |
| C14/C15 death (male/female) | Frame 7 of dying animation (players) | `entity_manager::update_animation` |
| C16 magic cast | Frame 1 of magic animation | `entity_manager::update_animation` |
| C1/C2/C3/C5/C18 weapon swing | Attack initiation (local player only, wrong frame) | `combat_system::initiate_attack` |
| C23/C24 critical hit | Attack initiation (local player only, wrong frame) | `combat_system::initiate_attack` |
| C21/C22 level up | Level up event (local player only) | `combat_system::add_experience` |
| M1-M156 monster move | Frame 1 of move/run animation | `entity_manager::update_animation` |
| M1-M156 monster attack | Frame 1 of attack animation | `entity_manager::update_animation` |
| M1-M156 monster damage | Frame 1 or 5 of damage animation (per type) | `entity_manager::update_animation` |
| E14 button click | UI button press | `game_state`, `dialog_callbacks` |

### Known Gaps

1. **Player weapon swing sounds fire at wrong time.** `combat_system::initiate_attack` plays C1/C2/C3/C5/C18 immediately on attack start instead of at the correct animation frame (frame 5 for melee swords, frame 2 for bows, frame 3 for axes). Fix requires storing weapon type on entity so `update_animation` can determine which sound to play.

2. **Player weapon swing sounds only play for local player.** Remote players' attacks (received via network) don't play weapon swing sounds because `combat_system::initiate_attack` is only called locally, and entities don't store weapon type for frame-driven playback. Fix: add `weapon_type` field to entity (from appearance data or attack broadcast).

3. **Player critical hit sounds fire at wrong time and only for local player.** Should play at frame 2 of attack animation. Same root cause as weapon swing: need attack metadata on entity for frame-driven playback.

4. **Level up sound only plays for local player.** Network handlers for stat updates don't go through `combat_system::add_experience`. Fix: trigger from the network message handler that processes level-up notifications.

5. **Most effect sounds loaded but unused.** Only E14 (button click) is triggered. E1-E53 are loaded but not wired to spell effects, environmental sounds, etc.

6. **No battle or boss music.** Multiple tracks exist on disk (battle1-10.ogg, boss.ogg) but `select_bgm_track()` does not map to them.

7. **Magic failed sound not wired.** C17 has an enum entry and WAV file but is not triggered when spell casting fails.

8. **Eat/food sounds not wired.** C19/C20 have enum entries, helper functions, and WAV files but are not triggered by item usage.

9. **Impact sounds not wired.** C6 (sword impact) and C7 (arrow impact) are not played on hit -- only swing sounds play, not impact confirmation.
