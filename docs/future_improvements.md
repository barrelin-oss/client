# Future Improvements Tracker

This document tracks planned enhancements and improvements for the Helbreath client that are not part of the current implementation plan.

## Movement & Navigation

### Pathfinding
- **Description:** Implement A* pathfinding algorithm for navigating around obstacles
- **Current:** Direct line movement only; server handles pathfinding
- **Benefit:** Smoother client-side prediction, automatic navigation around walls
- **Priority:** Medium
- **Complexity:** Medium
- **Files:** `src/world/pathfinding.hpp/cpp` (new), `src/entity/movement_component.hpp`

### Right-Click Direction Change
- **Description:** Face a direction without moving by right-clicking
- **Current:** Can only change direction by moving
- **Benefit:** Better combat positioning, strategic facing without movement
- **Priority:** Low
- **Complexity:** Low
- **Files:** `src/gameplay/game_state.cpp` (handle_movement_input)

### Movement Queuing
- **Description:** Queue multiple waypoints for complex paths
- **Current:** Each click replaces previous destination
- **Benefit:** More sophisticated movement control, especially for auto-navigation
- **Priority:** Low
- **Complexity:** Medium
- **Files:** `src/entity/movement_component.hpp` (path field exists)

### Click-and-Hold Movement
- **Description:** Hold mouse button for continuous movement toward cursor
- **Current:** Must click for each movement
- **Benefit:** More intuitive movement feel, common in modern games
- **Priority:** Low
- **Complexity:** Low
- **Files:** `src/gameplay/game_state.cpp`

### Smart Targeting
- **Description:** Auto-attack enemies when clicking on them
- **Current:** Separate attack input required
- **Benefit:** Streamlined combat input, fewer clicks
- **Priority:** Medium
- **Complexity:** Low
- **Files:** `src/gameplay/game_state.cpp` (handle_combat_input)

## Visual Feedback

### Direction Indicator
- **Description:** Visual arrow showing facing direction (debugging/clarity)
- **Current:** Direction only visible from sprite orientation
- **Benefit:** Clear visual feedback, easier debugging
- **Priority:** Very Low
- **Complexity:** Low
- **Files:** `src/entity/entity_manager.cpp` (render function)
- **Note:** Should be toggleable debug feature

### Movement Path Preview
- **Description:** Show path character will take before clicking
- **Current:** No preview of movement
- **Benefit:** Better UX, see path before committing
- **Priority:** Low
- **Complexity:** Medium
- **Files:** `src/ui/world_overlay.hpp/cpp` (new)

### Destination Marker
- **Description:** Visual indicator showing current movement destination
- **Current:** No visual feedback for where you're going
- **Benefit:** Clarity during movement, especially with latency
- **Priority:** Low
- **Complexity:** Low
- **Files:** `src/world/map_renderer.cpp`

## Animation System

### Animation Timing Modernization
- **Description:** Migrate animation data from `MapData.cpp` to data-driven system
- **Current:** Hardcoded in CMapData constructor (lines 20-44)
- **Future:** JSON/YAML configuration files
- **Benefit:** Easier to modify, better separation of data/code, modding support
- **Priority:** Medium
- **Complexity:** Medium
- **Files:**
  - `data/animations/characters.json` (new)
  - `data/animations/monsters.json` (new)
  - `src/entity/animation_loader.hpp/cpp` (new)
  - Remove from `MapData.cpp`
- **Reference:** See `docs/legacy/28_entity_management.md` for complete timing data

### Animation Blending
- **Description:** Smooth transitions between animation states
- **Current:** Instant snap between animations
- **Benefit:** More polished visual appearance
- **Priority:** Low
- **Complexity:** High
- **Files:** `src/entity/animation_component.hpp`, `src/entity/entity_manager.cpp`

### Diagonal Movement Smoothing
- **Description:** Better interpolation for diagonal movement
- **Current:** Basic linear interpolation
- **Benefit:** Smoother diagonal movement appearance
- **Priority:** Very Low
- **Complexity:** Medium
- **Files:** `src/entity/entity_manager.cpp` (update_movement)

## Input & Controls

### Configurable Keybindings
- **Description:** Allow users to rebind keyboard shortcuts
- **Current:** Hardcoded keybindings
- **Benefit:** User customization, accessibility
- **Priority:** Low
- **Complexity:** Medium
- **Files:** `src/input/keybindings.hpp/cpp` (new), settings UI

### Mouse Gesture Support
- **Description:** Support for mouse gestures (hold-drag for abilities)
- **Current:** Click-only input
- **Benefit:** Advanced control options
- **Priority:** Very Low
- **Complexity:** Medium
- **Files:** `src/input/gesture_recognizer.hpp/cpp` (new)

## Performance

### Render Batching
- **Description:** Group entities by sprite sheet for batched rendering
- **Current:** Individual draw calls per entity
- **Benefit:** Significant performance improvement with many entities
- **Priority:** High
- **Complexity:** High
- **Files:** `src/entity/entity_manager.cpp`, `src/graphics/sprite_batch.hpp/cpp` (new)

### Instanced Rendering
- **Description:** Use instancing for same-type entities
- **Current:** Individual rendering
- **Benefit:** Better performance for groups of similar entities
- **Priority:** Medium
- **Complexity:** High
- **Files:** `src/graphics/renderer.cpp`, sprite system

### Spatial Partitioning
- **Description:** Replace fixed viewport array with spatial hash or quadtree
- **Current:** Fixed 40x35 tile array
- **Benefit:** Dynamic viewport sizes, better entity lookup performance
- **Priority:** Medium
- **Complexity:** High
- **Files:** `src/world/spatial_index.hpp/cpp` (new)

## Architecture

### Entity Component System Refinement
- **Description:** Further modernize ECS architecture
- **Current:** Basic component system
- **Future:** Full ECS with system architecture
- **Benefit:** Better code organization, more flexible entity composition
- **Priority:** Low
- **Complexity:** Very High
- **Files:** Major refactor of `src/entity/`

### Animation State Machine
- **Description:** Formal state machine for animation transitions
- **Current:** Direct state setting
- **Benefit:** More robust animation control, easier to add complex behaviors
- **Priority:** Medium
- **Complexity:** Medium
- **Files:** `src/entity/animation_state_machine.hpp/cpp` (new)

## Debugging & Tools

### Movement Debug Overlay
- **Description:** Show tile grid, movement targets, collision boxes
- **Current:** Limited debug visualization
- **Benefit:** Easier debugging of movement issues
- **Priority:** Low
- **Complexity:** Low
- **Files:** `src/debug/movement_debug.hpp/cpp` (new)

### Animation Debug Panel
- **Description:** Show current animation state, frame, timing info
- **Current:** No animation debugging UI
- **Benefit:** Easier animation debugging and tuning
- **Priority:** Very Low
- **Complexity:** Low
- **Files:** `src/debug/animation_debug.hpp/cpp` (new)

---

## Priority Levels

- **Very Low:** Nice to have, no immediate benefit
- **Low:** Useful but not essential
- **Medium:** Noticeable improvement, should do eventually
- **High:** Significant benefit, prioritize when possible
- **Critical:** Must have for core functionality

## Complexity Levels

- **Low:** < 1 day of work, minimal risk
- **Medium:** 1-3 days of work, some integration challenges
- **High:** 1+ weeks of work, significant architectural changes
- **Very High:** Major refactor, multiple weeks of work

---

## How to Use This Document

1. **Adding Items:** When identifying new improvements during development, add them here
2. **Prioritizing:** Review periodically and adjust priorities based on user needs
3. **Planning:** Use this to select items for future sprint/milestone planning
4. **Tracking:** Move implemented items to a "Completed Improvements" section at the bottom

## Completed Improvements

*This section will track improvements as they are implemented*

### ✅ Click-to-Move Implementation
- **Completed:** [Date TBD]
- **PR/Commit:** [Link TBD]
- **Description:** Basic click-to-move with direction calculation and run mode
