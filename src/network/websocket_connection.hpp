#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <queue>
#include <mutex>
#include <atomic>
#include <optional>

namespace hb
{

using json = nlohmann::json;

enum class ws_connection_state
{
    disconnected,
    connecting,
    connected,
    failed
};

class websocket_connection
{
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

    // Poll for connection events (call this in update loop for thread-safe event handling)
    // Returns true if a connect event occurred since last poll
    bool poll_connect_event();

    // Poll for disconnection events (call this in update loop for thread-safe event handling)
    // Returns true if a disconnect event occurred, with reason in out_reason
    bool poll_disconnect_event(std::string& out_reason);

    // Get last error message (thread-safe)
    std::string last_error() const
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        return last_error_;
    }

    // Update (call from main loop for ping measurement)
    void update(float delta_time);

    // Statistics
    uint64_t messages_sent() const { return messages_sent_; }
    uint64_t messages_received() const { return messages_received_; }
    int32_t ping_ms() const { return ping_ms_.load(std::memory_order_relaxed); }

private:
    void on_message(const ix::WebSocketMessagePtr& msg);

    ix::WebSocket websocket_;
    std::atomic<ws_connection_state> state_{ws_connection_state::disconnected};

    // last_error_ is written from background thread, read from main thread
    std::string last_error_;
    mutable std::mutex error_mutex_;

    // Message queue (for polling mode) - protected by mutex for thread safety
    std::queue<json> incoming_messages_;
    mutable std::mutex queue_mutex_;

    // Thread-safe event flags for polling
    std::atomic<bool> pending_connect_{false};
    std::atomic<bool> pending_disconnect_{false};
    std::string pending_disconnect_reason_;
    mutable std::mutex disconnect_reason_mutex_;

    // Statistics
    uint64_t messages_sent_ = 0;
    uint64_t messages_received_ = 0;

    // Ping measurement
    std::atomic<int32_t> ping_ms_{0};
    std::chrono::steady_clock::time_point ping_sent_time_;
    std::atomic<bool> ping_pending_{false};
    float ping_timer_ = 0.0f;
    static constexpr float ping_interval_ = 3.0f; // seconds between pings
};

} // namespace hb
