#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <queue>
#include <mutex>
#include <optional>
#include <functional>

namespace hb {

using json = nlohmann::json;

enum class ws_connection_state {
    disconnected,
    connecting,
    connected,
    failed
};

// Callbacks
using message_callback = std::function<void(const json&)>;
using connect_callback = std::function<void()>;
using disconnect_callback = std::function<void(const std::string& reason)>;

class websocket_connection {
public:
    websocket_connection();
    ~websocket_connection();

    websocket_connection(const websocket_connection&) = delete;
    websocket_connection& operator=(const websocket_connection&) = delete;

    // Connect to server (ws://host:port)
    bool connect(std::string_view url);
    void disconnect();

    // State
    ws_connection_state state() const { return state_; }
    bool is_connected() const { return state_ == ws_connection_state::connected; }

    // Send JSON message
    bool send(const json& message);

    // Receive messages (call this in update loop)
    // Returns the next available message, or nullopt if none
    std::optional<json> receive();

    // Set callback for incoming messages (alternative to polling with receive())
    void set_message_callback(message_callback callback) { message_callback_ = std::move(callback); }

    // Set callback for connection established
    void set_connect_callback(connect_callback callback) { connect_callback_ = std::move(callback); }

    // Set callback for disconnection
    void set_disconnect_callback(disconnect_callback callback) { disconnect_callback_ = std::move(callback); }

    // Get last error message
    std::string_view last_error() const { return last_error_; }

    // Statistics
    uint64_t messages_sent() const { return messages_sent_; }
    uint64_t messages_received() const { return messages_received_; }

private:
    void on_message(const ix::WebSocketMessagePtr& msg);

    ix::WebSocket websocket_;
    ws_connection_state state_ = ws_connection_state::disconnected;
    std::string last_error_;

    // Message queue (for polling mode)
    std::queue<json> incoming_messages_;
    std::mutex queue_mutex_;

    // Callback mode
    message_callback message_callback_;
    connect_callback connect_callback_;
    disconnect_callback disconnect_callback_;

    // Statistics
    uint64_t messages_sent_ = 0;
    uint64_t messages_received_ = 0;
};

} // namespace hb
