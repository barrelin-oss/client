#include "network/websocket_connection.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

websocket_connection::websocket_connection()
{
    // Disable automatic reconnection - we'll create new connections as needed
    // based on game state (only reconnect during character select or in-game)
    websocket_.disableAutomaticReconnection();

    // Configure WebSocket
    websocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) { on_message(msg); });
}

websocket_connection::~websocket_connection()
{
    disconnect();
}

bool websocket_connection::connect(std::string_view url)
{
    // Always stop the underlying websocket before starting a new connection.
    // A server-initiated close sets our state to disconnected but doesn't call
    // websocket_.stop(), leaving the internal thread running. stop() is safe
    // to call in any state.
    websocket_.stop();

    // Reset all state for a fresh connection
    state_.store(ws_connection_state::connecting);
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        last_error_.clear();
    }

    // Clear any pending events from previous connections
    pending_connect_.store(false);
    pending_disconnect_.store(false);
    {
        std::lock_guard<std::mutex> lock(disconnect_reason_mutex_);
        pending_disconnect_reason_.clear();
    }

    // Clear message queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!incoming_messages_.empty())
        {
            incoming_messages_.pop();
        }
    }

    std::string url_str(url);
    spdlog::info("WebSocket connecting to: {}", url_str);

    websocket_.setUrl(url_str);

    // Start the connection (non-blocking)
    websocket_.start();

    return true;
}

void websocket_connection::disconnect()
{
    // Always stop the underlying socket. A server-initiated close sets our
    // state to disconnected without calling stop(), so we can't rely on
    // the state alone to decide whether cleanup is needed.
    spdlog::info("WebSocket disconnecting");
    websocket_.stop();
    state_.store(ws_connection_state::disconnected);

    // Clear message queue
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!incoming_messages_.empty())
    {
        incoming_messages_.pop();
    }
}

void websocket_connection::update(float delta_time)
{
    if (state_.load() != ws_connection_state::connected)
    {
        return;
    }

    ping_timer_ += delta_time;
    if (ping_timer_ >= ping_interval_ && !ping_pending_.load(std::memory_order_relaxed))
    {
        ping_sent_time_ = std::chrono::steady_clock::now();
        ping_pending_.store(true, std::memory_order_relaxed);
        websocket_.ping("");
        ping_timer_ = 0.0f;
    }
}

bool websocket_connection::send(const json& message)
{
    if (state_.load() != ws_connection_state::connected)
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        last_error_ = "Not connected";
        return false;
    }

    std::string msg_str = message.dump();
    spdlog::debug("WebSocket sending: {}", msg_str);

    auto result = websocket_.send(msg_str);
    if (result.success)
    {
        messages_sent_++;
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        last_error_ = "Failed to send message";
    }
    return false;
}

std::optional<json> websocket_connection::receive()
{
    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (incoming_messages_.empty())
    {
        return std::nullopt;
    }

    json msg = std::move(incoming_messages_.front());
    incoming_messages_.pop();
    return msg;
}

bool websocket_connection::poll_connect_event()
{
    return pending_connect_.exchange(false);
}

bool websocket_connection::poll_disconnect_event(std::string& out_reason)
{
    if (pending_disconnect_.exchange(false))
    {
        std::lock_guard<std::mutex> lock(disconnect_reason_mutex_);
        out_reason = std::move(pending_disconnect_reason_);
        pending_disconnect_reason_.clear();
        return true;
    }
    return false;
}

void websocket_connection::on_message(const ix::WebSocketMessagePtr& msg)
{
    // NOTE: This function runs on a background thread managed by ixwebsocket
    switch (msg->type)
    {
    case ix::WebSocketMessageType::Open:
        spdlog::info("WebSocket connected");
        state_.store(ws_connection_state::connected);
        ping_ms_.store(0, std::memory_order_relaxed);
        ping_pending_.store(false, std::memory_order_relaxed);
        ping_timer_ = 0.0f;
        // Set flag for thread-safe polling (preferred)
        pending_connect_.store(true);
        // Also call callback for backward compatibility (WARNING: runs on background thread!)
        if (connect_callback_)
        {
            connect_callback_();
        }
        break;

    case ix::WebSocketMessageType::Close:
        spdlog::info("WebSocket closed: {} ({})", msg->closeInfo.reason, msg->closeInfo.code);
        state_.store(ws_connection_state::disconnected);
        // Set flag for thread-safe polling (preferred)
        {
            std::lock_guard<std::mutex> lock(disconnect_reason_mutex_);
            pending_disconnect_reason_ = msg->closeInfo.reason;
        }
        pending_disconnect_.store(true);
        // Also call callback for backward compatibility (WARNING: runs on background thread!)
        if (disconnect_callback_)
        {
            disconnect_callback_(msg->closeInfo.reason);
        }
        break;

    case ix::WebSocketMessageType::Error:
        spdlog::error("WebSocket error: {}", msg->errorInfo.reason);
        {
            std::lock_guard<std::mutex> lock(error_mutex_);
            last_error_ = msg->errorInfo.reason;
        }
        state_.store(ws_connection_state::failed);
        break;

    case ix::WebSocketMessageType::Message:
        spdlog::trace("WebSocket received: {}", msg->str);
        try
        {
            json parsed = json::parse(msg->str);
            messages_received_++;

            // If callback is set, use it; otherwise queue the message
            if (message_callback_)
            {
                message_callback_(parsed);
            }
            else
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                incoming_messages_.push(std::move(parsed));
            }
        }
        catch (const json::parse_error& e)
        {
            spdlog::error("Failed to parse JSON message: {}", e.what());
        }
        break;

    case ix::WebSocketMessageType::Ping:
        spdlog::trace("WebSocket ping received");
        break;

    case ix::WebSocketMessageType::Pong:
        if (ping_pending_.load(std::memory_order_relaxed))
        {
            auto rtt = std::chrono::steady_clock::now() - ping_sent_time_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(rtt).count();
            ping_ms_.store(static_cast<int32_t>(ms), std::memory_order_relaxed);
            ping_pending_.store(false, std::memory_order_relaxed);
            spdlog::trace("WebSocket pong: {} ms", ms);
        }
        break;

    default:
        break;
    }
}

} // namespace hb
