#pragma once

#include "network/packet.hpp"
#include "network/protocol.hpp"
#include <functional>

namespace hb {

class game_state_manager;

// Motion/Event handler for entity movement and actions
class motion_handler {
public:
    motion_handler() = default;

    void initialize(game_state_manager& game);

    // Handle motion response (player's own motion confirmed)
    void handle_motion_response(packet_reader& reader);

    // Handle motion event (other entities' motion)
    void handle_motion_event(packet_reader& reader);

    // Handle common event (various gameplay events)
    void handle_common_event(packet_reader& reader);

private:
    // Motion types
    void process_stop(uint32_t entity_id, packet_reader& reader);
    void process_move(uint32_t entity_id, packet_reader& reader);
    void process_run(uint32_t entity_id, packet_reader& reader);
    void process_attack(uint32_t entity_id, packet_reader& reader);
    void process_attack_move(uint32_t entity_id, packet_reader& reader);
    void process_damage(uint32_t entity_id, packet_reader& reader);
    void process_damage_move(uint32_t entity_id, packet_reader& reader);
    void process_magic(uint32_t entity_id, packet_reader& reader);
    void process_get_item(uint32_t entity_id, packet_reader& reader);
    void process_dying(uint32_t entity_id, packet_reader& reader);
    void process_dead(uint32_t entity_id, packet_reader& reader);

    // Common events
    void process_spawn_object(packet_reader& reader);
    void process_despawn_object(packet_reader& reader);
    void process_magic_effect(packet_reader& reader);
    void process_effect(packet_reader& reader);

    game_state_manager* game_ = nullptr;
};

} // namespace hb
