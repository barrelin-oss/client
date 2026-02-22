#pragma once

#include "entity/components.hpp"
#include <cstdint>
#include <optional>

namespace hb
{

struct npc_frame_entry
{
    int16_t max_frame = -1;     // -1 = use default
    int16_t frame_time_ms = -1; // -1 = use default
};

// Returns frame data for a given NPC type and animation state.
// Returns nullopt for non-NPC types or if no override exists.
std::optional<npc_frame_entry> get_npc_frame_data(uint16_t visual_type, entity_anim_state state);

} // namespace hb
