#include "network/connection.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

connection::connection()
{
    recv_buffer_.resize(max_packet_size);
    socket_.setBlocking(false);
}

connection::~connection()
{
    disconnect();
}

bool connection::connect(std::string_view host, uint16_t port, uint32_t timeout_ms)
{
    if (state_ == connection_state::connected)
    {
        disconnect();
    }

    state_ = connection_state::connecting;
    last_error_.clear();

    // Temporarily set blocking for connect
    socket_.setBlocking(true);

    auto status =
        socket_.connect(sf::IpAddress::resolve(std::string(host)).value(), port, sf::milliseconds(timeout_ms));

    socket_.setBlocking(false);

    if (status != sf::Socket::Status::Done)
    {
        state_ = connection_state::failed;
        last_error_ = "Failed to connect to " + std::string(host) + ":" + std::to_string(port);
        spdlog::error("{}", last_error_);
        return false;
    }

    state_ = connection_state::connected;
    recv_pos_ = 0;
    expected_size_ = 0;
    reading_header_ = true;

    spdlog::info("Connected to {}:{}", host, port);
    return true;
}

void connection::disconnect()
{
    if (state_ != connection_state::disconnected)
    {
        socket_.disconnect();
        state_ = connection_state::disconnected;

        // Clear queues
        while (!incoming_packets_.empty())
            incoming_packets_.pop();
        while (!outgoing_packets_.empty())
            outgoing_packets_.pop();

        recv_pos_ = 0;
        expected_size_ = 0;
        reading_header_ = true;

        spdlog::info("Disconnected");
    }
}

bool connection::send(const packet& pkt)
{
    if (state_ != connection_state::connected)
    {
        return false;
    }

    size_t sent = 0;
    auto status = socket_.send(pkt.data(), pkt.size(), sent);

    if (status == sf::Socket::Status::Done)
    {
        bytes_sent_ += pkt.size();
        packets_sent_++;
        return true;
    }
    else if (status == sf::Socket::Status::Partial)
    {
        // Handle partial send - in practice, queue the remainder
        bytes_sent_ += sent;
        spdlog::warn("Partial send: {} of {} bytes", sent, pkt.size());
        return true;
    }
    else if (status == sf::Socket::Status::Disconnected)
    {
        state_ = connection_state::disconnected;
        last_error_ = "Connection lost during send";
        spdlog::error("{}", last_error_);
        return false;
    }

    return false;
}

std::optional<packet> connection::receive()
{
    if (incoming_packets_.empty())
    {
        return std::nullopt;
    }

    packet pkt = std::move(incoming_packets_.front());
    incoming_packets_.pop();
    return pkt;
}

void connection::update()
{
    if (state_ != connection_state::connected)
    {
        return;
    }

    process_incoming();
}

void connection::process_incoming()
{
    // Try to read data
    size_t received = 0;

    while (true)
    {
        size_t to_read;
        uint8_t* buffer;

        if (reading_header_)
        {
            // Reading the 2-byte size header
            to_read = packet_header_size - recv_pos_;
            buffer = recv_buffer_.data() + recv_pos_;
        }
        else
        {
            // Reading the packet body
            to_read = expected_size_ - recv_pos_;
            buffer = recv_buffer_.data() + recv_pos_;
        }

        auto status = socket_.receive(buffer, to_read, received);

        if (status == sf::Socket::Status::Done || status == sf::Socket::Status::Partial)
        {
            recv_pos_ += received;
            bytes_received_ += received;

            if (reading_header_ && recv_pos_ >= packet_header_size)
            {
                // Got the header, extract size
                expected_size_ = static_cast<uint16_t>(recv_buffer_[1]) | (static_cast<uint16_t>(recv_buffer_[2]) << 8);

                if (expected_size_ > max_packet_size)
                {
                    spdlog::error("Packet too large: {} bytes", expected_size_);
                    disconnect();
                    return;
                }

                if (expected_size_ < packet_header_size)
                {
                    spdlog::error("Packet too small: {} bytes", expected_size_);
                    disconnect();
                    return;
                }

                reading_header_ = false;
                // recv_pos_ stays the same, we continue reading into the same buffer
            }

            if (!reading_header_ && recv_pos_ >= expected_size_)
            {
                // Complete packet received
                packet pkt(recv_buffer_.data(), expected_size_);
                incoming_packets_.push(std::move(pkt));
                packets_received_++;

                // Reset for next packet
                recv_pos_ = 0;
                expected_size_ = 0;
                reading_header_ = true;
            }

            if (status == sf::Socket::Status::Partial || received == 0)
            {
                break;
            }
        }
        else if (status == sf::Socket::Status::NotReady)
        {
            // No data available
            break;
        }
        else if (status == sf::Socket::Status::Disconnected)
        {
            state_ = connection_state::disconnected;
            last_error_ = "Connection lost";
            spdlog::warn("{}", last_error_);
            break;
        }
        else
        {
            // Error
            spdlog::error("Socket receive error");
            break;
        }
    }
}

} // namespace hb
