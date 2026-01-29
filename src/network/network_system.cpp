#include "network/network_system.hpp"
#include <spdlog/spdlog.h>

namespace hb {

bool network_system::initialize() {
    spdlog::info("Network system initialized");
    return true;
}

void network_system::shutdown() {
    disconnect();
    handlers_.clear();
    spdlog::info("Network system shutdown");
}

void network_system::update() {
    login_connection_.update();
    game_connection_.update();

    process_login_packets();
    process_game_packets();
}

bool network_system::connect_login_server(std::string_view host, uint16_t port) {
    spdlog::info("Connecting to login server {}:{}", host, port);
    return login_connection_.connect(host, port);
}

bool network_system::connect_game_server(std::string_view host, uint16_t port) {
    spdlog::info("Connecting to game server {}:{}", host, port);
    return game_connection_.connect(host, port);
}

void network_system::disconnect() {
    login_connection_.disconnect();
    game_connection_.disconnect();
}

bool network_system::send_login_packet(const packet& pkt) {
    return login_connection_.send(pkt);
}

bool network_system::send_game_packet(const packet& pkt) {
    return game_connection_.send(pkt);
}

void network_system::register_handler(uint32_t message_id, packet_handler handler) {
    handlers_[message_id] = std::move(handler);
}

void network_system::process_login_packets() {
    while (auto pkt = login_connection_.receive()) {
        // Decrypt if needed
        if (pkt->key() != 0) {
            pkt->decrypt(pkt->key());
        }
        packet_reader reader(*pkt);
        dispatch_packet(reader);
    }
}

void network_system::process_game_packets() {
    while (auto pkt = game_connection_.receive()) {
        // Decrypt if needed
        if (pkt->key() != 0) {
            pkt->decrypt(pkt->key());
        }
        packet_reader reader(*pkt);
        dispatch_packet(reader);
    }
}

void network_system::dispatch_packet(packet_reader& reader) {
    uint32_t msg_id = reader.message_id();

    auto it = handlers_.find(msg_id);
    if (it != handlers_.end()) {
        reader.reset_after_message_id();  // Skip past the message ID for handler
        it->second(reader);
    } else {
        spdlog::debug("Unhandled message ID: 0x{:08X}", msg_id);
    }
}

// Login flow convenience methods

bool network_system::request_login(std::string_view account, std::string_view password) {
    if (!login_connection_.is_connected()) {
        spdlog::error("Not connected to login server");
        return false;
    }

    current_account_ = account;
    current_password_ = password;

    auto pkt = packets::make_login_request(account, password, protocol_version);
    return send_login_packet(pkt);
}

bool network_system::request_create_account(std::string_view account, std::string_view password, std::string_view email) {
    if (!login_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_create_account(account, password, email);
    return send_login_packet(pkt);
}

bool network_system::request_character_list() {
    if (!login_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_character_list_request();
    return send_login_packet(pkt);
}

bool network_system::request_create_character(std::string_view name, uint8_t gender, uint8_t skin_color,
                                              uint8_t hair_style, uint8_t hair_color, uint8_t underwear_color,
                                              uint8_t str, uint8_t vit, uint8_t dex, uint8_t intel, uint8_t mag, uint8_t cha) {
    if (!login_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_create_character(name, gender, skin_color, hair_style, hair_color, underwear_color,
                                              str, vit, dex, intel, mag, cha);
    return send_login_packet(pkt);
}

bool network_system::request_delete_character(std::string_view name, std::string_view password) {
    if (!login_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_delete_character(name, password);
    return send_login_packet(pkt);
}

bool network_system::request_enter_game(std::string_view character_name) {
    if (!login_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_enter_game(character_name);
    return send_login_packet(pkt);
}

// Game action convenience methods

bool network_system::request_move(int32_t x, int32_t y, uint8_t direction) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_move(x, y, direction);
    return send_game_packet(pkt);
}

bool network_system::request_attack(uint32_t target_id, uint8_t attack_type) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_attack(target_id, attack_type);
    return send_game_packet(pkt);
}

bool network_system::request_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_magic(spell_id, target_x, target_y, target_id);
    return send_game_packet(pkt);
}

bool network_system::request_use_skill(uint16_t skill_id) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_use_skill(skill_id);
    return send_game_packet(pkt);
}

bool network_system::request_use_item(uint8_t slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_use_item(slot);
    return send_game_packet(pkt);
}

bool network_system::request_drop_item(uint8_t slot, uint32_t amount) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_drop_item(slot, amount);
    return send_game_packet(pkt);
}

bool network_system::request_pickup_item(uint32_t item_id) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_pickup_item(item_id);
    return send_game_packet(pkt);
}

bool network_system::request_chat(std::string_view message) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_chat(message);
    return send_game_packet(pkt);
}

bool network_system::request_whisper(std::string_view target, std::string_view message) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_whisper(target, message);
    return send_game_packet(pkt);
}

uint64_t network_system::total_bytes_sent() const {
    return login_connection_.bytes_sent() + game_connection_.bytes_sent();
}

uint64_t network_system::total_bytes_received() const {
    return login_connection_.bytes_received() + game_connection_.bytes_received();
}

// Stat allocation
bool network_system::request_add_stat(int stat_index) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_add_stat(stat_index);
    return send_game_packet(pkt);
}

// Inventory/Equipment
bool network_system::request_move_item(int32_t from_slot, int32_t to_slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_move_item(from_slot, to_slot);
    return send_game_packet(pkt);
}

bool network_system::request_unequip(uint8_t slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_unequip(slot);
    return send_game_packet(pkt);
}

// Chat with mode
bool network_system::send_chat(std::string_view message, int mode) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_chat_with_mode(message, mode);
    return send_game_packet(pkt);
}

// Shop/Trade
bool network_system::request_buy(uint16_t item_id, int32_t quantity) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_buy(item_id, quantity);
    return send_game_packet(pkt);
}

bool network_system::request_sell(uint16_t item_id, int32_t quantity) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_sell(item_id, quantity);
    return send_game_packet(pkt);
}

// Bank
bool network_system::request_bank_deposit(int32_t slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_bank_deposit(slot);
    return send_game_packet(pkt);
}

bool network_system::request_bank_withdraw(int32_t slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_bank_withdraw(slot);
    return send_game_packet(pkt);
}

// Party
bool network_system::request_party_invite(std::string_view name) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_party_invite(name);
    return send_game_packet(pkt);
}

bool network_system::request_party_leave() {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_party_leave();
    return send_game_packet(pkt);
}

bool network_system::request_party_kick(std::string_view name) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_party_kick(name);
    return send_game_packet(pkt);
}

// Guild
bool network_system::request_guild_invite(std::string_view name) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_guild_invite(name);
    return send_game_packet(pkt);
}

bool network_system::request_guild_leave() {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_guild_leave();
    return send_game_packet(pkt);
}

// NPC
bool network_system::send_npc_response(int32_t response_id) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_npc_response(response_id);
    return send_game_packet(pkt);
}

// Trade
bool network_system::request_trade_invite(std::string_view player_name) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_trade_invite(player_name);
    return send_game_packet(pkt);
}

bool network_system::request_trade_add_item(int32_t inventory_slot, int32_t trade_slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_trade_add_item(inventory_slot, trade_slot);
    return send_game_packet(pkt);
}

bool network_system::request_trade_remove_item(int32_t trade_slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_trade_remove_item(trade_slot);
    return send_game_packet(pkt);
}

bool network_system::request_trade_set_gold(uint32_t amount) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_trade_set_gold(amount);
    return send_game_packet(pkt);
}

bool network_system::request_trade_confirm() {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_trade_confirm();
    return send_game_packet(pkt);
}

bool network_system::request_trade_cancel() {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_trade_cancel();
    return send_game_packet(pkt);
}

// Crafting/Manufacturing
bool network_system::request_craft_item(int32_t recipe_index) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_craft_item(recipe_index);
    return send_game_packet(pkt);
}

bool network_system::request_add_craft_material(int32_t inventory_slot, int32_t material_slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_add_craft_material(inventory_slot, material_slot);
    return send_game_packet(pkt);
}

// Repair
bool network_system::request_repair_item(int32_t inventory_slot) {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_repair_item(inventory_slot);
    return send_game_packet(pkt);
}

bool network_system::request_repair_all() {
    if (!game_connection_.is_connected()) {
        return false;
    }

    auto pkt = packets::make_repair_all();
    return send_game_packet(pkt);
}

} // namespace hb
