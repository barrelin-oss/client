#pragma once

#include <cstddef>
#include <cstdint>

namespace hb {

// Display constants
inline constexpr uint32_t screen_width = 640;
inline constexpr uint32_t screen_height = 480;

// Resource limits
inline constexpr std::size_t max_sprites = 20000;
inline constexpr std::size_t max_tiles = 500;
inline constexpr std::size_t max_effect_sprites = 100;
inline constexpr std::size_t max_sound_effects = 110;

// Chat and messages
inline constexpr std::size_t max_chat_messages = 500;
inline constexpr std::size_t max_whisper_messages = 5;
inline constexpr std::size_t max_chat_scroll_messages = 80;
inline constexpr std::size_t max_effects = 300;
inline constexpr std::size_t max_game_messages = 300;

// Chat timing (milliseconds)
inline constexpr uint32_t chat_timeout_a = 4000;
inline constexpr uint32_t chat_timeout_b = 500;
inline constexpr uint32_t chat_timeout_c = 2000;

// Items and inventory
inline constexpr std::size_t max_items = 50;
inline constexpr std::size_t max_bank_items = 121;
inline constexpr std::size_t max_menu_items = 140;
inline constexpr std::size_t max_item_names = 1000;
inline constexpr std::size_t max_sell_list = 12;

// Combat and skills
inline constexpr std::size_t max_magic_types = 100;
inline constexpr std::size_t max_skill_types = 60;
inline constexpr std::size_t max_item_equip_positions = 15;

// Social
inline constexpr std::size_t max_guildsmen = 32;
inline constexpr std::size_t max_guild_names = 100;
inline constexpr std::size_t max_party_members = 8;

// World
inline constexpr std::size_t max_weather_objects = 600;
inline constexpr std::size_t max_build_items = 100;
inline constexpr std::size_t max_crusade_structures = 300;
inline constexpr std::size_t max_text_dialog_lines = 300;

// UI constants
inline constexpr int16_t button_width = 74;
inline constexpr int16_t button_height = 20;
inline constexpr int16_t left_button_x = 30;
inline constexpr int16_t right_button_x = 154;
inline constexpr int16_t button_y = 292;

// Timing
inline constexpr uint32_t double_click_time = 300;
inline constexpr uint32_t socket_block_limit = 300;

// Protocol indices
inline constexpr uint32_t msg_id_index = 0;
inline constexpr uint32_t msg_type_index = 4;

// Version info
inline constexpr uint16_t version_major = 3;
inline constexpr uint16_t version_minor = 0;

} // namespace hb
