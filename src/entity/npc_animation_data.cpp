#include "entity/npc_animation_data.hpp"

namespace
{

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
static constexpr int npc_type_last = 119; // 70-99 v3.82 monsters, 100-112 Olympia NPCs
static constexpr int npc_type_count = npc_type_last - npc_type_first + 1;

// Table populated once, indexed by [visual_type - 10][legacy_action]
static hb::npc_frame_entry npc_frame_table[npc_type_count][legacy_action_count];
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
    set(10, act_dying, 7, 240);  // 4 idle prefix + 4 dying frames

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

    // === v3.82 monsters (types 70-99) ===

    // Dragon (70)
    set(70, act_stop, 7, 250);
    set_ft(70, act_move, 90);
    set(70, act_attack, 7, 100);
    set(70, act_damage, 7, 100);
    set(70, act_dying, 10, 150);

    // Centaurus (71)
    set(71, act_stop, 7, 250);
    set_ft(71, act_move, 90);
    set(71, act_attack, 7, 100);
    set(71, act_damage, 7, 100);
    set(71, act_dying, 10, 150);

    // Claw Turtle (72)
    set(72, act_stop, 7, 100);
    set_ft(72, act_move, 90);
    set(72, act_attack, 7, 80);
    set(72, act_damage, 7, 60);
    set(72, act_dying, 10, 65);

    // Fire Wyvern (73)
    set(73, act_stop, 7, 250);
    set_ft(73, act_move, 90);
    set(73, act_attack, 7, 100);
    set(73, act_damage, 7, 100);
    set(73, act_dying, 10, 150);

    // Giant Crayfish (74)
    set(74, act_stop, 7, 250);
    set_ft(74, act_move, 90);
    set(74, act_attack, 7, 100);
    set(74, act_damage, 7, 100);
    set(74, act_dying, 10, 150);

    // Giant Lizard (75)
    set(75, act_stop, 7, 250);
    set_ft(75, act_move, 90);
    set(75, act_attack, 7, 100);
    set(75, act_damage, 7, 100);
    set(75, act_dying, 10, 150);

    // Giant Tree (76)
    set(76, act_stop, 7, 250);
    set_ft(76, act_move, 90);
    set(76, act_attack, 7, 100);
    set(76, act_damage, 7, 100);
    set(76, act_dying, 10, 150);

    // Master Mage Orc (77)
    set(77, act_stop, 7, 250);
    set_ft(77, act_move, 90);
    set(77, act_attack, 7, 100);
    set(77, act_damage, 7, 100);
    set(77, act_dying, 10, 150);

    // Minaus (78)
    set(78, act_stop, 7, 250);
    set_ft(78, act_move, 90);
    set(78, act_attack, 7, 100);
    set(78, act_damage, 7, 100);
    set(78, act_dying, 10, 150);

    // Nizie (79)
    set(79, act_stop, 7, 250);
    set_ft(79, act_move, 90);
    set(79, act_attack, 7, 100);
    set(79, act_damage, 7, 100);
    set(79, act_dying, 10, 150);

    // Tentacle (80)
    set(80, act_stop, 7, 250);
    set_ft(80, act_move, 90);
    set(80, act_attack, 7, 100);
    set(80, act_damage, 7, 100);
    set(80, act_dying, 10, 150);

    // Abaddon (81)
    set(81, act_stop, 7, 250);
    set_ft(81, act_move, 90);
    set(81, act_attack, 7, 100);
    set(81, act_damage, 7, 100);
    set(81, act_dying, 18, 180);

    // Sorceress (82)
    set(82, act_stop, 7, 250);
    set_ft(82, act_move, 90);
    set(82, act_attack, 7, 100);
    set(82, act_damage, 7, 100);
    set(82, act_dying, 10, 180);

    // ATK (83)
    set(83, act_stop, 7, 250);
    set_ft(83, act_move, 90);
    set(83, act_attack, 7, 100);
    set(83, act_damage, 7, 100);
    set(83, act_dying, 10, 180);

    // Master Elf (84)
    set(84, act_stop, 7, 250);
    set_ft(84, act_move, 90);
    set(84, act_attack, 7, 100);
    set(84, act_damage, 7, 100);
    set(84, act_dying, 10, 180);

    // DSK (85)
    set(85, act_stop, 7, 250);
    set_ft(85, act_move, 90);
    set(85, act_attack, 7, 100);
    set(85, act_damage, 7, 100);
    set(85, act_dying, 10, 180);

    // Heavy Battle Tank (86)
    set(86, act_stop, 7, 250);
    set_ft(86, act_move, 90);
    set(86, act_attack, 7, 100);
    set(86, act_damage, 7, 100);
    set(86, act_dying, 10, 180);

    // Crossbow Turret (87)
    set(87, act_stop, 7, 250);
    set_ft(87, act_move, 90);
    set(87, act_attack, 7, 100);
    set(87, act_damage, 7, 100);
    set(87, act_dying, 10, 180);

    // Barbarian (88)
    set(88, act_stop, 7, 250);
    set_ft(88, act_move, 90);
    set(88, act_attack, 7, 100);
    set(88, act_damage, 7, 100);
    set(88, act_dying, 10, 180);

    // Cannon Turret (89)
    set(89, act_stop, 7, 250);
    set_ft(89, act_move, 90);
    set(89, act_attack, 7, 100);
    set(89, act_damage, 7, 100);
    set(89, act_dying, 10, 180);

    // Scarecrow (90) — idle only
    set(90, act_stop, 7, 250);

    // Gate (91)
    set(91, act_stop, 7, 250);
    set(91, act_damage, 7, 100);
    set(91, act_dying, 10, 180);

    // Olympia NPCs (100-112): Scarecrow and the chests only stand there
    set(100, act_stop, 7, 250);
    set(109, act_stop, 0, 1000);
    set(110, act_stop, 0, 1000);
    set(111, act_stop, 0, 1000);

    // Willowisp (95)
    set(95, act_stop, 21, 150);
    set(95, act_move, 21, 100);
    set(95, act_attack, 21, 100);
    set(95, act_damage, 21, 50);
    set(95, act_dying, 18, 80);

    // Air Elemental (96)
    set(96, act_stop, 7, 150);
    set(96, act_move, 7, 100);
    set(96, act_attack, 7, 100);
    set(96, act_damage, 7, 50);
    set(96, act_dying, 7, 10);

    // Fire Elemental (97)
    set(97, act_stop, 7, 150);
    set(97, act_move, 7, 100);
    set(97, act_attack, 7, 100);
    set(97, act_damage, 7, 50);
    set(97, act_dying, 7, 10);

    // Earth Elemental (98)
    set(98, act_stop, 7, 150);
    set(98, act_move, 7, 100);
    set(98, act_attack, 7, 100);
    set(98, act_damage, 7, 50);
    set(98, act_dying, 7, 10);

    // Ice Elemental (99)
    set(99, act_stop, 9, 150);
    set(99, act_move, 9, 100);
    set(99, act_attack, 9, 100);
    set(99, act_damage, 9, 50);
    set(99, act_dying, 9, 10);

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
int anim_state_to_legacy_action(hb::entity_anim_state state)
{
    switch (state)
    {
    case hb::entity_anim_state::stop:
        return act_stop;
    case hb::entity_anim_state::move:
        return act_move;
    case hb::entity_anim_state::run:
        return act_run;
    case hb::entity_anim_state::attack:
        return act_attack;
    case hb::entity_anim_state::attack_move:
        return act_attackmove;
    case hb::entity_anim_state::damage:
        return act_damage;
    case hb::entity_anim_state::damage_move:
        return act_damagemove;
    case hb::entity_anim_state::magic:
    case hb::entity_anim_state::magic_attack:
        return act_magic;
    case hb::entity_anim_state::get_item:
        return act_getitem;
    case hb::entity_anim_state::dying:
        return act_dying;
    default:
        return act_stop;
    }
}

} // anonymous namespace

namespace hb
{

std::optional<npc_frame_entry> get_npc_frame_data(uint16_t visual_type, entity_anim_state state)
{
    if (visual_type < npc_type_first || visual_type > npc_type_last)
        return std::nullopt;

    init_npc_frame_table();

    int type_idx = visual_type - npc_type_first;
    int action_idx = anim_state_to_legacy_action(state);
    if (action_idx < 0 || action_idx >= legacy_action_count)
        return std::nullopt;

    const auto& entry = npc_frame_table[type_idx][action_idx];
    if (entry.max_frame < 0 && entry.frame_time_ms < 0)
        return std::nullopt;

    return entry;
}

} // namespace hb
