#include "network/packet.hpp"
#include <cstring>
#include <algorithm>

namespace hb {

packet::packet(uint32_t message_id) {
    data_.reserve(64);
    // Header: key (1 byte) + size (2 bytes) = 3 bytes
    data_.push_back(0);  // Key = 0 (no encryption by default)
    data_.push_back(0);  // Size low byte (set in finalize)
    data_.push_back(0);  // Size high byte (set in finalize)
    // Write 32-bit message ID as first part of payload
    write_u32(message_id);
}

packet::packet(const uint8_t* data, size_t size) {
    data_.assign(data, data + size);
}

uint32_t packet::message_id() const {
    // Message ID is first 4 bytes of payload (after 3-byte header)
    if (data_.size() < packet_header_size + 4) {
        return 0;
    }
    return static_cast<uint32_t>(data_[3]) |
           (static_cast<uint32_t>(data_[4]) << 8) |
           (static_cast<uint32_t>(data_[5]) << 16) |
           (static_cast<uint32_t>(data_[6]) << 24);
}

std::span<const uint8_t> packet::payload() const {
    if (data_.size() <= packet_header_size) {
        return {};
    }
    return std::span<const uint8_t>(data_.data() + packet_header_size,
                                     data_.size() - packet_header_size);
}

std::span<uint8_t> packet::payload_mutable() {
    if (data_.size() <= packet_header_size) {
        return {};
    }
    return std::span<uint8_t>(data_.data() + packet_header_size,
                               data_.size() - packet_header_size);
}

void packet::write_u8(uint8_t value) {
    data_.push_back(value);
}

void packet::write_u16(uint16_t value) {
    data_.push_back(static_cast<uint8_t>(value & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void packet::write_u32(uint32_t value) {
    data_.push_back(static_cast<uint8_t>(value & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void packet::write_i8(int8_t value) {
    write_u8(static_cast<uint8_t>(value));
}

void packet::write_i16(int16_t value) {
    write_u16(static_cast<uint16_t>(value));
}

void packet::write_i32(int32_t value) {
    write_u32(static_cast<uint32_t>(value));
}

void packet::write_string(std::string_view str) {
    // Write null-terminated string
    for (char c : str) {
        data_.push_back(static_cast<uint8_t>(c));
    }
    data_.push_back(0);
}

void packet::write_bytes(std::span<const uint8_t> bytes) {
    data_.insert(data_.end(), bytes.begin(), bytes.end());
}

void packet::finalize() {
    if (data_.size() >= packet_header_size) {
        // Size is stored in bytes 1-2 (little-endian)
        // Includes the 3-byte header
        uint16_t size = static_cast<uint16_t>(data_.size());
        data_[1] = static_cast<uint8_t>(size & 0xFF);
        data_[2] = static_cast<uint8_t>((size >> 8) & 0xFF);
    }
}

void packet::encrypt(uint8_t key) {
    if (key == 0 || data_.size() <= packet_header_size) {
        return;
    }

    // Set the key in header
    data_[0] = key;

    // Encrypt payload using Helbreath XOR cipher
    // Original algorithm from XSocket.cpp:486-490:
    //   for (i = 0; i < dwSize; i++) {
    //       buffer[3+i] += (i ^ key);
    //       buffer[3+i] = buffer[3+i] ^ (key ^ (dwSize - i));
    //   }
    size_t payload_len = data_.size() - packet_header_size;
    for (size_t i = 0; i < payload_len; ++i) {
        data_[packet_header_size + i] += static_cast<uint8_t>(i ^ key);
        data_[packet_header_size + i] ^= static_cast<uint8_t>(key ^ (payload_len - i));
    }
}

void packet::decrypt(uint8_t key) {
    if (key == 0 || data_.size() <= packet_header_size) {
        return;
    }

    // Decrypt payload using Helbreath XOR cipher
    // Original algorithm from XSocket.cpp:604-608:
    //   for (i = 0; i < dwSize; i++) {
    //       buffer[3+i] = buffer[3+i] ^ (key ^ (dwSize - i));
    //       buffer[3+i] -= (i ^ key);
    //   }
    size_t payload_len = data_.size() - packet_header_size;
    for (size_t i = 0; i < payload_len; ++i) {
        data_[packet_header_size + i] ^= static_cast<uint8_t>(key ^ (payload_len - i));
        data_[packet_header_size + i] -= static_cast<uint8_t>(i ^ key);
    }

    // Clear the key after decryption
    data_[0] = 0;
}

// packet_reader implementation

packet_reader::packet_reader(const packet& pkt)
    : data_(pkt.data()), size_(pkt.size()), pos_(packet_header_size) {}

packet_reader::packet_reader(const uint8_t* data, size_t size)
    : data_(data), size_(size), pos_(packet_header_size) {}

uint32_t packet_reader::message_id() const {
    // Message ID is first 4 bytes of payload (after 3-byte header)
    if (size_ < packet_header_size + 4) {
        return 0;
    }
    return static_cast<uint32_t>(data_[3]) |
           (static_cast<uint32_t>(data_[4]) << 8) |
           (static_cast<uint32_t>(data_[5]) << 16) |
           (static_cast<uint32_t>(data_[6]) << 24);
}

uint8_t packet_reader::key() const {
    if (size_ < 1) {
        return 0;
    }
    return data_[0];
}

uint16_t packet_reader::packet_size() const {
    if (size_ < packet_header_size) {
        return 0;
    }
    return static_cast<uint16_t>(data_[1]) | (static_cast<uint16_t>(data_[2]) << 8);
}

std::optional<uint8_t> packet_reader::read_u8() {
    if (!has_bytes(1)) return std::nullopt;
    return data_[pos_++];
}

std::optional<uint16_t> packet_reader::read_u16() {
    if (!has_bytes(2)) return std::nullopt;
    uint16_t value = static_cast<uint16_t>(data_[pos_]) |
                     (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
    pos_ += 2;
    return value;
}

std::optional<uint32_t> packet_reader::read_u32() {
    if (!has_bytes(4)) return std::nullopt;
    uint32_t value = static_cast<uint32_t>(data_[pos_]) |
                     (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                     (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                     (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
    pos_ += 4;
    return value;
}

std::optional<int8_t> packet_reader::read_i8() {
    auto v = read_u8();
    if (!v) return std::nullopt;
    return static_cast<int8_t>(*v);
}

std::optional<int16_t> packet_reader::read_i16() {
    auto v = read_u16();
    if (!v) return std::nullopt;
    return static_cast<int16_t>(*v);
}

std::optional<int32_t> packet_reader::read_i32() {
    auto v = read_u32();
    if (!v) return std::nullopt;
    return static_cast<int32_t>(*v);
}

std::optional<std::string> packet_reader::read_string(size_t max_length) {
    std::string result;
    result.reserve(32);

    while (pos_ < size_ && result.size() < max_length) {
        uint8_t c = data_[pos_++];
        if (c == 0) {
            return result;
        }
        result.push_back(static_cast<char>(c));
    }

    // No null terminator found within max_length
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> packet_reader::read_bytes(size_t count) {
    if (!has_bytes(count)) return std::nullopt;
    std::vector<uint8_t> result(data_ + pos_, data_ + pos_ + count);
    pos_ += count;
    return result;
}

bool packet_reader::skip(size_t count) {
    if (!has_bytes(count)) return false;
    pos_ += count;
    return true;
}

// Packet factory functions

namespace packets {

packet make_login_request(std::string_view account, std::string_view password, uint32_t version) {
    packet pkt(msg_request_login);
    pkt.write_u32(version);
    pkt.write_string(account);
    pkt.write_string(password);
    pkt.finalize();
    return pkt;
}

packet make_create_account(std::string_view account, std::string_view password, std::string_view email) {
    packet pkt(msg_request_create_account);
    pkt.write_string(account);
    pkt.write_string(password);
    pkt.write_string(email);
    pkt.finalize();
    return pkt;
}

packet make_character_list_request() {
    // Character list is obtained from login response
    packet pkt(msg_request_login);
    pkt.finalize();
    return pkt;
}

packet make_create_character(std::string_view name, uint8_t gender, uint8_t skin_color,
                             uint8_t hair_style, uint8_t hair_color, uint8_t underwear_color,
                             uint8_t str, uint8_t vit, uint8_t dex, uint8_t intel, uint8_t mag, uint8_t cha) {
    packet pkt(msg_request_create_character);
    pkt.write_string(name);
    pkt.write_u8(gender);
    pkt.write_u8(skin_color);
    pkt.write_u8(hair_style);
    pkt.write_u8(hair_color);
    pkt.write_u8(underwear_color);
    pkt.write_u8(str);
    pkt.write_u8(vit);
    pkt.write_u8(dex);
    pkt.write_u8(intel);
    pkt.write_u8(mag);
    pkt.write_u8(cha);
    pkt.finalize();
    return pkt;
}

packet make_delete_character(std::string_view name, std::string_view password) {
    packet pkt(msg_request_delete_character);
    pkt.write_string(name);
    pkt.write_string(password);
    pkt.finalize();
    return pkt;
}

packet make_enter_game(std::string_view character_name) {
    packet pkt(msg_request_enter_game);
    pkt.write_string(character_name);
    pkt.finalize();
    return pkt;
}

packet make_move(int32_t x, int32_t y, uint8_t direction) {
    // Movement uses msg_command_motion
    packet pkt(msg_command_motion);
    pkt.write_i32(x);
    pkt.write_i32(y);
    pkt.write_u8(direction);
    pkt.finalize();
    return pkt;
}

packet make_attack(uint32_t target_id, uint8_t attack_type) {
    // Attack is a common command
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::toggle_combat_mode);
    pkt.write_u32(target_id);
    pkt.write_u8(attack_type);
    pkt.finalize();
    return pkt;
}

packet make_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::magic);
    pkt.write_u16(spell_id);
    pkt.write_i32(target_x);
    pkt.write_i32(target_y);
    pkt.write_u32(target_id);
    pkt.finalize();
    return pkt;
}

packet make_use_skill(uint16_t skill_id) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_use_skill);
    pkt.write_u16(skill_id);
    pkt.finalize();
    return pkt;
}

packet make_use_item(uint8_t slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_use_item);
    pkt.write_u8(slot);
    pkt.finalize();
    return pkt;
}

packet make_drop_item(uint8_t slot, uint32_t amount) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::item_drop);
    pkt.write_u8(slot);
    pkt.write_u32(amount);
    pkt.finalize();
    return pkt;
}

packet make_pickup_item(uint32_t item_id) {
    // Pickup is handled via motion/interaction with ground items
    packet pkt(msg_command_motion);
    pkt.write_u32(item_id);
    pkt.finalize();
    return pkt;
}

packet make_chat(std::string_view message) {
    packet pkt(msg_chat);
    pkt.write_string(message);
    pkt.finalize();
    return pkt;
}

packet make_whisper(std::string_view target, std::string_view message) {
    // Whispers are formatted as "/w target message" in the chat packet
    std::string whisper_msg = "/w ";
    whisper_msg += target;
    whisper_msg += " ";
    whisper_msg += message;
    packet pkt(msg_chat);
    pkt.write_string(whisper_msg);
    pkt.finalize();
    return pkt;
}

packet make_ping() {
    packet pkt(msg_check_connection);
    pkt.finalize();
    return pkt;
}

// Stat allocation
packet make_add_stat(int stat_index) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_set_down_skill_index);  // Reused for stat allocation
    pkt.write_u8(static_cast<uint8_t>(stat_index));
    pkt.finalize();
    return pkt;
}

// Inventory/Equipment
packet make_move_item(int32_t from_slot, int32_t to_slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::set_item);
    pkt.write_u8(static_cast<uint8_t>(from_slot));
    pkt.write_u8(static_cast<uint8_t>(to_slot));
    pkt.finalize();
    return pkt;
}

packet make_unequip(uint8_t slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::release_item);
    pkt.write_u8(slot);
    pkt.finalize();
    return pkt;
}

// Chat with mode
packet make_chat_with_mode(std::string_view message, int mode) {
    // Mode 0 = normal, 1 = shout, 2 = party, 3 = guild
    std::string formatted;
    switch (mode) {
        case 1:  // shout
            formatted = "!";
            break;
        case 2:  // party
            formatted = "~";
            break;
        case 3:  // guild
            formatted = "@";
            break;
        default:
            break;
    }
    formatted += message;

    packet pkt(msg_chat);
    pkt.write_string(formatted);
    pkt.finalize();
    return pkt;
}

// Shop/Trade
packet make_buy(uint16_t item_id, int32_t quantity) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_purchase_item);
    pkt.write_u16(item_id);
    pkt.write_i32(quantity);
    pkt.finalize();
    return pkt;
}

packet make_sell(uint16_t item_id, int32_t quantity) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_sell_item_confirm);
    pkt.write_u16(item_id);
    pkt.write_i32(quantity);
    pkt.finalize();
    return pkt;
}

// Bank
packet make_bank_deposit(int32_t slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::set_item);  // Bank uses same item command with different context
    pkt.write_u8(static_cast<uint8_t>(slot));
    pkt.write_u8(0xFF);  // Special marker for bank deposit
    pkt.finalize();
    return pkt;
}

packet make_bank_withdraw(int32_t slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::set_item);
    pkt.write_u8(0xFF);  // Special marker for bank withdraw
    pkt.write_u8(static_cast<uint8_t>(slot));
    pkt.finalize();
    return pkt;
}

// Party
packet make_party_invite(std::string_view name) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::request_join_party);
    pkt.write_string(name);
    pkt.finalize();
    return pkt;
}

packet make_party_leave() {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::request_join_party);
    pkt.write_u8(0);  // 0 = leave
    pkt.finalize();
    return pkt;
}

packet make_party_kick(std::string_view name) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::request_join_party);
    pkt.write_u8(2);  // 2 = kick
    pkt.write_string(name);
    pkt.finalize();
    return pkt;
}

// Guild
packet make_guild_invite(std::string_view name) {
    packet pkt(msg_request_guild_add_member);
    pkt.write_string(name);
    pkt.finalize();
    return pkt;
}

packet make_guild_leave() {
    packet pkt(msg_request_guild_del_member);
    pkt.write_u8(1);  // Leave flag
    pkt.finalize();
    return pkt;
}

// NPC
packet make_npc_response(int32_t response_id) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::talk_to_npc);
    pkt.write_i32(response_id);
    pkt.finalize();
    return pkt;
}

// Trade
packet make_trade_invite(std::string_view player_name) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::request_exchange);
    pkt.write_string(player_name);
    pkt.finalize();
    return pkt;
}

packet make_trade_add_item(int32_t inventory_slot, int32_t trade_slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::confirm_exchange_item);
    pkt.write_u8(0);  // Action: add
    pkt.write_u8(static_cast<uint8_t>(inventory_slot));
    pkt.write_u8(static_cast<uint8_t>(trade_slot));
    pkt.finalize();
    return pkt;
}

packet make_trade_remove_item(int32_t trade_slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::confirm_exchange_item);
    pkt.write_u8(1);  // Action: remove
    pkt.write_u8(static_cast<uint8_t>(trade_slot));
    pkt.finalize();
    return pkt;
}

packet make_trade_set_gold(uint32_t amount) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::confirm_exchange_item);
    pkt.write_u8(2);  // Action: set gold
    pkt.write_u32(amount);
    pkt.finalize();
    return pkt;
}

packet make_trade_confirm() {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::confirm_exchange_item);
    pkt.write_u8(3);  // Action: confirm
    pkt.finalize();
    return pkt;
}

packet make_trade_cancel() {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::cancel_exchange_item);
    pkt.finalize();
    return pkt;
}

// Crafting/Manufacturing
packet make_craft_item(int32_t recipe_index) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::manu_item);
    pkt.write_i32(recipe_index);
    pkt.finalize();
    return pkt;
}

packet make_add_craft_material(int32_t inventory_slot, int32_t material_slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_set_manu_item);
    pkt.write_u8(static_cast<uint8_t>(inventory_slot));
    pkt.write_u8(static_cast<uint8_t>(material_slot));
    pkt.finalize();
    return pkt;
}

// Repair
packet make_repair_item(int32_t inventory_slot) {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_repair_item);
    pkt.write_u8(static_cast<uint8_t>(inventory_slot));
    pkt.finalize();
    return pkt;
}

packet make_repair_all() {
    packet pkt(msg_command_common);
    pkt.write_u16(common_type::req_repair_all);
    pkt.finalize();
    return pkt;
}

} // namespace packets

} // namespace hb
