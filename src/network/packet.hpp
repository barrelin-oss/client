#pragma once

#include "network/protocol.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <optional>

namespace hb {

// Raw packet data
// Helbreath packet format:
//   Byte 0:     Encryption key (0 = no encryption)
//   Bytes 1-2:  Packet size (little-endian, includes 3-byte header)
//   Bytes 3+:   Payload (encrypted if key != 0)
class packet {
public:
    packet() = default;
    explicit packet(uint32_t message_id);  // 32-bit message ID
    packet(const uint8_t* data, size_t size);

    // Packet info
    uint32_t message_id() const;  // 32-bit message ID from payload
    size_t size() const { return data_.size(); }
    size_t payload_size() const { return data_.size() > packet_header_size ? data_.size() - packet_header_size : 0; }
    bool empty() const { return data_.empty(); }
    bool valid() const { return data_.size() >= packet_header_size; }

    // Encryption key
    uint8_t key() const { return data_.empty() ? 0 : data_[0]; }
    void set_key(uint8_t key) { if (!data_.empty()) data_[0] = key; }

    // Raw access
    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }
    std::span<const uint8_t> span() const { return data_; }
    std::span<const uint8_t> payload() const;
    std::span<uint8_t> payload_mutable();

    // Reserve space
    void reserve(size_t size) { data_.reserve(size); }

    // For building packets
    void write_u8(uint8_t value);
    void write_u16(uint16_t value);
    void write_u32(uint32_t value);
    void write_i8(int8_t value);
    void write_i16(int16_t value);
    void write_i32(int32_t value);
    void write_string(std::string_view str);
    void write_bytes(std::span<const uint8_t> bytes);

    // Finalize packet (updates size header)
    void finalize();

    // Encryption/decryption (XOR cipher from original Helbreath)
    // Note: These operate on the payload only, not the header
    void encrypt(uint8_t key);
    void decrypt(uint8_t key);

    // Check if payload is encrypted
    bool is_encrypted() const { return key() != 0; }

private:
    std::vector<uint8_t> data_;
};

// Packet reader for parsing incoming packets
class packet_reader {
public:
    explicit packet_reader(const packet& pkt);
    packet_reader(const uint8_t* data, size_t size);

    // Check remaining bytes
    size_t remaining() const { return size_ - pos_; }
    bool has_bytes(size_t count) const { return remaining() >= count; }
    bool at_end() const { return pos_ >= size_; }

    // Read values (returns nullopt if not enough data)
    std::optional<uint8_t> read_u8();
    std::optional<uint16_t> read_u16();
    std::optional<uint32_t> read_u32();
    std::optional<int8_t> read_i8();
    std::optional<int16_t> read_i16();
    std::optional<int32_t> read_i32();
    std::optional<std::string> read_string(size_t max_length = 256);
    std::optional<std::vector<uint8_t>> read_bytes(size_t count);

    // Skip bytes
    bool skip(size_t count);

    // Get message ID (32-bit, from start of payload)
    uint32_t message_id() const;

    // Get encryption key from header
    uint8_t key() const;

    // Get packet size from header
    uint16_t packet_size() const;

    // Reset position to start of payload (after header)
    void reset() { pos_ = packet_header_size; }

    // Reset position to after message ID (for reading message data)
    void reset_after_message_id() { pos_ = packet_header_size + 4; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

// Convenience functions for creating common packets
namespace packets {

// Login packets
packet make_login_request(std::string_view account, std::string_view password, uint32_t version);
packet make_create_account(std::string_view account, std::string_view password, std::string_view email);
packet make_character_list_request();
packet make_create_character(std::string_view name, uint8_t gender, uint8_t skin_color,
                             uint8_t hair_style, uint8_t hair_color, uint8_t underwear_color,
                             uint8_t str, uint8_t vit, uint8_t dex, uint8_t intel, uint8_t mag, uint8_t cha);
packet make_delete_character(std::string_view name, std::string_view password);
packet make_enter_game(std::string_view character_name);

// Game packets
packet make_move(int32_t x, int32_t y, uint8_t direction);
packet make_attack(uint32_t target_id, uint8_t attack_type);
packet make_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id = 0);
packet make_use_skill(uint16_t skill_id);
packet make_use_item(uint8_t slot);
packet make_drop_item(uint8_t slot, uint32_t amount);
packet make_pickup_item(uint32_t item_id);
packet make_chat(std::string_view message);
packet make_whisper(std::string_view target, std::string_view message);
packet make_ping();

// Stat allocation
packet make_add_stat(int stat_index);

// Inventory/Equipment
packet make_move_item(int32_t from_slot, int32_t to_slot);
packet make_unequip(uint8_t slot);

// Chat with mode
packet make_chat_with_mode(std::string_view message, int mode);

// Shop/Trade
packet make_buy(uint16_t item_id, int32_t quantity);
packet make_sell(uint16_t item_id, int32_t quantity);

// Bank
packet make_bank_deposit(int32_t slot);
packet make_bank_withdraw(int32_t slot);

// Party
packet make_party_invite(std::string_view name);
packet make_party_leave();
packet make_party_kick(std::string_view name);

// Guild
packet make_guild_invite(std::string_view name);
packet make_guild_leave();

// NPC
packet make_npc_response(int32_t response_id);

// Trade
packet make_trade_invite(std::string_view player_name);
packet make_trade_add_item(int32_t inventory_slot, int32_t trade_slot);
packet make_trade_remove_item(int32_t trade_slot);
packet make_trade_set_gold(uint32_t amount);
packet make_trade_confirm();
packet make_trade_cancel();

// Crafting/Manufacturing
packet make_craft_item(int32_t recipe_index);
packet make_add_craft_material(int32_t inventory_slot, int32_t material_slot);

// Repair
packet make_repair_item(int32_t inventory_slot);
packet make_repair_all();

} // namespace packets

} // namespace hb
