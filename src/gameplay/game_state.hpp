#pragma once

#include "core/game_enums.hpp"
#include "core/launch_options.hpp"
#include "gameplay/combat.hpp"
#include "gameplay/dialog_callbacks.hpp"
#include "gameplay/guild_system.hpp"
#include "gameplay/input_handler.hpp"
#include "gameplay/inventory.hpp"
#include "gameplay/magic.hpp"
#include "gameplay/skills.hpp"
#include "gameplay/chat_input_overlay.hpp"
#include "gameplay/ws_message_handler.hpp"
#include "network/network_system.hpp"
#include "network/websocket_connection.hpp"
#include "network/messages.hpp"
#include "network/handlers/notify_handlers.hpp"
#include "network/handlers/motion_handlers.hpp"
#include "entity/entity_manager.hpp"
#include "world/ground_item.hpp"
#include "world/world.hpp"
#include "ui/ui_system.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "ui/screens/screen_manager.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/tile_sprite_registry.hpp"
#include "graphics/menu_character_renderer.hpp"
#include "audio/sound_manager.hpp"
#include "gameplay/floating_text.hpp"
#include "gameplay/effect_system.hpp"
#include "world/weather_system.hpp"
#include "graphics/screen_transition.hpp"
#include "ui/status_log.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace hb {

class renderer;
class input;
class audio;
class cursor_manager;

// Quest objective progress
struct quest_objective {
    uint16_t id = 0;
    uint8_t status = 0;          // 0=incomplete, 1=complete, 2=failed
    int32_t current = 0;
    int32_t required = 0;
};

// Active quest state
struct active_quest {
    uint16_t quest_id = 0;
    uint8_t status = 0;          // 0=available, 1=active, 2=complete, 3=turned_in, 4=failed, 5=abandoned
    std::vector<quest_objective> objectives;
};

// Quest log holding active and completed quests
struct quest_log {
    std::vector<active_quest> active;
    std::vector<uint16_t> completed;

    void clear()
    {
        active.clear();
        completed.clear();
    }

    bool is_quest_completed(uint16_t quest_id) const
    {
        return std::find(completed.begin(), completed.end(), quest_id) != completed.end();
    }

    const active_quest* find_active(uint16_t quest_id) const
    {
        for (const auto& q : active)
        {
            if (q.quest_id == quest_id) return &q;
        }
        return nullptr;
    }
};

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
    void set_cursor_manager(cursor_manager& cursor) { cursor_ = &cursor; }
    void shutdown();

    // Async initialization - call each frame after initialize() until done
    bool has_pending_init_steps() const;
    std::pair<float, std::string> run_next_init_step();

    // Main loop
    void update(float delta_time, const hb::input& inp);
    void render(renderer& rend);

    // State transitions
    void change_state(game_state new_state);
    game_state current_state() const { return state_; }

    // Network
    network_system& network() { return network_; }
    websocket_connection& ws_connection() { return ws_connection_; }

    // Launch options (command-line credentials for auto-login)
    void set_launch_options(launch_options opts);

    // WebSocket login
    void attempt_login(const std::string& username, const std::string& password);
    bool is_ws_connected() const { return ws_connection_.is_connected(); }

    // Player actions
    void request_pickup(int32_t tile_x, int32_t tile_y);

    // Subsystems
    world& game_world() { return world_; }
    entity_manager& entities() { return entities_; }
    ui_system& ui() { return ui_; }
    inventory_system& inventory() { return inventory_; }
    magic_system& magic() { return magic_; }
    skills_system& skills() { return skills_; }
    combat_system& combat() { return combat_; }
    sound_manager& sounds() { return sounds_; }
    effect_system& effects() { return effects_; }
    guild_system& guild() { return guild_; }
    const guild_system& guild() const { return guild_; }
    quest_log& quests() { return quests_; }
    const quest_log& quests() const { return quests_; }
    hb::sprite_manager& sprites() { return sprites_; }
    hb::status_log& get_status_log() { return status_log_; }
    floating_text_manager& floating_text() { return floating_text_; }
    screen_transition& transition() { return transition_; }
    ground_item_manager& ground_items() { return ground_items_; }
    weather_system& weather() { return weather_system_; }

    // Extracted subsystems
    hb::input_handler& input_handler() { return input_handler_; }
    ws_message_handler& ws_handler() { return ws_handler_; }
    chat_input_overlay& chat_input() { return chat_input_; }

    // Local player
    void set_local_player_id(entity_id id);
    entity* local_player();
    const entity* local_player() const;

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

    // Combat mode (forwarded to input_handler)
    bool is_combat_mode() const { return input_handler_.is_combat_mode(); }
    void toggle_combat_mode() { input_handler_.toggle_combat_mode(); ws_handler_.request_combat_mode_toggle(); }

    // Camera drag lock (forwarded to input_handler)
    void set_camera_drag_locked(bool locked) { input_handler_.set_camera_drag_locked(locked); }
    bool is_camera_drag_locked() const { return input_handler_.is_camera_drag_locked(); }

    // Movement destination (forwarded to input_handler)
    void set_move_dest(int32_t x, int32_t y) { input_handler_.set_move_dest(x, y); }

    // Screen transition for map teleports
    void start_map_transition(std::function<void()> on_midpoint);
    bool is_transitioning() const { return transition_.is_active(); }
    void cancel_transition() { transition_.cancel(); }

    // Resolution change API
    bool change_resolution(uint32_t width, uint32_t height, bool fullscreen,
                           bool borderless = false, int32_t monitor_x = 0, int32_t monitor_y = 0);

    // Spell hotbar
    void set_spell_hotbar_slot(size_t slot, uint16_t spell_id);
    uint16_t get_spell_hotbar_slot(size_t slot) const;
    void auto_populate_spell_hotbar();

    // Internal accessors for extracted subsystems
    renderer* get_renderer() { return renderer_; }
    audio* get_audio() { return audio_; }
    cursor_manager* get_cursor() { return cursor_; }

    // View range
    void send_view_range();
    void set_view_radius(int16_t radius_x, int16_t radius_y, bool sees_all);
    int16_t view_radius_x() const { return view_radius_x_; }
    int16_t view_radius_y() const { return view_radius_y_; }
    bool sees_all() const { return sees_all_; }

    // WebSocket requests (forwarded to ws_handler)
    void request_enter_game(int32_t character_id, bool force_disconnect = false)
    {
        ws_handler_.request_enter_game(character_id, force_disconnect);
    }

    // HUD updates
    void update_icon_panel();

private:
    // State handlers
    void enter_state(game_state state);
    void exit_state(game_state state);

    void refresh_character_select_screen();

    void update_main_menu(float delta_time, const hb::input& inp);
    void update_login(float delta_time, const hb::input& inp);
    void update_character_select(float delta_time, const hb::input& inp);
    void update_character_create(float delta_time, const hb::input& inp);
    void update_loading(float delta_time, const hb::input& inp);
    void update_playing(float delta_time, const hb::input& inp);

    void render_main_menu(renderer& rend);
    void render_login(renderer& rend);
    void render_character_select(renderer& rend);
    void render_character_create(renderer& rend);
    void render_loading(renderer& rend);
    void render_playing(renderer& rend);

    // Network handlers (legacy binary protocol)
    void setup_network_handlers();
    void handle_login_response(packet_reader& reader);
    void handle_character_list(packet_reader& reader);
    void handle_enter_game(packet_reader& reader);
    void handle_player_data(packet_reader& reader);
    void handle_map_data(packet_reader& reader);

    // Clear all in-game data (map, entities, etc.) for clean re-entry
    void clear_game_data();

    // State
    game_state state_ = game_state::main_menu;
    game_state pending_state_ = game_state::main_menu;
    bool state_transition_ = false;

    // Command-line credentials (persist for reconnect)
    launch_options launch_options_;
    bool first_launch_connect_ = true;

    // WebSocket connection retry state
    bool ws_connecting_ = false;       // true while actively trying to establish connection
    float ws_retry_timer_ = 0.0f;     // accumulates until retry_delay, then retries
    static constexpr float ws_retry_delay_ = 2.0f;

    // Server-controlled view radius
    int16_t view_radius_x_ = 40;  // Visibility radius in tiles (15-80)
    int16_t view_radius_y_ = 40;
    bool sees_all_ = false;        // Player sees all events on current map

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
    guild_system guild_;
    quest_log quests_;

    // Extracted subsystems
    hb::input_handler input_handler_;
    dialog_callbacks dialog_callbacks_;
    ws_message_handler ws_handler_;
    chat_input_overlay chat_input_;

    // Network handlers (legacy binary protocol)
    notify_handler notify_handler_;
    motion_handler motion_handler_;

    // Assets
    hb::sprite_manager sprites_;
    tile_sprite_registry tile_registry_;

    // Character rendering for menus
    menu_character_renderer menu_char_renderer_;

    // Screens (sprite-based UI for login, main menu, etc.)
    screen_manager screens_;
    renderer* renderer_ = nullptr;
    cursor_manager* cursor_ = nullptr;

    // Character selection
    std::vector<character_info> characters_;
    size_t selected_character_ = 0;

    // Loading
    float loading_progress_ = 0.0f;
    std::string loading_message_;

    // Async initialization step queue
    struct init_step {
        std::string message;
        std::function<void()> action;
    };
    std::vector<init_step> init_steps_;
    size_t init_step_index_ = 0;

    // Audio reference
    audio* audio_ = nullptr;

    // Sound manager for SFX and BGM
    sound_manager sounds_;

    // Status log for persistent status messages (hunger, etc.)
    hb::status_log status_log_;

    // Screen transition overlay (diamond wave dissolve)
    screen_transition transition_;

    // Visual effects (spell explosions, projectiles, particles)
    effect_system effects_;

    // Ground items (separate from entity system)
    ground_item_manager ground_items_;

    // Weather particles and day/night overlay
    weather_system weather_system_;

    // Floating text for damage numbers, heals, etc.
    floating_text_manager floating_text_;

    // Spell hotbar (9 slots, indexed 0-8 for keys 1-9)
    std::array<uint16_t, 9> spell_hotbar_{};
};

} // namespace hb
