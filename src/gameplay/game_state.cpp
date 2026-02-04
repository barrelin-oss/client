#include "gameplay/game_state.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/tile_sprite_registry.hpp"
#include "core/constants.hpp"
#include "core/config.hpp"
#include "core/direction_utils.hpp"
#include "gameplay/pathfinding.hpp"
#include "ui/dialog_manager.hpp"
#include "ui/managed_dialog.hpp"
#include "ui/dialogs/icon_panel_dialog.hpp"
#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#endif

#include "debug/debug_stats.hpp"

namespace hb {

bool game_state_manager::initialize(renderer& rend, audio& aud) {
    audio_ = &aud;
    renderer_ = &rend;

    // Initialize subsystems
    if (!network_.initialize()) {
        spdlog::error("Failed to initialize network");
        return false;
    }

    entities_.initialize();

    // Initialize sprite manager and load PAK files FIRST
    // (UI needs sprites for classic rendering)
    sprites_.initialize("assets/");

    // Load UI PAK files before UI initialization
    if (sprites_.load_pak("interface", "sprites/interface.pak")) {
        spdlog::info("Loaded interface.pak");
        sprites_.store_sprite_at_id(0, "interface", 0);  // Mouse cursor
    }

    if (sprites_.load_pak("interface2", "sprites/interface2.pak")) {
        spdlog::info("Loaded interface2.pak");
    }

    if (sprites_.load_pak("New-Dialog", "sprites/New-Dialog.pak")) {
        spdlog::info("Loaded New-Dialog.pak");
        sprites_.store_sprite_at_id(51, "New-Dialog", 0);  // Loading
        sprites_.store_sprite_at_id(52, "New-Dialog", 1);  // Main Menu
        sprites_.store_sprite_at_id(54, "New-Dialog", 2);  // Logout/Exit
    }

    if (sprites_.load_pak("LoginDialog", "sprites/LoginDialog.pak")) {
        spdlog::info("Loaded LoginDialog.pak");
        sprites_.store_sprite_at_id(53, "LoginDialog", 0);  // Login screen
    }

    if (sprites_.load_pak("GameDialog", "sprites/GameDialog.pak")) {
        spdlog::info("Loaded GameDialog.pak");
        sprites_.store_sprite_at_id(57, "GameDialog", 8);   // Character select
        sprites_.store_sprite_at_id(58, "GameDialog", 9);   // Character create
        sprites_.store_sprite_at_id(60, "GameDialog", 0);   // Game dialog 1
        sprites_.store_sprite_at_id(61, "GameDialog", 1);   // Game dialog 2
        sprites_.store_sprite_at_id(62, "GameDialog", 2);   // Game dialog 3
        sprites_.store_sprite_at_id(63, "GameDialog", 3);   // Game dialog 4
    }

    if (sprites_.load_pak("DialogText", "sprites/DialogText.pak")) {
        spdlog::info("Loaded DialogText.pak");
        sprites_.store_sprite_at_id(71, "DialogText", 1);   // Button sprites
    }

    // Set sprite manager on UI before initializing
    // (so dialog_manager gets it during initialization)
    ui_.set_sprite_manager(&sprites_);
    ui_.initialize();

    inventory_.initialize();
    magic_.initialize();
    skills_.initialize();
    combat_.initialize(&entities_, &magic_, &skills_, &sounds_);

    // Setup network handlers
    setup_network_handlers();

    // Initialize menu character renderer (loads body, underwear, hair PAK files)
    if (menu_char_renderer_.initialize(sprites_)) {
        spdlog::info("Menu character renderer initialized");
    } else {
        spdlog::warn("Menu character renderer initialization failed - character previews may not display");
    }

    // Initialize screen manager
    screens_.initialize();

    // Setup screen callbacks
    screens_.get_main_menu_screen().set_on_start([this]() {
        change_state(game_state::login);
    });
    screens_.get_main_menu_screen().set_on_quit([this]() {
        change_state(game_state::quit);
    });

    screens_.get_login_screen().set_on_login([this](const std::string& account, const std::string& password) {
        spdlog::info("Login attempt: {} (pass length: {})", account, password.length());
        attempt_login(account, password);
    });
    screens_.get_login_screen().set_on_cancel([this]() {
        change_state(game_state::main_menu);
    });

    // Character select screen callbacks
    screens_.get_character_select_screen().set_on_select([this](int32_t index) {
        if (index >= 0 && index < static_cast<int32_t>(characters_.size())) {
            int32_t character_id = characters_[index].id;
            spdlog::info("Entering game with character '{}' (ID: {})", characters_[index].name, character_id);
            request_enter_game(character_id);
        }
    });
    screens_.get_character_select_screen().set_on_create([this]() {
        spdlog::info("Opening character creation");
        change_state(game_state::create_character);
    });
    screens_.get_character_select_screen().set_on_delete([this](int32_t index) {
        spdlog::info("Delete character requested: {}", index);
        // TODO: Implement delete character request
    });
    screens_.get_character_select_screen().set_on_logout([this]() {
        spdlog::info("Logging out to main menu");
        ws_connection_.disconnect();
        change_state(game_state::main_menu);
    });

    // Character create screen callbacks
    screens_.get_character_create_screen().set_on_create([this](const character_create_data& data) {
        spdlog::info("Creating character: name='{}' gender={} stats={}/{}/{}/{}/{}/{}",
                     data.name, data.gender, data.strength, data.vitality, data.dexterity,
                     data.intelligence, data.magic, data.charisma);
        request_create_character(data);
    });
    screens_.get_character_create_screen().set_on_cancel([this]() {
        spdlog::info("Canceling character creation, returning to character select");
        change_state(game_state::select_character);
    });

    // Connection lost screen callback
    screens_.get_connection_lost_screen().set_on_timeout([this]() {
        spdlog::info("Connection lost timeout, returning to main menu");
        change_state(game_state::main_menu);
    });

    // Set up character renderer for menu screens
    screens_.get_character_select_screen().set_character_renderer(&menu_char_renderer_);
    screens_.get_character_create_screen().set_character_renderer(&menu_char_renderer_);

    // Set up sound callbacks for all screens
    auto play_ui_sound = [this]() { sounds_.play_ui_sound(14); };
    screens_.get_main_menu_screen().set_on_button_sound(play_ui_sound);
    screens_.get_login_screen().set_on_button_sound(play_ui_sound);
    screens_.get_character_select_screen().set_on_button_sound(play_ui_sound);
    screens_.get_character_create_screen().set_on_button_sound(play_ui_sound);

    // Create UI dialogs for in-game use (not for login/main menu)
    ui_.create_character_select_dialog();  // Keep for now as fallback
    ui_.create_character_create_dialog();
    ui_.create_character_dialog();
    ui_.create_inventory_dialog();
    ui_.create_equipment_dialog();
    ui_.create_spellbook_dialog();
    ui_.create_skills_dialog();
    ui_.create_chat_dialog();
    ui_.create_shop_dialog();
    ui_.create_bank_dialog();
    ui_.create_party_dialog();
    ui_.create_guild_dialog();
    ui_.create_npc_dialog();
    ui_.create_trade_dialog();
    ui_.create_craft_dialog();
    ui_.create_map_dialog();
    ui_.create_repair_dialog();
    ui_.create_help_dialog();
    ui_.create_system_menu_dialog();
    ui_.create_options_dialog();

    // Load YAML-based dialog definitions BEFORE creating icon panel
    // (icon_panel uses YAML definition if available)
    ui_.load_dialog_definitions_from_directory("assets/ui/dialogs");

    ui_.create_icon_panel_dialog();

    // Set initial screen size for icon panel to position it correctly
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_screen_size(renderer_->width(), renderer_->height());
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        icon_dlg->set_screen_size(renderer_->width(), renderer_->height());
    }

    ui_.create_gauge_panel_dialog();
    ui_.create_levelup_dialog();

    // Wire up dialog callbacks (for in-game dialogs)
    setup_dialog_callbacks();

    // Initialize tile sprite registry for world/terrain rendering
    // This maps legacy sprite IDs to PAK files and provides lazy-loading
    // Note: sprite_manager already prepends "assets/" so we use "sprites/" here
    if (tile_registry_.initialize(sprites_, "sprites/"))
    {
        spdlog::info("Tile sprite registry initialized with {} mappings", tile_registry_.registered_count());

        // Initialize world with the tile registry
        if (!world_.initialize(tile_registry_))
        {
            spdlog::warn("World initialization failed - terrain rendering may not work");
        }

        // Set initial screen size for camera centering
        const auto& video = config::instance().video();
        world_.set_screen_size(video.screen_width, video.screen_height);
    }
    else
    {
        spdlog::warn("Tile sprite registry initialization failed - terrain rendering disabled");
    }

    // Initialize sound manager
    if (sounds_.initialize(*audio_))
    {
        sounds_.set_sfx_enabled(config::instance().audio().sfx_enabled);
        sounds_.set_music_enabled(config::instance().audio().music_enabled);
        spdlog::info("Sound manager initialized with sfx={}, music={}",
                     sounds_.is_sfx_enabled(), sounds_.is_music_enabled());

        // Wire up entity manager with sound manager for footstep sounds
        entities_.set_sound_manager(&sounds_);
    }
    else
    {
        spdlog::warn("Sound manager initialization failed - audio may not work");
    }

    // Wire up world events for automatic BGM changes
    world_.set_events({
        .on_map_changed = [this](std::string_view /*old_map*/, std::string_view new_map) {
            sounds_.start_bgm(new_map, static_cast<int>(world_.weather()));
        },
        .on_weather_changed = [this](weather_type w) {
            // Restart BGM if Christmas weather (types 4-6)
            int weather_int = static_cast<int>(w);
            if (weather_int >= 4 && weather_int <= 6)
            {
                sounds_.start_bgm(world_.current_map_name(), weather_int);
            }
        },
        .on_time_changed = nullptr
    });

    // Start at main menu - directly enter the state since it's the initial state
    enter_state(game_state::main_menu);

    spdlog::info("Game state manager initialized");
    return true;
}

void game_state_manager::shutdown()
{
    exit_state(state_);

    sounds_.shutdown();
    screens_.shutdown();
    network_.shutdown();
    world_.shutdown();
    entities_.shutdown();
    ui_.shutdown();
    sprites_.clear_cache();
    tile_registry_.clear_cache();

    spdlog::info("Game state manager shutdown");
}

void game_state_manager::update(float delta_time, const input& inp) {
    // Handle state transitions
    if (state_transition_) {
        exit_state(state_);
        state_ = pending_state_;
        enter_state(state_);
        state_transition_ = false;
    }

    // Update network
    network_.update();

    // Poll for WebSocket messages on main thread (avoids race conditions)
    while (auto msg = ws_connection_.receive()) {
        handle_ws_message(*msg);
    }

    // Handle pending connect event from background thread
    if (ws_connection_.poll_connect_event()) {
        spdlog::info("WebSocket connected (polled on main thread)");
        // Send login request if we were waiting for connection
        if (pending_login_on_connect_.exchange(false)) {
            spdlog::info("Sending login request for user: {}", pending_username_);
            json login_msg = make_login_request(pending_username_, pending_password_);
            ws_connection_.send(login_msg);
        }
    }

    // Handle pending disconnect event from background thread
    std::string disconnect_reason;
    if (ws_connection_.poll_disconnect_event(disconnect_reason)) {
        spdlog::warn("WebSocket disconnected (polled on main thread): {}", disconnect_reason);
        pending_login_on_connect_.store(false);  // Cancel any pending login

        // Clear session state
        session_token_.clear();
        pending_username_.clear();
        pending_password_.clear();
        characters_.clear();

        // Show connection lost screen if we were in an authenticated state
        if (state_ != game_state::main_menu && state_ != game_state::connection_lost) {
            screens_.get_connection_lost_screen().set_reason(
                disconnect_reason.empty() ? "Server disconnected" : disconnect_reason);
            change_state(game_state::connection_lost);
        }
    }

    // Legacy disconnect handling (for backward compatibility with existing callback users)
    if (has_pending_disconnect_.exchange(false)) {
        std::string reason;
        {
            std::lock_guard<std::mutex> lock(pending_disconnect_mutex_);
            reason = std::move(pending_disconnect_reason_);
        }

        // Clear session state
        session_token_.clear();
        pending_username_.clear();
        pending_password_.clear();
        characters_.clear();

        // Show connection lost screen if we were in an authenticated state
        if (state_ != game_state::main_menu && state_ != game_state::connection_lost) {
            screens_.get_connection_lost_screen().set_reason(
                reason.empty() ? "Server disconnected" : reason);
            change_state(game_state::connection_lost);
        }
    }

    // Update sprite memory management (evict unused bitmaps)
    sprites_.update_memory(delta_time);

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Update debug overlay FIRST (handles F11 toggle, input, hot-reload)
    // This must happen before other input processing so it can consume input
    auto& debug_overlay = debug::debug_overlay::instance();
    debug_overlay.update(delta_time, inp);
    bool debug_consumed_input = debug_overlay.consumed_mouse_input() ||
                                 debug_overlay.consumed_keyboard_input();
#else
    bool debug_consumed_input = false;
#endif

    // Update based on current state (skip if debug overlay consumed input)
    if (!debug_consumed_input) {
        switch (state_) {
            case game_state::main_menu:
                update_main_menu(delta_time, inp);
                break;
            case game_state::login:
                update_login(delta_time, inp);
                break;
            case game_state::select_character:
                update_character_select(delta_time, inp);
                break;
            case game_state::create_character:
                update_character_create(delta_time, inp);
                break;
            case game_state::loading:
                update_loading(delta_time, inp);
                break;
            case game_state::playing:
                update_playing(delta_time, inp);
                break;
            case game_state::connection_lost:
                screens_.update(delta_time, inp);
                break;
            default:
                break;
        }

        // Update UI
        ui_.update(delta_time, inp);
    }
}

void game_state_manager::render(renderer& rend) {
    switch (state_) {
        case game_state::main_menu:
            render_main_menu(rend);
            break;
        case game_state::login:
            render_login(rend);
            break;
        case game_state::select_character:
            render_character_select(rend);
            break;
        case game_state::create_character:
            render_character_create(rend);
            break;
        case game_state::loading:
            render_loading(rend);
            break;
        case game_state::playing:
            render_playing(rend);
            break;
        case game_state::connection_lost:
            screens_.render(rend, sprites_);
            screens_.render_cursor(rend, sprites_);
            break;
        default:
            break;
    }

    // Render UI on top
    ui_.render(rend);

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Render debug overlay (draws outlines, selection, status bar)
    debug::debug_overlay::instance().render(rend);
#endif

    // Render cursor last (on top of everything including dialogs)
    screens_.render_cursor(rend, sprites_);
}

void game_state_manager::change_state(game_state new_state) {
    if (state_ == new_state) return;

    spdlog::info("State change requested: {} -> {}",
        static_cast<int>(state_), static_cast<int>(new_state));

    pending_state_ = new_state;
    state_transition_ = true;
}

void game_state_manager::enter_state(game_state state) {
    spdlog::debug("Entering state: {}", static_cast<int>(state));

    switch (state) {
        case game_state::main_menu:
            ui_.close_all_dialogs();
            screens_.change_screen(screen_type::main_menu);
            break;

        case game_state::login:
            ui_.close_all_dialogs();
            screens_.change_screen(screen_type::login);
            break;

        case game_state::select_character: {
            ui_.close_all_dialogs();

            // Populate character select screen with characters from server
            std::vector<char_slot_info> slot_chars;
            for (const auto& ch : characters_) {
                char_slot_info slot;
                slot.has_character = true;
                slot.name = ch.name;
                slot.level = ch.level;
                slot.exp = 0;  // Not provided by server yet
                slot.class_name = ch.warrior ? "Warrior" : "Mage";
                // Appearance data for rendering
                slot.gender = ch.gender;
                slot.skin_color = ch.skin_color;
                slot.hair_style = ch.hair_style;
                slot.hair_color = ch.hair_color;
                slot.underwear_color = ch.underwear_color;
                // Equipment data for rendering
                slot.body_armor = ch.body_armor;
                slot.arm_armor = ch.arm_armor;
                slot.pants = ch.pants;
                slot.boots = ch.boots;
                slot.helmet = ch.helmet;
                slot.mantle = ch.mantle;
                slot.weapon = ch.weapon;
                slot.shield = ch.shield;
                slot_chars.push_back(slot);
            }
            screens_.get_character_select_screen().set_characters(slot_chars);

            // Switch to character select screen
            screens_.change_screen(screen_type::character_select);
            break;
        }

        case game_state::create_character:
            ui_.close_dialog(dialog_type::character_select);
            screens_.change_screen(screen_type::character_create);
            break;

        case game_state::loading:
            screens_.change_screen(screen_type::none);
            ui_.close_all_dialogs();
            loading_progress_ = 0.0f;
            loading_message_ = "Loading...";
            break;

        case game_state::playing:
            screens_.change_screen(screen_type::none);
            ui_.close_all_dialogs();
            // Open HUD elements (chat dialog is toggled via icon panel)
            ui_.open_dialog(dialog_type::icon_panel);
            // Note: gauge_panel is now integrated into icon_panel
            // Ensure icon panel is positioned correctly for current screen size
            if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
                yaml_dlg->set_screen_size(renderer_->width(), renderer_->height());
            } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
                icon_dlg->set_screen_size(renderer_->width(), renderer_->height());
            }
            // Initialize icon panel with player data
            update_icon_panel();
            // Start background music for the current map
            sounds_.start_bgm(world_.current_map_name(), static_cast<int>(world_.weather()));
            break;

        case game_state::connection_lost:
            screens_.change_screen(screen_type::connection_lost);
            ui_.close_all_dialogs();
            break;

        default:
            break;
    }
}

void game_state_manager::exit_state(game_state state) {
    spdlog::debug("Exiting state: {}", static_cast<int>(state));

    switch (state) {
        case game_state::playing:
            // Stop background music when leaving game
            sounds_.stop_bgm();

            // Clear all game state to prevent stale data on re-entry
            clear_game_data();
            break;

        default:
            break;
    }
}

void game_state_manager::clear_game_data() {
    spdlog::info("Clearing game data (map, entities, combat, inventory)");

    // Clear all entities (players, NPCs, monsters, items)
    entities_.remove_all_entities();
    entities_.set_local_player(invalid_entity_id);

    // Clear map data
    world_.unload_map();

    // Reset camera state
    world_.set_cinematic_mode(false);
    world_.set_global_render_mode(false);
    world_.set_zoom_mode_enabled(false);

    // Reset combat state
    combat_mode_ = false;
    safe_attack_mode_ = false;

    // Reset movement state
    pending_action_ = queued_action{};
    action_in_progress_ = false;
    move_dest_x_ = -1;
    move_dest_y_ = -1;
    run_mode_enabled_ = false;
    blocked_movement_cooldown_ = 0.0f;

    // Clear status log
    status_log_.clear();
}

void game_state_manager::update_main_menu(float delta_time, const input& inp) {
    // Always update mouse position for cursor rendering
    screens_.update_mouse_position(inp);
    // Only process screen input when no modal dialog is open
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_login(float delta_time, const input& inp) {
    // Always update mouse position for cursor rendering
    screens_.update_mouse_position(inp);
    // Only process screen input when no modal dialog is open
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_character_select(float delta_time, const input& inp) {
    // Always update mouse position for cursor rendering
    screens_.update_mouse_position(inp);
    // Only process screen input when no modal dialog is open
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_character_create(float delta_time, const input& inp) {
    // Always update mouse position for cursor rendering
    screens_.update_mouse_position(inp);
    // Only process screen input when no modal dialog is open
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_loading(float delta_time, const input& inp) {
    (void)inp;

    // Simulate loading progress
    loading_progress_ += delta_time * 0.5f;
    if (loading_progress_ >= 1.0f) {
        loading_progress_ = 1.0f;
        change_state(game_state::playing);
    }
}

void game_state_manager::update_playing(float delta_time, const input& inp) {
    // Store mouse position for entity hover detection during rendering
    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();

    // Update blocked movement cooldown
    if (blocked_movement_cooldown_ > 0.0f) {
        blocked_movement_cooldown_ -= delta_time;
        if (blocked_movement_cooldown_ < 0.0f) {
            blocked_movement_cooldown_ = 0.0f;
        }
    }

    // Handle input
    handle_playing_input(inp);

    // Track alt key state for super attack indicator
    bool alt_held = inp.is_key_down(sf::Keyboard::Key::LAlt) ||
                    inp.is_key_down(sf::Keyboard::Key::RAlt);
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_alt_held(alt_held);
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        icon_dlg->set_alt_held(alt_held);
    }

    // Mouse wheel zoom (only when zoom mode enabled)
    if (world_.is_zoom_mode_enabled()) {
        int32_t wheel = inp.wheel_delta();
        if (wheel != 0) {
            // Scroll up = zoom in (smaller zoom_level), scroll down = zoom out
            // Pass cursor position for zoom-to-cursor in cinematic mode
            if (world_.is_cinematic_mode()) {
                world_.adjust_zoom(static_cast<float>(-wheel) * 0.1f, inp.mouse_x(), inp.mouse_y());
            } else {
                world_.adjust_zoom(static_cast<float>(-wheel) * 0.1f);
            }
        }
    }

    // Update world
    world_.update(delta_time);

    // Update entities
    entities_.update(delta_time, world_, combat_mode_);

    // Process any queued actions (executes when animation finishes)
    process_queued_action();

    // Update camera to follow local player
    if (entity* player = local_player()) {
        const auto& transform = player->transform();
        // Use pixel position for smooth following
        world_.set_player_position(transform.x, transform.y);
    }

    // Update combat
    combat_.update(delta_time);

    // Update magic effects
    magic_.update_effects(delta_time);

    // Update skill cooldowns
    skills_.update_cooldowns(delta_time);

    // Update HUD with current player stats
    update_icon_panel();

    // Update debug stats
    auto& debug_stats = debug::debug_stats::instance();
    debug_stats.update(delta_time);
    if (debug_stats.visible()) {
        // Update camera bounds
        int32_t cam_x = world_.camera_x();
        int32_t cam_y = world_.camera_y();
        debug_stats.set_camera_bounds(
            cam_x, cam_y,
            cam_x + static_cast<int32_t>(renderer_->width()),
            cam_y + static_cast<int32_t>(renderer_->height())
        );

        // Update player position
        if (entity* player = local_player()) {
            const auto& transform = player->transform();
            debug_stats.set_player_position(
                transform.tile_x, transform.tile_y,
                transform.x, transform.y
            );
        }

        // Update entity count and map name
        debug_stats.set_entity_count(static_cast<int32_t>(entities_.entity_count()));
        debug_stats.set_map_name(std::string(world_.current_map_name()));

        // Update network stats
        debug_stats.set_network_connected(ws_connection_.is_connected());

        // Update asset stats
        debug_stats.set_sprite_cache_count(static_cast<int32_t>(sprites_.cached_sprite_count()));
        debug_stats.set_pak_files_loaded(static_cast<int32_t>(sprites_.loaded_pak_count()));

        // Update mouse position
        debug_stats.set_mouse_screen_pos(inp.mouse_x(), inp.mouse_y());
        int32_t mouse_world_x = inp.mouse_x() + cam_x;
        int32_t mouse_world_y = inp.mouse_y() + cam_y;
        debug_stats.set_mouse_world_pos(mouse_world_x, mouse_world_y);
        auto [tile_x, tile_y] = world_.screen_to_tile(inp.mouse_x(), inp.mouse_y());
        debug_stats.set_mouse_tile_pos(tile_x, tile_y);

        // Update hovered entity info
        entity* hovered = entities_.get_entity_at_screen_pos(
            inp.mouse_x(), inp.mouse_y(), cam_x, cam_y);
        if (hovered && hovered->has_name()) {
            debug_stats.set_hovered_entity(hovered->name().name + " (ID:" + std::to_string(hovered->id()) + ")");
        } else {
            debug_stats.set_hovered_entity("");
        }

        // Update game state info
        debug_stats.set_game_state("Playing");
        debug_stats.set_combat_mode(combat_mode_, safe_attack_mode_);
    }

    // Update status log (hunger warnings, etc.)
    status_log_.update(delta_time);
}

void game_state_manager::render_main_menu(renderer& rend) {
    // If sprites loaded, render via screen manager
    if (sprites_.has_sprite_at_id(52)) {
        screens_.render(rend, sprites_);
    } else {
        // Fallback if sprites not loaded
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("HELBREATH", screen_width / 2 - 60, 150, sf::Color::White, 24);
        rend.draw_text("Click to Start (sprites not loaded)", screen_width / 2 - 100, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_login(renderer& rend) {
    // If sprites loaded, render via screen manager
    if (sprites_.has_sprite_at_id(53)) {
        screens_.render(rend, sprites_);
    } else {
        // Fallback if sprites not loaded
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("LOGIN (sprites not loaded)", screen_width / 2 - 80, 100, sf::Color::White, 16);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_character_select(renderer& rend) {
    // If sprites loaded, render via screen manager
    if (sprites_.has_sprite_at_id(57)) {  // Character select sprite
        screens_.render(rend, sprites_);
    } else {
        // Fallback if sprites not loaded
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("CHARACTER SELECT (sprites not loaded)", screen_width / 2 - 120, 100, sf::Color::White);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_character_create(renderer& rend) {
    // If sprites loaded, render via screen manager
    if (sprites_.has_sprite_at_id(58)) {  // Character create sprite
        screens_.render(rend, sprites_);
    } else {
        // Fallback if sprites not loaded
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("CHARACTER CREATE (sprites not loaded)", screen_width / 2 - 120, 100, sf::Color::White);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_loading(renderer& rend) {
    // Background
    rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(10, 10, 20), true);

    // Loading bar
    int32_t bar_width = 400;
    int32_t bar_height = 20;
    int32_t bar_x = (screen_width - bar_width) / 2;
    int32_t bar_y = screen_height / 2;

    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(40, 40, 60), true);
    rend.draw_rect(bar_x, bar_y, static_cast<int32_t>(bar_width * loading_progress_), bar_height,
                   sf::Color(100, 150, 200), true);
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(80, 80, 120), false);

    // Loading text
    rend.draw_text(loading_message_, bar_x, bar_y - 30, sf::Color::White);
}

void game_state_manager::render_playing(renderer& rend) {
    // Apply zoom view for world and entity rendering
    world_.apply_zoom_view(rend);

    // Render world
    world_.render(rend);

    // Render entities (with zoom still applied)
    entities_.render(rend, sprites_, world_.camera_x(), world_.camera_y(), mouse_x_, mouse_y_);

    // Reset zoom view before UI rendering
    world_.reset_zoom_view(rend);

    // Render debug stats overlay (before UI so UI renders on top)
    debug::debug_stats::instance().render(rend);

    // Render status log (hunger warnings, etc.) above icon panel
    // Icon panel height is approximately 70 pixels
    status_log_.render(rend, static_cast<int32_t>(rend.width()),
                       static_cast<int32_t>(rend.height()), 70);
}

void game_state_manager::setup_network_handlers() {
    // Initialize handlers
    notify_handler_.initialize(*this);
    motion_handler_.initialize(*this);

    // Login response handler (handles login results)
    network_.register_handler(msg_response_log,
        [this](packet_reader& r) { handle_login_response(r); });

    // Enter game response handler
    network_.register_handler(msg_response_enter_game,
        [this](packet_reader& r) { handle_enter_game(r); });

    // Player data handler
    network_.register_handler(msg_response_player_data,
        [this](packet_reader& r) { handle_player_data(r); });

    // Notification handler for various game events
    network_.register_handler(msg_notify,
        [this](packet_reader& r) {
            // Read notification subtype
            auto subtype = r.read_u16();
            if (!subtype) return;

            // Dispatch to notify handler
            notify_handler_.dispatch(*subtype, r);
        });

    // Motion response (our own movement confirmed)
    network_.register_handler(msg_response_motion,
        [this](packet_reader& r) { motion_handler_.handle_motion_response(r); });

    // Motion event (other entities' movement)
    network_.register_handler(msg_event_motion,
        [this](packet_reader& r) { motion_handler_.handle_motion_event(r); });

    // Common event (spawn/despawn, effects, etc.)
    network_.register_handler(msg_event_common,
        [this](packet_reader& r) { motion_handler_.handle_common_event(r); });

    // Player character data (includes character list)
    network_.register_handler(msg_player_character_contents,
        [this](packet_reader& r) { handle_character_list(r); });
}

void game_state_manager::handle_login_response(packet_reader& reader) {
    // Read response type (uint16_t from login_response namespace)
    auto result_type = reader.read_u16();
    if (!result_type) return;

    // Check result against login_response codes
    if (*result_type == login_response::confirm) {
        spdlog::info("Login successful");
        // Character list should follow or we need to request it
        network_.request_character_list();
    } else if (*result_type == login_response::password_mismatch) {
        show_error("Invalid password");
    } else if (*result_type == login_response::not_existing_account) {
        show_error("Account does not exist");
    } else if (*result_type == login_response::account_locked) {
        show_error("Account is locked");
    } else if (*result_type == login_response::service_not_available) {
        show_error("Service not available");
    } else {
        show_error("Login failed");
    }
}

void game_state_manager::handle_character_list(packet_reader& reader) {
    auto count = reader.read_u8();
    if (!count) return;

    characters_.clear();
    for (uint8_t i = 0; i < *count; ++i) {
        character_info info;
        if (auto name = reader.read_string(20)) {
            info.name = *name;
        }
        if (auto level = reader.read_u16()) {
            info.level = *level;
        }
        // ... read other character data
        characters_.push_back(info);
    }

    change_state(game_state::select_character);
}

void game_state_manager::handle_enter_game(packet_reader& reader) {
    auto result = reader.read_u8();
    if (!result || *result != 0) {
        show_error("Failed to enter game");
        return;
    }

    // Read game server address
    auto host = reader.read_string(64);
    auto port = reader.read_u16();

    if (host && port) {
        // Connect to game server
        if (network_.connect_game_server(*host, *port)) {
            change_state(game_state::loading);
        } else {
            show_error("Failed to connect to game server");
        }
    }
}

void game_state_manager::handle_player_data(packet_reader& reader) {
    (void)reader;
    // Parse and apply player data
}

void game_state_manager::handle_map_data(packet_reader& reader) {
    (void)reader;
    // Load map from data
}

void game_state_manager::set_local_player_id(entity_id id) {
    entities_.set_local_player(id);
}

entity* game_state_manager::local_player() {
    return entities_.local_player();
}

const entity* game_state_manager::local_player() const {
    return entities_.local_player();
}

void game_state_manager::set_characters(std::vector<character_info> characters) {
    characters_ = std::move(characters);
}

void game_state_manager::select_character(size_t index) {
    if (index < characters_.size()) {
        selected_character_ = index;
    }
}

void game_state_manager::set_login_result(uint16_t result) {
    if (result == login_response::confirm) {
        spdlog::info("Login successful");
    } else {
        spdlog::warn("Login failed: 0x{:04X}", result);
    }
}

void game_state_manager::set_character_create_result(uint16_t result) {
    if (result == login_response::new_character_created) {
        spdlog::info("Character created successfully");
        network_.request_character_list();
    } else if (result == login_response::already_existing_character) {
        spdlog::warn("Character name already exists");
        show_error("Character name already exists");
    } else {
        spdlog::warn("Character creation failed: 0x{:04X}", result);
        show_error("Character creation failed");
    }
}

void game_state_manager::show_message(std::string_view message) {
    // Show the waiting dialog (message parameter is ignored now)
    (void)message; // Unused - dialog always shows "Waiting for response from the server"
    ui_.show_connection_dialog(nullptr);
}

void game_state_manager::show_error(std::string_view error) {
    // Use classic error dialog (with OK button)
    ui_.show_error_dialog(error, nullptr);
}

void game_state_manager::handle_playing_input(const input& inp) {
    // Check if UI is handling input
    if (ui_.is_modal_open()) {
        return;
    }

    // Ctrl+click drag to pan in cinematic mode (if not locked)
    if (world_.is_cinematic_mode() && !camera_drag_locked_) {
        bool ctrl_held = inp.is_key_down(sf::Keyboard::Key::LControl) ||
                         inp.is_key_down(sf::Keyboard::Key::RControl);

        if (ctrl_held && inp.is_mouse_pressed(sf::Mouse::Button::Left)) {
            world_.start_drag(inp.mouse_x(), inp.mouse_y());
        }
        else if (world_.is_dragging()) {
            if (inp.is_mouse_down(sf::Mouse::Button::Left) && ctrl_held) {
                world_.update_drag(inp.mouse_x(), inp.mouse_y());
            } else {
                world_.end_drag();
            }
        }

        // Don't process other input while dragging
        if (world_.is_dragging()) {
            handle_hotkey_input(inp);
            return;
        }
    }

    handle_movement_input(inp);
    handle_combat_input(inp);
    handle_hotkey_input(inp);
}

bool game_state_manager::can_perform_action() const {
    // Check blocked movement cooldown first
    if (blocked_movement_cooldown_ > 0.0f) {
        return false;
    }

    const entity* player = local_player();
    if (!player) return false;

    // Block actions while movement is in progress
    if (player->transform().moving) {
        return false;
    }

    const auto& anim = player->animation();

    // Block actions while any non-looping animation is in progress
    // (attacks, magic, damage, etc.)
    if (!anim.looping && !anim.finished) {
        return false;
    }

    // For movement animations (walk/run), only allow new actions when the
    // animation cycle is complete (at last frame with timer expired, or at frame 0)
    bool is_movement_anim = (anim.state == entity_anim_state::move ||
                             anim.state == entity_anim_state::run);
    if (is_movement_anim && anim.frame_count > 0) {
        bool at_cycle_end = (anim.current_frame == anim.frame_count - 1) &&
                           (anim.frame_timer >= anim.frame_duration);
        bool at_cycle_start = (anim.current_frame == 0) &&
                             (anim.frame_timer < 0.016f);  // Within ~1 frame of start
        if (!at_cycle_end && !at_cycle_start) {
            return false;
        }
    }

    return true;
}

void game_state_manager::queue_action(queued_action action) {
    // Only queue if there's an action in progress
    if (!can_perform_action()) {
        pending_action_ = action;
        spdlog::debug("Queued action type {}", static_cast<int>(action.type));
    }
}

void game_state_manager::process_queued_action() {
    if (pending_action_.type == queued_action_type::none) {
        return;
    }

    if (!can_perform_action()) {
        return;  // Still waiting for current action to finish
    }

    entity* player = local_player();
    if (!player) {
        pending_action_.type = queued_action_type::none;
        return;
    }

    spdlog::debug("Executing queued action type {}", static_cast<int>(pending_action_.type));

    switch (pending_action_.type) {
        case queued_action_type::move:
            // Set movement destination
            move_dest_x_ = pending_action_.target_x;
            move_dest_y_ = pending_action_.target_y;
            break;

        case queued_action_type::attack:
            if (pending_action_.target_id != 0) {
                network_.request_attack(pending_action_.target_id);
            }
            break;

        case queued_action_type::pickup:
            request_pickup(pending_action_.target_x, pending_action_.target_y);
            break;

        case queued_action_type::magic:
            network_.request_magic(pending_action_.spell_id,
                                   pending_action_.target_x, pending_action_.target_y,
                                   pending_action_.target_id);
            break;

        case queued_action_type::face_direction:
            if (entity* p = local_player()) {
                auto& t = p->transform();
                // Use the queued direction (player may have changed direction since queuing)
                // Convert internal direction (1-8) to protocol format (0-7)
                json msg = make_player_stop_request(t.tile_x, t.tile_y,
                                                    static_cast<uint8_t>(direction_to_protocol(pending_action_.face_dir)));
                ws_connection_.send(msg);
            }
            break;

        case queued_action_type::stop:
            // Stop movement and return to idle
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            if (entity* p = local_player()) {
                p->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
            }
            break;

        default:
            break;
    }

    // Clear the queued action
    pending_action_.type = queued_action_type::none;
}

void game_state_manager::handle_movement_input(const input& inp) {
    entity* player = local_player();
    if (!player || !player->has_movement()) {
        return;
    }

    // Left mouse button - movement or actions
    // Use is_mouse_pressed for click-on-self actions (pickup, attack north)
    // Use is_mouse_down for continuous movement destination updates
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left)) {
        auto [dest_x, dest_y] = world_.screen_to_tile(inp.mouse_x(), inp.mouse_y());
        auto& t = player->transform();

        // Check if clicking on own character's actual sprite bounds (only on initial press)
        bool clicked_on_self = entities_.is_point_in_entity_sprite(
            *player, sprites_, world_.camera_x(), world_.camera_y(),
            inp.mouse_x(), inp.mouse_y());

        if (clicked_on_self) {
            bool ctrl_held = inp.is_key_down(sf::Keyboard::Key::LControl) ||
                             inp.is_key_down(sf::Keyboard::Key::RControl);

            if (ctrl_held) {
                // Ctrl+click on self: attack north if enemy present
                int32_t north_x = t.tile_x;
                int32_t north_y = t.tile_y - 1;
                entity* target = entities_.find_at_tile(north_x, north_y);

                if (target && target->type() == entity_type::monster) {
                    if (can_perform_action()) {
                        spdlog::debug("Ctrl+click on self: attacking north at ({},{})", north_x, north_y);
                        network_.request_attack(target->id(), 0);
                        player->transform().direction = direction::north;
                    } else {
                        // Queue the attack
                        queued_action action;
                        action.type = queued_action_type::attack;
                        action.target_id = target->id();
                        queue_action(action);
                    }
                } else {
                    spdlog::debug("Ctrl+click on self: no enemy to north");
                }
            } else {
                // Click on self without modifiers: pickup item
                if (can_perform_action()) {
                    spdlog::debug("Click on self: sending pickup request at ({},{})", t.tile_x, t.tile_y);
                    request_pickup(t.tile_x, t.tile_y);
                } else {
                    // Queue the pickup
                    queued_action action;
                    action.type = queued_action_type::pickup;
                    action.target_x = t.tile_x;
                    action.target_y = t.tile_y;
                    queue_action(action);
                }
            }
            return;  // Don't process as movement
        }
    }

    // Continuously update movement destination while holding left mouse button
    if (inp.is_mouse_down(sf::Mouse::Button::Left)) {
        auto [dest_x, dest_y] = world_.screen_to_tile(inp.mouse_x(), inp.mouse_y());

        // Don't update movement if hovering over own sprite
        bool hovering_self = entities_.is_point_in_entity_sprite(
            *player, sprites_, world_.camera_x(), world_.camera_y(),
            inp.mouse_x(), inp.mouse_y());
        if (hovering_self) {
            return;
        }

        // Update movement destination (continuous tracking)
        // Never set destination to current tile - nothing to do
        auto& t = player->transform();
        if (dest_x == t.tile_x && dest_y == t.tile_y) {
            return;
        }

        // Allow clicking on any tile (even blocked ones) - pathfinding will
        // move as close as possible and stop at the last walkable tile
        if (dest_x != move_dest_x_ || dest_y != move_dest_y_) {
            move_dest_x_ = dest_x;
            move_dest_y_ = dest_y;
        }
    }

    // Right click initial press - queue stop action to cancel movement
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right)) {
        // Queue stop action - will execute when current animation completes
        pending_action_.type = queued_action_type::stop;
    }

    // Right click held - continuously update face direction
    if (inp.is_mouse_down(sf::Mouse::Button::Right)) {
        auto& t = player->transform();

        // Calculate direction from player to mouse position
        auto [click_x, click_y] = world_.screen_to_tile(inp.mouse_x(), inp.mouse_y());
        int32_t dx = click_x - t.tile_x;
        int32_t dy = click_y - t.tile_y;

        // Determine cardinal/diagonal direction based on relative position
        direction face_dir = direction::none;
        if (dx == 0 && dy == 0) {
            // Mouse on self - don't change direction
        } else if (std::abs(dx) > std::abs(dy) * 2) {
            // Mostly horizontal
            face_dir = (dx > 0) ? direction::east : direction::west;
        } else if (std::abs(dy) > std::abs(dx) * 2) {
            // Mostly vertical
            face_dir = (dy > 0) ? direction::south : direction::north;
        } else {
            // Diagonal
            if (dx > 0 && dy < 0) face_dir = direction::north_east;
            else if (dx > 0 && dy > 0) face_dir = direction::south_east;
            else if (dx < 0 && dy > 0) face_dir = direction::south_west;
            else if (dx < 0 && dy < 0) face_dir = direction::north_west;
        }

        if (face_dir != direction::none && t.direction != face_dir) {
            // Update local direction immediately for visual feedback
            t.direction = face_dir;

            // Send to server only if we can perform an action, otherwise queue it
            if (can_perform_action()) {
                // Convert internal direction (1-8) to protocol format (0-7)
                json msg = make_player_stop_request(t.tile_x, t.tile_y,
                                                    static_cast<uint8_t>(direction_to_protocol(face_dir)));
                ws_connection_.send(msg);
            } else {
                // Queue the direction change - will send when animation completes
                queued_action action;
                action.type = queued_action_type::face_direction;
                action.face_dir = face_dir;
                queue_action(action);
            }
        }
    }

    // Continue moving toward destination (pathfinding)
    // Check cooldown - don't send movement requests during blocked movement cooldown
    if (move_dest_x_ >= 0 && move_dest_y_ >= 0 && !player->transform().moving && can_perform_action()) {
        auto& t = player->transform();

        // Use legacy pathfinding to get next move direction
        direction dir = get_next_move_dir(t.tile_x, t.tile_y, move_dest_x_, move_dest_y_);

        if (dir == direction::none) {
            // Reached destination or path blocked
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            return;
        }

        // Calculate next tile in the direction
        int32_t next_x = t.tile_x;
        int32_t next_y = t.tile_y;

        switch (dir) {
            case direction::north:      next_y -= 1; break;
            case direction::north_east: next_x += 1; next_y -= 1; break;
            case direction::east:       next_x += 1; break;
            case direction::south_east: next_x += 1; next_y += 1; break;
            case direction::south:      next_y += 1; break;
            case direction::south_west: next_x -= 1; next_y += 1; break;
            case direction::west:       next_x -= 1; break;
            case direction::north_west: next_x -= 1; next_y -= 1; break;
            default: break;
        }

        // Check if the next tile is walkable before sending movement request
        if (!world_.current_map().is_walkable(next_x, next_y)) {
            // Terrain blocked - we've reached as close as possible, clear destination
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            return;
        }

        // Check if a living entity is blocking the tile
        auto entities_on_tile = entities_.get_entities_on_tile(next_x, next_y);
        for (auto* e : entities_on_tile) {
            if (e && e->is_alive() && e != player) {
                // Living entity is blocking - don't move but keep destination
                // (entity might move, allowing us to continue)
                return;
            }
        }

        // Never request move to the tile we're already on - just stop and idle
        if (next_x == t.tile_x && next_y == t.tile_y) {
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
            return;
        }

        // Convert internal direction (1-8) to protocol format (0-7)
        uint8_t dir_protocol = static_cast<uint8_t>(direction_to_protocol(dir));

        // Determine run mode (Shift overrides toggle)
        bool should_run = inp.is_key_down(sf::Keyboard::Key::LShift) ||
                         inp.is_key_down(sf::Keyboard::Key::RShift) ||
                         run_mode_enabled_;

        // Client-side prediction - move to next tile
        t.dest_tile_x = next_x;
        t.dest_tile_y = next_y;
        t.direction = dir;
        t.moving = true;
        t.move_progress = 0.0f;

        if (player->has_movement()) {
            player->movement().running = should_run;
        }

        // Set animation state with combat mode
        object_action base_action = should_run ? object_action::run : object_action::move_peace;
        player->set_action_with_combat_mode(base_action, combat_mode_);

        // Send to server via WebSocket (server is authoritative)
        // Protocol: send current position + direction to move
        json msg = make_player_move_request(t.tile_x, t.tile_y, dir_protocol, should_run);
        ws_connection_.send(msg);
    }

    // Keyboard movement
    int32_t dx = 0, dy = 0;
    if (inp.is_key_down(sf::Keyboard::Key::W) || inp.is_key_down(sf::Keyboard::Key::Up)) dy = -1;
    if (inp.is_key_down(sf::Keyboard::Key::S) || inp.is_key_down(sf::Keyboard::Key::Down)) dy = 1;
    if (inp.is_key_down(sf::Keyboard::Key::A) || inp.is_key_down(sf::Keyboard::Key::Left)) dx = -1;
    if (inp.is_key_down(sf::Keyboard::Key::D) || inp.is_key_down(sf::Keyboard::Key::Right)) dx = 1;

    // Check cooldown before processing keyboard movement
    if ((dx != 0 || dy != 0) && can_perform_action()) {
        auto& t = player->transform();
        int32_t target_x = t.tile_x + dx;
        int32_t target_y = t.tile_y + dy;

        if (world_.current_map().is_walkable(target_x, target_y)) {
            // Calculate direction from deltas
            direction move_dir = direction::none;
            if (dx == 0 && dy == -1) move_dir = direction::north;
            else if (dx == 1 && dy == -1) move_dir = direction::north_east;
            else if (dx == 1 && dy == 0) move_dir = direction::east;
            else if (dx == 1 && dy == 1) move_dir = direction::south_east;
            else if (dx == 0 && dy == 1) move_dir = direction::south;
            else if (dx == -1 && dy == 1) move_dir = direction::south_west;
            else if (dx == -1 && dy == 0) move_dir = direction::west;
            else if (dx == -1 && dy == -1) move_dir = direction::north_west;

            if (move_dir != direction::none) {
                bool should_run = inp.is_key_down(sf::Keyboard::Key::LShift) ||
                                 inp.is_key_down(sf::Keyboard::Key::RShift) ||
                                 run_mode_enabled_;
                // Convert internal direction (1-8) to protocol format (0-7)
                json msg = make_player_move_request(t.tile_x, t.tile_y,
                                                    static_cast<uint8_t>(direction_to_protocol(move_dir)),
                                                    should_run);
                ws_connection_.send(msg);
            }
        }
    }
}

void game_state_manager::handle_combat_input(const input& inp) {
    // Right click on enemy to attack (movement input handles right-click for facing direction)
    // This is checked here for attacking enemies specifically
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right)) {
        entity* target = entities_.get_entity_at_screen_pos(
            inp.mouse_x(), inp.mouse_y(),
            world_.camera_x(), world_.camera_y());

        // Only attack if clicking directly on a hostile entity
        if (target && target->id() != entities_.local_player_id() &&
            (target->type() == entity_type::monster || target->type() == entity_type::character)) {
            if (can_perform_action()) {
                network_.request_attack(target->id());
            } else {
                // Queue the attack
                queued_action action;
                action.type = queued_action_type::attack;
                action.target_id = target->id();
                queue_action(action);
            }
            // Don't return - let movement input handle the direction facing
        }
    }

    // Spell hotkeys (1-9)
    for (int i = 0; i < 9; ++i) {
        sf::Keyboard::Key key = static_cast<sf::Keyboard::Key>(
            static_cast<int>(sf::Keyboard::Key::Num1) + i);

        if (inp.is_key_pressed(key)) {
            // Cast spell at mouse position or selected target
            // TODO: Implement spell hotbar
        }
    }
}

void game_state_manager::handle_hotkey_input(const input& inp) {
    // Toggle debug stats (Alt+`)
    if (inp.is_key_pressed(sf::Keyboard::Key::Grave) &&
        (inp.is_key_down(sf::Keyboard::Key::LAlt) || inp.is_key_down(sf::Keyboard::Key::RAlt))) {
        debug::debug_stats::instance().toggle();
        spdlog::info("Debug stats: {}", debug::debug_stats::instance().visible() ? "ON" : "OFF");
    }

    // Toggle cinematic mode (F5)
    if (inp.is_key_pressed(sf::Keyboard::Key::F5)) {
        bool cinematic = !world_.is_cinematic_mode();
        world_.set_cinematic_mode(cinematic);
        spdlog::info("Cinematic mode: {}", cinematic ? "ON" : "OFF");

        // Disable global render mode when exiting cinematic mode
        if (!cinematic && world_.is_global_render_mode()) {
            world_.set_global_render_mode(false);
            entities_.set_global_render_mode(false);
            spdlog::info("Global render mode: OFF (cinematic disabled)");
        }
    }

    // Toggle tile debug overlay (F6) - shows walkability and teleport tiles
    if (inp.is_key_pressed(sf::Keyboard::Key::F6)) {
        auto config = world_.render_config();
        config.show_walkability = !config.show_walkability;
        world_.set_render_config(config);
        spdlog::info("Tile debug overlay: {}", config.show_walkability ? "ON" : "OFF");
    }

    // Toggle zoom mode (Ctrl+Q)
    if (inp.is_key_pressed(sf::Keyboard::Key::Q) &&
        (inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl))) {
        world_.set_zoom_mode_enabled(!world_.is_zoom_mode_enabled());
        spdlog::info("Zoom mode: {}", world_.is_zoom_mode_enabled() ? "ON" : "OFF");
    }

    // Toggle global render mode (Ctrl+G) - only in cinematic mode
    if (inp.is_key_pressed(sf::Keyboard::Key::G) &&
        (inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl))) {
        if (world_.is_cinematic_mode()) {
            bool global = !world_.is_global_render_mode();
            world_.set_global_render_mode(global);
            entities_.set_global_render_mode(global);
            spdlog::info("Global render mode: {}", global ? "ON" : "OFF");
        } else {
            spdlog::info("Global render mode requires cinematic mode (F5)");
        }
    }

    // Camera panning with arrow keys - only in cinematic mode (Shift = 5 tiles)
    if (world_.is_cinematic_mode()) {
        int32_t pan_amount = 32;  // 1 tile
        if (inp.is_key_down(sf::Keyboard::Key::LShift) || inp.is_key_down(sf::Keyboard::Key::RShift)) {
            pan_amount = 32 * 5;  // 5 tiles
        }

        if (inp.is_key_down(sf::Keyboard::Key::Left)) {
            world_.move_camera(-pan_amount, 0);
        }
        if (inp.is_key_down(sf::Keyboard::Key::Right)) {
            world_.move_camera(pan_amount, 0);
        }
        if (inp.is_key_down(sf::Keyboard::Key::Up)) {
            world_.move_camera(0, -pan_amount);
        }
        if (inp.is_key_down(sf::Keyboard::Key::Down)) {
            world_.move_camera(0, pan_amount);
        }
    }

    // Toggle run mode with Ctrl+R
    if ((inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl)) &&
        inp.is_key_pressed(sf::Keyboard::Key::R)) {
        run_mode_enabled_ = !run_mode_enabled_;
        spdlog::debug("Run mode: {}", run_mode_enabled_ ? "enabled" : "disabled");
    }

    // Toggle dialogs
    if (inp.is_key_pressed(sf::Keyboard::Key::C)) {
        ui_.toggle_dialog(dialog_type::character_info);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::I)) {
        ui_.toggle_dialog(dialog_type::inventory);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::E)) {
        ui_.toggle_dialog(dialog_type::equipment);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::K)) {
        ui_.toggle_dialog(dialog_type::skills);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::M)) {
        ui_.toggle_dialog(dialog_type::spellbook);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::P)) {
        ui_.toggle_dialog(dialog_type::party);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::G)) {
        ui_.toggle_dialog(dialog_type::guild);
    }

    if (inp.is_key_pressed(sf::Keyboard::Key::Escape)) {
        ui_.toggle_dialog(dialog_type::system_menu);
    }

    // Toggle attack mode (Tab)
    if (inp.is_key_pressed(sf::Keyboard::Key::Tab)) {
        combat_mode_ = !combat_mode_;
        spdlog::debug("Combat mode toggled: {}", combat_mode_ ? "attack" : "peace");

        // Update player action to reflect new combat stance
        entity* player = local_player();
        if (player && !player->transform().moving) {
            player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
        }
    }

    // Toggle safe attack mode (Home)
    if (inp.is_key_pressed(sf::Keyboard::Key::Home)) {
        safe_attack_mode_ = !safe_attack_mode_;
        spdlog::debug("Safe attack mode toggled: {}", safe_attack_mode_ ? "safe" : "PK");
        // Update will push this to icon panel
    }

    // Help dialog
    if (inp.is_key_pressed(sf::Keyboard::Key::F1)) {
        ui_.toggle_dialog(dialog_type::help);
    }

    // Debug: Test event log colors with Ctrl+Number keys
    bool ctrl_held = inp.is_key_down(sf::Keyboard::Key::LControl) ||
                     inp.is_key_down(sf::Keyboard::Key::RControl);
    if (ctrl_held)
    {
        if (inp.is_key_pressed(sf::Keyboard::Key::Num1)) {
            status_log_.add_event("White: Default message color", message_color::white);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num2)) {
            status_log_.add_event("Green: You gained 150 experience!", message_color::green);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num3)) {
            status_log_.add_event("Red: You took 47 damage!", message_color::red);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num4)) {
            status_log_.add_event("Blue: Magic Missile hits for 32 damage", message_color::blue);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num5)) {
            status_log_.add_event("Yellow: Warning - low health!", message_color::yellow);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num6)) {
            status_log_.add_event("System: Server maintenance in 10 minutes", message_color::system);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num7)) {
            status_log_.add_event("Rainbow cycling color effect!", message_color::rainbow);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num8)) {
            status_log_.add_event("Special per-letter rainbow gradient!", message_color::special);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num9)) {
            status_log_.add_event("Terror shaky bouncing text!", message_color::terror);
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Num0)) {
            status_log_.clear_events();
            spdlog::info("Status log events cleared");
        }
    }
}

void game_state_manager::setup_dialog_callbacks() {
    // Login is now handled by sprite-based screen_manager, not widget dialogs
    // The callbacks are set up in initialize() via screens_.get_login_screen()

    // Character select dialog
    if (auto* select_dlg = dynamic_cast<character_select_dialog*>(ui_.get_dialog(dialog_type::character_select))) {
        // Characters are now populated from server response in enter_state()

        select_dlg->set_on_enter([this](int32_t index) {
            if (index >= 0 && index < static_cast<int32_t>(characters_.size())) {
                int32_t character_id = characters_[index].id;
                spdlog::info("Entering game with character '{}' (ID: {})", characters_[index].name, character_id);
                request_enter_game(character_id);
            }
        });

        select_dlg->set_on_create([this]() {
            spdlog::info("Opening character creation");
            change_state(game_state::create_character);
        });

        select_dlg->set_on_delete([this](int32_t index) {
            spdlog::info("Delete character requested: {}", index);
            // TODO: Implement delete character request
        });
    }

    // Character create dialog
    if (auto* create_dlg = dynamic_cast<character_create_dialog*>(ui_.get_dialog(dialog_type::character_create))) {
        create_dlg->set_on_create([this](const character_create_dialog::character_data& data) {
            spdlog::info("Creating character: {} ({}, {})",
                data.name, data.warrior ? "Warrior" : "Mage", data.gender == 0 ? "Male" : "Female");
            // In real implementation: network_.request_create_character(data);
            change_state(game_state::select_character);
        });

        create_dlg->set_on_cancel([this]() {
            change_state(game_state::select_character);
        });
    }

    // Character dialog - stat point allocation
    if (auto* char_dlg = dynamic_cast<character_dialog*>(ui_.get_dialog(dialog_type::character_info))) {
        char_dlg->set_on_add_stat([this](int stat_index) {
            network_.request_add_stat(stat_index);
        });
    }

    // Inventory dialog - item interactions
    if (auto* inv_dlg = dynamic_cast<inventory_dialog*>(ui_.get_dialog(dialog_type::inventory))) {
        inv_dlg->set_on_item_click([this](int32_t slot) {
            // Select item
            spdlog::debug("Inventory slot {} clicked", slot);
        });

        inv_dlg->set_on_item_right_click([this](int32_t slot) {
            // Show item context menu or use item
            network_.request_use_item(static_cast<uint8_t>(slot));
        });

        inv_dlg->set_on_item_double_click([this](int32_t slot) {
            // Equip or use item
            network_.request_use_item(static_cast<uint8_t>(slot));
        });

        inv_dlg->set_on_item_drag([this](int32_t from_slot, int32_t to_slot) {
            // Move item within inventory
            inventory_.move_item(from_slot, to_slot);
            network_.request_move_item(from_slot, to_slot);
        });
    }

    // Equipment dialog - equip/unequip
    if (auto* equip_dlg = dynamic_cast<equipment_dialog*>(ui_.get_dialog(dialog_type::equipment))) {
        equip_dlg->set_on_slot_click([this](equip_slot slot) {
            // Select equipment slot
            spdlog::debug("Equipment slot {} clicked", static_cast<int>(slot));
        });

        equip_dlg->set_on_slot_right_click([this](equip_slot slot) {
            // Unequip item
            network_.request_unequip(static_cast<uint8_t>(slot));
        });
    }

    // Spellbook dialog - spell casting
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(ui_.get_dialog(dialog_type::spellbook))) {
        spell_dlg->set_on_spell_click([this](uint16_t spell_id) {
            // Select spell for hotbar assignment
            spdlog::debug("Spell {} selected", spell_id);
        });

        spell_dlg->set_on_spell_double_click([this](uint16_t spell_id) {
            // Quick cast spell (if target selected)
            magic_.set_pending_spell(spell_id);
        });
    }

    // Skills dialog - skill usage
    if (auto* skill_dlg = dynamic_cast<skills_dialog*>(ui_.get_dialog(dialog_type::skills))) {
        skill_dlg->set_on_skill_click([this](uint16_t skill_id) {
            spdlog::debug("Skill {} clicked", skill_id);
        });
    }

    // Chat dialog - message sending
    if (auto* chat_dlg = dynamic_cast<chat_dialog*>(ui_.get_dialog(dialog_type::chat))) {
        chat_dlg->set_on_send([this](std::string_view message, chat_dialog::chat_mode mode) {
            switch (mode) {
                case chat_dialog::chat_mode::normal:
                    network_.send_chat(message, 0);
                    break;
                case chat_dialog::chat_mode::shout:
                    network_.send_chat(message, 1);
                    break;
                case chat_dialog::chat_mode::whisper:
                    // Need target name - handled separately
                    break;
                case chat_dialog::chat_mode::party:
                    network_.send_chat(message, 2);
                    break;
                case chat_dialog::chat_mode::guild:
                    network_.send_chat(message, 3);
                    break;
                default:
                    network_.send_chat(message, 0);
                    break;
            }
        });
    }

    // Shop dialog - buy/sell
    if (auto* shop_dlg = dynamic_cast<shop_dialog*>(ui_.get_dialog(dialog_type::shop))) {
        shop_dlg->set_on_buy([this](size_t item_index, uint32_t quantity) {
            network_.request_buy(static_cast<uint16_t>(item_index), static_cast<int32_t>(quantity));
        });
    }

    // Bank dialog - deposit/withdraw
    if (auto* bank_dlg = dynamic_cast<bank_dialog*>(ui_.get_dialog(dialog_type::bank))) {
        bank_dlg->set_on_withdraw([this](int32_t slot) {
            network_.request_bank_withdraw(slot);
        });
    }

    // Party dialog - party actions
    if (auto* party_dlg = dynamic_cast<party_dialog*>(ui_.get_dialog(dialog_type::party))) {
        party_dlg->set_on_invite([this](std::string_view name) {
            network_.request_party_invite(name);
        });

        party_dlg->set_on_leave([this]() {
            network_.request_party_leave();
        });
    }

    // NPC dialog - conversation responses
    if (auto* npc_dlg = dynamic_cast<npc_dialog*>(ui_.get_dialog(dialog_type::npc_dialog))) {
        npc_dlg->set_on_option_select([this](int32_t option_id) {
            network_.send_npc_response(option_id);
        });
    }

    // Trade dialog - player trading
    if (auto* trade_dlg = dynamic_cast<trade_dialog*>(ui_.get_dialog(dialog_type::trade))) {
        trade_dlg->set_on_add_item([this](int32_t slot) {
            // Request to add item from inventory to trade slot
            network_.request_trade_add_item(slot, slot);  // slot is the trade slot position
        });

        trade_dlg->set_on_remove_item([this](int32_t slot) {
            network_.request_trade_remove_item(slot);
        });

        trade_dlg->set_on_confirm([this]() {
            network_.request_trade_confirm();
        });

        trade_dlg->set_on_cancel([this]() {
            network_.request_trade_cancel();
        });

        trade_dlg->set_on_set_gold([this](uint32_t amount) {
            network_.request_trade_set_gold(amount);
        });
    }

    // Craft dialog - item manufacturing
    if (auto* craft_dlg = dynamic_cast<craft_dialog*>(ui_.get_dialog(dialog_type::manufacture))) {
        craft_dlg->set_on_craft([this](int32_t recipe_index) {
            network_.request_craft_item(recipe_index);
        });

        craft_dlg->set_on_add_ingredient([this](int32_t slot) {
            // Add ingredient from inventory
            spdlog::debug("Adding ingredient to craft slot {}", slot);
        });

        craft_dlg->set_on_remove_ingredient([this](int32_t slot) {
            spdlog::debug("Removing ingredient from craft slot {}", slot);
        });
    }

    // Repair dialog - item repair
    if (auto* repair_dlg = dynamic_cast<repair_dialog*>(ui_.get_dialog(dialog_type::repair))) {
        repair_dlg->set_on_repair([this](int32_t inventory_slot) {
            network_.request_repair_item(inventory_slot);
        });

        repair_dlg->set_on_repair_all([this]() {
            network_.request_repair_all();
        });
    }

    // Map dialog - click on location
    if (auto* map_dlg = dynamic_cast<map_dialog*>(ui_.get_dialog(dialog_type::map))) {
        map_dlg->set_on_click_location([this](int32_t x, int32_t y) {
            spdlog::debug("Map clicked at ({}, {})", x, y);
            // Could be used for auto-walk or marking waypoints
        });
    }

    // Icon panel - bottom HUD buttons
    // First try YAML-based icon panel
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        // YAML icon panel uses callbacks set in the dialog constructor via on_button_click
        // We just need to wire up the high-level game actions
        yaml_dlg->set_on_character([this]() {
            ui_.toggle_dialog(dialog_type::character_info);
        });

        yaml_dlg->set_on_inventory([this]() {
            ui_.toggle_dialog(dialog_type::inventory);
        });

        yaml_dlg->set_on_spellbook([this]() {
            ui_.toggle_dialog(dialog_type::spellbook);
        });

        yaml_dlg->set_on_skills([this]() {
            ui_.toggle_dialog(dialog_type::skills);
        });

        yaml_dlg->set_on_chat_history([this]() {
            ui_.toggle_dialog(dialog_type::chat);
        });

        yaml_dlg->set_on_system_menu([this]() {
            // Try YAML-based system menu first
            auto& dlg_mgr = ui_.dialogs();
            auto* sys_menu = dlg_mgr.find_dialog("system_menu");

            if (sys_menu) {
                // YAML system menu exists, toggle it
                if (sys_menu->is_open()) {
                    sys_menu->close();
                } else {
                    sys_menu->open();
                }
            } else if (dlg_mgr.get_definition("system_menu")) {
                // YAML definition exists, create and open
                sys_menu = dlg_mgr.open_dialog("system_menu");
                if (sys_menu) {
                    // Wire up system menu buttons on first creation
                    sys_menu->on_button_click("btn_settings", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        if (auto* settings = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
                            settings->set_ui_style(ui_.style());
                        }
                        ui_.open_dialog(dialog_type::options);
                    });
                    sys_menu->on_button_click("btn_help", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        auto* help_dlg = ui_.dialogs().create_help_dialog();
                        if (help_dlg) {
                            help_dlg->open();
                        } else {
                            ui_.open_dialog(dialog_type::help);
                        }
                    });
                    sys_menu->on_button_click("btn_logout", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        spdlog::info("Logout requested");
                        ws_connection_.disconnect();
                        change_state(game_state::login);
                    });
                    sys_menu->on_button_click("btn_exit", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        spdlog::info("Exit game requested");
                        change_state(game_state::quit);
                    });
                }
            } else {
                // Fall back to legacy code-based system menu
                ui_.toggle_dialog(dialog_type::system_menu);
            }
        });

        yaml_dlg->set_on_combat_indicator([this]() {
            combat_mode_ = !combat_mode_;
            spdlog::debug("Combat mode toggled via click: {}", combat_mode_ ? "attack" : "peace");
        });

        // Sound callback for icon panel buttons
        yaml_dlg->set_on_button_sound([this]() { sounds_.play_ui_sound(14); });
    }
    // Fallback to code-based icon panel
    else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        // Set up UI style and sprite manager for dual rendering support
        icon_dlg->set_ui_style(ui_.style());
        icon_dlg->set_sprite_manager(&sprites_);

        icon_dlg->set_on_character([this]() {
            ui_.toggle_dialog(dialog_type::character_info);
        });

        icon_dlg->set_on_inventory([this]() {
            ui_.toggle_dialog(dialog_type::inventory);
        });

        icon_dlg->set_on_spellbook([this]() {
            ui_.toggle_dialog(dialog_type::spellbook);
        });

        icon_dlg->set_on_skills([this]() {
            ui_.toggle_dialog(dialog_type::skills);
        });

        icon_dlg->set_on_chat_history([this]() {
            // Toggle chat history/log view
            ui_.toggle_dialog(dialog_type::chat);
        });

        icon_dlg->set_on_system_menu([this]() {
            // Use YAML-based system menu dialog
            auto& dlg_mgr = ui_.dialogs();
            auto* sys_menu = dlg_mgr.find_dialog("system_menu");

            if (sys_menu) {
                // Toggle existing dialog
                if (sys_menu->is_open()) {
                    sys_menu->close();
                } else {
                    sys_menu->open();
                }
            } else {
                // Create and open if not yet created
                sys_menu = dlg_mgr.open_dialog("system_menu");
                if (sys_menu) {
                    // Wire up button callbacks on first creation
                    sys_menu->on_button_click("btn_settings", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        // Open settings dialog and initialize with current values
                        if (auto* settings = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
                            settings->set_ui_style(ui_.style());
                        }
                        ui_.open_dialog(dialog_type::options);
                    });

                    sys_menu->on_button_click("btn_help", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        // Use YAML-based help dialog
                        auto* help_dlg = ui_.dialogs().create_help_dialog();
                        if (help_dlg) {
                            help_dlg->open();
                        } else {
                            // Fallback to legacy help dialog
                            ui_.open_dialog(dialog_type::help);
                        }
                    });

                    sys_menu->on_button_click("btn_logout", [this]() {
                        ui_.dialogs().close_dialog("system_menu");
                        spdlog::info("Logout requested");
                        ws_connection_.disconnect();
                        change_state(game_state::login);
                    });

                    sys_menu->on_button_click("btn_exit", [this]() {
                        spdlog::info("Exit requested");
                        change_state(game_state::quit);
                    });
                }
            }
        });

        icon_dlg->set_on_combat_indicator([this]() {
            combat_mode_ = !combat_mode_;
            spdlog::debug("Combat mode toggled via click: {}", combat_mode_ ? "attack" : "peace");
        });

        // Sound callback for icon panel buttons
        icon_dlg->set_on_button_sound([this]() { sounds_.play_ui_sound(14); });
    }

    // Legacy system menu dialog - keep for fallback (can be removed later)
    if (auto* sys_dlg = dynamic_cast<system_menu_dialog*>(ui_.get_dialog(dialog_type::system_menu))) {
        sys_dlg->set_on_settings([this]() {
            if (auto* settings = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
                settings->set_ui_style(ui_.style());
            }
            ui_.open_dialog(dialog_type::options);
        });

        sys_dlg->set_on_help([this]() {
            // Use YAML-based help dialog
            auto* help_dlg = ui_.dialogs().create_help_dialog();
            if (help_dlg) {
                help_dlg->open();
            } else {
                // Fallback to legacy help dialog
                ui_.open_dialog(dialog_type::help);
            }
        });

        sys_dlg->set_on_logout([this]() {
            spdlog::info("Logout requested");
            ws_connection_.disconnect();
            change_state(game_state::login);
        });

        sys_dlg->set_on_exit([this]() {
            spdlog::info("Exit requested");
            change_state(game_state::quit);
        });
    }

    // Settings dialog - game configuration including UI style
    if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
        // Initialize with current config values
        const auto& video = config::instance().video();
        settings_dlg->set_resolution(video.screen_width, video.screen_height);
        settings_dlg->set_fullscreen(video.fullscreen);
        settings_dlg->set_vsync(video.vsync);
        settings_dlg->set_framerate(video.framerate_limit);

        // Initialize audio volumes from config
        const auto& audio_cfg = config::instance().audio();
        settings_dlg->set_music_volume(audio_cfg.music_volume);
        settings_dlg->set_sound_volume(audio_cfg.sfx_volume);

        settings_dlg->set_on_style_change([this](ui_style style) {
            // Apply UI style change immediately for preview
            set_ui_style(style);
            spdlog::info("UI style preview: {}", style == ui_style::classic ? "classic" : "modern");
        });

        settings_dlg->set_on_resolution_change([this](uint32_t width, uint32_t height, bool fullscreen) {
            spdlog::info("Resolution change requested: {}x{} {}", width, height, fullscreen ? "fullscreen" : "windowed");
            if (change_resolution(width, height, fullscreen)) {
                spdlog::info("Resolution changed successfully");
            } else {
                spdlog::warn("Failed to change resolution");
            }
        });

        settings_dlg->set_on_framerate_change([this](uint32_t fps) {
            spdlog::info("Framerate limit changed to: {}", fps == 0 ? "unlimited" : std::to_string(fps));
            config::instance().video().framerate_limit = fps;
            // Only apply framerate limit if vsync is off
            if (renderer_ && !config::instance().video().vsync) {
                renderer_->window().setFramerateLimit(fps);
            }
        });

        settings_dlg->set_on_vsync_change([this](bool vsync) {
            spdlog::info("VSync changed to: {}", vsync ? "enabled" : "disabled");
            auto& video = config::instance().video();
            video.vsync = vsync;
            if (renderer_) {
                renderer_->window().setVerticalSyncEnabled(vsync);
                // When vsync is off, apply framerate limit; when on, disable it
                renderer_->window().setFramerateLimit(vsync ? 0 : video.framerate_limit);
            }
        });

        settings_dlg->set_on_music_volume_change([this](float volume) {
            if (audio_) {
                audio_->set_music_volume(volume);
            }
            // Update config so it persists
            config::instance().audio().music_volume = volume;
        });

        settings_dlg->set_on_sound_volume_change([this](float volume) {
            if (audio_) {
                audio_->set_sound_volume(volume);
            }
            // Update config so it persists
            config::instance().audio().sfx_volume = volume;
        });

        settings_dlg->set_on_apply([this]() {
            spdlog::info("Settings applied");
            // Save config to persist all settings
            config::instance().save();
        });
    }
}

void game_state_manager::update_icon_panel() {
    // Get local player entity
    entity* player = local_player();
    if (!player) return;

    // Try YAML-based icon panel first
    auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel));
    if (yaml_dlg) {
        // Update stats from player entity
        if (player->has_stats()) {
            const auto& stats = player->stats();
            yaml_dlg->set_hp(stats.hp, stats.max_hp);
            yaml_dlg->set_mp(stats.mp, stats.max_mp);
            yaml_dlg->set_sp(stats.sp, stats.max_sp);

            // Calculate experience for next level (simplified)
            int64_t exp_for_next_level = static_cast<int64_t>(stats.level) * 1000;  // Placeholder formula
            yaml_dlg->set_experience(stats.experience, exp_for_next_level, stats.level);
        }

        // Update position from transform
        const auto& transform = player->transform();
        yaml_dlg->set_position(transform.tile_x, transform.tile_y);

        // Set map name from world
        yaml_dlg->set_map_name(world_.current_map_name());

        // Update status effects
        if (player->has_combat()) {
            const auto& combat_comp = player->combat();
            yaml_dlg->set_poisoned(combat_comp.poisoned);
        }

        // Check if equipped weapon skill is mastered (100%)
        bool weapon_mastered = false;
        if (const item* weapon = inventory_.get_equipped(equip_slot::right_hand)) {
            weapon_skill ws = combat_.get_weapon_skill(weapon->type_id);
            weapon_mastered = skills_.is_skill_mastered(static_cast<uint16_t>(ws));
        }
        yaml_dlg->set_super_attack_available(weapon_mastered);

        // Push combat mode state
        yaml_dlg->set_combat_mode(combat_mode_);
        yaml_dlg->set_safe_attack_mode(safe_attack_mode_);
        return;
    }

    // Fallback to code-based icon panel
    auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel));
    if (!icon_dlg) return;

    // Update stats from player entity
    if (player->has_stats()) {
        const auto& stats = player->stats();
        icon_dlg->set_hp(stats.hp, stats.max_hp);
        icon_dlg->set_mp(stats.mp, stats.max_mp);
        icon_dlg->set_sp(stats.sp, stats.max_sp);

        // Calculate experience for next level (simplified)
        int64_t exp_for_next_level = static_cast<int64_t>(stats.level) * 1000;  // Placeholder formula
        icon_dlg->set_experience(stats.experience, exp_for_next_level, stats.level);
    }

    // Update position from transform
    const auto& transform = player->transform();
    icon_dlg->set_position(transform.tile_x, transform.tile_y);

    // Set map name from world
    icon_dlg->set_map_name(world_.current_map_name());

    // Update status effects
    if (player->has_combat()) {
        const auto& combat_comp = player->combat();
        icon_dlg->set_poisoned(combat_comp.poisoned);
    }

    // Check if equipped weapon skill is mastered (100%)
    bool weapon_mastered = false;
    if (const item* weapon = inventory_.get_equipped(equip_slot::right_hand)) {
        weapon_skill ws = combat_.get_weapon_skill(weapon->type_id);
        weapon_mastered = skills_.is_skill_mastered(static_cast<uint16_t>(ws));
    }
    icon_dlg->set_super_attack_available(weapon_mastered);

    // Push combat mode state
    icon_dlg->set_combat_mode(combat_mode_);
    icon_dlg->set_safe_attack_mode(safe_attack_mode_);
}

void game_state_manager::set_ui_style(ui_style style) {
    ui_.set_style(style);

    // Convert ui_style to render_mode for managed dialogs
    render_mode mode = (style == ui_style::modern) ? render_mode::modern : render_mode::classic;

    // Propagate style to icon panel - check both dialog types
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_render_mode(mode);
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        icon_dlg->set_ui_style(style);
    }

    spdlog::info("UI style changed to {}", style == ui_style::modern ? "modern" : "classic");
}

void game_state_manager::attempt_login(const std::string& username, const std::string& password) {
    // Get server address from config
    auto& net_cfg = config::instance().network();
    std::string ws_url = "ws://" + net_cfg.login_server_host + ":" + std::to_string(net_cfg.login_server_port);

    spdlog::info("Connecting to server: {}", ws_url);
    spdlog::info("attempt_login called with username='{}' password_len={}", username, password.length());

    // Store credentials for after connection
    pending_username_ = username;
    pending_password_ = password;
    spdlog::info("Stored pending credentials: username='{}' password_len={}", pending_username_, pending_password_.length());

    // If already connected, send login request directly
    if (ws_connection_.is_connected()) {
        spdlog::info("Already connected, sending login request directly for user: {}", pending_username_);
        json login_msg = make_login_request(pending_username_, pending_password_);
        ws_connection_.send(login_msg);
        return;
    }

    // Set flag to send login request when connection is established
    // We use polling instead of callbacks to avoid thread-safety issues
    pending_login_on_connect_.store(true);

    // NOTE: We use polling (poll_connect_event, poll_disconnect_event, receive)
    // instead of callbacks because ixwebsocket runs callbacks on a background
    // thread, and accessing game state from there causes race conditions.

    // Connect to server
    if (!ws_connection_.connect(ws_url)) {
        show_error("Failed to connect to server");
        return;
    }

    // Show connection dialog (escape to cancel after 7 seconds)
    ui_.show_connection_dialog([this]() {
        // User cancelled - disconnect and return to main menu
        ws_connection_.disconnect();
        pending_username_.clear();
        pending_password_.clear();
        change_state(game_state::main_menu);
    });
}

void game_state_manager::handle_ws_message(const json& message) {
    spdlog::debug("Received WebSocket message: {}", message.dump());

    if (!message.contains("type")) {
        spdlog::warn("Received message without type field");
        return;
    }

    std::string type = message["type"].get<std::string>();

    if (type == msg_type::login_response) {
        handle_login_response_ws(message);
    }
    else if (type == msg_type::get_characters_response) {
        handle_get_characters_response(message);
    }
    else if (type == msg_type::enter_game_response) {
        handle_enter_game_response(message);
    }
    else if (type == msg_type::create_character_response) {
        handle_create_character_response(message);
    }
    else if (type == msg_type::player_pickup_response) {
        handle_pickup_response(message);
    }
    else if (type == msg_type::ground_item_removed) {
        handle_ground_item_removed(message);
    }
    else if (type == msg_type::player_position_update) {
        handle_player_position_update(message);
    }
    else if (type == msg_type::player_stop_response) {
        handle_player_stop_response(message);
    }
    else if (type == msg_type::player_move_response) {
        handle_player_move_response(message);
    }
    else if (type == msg_type::hunger_update) {
        handle_hunger_update(message);
    }
    else {
        spdlog::warn("Unknown message type: {}", type);
    }
}

void game_state_manager::handle_login_response_ws(const json& message) {
    auto response = login_response_data::from_json(message);

    if (response.success) {
        spdlog::info("Login successful! Session: {}", response.session_token);
        session_token_ = response.session_token;

        // Clear pending credentials
        pending_username_.clear();
        pending_password_.clear();

        // Show loading dialog while waiting for character data
        ui_.show_connection_dialog(nullptr);

        // Request character list
        request_characters();
    } else {
        std::string error_msg = response.error_message.empty() ? "Login failed" : response.error_message;
        spdlog::warn("Login failed: {}", error_msg);
        show_error(error_msg);

        // Clear pending credentials
        pending_username_.clear();
        pending_password_.clear();
    }
}

void game_state_manager::handle_get_characters_response(const json& message) {
    auto response = get_characters_response_data::from_json(message);

    // Hide the connection dialog
    ui_.hide_connection_dialog();

    if (response.success) {
        // Convert server characters to our format
        characters_.clear();
        for (const auto& sc : response.characters) {
            character_info info;
            info.id = sc.id;
            info.name = sc.name;
            info.level = static_cast<uint16_t>(sc.level);
            info.warrior = (sc.class_type == 0);  // 0=Warrior per protocol
            // Appearance data (server sends 0=Male, 1=Female; we use 1=Male, 2=Female)
            info.gender = static_cast<uint8_t>(sc.gender == 0 ? 1 : 2);
            info.skin_color = static_cast<uint8_t>(sc.skin_color);
            info.hair_style = static_cast<uint8_t>(sc.hair_style);
            info.hair_color = static_cast<uint8_t>(sc.hair_color);
            info.underwear_color = static_cast<uint8_t>(sc.underwear_color);
            // Equipment data
            info.body_armor = sc.body_armor;
            info.arm_armor = sc.arm_armor;
            info.pants = sc.pants;
            info.boots = sc.boots;
            info.helmet = sc.helmet;
            info.mantle = sc.mantle;
            info.weapon = sc.weapon;
            info.shield = sc.shield;
            characters_.push_back(info);
        }

        spdlog::info("Received {} characters", characters_.size());

        // Transition to character select
        change_state(game_state::select_character);
    } else {
        std::string error_msg = response.error_message.empty() ? "Failed to get characters" : response.error_message;
        spdlog::warn("Get characters failed: {}", error_msg);
        show_error(error_msg);
    }
}

void game_state_manager::handle_enter_game_response(const json& message) {
    auto response = enter_game_response_data::from_json(message);

    // Hide connection dialog
    ui_.hide_connection_dialog();

    if (!response.success) {
        std::string error_msg = response.error_message.empty() ? "Failed to enter game" : response.error_message;
        spdlog::warn("Enter game failed: {}", error_msg);

        // Check for account already in game error - offer force disconnect
        if (response.error_message == "account_already_in_game") {
            int32_t char_id = pending_enter_game_character_id_;
            ui_.create_confirm_box(
                "Session Conflict",
                "This account is already logged in.\nDisconnect other session?",
                [this, char_id](bool confirmed) {
                    if (confirmed) {
                        request_enter_game(char_id, true);
                    }
                }
            );
            return;
        }

        show_error(error_msg);
        return;
    }

    const auto& ch = response.character;
    spdlog::info("Entering game as '{}' on map: {} at ({}, {})",
                 ch.name, ch.map_name, ch.pos_x, ch.pos_y);

    // === Create local player entity ===
    auto& player = entities_.create_entity_with_id(ch.id, entity_type::player);
    entities_.set_local_player(ch.id);

    // Set position (feet at tile center)
    auto& transform = player.transform();
    transform.tile_x = ch.pos_x;
    transform.tile_y = ch.pos_y;
    transform.x = ch.pos_x * 32 + 16;  // Tile center X
    transform.y = ch.pos_y * 32 + 16;  // Tile center Y (feet position)
    spdlog::info("PLAYER SPAWN: tile=({},{}) world=({},{})", ch.pos_x, ch.pos_y, transform.x, transform.y);

    // Set name
    auto& name = player.name();
    name.name = ch.name;

    // Set stats
    auto& stats = player.stats();
    stats.level = static_cast<uint16_t>(ch.level);
    stats.strength = static_cast<uint16_t>(ch.str);
    stats.vitality = static_cast<uint16_t>(ch.vit);
    stats.dexterity = static_cast<uint16_t>(ch.dex);
    stats.intelligence = static_cast<uint16_t>(ch.int_);
    stats.magic = static_cast<uint16_t>(ch.mag);
    stats.charisma = static_cast<uint16_t>(ch.cha);
    stats.hp = ch.hp;
    stats.max_hp = ch.hp_max;
    stats.mp = ch.mp;
    stats.max_mp = ch.mp_max;
    stats.sp = ch.sp;
    stats.max_sp = ch.sp_max;
    stats.experience = static_cast<uint32_t>(ch.experience);
    stats.hunger = static_cast<uint8_t>(ch.hunger_level);

    // Set combat info
    auto& combat = player.combat();
    combat.pk_count = ch.pk_count;

    // Set appearance
    auto& sprite = player.sprite();
    sprite.skin_color = static_cast<uint8_t>(ch.skin_color);
    sprite.hair_style = static_cast<uint8_t>(ch.hair_style);
    sprite.hair_color = static_cast<uint8_t>(ch.hair_color);
    // Server sends 0=Male, 1=Female; sprite_component uses 1=Male, 2=Female
    sprite.gender = (ch.gender == 0) ? 1 : 2;

    // Load character sprites based on appearance
    entities_.load_character_sprites(player, sprites_);

    // Initialize animation state
    auto& anim = player.animation();
    anim.set_state(entity_anim_state::stop);

    spdlog::debug("Player stats - Level: {}, STR: {}, DEX: {}, VIT: {}, INT: {}, MAG: {}, CHA: {}",
                  stats.level, stats.strength, stats.dexterity, stats.vitality,
                  stats.intelligence, stats.magic, stats.charisma);
    spdlog::debug("Player vitals - HP: {}/{}, MP: {}/{}, SP: {}/{}",
                  stats.hp, stats.max_hp, stats.mp, stats.max_mp, stats.sp, stats.max_sp);

    // === Populate inventory ===
    inventory_.clear();
    inventory_.set_gold(static_cast<uint32_t>(ch.gold));

    for (const auto& inv_item : response.inventory.items) {
        item itm;
        itm.id = inv_item.item_id;
        itm.type_id = static_cast<uint16_t>(inv_item.item_id);
        itm.name = inv_item.name;
        itm.amount = static_cast<uint32_t>(inv_item.count);
        itm.durability = static_cast<uint16_t>(inv_item.durability);
        itm.max_durability = static_cast<uint16_t>(inv_item.max_durability);
        inventory_.set_item_at(inv_item.slot, itm);
    }
    spdlog::debug("Loaded {} inventory items, {} gold",
                  response.inventory.items.size(), ch.gold);

    // === Set equipped items ===
    for (const auto& eq_item : response.equipment) {
        item itm;
        itm.id = eq_item.item_id;
        itm.type_id = static_cast<uint16_t>(eq_item.item_id);
        itm.name = eq_item.name;
        itm.durability = static_cast<uint16_t>(eq_item.durability);
        itm.max_durability = static_cast<uint16_t>(eq_item.max_durability);

        // Map equipment slot from server to our enum
        equip_slot slot = equip_slot::none;
        switch (eq_item.slot) {
            case 0: slot = equip_slot::head; break;
            case 1: slot = equip_slot::body; break;
            case 2: slot = equip_slot::arms; break;
            case 3: slot = equip_slot::pants; break;
            case 4: slot = equip_slot::boots; break;
            case 5: slot = equip_slot::right_hand; break;  // Weapon
            case 6: slot = equip_slot::left_hand; break;   // Shield
            case 7: slot = equip_slot::right_finger; break; // Ring 1
            case 8: slot = equip_slot::left_finger; break;  // Ring 2
            case 9: slot = equip_slot::neck; break;         // Amulet
            case 10: slot = equip_slot::back; break;        // Cape
            case 11: slot = equip_slot::none; break;        // Accessory - no direct mapping
            default: slot = equip_slot::none; break;
        }
        if (slot != equip_slot::none) {
            inventory_.set_equipped(slot, itm);
        }
    }
    spdlog::debug("Loaded {} equipped items", response.equipment.size());

    // === Set skill levels ===
    for (const auto& sk : response.skills) {
        // Server sends level 0-200, we store mastery 0-100
        uint8_t mastery = static_cast<uint8_t>(std::min(sk.level / 2, 100));
        skills_.set_mastery(sk.skill_id, mastery);
    }
    spdlog::debug("Loaded {} skills", response.skills.size());

    // === Spawn nearby entities ===
    for (const auto& ent : response.world.entities) {
        entity_type type = entity_type::character;
        if (ent.type == "npc") {
            type = entity_type::npc;
        }

        auto& world_entity = entities_.create_entity_with_id(ent.entity_id, type);

        auto& ent_transform = world_entity.transform();
        ent_transform.tile_x = ent.x;
        ent_transform.tile_y = ent.y;
        ent_transform.x = ent.x * 32;
        ent_transform.y = ent.y * 32;

        // Convert direction from protocol (0-7) to internal enum (1-8)
        ent_transform.direction = direction_from_protocol(ent.direction);

        // Set name (NPCs and characters both have name component)
        if (world_entity.has_name()) {
            world_entity.name().name = ent.name;
        }

        // Set HP from percentage (only for entities with stats component)
        if (world_entity.has_stats()) {
            auto& ent_stats = world_entity.stats();
            ent_stats.hp = ent.hp_percent;
            ent_stats.max_hp = 100;  // Will be updated by later messages
        }
    }
    spdlog::debug("Spawned {} nearby entities", response.world.entities.size());

    // Load map data from AMD file
    if (!world_.load_map(ch.map_name)) {
        spdlog::warn("Failed to load map data for '{}', continuing without tile data", ch.map_name);
        // Still set the map name even if loading fails
        world_.current_map_mut().set_name(ch.map_name);
    }

    // Set player position for camera to follow (convert tile coords to world coords, use tile center)
    int32_t player_world_x = ch.pos_x * 32 + 16;
    int32_t player_world_y = ch.pos_y * 32 + 16;
    world_.set_player_position(player_world_x, player_world_y);
    spdlog::debug("Player position set at tile ({},{}) -> world ({},{})",
                  ch.pos_x, ch.pos_y, player_world_x, player_world_y);

    // Go directly to playing - no loading screen needed
    // Assets are loaded on-demand via sprite_manager
    spdlog::info("Entering game world: {}", ch.map_name);

    // Notify server of our screen dimensions for view range calculation
    send_view_range();

    change_state(game_state::playing);
}

void game_state_manager::handle_create_character_response(const json& message) {
    auto response = create_character_response_data::from_json(message);

    if (response.success) {
        spdlog::info("Character created successfully! ID: {}", response.character_id);
        // Request updated character list and return to character select
        request_characters();
        change_state(game_state::select_character);
    } else {
        std::string error_msg = response.error_message.empty() ? "Failed to create character" : response.error_message;
        spdlog::warn("Character creation failed: {}", error_msg);
        show_error(error_msg);
    }
}

void game_state_manager::handle_pickup_response(const json& message) {
    auto response = player_pickup_response_data::from_json(message);

    if (response.success) {
        spdlog::info("Picked up item: {} x{} (slot {})",
                     response.item_name, response.quantity, response.inventory_slot);

        // TODO: Update inventory UI when inventory system is fully implemented
        // inventory_.add_item(response.inventory_slot, response.item_id, response.item_name, response.quantity);

        // Could show a pickup notification
        // chat_.add_system_message("Picked up " + std::to_string(response.quantity) + " " + response.item_name);
    } else {
        spdlog::debug("Pickup failed: {}", response.error_message);
        // Pickup failed - no item at location or other error
        // This is normal when clicking on self with no items, don't show error
    }

    // Return player to idle after pickup animation
    if (entity* player = local_player()) {
        player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
    }
}

void game_state_manager::handle_ground_item_removed(const json& message) {
    auto data = ground_item_removed_data::from_json(message);

    spdlog::debug("Ground item removed: {} picked up {} at ({},{})",
                  data.picker_name, data.item_name, data.x, data.y);

    // Remove the item entity from the world if it exists
    // The item_id from the server corresponds to the entity ID in our system
    entities_.destroy(data.item_id);
}

void game_state_manager::handle_player_position_update(const json& message) {
    auto data = player_position_update_data::from_json(message);

    // Find the entity
    entity* ent = entities_.get_entity(data.entity_id);
    if (!ent) {
        spdlog::debug("Position update for unknown entity {}", data.entity_id);
        return;
    }

    auto& t = ent->transform();

    // For the local player, don't teleport during movement - let interpolation run
    if (data.entity_id == entities_.local_player_id()) {
        if (t.moving) {
            // Local player is mid-movement - only log, don't teleport
            spdlog::debug("Ignoring position update for moving local player (server: {},{}, dest: {},{})",
                          data.x, data.y, t.dest_tile_x, t.dest_tile_y);
            return;
        }
    }

    // Update transform
    t.tile_x = data.x;
    t.tile_y = data.y;
    t.direction = direction_from_protocol(data.direction);

    // Update world position (centered on tile)
    t.x = data.x * hb::tile_width + 16;
    t.y = data.y * hb::tile_height + 16;

    // Update running state if entity has movement component
    if (ent->has_movement()) {
        ent->movement().running = data.is_running;
    }

    spdlog::debug("Entity {} position updated: ({},{}) dir={} running={}",
                  data.entity_id, data.x, data.y, static_cast<int>(t.direction), data.is_running);
}

void game_state_manager::handle_player_stop_response(const json& message) {
    auto data = player_stop_response_data::from_json(message);

    if (!data.success) {
        spdlog::debug("Player stop request rejected by server");
        return;
    }

    // Update local player position/direction to match server's authoritative state
    entity* player = local_player();
    if (!player) {
        return;
    }

    auto& t = player->transform();
    t.tile_x = data.x;
    t.tile_y = data.y;
    t.direction = direction_from_protocol(data.direction);

    // Update world position (centered on tile)
    t.x = data.x * hb::tile_width + 16;
    t.y = data.y * hb::tile_height + 16;

    spdlog::debug("Player stop confirmed: ({},{}) dir={}",
                  data.x, data.y, static_cast<int>(t.direction));
}

void game_state_manager::handle_player_move_response(const json& message) {
    auto data = player_move_response_data::from_json(message);

    entity* player = local_player();
    if (!player) {
        return;
    }

    if (!data.success) {
        // Movement rejected - revert client-side prediction
        spdlog::debug("Movement rejected: {}", data.error);

        auto& t = player->transform();
        // Stop the movement animation
        t.moving = false;
        t.move_progress = 0.0f;
        t.dest_tile_x = t.tile_x;
        t.dest_tile_y = t.tile_y;

        // Correct position to server's authoritative state
        if (data.x != 0 || data.y != 0) {
            t.tile_x = data.x;
            t.tile_y = data.y;
            t.x = data.x * hb::tile_width + 16;
            t.y = data.y * hb::tile_height + 16;
        }

        // Return to idle animation
        player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);

        // Handle blocked_occupied - apply cooldown and play hurt sound
        if (data.error == "blocked_occupied") {
            // Apply 250ms action cooldown
            blocked_movement_cooldown_ = blocked_movement_cooldown_duration;

            // Clear movement destination to stop pathfinding retry
            move_dest_x_ = -1;
            move_dest_y_ = -1;

            // Play hurt sound: C12 for male, C13 for female
            uint8_t gender = player->sprite().gender;
            int sound_num = (gender == 2) ? 13 : 12;
            sounds_.play_sound('C', sound_num, 0);

            spdlog::debug("Movement blocked by entity, cooldown applied, playing C{}", sound_num);
        }

        // Handle blocked_terrain - apply cooldown (no sound)
        // Server says we're blocked, apply penalty
        if (data.error == "blocked_terrain") {
            blocked_movement_cooldown_ = blocked_movement_cooldown_duration;
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            spdlog::debug("Movement blocked by terrain (server), cooldown applied");
        }

        return;
    }

    // Movement confirmed by server - DON'T immediately teleport
    // Let the client-side animation continue interpolating smoothly
    auto& t = player->transform();

    // Only correct position if there's a significant discrepancy
    // (e.g., server correction due to lag or cheating)
    bool position_mismatch = (t.dest_tile_x != data.x || t.dest_tile_y != data.y);

    if (position_mismatch) {
        // Server says we're at a different position - correct it
        spdlog::warn("Movement position mismatch: client dest ({},{}) vs server ({},{})",
                     t.dest_tile_x, t.dest_tile_y, data.x, data.y);
        t.tile_x = data.x;
        t.tile_y = data.y;
        t.dest_tile_x = data.x;
        t.dest_tile_y = data.y;
        t.x = data.x * hb::tile_width + 16;
        t.y = data.y * hb::tile_height + 16;
        t.moving = false;
        t.move_progress = 0.0f;
        player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);

        // Apply cooldown to prevent spam when server corrects position
        blocked_movement_cooldown_ = blocked_movement_cooldown_duration;

        // Clear movement destination to prevent retry loop
        move_dest_x_ = -1;
        move_dest_y_ = -1;
    }
    // Otherwise let the movement animation continue naturally
    // entity_manager::update_movement() will handle interpolation and completion

    spdlog::debug("Movement confirmed: ({},{}) dir={} (interpolating={})",
                  data.x, data.y, static_cast<int>(t.direction), t.moving);
}

void game_state_manager::handle_hunger_update(const json& message) {
    auto data = hunger_update_data::from_json(message);

    entity* player = local_player();
    if (!player) {
        return;
    }

    player->stats().hunger = static_cast<uint8_t>(std::max(0, static_cast<int>(data.level)));

    // Update status log based on hunger level
    if (data.is_starving) {
        status_log_.set_message("hunger", "Starving! Regeneration blocked.",
                                status_severity::critical);
        spdlog::warn("Player is starving! Regeneration blocked.");
    } else if (data.level < 30) {
        status_log_.set_message("hunger", "Hungry - regeneration delayed",
                                status_severity::warning);
        spdlog::debug("Hunger low: {} - regeneration delayed", data.level);
    } else {
        // Hunger is normal, remove any hunger warning
        status_log_.remove_message("hunger");
        spdlog::debug("Hunger updated: {}", data.level);
    }
}

void game_state_manager::request_characters() {
    spdlog::info("Requesting character list");
    json msg = make_get_characters_request();
    ws_connection_.send(msg);
}

void game_state_manager::request_enter_game(int32_t character_id, bool force_disconnect) {
    spdlog::info("Requesting to enter game with character ID: {}{}",
                 character_id, force_disconnect ? " (force disconnect)" : "");

    // Store character ID for potential retry
    pending_enter_game_character_id_ = character_id;

    // Show waiting dialog while entering game
    ui_.show_connection_dialog(nullptr);

    json msg = make_enter_game_request(character_id, force_disconnect);
    ws_connection_.send(msg);
}

void game_state_manager::request_pickup(int32_t tile_x, int32_t tile_y) {
    spdlog::info("Requesting pickup at ({}, {})", tile_x, tile_y);

    // Play pickup animation
    if (entity* player = local_player()) {
        player->set_action(object_action::get_item);
    }

    json msg = make_player_pickup_request(tile_x, tile_y);
    ws_connection_.send(msg);
}

void game_state_manager::request_create_character(const character_create_data& data) {
    spdlog::info("Requesting to create character: {}", data.name);

    json msg;
    msg["type"] = "create_character_request";
    msg["data"] = {
        {"name", data.name},
        {"gender", data.gender},
        {"skin_color", data.skin_color},
        {"hair_style", data.hair_style},
        {"hair_color", data.hair_color},
        {"underwear_color", data.underwear_color},
        {"strength", data.strength},
        {"vitality", data.vitality},
        {"dexterity", data.dexterity},
        {"intelligence", data.intelligence},
        {"magic", data.magic},
        {"charisma", data.charisma}
    };
    ws_connection_.send(msg);
}

void game_state_manager::send_view_range() {
    const auto& video = config::instance().video();
    json msg = make_set_view_range_request(video.screen_width, video.screen_height);
    ws_connection_.send(msg);
    spdlog::info("Sent view range: {}x{}", video.screen_width, video.screen_height);
}

bool game_state_manager::change_resolution(uint32_t width, uint32_t height, bool fullscreen) {
    if (!renderer_) {
        spdlog::error("Cannot change resolution: renderer not initialized");
        return false;
    }

    // Check if settings dialog is open before resolution change
    bool settings_was_open = false;
    ui_style current_style = ui_style::classic;
    float music_vol = 1.0f;
    float sound_vol = 1.0f;

    if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
        settings_was_open = settings_dlg->visible();
        if (settings_was_open) {
            current_style = settings_dlg->get_ui_style();
            music_vol = settings_dlg->get_music_volume();
            sound_vol = settings_dlg->get_sound_volume();
            settings_dlg->close();
        }
    }

    if (!renderer_->set_resolution(width, height, fullscreen)) {
        return false;
    }

    // Update config
    auto& video = config::instance().video();
    video.screen_width = width;
    video.screen_height = height;
    video.fullscreen = fullscreen;

    // Update world screen size and re-center camera on player
    world_.set_screen_size(width, height);

    // Notify server if we're in the playing state
    if (state_ == game_state::playing) {
        send_view_range();
    }

    // Reopen settings dialog if it was open, with updated position for new resolution
    if (settings_was_open) {
        if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
            // Recenter dialog for new resolution
            int32_t dlg_width = settings_dlg->bounds().width;
            int32_t dlg_height = settings_dlg->bounds().height;
            int32_t new_x = (static_cast<int32_t>(width) - dlg_width) / 2;
            int32_t new_y = (static_cast<int32_t>(height) - dlg_height) / 2;
            settings_dlg->set_position(new_x, new_y);

            // Restore settings state
            settings_dlg->set_ui_style(current_style);
            settings_dlg->set_resolution(width, height);
            settings_dlg->set_fullscreen(fullscreen);
            settings_dlg->set_music_volume(music_vol);
            settings_dlg->set_sound_volume(sound_vol);

            // Tell dialog not to close when Apply handler returns
            settings_dlg->keep_open_after_apply();

            ui_.open_dialog(dialog_type::options);
        }
    }

    // Update icon panel position to snap to bottom of new screen size
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_screen_size(width, height);
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        icon_dlg->set_screen_size(width, height);
    }

    return true;
}

} // namespace hb
