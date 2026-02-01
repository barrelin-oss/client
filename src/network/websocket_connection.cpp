#include "network/websocket_connection.hpp"
#include <spdlog/spdlog.h>

namespace hb {

websocket_connection::websocket_connection() {
    // Disable automatic reconnection - we'll create new connections as needed
    // based on game state (only reconnect during character select or in-game)
    websocket_.disableAutomaticReconnection();

    // Configure WebSocket
    websocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        on_message(msg);
    });
}

websocket_connection::~websocket_connection() {
    disconnect();
}

bool websocket_connection::connect(std::string_view url) {
    if (state_ == ws_connection_state::connected || state_ == ws_connection_state::connecting) {
        disconnect();
    }

    state_ = ws_connection_state::connecting;
    last_error_.clear();

    std::string url_str(url);
    spdlog::info("WebSocket connecting to: {}", url_str);

    websocket_.setUrl(url_str);

    // Start the connection (non-blocking)
    websocket_.start();

    return true;
}

void websocket_connection::disconnect() {
    if (state_ != ws_connection_state::disconnected) {
        spdlog::info("WebSocket disconnecting");
        websocket_.stop();
        state_ = ws_connection_state::disconnected;

        // Clear message queue
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!incoming_messages_.empty()) {
            incoming_messages_.pop();
        }
    }
}

bool websocket_connection::send(const json& message) {
    if (state_ != ws_connection_state::connected) {
        last_error_ = "Not connected";
        return false;
    }

    std::string msg_str = message.dump();
    spdlog::debug("WebSocket sending: {}", msg_str);

    auto result = websocket_.send(msg_str);
    if (result.success) {
        messages_sent_++;
        return true;
    }

    last_error_ = "Failed to send message";
    return false;
}

std::optional<json> websocket_connection::receive() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (incoming_messages_.empty()) {
        return std::nullopt;
    }

    json msg = std::move(incoming_messages_.front());
    incoming_messages_.pop();
    return msg;
}

void websocket_connection::on_message(const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Open:
            spdlog::info("WebSocket connected");
            state_ = ws_connection_state::connected;
            if (connect_callback_) {
                connect_callback_();
            }
            break;

        case ix::WebSocketMessageType::Close:
            spdlog::info("WebSocket closed: {} ({})", msg->closeInfo.reason, msg->closeInfo.code);
            state_ = ws_connection_state::disconnected;
            if (disconnect_callback_) {
                disconnect_callback_(msg->closeInfo.reason);
            }
            break;

        case ix::WebSocketMessageType::Error:
            spdlog::error("WebSocket error: {}", msg->errorInfo.reason);
            last_error_ = msg->errorInfo.reason;
            state_ = ws_connection_state::failed;
            break;

        case ix::WebSocketMessageType::Message:
            spdlog::debug("WebSocket received: {}", msg->str);
            try {
                json parsed = json::parse(msg->str);
                messages_received_++;

                // If callback is set, use it; otherwise queue the message
                if (message_callback_) {
                    message_callback_(parsed);
                } else {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    incoming_messages_.push(std::move(parsed));
                }
            } catch (const json::parse_error& e) {
                spdlog::error("Failed to parse JSON message: {}", e.what());
            }
            break;

        case ix::WebSocketMessageType::Ping:
            spdlog::trace("WebSocket ping received");
            break;

        case ix::WebSocketMessageType::Pong:
            spdlog::trace("WebSocket pong received");
            break;

        default:
            break;
    }
}

} // namespace hb
