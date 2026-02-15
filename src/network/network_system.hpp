#pragma once

#include "network/connection.hpp"
#include "network/packet.hpp"
#include "network/protocol.hpp"
#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>

namespace hb
{

// Packet handler callback
using packet_handler = std::function<void(packet_reader&)>;

class network_system
{
public:
    network_system() = default;
    ~network_system() = default;

    network_system(const network_system&) = delete;
    network_system& operator=(const network_system&) = delete;

    // Initialize/shutdown
    bool initialize();
    void shutdown();

    // Update - call every frame
    void update();

    // Connection management
    bool connect_login_server(std::string_view host, uint16_t port = default_login_port);
    bool connect_game_server(std::string_view host, uint16_t port = default_game_port);
    void disconnect();

    // Connection state
    bool is_connected() const { return login_connection_.is_connected() || game_connection_.is_connected(); }
    bool is_login_connected() const { return login_connection_.is_connected(); }
    bool is_game_connected() const { return game_connection_.is_connected(); }

    // Send packets
    bool send_login_packet(const packet& pkt);
    bool send_game_packet(const packet& pkt);

    // Register packet handlers (32-bit message IDs)
    void register_handler(uint32_t message_id, packet_handler handler);

    // Convenience: login flow
    bool request_login(std::string_view account, std::string_view password);
    bool request_create_account(std::string_view account, std::string_view password, std::string_view email);
    bool request_character_list();
    bool request_create_character(std::string_view name,
                                  uint8_t gender,
                                  uint8_t skin_color,
                                  uint8_t hair_style,
                                  uint8_t hair_color,
                                  uint8_t underwear_color,
                                  uint8_t str,
                                  uint8_t vit,
                                  uint8_t dex,
                                  uint8_t intel,
                                  uint8_t mag,
                                  uint8_t cha);
    bool request_delete_character(std::string_view name, std::string_view password);
    bool request_enter_game(std::string_view character_name);

    // Convenience: game actions
    bool request_move(int32_t x, int32_t y, uint8_t direction);
    bool request_attack(uint32_t target_id, uint8_t attack_type = 0);
    bool request_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id = 0);
    bool request_use_skill(uint16_t skill_id);
    bool request_use_item(uint8_t slot);
    bool request_drop_item(uint8_t slot, uint32_t amount = 1);
    bool request_pickup_item(uint32_t item_id);
    bool request_chat(std::string_view message);
    bool request_whisper(std::string_view target, std::string_view message);

    // Stat allocation
    bool request_add_stat(int stat_index);

    // Inventory/Equipment
    bool request_move_item(int32_t from_slot, int32_t to_slot);
    bool request_unequip(uint8_t slot);

    // Chat with mode
    bool send_chat(std::string_view message, int mode);

    // Shop/Trade
    bool request_buy(uint16_t item_id, int32_t quantity);
    bool request_sell(uint16_t item_id, int32_t quantity);

    // Bank
    bool request_bank_deposit(int32_t slot);
    bool request_bank_withdraw(int32_t slot);

    // Party
    bool request_party_invite(std::string_view name);
    bool request_party_leave();
    bool request_party_kick(std::string_view name);

    // Guild
    bool request_guild_invite(std::string_view name);
    bool request_guild_leave();

    // NPC
    bool send_npc_response(int32_t response_id);

    // Trade
    bool request_trade_invite(std::string_view player_name);
    bool request_trade_add_item(int32_t inventory_slot, int32_t trade_slot);
    bool request_trade_remove_item(int32_t trade_slot);
    bool request_trade_set_gold(uint32_t amount);
    bool request_trade_confirm();
    bool request_trade_cancel();

    // Crafting/Manufacturing
    bool request_craft_item(int32_t recipe_index);
    bool request_add_craft_material(int32_t inventory_slot, int32_t material_slot);

    // Repair
    bool request_repair_item(int32_t inventory_slot);
    bool request_repair_all();

    // Statistics
    uint64_t total_bytes_sent() const;
    uint64_t total_bytes_received() const;

private:
    void process_login_packets();
    void process_game_packets();
    void dispatch_packet(packet_reader& reader);

    connection login_connection_;
    connection game_connection_;

    // Handlers keyed by 32-bit message ID
    std::unordered_map<uint32_t, packet_handler> handlers_;

    std::string current_account_;
    std::string current_password_;
};

} // namespace hb
