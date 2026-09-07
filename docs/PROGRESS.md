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
- [x] Dead body / corpse rendering
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
- [x] Inventory dialog
- [x] Shop/trade dialogs
- [x] Party dialog
- [x] Quest dialogs
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
- [x] Party system
- [ ] Trade system
- [ ] Crusade/Heldenian events

---

## Recent Changes

### 2026-09-06: The legacy dialog art is back

- `dialog::set_art` draws a frame of a pak as the window (the dialog takes the frame's size) with optional overlays for the title banners of DialogText; `ui_system::set_sprite_manager` hands the sprite manager to the dialogs. Windows without art use a parchment palette instead of the blue-grey boxes.
- Buttons everywhere use the same parchment palette (brown faces, gold border, cream text).
- Inventory is the chest (GameDialog 7), items inside the lid, gold/count/weight on the rim. Character Info is the full legacy panel (DialogText 0, labels included): name and nation in the name box, the figure in the portrait square at the legacy anchor, values right-aligned in their boxes, attributes in theirs, the three buttons along the bottom. Skills is the portrait frame with the "Skills" banner, the spellbook the pentagram page with "Magic List".

### 2026-09-06: Elites in gold, item tooltips, the new events in the log

- `npc_spawn` carries `"elite": true`; an elite monster's name (the server puts "Elite " in front) is drawn in gold.
- Inventory tooltip: the item under the mouse shows its name, the description the server now sends (the Olympia texts, in yellow, word-wrapped), damage or defense, level, durability, weight and price. `inventory_dialog::render_tooltip`.
- Opening a treasure chest, a specialty level (`specialty_update`) and an unlocked achievement (`achievement_unlocked`) go to the on-screen event log in yellow, next to the system chat line the server also sends.

### 2026-09-05: Every sprite pack on disk is registered; NPC ids move to 20000

- The equipment tables in `menu_character_renderer.cpp` only knew the 3.51 set. Packs that had been on disk all along (StormBringer, the Kloness weapons, Devastator, LightBlade, the hero mails/robes/hauberks/leggings/helms/caps, Staff3, ReMagicWand, Direct/Fire bows, mantles 4-6) were never loaded, so a character with those items showed bare hands and no armour. The tables now follow the 3.82 Game.cpp list, plus the Olympia packs where the id layout has room (Hanbok, the D-plate/robe/helm/wizhat, HauberkN, angels/dark/knock bows, absorpwand, AM/AW mantles).
- Mantles live at 9600/19600: the legacy base 9230 runs into the helmets at 9300 past the 4th mantle.
- NPC sprite ids start at 20000 instead of 1220: the legacy base only reaches type 78 before the equipment ids at 5060. Types 70-91 (Barlog to Gate; packs on disk, never registered) and the Olympia NPCs at 100-112 (Scarecrow, Ghost, Princess, Bat, officers, guard variants, chests, black beholder; our own numbering) are in the monster table, `npc_type_last` is 119.
- Packs keep empty records for the directions a figure lacks (Gate, the officers): `store_sprite_at_id` skips them quietly instead of logging an error per record.
- Tile registry completed from the 3.82 Game.cpp table: Structures1 (ids 50-69), Objects5-7 (230-248), maptiles4-6 (320-352) and maptiles353-361 were never registered, so 55 maps (Aresden and Elvine among them) drew holes where buildings, later objects and newer ground sets belong; objects1 and objects4 grew to their 3.82 counts, and the Olympia lgn_objects (249) and lgn_maptiles (315-319) fill the ids its maps use.
- The character list reads the `equipment` object the server now sends with `get_characters_response` (same shape as the entity spawn), so the character select shows the figure dressed.
- Checked in the real client: GmSmoke wearing eHeroOfCap, eHeroOfArmor, KnightHauberk, Cape+1 and KlonessBlade renders dressed in game; Minotaurus and Centaurus (types 78 and 71) visible in procella.

### 2026-09-05: Olympia assets: sprite packs, maps and the music
- `tools/opk2pak.mjs` converts the `.opk` sprite packs of the Helbreath Olympia client into the classic `.pak` this client reads (same frames and 8-bit BMPs, reorganised: a 13-byte record per sprite up front, BMPs at the end). Validated against `Ant.pak`/`ABS.pak`: identical frame tables, BMPs differing only in the palette's reserved byte and a handful of pixels
- Imported into `bin/assets/sprites` (not versioned) the 79 packs Olympia has and this client lacked: 56 player equipment packs (mantles, dark/hero sets, staffs, bows), 13 NPCs (Bat, Ghost, Scarecrow, Battle/Event officers, the three guards, the three chests, the princess), 1 effect pack, 2 object sets, 5 UI packs and 2 tile sets (`create_acc.opk` has another layout and was skipped). Nothing existing was replaced
- 16 maps Olympia has and this world lacked, copied to the client and the server (`arena1`-`arena9`, `astoria`, `huntzone5`, `huntzone6`, `oldelvine`, `village`, `wzdtwr_1f`, `wzdtwr_2f`); the server loads them without generators until someone writes their YAML
- Music: the tracks are on disk as `.wav` (MainTm, aresden, elvine, dungeon, middleland, abaddon, druncncity, Carol) while the sound manager asked for `.ogg`, so nothing ever played. `resolve_music_path` falls back to the same name as `.wav`/`.flac`, and the title screen maps to MainTm

### 2026-09-05: Player always centred; right click reaches the bank
- The camera centred on the window size from the config (1280x720) instead of the scene size (800x600 in the scaled view mode) until the first resolution change, which put the player right of centre; the world and weather now take the renderer's scene size at start-up. `world::center_on_player` also no longer clamps the camera to the map bounds: the player stays in the middle of the screen on every map, as in the original client, and nothing is drawn past the map edge (`docs/camera_system.md` updated)
- A right click on a dialog that is not right-click-closeable is delivered to it instead of ignored; the bank dialog is such a dialog, so "right-click to withdraw" works. The shop, sell and bank dialogs are not modal, since two of them are open at once and a modal front dialog swallowed the other's clicks

### 2026-09-05: Shops and the bank on the JSON protocol
- Clicking a merchant opens the shop dialog with the server's catalogue (`player_interact_response`, interaction_type `shop`) and the sell dialog listing the bag with the merchant's own offer per item: a quote (`shop_sell_request`) is asked for every item and commits to nothing, items the merchant refuses leave the list. Buying is `shop_buy_request`; Sell asks the quote again and confirms it (`shop_sell_confirm_request`); the inventory pushes keep the bag and the list in sync (`ws_shop_handlers.cpp`)
- Clicking the banker opens the bank dialog with its slots; withdraw by clicking a slot (`bank_withdraw_request`), deposit by dragging an item from the inventory onto the bank dialog (`bank_deposit_request`); after either the bank is re-read by interacting again
- The NPC dialog actions `open_shop`/`open_bank` interact again, which is how the server hands out the shop or bank

### 2026-09-05: Quests and party on the JSON protocol; NPC talk by click
- A plain click on a town NPC sends `player_interact_request`; the `dialog` interaction opens the NPC dialog with the server's text and options, and each option goes back as `dialog_choice_request` (`ws_quest_handlers.cpp`). `goto_node` continues the conversation; `open_quests` and `claim_rewards` close it and let the server push what follows
- Quest dialog rewritten (`ui/dialogs/quest_dialog.*`): the officer's offers (from `quest_list_response`) and the journal (J key or the character dialog's Quest button, `quest_journal_request`) in one list-and-details dialog with Accept, Complete and Abandon; `quest_update` pushes refresh it and report progress in the status log
- Party moved from the legacy packet to JSON (`ws_party_handlers.cpp`): invite by name from the party dialog's new field, invite banner with Accept/Decline for `party_invite_notice`, leave, and `party_update` membership
- A plain click on a town NPC is sent once per press; the NPC dialog box grew to fit the city hall officer's six options. Corpses were already drawn (entity_death keeps the entity as a corpse until despawn); the checklist now says so

### 2026-09-05: In-game UI follows the scaled view
- In the scaled view mode the HUD, dialogs, chat, status bar, cursor and FPS text draw through the same placement as the scene (internal resolution letterboxed or stretched into the window), so a 1200x900 window shows the whole game 1.5x instead of a small HUD in the corner. Mouse pixels are mapped into the internal resolution once (`input::set_mouse_transform`, per axis for the stretch aspect) and `display_to_scene` is identity there
- Scissor rectangles and the three places that measured the window instead of the logical screen (time/weather bar, chat, death dialog) now use the logical size, which is what broke the first attempt
- Text under a scaled view is rasterized at the final pixel size (`text_renderer::set_pixel_scale`) and drawn back down, so it stays crisp; positions and measurements stay logical

### 2026-02-22: Item system v2
- Rewrite item types to match v2 wire protocol (universal item shape)
- Replace equip_slot with equip_pos (string enums at wire boundary)
- Equipment model: ID references into inventory instead of copies
- Rewrite all inventory/equipment message handlers for v2 channels
- Update all UI consumers (inventory, shop, bank, trade, craft, paperdoll)
- Switch to PAK-relative sprite lookups (sprite_id + sprite_frame from server)
- Add sprite_frame to protocol spec, fix full_body equip_pos naming

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
