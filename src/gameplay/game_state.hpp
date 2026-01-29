#pragma once

#include "core/game_enums.hpp"
#include "gameplay/combat.hpp"
#include "gameplay/inventory.hpp"
#include "gameplay/magic.hpp"
#include "gameplay/skills.hpp"
#include "network/network_system.hpp"
#include "network/websocket_connection.hpp"
#include "network/messages.hpp"
#include "network/handlers/notify_handlers.hpp"
#include "network/handlers/motion_handlers.hpp"
#include "entity/entity_manager.hpp"
#include "world/world.hpp"
#include "ui/ui_system.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "ui/screens/screen_manager.hpp"
#include "assets/pak_file.hpp"
#include "assets/sprite_manager.hpp"
#include "graphics/menu_character_renderer.hpp"
#include <cstdint>
#include <string>
#include <functional>
#include <memory>

namespace hb {

class renderer;
class input;
class audio;

// Character data for character select
struct character_info {
    int32_t id = 0;  // Server-assigned character ID
    std::string name;
    uint16_t level = 1;
    uint16_t exp_level = 0;  // Experience level percentage
    uint8_t gender = 1;         // 1 = male, 2 = female
    uint8_t skin_color = 1;     // 1-3
    uint8_t hair_style = 0;     // 0-7
    uint8_t hair_color = 0;     // 0-15
    uint8_t underwear_color = 0; // 0-7
    uint16_t strength = 0;
    uint16_t vitality = 0;
    uint16_t dexterity = 0;
    uint16_t intelligence = 0;
    uint16_t magic = 0;
    uint16_t charisma = 0;
    std::string map_name;
    bool warrior = true;  // true = warrior, false = mage

    // Equipment (0 = not equipped)
    uint8_t body_armor = 0;
    uint8_t arm_armor = 0;
    uint8_t pants = 0;
    uint8_t boots = 0;
    uint8_t helmet = 0;
    uint8_t mantle = 0;
    uint8_t weapon = 0;
    uint8_t shield = 0;
};

// Main game state manager
class game_state_manager {
public:
    game_state_manager() = default;
    ~game_state_manager() = default;

    game_state_manager(const game_state_manager&) = delete;
    game_state_manager& operator=(const game_state_manager&) = delete;

    // Initialization
    bool initialize(renderer& rend, audio& aud);
    void shutdown();

    // Main loop
    void update(float delta_time, const input& inp);
    void render(renderer& rend);

    // State transitions
    void change_state(game_state new_state);
    game_state current_state() const { return state_; }

    // Network
    network_system& network() { return network_; }

    // WebSocket login
    void attempt_login(const std::string& username, const std::string& password);
    bool is_ws_connected() const { return ws_connection_.is_connected(); }

    // Subsystems
    world& game_world() { return world_; }
    entity_manager& entities() { return entities_; }
    ui_system& ui() { return ui_; }
    inventory_system& inventory() { return inventory_; }
    magic_system& magic() { return magic_; }
    skills_system& skills() { return skills_; }
    combat_system& combat() { return combat_; }

    // Local player
    void set_local_player_id(entity_id id);
    entity* local_player();

    // Character selection
    void set_characters(std::vector<character_info> characters);
    const std::vector<character_info>& characters() const { return characters_; }
    void select_character(size_t index);
    size_t selected_character() const { return selected_character_; }

    // Login results (using protocol response codes)
    void set_login_result(uint16_t result);
    void set_character_create_result(uint16_t result);

    // Messages
    void show_message(std::string_view message);
    void show_error(std::string_view error);

    // UI style switching (modern vs classic)
    void set_ui_style(ui_style style);
    ui_style get_ui_style() const { return ui_.style(); }

private:
    // State handlers
    void enter_state(game_state state);
    void exit_state(game_state state);

    void update_main_menu(float delta_time, const input& inp);
    void update_login(float delta_time, const input& inp);
    void update_character_select(float delta_time, const input& inp);
    void update_character_create(float delta_time, const input& inp);
    void update_loading(float delta_time, const input& inp);
    void update_playing(float delta_time, const input& inp);

    void render_main_menu(renderer& rend);
    void render_login(renderer& rend);
    void render_character_select(renderer& rend);
    void render_character_create(renderer& rend);
    void render_loading(renderer& rend);
    void render_playing(renderer& rend);

    // Network handlers
    void setup_network_handlers();
    void handle_login_response(packet_reader& reader);
    void handle_character_list(packet_reader& reader);
    void handle_enter_game(packet_reader& reader);
    void handle_player_data(packet_reader& reader);
    void handle_map_data(packet_reader& reader);

    // Input handling
    void handle_playing_input(const input& inp);
    void handle_movement_input(const input& inp);
    void handle_combat_input(const input& inp);
    void handle_hotkey_input(const input& inp);

    // Dialog callbacks
    void setup_dialog_callbacks();

    // HUD updates
    void update_icon_panel();

    // WebSocket message handling
    void handle_ws_message(const json& message);
    void handle_login_response_ws(const json& message);
    void handle_get_characters_response(const json& message);
    void handle_enter_game_response(const json& message);
    void handle_create_character_response(const json& message);

    // WebSocket requests
    void request_characters();
    void request_enter_game(int32_t character_id);
    void request_create_character(const character_create_data& data);

    // State
    game_state state_ = game_state::main_menu;
    game_state pending_state_ = game_state::main_menu;
    bool state_transition_ = false;

    // Subsystems
    network_system network_;
    websocket_connection ws_connection_;
    world world_;
    entity_manager entities_;
    ui_system ui_;
    combat_system combat_;
    inventory_system inventory_;
    magic_system magic_;
    skills_system skills_;

    // WebSocket state
    std::string session_token_;
    std::string pending_username_;
    std::string pending_password_;

    // Network handlers
    notify_handler notify_handler_;
    motion_handler motion_handler_;

    // Assets
    std::unique_ptr<pak_file> terrain_pak_;
    std::unique_ptr<pak_file> sprite_pak_;
    sprite_manager sprites_;

    // Character rendering for menus
    menu_character_renderer menu_char_renderer_;

    // Screens (sprite-based UI for login, main menu, etc.)
    screen_manager screens_;
    renderer* renderer_ = nullptr;  // Stored for screen rendering

    // Character selection
    std::vector<character_info> characters_;
    size_t selected_character_ = 0;

    // Loading
    float loading_progress_ = 0.0f;
    std::string loading_message_;

    // Audio reference
    audio* audio_ = nullptr;
};

} // namespace hb
