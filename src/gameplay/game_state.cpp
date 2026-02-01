#include "gameplay/game_state.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"
#include "assets/pak_file.hpp"
#include "assets/sprite_manager.hpp"
#include "core/constants.hpp"
#include "core/config.hpp"
#include "ui/dialog_manager.hpp"
#include "ui/managed_dialog.hpp"
#include "ui/dialogs/icon_panel_dialog.hpp"
#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include <spdlog/spdlog.h>

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
    combat_.initialize(&entities_, &magic_, &skills_);

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

    // Set up character renderer for menu screens
    screens_.get_character_select_screen().set_character_renderer(&menu_char_renderer_);
    screens_.get_character_create_screen().set_character_renderer(&menu_char_renderer_);

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
    ui_.create_gauge_panel_dialog();
    ui_.create_levelup_dialog();

    // Wire up dialog callbacks (for in-game dialogs)
    setup_dialog_callbacks();

    // Try to load asset packs for world/terrain
    terrain_pak_ = std::make_unique<pak_file>();
    sprite_pak_ = std::make_unique<pak_file>();

    // Assets might not exist yet, that's OK
    if (terrain_pak_->open("assets/tiles.pak")) {
        spdlog::info("Loaded terrain pak");
    }

    if (sprite_pak_->open("assets/sprites.pak")) {
        spdlog::info("Loaded sprite pak");
        if (terrain_pak_->is_open()) {
            world_.initialize(*terrain_pak_, *sprite_pak_);
        }
    }

    // Start at main menu - directly enter the state since it's the initial state
    enter_state(game_state::main_menu);

    spdlog::info("Game state manager initialized");
    return true;
}

void game_state_manager::shutdown() {
    exit_state(state_);

    screens_.shutdown();
    network_.shutdown();
    world_.shutdown();
    entities_.shutdown();
    ui_.shutdown();
    sprites_.clear_cache();

    terrain_pak_.reset();
    sprite_pak_.reset();

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

    // Handle pending disconnect from background thread
    if (has_pending_disconnect_.exchange(false)) {
        std::string reason;
        {
            std::lock_guard<std::mutex> lock(pending_disconnect_mutex_);
            reason = std::move(pending_disconnect_reason_);
        }
        // Now safe to access pending_username_ and show_error on main thread
        if (!pending_username_.empty()) {
            show_error("Connection lost: " + reason);
            pending_username_.clear();
            pending_password_.clear();
        }
    }

    // Update sprite memory management (evict unused bitmaps)
    sprites_.update_memory(delta_time);

    // Update based on current state
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
        default:
            break;
    }

    // Update UI
    ui_.update(delta_time, inp);
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
        default:
            break;
    }

    // Render UI on top
    ui_.render(rend);

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
            // Open HUD elements
            ui_.open_dialog(dialog_type::chat);
            ui_.open_dialog(dialog_type::icon_panel);
            // Note: gauge_panel is now integrated into icon_panel
            // Initialize icon panel with player data
            update_icon_panel();
            break;

        default:
            break;
    }
}

void game_state_manager::exit_state(game_state state) {
    spdlog::debug("Exiting state: {}", static_cast<int>(state));

    switch (state) {
        case game_state::playing:
            // Save any state if needed
            break;

        default:
            break;
    }
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

    // Update world
    world_.update(delta_time);

    // Update entities
    entities_.update(delta_time, world_);

    // Update combat
    combat_.update(delta_time);

    // Update magic effects
    magic_.update_effects(delta_time);

    // Update skill cooldowns
    skills_.update_cooldowns(delta_time);

    // Update HUD with current player stats
    update_icon_panel();
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
    // Render world
    world_.render(rend);

    // Render entities
    entities_.render(rend, world_.camera_x(), world_.camera_y());
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

    handle_movement_input(inp);
    handle_combat_input(inp);
    handle_hotkey_input(inp);
}

void game_state_manager::handle_movement_input(const input& inp) {
    entity* player = local_player();
    if (!player || !player->has_movement()) {
        return;
    }

    // Click to move
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left)) {
        auto [tile_x, tile_y] = world_.screen_to_tile(inp.mouse_x(), inp.mouse_y());

        // Check if tile is walkable
        if (world_.current_map().is_walkable(tile_x, tile_y)) {
            // Request movement from server
            network_.request_move(tile_x, tile_y, 0);
        }
    }

    // Keyboard movement
    int32_t dx = 0, dy = 0;
    if (inp.is_key_down(sf::Keyboard::Key::W) || inp.is_key_down(sf::Keyboard::Key::Up)) dy = -1;
    if (inp.is_key_down(sf::Keyboard::Key::S) || inp.is_key_down(sf::Keyboard::Key::Down)) dy = 1;
    if (inp.is_key_down(sf::Keyboard::Key::A) || inp.is_key_down(sf::Keyboard::Key::Left)) dx = -1;
    if (inp.is_key_down(sf::Keyboard::Key::D) || inp.is_key_down(sf::Keyboard::Key::Right)) dx = 1;

    if (dx != 0 || dy != 0) {
        auto& t = player->transform();
        int32_t target_x = t.tile_x + dx;
        int32_t target_y = t.tile_y + dy;

        if (world_.current_map().is_walkable(target_x, target_y)) {
            network_.request_move(target_x, target_y, 0);
        }
    }
}

void game_state_manager::handle_combat_input(const input& inp) {
    // Right click to attack
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right)) {
        // World coordinates (for AoE spells targeting locations)
        // int32_t world_x = inp.mouse_x() + world_.camera_x();
        // int32_t world_y = inp.mouse_y() + world_.camera_y();

        entity* target = entities_.get_entity_at_screen_pos(
            inp.mouse_x(), inp.mouse_y(),
            world_.camera_x(), world_.camera_y());

        if (target && target->id() != entities_.local_player_id()) {
            network_.request_attack(target->id());
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
        // Update will push this to icon panel
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
        settings_dlg->set_on_style_change([this](ui_style style) {
            // Apply UI style change immediately for preview
            set_ui_style(style);
            spdlog::info("UI style preview: {}", style == ui_style::classic ? "classic" : "modern");
        });

        settings_dlg->set_on_music_volume_change([this](float volume) {
            if (audio_) {
                audio_->set_music_volume(volume);
            }
        });

        settings_dlg->set_on_sound_volume_change([this](float volume) {
            if (audio_) {
                audio_->set_sound_volume(volume);
            }
        });

        settings_dlg->set_on_apply([this]() {
            spdlog::info("Settings applied");
            // Settings are already applied in real-time via the callbacks above
            // This is just for confirmation/saving
            // TODO: Save settings to config file
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

    // Propagate style to icon panel (and any other style-aware dialogs)
    // Note: yaml_icon_panel_dialog uses render_mode from dialog_manager, not ui_style
    if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
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

    // Set up callbacks for new connection
    ws_connection_.set_connect_callback([this]() {
        // Connection established - send login request
        spdlog::info("Connected! Sending login request for user: {}", pending_username_);
        json login_msg = make_login_request(pending_username_, pending_password_);
        ws_connection_.send(login_msg);
    });

    ws_connection_.set_disconnect_callback([this](const std::string& reason) {
        spdlog::warn("Disconnected from server: {}", reason);
        // Queue the disconnect for main thread processing (avoid race conditions)
        // Don't access pending_username_ here - it's not thread-safe
        {
            std::lock_guard<std::mutex> lock(pending_disconnect_mutex_);
            pending_disconnect_reason_ = reason;
        }
        has_pending_disconnect_.store(true);
    });

    // NOTE: Do NOT set message_callback here - it runs on a background thread
    // and modifying UI from there causes race conditions. Instead, we poll
    // for messages on the main thread in update().

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
            info.warrior = (sc.char_class == "warrior");
            // Appearance data
            info.gender = sc.gender;
            info.skin_color = sc.skin_color;
            info.hair_style = sc.hair_style;
            info.hair_color = sc.hair_color;
            info.underwear_color = sc.underwear_color;
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
        show_error(error_msg);
        return;
    }

    const auto& ch = response.character;
    spdlog::info("Entering game as '{}' on map: {} at ({}, {})",
                 ch.name, ch.map_name, ch.pos_x, ch.pos_y);

    // === Create local player entity ===
    auto& player = entities_.create_entity_with_id(ch.id, entity_type::player);
    entities_.set_local_player(ch.id);

    // Set position
    auto& transform = player.transform();
    transform.tile_x = ch.pos_x;
    transform.tile_y = ch.pos_y;
    transform.x = ch.pos_x * 32;  // World coords
    transform.y = ch.pos_y * 32;

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

        // Convert direction (1-8 server format)
        if (ent.direction >= 1 && ent.direction <= 8) {
            ent_transform.direction = static_cast<direction>(ent.direction);
        }

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

    // Set current map name in world
    world_.current_map_mut().set_name(ch.map_name);

    // Go directly to playing - no loading screen needed
    // Assets are loaded on-demand via sprite_manager
    spdlog::info("Entering game world: {}", ch.map_name);
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

void game_state_manager::request_characters() {
    spdlog::info("Requesting character list");
    json msg = make_get_characters_request();
    ws_connection_.send(msg);
}

void game_state_manager::request_enter_game(int32_t character_id) {
    spdlog::info("Requesting to enter game with character ID: {}", character_id);

    // Show waiting dialog while entering game
    ui_.show_connection_dialog(nullptr);

    json msg = make_enter_game_request(character_id);
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

} // namespace hb
