#include "entity/entity_manager.hpp"
#include "audio/sound_manager.hpp"
#include "audio/sound_types.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "world/world.hpp"
#include "world/tile.hpp"
#include "core/constants.hpp"
#include "core/direction_utils.hpp"
#include "gameplay/effect_types.hpp"
#include "gameplay/effect_system.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <array>
#include <string>

namespace hb
{

namespace
{

// Character sprite constants (from menu_character_renderer.hpp)
struct character_sprite_constants
{
    static constexpr uint16_t body_base = 500;
    static constexpr uint16_t body_stride = 120;
    static constexpr uint16_t male_underwear_base = 4580;
    static constexpr uint16_t female_underwear_base = 14580;
    static constexpr uint16_t underwear_stride = 15;
    static constexpr uint16_t male_hair_base = 4820;
    static constexpr uint16_t female_hair_base = 14820;
    static constexpr uint16_t hair_stride = 15;
};

// NPC/Monster sprite constants
// Legacy formula: 1220 + (type - 10) * 56 + action * 8 + (dir - 1)
// Each NPC/monster type has 56 frames: 7 actions × 8 directions
struct npc_sprite_constants
{
    static constexpr uint16_t npc_base = 1220;
    static constexpr uint16_t npc_type_offset = 10;
    static constexpr uint16_t frames_per_type = 56; // 7 actions * 8 directions
    static constexpr uint16_t directions_per_action = 8;
};

// Calculate owner type from gender and skin color
// Returns 1-3 for male, 4-6 for female
inline int32_t calculate_owner_type(uint8_t gender, uint8_t skin_color)
{
    uint8_t clamped_gender = std::clamp(gender, uint8_t(1), uint8_t(2));
    uint8_t clamped_skin = std::clamp(skin_color, uint8_t(1), uint8_t(3));
    bool is_female = (clamped_gender == 2);
    return is_female ? (3 + clamped_skin) : clamped_skin;
}

// Calculate body sprite ID for a character
inline uint16_t calculate_body_sprite_id(int32_t owner_type, int32_t action, int32_t direction)
{
    // Body sprite ID: 500 + (owner_type - 1) * 120 + action * 8 + (dir - 1)
    return static_cast<uint16_t>(character_sprite_constants::body_base +
                                 (owner_type - 1) * character_sprite_constants::body_stride + action * 8 +
                                 (direction - 1));
}

// Calculate underwear sprite ID for a character
inline uint16_t calculate_underwear_sprite_id(bool is_female, uint8_t underwear_color, int32_t action)
{
    uint8_t clamped_color = std::clamp(underwear_color, uint8_t(0), uint8_t(7));
    uint16_t base =
        is_female ? character_sprite_constants::female_underwear_base : character_sprite_constants::male_underwear_base;
    return static_cast<uint16_t>(base + clamped_color * character_sprite_constants::underwear_stride + action);
}

// Calculate hair sprite ID for a character
inline uint16_t calculate_hair_sprite_id(bool is_female, uint8_t hair_style, int32_t action)
{
    uint8_t clamped_style = std::clamp(hair_style, uint8_t(0), uint8_t(7));
    uint16_t base =
        is_female ? character_sprite_constants::female_hair_base : character_sprite_constants::male_hair_base;
    return static_cast<uint16_t>(base + clamped_style * character_sprite_constants::hair_stride + action);
}

// Equipment sprites have different frame counts per direction depending on action type.
// Legacy: OnStop/OnMove/OnRun/OnAttack = 8 frames/dir, OnMagic = 16, OnGetItem/OnDamage = 4
inline int32_t equipment_frames_per_direction(int32_t action)
{
    switch (action)
    {
    case 8:
        return 16; // magic
    case 9:
        return 4; // get_item
    case 10:
        return 4; // damage
    default:
        return 8;
    }
}

// Hair color RGB offsets (from legacy _GetHairColorRGB)
// Legacy values are for RGB565 (5-bit R, 6-bit G, 5-bit B).
// Pre-normalized to 0-1 range: R,B divided by 31, G divided by 63.
struct hair_color_offset
{
    float r, g, b;
};

static constexpr hair_color_offset hair_colors[16] = {
    {14.0f / 31, -5.0f / 63, -5.0f / 31},  //  0: dark red
    {20.0f / 31, 0.0f / 63, 0.0f / 31},    //  1: orange
    {22.0f / 31, 13.0f / 63, -10.0f / 31}, //  2: light brown
    {0.0f / 31, 10.0f / 63, 0.0f / 31},    //  3: green
    {0.0f / 31, 0.0f / 63, 22.0f / 31},    //  4: flashy blue
    {-5.0f / 31, -5.0f / 63, 15.0f / 31},  //  5: dark blue
    {15.0f / 31, -5.0f / 63, 16.0f / 31},  //  6: mauve
    {-6.0f / 31, -6.0f / 63, -6.0f / 31},  //  7: black
    {-10.0f / 31, 0.0f / 63, 0.0f / 31},   //  8: dark
    {0.0f / 31, 0.0f / 63, 0.0f / 31},     //  9: natural
    {0.0f / 31, 0.0f / 63, 0.0f / 31},     // 10: natural
    {22.0f / 31, 22.0f / 63, 22.0f / 31},  // 11: white/bright
    {22.0f / 31, 17.0f / 63, 0.0f / 31},   // 12: golden
    {-5.0f / 31, 0.0f / 63, 22.0f / 31},   // 13: cyan-blue
    {0.0f / 31, 0.0f / 63, 0.0f / 31},     // 14: natural
    {0.0f / 31, 22.0f / 63, 0.0f / 31},    // 15: green
};

// Map object_action to NPC action index (0-6)
// NPC actions: 0=stop, 1=move, 2=attack, 3=damage, 4=dying, 5=dead, 6=magic
inline int32_t action_to_npc_action_index(object_action action)
{
    switch (action)
    {
    case object_action::stop_peace:
    case object_action::stop_combat:
        return 0; // Stop/idle
    case object_action::move_peace:
    case object_action::move_combat:
    case object_action::run:
        return 1; // Move
    case object_action::attack_peace:
    case object_action::attack_combat:
    case object_action::attack_combat_bow:
        return 2; // Attack
    case object_action::damage:
        return 3; // Damage
    case object_action::dying:
        return 4; // Dying
    case object_action::magic:
        return 6; // Magic
    case object_action::get_item:
    default:
        return 0; // Default to stop
    }
}

// Calculate NPC/monster sprite ID
// Formula: 1220 + (type - 10) * 56 + action * 8 + (dir - 1)
inline uint16_t calculate_npc_sprite_id(uint16_t npc_type, int32_t action, int32_t dir)
{
    // npc_type is already the visual type (10=Slime, 11=Skeleton, etc.)
    // Clamp direction to valid range (1-8)
    dir = std::clamp(dir, 1, 8);

    return static_cast<uint16_t>(npc_sprite_constants::npc_base +
                                 (npc_type - npc_sprite_constants::npc_type_offset) *
                                     npc_sprite_constants::frames_per_type +
                                 action * npc_sprite_constants::directions_per_action + (dir - 1));
}

// NPC/Monster animation frame data from legacy MapData.cpp
// m_stFrame[type][action] = {m_sMaxFrame, m_sFrameTime}
// m_sMaxFrame = last frame INDEX (frame count = maxFrame + 1)
// m_sFrameTime = milliseconds per frame
struct npc_frame_entry
{
    int16_t max_frame;     // -1 = use default
    int16_t frame_time_ms; // -1 = use default
};

// Legacy action indices (from ActionID.h)
enum legacy_action : int
{
    act_stop = 0,
    act_move = 1,
    act_run = 2,
    act_attack = 3,
    act_magic = 4,
    act_getitem = 5,
    act_damage = 6,
    act_damagemove = 7,
    act_attackmove = 8,
    // 9 unused
    act_dying = 10,
    legacy_action_count = 11,
};

static constexpr int npc_type_first = 10;
static constexpr int npc_type_last = 69;
static constexpr int npc_type_count = npc_type_last - npc_type_first + 1;

// Table populated once, indexed by [visual_type - 10][legacy_action]
static npc_frame_entry npc_frame_table[npc_type_count][legacy_action_count];
static bool npc_frame_table_ready = false;

static void init_npc_frame_table()
{
    if (npc_frame_table_ready)
        return;

    // Initialize all to "use default"
    for (int i = 0; i < npc_type_count; i++)
        for (int j = 0; j < legacy_action_count; j++)
            npc_frame_table[i][j] = {-1, -1};

    // Default: all NPC types get MOVE maxFrame = 7
    for (int i = 0; i < npc_type_count; i++)
        npc_frame_table[i][act_move].max_frame = 7;

    // Helper lambda: set(type, action, maxFrame, frameTime)
    auto set = [](int type, int action, int16_t mf, int16_t ft)
    {
        npc_frame_table[type - npc_type_first][action] = {mf, ft};
    };
    // Helper: set only frame time
    auto set_ft = [](int type, int action, int16_t ft)
    {
        npc_frame_table[type - npc_type_first][action].frame_time_ms = ft;
    };

    // === Data from MapData.cpp constructor ===

    // Slime (10)
    set(10, act_stop, 3, 240);
    set_ft(10, act_move, 120);
    set(10, act_attack, 3, 90);
    set(10, act_damage, 7, 150); // 3+4
    set(10, act_dying, 3, 240);

    // Skeleton (11)
    set(11, act_stop, 3, 150);
    set_ft(11, act_move, 90);
    set(11, act_attack, 3, 90);
    set(11, act_damage, 7, 150);
    set(11, act_dying, 7, 180);

    // Stone-Golem (12)
    set(12, act_stop, 3, 210);
    set_ft(12, act_move, 100);
    set(12, act_attack, 3, 120);
    set(12, act_damage, 7, 150);
    set(12, act_dying, 7, 180);

    // Cyclops (13)
    set(13, act_stop, 3, 210);
    set_ft(13, act_move, 80);
    set(13, act_attack, 3, 90);
    set(13, act_damage, 7, 150);
    set(13, act_dying, 7, 180);

    // Orc (14)
    set(14, act_stop, 3, 180);
    set_ft(14, act_move, 80);
    set(14, act_attack, 3, 120);
    set(14, act_damage, 7, 150);
    set(14, act_dying, 7, 180);

    // ShopKeeper (15)
    set(15, act_stop, 7, 180);
    set_ft(15, act_move, 100);
    set(15, act_attack, 3, 150);
    set(15, act_damage, 3, 180);
    set(15, act_dying, 7, 180);

    // Giant Ant (16)
    set(16, act_stop, 3, 120);
    set_ft(16, act_move, 60);
    set(16, act_attack, 3, 120);
    set(16, act_damage, 7, 150);
    set(16, act_dying, 7, 180);

    // Scorpion (17)
    set(17, act_stop, 3, 120);
    set_ft(17, act_move, 45);
    set(17, act_attack, 3, 120);
    set(17, act_damage, 7, 150);
    set(17, act_dying, 7, 180);

    // Zombie (18)
    set(18, act_stop, 3, 210);
    set_ft(18, act_move, 130);
    set(18, act_attack, 3, 150);
    set(18, act_damage, 7, 150);
    set(18, act_dying, 7, 180);

    // Gandalf (19)
    set(19, act_stop, 7, 250);
    set_ft(19, act_move, 100);
    set(19, act_attack, 3, 150);
    set(19, act_damage, 3, 180);
    set(19, act_dying, 7, 180);

    // Howard (20)
    set(20, act_stop, 7, 250);
    set_ft(20, act_move, 100);
    set(20, act_attack, 3, 150);
    set(20, act_damage, 3, 180);
    set(20, act_dying, 7, 180);

    // Guard (21)
    set(21, act_stop, 3, 250);
    set_ft(21, act_move, 80);
    set(21, act_attack, 3, 120);
    set(21, act_damage, 7, 150);
    set(21, act_dying, 7, 180);

    // Amphisbena (22)
    set(22, act_stop, 3, 250);
    set_ft(22, act_move, 80);
    set(22, act_attack, 3, 120);
    set(22, act_damage, 7, 150);
    set(22, act_dying, 7, 180);

    // Clay-Golem (23)
    set(23, act_stop, 3, 250);
    set_ft(23, act_move, 80);
    set(23, act_attack, 3, 120);
    set(23, act_damage, 7, 150);
    set(23, act_dying, 7, 180);

    // Tom (24)
    set(24, act_stop, 7, 150);

    // William (25)
    set(25, act_stop, 7, 250);

    // Kennedy (26)
    set(26, act_stop, 7, 250);

    // Hellbound (27)
    set(27, act_stop, 3, 250);
    set_ft(27, act_move, 50);
    set(27, act_attack, 3, 120);
    set(27, act_damage, 7, 120);
    set(27, act_dying, 7, 180);

    // Troll (28)
    set(28, act_stop, 3, 250);
    set_ft(28, act_move, 100);
    set(28, act_attack, 5, 60);
    set(28, act_damage, 7, 120);
    set(28, act_dying, 9, 100);

    // Ogre (29)
    set(29, act_stop, 3, 250);
    set_ft(29, act_move, 100);
    set(29, act_attack, 5, 120);
    set(29, act_damage, 7, 120);
    set(29, act_dying, 9, 100);

    // Liche (30)
    set(30, act_stop, 3, 250);
    set_ft(30, act_move, 100);
    set(30, act_attack, 5, 120);
    set(30, act_damage, 7, 120);
    set(30, act_dying, 9, 100);

    // Demon (31)
    set(31, act_stop, 3, 250);
    set_ft(31, act_move, 100);
    set(31, act_attack, 7, 120);
    set(31, act_damage, 7, 120);
    set(31, act_dying, 9, 100);

    // Unicorn (32)
    set(32, act_stop, 3, 250);
    set_ft(32, act_move, 100);
    set(32, act_attack, 7, 120);
    set(32, act_damage, 7, 120);
    set(32, act_dying, 11, 100);

    // WereWolf (33)
    set(33, act_stop, 3, 250);
    set_ft(33, act_move, 120);
    set(33, act_attack, 7, 120);
    set(33, act_damage, 7, 120);
    set(33, act_dying, 11, 100);

    // Dummy (34)
    set(34, act_stop, 3, 240);
    set_ft(34, act_move, 120);
    set(34, act_attack, 3, 90);
    set(34, act_damage, 7, 150);
    set(34, act_dying, 7, 240);

    // Energy-Ball (35)
    set(35, act_stop, 9, 80);
    set(35, act_move, 3, 20);
    set(35, act_attack, 3, 80);
    set(35, act_damage, 7, 80);
    set(35, act_dying, 7, 80);

    // Crossbow Guard Tower (36)
    set(36, act_stop, 0, 250);
    set(36, act_move, 0, 80);
    set(36, act_attack, 3, 120);
    set(36, act_damage, 0, 150);
    set(36, act_dying, 6, 200);

    // Cannon Guard Tower (37)
    set(37, act_stop, 0, 250);
    set(37, act_move, 0, 80);
    set(37, act_attack, 3, 120);
    set(37, act_damage, 0, 150);
    set(37, act_dying, 6, 200);

    // Mana Collector (38)
    set(38, act_stop, 0, 250);
    set(38, act_move, 0, 80);
    set(38, act_attack, 3, 120);
    set(38, act_damage, 0, 150);
    set(38, act_dying, 6, 200);

    // Detector (39)
    set(39, act_stop, 0, 250);
    set(39, act_move, 0, 80);
    set(39, act_attack, 3, 120);
    set(39, act_damage, 0, 150);
    set(39, act_dying, 6, 200);

    // ESG (40)
    set(40, act_stop, 0, 250);
    set(40, act_move, 0, 80);
    set(40, act_attack, 3, 120);
    set(40, act_damage, 0, 150);
    set(40, act_dying, 6, 200);

    // GMG (41)
    set(41, act_stop, 0, 250);
    set(41, act_move, 0, 80);
    set(41, act_attack, 3, 120);
    set(41, act_damage, 0, 150);
    set(41, act_dying, 6, 200);

    // ManaStone (42)
    set(42, act_stop, 0, 250);
    set(42, act_move, 0, 80);
    set(42, act_attack, 3, 120);
    set(42, act_damage, 0, 150);
    set(42, act_dying, 0, 200);

    // Light War Beetle (43)
    set(43, act_stop, 7, 250);
    set_ft(43, act_move, 100);
    set(43, act_attack, 7, 60);
    set(43, act_damage, 10, 120); // 3+7
    set(43, act_dying, 9, 100);

    // God's Hand Knight (44)
    set(44, act_stop, 7, 250);
    set_ft(44, act_move, 100);
    set(44, act_attack, 7, 60);
    set(44, act_damage, 10, 120);
    set(44, act_dying, 9, 100);

    // GHKABS (45)
    set(45, act_stop, 7, 250);
    set_ft(45, act_move, 100);
    set(45, act_attack, 7, 60);
    set(45, act_damage, 10, 120);
    set(45, act_dying, 9, 100);

    // Temple Knight (46)
    set(46, act_stop, 7, 250);
    set_ft(46, act_move, 100);
    set(46, act_attack, 7, 60);
    set(46, act_damage, 10, 120);
    set(46, act_dying, 9, 100);

    // Battle Golem (47)
    set(47, act_stop, 7, 250);
    set_ft(47, act_move, 100);
    set(47, act_attack, 7, 60);
    set(47, act_damage, 10, 120);
    set(47, act_dying, 9, 100);

    // Stalker (48)
    set(48, act_stop, 7, 250);
    set_ft(48, act_move, 100);
    set(48, act_attack, 7, 60);
    set(48, act_damage, 10, 120);
    set(48, act_dying, 9, 100);

    // Hellclaw (49)
    set(49, act_stop, 7, 250);
    set_ft(49, act_move, 100);
    set(49, act_attack, 7, 60);
    set(49, act_damage, 10, 120);
    set(49, act_dying, 9, 100);

    // Tigerworm (50)
    set(50, act_stop, 7, 250);
    set_ft(50, act_move, 100);
    set(50, act_attack, 7, 60);
    set(50, act_damage, 10, 120);
    set(50, act_dying, 9, 100);

    // Catapult (51)
    set(51, act_stop, 0, 250);
    set_ft(51, act_move, 100);
    set(51, act_attack, 4, 60);
    set(51, act_damage, 0, 120);
    set(51, act_dying, 6, 100);

    // Gargoyle (52)
    set(52, act_stop, 7, 250);
    set_ft(52, act_move, 100);
    set(52, act_attack, 9, 70);
    set(52, act_damage, 7, 120);
    set(52, act_dying, 14, 100); // 11+3

    // Beholder (53)
    set(53, act_stop, 7, 250);
    set_ft(53, act_move, 100);
    set(53, act_attack, 12, 60);
    set(53, act_damage, 7, 120);
    set(53, act_dying, 10, 70); // 7+3

    // DarkElf (54)
    set(54, act_stop, 7, 250);
    set_ft(54, act_move, 100);
    set(54, act_attack, 9, 60);
    set(54, act_damage, 7, 120);
    set(54, act_dying, 10, 100); // 7+3

    // Bunny (55)
    set(55, act_stop, 7, 250);
    set_ft(55, act_move, 70);
    set(55, act_attack, 7, 100);
    set(55, act_damage, 7, 100);
    set(55, act_dying, 10, 150); // 7+3

    // Cat (56)
    set(56, act_stop, 7, 250);
    set_ft(56, act_move, 100);
    set(56, act_attack, 7, 60);
    set(56, act_damage, 7, 100);
    set(56, act_dying, 10, 150); // 7+3

    // GiantFrog (57)
    set(57, act_stop, 7, 300);
    set_ft(57, act_move, 100);
    set(57, act_attack, 7, 100);
    set(57, act_damage, 7, 100);
    set(57, act_dying, 10, 150); // 7+3

    // Mountain Giant (58)
    set(58, act_stop, 7, 250);
    set_ft(58, act_move, 90);
    set(58, act_attack, 7, 100);
    set(58, act_damage, 7, 100);
    set(58, act_dying, 10, 150); // 7+3

    // Ettin (59)
    set(59, act_stop, 7, 250);
    set_ft(59, act_move, 90);
    set(59, act_attack, 7, 100);
    set(59, act_damage, 7, 100);
    set(59, act_dying, 10, 150); // 7+3

    // Cannibal Plant (60)
    set(60, act_stop, 7, 250);
    set_ft(60, act_move, 120);
    set(60, act_attack, 7, 100);
    set(60, act_damage, 7, 100);
    set(60, act_dying, 10, 150); // 7+3

    // Rudolph (61)
    set(61, act_stop, 7, 200);
    set_ft(61, act_move, 90);
    set(61, act_attack, 7, 120);
    set(61, act_damage, 7, 60);
    set(61, act_dying, 10, 150); // 7+3

    // Dire Boar (62)
    set(62, act_stop, 7, 200);
    set_ft(62, act_move, 60);
    set(62, act_attack, 7, 60);
    set(62, act_damage, 7, 60);
    set(62, act_dying, 10, 150); // 7+3

    // Frost (63)
    set(63, act_stop, 7, 200);
    set_ft(63, act_move, 60);
    set(63, act_attack, 7, 80);
    set(63, act_damage, 7, 60);
    set(63, act_dying, 8, 150); // 5+3

    // Crop (64)
    set(64, act_stop, 40, 200);
    set_ft(64, act_move, 200);
    set(64, act_attack, 3, 200);
    set(64, act_damage, 3, 200);
    set(64, act_dying, 3, 200);

    // Ice Golem (65)
    set(65, act_stop, 7, 200);
    set_ft(65, act_move, 140);
    set(65, act_attack, 7, 105);
    set(65, act_damage, 7, 60);
    set(65, act_dying, 10, 150); // 7+3

    // Wyvern (66)
    set(66, act_stop, 7, 100);
    set_ft(66, act_move, 90);
    set(66, act_attack, 7, 80);
    set(66, act_damage, 7, 60);
    set(66, act_dying, 18, 65); // 15+3

    // McGaffin (67)
    set(67, act_stop, 3, 200);
    set_ft(67, act_move, 120);
    set(67, act_attack, 3, 80);
    set(67, act_damage, 3, 60);
    set(67, act_dying, 6, 65); // 3+3

    // Perry (68)
    set(68, act_stop, 3, 200);
    set(68, act_move, 3, 90);
    set(68, act_attack, 3, 80);
    set(68, act_damage, 3, 60);
    set(68, act_dying, 6, 65); // 3+3

    // Devlin (69)
    set(69, act_stop, 3, 200);
    set(69, act_move, 3, 90);
    set(69, act_attack, 3, 80);
    set(69, act_damage, 3, 60);
    set(69, act_dying, 6, 65); // 3+3

    // Legacy inherited defaults for NPC types (MapData.cpp lines 44-55):
    // RUN = MOVE, DAMAGEMOVE = DAMAGE, ATTACKMOVE = ATTACK, MAGIC = STOP, GETITEM = STOP
    for (int i = 0; i < npc_type_count; i++)
    {
        auto& tbl = npc_frame_table[i];
        tbl[act_run] = tbl[act_move];
        tbl[act_damagemove] = tbl[act_damage];
        tbl[act_attackmove] = tbl[act_attack];

        // MAGIC and GETITEM inherit from STOP only if not already set
        if (tbl[act_magic].max_frame < 0)
            tbl[act_magic] = tbl[act_stop];
        if (tbl[act_getitem].max_frame < 0)
            tbl[act_getitem] = tbl[act_stop];
    }

    npc_frame_table_ready = true;
}

// Map entity_anim_state to legacy action index for NPC frame table lookup
inline int anim_state_to_legacy_action(entity_anim_state state)
{
    switch (state)
    {
    case entity_anim_state::stop:
        return act_stop;
    case entity_anim_state::move:
        return act_move;
    case entity_anim_state::run:
        return act_run;
    case entity_anim_state::attack:
        return act_attack;
    case entity_anim_state::attack_move:
        return act_attackmove;
    case entity_anim_state::damage:
        return act_damage;
    case entity_anim_state::damage_move:
        return act_damagemove;
    case entity_anim_state::magic:
    case entity_anim_state::magic_attack:
        return act_magic;
    case entity_anim_state::get_item:
        return act_getitem;
    case entity_anim_state::dying:
        return act_dying;
    default:
        return act_stop;
    }
}

// Apply NPC-specific frame data to an animation component.
// Returns true if overrides were applied.
inline bool apply_npc_frame_data(animation_component& anim, uint16_t visual_type)
{
    if (visual_type < npc_type_first || visual_type > npc_type_last)
        return false;

    init_npc_frame_table();

    int type_idx = visual_type - npc_type_first;
    int action_idx = anim_state_to_legacy_action(anim.state);
    if (action_idx < 0 || action_idx >= legacy_action_count)
        return false;

    const auto& entry = npc_frame_table[type_idx][action_idx];

    if (entry.max_frame >= 0)
        anim.frame_count = static_cast<uint8_t>(entry.max_frame + 1);

    if (entry.frame_time_ms > 0)
        anim.frame_duration = static_cast<float>(entry.frame_time_ms) / 1000.0f;

    return true;
}

// Get the visual type for an entity (NPC/monster type ID)
// Returns 0 if no valid visual type is set.
inline uint16_t get_entity_visual_type(const entity& e)
{
    uint16_t visual_type = e.visual_type();
    if (visual_type == 0)
    {
        if (e.has_npc())
            visual_type = e.npc().npc_type;
        else if (e.has_monster())
            visual_type = e.monster().monster_type;
    }
    return visual_type;
}

} // anonymous namespace

void entity_manager::initialize(sound_manager* sounds)
{
    sounds_ = sounds;
    spdlog::info("Entity manager initialized");
}

void entity_manager::shutdown()
{
    remove_all_entities();
    spdlog::info("Entity manager shutdown");
}

entity& entity_manager::create_entity(entity_type type)
{
    entity_id id = next_entity_id_++;
    return create_entity_with_id(id, type);
}

entity& entity_manager::create_entity_with_id(entity_id id, entity_type type)
{
    auto e = std::make_unique<entity>(id, type);

    // Add default components based on type
    switch (type)
    {
    case entity_type::player:
    case entity_type::character:
        e->add_stats();
        e->add_combat();
        e->add_name();
        e->add_movement();
        break;

    case entity_type::npc:
        e->add_name();
        e->add_npc();
        e->add_movement();
        break;

    case entity_type::monster:
        e->add_stats();
        e->add_combat();
        e->add_name();
        e->add_movement();
        e->add_monster();
        break;

    case entity_type::effect:
        e->add_effect();
        break;

    default:
        break;
    }

    entity& ref = *e;
    entities_[id] = std::move(e);

    if (on_created_)
    {
        on_created_(ref);
    }

    spdlog::debug("Created entity {} of type {}", id, static_cast<int>(type));
    return ref;
}

entity* entity_manager::get_entity(entity_id id)
{
    auto it = entities_.find(id);
    if (it != entities_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

const entity* entity_manager::get_entity(entity_id id) const
{
    auto it = entities_.find(id);
    if (it != entities_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

bool entity_manager::entity_exists(entity_id id) const
{
    return entities_.find(id) != entities_.end();
}

void entity_manager::remove_entity(entity_id id)
{
    auto it = entities_.find(id);
    if (it != entities_.end())
    {
        it->second->begin_fade_out();
    }
}

void entity_manager::transition_to_dead(entity_id id)
{
    entity* ent = get_entity(id);
    if (!ent)
        return;

    ent->set_alive(false);

    // Enforce 1-dead-per-tile: remove any existing corpse on this tile
    const auto& t = ent->transform();
    for (auto& [oid, other] : entities_)
    {
        if (oid == id || other->should_remove())
            continue;
        if (!other->is_alive() && other->transform().tile_x == t.tile_x && other->transform().tile_y == t.tile_y)
        {
            other->mark_for_removal();
        }
    }
}

void entity_manager::remove_all_entities()
{
    for (auto& [id, e] : entities_)
    {
        if (on_removed_)
        {
            on_removed_(id);
        }
    }
    entities_.clear();
    local_player_id_ = invalid_entity_id;
}

void entity_manager::remove_entities_of_type(entity_type type)
{
    for (auto& [id, e] : entities_)
    {
        if (e->type() == type)
        {
            e->mark_for_removal();
        }
    }
}

std::vector<entity*> entity_manager::get_entities_of_type(entity_type type)
{
    std::vector<entity*> result;
    for (auto& [id, e] : entities_)
    {
        if (e->type() == type && !e->should_remove())
        {
            result.push_back(e.get());
        }
    }
    return result;
}

std::vector<entity*> entity_manager::get_entities_in_range(int32_t x, int32_t y, int32_t range)
{
    std::vector<entity*> result;
    int32_t range_sq = range * range;

    for (auto& [id, e] : entities_)
    {
        if (e->should_remove() || e->is_fading_out())
            continue;

        const auto& t = e->transform();
        int32_t dx = t.x - x;
        int32_t dy = t.y - y;
        if (dx * dx + dy * dy <= range_sq)
        {
            result.push_back(e.get());
        }
    }
    return result;
}

std::vector<entity*> entity_manager::get_entities_on_tile(int32_t tile_x, int32_t tile_y)
{
    std::vector<entity*> result;
    for (auto& [id, e] : entities_)
    {
        if (e->should_remove() || e->is_fading_out())
            continue;

        const auto& t = e->transform();
        if (t.tile_x == tile_x && t.tile_y == tile_y)
        {
            result.push_back(e.get());
        }
    }
    return result;
}

std::vector<std::pair<int32_t, int32_t>> entity_manager::get_occupied_tiles() const
{
    std::vector<std::pair<int32_t, int32_t>> result;
    for (const auto& [id, e] : entities_)
    {
        if (e->should_remove() || e->is_fading_out())
            continue;
        if (!e->is_alive())
            continue;
        const auto& t = e->transform();
        result.emplace_back(t.tile_x, t.tile_y);
    }
    return result;
}

entity* entity_manager::find_at_tile(int32_t tile_x, int32_t tile_y)
{
    for (auto& [id, e] : entities_)
    {
        if (e->should_remove() || e->is_fading_out())
            continue;
        if (!e->is_alive())
            continue; // Skip dead entities (corpses)
        if (id == local_player_id_)
            continue; // Skip local player

        const auto& t = e->transform();
        if (t.tile_x == tile_x && t.tile_y == tile_y)
        {
            return e.get();
        }
    }
    return nullptr;
}

entity* entity_manager::get_entity_at_screen_pos(int32_t screen_x, int32_t screen_y, int32_t camera_x, int32_t camera_y)
{
    // Convert screen to world position
    int32_t world_x = screen_x + camera_x;
    int32_t world_y = screen_y + camera_y;

    // Find entity closest to click point
    entity* closest = nullptr;
    int32_t closest_dist_sq = INT32_MAX;

    // Two-pass: prefer alive entities, fall back to dead
    for (int pass = 0; pass < 2; ++pass)
    {
        bool want_alive = (pass == 0);
        for (auto& [id, e] : entities_)
        {
            if (e->should_remove() || e->is_fading_out())
                continue;
            if (e->type() == entity_type::effect)
                continue;
            if (e->is_alive() != want_alive)
                continue;

            const auto& t = e->transform();
            int32_t dx = t.x - world_x;
            int32_t dy = t.y - world_y;
            int32_t dist_sq = dx * dx + dy * dy;

            if (dist_sq < 32 * 32 && dist_sq < closest_dist_sq)
            {
                closest = e.get();
                closest_dist_sq = dist_sq;
            }
        }
        if (closest)
            break; // Found alive entity, don't check dead
    }

    return closest;
}

entity* entity_manager::get_entity_at_screen_pos(sprite_manager& sprites, int32_t screen_x, int32_t screen_y, int32_t camera_x, int32_t camera_y)
{
    // Sprite-bounds-based hit testing: finds the entity whose rendered sprite
    // contains the mouse point. Among overlapping sprites, prefers alive over dead
    // and higher Y (rendered on top due to depth sorting).
    entity* best = nullptr;

    for (int pass = 0; pass < 2; ++pass)
    {
        bool want_alive = (pass == 0);
        for (auto& [id, e] : entities_)
        {
            if (e->should_remove() || e->is_fading_out())
                continue;
            if (e->type() == entity_type::effect)
                continue;
            if (e->is_alive() != want_alive)
                continue;

            if (!is_point_in_entity_sprite(*e, sprites, camera_x, camera_y, screen_x, screen_y))
                continue;

            // Prefer entity with highest Y (rendered on top)
            if (!best || e->transform().y > best->transform().y)
                best = e.get();
        }
        if (best)
            break;
    }

    return best;
}

bool entity_manager::is_point_in_entity_sprite(const entity& e,
                                               sprite_manager& sprites,
                                               int32_t camera_x,
                                               int32_t camera_y,
                                               int32_t mouse_x,
                                               int32_t mouse_y) const
{
    const auto& t = e.transform();
    const auto& s = e.sprite();
    const auto& a = e.animation();

    int32_t screen_x = t.x - camera_x;
    int32_t screen_y = t.y - camera_y;

    // Get sprite bounds based on entity type
    if (e.type() == entity_type::effect)
    {
        // Effects are not clickable
        return false;
    }
    else if (e.type() == entity_type::npc || e.type() == entity_type::monster)
    {
        // NPCs and monsters - use NPC sprite bounds
        int32_t dir = direction_to_sprite_index(t.facing);
        int32_t npc_action = action_to_npc_action_index(e.current_action());

        uint16_t visual_type = get_entity_visual_type(e);

        // No valid visual type - use fallback bounds
        if (visual_type < npc_sprite_constants::npc_type_offset)
        {
            return (mouse_x >= screen_x - 32 && mouse_x < screen_x + 32 && mouse_y >= screen_y - 48 &&
                    mouse_y < screen_y + 16);
        }

        uint16_t sprite_id = calculate_npc_sprite_id(visual_type, npc_action, dir);
        const sprite* npc_spr = sprites.get_sprite_by_id(sprite_id);

        if (npc_spr && npc_spr->has_metadata())
        {
            uint32_t frame = a.current_frame;
            if (npc_spr->frame_count() > 0 && frame >= npc_spr->frame_count())
                frame = frame % npc_spr->frame_count();
            sf::IntRect bounds = npc_spr->get_bounds(screen_x, screen_y, frame);
            return (mouse_x >= bounds.position.x && mouse_x < bounds.position.x + bounds.size.x &&
                    mouse_y >= bounds.position.y && mouse_y < bounds.position.y + bounds.size.y);
        }

        // Fallback to approximate bounds
        return (mouse_x >= screen_x - 32 && mouse_x < screen_x + 32 && mouse_y >= screen_y - 48 &&
                mouse_y < screen_y + 16);
    }
    else
    {
        // Player characters - use body sprite bounds
        int32_t dir = direction_to_sprite_index(t.facing);
        int32_t action = static_cast<int32_t>(e.current_action());
        int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);

        uint16_t body_id = calculate_body_sprite_id(owner_type, action, dir);
        const sprite* body_spr = sprites.get_sprite_by_id(body_id);

        if (body_spr && body_spr->has_metadata())
        {
            uint32_t frame = a.current_frame;
            if (body_spr->frame_count() > 0 && frame >= body_spr->frame_count())
                frame = frame % body_spr->frame_count();
            sf::IntRect bounds = body_spr->get_bounds(screen_x, screen_y, frame);
            return (mouse_x >= bounds.position.x && mouse_x < bounds.position.x + bounds.size.x &&
                    mouse_y >= bounds.position.y && mouse_y < bounds.position.y + bounds.size.y);
        }

        // Fallback to approximate 64x64 bounds if sprite not available
        return (mouse_x >= screen_x - 32 && mouse_x < screen_x + 32 && mouse_y >= screen_y - 64 && mouse_y < screen_y);
    }
}

std::optional<sf::IntRect> entity_manager::get_entity_screen_bounds(const entity& e,
                                                                    sprite_manager& sprites,
                                                                    int32_t camera_x,
                                                                    int32_t camera_y) const
{
    const auto& t = e.transform();
    const auto& s = e.sprite();
    const auto& a = e.animation();

    int32_t screen_x = t.x - camera_x;
    int32_t screen_y = t.y - camera_y;

    if (e.type() == entity_type::npc || e.type() == entity_type::monster)
    {
        int32_t dir = direction_to_sprite_index(t.facing);
        int32_t npc_action = action_to_npc_action_index(e.current_action());
        uint16_t visual_type = get_entity_visual_type(e);

        if (visual_type < npc_sprite_constants::npc_type_offset)
            return std::nullopt;

        uint16_t sprite_id = calculate_npc_sprite_id(visual_type, npc_action, dir);
        const sprite* npc_spr = sprites.get_sprite_by_id(sprite_id);

        if (npc_spr && npc_spr->has_metadata())
        {
            uint32_t frame = a.current_frame;
            if (npc_spr->frame_count() > 0 && frame >= npc_spr->frame_count())
                frame = frame % npc_spr->frame_count();
            return npc_spr->get_bounds(screen_x, screen_y, frame);
        }
    }
    else if (e.type() == entity_type::player || e.type() == entity_type::character)
    {
        int32_t dir = direction_to_sprite_index(t.facing);
        int32_t action = static_cast<int32_t>(e.current_action());
        int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);

        uint16_t body_id = calculate_body_sprite_id(owner_type, action, dir);
        const sprite* body_spr = sprites.get_sprite_by_id(body_id);

        if (body_spr && body_spr->has_metadata())
        {
            uint32_t frame = a.current_frame;
            if (body_spr->frame_count() > 0 && frame >= body_spr->frame_count())
                frame = frame % body_spr->frame_count();
            return body_spr->get_bounds(screen_x, screen_y, frame);
        }
    }

    return std::nullopt;
}

entity* entity_manager::local_player()
{
    return get_entity(local_player_id_);
}

const entity* entity_manager::local_player() const
{
    return get_entity(local_player_id_);
}

void entity_manager::update(float delta_time, world& w, bool local_player_combat_mode)
{
    // Update all entities
    for (auto& [id, e] : entities_)
    {
        if (!e->should_remove())
        {
            update_entity(*e, delta_time, w, local_player_combat_mode);
        }
    }

    // Clean up removed entities
    cleanup_removed_entities();
}

void entity_manager::cleanup_removed_entities()
{
    for (auto it = entities_.begin(); it != entities_.end();)
    {
        if (it->second->should_remove())
        {
            if (on_removed_)
            {
                on_removed_(it->first);
            }
            it = entities_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void entity_manager::update_entity(entity& e, float delta_time, world& w, bool local_player_combat_mode)
{
    // Update fade-out (skip everything else if fading)
    if (e.is_fading_out())
    {
        e.update_fade(delta_time);
        return;
    }

    // Update animation
    update_animation(e, delta_time, local_player_combat_mode);

    // Update movement
    if (e.has_movement())
    {
        update_movement(e, delta_time, w, local_player_combat_mode);
    }

    // Update combat cooldowns
    if (e.has_combat())
    {
        auto& combat = e.combat();
        if (combat.attack_cooldown > 0)
        {
            combat.attack_cooldown -= delta_time;
        }
        if (combat.spell_cooldown > 0)
        {
            combat.spell_cooldown -= delta_time;
        }
        if (combat.status_duration > 0)
        {
            combat.status_duration -= delta_time;
            if (combat.status_duration <= 0)
            {
                // Clear status effects
                combat.poisoned = false;
                combat.paralyzed = false;
                combat.frozen = false;
            }
        }
    }

    // Update chat bubble timer
    if (e.has_name())
    {
        auto& name = e.name();
        if (name.chat_timer > 0)
        {
            name.chat_timer -= delta_time;
            name.chat_elapsed += delta_time;
            if (name.chat_timer <= 0)
            {
                name.chat_message.clear();
                name.chat_elapsed = 0.0f;
            }
        }
    }

    // Update effect
    if (e.has_effect())
    {
        auto& effect = e.effect();
        effect.effect_timer += delta_time;
        if (effect.effect_timer >= effect.effect_duration)
        {
            e.mark_for_removal();
        }
    }
}

// Structs defined here so update_animation can use get_monster_sounds for damage frame lookup.
// Full lookup table is defined below play_footstep_sound.
struct monster_sound_entry
{
    char type;
    int16_t num;
};

struct monster_sounds
{
    monster_sound_entry move;
    monster_sound_entry attack;
    monster_sound_entry damage;
    int8_t damage_frame;
    monster_sound_entry death;
    int8_t death_frame;
};

static monster_sounds get_monster_sounds(uint16_t visual_type);

void entity_manager::update_animation(entity& e, float delta_time, bool local_player_combat_mode)
{
    auto& anim = e.animation();

    // Override frame count and timing for NPCs/monsters using per-type data
    if (e.type() == entity_type::npc || e.type() == entity_type::monster)
    {
        uint16_t vtype = get_entity_visual_type(e);
        if (vtype >= npc_type_first)
        {
            apply_npc_frame_data(anim, vtype);
            // Clamp frame if override reduced frame_count below current position
            if (anim.current_frame >= anim.frame_count)
            {
                if (anim.state == entity_anim_state::dying)
                {
                    spdlog::warn("Entity {} dying frame CLAMPED: frame={} >= count={} (resetting to 0)",
                                 e.id(),
                                 anim.current_frame,
                                 anim.frame_count);
                }
                anim.current_frame = 0;
            }
        }
    }

    if (anim.finished && !anim.looping)
    {
        // Handle state transitions when animation finishes
        switch (anim.state)
        {
        case entity_anim_state::attack:
        case entity_anim_state::attack_move:
        case entity_anim_state::damage:
        case entity_anim_state::damage_move:
        case entity_anim_state::magic:
        case entity_anim_state::magic_attack:
        case entity_anim_state::get_item:
            // Return to idle after these animations (combat idle if in combat mode)
            {
                bool combat = (e.id() == local_player_id_) ? local_player_combat_mode
                                                           : (e.has_combat() && e.combat().combat_stance);
                e.set_action_with_combat_mode(object_action::stop_peace, combat);
            }
            break;
        case entity_anim_state::dying:
            // Hold on last frame — transition_to_dead already called at death time
            anim.current_frame = anim.frame_count > 0 ? anim.frame_count - 1 : 0;
            break;
        default:
            break;
        }
        return;
    }

    anim.frame_timer += delta_time;
    if (anim.frame_timer >= anim.frame_duration)
    {
        anim.frame_timer -= anim.frame_duration;
        anim.current_frame++;

        if (!e.is_alive() && anim.state == entity_anim_state::dying)
        {
            spdlog::debug("Entity {} dying frame tick: {}/{} (dt={:.3f} dur={:.3f})",
                          e.id(),
                          anim.current_frame,
                          anim.frame_count,
                          delta_time,
                          anim.frame_duration);
        }

        // Play footstep sounds on walk/run animations
        // Walk: frames 1 and 3 (alternating footsteps)
        // Run: frames 1 and 3 as well
        if (anim.state == entity_anim_state::move)
        {
            if (anim.current_frame == 1 || anim.current_frame == 3)
            {
                play_footstep_sound(e, false); // Walking
            }
        }
        else if (anim.state == entity_anim_state::run)
        {
            if (anim.current_frame == 1 || anim.current_frame == 3)
            {
                play_footstep_sound(e, true); // Running
            }
        }

        // Play monster movement sound on frame 1
        if (anim.current_frame == 1 && (e.type() == entity_type::npc || e.type() == entity_type::monster) &&
            (anim.state == entity_anim_state::move || anim.state == entity_anim_state::run))
        {
            play_monster_sound(e, monster_sound_type::move);
        }

        // Check for attack trigger frame (frame 4 for attacks)
        if (anim.state == entity_anim_state::attack || anim.state == entity_anim_state::attack_move ||
            anim.state == entity_anim_state::magic_attack)
        {
            if (anim.current_frame == 4 && !anim.attack_triggered)
            {
                anim.attack_triggered = true;
                // Attack damage would be triggered here through callback
            }
        }

        // Spawn hit effect at frame 4 of damage/dying animations (legacy frame split transition)
        if (anim.current_frame == 4 &&
            (anim.state == entity_anim_state::damage || anim.state == entity_anim_state::damage_move ||
             anim.state == entity_anim_state::dying))
        {
            if (effects_)
            {
                const auto& t = e.transform();
                effects_->add_effect_at_pixel(
                    effect_type_id::sword_slash, static_cast<float>(t.x), static_cast<float>(t.y));
            }
        }

        // Play monster attack sound on frame 1
        if (anim.current_frame == 1 && (e.type() == entity_type::npc || e.type() == entity_type::monster) &&
            (anim.state == entity_anim_state::attack || anim.state == entity_anim_state::attack_move))
        {
            play_monster_sound(e, monster_sound_type::attack);
        }

        // Play monster damage sound (frame varies by monster type)
        // Legacy frame numbers are absolute (already include the 4-frame idle prefix).
        if ((e.type() == entity_type::npc || e.type() == entity_type::monster) &&
            (anim.state == entity_anim_state::damage || anim.state == entity_anim_state::damage_move))
        {
            uint16_t vtype = get_entity_visual_type(e);
            auto sounds = get_monster_sounds(vtype);
            if (sounds.damage.type != 0 && anim.current_frame == sounds.damage_frame)
            {
                play_monster_sound(e, monster_sound_type::damage);
            }
        }

        // Play player/character hurt sound on frame 5 of damage animation (legacy absolute frame)
        if (anim.current_frame == 5 && (e.type() == entity_type::player || e.type() == entity_type::character) &&
            (anim.state == entity_anim_state::damage || anim.state == entity_anim_state::damage_move))
        {
            if (sounds_)
            {
                int ptype = static_cast<int>(e.visual_type());
                character_sound sound = get_hurt_sound(ptype);
                const auto& t = e.transform();
                sounds_->play_character_sound_at(sound, t.x, t.y);
            }
        }

        // Play player/character death sound on frame 7 of dying animation (legacy absolute frame)
        if (anim.current_frame == 7 && (e.type() == entity_type::player || e.type() == entity_type::character) &&
            anim.state == entity_anim_state::dying)
        {
            if (sounds_)
            {
                int ptype = static_cast<int>(e.visual_type());
                character_sound sound = get_death_sound(ptype);
                const auto& t = e.transform();
                sounds_->play_character_sound_at(sound, t.x, t.y);
            }
        }

        // Play monster/NPC death sound at per-type frame during dying animation
        // Legacy frame numbers are absolute (already include idle prefix)
        if ((e.type() == entity_type::npc || e.type() == entity_type::monster) &&
            anim.state == entity_anim_state::dying)
        {
            uint16_t vtype = get_entity_visual_type(e);
            auto sounds = get_monster_sounds(vtype);
            if (sounds.death.type != 0 && anim.current_frame == sounds.death_frame)
            {
                play_monster_sound(e, monster_sound_type::death);
            }
        }

        // Play magic cast sound at frame 1 of magic animation
        if ((anim.state == entity_anim_state::magic || anim.state == entity_anim_state::magic_attack) &&
            anim.current_frame == 1 && !anim.attack_triggered)
        {
            anim.attack_triggered = true;
            if (sounds_)
            {
                sounds_->play_character_sound_at(character_sound::magic_cast, e.transform().x, e.transform().y);
            }
        }

        if (anim.current_frame >= anim.frame_count)
        {
            if (anim.looping)
            {
                anim.current_frame = 0;
            }
            else
            {
                anim.current_frame = anim.frame_count - 1;
                anim.finished = true;
                if (anim.state == entity_anim_state::dying)
                {
                    spdlog::debug("Entity {} dying animation FINISHED at frame {}/{}",
                                  e.id(),
                                  anim.current_frame,
                                  anim.frame_count);
                }
            }
        }
    }
}

void entity_manager::update_movement(entity& e, float delta_time, world& w, bool local_player_combat_mode)
{
    auto& t = e.transform();
    auto& m = e.movement();

    if (!t.moving || !m.can_move)
    {
        return;
    }

    // Update facing direction during movement (from origin toward destination)
    if (t.move_start_x != t.tile_x || t.move_start_y != t.tile_y)
    {
        if (auto dir = calculate_direction(t.move_start_x, t.move_start_y, t.tile_x, t.tile_y))
        {
            t.facing = *dir;
        }
    }

    // Movement timing based on animation frames (legacy Helbreath system)
    // MOVE: 8 frames @ 70ms = 560ms per tile
    // RUN: 8 frames @ 42ms = 336ms per tile
    // ATTACK_MOVE (dash): approach phase is frames 0-3 @ 78ms = 312ms, then character
    // stands at destination for the remaining attack frames (4-12).
    float move_time_ms;
    if (e.animation().state == entity_anim_state::attack_move)
        move_time_ms = 312.0f;
    else
        move_time_ms = m.running ? 336.0f : 560.0f;
    float move_time_sec = move_time_ms / 1000.0f;

    t.move_progress += delta_time / move_time_sec;

    if (t.move_progress >= 1.0f)
    {
        // Arrived at destination - snap to tile position
        t.move_start_x = t.tile_x;
        t.move_start_y = t.tile_y;
        t.x = t.tile_x * tile_width + 16;
        t.y = t.tile_y * tile_height + 16;
        t.move_progress = 0.0f;
        t.moving = false;

        // Check for next waypoint in path
        if (m.path_index < m.path.size())
        {
            auto [next_x, next_y] = m.path[m.path_index++];
            if (w.current_map().is_walkable(next_x, next_y))
            {
                t.move_start_x = t.tile_x;
                t.move_start_y = t.tile_y;
                t.tile_x = next_x;
                t.tile_y = next_y;
                t.moving = true;
            }
            else
            {
                m.path.clear();
                m.path_index = 0;
                {
                    bool combat = (e.id() == local_player_id_) ? local_player_combat_mode
                                                               : (e.has_combat() && e.combat().combat_stance);
                    e.set_action_with_combat_mode(object_action::stop_peace, combat);
                }
            }
        }
        else
        {
            bool reached_destination =
                (m.target_x < 0 || m.target_y < 0) || (t.tile_x == m.target_x && t.tile_y == m.target_y);

            if (e.animation().state == entity_anim_state::attack_move)
            {
                // During attack_move (dash), the movement arrives early while the
                // attack animation is still playing. Don't override it with idle —
                // update_animation will transition to idle when the animation finishes.
            }
            else
            {
                if (reached_destination)
                {
                    m.target_x = -1;
                    m.target_y = -1;
                }
                // For the local player in a movement animation (run/walk), defer
                // the idle transition to avoid a 1-frame animation flash between
                // tiles during continuous movement. The input handler will either
                // continue movement or transition to idle on the next frame.
                if (e.id() == local_player_id_)
                {
                    auto anim_state = e.animation().state;
                    if (anim_state != entity_anim_state::run && anim_state != entity_anim_state::move)
                    {
                        e.set_action_with_combat_mode(object_action::stop_peace, local_player_combat_mode);
                    }
                }
                else
                {
                    bool combat = e.has_combat() && e.combat().combat_stance;
                    e.set_action_with_combat_mode(object_action::stop_peace, combat);
                }
            }
        }
    }
    else
    {
        // Interpolate world position from move_start toward tile (destination)
        int32_t start_x = t.move_start_x * tile_width + 16;
        int32_t start_y = t.move_start_y * tile_height + 16;
        int32_t end_x = t.tile_x * tile_width + 16;
        int32_t end_y = t.tile_y * tile_height + 16;

        t.x = start_x + static_cast<int32_t>((end_x - start_x) * t.move_progress);
        t.y = start_y + static_cast<int32_t>((end_y - start_y) * t.move_progress);
    }
}

void entity_manager::render(
    renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y)
{
    // Collect visible entities and sort by Y position for depth ordering
    std::vector<entity*> visible_entities;

    // Use actual screen dimensions from renderer with margin for off-screen sprites
    // that may extend into view (characters render at -32, -64 from their position)
    static constexpr int32_t render_margin = 128;
    int32_t scr_width = static_cast<int32_t>(rend.scene_width());
    int32_t scr_height = static_cast<int32_t>(rend.scene_height());

    // Extended mode: entities outside fair zone are hidden
    bool extended_cull = rend.current_view_mode() == view_mode::extended;
    sf::IntRect fair;
    if (extended_cull)
        fair = rend.fair_bounds();

    for (auto& [id, e] : entities_)
    {
        if (e->should_remove())
            continue;
        if (!e->sprite().visible)
            continue;

        // In global render mode, skip distance culling - render all entities
        if (global_render_mode_)
        {
            visible_entities.push_back(e.get());
            continue;
        }

        const auto& t = e->transform();
        int32_t screen_x = t.x - camera_x;
        int32_t screen_y = t.y - camera_y;

        // Check if on screen (with margin to prevent pop-in/pop-out)
        if (screen_x >= -render_margin && screen_x < scr_width + render_margin && screen_y >= -render_margin &&
            screen_y < scr_height + render_margin)
        {
            // Extended mode: additionally check fair zone bounds
            if (extended_cull)
            {
                if (screen_x < fair.position.x - 64 || screen_x > fair.position.x + fair.size.x + 64 ||
                    screen_y < fair.position.y - 64 || screen_y > fair.position.y + fair.size.y)
                    continue;
            }
            visible_entities.push_back(e.get());
        }
    }

    // Sort by Y position (entities lower on screen render on top)
    // Dead entities (corpses) render before alive ones at the same Y
    std::sort(visible_entities.begin(),
              visible_entities.end(),
              [](const entity* a, const entity* b)
              {
                  if (a->transform().y != b->transform().y)
                      return a->transform().y < b->transform().y;
                  // Dead entities sort before alive at same Y (render underneath)
                  return !a->is_alive() && b->is_alive();
              });

    // Pass 1: Render all entity sprites, collecting hover state
    struct name_overlay
    {
        const entity* ent;
        int32_t screen_x;
        int32_t screen_y;
        bool hovered;
    };
    std::vector<name_overlay> name_overlays;

    for (entity* e : visible_entities)
    {
        render_entity(rend, sprites, *e, camera_x, camera_y, mouse_x, mouse_y);

        // Collect entities that need name/health overlay
        if (e->has_name() && (e->type() == entity_type::player || e->type() == entity_type::character ||
                              e->type() == entity_type::npc || e->type() == entity_type::monster))
        {
            const auto& t = e->transform();
            int32_t sx = t.x - camera_x;
            int32_t sy = t.y - camera_y;
            bool hovered = is_point_in_entity_sprite(*e, sprites, camera_x, camera_y, mouse_x, mouse_y);
            name_overlays.push_back({e, sx, sy, hovered});
        }
    }

    // Pass 2: Render names (with inline health bars) on top of all sprites
    for (const auto& overlay : name_overlays)
    {
        render_entity_name(rend, *overlay.ent, overlay.screen_x, overlay.screen_y, overlay.hovered);
    }
}

std::vector<entity*> entity_manager::get_visible_entities_sorted(renderer& rend, int32_t camera_x, int32_t camera_y)
{
    std::vector<entity*> visible_entities;

    static constexpr int32_t render_margin = 128;
    int32_t scr_width = static_cast<int32_t>(rend.scene_width());
    int32_t scr_height = static_cast<int32_t>(rend.scene_height());

    // Extended mode: entities outside fair zone are hidden
    bool extended_cull = rend.current_view_mode() == view_mode::extended;
    sf::IntRect fair;
    if (extended_cull)
        fair = rend.fair_bounds();

    for (auto& [id, e] : entities_)
    {
        if (e->should_remove())
            continue;
        if (!e->sprite().visible)
            continue;

        if (global_render_mode_)
        {
            visible_entities.push_back(e.get());
            continue;
        }

        const auto& t = e->transform();
        int32_t screen_x = t.x - camera_x;
        int32_t screen_y = t.y - camera_y;

        if (screen_x >= -render_margin && screen_x < scr_width + render_margin && screen_y >= -render_margin &&
            screen_y < scr_height + render_margin)
        {
            // Extended mode: additionally check fair zone bounds
            if (extended_cull)
            {
                if (screen_x < fair.position.x - 64 || screen_x > fair.position.x + fair.size.x + 64 ||
                    screen_y < fair.position.y - 64 || screen_y > fair.position.y + fair.size.y)
                    continue;
            }
            visible_entities.push_back(e.get());
        }
    }

    // Dead entities (corpses) render before alive ones at the same Y
    std::sort(visible_entities.begin(),
              visible_entities.end(),
              [](const entity* a, const entity* b)
              {
                  if (a->transform().y != b->transform().y)
                      return a->transform().y < b->transform().y;
                  return !a->is_alive() && b->is_alive();
              });

    return visible_entities;
}

void entity_manager::render_single_entity(renderer& rend,
                                          sprite_manager& sprites,
                                          entity& e,
                                          int32_t camera_x,
                                          int32_t camera_y,
                                          int32_t mouse_x,
                                          int32_t mouse_y)
{
    render_entity(rend, sprites, e, camera_x, camera_y, mouse_x, mouse_y);
}

void entity_manager::render_name_overlays(renderer& rend,
                                          sprite_manager& sprites,
                                          const std::vector<entity*>& visible,
                                          int32_t camera_x,
                                          int32_t camera_y,
                                          int32_t mouse_x,
                                          int32_t mouse_y)
{
    for (const entity* e : visible)
    {
        if (!e->has_name())
            continue;
        if (e->type() != entity_type::player && e->type() != entity_type::character && e->type() != entity_type::npc &&
            e->type() != entity_type::monster)
            continue;

        const auto& t = e->transform();
        int32_t sx = t.x - camera_x;
        int32_t sy = t.y - camera_y;
        bool hovered = is_point_in_entity_sprite(*e, sprites, camera_x, camera_y, mouse_x, mouse_y);

        render_entity_name(rend, *e, sx, sy, hovered);
    }
}

void entity_manager::render_entity(renderer& rend,
                                   sprite_manager& sprites,
                                   const entity& e,
                                   int32_t camera_x,
                                   int32_t camera_y,
                                   [[maybe_unused]] int32_t mouse_x,
                                   [[maybe_unused]] int32_t mouse_y)
{
    const auto& t = e.transform();
    const auto& a = e.animation();

    int32_t screen_x = t.x - camera_x;
    int32_t screen_y = t.y - camera_y;

    // Render sprite layers based on entity type (names/health drawn in separate pass)
    if (e.type() == entity_type::effect)
    {
        if (e.has_effect() && e.effect().effect_sprite)
        {
            const auto& eff = e.effect();
            rend.draw_sprite(*eff.effect_sprite, screen_x + eff.offset_x, screen_y + eff.offset_y, eff.effect_frame);
        }
    }
    else if (e.type() == entity_type::npc || e.type() == entity_type::monster)
    {
        render_npc_or_monster(rend, sprites, e, screen_x, screen_y, a);
    }
    else
    {
        render_player_character(rend, sprites, e, screen_x, screen_y, a);
    }
}

void entity_manager::render_player_character(renderer& rend,
                                             sprite_manager& sprites,
                                             const entity& e,
                                             int32_t screen_x,
                                             int32_t screen_y,
                                             const animation_component& a)
{
    const auto& t = e.transform();
    const auto& s = e.sprite();

    int32_t dir = direction_to_sprite_index(t.facing);
    int32_t action = static_cast<int32_t>(e.current_action());

    // For attack_move (dash), map 13 animation frames to 8 sprite frames.
    // Legacy: frames 0-3 = approach, 4-9 held on sprite frame 4, 10-12 = strike (5-7).
    uint8_t sprite_frame = a.current_frame;
    if (a.state == entity_anim_state::attack_move)
    {
        static constexpr uint8_t attack_move_frame_map[] = {0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 5, 6, 7};
        sprite_frame = (a.current_frame < 13) ? attack_move_frame_map[a.current_frame] : 7;
    }

    // Equipment frame index varies by action type (different frame counts per direction)
    int32_t equip_fpd = equipment_frames_per_direction(action);
    int32_t equip_frame = (dir - 1) * equip_fpd + sprite_frame;

    // Calculate sprite IDs using helper functions
    int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);
    bool is_female = (s.gender == 2);

    uint16_t body_id = calculate_body_sprite_id(owner_type, action, dir);
    const sprite* body_spr = sprites.get_sprite_by_id(body_id);

    uint16_t underwear_id = calculate_underwear_sprite_id(is_female, s.underwear_color, action);
    const sprite* underwear_spr = sprites.get_sprite_by_id(underwear_id);

    uint16_t hair_id = calculate_hair_sprite_id(is_female, s.hair_style, action);
    const sprite* hair_spr = sprites.get_sprite_by_id(hair_id);

    // Draw layers: body (skin) first, then underwear on top, then hair
    if (body_spr)
    {
        if (s.alpha < 1.0f)
        {
            rend.draw_sprite_alpha(*body_spr, screen_x, screen_y, sprite_frame, s.alpha);
        }
        else
        {
            rend.draw_sprite(*body_spr, screen_x, screen_y, sprite_frame);
        }
    }

    if (underwear_spr)
    {
        if (s.alpha < 1.0f)
        {
            rend.draw_sprite_alpha(*underwear_spr, screen_x, screen_y, equip_frame, s.alpha);
        }
        else
        {
            rend.draw_sprite(*underwear_spr, screen_x, screen_y, equip_frame);
        }
    }

    // Hair with color tinting (if no helm)
    if (!s.helm_sprite && hair_spr)
    {
        uint8_t hc = std::clamp(s.hair_color, uint8_t(0), uint8_t(15));
        const auto& tint = hair_colors[hc];
        if (tint.r == 0.0f && tint.g == 0.0f && tint.b == 0.0f)
        {
            rend.draw_sprite(*hair_spr, screen_x, screen_y, equip_frame);
        }
        else
        {
            rend.draw_sprite_tinted(*hair_spr, screen_x, screen_y, equip_frame, tint.r, tint.g, tint.b);
        }
    }

    // TODO: Armor, helmet, weapon, shield rendering with dynamic lookup

    // Effect overlay (keep using pre-loaded sprite pointer)
    if (s.effect_sprite)
    {
        rend.draw_sprite(*s.effect_sprite, screen_x, screen_y, a.current_frame);
    }
}

void entity_manager::render_npc_or_monster(renderer& rend,
                                           sprite_manager& sprites,
                                           const entity& e,
                                           int32_t screen_x,
                                           int32_t screen_y,
                                           const animation_component& a)
{
    const auto& t = e.transform();
    const auto& s = e.sprite();

    int32_t dir = direction_to_sprite_index(t.facing);
    int32_t npc_action = action_to_npc_action_index(e.current_action());

    // Get visual type from component or entity
    uint16_t visual_type = get_entity_visual_type(e);

    // Skip rendering if no valid visual type is set
    if (visual_type < npc_sprite_constants::npc_type_offset)
    {
        return;
    }

    // Legacy split rendering: damage/dying animations draw idle sprite for frames 0-3,
    // then the actual damage/dying sprite for frames 4+ (with frame offset -4).
    // See Game.cpp DrawObject_OnDamage lines 10720-10735.
    uint32_t frame = a.current_frame;
    bool is_damage_or_dying = (a.state == entity_anim_state::damage ||
                               a.state == entity_anim_state::damage_move ||
                               a.state == entity_anim_state::dying);
    if (is_damage_or_dying && frame < 4)
    {
        // Frames 0-3: draw idle sprite instead
        npc_action = 0; // idle action
    }
    else if (is_damage_or_dying && frame >= 4)
    {
        // Frames 4+: draw actual damage/dying sprite, re-indexed from 0
        frame -= 4;
    }

    uint16_t sprite_id = calculate_npc_sprite_id(visual_type, npc_action, dir);
    const sprite* npc_spr = sprites.get_sprite_by_id(sprite_id);

    if (npc_spr)
    {
        // Clamp frame to actual sprite frame count to avoid invisible frames
        if (npc_spr->frame_count() > 0 && frame >= npc_spr->frame_count())
        {
            frame = frame % npc_spr->frame_count();
        }

        if (s.alpha < 1.0f)
        {
            rend.draw_sprite_alpha(*npc_spr, screen_x, screen_y, frame, s.alpha);
        }
        else
        {
            rend.draw_sprite(*npc_spr, screen_x, screen_y, frame);
        }
    }

    // Effect overlay
    if (s.effect_sprite)
    {
        uint32_t effect_frame = a.current_frame;
        if (s.effect_sprite->frame_count() > 0 && effect_frame >= s.effect_sprite->frame_count())
        {
            effect_frame = effect_frame % s.effect_sprite->frame_count();
        }
        rend.draw_sprite(*s.effect_sprite, screen_x, screen_y, effect_frame);
    }
}

void entity_manager::render_entity_name(
    renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y, bool is_hovered)
{
    const auto& name = e.name();

    // Only render name if mouse is hovering
    if (is_hovered)
    {
        static constexpr float outline_thickness = 1.5f;
        static constexpr uint32_t name_font_size = 14;
        static constexpr uint32_t sub_font_size = 12;
        static constexpr int32_t line_spacing = 16;
        static const sf::Color outline_color = sf::Color::Black;
        static const sf::Color guild_color = sf::Color(160, 160, 160); // Mid-gray
        static const sf::Color attrib_color = sf::Color(218, 165, 32); // Gold

        // Determine name color based on hostility and pk status
        sf::Color name_color = sf::Color::White;

        if (e.type() == entity_type::player)
        {
            name_color = sf::Color::White;
        }
        else if (e.type() == entity_type::character)
        {
            if (name.pk == pk_status::murderer)
                name_color = sf::Color::Red;
            else if (name.pk == pk_status::criminal)
                name_color = sf::Color(255, 165, 0); // Orange
            else if (name.hostile == hostility::friendly)
                name_color = sf::Color(80, 255, 80); // Green
            else if (name.hostile == hostility::enemy)
                name_color = sf::Color(255, 80, 80); // Red
            else
                name_color = sf::Color(80, 160, 255); // Blue - neutral
        }
        else if (e.type() == entity_type::npc || e.type() == entity_type::monster)
        {
            if (name.hostile == hostility::friendly)
                name_color = sf::Color(80, 255, 80); // Green
            else if (name.hostile == hostility::neutral)
                name_color = sf::Color(80, 160, 255); // Blue
            else
                name_color = sf::Color(255, 80, 80); // Red - enemy (default)
        }

        // Position below entity feet
        int32_t cur_y = screen_y + 10;
        if (e.type() != entity_type::player)
            cur_y += 10;

        // Draw name (centered)
        float name_w = rend.text().measure_width(name.name, name_font_size);
        int32_t name_x = screen_x - static_cast<int32_t>(name_w * 0.5f);
        rend.draw_text_outlined(name.name, name_x, cur_y, name_color, outline_color, name_font_size, outline_thickness);
        cur_y += line_spacing;

        // Health bar (under name, above guild/attributes)
        if (e.has_stats() && e.type() != entity_type::player)
        {
            render_health_bar_inline(rend, e, screen_x, cur_y);
        }

        // Players: show guild below name
        if ((e.type() == entity_type::player || e.type() == entity_type::character) && !name.guild_name.empty())
        {
            auto guild_text = "<" + name.guild_name + ">";
            float guild_w = rend.text().measure_width(guild_text, sub_font_size);
            int32_t guild_x = screen_x - static_cast<int32_t>(guild_w * 0.5f);
            rend.draw_text_outlined(
                guild_text, guild_x, cur_y, guild_color, outline_color, sub_font_size, outline_thickness);
            cur_y += line_spacing;
        }

        // Monsters/NPCs: show attributes below name in gold, then status effects
        if (e.type() == entity_type::monster && e.has_monster())
        {
            const auto& mon = e.monster();

            // Special abilities (gold)
            for (const auto& attr : mon.attributes)
            {
                float attr_w = rend.text().measure_width(attr, sub_font_size);
                int32_t attr_x = screen_x - static_cast<int32_t>(attr_w * 0.5f);
                rend.draw_text_outlined(
                    attr, attr_x, cur_y, attrib_color, outline_color, sub_font_size, outline_thickness);
                cur_y += line_spacing;
            }

            // Status effects (below attributes so toggling doesn't shift text above)
            if (mon.berserked || mon.frozen)
            {
                std::string status;
                if (mon.berserked)
                    status = "Berserked";
                if (mon.frozen)
                {
                    if (!status.empty())
                        status += ", ";
                    status += "Frozen";
                }
                static const sf::Color status_color = sf::Color(255, 120, 120); // Light red
                float status_w = rend.text().measure_width(status, sub_font_size);
                int32_t status_x = screen_x - static_cast<int32_t>(status_w * 0.5f);
                rend.draw_text_outlined(
                    status, status_x, cur_y, status_color, outline_color, sub_font_size, outline_thickness);
            }
        }
    }

    // Always render chat bubble (above entity, not affected by hover)
    if (!name.chat_message.empty())
    {
        // Legacy slide-up animation (Game.cpp:21412-21424)
        float elapsed_ms = name.chat_elapsed * 1000.0f;
        int32_t slide_offset = 0;
        if (elapsed_ms <= 320.0f)
            slide_offset = static_cast<int32_t>(elapsed_ms / 32.0f);
        else if (elapsed_ms <= 352.0f)
            slide_offset = 10;
        else
            slide_offset = 9;

        // Word-wrap: max 20 chars per line, max 3 lines
        std::array<std::string, 3> lines;
        int line_count = 0;
        {
            const auto& msg = name.chat_message;
            size_t pos = 0;
            while (pos < msg.size() && line_count < 3)
            {
                if (line_count == 2)
                {
                    // Last line: take remaining, truncate if needed
                    lines[line_count] = msg.substr(pos, 20);
                    if (pos + 20 < msg.size())
                        lines[line_count] += "..";
                    ++line_count;
                    break;
                }

                // Find break point: prefer space within 20 chars
                size_t remaining = msg.size() - pos;
                if (remaining <= 20)
                {
                    lines[line_count++] = msg.substr(pos);
                    break;
                }

                size_t break_at = 20;
                for (size_t i = 20; i > 0; --i)
                {
                    if (msg[pos + i] == ' ')
                    {
                        break_at = i;
                        break;
                    }
                }
                lines[line_count++] = msg.substr(pos, break_at);
                pos += break_at;
                if (pos < msg.size() && msg[pos] == ' ')
                    ++pos;
            }
        }

        // Draw lines centered above entity, stacking upward
        static constexpr int32_t line_spacing = 16;
        int32_t base_y = screen_y - 65 - slide_offset;
        int32_t top_y = base_y - (line_count - 1) * line_spacing;

        for (int i = 0; i < line_count; ++i)
        {
            float text_w = rend.text().measure_width(lines[i], name.chat_style.size);
            int32_t line_x = screen_x - static_cast<int32_t>(text_w * 0.5f);
            int32_t line_y = top_y + i * line_spacing;
            rend.text().draw(lines[i], line_x, line_y, name.chat_style, name.chat_elapsed);
        }
    }
}

void entity_manager::render_entity_health_bar(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y)
{
    // This overload exists for API compatibility but is no longer called directly.
    // Health bars are now drawn inline via render_entity_name.
    int32_t cur_y = screen_y + 26;
    render_health_bar_inline(rend, e, screen_x, cur_y);
}

void entity_manager::render_health_bar_inline(renderer& rend, const entity& e, int32_t center_x, int32_t& cur_y)
{
    if (!e.has_stats())
        return;
    const auto& stats = e.stats();

    static constexpr int32_t bar_width = 64;
    static constexpr int32_t bar_height = 6;
    static constexpr int32_t border = 1;

    cur_y += 5;
    int32_t bar_x = center_x - bar_width / 2;
    int32_t bar_y = cur_y;

    // Outer border (dark)
    rend.draw_rect(
        bar_x - border, bar_y - border, bar_width + border * 2, bar_height + border * 2, sf::Color(30, 30, 30), true);

    // Background (gray = missing health)
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(80, 80, 80), true);

    // Health fill (red)
    if (stats.max_hp > 0)
    {
        float hp_ratio = std::clamp(static_cast<float>(stats.hp) / static_cast<float>(stats.max_hp), 0.0f, 1.0f);
        int32_t fill_width = static_cast<int32_t>(bar_width * hp_ratio);
        if (fill_width > 0)
        {
            rend.draw_rect(bar_x, bar_y, fill_width, bar_height, sf::Color(200, 30, 30), true);
        }
    }

    // Inner highlight line (subtle shine on top row)
    rend.draw_rect(bar_x, bar_y, bar_width, 1, sf::Color(255, 255, 255, 40), true);

    // Border outline
    rend.draw_rect(bar_x - border,
                   bar_y - border,
                   bar_width + border * 2,
                   bar_height + border * 2,
                   sf::Color(120, 120, 120),
                   false);

    cur_y += bar_height + border * 2 + 3;
}

void entity_manager::load_character_sprites(entity& ent, sprite_manager& sprites)
{
    auto& s = ent.sprite();

    // Action index for idle/stop (action 0)
    static constexpr int32_t action_stop = 0;

    // Calculate sprite IDs using helper functions
    int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);
    bool is_female = (s.gender == 2);

    // Body sprite ID (direction 1 = north, will be changed during rendering)
    uint16_t body_id = calculate_body_sprite_id(owner_type, action_stop, 1);
    s.body_sprite = sprites.get_sprite_by_id(body_id);

    uint16_t underwear_id = calculate_underwear_sprite_id(is_female, s.underwear_color, action_stop);
    s.underwear_sprite = sprites.get_sprite_by_id(underwear_id);

    uint16_t hair_id = calculate_hair_sprite_id(is_female, s.hair_style, action_stop);
    s.hair_sprite = sprites.get_sprite_by_id(hair_id);

    spdlog::debug("Loaded character sprites - body:{} underwear:{} hair:{} (gender:{} skin:{} hair_style:{})",
                  body_id,
                  underwear_id,
                  hair_id,
                  s.gender,
                  s.skin_color,
                  s.hair_style);

    if (!s.body_sprite)
    {
        spdlog::warn("Failed to load body sprite ID {}", body_id);
    }
    if (!s.underwear_sprite)
    {
        spdlog::warn("Failed to load underwear sprite ID {}", underwear_id);
    }
    if (!s.hair_sprite)
    {
        spdlog::warn("Failed to load hair sprite ID {}", hair_id);
    }
}

size_t entity_manager::entity_count_of_type(entity_type type) const
{
    size_t count = 0;
    for (const auto& [id, e] : entities_)
    {
        if (e->type() == type && !e->should_remove())
        {
            ++count;
        }
    }
    return count;
}

void entity_manager::play_footstep_sound(const entity& e, bool running)
{
    if (!sounds_)
        return;

    // Only play footsteps for players and characters
    if (e.type() != entity_type::player && e.type() != entity_type::character)
    {
        return;
    }

    const auto& t = e.transform();
    character_sound sound = running ? character_sound::run_step : character_sound::walk_step;
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

// Lookup monster sounds by visual type. Returns move, attack, and damage sounds.
// Derived from legacy MapData.cpp DEF_OBJECTMOVE, DEF_OBJECTATTACK, DEF_OBJECTDAMAGE blocks.
// Damage frame: older monsters (10-53) use frame 5, newer monsters (55+) use frame 1.
static monster_sounds get_monster_sounds(uint16_t visual_type)
{
    //                      move          attack        damage        dmg_f  death         dth_f
    switch (visual_type)
    {
        // clang-format off
        case 10: return {{'M',   1}, {'M',   2}, {'M',   3}, 5, {'M',   4}, 1};  // Slime
        case 11: return {{'M',  13}, {'M',  14}, {'M',  15}, 5, {'M',  16}, 5};  // Skeleton
        case 12: return {{'M',  33}, {'M',  34}, {'M',  35}, 5, {'M',  36}, 5};  // Stone Golem
        case 13: return {{'M',  41}, {'M',  42}, {'M',  43}, 5, {'M',  44}, 5};  // Cyclops
        case 14: return {{'M',   9}, {'M',  10}, {'M',  11}, 5, {'M',  12}, 5};  // Orc
        case 16: return {{'M',  29}, {'M',  30}, {'M',  31}, 5, {'M',  32}, 5};  // Ant
        case 17: return {{'M',  21}, {'M',  22}, {'M',  23}, 5, {'M',  24}, 5};  // Scorpion
        case 18: return {{'M',  17}, {'M',  18}, {'M',  19}, 5, {'M',  20}, 5};  // Zombie
        case 22: return {{'M',  25}, {'M',  26}, {'M',  27}, 5, {'M',  28}, 5};  // Snake
        case 23: return {{'M',  37}, {'M',  38}, {'M',  39}, 5, {'M',  40}, 5};  // Clay Golem
        case 27: return {{'M',   5}, {'M',   6}, {'M',   7}, 5, {'M',   8}, 5};  // Hell Hound
        case 28: return {{'M',  46}, {'M',  47}, {'M',  48}, 5, {'M',  49}, 5};  // Troll
        case 29: return {{'M',  51}, {'M',  52}, {'M',  53}, 5, {'M',  54}, 5};  // Ogre
        case 30: return {{'M',  55}, {'M',  56}, {'M',  57}, 5, {'M',  58}, 5};  // Liche
        case 31: return {{'M',  59}, {'M',  60}, {'M',  61}, 5, {'M',  62}, 5};  // Demon
        case 32: return {{'M',  63}, {'M',  64}, {'M',  65}, 5, {'M',  66}, 5};  // Unicorn
        case 33: return {{'M',  67}, {'M',  68}, {'M',  69}, 5, {'M',  70}, 5};  // Werewolf
        case 34: return {{  0,   0}, {  0,   0}, {'M',   2}, 5, {'M',   4}, 5};  // Dummy
        case 35: return {{  0,   0}, {  0,   0}, {'M',   2}, 5, {'M',   4}, 5};  // Energy Ball
        case 43: return {{'M',  29}, {'M',  30}, {'M',  31}, 5, {'M',  32}, 5};  // LW Beetle (same as Ant)
        case 44: return {{  0,   0}, {'C',   2}, {  0,   0}, 5, {  0,   0}, 0};  // GHK (no death sound)
        case 45: return {{'M',  63}, {'C',   2}, {  0,   0}, 5, {  0,   0}, 0};  // GHKABS
        case 46: return {{  0,   0}, {'C',   2}, {  0,   0}, 5, {  0,   0}, 0};  // TK (no death sound)
        case 47: return {{'M',  33}, {'M',  34}, {  0,   0}, 5, {'M',  36}, 5};  // Beholder Giant
        case 48: return {{'M',   9}, {'M',  10}, {'M',  11}, 5, {'M',  12}, 5};  // Skeleton Knight (same as Orc)
        case 49: return {{'M',  41}, {'M',  42}, {'M',  43}, 5, {'M',  44}, 5};  // Hell Cyclops (same as Cyclops)
        case 50: return {{'M',   1}, {'C',   1}, {  0,   0}, 5, {'M',  58}, 5};  // Tree Warrior (same as Liche)
        case 52: return {{'M',  37}, {'C',   2}, {'M',  43}, 5, {'M',  44}, 5};  // Gargoyle
        case 53: return {{  0,   0}, {'E',  46}, {'M',   3}, 5, {'M',  39}, 5};  // Beholder
        case 55: return {{'M',  71}, {'M',  75}, {'M',  79}, 1, {'M',  83}, 1};  // Rabbit
        case 56: return {{'M',  72}, {'M',  76}, {'M',  80}, 1, {'M',  84}, 1};  // Cat
        case 57: return {{'M',  73}, {'M',  77}, {'M',  81}, 1, {'M',  85}, 1};  // Giant Frog
        case 58: return {{'M',  87}, {'M',  88}, {'M',  89}, 1, {'M',  90}, 1};  // Mountain Giant
        case 59: return {{'M',  91}, {'M',  92}, {'M',  93}, 1, {'M',  94}, 1};  // Ettin
        case 60: return {{'M',  95}, {'M',  96}, {'M',  97}, 1, {'M',  98}, 1};  // Cannibal Plant
        case 61: return {{'C',  11}, {'M',  38}, {'M',  69}, 1, {'M',  65}, 1};  // Rudolph
        case 62: return {{'M',  87}, {'M',  68}, {'M',  78}, 1, {'M',  94}, 1};  // Dire Boar
        case 63: return {{'M',  25}, {'C',   4}, {'C',  13}, 1, {  0,   0}, 0};  // Frost (no death sound)
        case 65: return {{'M',  33}, {'M',  34}, {'M',  35}, 5, {'M',  36}, 5};  // Ice Golem (same as Stone Golem)
        case 66: return {{  0,   0}, {  0,   0}, {  0,   0}, 0, {'E',   7}, 1};  // Wyvern
        case 70: return {{'M', 130}, {'M', 131}, {'M', 128}, 1, {'M', 129}, 1};  // Dragon
        case 71: return {{'M', 117}, {'M', 119}, {'M', 116}, 1, {'M', 129}, 1};  // Centaurus
        case 72: return {{'M', 114}, {'M', 115}, {'M', 112}, 1, {'M', 113}, 1};  // Claw Turtle
        case 73: return {{'M', 106}, {'M', 107}, {  0,   0}, 0, {'M', 105}, 1};  // Fire Wyvern
        case 74: return {{'M',  87}, {'M', 100}, {'M', 101}, 1, {'M',  98}, 1};  // Giant Crayfish
        case 75: return {{'M', 126}, {'M', 127}, {'M', 124}, 1, {'M', 125}, 1};  // Giant Lizard
        case 76: return {{'M', 122}, {'M', 123}, {'M', 120}, 1, {'M', 121}, 1};  // Giant Tree
        case 77: return {{'M',  91}, {'M',  78}, {'M',  89}, 1, {'M',  90}, 1};  // Master Mage Orc
        case 78: return {{'M',  46}, {'M', 104}, {'M', 102}, 1, {'M', 103}, 1};  // Minaus
        case 79: return {{'M', 134}, {'M', 135}, {'M', 132}, 1, {'M', 133}, 1};  // Nizie
        case 80: return {{'M', 110}, {'M', 111}, {'M', 108}, 1, {'M', 109}, 1};  // Tentacle
        case 81: return {{'M', 136}, {'M', 137}, {'M', 138}, 1, {'M', 139}, 1};  // Abaddon
        case 82: return {{'M', 149}, {  0,   0}, {'M', 116}, 1, {'M', 129}, 1};  // Sorceress
        case 83: return {{'M', 142}, {'M', 140}, {'M', 143}, 1, {'M', 141}, 1};  // ATK
        case 84: return {{'C',  10}, {'C',   8}, {'C',   7}, 1, {'M', 150}, 1};  // Master Elf
        case 85: return {{'M', 147}, {'M', 145}, {'M', 148}, 1, {'M', 146}, 1};  // DSK
        case 86: return {{'M', 151}, {'M', 151}, {  0,   0}, 0, {'M', 152}, 1};  // Heavy Battle Tank
        case 87: return {{'M', 153}, {'M', 153}, {  0,   0}, 0, {'M', 154}, 1};  // Crossbow Turret
        case 88: return {{'M',  87}, {'M',  78}, {'M', 144}, 1, {'M',  94}, 1};  // Barbarian
        case 89: return {{'M', 155}, {'M', 155}, {  0,   0}, 0, {'M', 156}, 1};  // Cannon Turret
        case 91: return {{  0,   0}, {  0,   0}, {  0,   0}, 0, {  0,   0}, 0};  // Gate (no death sound)
        case 95: return {{'M', 149}, {'M', 149}, {'M', 129}, 1, {'E',   4}, 1};  // Willowisp
        case 96: return {{'E',   6}, {'M', 149}, {'M', 129}, 1, {'M', 129}, 1};  // Air Elemental
        case 97: return {{'E',   9}, {'E',   1}, {'E',  15}, 1, {'M',  58}, 1};  // Fire Elemental
        case 99: return {{'E',   6}, {'M', 149}, {'M', 129}, 1, {'M', 129}, 1};  // Ice Elemental
        // clang-format on

    default:
        return {{0, 0}, {'C', 2}, {0, 0}, 0, {'C', 15}, 5};
    }
}

void entity_manager::play_monster_sound(const entity& e, monster_sound_type sound_type)
{
    if (!sounds_)
        return;
    if (e.type() != entity_type::npc && e.type() != entity_type::monster)
        return;

    uint16_t vtype = get_entity_visual_type(e);
    if (vtype == 0)
        return;

    auto sounds = get_monster_sounds(vtype);
    monster_sound_entry entry;
    switch (sound_type)
    {
    case monster_sound_type::move:
        entry = sounds.move;
        break;
    case monster_sound_type::attack:
        entry = sounds.attack;
        break;
    case monster_sound_type::damage:
        entry = sounds.damage;
        break;
    case monster_sound_type::death:
        entry = sounds.death;
        break;
    }

    if (entry.type == 0)
        return;

    const auto& t = e.transform();
    sounds_->play_sound_at(entry.type, entry.num, t.x, t.y);
}

} // namespace hb
