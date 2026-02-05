#pragma once

#include "network/messages.hpp"
#include "ui/screens/character_create_screen.hpp"
#include <atomic>
#include <mutex>
#include <string>

namespace hb {

class game_state_manager;

// Handles all WebSocket JSON messages received from the server.
// Owns session/disconnect state that bridges the background WS thread to the main thread.
class ws_message_handler
{
public:
    ws_message_handler() = default;

    void initialize(game_state_manager& game);
    void clear();

    // Main dispatch entry point (called from game_state_manager::update)
    void handle_message(const json& message);

    // Session state
    const std::string& session_token() const { return session_token_; }
    void clear_session();

    // Pending login state (set before connect, consumed on connect event)
    void set_pending_login(const std::string& username, const std::string& password);
    bool consume_pending_login(std::string& username, std::string& password);

    // WebSocket requests
    void request_characters();
    void request_enter_game(int32_t character_id, bool force_disconnect = false);
    void request_create_character(const character_create_data& data);
    void request_entity_info(uint32_t entity_id);
    void send_view_range();

private:
    // Individual message handlers
    void handle_login_response_ws(const json& message);
    void handle_get_characters_response(const json& message);
    void handle_enter_game_response(const json& message);
    void handle_create_character_response(const json& message);
    void handle_pickup_response(const json& message);
    void handle_ground_item_removed(const json& message);
    void handle_player_position_update(const json& message);
    void handle_player_stop_response(const json& message);
    void handle_player_move_response(const json& message);
    void handle_hunger_update(const json& message);
    void handle_npc_move(const json& message);
    void handle_entity_info_response(const json& message);

    game_state_manager* game_ = nullptr;

    // Session state
    std::string session_token_;
    std::string pending_username_;
    std::string pending_password_;
    std::atomic<bool> pending_login_on_connect_{false};

    // Character enter-game retry
    int32_t pending_enter_game_character_id_ = 0;
};

} // namespace hb
