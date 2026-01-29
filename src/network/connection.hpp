#pragma once

#include "network/packet.hpp"
#include <SFML/Network.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <queue>
#include <mutex>
#include <optional>
#include <functional>

namespace hb {

enum class connection_state {
    disconnected,
    connecting,
    connected,
    failed
};

class connection {
public:
    connection();
    ~connection();

    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    // Connect to server
    bool connect(std::string_view host, uint16_t port, uint32_t timeout_ms = 5000);
    void disconnect();

    // State
    connection_state state() const { return state_; }
    bool is_connected() const { return state_ == connection_state::connected; }

    // Send packet
    bool send(const packet& pkt);

    // Receive packets (call this in update loop)
    // Returns the next available packet, or nullopt if none
    std::optional<packet> receive();

    // Update - processes socket I/O
    void update();

    // Get last error message
    std::string_view last_error() const { return last_error_; }

    // Statistics
    uint64_t bytes_sent() const { return bytes_sent_; }
    uint64_t bytes_received() const { return bytes_received_; }
    uint64_t packets_sent() const { return packets_sent_; }
    uint64_t packets_received() const { return packets_received_; }

private:
    void process_incoming();
    bool read_packet_header();
    bool read_packet_body();

    sf::TcpSocket socket_;
    connection_state state_ = connection_state::disconnected;
    std::string last_error_;

    // Receive buffer
    std::vector<uint8_t> recv_buffer_;
    size_t recv_pos_ = 0;
    size_t expected_size_ = 0;
    bool reading_header_ = true;

    // Packet queues
    std::queue<packet> incoming_packets_;
    std::queue<packet> outgoing_packets_;

    // Statistics
    uint64_t bytes_sent_ = 0;
    uint64_t bytes_received_ = 0;
    uint64_t packets_sent_ = 0;
    uint64_t packets_received_ = 0;
};

} // namespace hb
