#include "gameplay/game_state.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"
#include "assets/sprite_manager.hpp"
#include "assets/tile_sprite_registry.hpp"
#include "core/constants.hpp"
#include "world/tile.hpp"
#include "core/config.hpp"
#include "core/direction_utils.hpp"
#include "ui/dialog_manager.hpp"
#include "ui/managed_dialog.hpp"
#include "ui/dialogs/icon_panel_dialog.hpp"
#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include "ui/dialogs/system_menu_dialog.hpp"
#include "chat/chat_message.hpp"
#include "ui/cursor.hpp"
#include <sodium/crypto_hash_sha256.h>
#include <spdlog/spdlog.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#endif

#include "debug/debug_stats.hpp"
#include "debug/mcp_server/debug_server.hpp"

namespace hb
{

namespace
{

// Effect PAK loading table
// Matches legacy Game.cpp MakeEffectSpr() calls (lines 4213-4227)
// Each entry: {pak_name, global_start, sprite_count, local_start}
// Global effect sprite array: m_pEffectSpr[0..86]
struct effect_pak_entry
{
    const char* pak_name;
    uint8_t global_start; // First index in global effect sprite array
    uint8_t sprite_count; // Number of sprites to load from this PAK
    uint8_t local_start;  // First sprite index within the PAK (usually 0)
};

static constexpr std::array effect_paks = {
    effect_pak_entry{"effect", 0, 10, 0},     // effect.pak:     global 0-9
    effect_pak_entry{"effect2", 10, 3, 0},    // effect2.pak:    global 10-12
    effect_pak_entry{"effect3", 13, 6, 0},    // effect3.pak:    global 13-18
    effect_pak_entry{"effect4", 19, 5, 0},    // effect4.pak:    global 19-23
    effect_pak_entry{"effect5", 24, 7, 1},    // effect5.pak:    global 24-30 (local starts at 1)
    effect_pak_entry{"CruEffect1", 31, 9, 0}, // CruEffect1.pak: global 31-39
    effect_pak_entry{"effect6", 40, 5, 0},    // effect6.pak:    global 40-44
    effect_pak_entry{"effect7", 45, 12, 0},   // effect7.pak:    global 45-56
    effect_pak_entry{"effect8", 57, 9, 0},    // effect8.pak:    global 57-65
    effect_pak_entry{"effect9", 66, 21, 0},   // effect9.pak:    global 66-86
    effect_pak_entry{"effect10", 87, 2, 0},   // effect10.pak:   global 87-88
    effect_pak_entry{"effect11", 89, 14, 0},  // Effect11.pak:   global 89-102
    effect_pak_entry{"effect11s", 104, 1, 0}, // effect11s.pak:  global 104
    effect_pak_entry{"effect13", 105, 3, 0},  // Effect13.pak:   global 105-107
    effect_pak_entry{"effect12", 148, 4, 0},  // Effect12.pak:   global 148-151 (slates auras)
};

// Monster/NPC PAK loading table
// Matches legacy Game.cpp MakeSprite() calls: MakeSprite("slm", 1220 + 7*8*0, 40, TRUE)
// Each entry: {pak_name, sprite_id_start, sprite_count}
// Formula: sprite_id = npc_base + (type - 10) * 56 + action * 8 + (dir - 1)
// (npc_base is 20000, not the legacy 1220: see npc_sprite_constants in entity_manager.cpp)
struct monster_pak_entry
{
    const char* pak_name;
    uint16_t sprite_id;
    uint16_t sprite_count;
};

static constexpr uint16_t npc_base = 20000;
static constexpr uint16_t npc_stride = 56; // 7 actions * 8 directions

// All monster/NPC PAK files from legacy loading (Game.cpp lines 3836-3930)
static constexpr std::array monster_paks = {
    monster_pak_entry{"slm", static_cast<uint16_t>(npc_base + npc_stride * 0), 40},       // Slime (Type: 10)
    monster_pak_entry{"ske", static_cast<uint16_t>(npc_base + npc_stride * 1), 40},       // Skeleton (Type: 11)
    monster_pak_entry{"Gol", static_cast<uint16_t>(npc_base + npc_stride * 2), 40},       // Stone-Golem (Type: 12)
    monster_pak_entry{"Cyc", static_cast<uint16_t>(npc_base + npc_stride * 3), 40},       // Cyclops (Type: 13)
    monster_pak_entry{"Orc", static_cast<uint16_t>(npc_base + npc_stride * 4), 40},       // Orc (Type: 14)
    monster_pak_entry{"Shopkpr", static_cast<uint16_t>(npc_base + npc_stride * 5), 8},    // ShopKeeper (Type: 15)
    monster_pak_entry{"Ant", static_cast<uint16_t>(npc_base + npc_stride * 6), 40},       // Giant-Ant (Type: 16)
    monster_pak_entry{"Scp", static_cast<uint16_t>(npc_base + npc_stride * 7), 40},       // Scorpion (Type: 17)
    monster_pak_entry{"Zom", static_cast<uint16_t>(npc_base + npc_stride * 8), 40},       // Zombie (Type: 18)
    monster_pak_entry{"Gandlf", static_cast<uint16_t>(npc_base + npc_stride * 9), 8},     // Gandalf (Type: 19)
    monster_pak_entry{"Howard", static_cast<uint16_t>(npc_base + npc_stride * 10), 8},    // Howard (Type: 20)
    monster_pak_entry{"Guard", static_cast<uint16_t>(npc_base + npc_stride * 11), 40},    // Guard (Type: 21)
    monster_pak_entry{"Amp", static_cast<uint16_t>(npc_base + npc_stride * 12), 40},      // Amphis (Type: 22)
    monster_pak_entry{"Cla", static_cast<uint16_t>(npc_base + npc_stride * 13), 40},      // Clay-Golem (Type: 23)
    monster_pak_entry{"tom", static_cast<uint16_t>(npc_base + npc_stride * 14), 8},       // Tom (Type: 24)
    monster_pak_entry{"William", static_cast<uint16_t>(npc_base + npc_stride * 15), 8},   // William (Type: 25)
    monster_pak_entry{"Kennedy", static_cast<uint16_t>(npc_base + npc_stride * 16), 8},   // Kennedy (Type: 26)
    monster_pak_entry{"Helb", static_cast<uint16_t>(npc_base + npc_stride * 17), 40},     // Hellbound (Type: 27)
    monster_pak_entry{"Troll", static_cast<uint16_t>(npc_base + npc_stride * 18), 40},    // Troll (Type: 28)
    monster_pak_entry{"Orge", static_cast<uint16_t>(npc_base + npc_stride * 19), 40},     // Ogre (Type: 29)
    monster_pak_entry{"Liche", static_cast<uint16_t>(npc_base + npc_stride * 20), 40},    // Liche (Type: 30)
    monster_pak_entry{"Demon", static_cast<uint16_t>(npc_base + npc_stride * 21), 40},    // Demon (Type: 31)
    monster_pak_entry{"Unicorn", static_cast<uint16_t>(npc_base + npc_stride * 22), 40},  // Unicorn (Type: 32)
    monster_pak_entry{"WereWolf", static_cast<uint16_t>(npc_base + npc_stride * 23), 40}, // WereWolf (Type: 33)
    monster_pak_entry{"Dummy", static_cast<uint16_t>(npc_base + npc_stride * 24), 40},    // Dummy (Type: 34)
    // Type 35 (Energy-Ball) uses Effect5.pak - skipped, handled separately if needed
    monster_pak_entry{"GT-Arrow", static_cast<uint16_t>(npc_base + npc_stride * 26), 40}, // Arrow-GuardTower (Type: 36)
    monster_pak_entry{
        "GT-Cannon", static_cast<uint16_t>(npc_base + npc_stride * 27), 40}, // Cannon-GuardTower (Type: 37)
    monster_pak_entry{
        "ManaCollector", static_cast<uint16_t>(npc_base + npc_stride * 28), 40},           // Mana Collector (Type: 38)
    monster_pak_entry{"Detector", static_cast<uint16_t>(npc_base + npc_stride * 29), 40},  // Detector (Type: 39)
    monster_pak_entry{"ESG", static_cast<uint16_t>(npc_base + npc_stride * 30), 40},       // ESG (Type: 40)
    monster_pak_entry{"GMG", static_cast<uint16_t>(npc_base + npc_stride * 31), 40},       // GMG (Type: 41)
    monster_pak_entry{"ManaStone", static_cast<uint16_t>(npc_base + npc_stride * 32), 40}, // ManaStone (Type: 42)
    monster_pak_entry{"LWB", static_cast<uint16_t>(npc_base + npc_stride * 33), 40},    // Light War Beetle (Type: 43)
    monster_pak_entry{"GHK", static_cast<uint16_t>(npc_base + npc_stride * 34), 40},    // God's Hand Knight (Type: 44)
    monster_pak_entry{"GHKABS", static_cast<uint16_t>(npc_base + npc_stride * 35), 40}, // GHK + Battle Steed (Type: 45)
    monster_pak_entry{"TK", static_cast<uint16_t>(npc_base + npc_stride * 36), 40},     // Temple Knight (Type: 46)
    monster_pak_entry{"BG", static_cast<uint16_t>(npc_base + npc_stride * 37), 40},     // Battle Golem (Type: 47)
    monster_pak_entry{"Stalker", static_cast<uint16_t>(npc_base + npc_stride * 38), 40},   // Stalker (Type: 48)
    monster_pak_entry{"Hellclaw", static_cast<uint16_t>(npc_base + npc_stride * 39), 40},  // Hellclaw (Type: 49)
    monster_pak_entry{"Tigerworm", static_cast<uint16_t>(npc_base + npc_stride * 40), 40}, // Tigerworm (Type: 50)
    monster_pak_entry{"Catapult", static_cast<uint16_t>(npc_base + npc_stride * 41), 40},  // Catapult (Type: 51)
    monster_pak_entry{"Gagoyle", static_cast<uint16_t>(npc_base + npc_stride * 42), 40},   // Gargoyle (Type: 52)
    monster_pak_entry{"Beholder", static_cast<uint16_t>(npc_base + npc_stride * 43), 40},  // Beholder (Type: 53)
    monster_pak_entry{"DarkElf", static_cast<uint16_t>(npc_base + npc_stride * 44), 40},   // Dark-Elf (Type: 54)
    monster_pak_entry{"Bunny", static_cast<uint16_t>(npc_base + npc_stride * 45), 40},     // Bunny (Type: 55)
    monster_pak_entry{"Cat", static_cast<uint16_t>(npc_base + npc_stride * 46), 40},       // Cat (Type: 56)
    monster_pak_entry{"GiantFrog", static_cast<uint16_t>(npc_base + npc_stride * 47), 40}, // GiantFrog (Type: 57)
    monster_pak_entry{"MTGiant", static_cast<uint16_t>(npc_base + npc_stride * 48), 40},   // Mountain Giant (Type: 58)
    monster_pak_entry{"Ettin", static_cast<uint16_t>(npc_base + npc_stride * 49), 40},     // Ettin (Type: 59)
    monster_pak_entry{"CanPlant", static_cast<uint16_t>(npc_base + npc_stride * 50), 40},  // Cannibal Plant (Type: 60)
    monster_pak_entry{"Rudolph", static_cast<uint16_t>(npc_base + npc_stride * 51), 40},   // Rudolph (Type: 61)
    monster_pak_entry{"DireBoar", static_cast<uint16_t>(npc_base + npc_stride * 52), 40},  // Dire Boar (Type: 62)
    monster_pak_entry{"frost", static_cast<uint16_t>(npc_base + npc_stride * 53), 40},     // Frost (Type: 63)
    monster_pak_entry{"Crop", static_cast<uint16_t>(npc_base + npc_stride * 54), 40},      // Crop (Type: 64)
    monster_pak_entry{"IceGolem", static_cast<uint16_t>(npc_base + npc_stride * 55), 40},  // Ice Golem (Type: 65)
    monster_pak_entry{"Wyvern", static_cast<uint16_t>(npc_base + npc_stride * 56), 40},    // Wyvern (Type: 66)
    monster_pak_entry{"McGaffin", static_cast<uint16_t>(npc_base + npc_stride * 57), 16},  // McGaffin (Type: 67)
    monster_pak_entry{"Perry", static_cast<uint16_t>(npc_base + npc_stride * 58), 16},     // Perry (Type: 68)
    monster_pak_entry{"Devlin", static_cast<uint16_t>(npc_base + npc_stride * 59), 16},    // Devlin (Type: 69)
    // v3.82 monsters (types 70-91, numbering of the Korean 3.82 server) and the Olympia NPCs
    // (types 100-112: our own numbering, the packs come from the Helbreath Olympia client)
    monster_pak_entry{"Barlog", static_cast<uint16_t>(npc_base + npc_stride * 60), 40},   // Barlog (Type: 70)
    monster_pak_entry{"Centaurus", static_cast<uint16_t>(npc_base + npc_stride * 61), 40}, // Centaurus (Type: 71)
    monster_pak_entry{"Clawturtle", static_cast<uint16_t>(npc_base + npc_stride * 62), 40}, // Claw-Turtle (Type: 72)
    monster_pak_entry{"FireWyvern", static_cast<uint16_t>(npc_base + npc_stride * 63), 24}, // Fire-Wyvern (Type: 73)
    monster_pak_entry{"GiantCrayfish", static_cast<uint16_t>(npc_base + npc_stride * 64), 40}, // Giant-Cray-Fish (Type: 74)
    monster_pak_entry{"GiantLizard", static_cast<uint16_t>(npc_base + npc_stride * 65), 40}, // Lizard (Type: 75)
    monster_pak_entry{"GiantPlant", static_cast<uint16_t>(npc_base + npc_stride * 66), 40}, // Giant-Plant (Type: 76)
    monster_pak_entry{"MasterMageOrc", static_cast<uint16_t>(npc_base + npc_stride * 67), 40}, // Master-Mage-Orc (Type: 77)
    monster_pak_entry{"Minotaurs", static_cast<uint16_t>(npc_base + npc_stride * 68), 40}, // Minotaurus (Type: 78)
    monster_pak_entry{"Nizie", static_cast<uint16_t>(npc_base + npc_stride * 69), 40},    // Nizie (Type: 79)
    monster_pak_entry{"Tentocle", static_cast<uint16_t>(npc_base + npc_stride * 70), 40}, // Tentocle (Type: 80)
    monster_pak_entry{"yspro", static_cast<uint16_t>(npc_base + npc_stride * 71), 32},    // Abaddon (Type: 81)
    monster_pak_entry{"Sorceress", static_cast<uint16_t>(npc_base + npc_stride * 72), 40}, // Sorceress (Type: 82)
    monster_pak_entry{"TPKnight", static_cast<uint16_t>(npc_base + npc_stride * 73), 40}, // ATK (Temple Knight) (Type: 83)
    monster_pak_entry{"ElfMaster", static_cast<uint16_t>(npc_base + npc_stride * 74), 40}, // Elf Master (Type: 84)
    monster_pak_entry{"DarkKnight", static_cast<uint16_t>(npc_base + npc_stride * 75), 40}, // DSK (Dark Knight) (Type: 85)
    monster_pak_entry{"HBTank", static_cast<uint16_t>(npc_base + npc_stride * 76), 32},   // HBT (Heavy Battle Tank) (Type: 86)
    monster_pak_entry{"CBTurret", static_cast<uint16_t>(npc_base + npc_stride * 77), 32}, // CT (Crossbow Turret) (Type: 87)
    monster_pak_entry{"Babarian", static_cast<uint16_t>(npc_base + npc_stride * 78), 40}, // Barbarian (Type: 88)
    monster_pak_entry{"ACannon", static_cast<uint16_t>(npc_base + npc_stride * 79), 32},  // AGC (Cannon) (Type: 89)
    monster_pak_entry{"Gail", static_cast<uint16_t>(npc_base + npc_stride * 80), 8},      // Gail (Type: 90)
    monster_pak_entry{"Gate", static_cast<uint16_t>(npc_base + npc_stride * 81), 24},     // Gate (Type: 91)
    monster_pak_entry{"Scarecrow", static_cast<uint16_t>(npc_base + npc_stride * 90), 40}, // Scarecrow (Olympia) (Type: 100)
    monster_pak_entry{"Ghost", static_cast<uint16_t>(npc_base + npc_stride * 91), 40},    // Ghost (Olympia) (Type: 101)
    monster_pak_entry{"lgn_Princess", static_cast<uint16_t>(npc_base + npc_stride * 92), 8}, // Princess (Olympia) (Type: 102)
    monster_pak_entry{"Bat", static_cast<uint16_t>(npc_base + npc_stride * 93), 8},       // Bat (Olympia) (Type: 103)
    monster_pak_entry{"eventofficer", static_cast<uint16_t>(npc_base + npc_stride * 94), 8}, // Event Officer (Olympia) (Type: 104)
    monster_pak_entry{"battleofficer", static_cast<uint16_t>(npc_base + npc_stride * 95), 8}, // Battle Officer (Olympia) (Type: 105)
    monster_pak_entry{"Guard_Archer", static_cast<uint16_t>(npc_base + npc_stride * 96), 40}, // Guard Archer (Olympia) (Type: 106)
    monster_pak_entry{"Guard_Axe", static_cast<uint16_t>(npc_base + npc_stride * 97), 40}, // Guard Axe (Olympia) (Type: 107)
    monster_pak_entry{"Guard_Sword", static_cast<uint16_t>(npc_base + npc_stride * 98), 40}, // Guard Sword (Olympia) (Type: 108)
    monster_pak_entry{"ChestBronze", static_cast<uint16_t>(npc_base + npc_stride * 99), 1}, // Bronze Chest (Olympia) (Type: 109)
    monster_pak_entry{"ChestSilver", static_cast<uint16_t>(npc_base + npc_stride * 100), 1}, // Silver Chest (Olympia) (Type: 110)
    monster_pak_entry{"ChestGold", static_cast<uint16_t>(npc_base + npc_stride * 101), 1}, // Gold Chest (Olympia) (Type: 111)
    monster_pak_entry{"BBeholder", static_cast<uint16_t>(npc_base + npc_stride * 102), 40}, // Black Beholder (Olympia) (Type: 112)
};

const char* weather_to_str(weather_type w)
{
    switch (w)
    {
    case weather_type::clear:
        return "Clear";
    case weather_type::light_rain:
        return "Light Rain";
    case weather_type::medium_rain:
        return "Rain";
    case weather_type::heavy_rain:
        return "Heavy Rain";
    case weather_type::light_snow:
        return "Light Snow";
    case weather_type::medium_snow:
        return "Snow";
    case weather_type::heavy_snow:
        return "Heavy Snow";
    default:
        return "Unknown";
    }
}

const char* time_to_str(time_of_day t)
{
    switch (t)
    {
    case time_of_day::dawn:
        return "Dawn";
    case time_of_day::morning:
        return "Morning";
    case time_of_day::noon:
        return "Noon";
    case time_of_day::afternoon:
        return "Afternoon";
    case time_of_day::dusk:
        return "Dusk";
    case time_of_day::night:
        return "Night";
    case time_of_day::midnight:
        return "Midnight";
    default:
        return "Unknown";
    }
}

} // anonymous namespace

bool game_state_manager::initialize(renderer& rend, audio& aud)
{
    audio_ = &aud;
    renderer_ = &rend;

    // Synchronous critical initialization
    if (!network_.initialize())
    {
        spdlog::error("Failed to initialize network");
        return false;
    }
    entities_.initialize();
    sprites_.initialize("assets/");

    // Apply debug stats visibility from config
    debug::debug_stats::instance().set_visible(config::instance().video().show_debug_stats);
    debug::debug_stats::instance().set_entity_info_visible(config::instance().video().show_entity_info);

    // Queue async initialization steps
    init_steps_.clear();
    init_step_index_ = 0;

    init_steps_.push_back({"Loading interface sprites...",
                           [this]()
                           {
                               if (sprites_.load_pak("interface", "sprites/interface.pak"))
                               {
                                   spdlog::info("Loaded interface.pak");
                                   sprites_.store_sprite_at_id(0, "interface", 0);
                               }
                           }});

    init_steps_.push_back({"Loading dialog sprites...",
                           [this]()
                           {
                               if (sprites_.load_pak("interface2", "sprites/interface2.pak"))
                                   spdlog::info("Loaded interface2.pak");
                           }});
    init_steps_.push_back({"Loading dialog sprites...",
                           [this]()
                           {
                               if (sprites_.load_pak("New-Dialog", "sprites/New-Dialog.pak"))
                               {
                                   spdlog::info("Loaded New-Dialog.pak");
                                   sprites_.store_sprite_at_id(51, "New-Dialog", 0);
                                   sprites_.store_sprite_at_id(52, "New-Dialog", 1);
                                   sprites_.store_sprite_at_id(54, "New-Dialog", 2);
                               }
                           }});
    init_steps_.push_back({"Loading dialog sprites...",
                           [this]()
                           {
                               if (sprites_.load_pak("LoginDialog", "sprites/LoginDialog.pak"))
                               {
                                   spdlog::info("Loaded LoginDialog.pak");
                                   sprites_.store_sprite_at_id(53, "LoginDialog", 0);
                               }
                           }});
    init_steps_.push_back({"Loading dialog sprites...",
                           [this]()
                           {
                               if (sprites_.load_pak("GameDialog", "sprites/GameDialog.pak"))
                               {
                                   spdlog::info("Loaded GameDialog.pak");
                                   sprites_.store_sprite_at_id(57, "GameDialog", 8);
                                   sprites_.store_sprite_at_id(58, "GameDialog", 9);
                                   sprites_.store_sprite_at_id(60, "GameDialog", 0);
                                   sprites_.store_sprite_at_id(61, "GameDialog", 1);
                                   sprites_.store_sprite_at_id(62, "GameDialog", 2);
                                   sprites_.store_sprite_at_id(63, "GameDialog", 3);
                               }
                           }});
    init_steps_.push_back({"Loading dialog sprites...",
                           [this]()
                           {
                               if (sprites_.load_pak("DialogText", "sprites/DialogText.pak"))
                               {
                                   spdlog::info("Loaded DialogText.pak");
                                   sprites_.store_sprite_at_id(71, "DialogText", 1);
                               }
                           }});

    init_steps_.push_back({"Initializing UI...",
                           [this]()
                           {
                               ui_.set_sprite_manager(&sprites_);
                               ui_.initialize();
                           }});

    init_steps_.push_back({"Initializing game systems...",
                           [this]()
                           {
                               inventory_.initialize();
                           }});
    init_steps_.push_back({"Initializing game systems...",
                           [this]()
                           {
                               magic_.initialize();
                           }});
    init_steps_.push_back({"Initializing game systems...",
                           [this]()
                           {
                               skills_.initialize();
                           }});
    init_steps_.push_back({"Initializing game systems...",
                           [this]()
                           {
                               combat_.initialize(&entities_, &magic_, &skills_, &sounds_, &inventory_);
                           }});

    // Each effect PAK is its own step for fine-grained progress
    for (const auto& entry : effect_paks)
    {
        init_steps_.push_back({"Loading effect sprites...",
                               [this, entry]()
                               {
                                   std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";
                                   if (!sprites_.load_pak(entry.pak_name, pak_path))
                                   {
                                       spdlog::debug("Optional effect PAK not found: {}", pak_path);
                                       return;
                                   }
                                   sprites_.preload_pak_metadata(entry.pak_name);
                               }});
    }

    init_steps_.push_back({"Initializing effects...",
                           [this]()
                           {
                               effects_.initialize(sprites_, sounds_, world_);
                               magic_.set_effect_system(&effects_);
                           }});

    // Load ground item sprites (item-ground.pak)
    init_steps_.push_back({"Loading item sprites...",
                           [this]()
                           {
                               if (!sprites_.load_pak("item-ground", "sprites/item-ground.pak"))
                               {
                                   spdlog::warn("Failed to load item-ground.pak");
                                   return;
                               }
                               sprites_.preload_pak_metadata("item-ground");
                               spdlog::info("Loaded item-ground.pak");
                           }});

    // Load inventory item sprites (item-pack.pak) - used for inventory dialog and large ground items
    init_steps_.push_back({"Loading item pack sprites...",
                           [this]()
                           {
                               if (!sprites_.load_pak("item-pack", "sprites/item-pack.pak"))
                               {
                                   spdlog::warn("Failed to load item-pack.pak");
                                   return;
                               }
                               sprites_.preload_pak_metadata("item-pack");
                               spdlog::info("Loaded item-pack.pak");
                           }});

    init_steps_.push_back({"Setting up network...",
                           [this]()
                           {
                               setup_network_handlers();
                               input_handler_.initialize(*this);
                               dialog_callbacks_.initialize(*this);
                               ws_handler_.initialize(*this);
                           }});

    // Each equipment PAK is its own step for fine-grained progress
    for (const auto& entry : menu_character_renderer::get_pak_load_list())
    {
        init_steps_.push_back({"Loading character sprites...",
                               [this, entry]()
                               {
                                   std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";
                                   if (!sprites_.load_pak(entry.pak_name, pak_path))
                                   {
                                       spdlog::debug("Optional equipment PAK not found: {}", pak_path);
                                       return;
                                   }
                                   for (uint32_t i = 0; i < entry.sprite_count; ++i)
                                   {
                                       uint16_t global_id = static_cast<uint16_t>(entry.sprite_id + i);
                                       sprites_.store_sprite_at_id(
                                           global_id, entry.pak_name, entry.pak_start_index + i);
                                   }
                               }});
    }
    init_steps_.push_back({"Loading character sprites...",
                           [this]()
                           {
                               menu_char_renderer_.set_initialized();
                               spdlog::info("Menu character renderer initialized");
                           }});

    init_steps_.push_back({"Loading paperdoll sprites...",
                           [this]()
                           {
                               paperdoll_renderer_.initialize(sprites_);
                           }});

    // Each monster PAK is its own step for fine-grained progress
    for (const auto& entry : monster_paks)
    {
        init_steps_.push_back({"Loading monster sprites...",
                               [this, entry]()
                               {
                                   std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";
                                   if (!sprites_.load_pak(entry.pak_name, pak_path))
                                   {
                                       spdlog::debug("Optional monster PAK not found: {}", pak_path);
                                       return;
                                   }
                                   for (uint16_t i = 0; i < entry.sprite_count; ++i)
                                   {
                                       uint16_t global_id = static_cast<uint16_t>(entry.sprite_id + i);
                                       sprites_.store_sprite_at_id(global_id, entry.pak_name, i);
                                   }
                               }});
    }

    init_steps_.push_back(
        {"Initializing screens...",
         [this]()
         {
             screens_.initialize();
             screens_.get_main_menu_screen().set_on_start([this]() { change_state(game_state::login); });
             screens_.get_main_menu_screen().set_on_quit([this]() { change_state(game_state::quit); });
             auto open_settings = [this]()
             {
                 if (auto* settings = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options)))
                 {
                     settings->set_ui_style(ui_.style());
                 }
                 ui_.open_dialog(dialog_type::options);
             };
             screens_.get_main_menu_screen().set_on_settings(open_settings);
             screens_.get_login_screen().set_on_settings(open_settings);
             screens_.get_character_select_screen().set_on_settings(open_settings);
             screens_.get_login_screen().set_on_login(
                 [this](const std::string& account, const std::string& password)
                 {
                     spdlog::info("Login attempt: {} (pass length: {})", account, password.length());
                     attempt_login(account, password);
                 });
             screens_.get_login_screen().set_on_cancel([this]() { change_state(game_state::main_menu); });
             screens_.get_character_select_screen().set_on_select(
                 [this](int32_t index)
                 {
                     if (index >= 0 && index < static_cast<int32_t>(characters_.size()))
                     {
                         int32_t character_id = characters_[index].id;
                         spdlog::info(
                             "Entering game with character '{}' (ID: {})", characters_[index].name, character_id);
                         ws_handler_.request_enter_game(character_id);
                     }
                 });
             screens_.get_character_select_screen().set_on_create(
                 [this]()
                 {
                     spdlog::info("Opening character creation");
                     change_state(game_state::create_character);
                 });
             screens_.get_character_select_screen().set_on_delete(
                 [this](int32_t index)
                 {
                     if (index >= 0 && index < static_cast<int32_t>(characters_.size()))
                     {
                         int32_t character_id = characters_[index].id;
                         spdlog::info("Deleting character '{}' (ID: {})", characters_[index].name, character_id);
                         ws_handler_.request_delete_character(character_id);
                     }
                 });
             screens_.get_character_select_screen().set_on_logout(
                 [this]()
                 {
                     spdlog::info("Logging out to main menu");
                     ws_connection_.disconnect();
                     change_state(game_state::main_menu);
                 });
             screens_.get_character_create_screen().set_on_create(
                 [this](const character_create_data& data)
                 {
                     spdlog::info("Creating character: name='{}' gender={} stats={}/{}/{}/{}/{}/{}",
                                  data.name,
                                  data.gender,
                                  data.strength,
                                  data.vitality,
                                  data.dexterity,
                                  data.intelligence,
                                  data.magic,
                                  data.charisma);
                     ws_handler_.request_create_character(data);
                 });
             screens_.get_character_create_screen().set_on_cancel(
                 [this]()
                 {
                     spdlog::info("Canceling character creation, returning to character select");
                     change_state(game_state::select_character);
                 });
             screens_.get_connection_lost_screen().set_on_timeout(
                 [this]()
                 {
                     spdlog::info("Connection lost timeout, returning to main menu");
                     ws_connection_.disconnect();
                     change_state(game_state::main_menu);
                 });
             screens_.get_reconnect_screen().set_on_reconnect(
                 [this]()
                 {
                     if (launch_options_.has_credentials())
                     {
                         spdlog::info("Reconnect screen: attempting login with stored credentials");
                         attempt_login(*launch_options_.username, *launch_options_.password);
                     }
                 });
             screens_.get_reconnect_screen().set_on_exit(
                 [this]()
                 {
                     spdlog::info("Reconnect screen: player chose to exit");
                     change_state(game_state::quit);
                 });
             screens_.get_character_select_screen().set_character_renderer(&menu_char_renderer_);
             screens_.get_character_create_screen().set_character_renderer(&menu_char_renderer_);
             auto play_ui_sound = [this]()
             {
                 sounds_.play_ui_sound(14);
             };
             screens_.get_main_menu_screen().set_on_button_sound(play_ui_sound);
             screens_.get_login_screen().set_on_button_sound(play_ui_sound);
             screens_.get_character_select_screen().set_on_button_sound(play_ui_sound);
             screens_.get_character_create_screen().set_on_button_sound(play_ui_sound);
             screens_.get_reconnect_screen().set_on_button_sound(play_ui_sound);
         }});

    // Each dialog is its own step for fine-grained progress
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_character_select_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_character_create_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_character_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_inventory_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_spellbook_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_skills_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_chat_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_shop_dialog();
                               ui_.create_shop_sell_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_bank_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_party_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_guild_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_npc_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_trade_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_craft_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_map_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_repair_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_help_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_options_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.load_dialog_definitions_from_directory("assets/ui/dialogs");
                           }});
    init_steps_.push_back(
        {"Creating dialogs...",
         [this]()
         {
             ui_.create_icon_panel_dialog();
             if (auto* panel = ui_.get_icon_panel())
                 panel->set_screen_size(renderer_->width(), renderer_->height());
         }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_gauge_panel_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_levelup_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_quest_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_level_up_settings_dialog();
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               ui_.create_fishing_dialog();
                               if (auto* dlg = dynamic_cast<fishing_dialog*>(ui_.get_dialog(dialog_type::fishing)))
                               {
                                   dlg->set_on_catch_clicked([this]() { ws_handler_.request_fish_catch(); });
                               }
                           }});
    init_steps_.push_back({"Creating dialogs...",
                           [this]()
                           {
                               dialog_callbacks_.setup_callbacks();
                           }});

    // Wire inventory dialog to inventory system and drag system
    init_steps_.push_back({"Wiring inventory...",
                           [this]()
                           {
                               if (auto* inv_dlg =
                                       dynamic_cast<inventory_dialog*>(ui_.get_dialog(dialog_type::inventory)))
                               {
                                   inv_dlg->set_inventory(&inventory_);

                                   // When user starts dragging an item from inventory
                                   inv_dlg->set_on_drag_start(
                                       [this](uint32_t item_id, int32_t cx, int32_t cy, int32_t off_x, int32_t off_y)
                                       {
                                           const auto* entry = inventory_.get_bag_item(item_id);
                                           if (entry)
                                               ui_.begin_item_drag(entry->data, item_id, equip_pos::none,
                                                                   dialog_type::inventory, cx, cy, off_x, off_y);
                                       });

                                   // Double-click an equippable item to equip it
                                   inv_dlg->set_on_equip(
                                       [this](uint32_t item_id)
                                       {
                                           const auto* entry = inventory_.get_bag_item(item_id);
                                           if (!entry || !entry->data.is_equippable())
                                               return;
                                           spdlog::info("Equipping item {} via double-click", item_id);
                                           ws_connection_.send(
                                               make_equip_request(item_id, entry->data.equip_position));
                                       });

                                   // Double-click a potion / food / scroll to use it
                                   inv_dlg->set_on_use([this](uint32_t item_id)
                                                       { ws_handler_.request_use_item(item_id); });
                               }

                               // character_dialog reads inventory directly — no explicit slot-update needed.
                               // Set empty callbacks struct to satisfy the API.
                               inventory_.set_callbacks({});

                               // Wire character dialog — drag and double-click to unequip
                               if (auto* chr_dlg =
                                       dynamic_cast<character_dialog*>(ui_.get_dialog(dialog_type::character_info)))
                               {
                                   chr_dlg->set_on_drag_start_slot(
                                       [this, chr_dlg](equip_pos slot, int32_t cx, int32_t cy)
                                       {
                                           auto item_id_opt = inventory_.equipment().get(slot);
                                           const item* itm = inventory_.get_equipped_item(slot);
                                           if (!itm || !item_id_opt)
                                               return;
                                           chr_dlg->set_dragging_slot(slot);
                                           ui_.begin_item_drag(*itm, *item_id_opt, slot,
                                                               dialog_type::character_info, cx, cy, 0, 0);
                                       });

                                   chr_dlg->set_on_double_click_slot(
                                       [this](equip_pos slot)
                                       {
                                           spdlog::info("Unequipping slot {} via double-click",
                                                        static_cast<int>(slot));
                                           ws_connection_.send(make_unequip_request(slot));
                                       });
                               }

                               // Drag from character dialog paperdoll to inventory triggers unequip
                               ui_.set_on_unequip_from_drag(
                                   [this](equip_pos slot, int32_t bag_x, int32_t bag_y)
                                   {
                                       spdlog::info("Unequipping slot {} via drag-to-inventory at ({},{})",
                                                    static_cast<int>(slot), bag_x, bag_y);
                                       // Optimistically update client state so the item appears
                                       // at the drop position immediately (no flash while waiting
                                       // for the server). The inventory dialog skips equipped
                                       // items, so we must clear the slot too.
                                       if (auto item_id = inventory_.equipment().get(slot))
                                       {
                                           inventory_.set_position(
                                               *item_id,
                                               static_cast<int16_t>(bag_x),
                                               static_cast<int16_t>(bag_y));
                                           inventory_.equipment().clear(slot);
                                       }
                                       ws_connection_.send(make_unequip_request(
                                           slot, static_cast<int16_t>(bag_x),
                                           static_cast<int16_t>(bag_y)));
                                   });

                               // Drag from inventory to character dialog triggers equip
                               ui_.set_on_equip_from_drag(
                                   [this](uint32_t item_id)
                                   {
                                       const auto* entry = inventory_.get_bag_item(item_id);
                                       if (!entry || !entry->data.is_equippable())
                                           return;
                                       spdlog::info("Equipping item {} via drag-to-paperdoll", item_id);
                                       ws_connection_.send(
                                           make_equip_request(item_id, entry->data.equip_position));
                                   });

                               // Drop item to world — snap back, lock, confirm, then send
                               // Drop on the bank dialog: deposit (JSON), the bank re-reads itself
                               ui_.set_on_deposit_to_bank([this](uint32_t item_id)
                                                          { ws_handler().request_bank_deposit(item_id); });

                               ui_.set_on_drop_in_world(
                                   [this](uint32_t item_id)
                                   {
                                       auto* inv_dlg = dynamic_cast<inventory_dialog*>(
                                           ui_.get_dialog(dialog_type::inventory));
                                       if (inv_dlg)
                                           inv_dlg->restore_drag_position();

                                       auto* entry = inventory_.get_bag_item(item_id);
                                       if (!entry)
                                           return;

                                       entry->locked = true;
                                       uint32_t type_id = entry->data.template_id;

                                       // Send drop request (shared lambda)
                                       auto do_drop = [this, item_id]()
                                       {
                                           pending_drop_item_id_ = item_id;
                                           spdlog::info("Dropping item {}", item_id);
                                           json msg = make_drop_request(item_id);
                                           ws_connection_.send(msg);
                                       };

                                       // Skip confirm if user previously toggled "don't ask" for this type
                                       if (skip_drop_confirm_.contains(type_id))
                                       {
                                           do_drop();
                                           return;
                                       }

                                       // Show drop confirmation dialog
                                       item itm_copy = entry->data; // Copy for dialog lifetime
                                       ui_.create_drop_confirm_box(
                                           itm_copy,
                                           [this, item_id, type_id, do_drop](bool confirmed, bool skip_next)
                                           {
                                               if (confirmed)
                                               {
                                                   if (skip_next)
                                                       skip_drop_confirm_.insert(type_id);
                                                   do_drop();
                                               }
                                               else
                                               {
                                                   // Cancelled — unlock the item
                                                   auto* e = inventory_.get_bag_item(item_id);
                                                   if (e)
                                                       e->locked = false;
                                               }
                                           });
                                   });

                               // Reposition item within bag (free-form mode)
                               // Position is already updated locally by inventory_dialog during drag.
                               // Just send the server message.
                               ui_.set_on_reposition_item(
                                   [this](uint32_t item_id, int32_t x, int32_t y, bool shift_held)
                                   {
                                       auto send_reposition = [this](uint32_t id, int16_t rx, int16_t ry)
                                       {
                                           ws_connection_.send(make_inventory_reposition_request(id, rx, ry));
                                       };

                                       auto ix = static_cast<int16_t>(x);
                                       auto iy = static_cast<int16_t>(y);
                                       send_reposition(item_id, ix, iy);

                                       if (shift_held)
                                       {
                                           // Find the dragged item's template to match against
                                           const auto* dragged = inventory_.get_bag_item(item_id);
                                           if (!dragged)
                                               return;
                                           uint32_t target_template = dragged->data.template_id;

                                           // Move all other items of the same type to the same position
                                           std::vector<uint32_t> to_move;
                                           inventory_.for_each_bag_item(
                                               [&](uint32_t other_id, const bag_item& entry)
                                               {
                                                   if (other_id != item_id && entry.data.template_id == target_template)
                                                       to_move.push_back(other_id);
                                               });
                                           for (uint32_t other_id : to_move)
                                           {
                                               inventory_.set_position(other_id, ix, iy);
                                               send_reposition(other_id, ix, iy);
                                           }
                                       }
                                   });
                           }});

    // Wire chat input overlay
    init_steps_.push_back(
        {"Setting up chat...",
         [this]()
         {
             chat_input_.set_on_send(
                 [this](std::string_view content, std::string_view channel, std::string_view recipient)
                 {
                     // Send via WebSocket
                     ws_handler_.send_chat_message(content, channel, recipient);

                     // Local echo
                     chat_message msg;
                     if (channel == "shout")
                         msg.type = chat_type::shout;
                     else if (channel == "whisper")
                         msg.type = chat_type::whisper;
                     else if (channel == "guild")
                         msg.type = chat_type::guild;
                     else if (channel == "party")
                         msg.type = chat_type::party;
                     else if (channel == "faction")
                         msg.type = chat_type::faction;
                     else if (channel == "gm")
                         msg.type = chat_type::gm;
                     else if (channel == "trade")
                         msg.type = chat_type::trade;
                     else
                         msg.type = chat_type::normal;

                     if (entity* player = local_player())
                         msg.sender = player->name().name;
                     msg.content = std::string(content);
                     msg.recipient = std::string(recipient);
                     msg.timestamp = std::chrono::system_clock::now();

                     if (auto* chat_dlg = dynamic_cast<chat_dialog*>(ui_.get_dialog(dialog_type::chat)))
                         chat_dlg->add_message(msg);

                     // Set chat bubble on local player entity
                     if (entity* player = local_player())
                     {
                         auto& name = player->name();
                         name.chat_message = std::string(content);
                         name.chat_timer = 4.0f;
                         name.chat_elapsed = 0.0f;
                         name.chat_style = get_chat_bubble_style(channel);
                     }
                 });

             chat_input_.set_on_command([this](std::string_view command)
                                        { ws_handler_.send_chat_message(command, "command", ""); });

             chat_input_.set_type_to_chat(config::instance().game().type_to_chat);
         }});

    // Tile sprites: register core mappings + discover tile PAKs, then queue each load
    // initialize_core and discover are fast (no I/O), so run them now at queue time
    tile_registry_.initialize_core(sprites_, "sprites/");
    auto tile_paks = tile_registry_.discover_tile_pak_list();
    for (auto& tp : tile_paks)
    {
        init_steps_.push_back({"Loading tile sprites...",
                               [this, entry = std::move(tp)]()
                               {
                                   tile_registry_.load_tile_pak(entry);
                               }});
    }

    // World initialization (depends on tile registry being populated)
    init_steps_.push_back({"Initializing world...",
                           [this]()
                           {
                               if (tile_registry_.registered_count() > 0)
                               {
                                   if (!world_.initialize(tile_registry_))
                                       spdlog::warn("World initialization failed - terrain rendering may not work");
                                   // The camera centres on the scene, which in the scaled view mode is the
                                   // internal resolution, not the window: with the window size here the
                                   // player sat right of centre until the first resolution change
                                   const auto& video = config::instance().video();
                                   const uint32_t scene_w = renderer_ ? renderer_->scene_width() : video.screen_width;
                                   const uint32_t scene_h = renderer_ ? renderer_->scene_height() : video.screen_height;
                                   world_.set_screen_size(scene_w, scene_h);
                                   weather_system_.set_screen_size(scene_w, scene_h);
                                   spdlog::info("Tile sprite registry: {} mappings", tile_registry_.registered_count());
                               }
                               else
                               {
                                   spdlog::warn("Tile sprite registry empty - terrain rendering disabled");
                               }
                           }});

    init_steps_.push_back({"Initializing audio...",
                           [this]()
                           {
                               if (sounds_.initialize(*audio_))
                               {
                                   sounds_.set_sfx_enabled(config::instance().audio().sfx_enabled);
                                   sounds_.set_music_enabled(config::instance().audio().music_enabled);
                                   spdlog::info("Sound manager initialized with sfx={}, music={}",
                                                sounds_.is_sfx_enabled(),
                                                sounds_.is_music_enabled());
                                   entities_.set_sound_manager(&sounds_);
                                   entities_.set_effect_system(&effects_);
                               }
                               else
                               {
                                   spdlog::warn("Sound manager initialization failed - audio may not work");
                               }
                           }});

    init_steps_.push_back({"Finalizing...",
                           [this]()
                           {
                               world_.set_events({.on_map_changed =
                                                      [this](std::string_view /*old_map*/, std::string_view new_map)
                                                  {
                                                      sounds_.start_bgm(new_map, static_cast<int>(world_.weather()));
                                                      if (state_ == game_state::playing && renderer_)
                                                      {
                                                          transition_.set_show_label(false);
                                                          transition_.randomize_type();
                                                          transition_.start_reveal(renderer_->width(),
                                                                                   renderer_->height());
                                                      }
                                                  },
                                                  .on_weather_changed =
                                                      [this](weather_type w)
                                                  {
                                                      weather_system_.set_weather(w);
                                                      int weather_int = static_cast<int>(w);

                                                      // Rain ambient sound (legacy: E38 looping)
                                                      if (weather_int >= 1 && weather_int <= 3)
                                                          sounds_.start_ambient('E', 38);
                                                      else
                                                          sounds_.stop_ambient();

                                                      // Snow triggers christmas BGM
                                                      if (weather_int >= 4 && weather_int <= 6)
                                                      {
                                                          sounds_.start_bgm(world_.current_map_name(), weather_int);
                                                      }
                                                  },
                                                  .on_time_changed =
                                                      [this](time_of_day t)
                                                  {
                                                      // Transition sounds (legacy: E32 = birds/dawn, E31 = night)
                                                      if (t == time_of_day::dawn)
                                                          sounds_.play_sound('E', 32, 0);
                                                      else if (t == time_of_day::night)
                                                          sounds_.play_sound('E', 31, 0);
                                                  }});
                           }});

    init_steps_.push_back({"Ready!",
                           [this]()
                           {
                               register_debug_probes();
                               enter_state(game_state::main_menu);
                           }});

    spdlog::info("Game state manager initialized ({} loading steps queued)", init_steps_.size());
    return true;
}

void game_state_manager::shutdown()
{
    exit_state(state_);

    sounds_.shutdown();
    screens_.shutdown();
    network_.shutdown();
    world_.shutdown();
    entities_.shutdown();
    ui_.shutdown();
    sprites_.clear_cache();
    tile_registry_.clear_cache();

    spdlog::info("Game state manager shutdown");
}

bool game_state_manager::has_pending_init_steps() const
{
    return init_step_index_ < init_steps_.size();
}

std::pair<float, std::string> game_state_manager::run_next_init_step()
{
    if (init_step_index_ >= init_steps_.size())
        return {1.0f, "Ready!"};

    auto& step = init_steps_[init_step_index_];
    std::string msg = step.message;
    step.action();
    ++init_step_index_;

    float progress = static_cast<float>(init_step_index_) / static_cast<float>(init_steps_.size());
    return {progress, msg};
}

void game_state_manager::update(float delta_time, const input& inp)
{
    // Handle state transitions
    if (state_transition_)
    {
        exit_state(state_);
        state_ = pending_state_;
        enter_state(state_);
        state_transition_ = false;
    }

    // Update network
    network_.update();
    ws_connection_.update(delta_time);

    // Poll for WebSocket messages on main thread
    while (auto msg = ws_connection_.receive())
    {
        ws_handler_.handle_message(*msg);
    }

    // Handle pending connect event
    if (ws_connection_.poll_connect_event())
    {
        spdlog::info("WebSocket connected (polled on main thread)");
        ws_connecting_ = false;
        ws_retry_timer_ = 0.0f;
        std::string username, password;
        if (ws_handler_.consume_pending_login(username, password))
        {
            spdlog::info("Sending login request for user: {}", username);
            json login_msg = make_login_request(username, password);
            ws_connection_.send(login_msg);
        }
    }

    // Handle pending disconnect event
    std::string disconnect_reason;
    if (ws_connection_.poll_disconnect_event(disconnect_reason))
    {
        spdlog::warn("WebSocket disconnected (polled on main thread): {}", disconnect_reason);

        if (ws_connecting_)
        {
            // Still in initial connection attempt - retry logic will handle reconnect
            spdlog::info("Disconnected during connection attempt, will retry...");
        }
        else
        {
            ws_handler_.clear_session();
            characters_.clear();

            if (state_ != game_state::main_menu && state_ != game_state::connection_lost)
            {
                screens_.get_connection_lost_screen().set_reason(disconnect_reason.empty() ? "Server disconnected"
                                                                                           : disconnect_reason);
                change_state(game_state::connection_lost);
            }
        }
    }

    // Retry WebSocket connection on failure
    if (ws_connecting_)
    {
        auto ws_state = ws_connection_.state();
        if (ws_state == ws_connection_state::failed || ws_state == ws_connection_state::disconnected)
        {
            ws_retry_timer_ += delta_time;
            if (ws_retry_timer_ >= ws_retry_delay_)
            {
                ws_retry_timer_ = 0.0f;
                auto& net_cfg = config::instance().network();
                std::string ws_url =
                    "ws://" + net_cfg.login_server_host + ":" + std::to_string(net_cfg.login_server_port);
                spdlog::info("Retrying WebSocket connection to: {}", ws_url);
                ws_connection_.connect(ws_url);
            }
        }
        else
        {
            ws_retry_timer_ = 0.0f;
        }
    }

    // Update sprite memory management
    sprites_.update_memory(delta_time);

#ifdef HB_DEBUG_OVERLAY_ENABLED
    auto& debug_overlay = debug::debug_overlay::instance();
    debug_overlay.update(delta_time, inp);
    bool debug_consumed_input = debug_overlay.consumed_mouse_input() || debug_overlay.consumed_keyboard_input();
#else
    bool debug_consumed_input = false;
#endif

    if (!debug_consumed_input)
    {
        switch (state_)
        {
        case game_state::main_menu:
            update_main_menu(delta_time, inp);
            break;
        case game_state::login:
            update_login(delta_time, inp);
            break;
        case game_state::select_character:
            update_character_select(delta_time, inp);
            break;
        case game_state::create_character:
            update_character_create(delta_time, inp);
            break;
        case game_state::loading:
            update_loading(delta_time, inp);
            break;
        case game_state::playing:
            update_playing(delta_time, inp);
            break;
        case game_state::connection_lost:
            screens_.update(delta_time, inp);
            break;
        default:
            break;
        }

        ui_.update(delta_time, inp);
    }

    transition_.update(delta_time);
}

void game_state_manager::render(renderer& rend)
{
    switch (state_)
    {
    case game_state::main_menu:
        render_main_menu(rend);
        break;
    case game_state::login:
        render_login(rend);
        break;
    case game_state::select_character:
        render_character_select(rend);
        break;
    case game_state::create_character:
        render_character_create(rend);
        break;
    case game_state::loading:
        render_loading(rend);
        break;
    case game_state::playing:
        render_playing(rend);
        break;
    case game_state::connection_lost:
        screens_.render(rend, sprites_);
        break;
    default:
        break;
    }

    ui_.render(rend);

#ifdef HB_DEBUG_OVERLAY_ENABLED
    debug::debug_overlay::instance().render(rend);
#endif

    transition_.render(rend);
}

void game_state_manager::change_state(game_state new_state)
{
    if (state_ == new_state)
        return;

    spdlog::info("State change requested: {} -> {}", static_cast<int>(state_), static_cast<int>(new_state));

    pending_state_ = new_state;
    state_transition_ = true;
}

void game_state_manager::enter_state(game_state state)
{
    spdlog::debug("Entering state: {}", static_cast<int>(state));

    switch (state)
    {
    case game_state::main_menu:
        ui_.close_all_dialogs();
        if (launch_options_.has_credentials())
        {
            // Show reconnect screen instead of main menu
            screens_.get_reconnect_screen().set_auto_connect(first_launch_connect_);
            first_launch_connect_ = false;
            screens_.change_screen(screen_type::reconnect);
        }
        else
        {
            screens_.change_screen(screen_type::main_menu);
        }
        sounds_.play_bgm_track("title-screen.ogg");
        break;

    case game_state::login:
        ui_.close_all_dialogs();
        screens_.change_screen(screen_type::login);
        break;

    case game_state::select_character:
        ui_.close_all_dialogs();
        refresh_character_select_screen();
        screens_.change_screen(screen_type::character_select);
        break;

    case game_state::create_character:
        ui_.close_dialog(dialog_type::character_select);
        screens_.change_screen(screen_type::character_create);
        break;

    case game_state::loading:
        screens_.change_screen(screen_type::none);
        ui_.close_all_dialogs();
        loading_progress_ = 0.0f;
        loading_message_ = "Loading...";
        break;

    case game_state::playing:
        screens_.change_screen(screen_type::none);
        ui_.close_all_dialogs();
        ui_.clear_mouse_consumed();
        input_handler_.suppress_for(0.5f);
        transition_.set_show_label(false);
        transition_.randomize_type();
        transition_.start_reveal(renderer_->width(), renderer_->height());
        ui_.open_dialog(dialog_type::icon_panel);
        if (auto* panel = ui_.get_icon_panel())
            panel->set_screen_size(renderer_->width(), renderer_->height());
        update_icon_panel();
        sounds_.start_bgm(world_.current_map_name(), static_cast<int>(world_.weather()));
        break;

    case game_state::connection_lost:
        screens_.change_screen(screen_type::connection_lost);
        ui_.close_all_dialogs();
        break;

    default:
        break;
    }
}

void game_state_manager::exit_state(game_state state)
{
    spdlog::debug("Exiting state: {}", static_cast<int>(state));

    switch (state)
    {
    case game_state::playing:
        sounds_.stop_bgm();
        clear_game_data();
        break;

    default:
        break;
    }
}

void game_state_manager::clear_game_data()
{
    spdlog::info("Clearing game data (map, entities, combat, inventory)");

    entities_.remove_all_entities();
    entities_.set_local_player(invalid_entity_id);
    world_.unload_map();
    world_.set_cinematic_mode(false);
    world_.set_global_render_mode(false);
    world_.set_zoom_mode_enabled(false);

    // Clear extracted subsystems
    input_handler_.clear();
    ws_handler_.clear();

    // Clear visual effects, weather, status log, and floating text
    effects_.clear();
    weather_system_.set_weather(weather_type::clear);
    sounds_.stop_ambient();
    status_log_.clear();
    floating_text_.clear();
    ground_items_.clear();
    guild_.clear();
    quests_.clear();
    spell_hotbar_.fill(0);
    skip_drop_confirm_.clear();
    pending_drop_item_id_ = 0;
}

void game_state_manager::start_map_transition(std::function<void()> on_midpoint)
{
    if (renderer_)
    {
        transition_.start_full(renderer_->width(), renderer_->height(), std::move(on_midpoint));
    }
}

void game_state_manager::update_main_menu(float delta_time, const input& inp)
{
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open())
    {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_login(float delta_time, const input& inp)
{
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open())
    {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_character_select(float delta_time, const input& inp)
{
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open())
    {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_character_create(float delta_time, const input& inp)
{
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open())
    {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_loading(float delta_time, const input& inp)
{
    (void)inp;

    loading_progress_ += delta_time * 0.5f;
    if (loading_progress_ >= 1.0f)
    {
        loading_progress_ = 1.0f;
        change_state(game_state::playing);
    }
}

void game_state_manager::update_playing(float delta_time, const input& inp)
{
    // Map display-space mouse to scene-space for game world interactions
    auto [scene_mx, scene_my] = renderer_->display_to_scene(inp.mouse_x(), inp.mouse_y());

    // Convert scene-space mouse position to view coordinates for entity hover detection
    auto [mouse_world_x, mouse_world_y] = world_.screen_to_world(scene_mx, scene_my);
    input_handler_.set_mouse_position(mouse_world_x - world_.camera_x(), mouse_world_y - world_.camera_y());

    // Update blocked movement cooldown
    input_handler_.update_cooldown(delta_time);

    // Update debug stats early so consumed_mouse_click() is set before input handling
    auto& debug_stats = debug::debug_stats::instance();
    debug_stats.update(delta_time, inp);

    // Chat input overlay runs first - when active it consumes keyboard input.
    // Skip overlay activation when a dialog has text focus (e.g. chat search).
    bool chat_active = false;
    if (!ui_.has_text_focus())
        chat_active = chat_input_.update(delta_time, inp);

    // Handle game input only when chat overlay is not active and debug panel didn't consume click
    if (!chat_active && !ui_.has_text_focus() && !debug_stats.consumed_mouse_click())
        input_handler_.handle_input(inp);

    // Track alt key state for super attack indicator
    bool alt_held = inp.is_key_down(sf::Keyboard::Key::LAlt) || inp.is_key_down(sf::Keyboard::Key::RAlt);
    if (auto* panel = ui_.get_icon_panel())
        panel->set_alt_held(alt_held);

    // Mouse wheel zoom (use scene-space coordinates for anchor)
    if (world_.is_zoom_mode_enabled())
    {
        int32_t wheel = inp.wheel_delta();
        if (wheel != 0)
        {
            if (world_.is_cinematic_mode())
            {
                world_.adjust_zoom(static_cast<float>(-wheel) * 0.1f, scene_mx, scene_my);
            }
            else
            {
                world_.adjust_zoom(static_cast<float>(-wheel) * 0.1f);
            }
        }
    }

    // Update world
    world_.update(delta_time);
    weather_system_.update(delta_time);

    // Update entities
    entities_.update(delta_time, world_, input_handler_.is_combat_mode());

    // Cull entities that have drifted beyond the server's view radius.
    // The server should send despawn messages, but this acts as a client-side safety net
    // so stale entities don't accumulate when despawns are missed.
    if (!sees_all_)
    {
        if (const entity* player = local_player())
        {
            const auto& pt = player->transform();
            std::vector<entity_id> to_cull;
            entities_.for_each([&](const entity& ent)
            {
                if (ent.id() == entities_.local_player_id())
                    return;
                const auto& t = ent.transform();
                if (std::abs(t.tile_x - pt.tile_x) > view_radius_x_ ||
                    std::abs(t.tile_y - pt.tile_y) > view_radius_y_)
                {
                    to_cull.push_back(ent.id());
                }
            });
            for (auto id : to_cull)
                entities_.remove_entity(id);
        }
    }

    // Feed occupied tiles to map renderer for debug overlay
    if (world_.render_config().show_walkability)
        world_.set_occupied_tiles(entities_.get_occupied_tiles());
    else
        world_.clear_occupied_tiles();

    // Update camera to follow local player
    if (entity* player = local_player())
    {
        const auto& transform = player->transform();
        world_.set_player_position(transform.x, transform.y);
        sounds_.set_listener_position(transform.x, transform.y);
    }

    // Update combat
    combat_.update(delta_time);
    magic_.update_effects(delta_time);
    effects_.update(delta_time);
    skills_.update_cooldowns(delta_time);
    guild_.update(delta_time);

    // Update HUD
    update_icon_panel();

    // Update cursor based on what's under the mouse.
    // Spell targeting cursors are handled in input_handler::handle_spell_targeting().
    // Priority: entity hover (attack/friendly) > ground item grab > normal
    if (cursor_ && !input_handler_.is_spell_targeting())
    {
        int32_t cam_x = world_.camera_x();
        int32_t cam_y = world_.camera_y();
        int32_t mx = input_handler_.mouse_x();
        int32_t my = input_handler_.mouse_y();

        entity* hover = entities_.get_entity_at_screen_pos(sprites_, mx, my, cam_x, cam_y);
        if (hover && hover->id() != entities_.local_player_id() && hover->is_alive() &&
            hover->type() != entity_type::effect)
        {
            if (hover->type() == entity_type::monster)
            {
                cursor_->set_cursor(cursor_type::attack);
            }
            else
            {
                // NPCs and characters: check hostility (hostile NPCs/enemy players get attack cursor)
                auto h = hover->has_name() ? hover->name().hostile : hostility::neutral;
                cursor_->set_cursor(h == hostility::enemy ? cursor_type::attack : cursor_type::friendly);
            }
        }
        else if (ground_items_.hit_test(mx, my, cam_x, cam_y))
        {
            cursor_->set_cursor(cursor_type::ground_item_grab);
        }
    }

    // Populate debug stats data (update already called earlier for input consumption)
    if (debug_stats.visible())
    {
        int32_t cam_x = world_.camera_x();
        int32_t cam_y = world_.camera_y();
        debug_stats.set_camera_bounds(cam_x,
                                      cam_y,
                                      cam_x + static_cast<int32_t>(renderer_->width()),
                                      cam_y + static_cast<int32_t>(renderer_->height()));

        if (entity* player = local_player())
        {
            const auto& transform = player->transform();
            debug_stats.set_player_position(transform.tile_x, transform.tile_y, transform.x, transform.y);

            // Player movement state
            auto dir_to_str = [](direction d) -> const char*
            {
                switch (d)
                {
                case direction::north:
                    return "N";
                case direction::north_east:
                    return "NE";
                case direction::east:
                    return "E";
                case direction::south_east:
                    return "SE";
                case direction::south:
                    return "S";
                case direction::south_west:
                    return "SW";
                case direction::west:
                    return "W";
                case direction::north_west:
                    return "NW";
                default:
                    return "?";
                }
            };
            bool running = player->has_movement() && player->movement().running;
            debug_stats.set_player_movement(
                dir_to_str(transform.facing), transform.moving, transform.move_progress, running);

            // Player stats
            if (player->has_stats())
            {
                const auto& stats = player->stats();
                debug_stats.set_player_stats(stats.hp, stats.max_hp, stats.mp, stats.max_mp, stats.level);
            }
        }

        debug_stats.set_entity_count(static_cast<int32_t>(entities_.entity_count()));
        debug_stats.set_map_name(std::string(world_.current_map_name()));
        debug_stats.set_network_connected(ws_connection_.is_connected());
        debug_stats.set_ping(ws_connection_.ping_ms());
        debug_stats.set_network_message_totals(ws_connection_.messages_received(), ws_connection_.messages_sent());
        debug_stats.set_sprite_cache_count(static_cast<int32_t>(sprites_.cached_sprite_count()));
        debug_stats.set_pak_files_loaded(static_cast<int32_t>(sprites_.loaded_pak_count()));

        // World state
        debug_stats.set_zoom_level(world_.zoom_level());
        debug_stats.set_chunk_count(static_cast<int32_t>(world_.chunk_count()));

        debug_stats.set_weather(weather_to_str(world_.weather()));
        debug_stats.set_time_of_day(time_to_str(world_.time()));

        // Draw calls (from previous frame)
        debug_stats.set_draw_calls(renderer_->draw_call_count());

        // Input (use scene-space coords for world/tile debug, display-space for screen pos)
        debug_stats.set_mouse_screen_pos(inp.mouse_x(), inp.mouse_y());
        auto [dbg_world_x, dbg_world_y] = world_.screen_to_world(scene_mx, scene_my);
        debug_stats.set_mouse_world_pos(dbg_world_x, dbg_world_y);
        auto [tile_x, tile_y] = world_.screen_to_tile(scene_mx, scene_my);
        debug_stats.set_mouse_tile_pos(tile_x, tile_y);

        // Populate hovered tile info
        {
            debug::tile_info ti;
            const auto& m = world_.current_map();
            if (m.is_loaded() && m.is_valid_position(tile_x, tile_y))
            {
                ti.valid = true;
                ti.tile_x = tile_x;
                ti.tile_y = tile_y;

                const auto& t = m.get_tile(tile_x, tile_y);
                ti.terrain_id = t.terrain_id;
                ti.terrain_frame = t.terrain_frame;
                ti.object_id = t.object_id;
                ti.object_frame = t.object_frame;
                ti.roof_id = t.roof_id;
                ti.flags = static_cast<uint16_t>(t.flags);
                ti.light_level = t.light_level;

                ti.walkable = has_flag(t.flags, tile_flag::walkable);
                ti.water = has_flag(t.flags, tile_flag::water);
                ti.lava = has_flag(t.flags, tile_flag::lava);
                ti.ice = has_flag(t.flags, tile_flag::ice);
                ti.swamp = has_flag(t.flags, tile_flag::swamp);
                ti.teleport = has_flag(t.flags, tile_flag::teleport);
                ti.blocks_sight = has_flag(t.flags, tile_flag::blocks_sight);
                ti.blocks_magic = has_flag(t.flags, tile_flag::blocks_magic);
                ti.safe_zone = has_flag(t.flags, tile_flag::safe_zone);
                ti.pvp_zone = has_flag(t.flags, tile_flag::pvp_zone);

                // Entities on this tile
                auto tile_entities = entities_.get_entities_on_tile(tile_x, tile_y);
                for (const auto* ent : tile_entities)
                {
                    debug::tile_entity_info ei;
                    ei.id = ent->id();
                    ei.type = static_cast<int>(ent->type());
                    ei.action = static_cast<int>(ent->current_action());
                    ei.alive = ent->is_alive();
                    ei.moving = ent->transform().moving;
                    ei.direction = static_cast<int>(ent->transform().facing);
                    if (ent->has_name())
                        ei.name = ent->name().name;
                    ti.entities.push_back(std::move(ei));
                }

                // Ground item
                if (auto* item = ground_items_.get_at_tile(static_cast<int16_t>(tile_x), static_cast<int16_t>(tile_y)))
                {
                    ti.ground_item_id = item->data.item_id;
                    ti.ground_item_name = item->data.name;
                    ti.ground_item_count = item->data.count;
                }
            }
            debug_stats.set_hovered_tile(std::move(ti));
        }

        entity* hovered =
            entities_.get_entity_at_screen_pos(sprites_, input_handler_.mouse_x(), input_handler_.mouse_y(), cam_x, cam_y);
        if (hovered && hovered->has_name())
        {
            debug_stats.set_hovered_entity(hovered->name().name + " (ID:" + std::to_string(hovered->id()) + ")");
        }
        else
        {
            debug_stats.set_hovered_entity("");
        }

        // Audio
        if (audio_)
        {
            debug_stats.set_active_sounds(static_cast<int32_t>(audio_->active_sound_count()));
        }
        debug_stats.set_bgm_track(std::string(sounds_.current_bgm_track()));

        // UI
        debug_stats.set_open_dialog_count(static_cast<int32_t>(ui_.open_dialog_count()));

        debug_stats.set_game_state("Playing");
        debug_stats.set_combat_mode(input_handler_.is_combat_mode(), input_handler_.is_safe_attack_mode());
    }

    // Entity info overlay - works independently of the debug stats panel
    // Uses entity IDs (not raw pointers) to avoid dangling references on entity removal
    if (debug_stats.entity_info_visible())
    {
        int32_t cam_x = world_.camera_x();
        int32_t cam_y = world_.camera_y();

        entity* hovered =
            entities_.get_entity_at_screen_pos(sprites_, input_handler_.mouse_x(), input_handler_.mouse_y(), cam_x, cam_y);
        debug_stats.set_hovered_entity_id(hovered ? hovered->id() : 0);

        // Auto-clear pinned entity if it no longer exists or was marked for removal
        if (auto pinned_id = debug_stats.pinned_entity_id(); pinned_id != 0)
        {
            auto* pinned = entities_.get_entity(pinned_id);
            if (!pinned || pinned->should_remove())
            {
                debug_stats.clear_pinned_entity();
            }
        }

        // Middle-click to pin/unpin entity for debug info
        if (inp.is_mouse_pressed(sf::Mouse::Button::Middle))
        {
            if (hovered && hovered->id() != debug_stats.pinned_entity_id())
            {
                debug_stats.set_pinned_entity_id(hovered->id());
                spdlog::debug("Pinned entity {} for debug info", hovered->id());
            }
            else
            {
                debug_stats.clear_pinned_entity();
                spdlog::debug("Unpinned entity debug info");
            }
        }
    }
    else
    {
        debug_stats.set_hovered_entity_id(0);
    }

    // Update status log and floating text
    status_log_.update(delta_time);
    floating_text_.update(delta_time);
}

void game_state_manager::render_main_menu(renderer& rend)
{
    if (screens_.current_screen_type() == screen_type::reconnect || sprites_.has_sprite_at_id(52))
    {
        screens_.render(rend, sprites_);
    }
    else
    {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("HELBREATH", screen_width / 2 - 60, 150, sf::Color::White, 24);
        rend.draw_text("Click to Start (sprites not loaded)", screen_width / 2 - 100, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_login(renderer& rend)
{
    if (sprites_.has_sprite_at_id(53))
    {
        screens_.render(rend, sprites_);
    }
    else
    {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("LOGIN (sprites not loaded)", screen_width / 2 - 80, 100, sf::Color::White, 16);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_character_select(renderer& rend)
{
    if (sprites_.has_sprite_at_id(57))
    {
        screens_.render(rend, sprites_);
    }
    else
    {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("CHARACTER SELECT (sprites not loaded)", screen_width / 2 - 120, 100, sf::Color::White);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_character_create(renderer& rend)
{
    if (sprites_.has_sprite_at_id(58))
    {
        screens_.render(rend, sprites_);
    }
    else
    {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("CHARACTER CREATE (sprites not loaded)", screen_width / 2 - 120, 100, sf::Color::White);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_loading(renderer& rend)
{
    rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(10, 10, 20), true);

    int32_t bar_width = 400;
    int32_t bar_height = 20;
    int32_t bar_x = (screen_width - bar_width) / 2;
    int32_t bar_y = screen_height / 2;

    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(40, 40, 60), true);
    rend.draw_rect(
        bar_x, bar_y, static_cast<int32_t>(bar_width * loading_progress_), bar_height, sf::Color(100, 150, 200), true);
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(80, 80, 120), false);
    rend.draw_text(loading_message_, bar_x, bar_y - 30, sf::Color::White);
}

void game_state_manager::render_playing(renderer& rend)
{
    rend.begin_scene(); // Scaled: redirect to scene_rt_. Others: no-op.

    world_.apply_zoom_view(rend);

    // Render terrain chunks and debug overlays (no objects)
    world_.render_terrain(rend);
    world_.reset_objects_rendered();

    // Row-interleaved rendering: for each tile row, draw entities then objects.
    // A tree at row N (drawn after entities at row N) extends upward, covering
    // entities already drawn at rows < N. Entities at rows > N are drawn later
    // and appear in front of the tree.
    auto range = world_.get_visible_tile_range();
    int32_t cam_x = world_.camera_x();
    int32_t cam_y = world_.camera_y();
    int32_t mouse_x = input_handler_.mouse_x();
    int32_t mouse_y = input_handler_.mouse_y();

    // Draw ground item sprites (flat on ground, beneath entities and objects)
    std::string_view ground_pak =
        config::instance().game().use_large_ground_items ? "item-pack" : "item-ground";
    ground_items_.render_sprites(*renderer_, sprites_, cam_x, cam_y, ground_pak);

    auto sorted = entities_.get_visible_entities_sorted(rend, cam_x, cam_y);

    // Compute local player's screen bounds for tree transparency.
    // Only apply when the player is on the same row or north of the tree,
    // so trees south of the player render at full opacity.
    sf::IntRect player_bounds;
    bool have_player_bounds = false;
    int32_t player_tile_y = 0;
    if (auto* lp = entities_.local_player())
    {
        if (auto bounds = entities_.get_entity_screen_bounds(*lp, sprites_, cam_x, cam_y))
        {
            player_bounds = *bounds;
            have_player_bounds = true;
            player_tile_y = lp->transform().y / tile_height;
        }
    }

    size_t idx = 0;
    for (int32_t row = range.start_y; row < range.end_y; ++row)
    {
        // Pixel Y of the bottom edge of this tile row
        int32_t row_pixel_bottom = (row + 1) * tile_height;

        // Draw entities whose world Y position is above this row's bottom edge
        while (idx < sorted.size() && sorted[idx]->transform().y < row_pixel_bottom)
        {
            entities_.render_single_entity(rend, sprites_, *sorted[idx], cam_x, cam_y, mouse_x, mouse_y);
            ++idx;
        }

        // Draw objects on this row (trees paint over entities at higher rows).
        // Only make trees transparent when the player is on this row or north of it.
        const sf::IntRect* bounds_for_row = (have_player_bounds && player_tile_y <= row) ? &player_bounds : nullptr;
        world_.render_objects_row(rend, row, bounds_for_row);
    }

    // Draw any remaining entities past the last visible row
    while (idx < sorted.size())
    {
        entities_.render_single_entity(rend, sprites_, *sorted[idx], cam_x, cam_y, mouse_x, mouse_y);
        ++idx;
    }

    // Draw name/health overlays on top of all sprites and objects
    entities_.render_name_overlays(rend, sprites_, sorted, cam_x, cam_y, mouse_x, mouse_y);

    // Draw ground item labels (hover only)
    ground_items_.render_labels(rend, cam_x, cam_y, mouse_x, mouse_y);

    effects_.render(rend, cam_x, cam_y);
    world_.reset_zoom_view(rend);

    // Weather particles render in screen space (not affected by zoom)
    weather_system_.render_particles(rend);

    // Day/night tint overlay
    world_.render_overlay(rend);

    floating_text_.render(rend, cam_x, cam_y);

    // Provide camera info for fog overlay styles that need tile alignment
    rend.set_fog_camera_info(cam_x, cam_y, world_.zoom_level());

    rend.end_scene(); // Scaled: composite. Extended: fog overlay. Special: no-op.

    rend.begin_ui(); // UI overlay: native pixels, or the internal resolution placed in the window (scaled)

    // Update rendering stats
    auto& ds = debug::debug_stats::instance();
    int32_t visible_tiles = (range.end_x - range.start_x) * (range.end_y - range.start_y);
    ds.set_tiles_rendered(visible_tiles);
    ds.set_sprites_rendered(static_cast<int32_t>(sorted.size()));
    ds.set_objects_rendered(world_.objects_rendered());

    ds.render(rend);
    // Resolve entity info overlay target by ID (safe against entity removal)
    {
        const entity* info_ent = nullptr;
        if (auto pid = ds.pinned_entity_id(); pid != 0)
            info_ent = entities_.get_entity(pid);
        if (!info_ent)
        {
            if (auto hid = ds.hovered_entity_id(); hid != 0)
                info_ent = entities_.get_entity(hid);
        }
        ds.render_entity_info(
            rend, info_ent, static_cast<int32_t>(renderer_->width()), static_cast<int32_t>(renderer_->height()));
    }

    // Server time & weather status bar (top-right corner)
    {
        auto hour = world_.clock_hour();
        auto minute = world_.clock_minute();
        bool pm = hour >= 12;
        uint8_t display_hour = hour % 12;
        if (display_hour == 0)
            display_hour = 12;

        char time_buf[16];
        std::snprintf(time_buf, sizeof(time_buf), "%d:%02d %s", display_hour, minute, pm ? "PM" : "AM");

        const char* weather_str = weather_to_str(world_.weather());
        const char* time_str = time_to_str(world_.time());

        char status_buf[64];
        std::snprintf(status_buf, sizeof(status_buf), "%s  %s  %s", time_buf, time_str, weather_str);

        int32_t display_w = static_cast<int32_t>(rend.width()); // logical width: the UI view may be scaled
        int32_t padding = 6;
        int32_t text_w = static_cast<int32_t>(std::strlen(status_buf)) * 7;
        int32_t bar_w = text_w + padding * 2;
        int32_t bar_h = 20;
        int32_t bar_x = display_w - bar_w - 4;
        int32_t bar_y = 4;

        rend.draw_rect(bar_x, bar_y, bar_w, bar_h, sf::Color(0, 0, 0, 140), true);
        rend.draw_text(status_buf, bar_x + padding, bar_y + 2, sf::Color(220, 220, 220), 12);
    }

    status_log_.render(rend, static_cast<int32_t>(rend.width()), static_cast<int32_t>(rend.height()), 70);

    // Chat input overlay (always on top of game, above icon panel)
    chat_input_.render(rend, static_cast<int32_t>(rend.width()), static_cast<int32_t>(rend.height()));
}

void game_state_manager::setup_network_handlers()
{
    notify_handler_.initialize(*this);
    motion_handler_.initialize(*this);

    network_.register_handler(msg_response_log, [this](packet_reader& r) { handle_login_response(r); });

    network_.register_handler(msg_response_enter_game, [this](packet_reader& r) { handle_enter_game(r); });

    network_.register_handler(msg_response_player_data, [this](packet_reader& r) { handle_player_data(r); });

    network_.register_handler(msg_notify,
                              [this](packet_reader& r)
                              {
                                  auto subtype = r.read_u16();
                                  if (!subtype)
                                      return;
                                  notify_handler_.dispatch(*subtype, r);
                              });

    network_.register_handler(msg_response_motion,
                              [this](packet_reader& r) { motion_handler_.handle_motion_response(r); });

    network_.register_handler(msg_event_motion, [this](packet_reader& r) { motion_handler_.handle_motion_event(r); });

    network_.register_handler(msg_event_common, [this](packet_reader& r) { motion_handler_.handle_common_event(r); });

    network_.register_handler(msg_player_character_contents, [this](packet_reader& r) { handle_character_list(r); });
}

void game_state_manager::handle_login_response(packet_reader& reader)
{
    auto result_type = reader.read_u16();
    if (!result_type)
        return;

    if (*result_type == login_response::confirm)
    {
        spdlog::info("Login successful");
        network_.request_character_list();
    }
    else if (*result_type == login_response::password_mismatch)
    {
        show_error("Invalid password");
    }
    else if (*result_type == login_response::not_existing_account)
    {
        show_error("Account does not exist");
    }
    else if (*result_type == login_response::account_locked)
    {
        show_error("Account is locked");
    }
    else if (*result_type == login_response::service_not_available)
    {
        show_error("Service not available");
    }
    else
    {
        show_error("Login failed");
    }
}

void game_state_manager::handle_character_list(packet_reader& reader)
{
    auto count = reader.read_u8();
    if (!count)
        return;

    characters_.clear();
    for (uint8_t i = 0; i < *count; ++i)
    {
        character_info info;
        if (auto name = reader.read_string(20))
        {
            info.name = *name;
        }
        if (auto level = reader.read_u16())
        {
            info.level = *level;
        }
        characters_.push_back(info);
    }

    change_state(game_state::select_character);
}

void game_state_manager::handle_enter_game(packet_reader& reader)
{
    auto result = reader.read_u8();
    if (!result || *result != 0)
    {
        show_error("Failed to enter game");
        return;
    }

    auto host = reader.read_string(64);
    auto port = reader.read_u16();

    if (host && port)
    {
        if (network_.connect_game_server(*host, *port))
        {
            change_state(game_state::loading);
        }
        else
        {
            show_error("Failed to connect to game server");
        }
    }
}

void game_state_manager::handle_player_data(packet_reader& reader)
{
    (void)reader;
}

void game_state_manager::handle_map_data(packet_reader& reader)
{
    (void)reader;
}

void game_state_manager::set_local_player_id(entity_id id)
{
    entities_.set_local_player(id);
}

entity* game_state_manager::local_player()
{
    return entities_.local_player();
}

const entity* game_state_manager::local_player() const
{
    return entities_.local_player();
}

void game_state_manager::set_characters(std::vector<character_info> characters)
{
    characters_ = std::move(characters);

    if (state_ == game_state::select_character)
    {
        refresh_character_select_screen();
    }
}

void game_state_manager::select_character(size_t index)
{
    if (index < characters_.size())
    {
        selected_character_ = index;
    }
}

void game_state_manager::set_login_result(uint16_t result)
{
    if (result == login_response::confirm)
    {
        spdlog::info("Login successful");
    }
    else
    {
        spdlog::warn("Login failed: 0x{:04X}", result);
    }
}

void game_state_manager::refresh_character_select_screen()
{
    std::vector<char_slot_info> slot_chars;
    for (const auto& ch : characters_)
    {
        char_slot_info slot;
        slot.has_character = true;
        slot.name = ch.name;
        slot.level = ch.level;
        slot.exp = 0;
        slot.class_name = ch.warrior ? "Warrior" : "Mage";
        slot.gender = ch.gender;
        slot.skin_color = ch.skin_color;
        slot.hair_style = ch.hair_style;
        slot.hair_color = ch.hair_color;
        slot.underwear_color = ch.underwear_color;
        slot.body_armor = ch.body_armor;
        slot.arm_armor = ch.arm_armor;
        slot.pants = ch.pants;
        slot.boots = ch.boots;
        slot.helmet = ch.helmet;
        slot.mantle = ch.mantle;
        slot.weapon = ch.weapon;
        slot.shield = ch.shield;
        slot_chars.push_back(slot);
    }
    screens_.get_character_select_screen().set_characters(slot_chars);
}

void game_state_manager::set_character_create_result(uint16_t result)
{
    if (result == login_response::new_character_created)
    {
        spdlog::info("Character created successfully");
        network_.request_character_list();
    }
    else if (result == login_response::already_existing_character)
    {
        spdlog::warn("Character name already exists");
        show_error("Character name already exists");
    }
    else
    {
        spdlog::warn("Character creation failed: 0x{:04X}", result);
        show_error("Character creation failed");
    }
}

void game_state_manager::show_message(std::string_view message)
{
    (void)message;
    ui_.show_connection_dialog(nullptr);
}

void game_state_manager::show_error(std::string_view error)
{
    ui_.show_error_dialog(error, nullptr);
}

void game_state_manager::update_icon_panel()
{
    entity* player = local_player();
    if (!player)
        return;

    bool combat_mode = input_handler_.is_combat_mode();
    bool safe_attack_mode = input_handler_.is_safe_attack_mode();

    auto* panel = ui_.get_icon_panel();
    if (!panel)
        return;

    if (player->has_stats())
    {
        const auto& stats = player->stats();
        panel->set_hp(stats.hp, stats.max_hp);
        panel->set_mp(stats.mp, stats.max_mp);
        panel->set_sp(stats.sp, stats.max_sp);
        int64_t exp_for_next_level = static_cast<int64_t>(stats.level) * 1000;
        panel->set_experience(stats.experience, exp_for_next_level, stats.level);
    }
    const auto& transform = player->transform();
    panel->set_position(transform.tile_x, transform.tile_y);
    panel->set_map_name(world_.current_map_name());
    if (player->has_combat())
    {
        panel->set_poisoned(player->combat().poisoned);
    }
    bool weapon_mastered = false;
    if (const item* weapon = inventory_.get_equipped_item(equip_pos::weapon))
    {
        weapon_skill ws = combat_.get_weapon_skill(weapon->weapon);
        weapon_mastered = skills_.is_skill_mastered(static_cast<uint16_t>(ws));
    }
    panel->set_super_attack_available(weapon_mastered);
    if (player->has_stats())
        panel->set_super_attack_count(player->stats().super_attack_charges);
    panel->set_combat_mode(combat_mode);
    panel->set_safe_attack_mode(safe_attack_mode);
}

void game_state_manager::set_ui_style(ui_style style)
{
    ui_.set_style(style);

    render_mode mode = (style == ui_style::modern) ? render_mode::modern : render_mode::classic;

    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel)))
    {
        yaml_dlg->set_render_mode(mode);
    }
    else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel)))
    {
        icon_dlg->set_ui_style(style);
    }

    spdlog::info("UI style changed to {}", style == ui_style::modern ? "modern" : "classic");
}

void game_state_manager::set_spell_hotbar_slot(size_t slot, uint16_t spell_id)
{
    if (slot < spell_hotbar_.size())
    {
        spell_hotbar_[slot] = spell_id;
    }
}

uint16_t game_state_manager::get_spell_hotbar_slot(size_t slot) const
{
    if (slot < spell_hotbar_.size())
    {
        return spell_hotbar_[slot];
    }
    return 0;
}

void game_state_manager::auto_populate_spell_hotbar()
{
    spell_hotbar_.fill(0);

    auto learned = magic_.get_learned_spells();

    // Sort by circle (tier) then by spell ID for consistent ordering
    std::sort(learned.begin(),
              learned.end(),
              [](const spell* a, const spell* b)
              {
                  if (a->circle != b->circle)
                      return a->circle < b->circle;
                  return a->id < b->id;
              });

    size_t slot = 0;
    for (const auto* sp : learned)
    {
        if (slot >= spell_hotbar_.size())
            break;
        spell_hotbar_[slot++] = sp->id;
    }

    spdlog::debug("Auto-populated spell hotbar with {} spells", slot);
}

void game_state_manager::set_launch_options(launch_options opts)
{
    launch_options_ = std::move(opts);
}

void game_state_manager::attempt_login(const std::string& username, const std::string& password)
{
    auto& net_cfg = config::instance().network();
    std::string ws_url = "ws://" + net_cfg.login_server_host + ":" + std::to_string(net_cfg.login_server_port);

    spdlog::info("Connecting to server: {}", ws_url);
    spdlog::info("attempt_login called with username='{}'", username);

    // Store credentials in ws_handler for after connection
    ws_handler_.set_pending_login(username, password);

    // If already connected, send login request directly
    if (ws_connection_.is_connected())
    {
        spdlog::info("Already connected, sending login request directly for user: {}", username);
        json login_msg = make_login_request(username, password);
        ws_connection_.send(login_msg);
        return;
    }

    ws_connecting_ = true;
    ws_retry_timer_ = 0.0f;

    if (!ws_connection_.connect(ws_url))
    {
        ws_connecting_ = false;
        show_error("Failed to connect to server");
        return;
    }

    ui_.show_connection_dialog(
        [this]()
        {
            ws_connecting_ = false;
            ws_retry_timer_ = 0.0f;
            ws_connection_.disconnect();
            ws_handler_.clear_session();
            change_state(game_state::main_menu);
        });
}

void game_state_manager::request_pickup(int32_t tile_x, int32_t tile_y)
{
    spdlog::info("Requesting pickup at ({}, {})", tile_x, tile_y);

    if (entity* player = local_player())
    {
        player->set_action(object_action::get_item);
    }

    json msg = make_pickup_request(world_.current_map_name(), tile_x, tile_y);
    ws_connection_.send(msg);
}

void game_state_manager::set_view_radius(int16_t radius_x, int16_t radius_y, bool sees_all)
{
    view_radius_x_ = radius_x;
    view_radius_y_ = radius_y;
    sees_all_ = sees_all;
    // These radii control how far the server streams entities to us.
    // The fair zone (fog, culling, targeting) is set separately via set_render_mode.
}

void game_state_manager::send_view_range()
{
    // Send actual screen resolution so the server can compute visibility radii.
    uint32_t w = renderer_ ? renderer_->display_width() : config::instance().video().screen_width;
    uint32_t h = renderer_ ? renderer_->display_height() : config::instance().video().screen_height;
    json msg = make_set_view_range_request(w, h);
    ws_connection_.send(msg);
    spdlog::info("Sent view range: {}x{}", w, h);
}

bool game_state_manager::change_resolution(
    uint32_t width, uint32_t height, bool fullscreen, bool borderless, int32_t monitor_x, int32_t monitor_y)
{
    if (!renderer_)
    {
        spdlog::error("Cannot change resolution: renderer not initialized");
        return false;
    }

    bool settings_was_open = false;
    ui_style current_style = ui_style::classic;
    float music_vol = 1.0f;
    float sound_vol = 1.0f;

    if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options)))
    {
        settings_was_open = settings_dlg->visible();
        if (settings_was_open)
        {
            current_style = settings_dlg->get_ui_style();
            music_vol = settings_dlg->get_music_volume();
            sound_vol = settings_dlg->get_sound_volume();
            settings_dlg->close();
        }
    }

    if (!renderer_->set_resolution(width, height, fullscreen, borderless, monitor_x, monitor_y))
    {
        return false;
    }

    auto& video = config::instance().video();
    video.screen_width = width;
    video.screen_height = height;
    video.fullscreen = fullscreen;
    video.borderless = borderless;

    world_.set_screen_size(renderer_->scene_width(), renderer_->scene_height());
    weather_system_.set_screen_size(renderer_->scene_width(), renderer_->scene_height());

    if (state_ == game_state::playing)
    {
        send_view_range();
    }

    if (settings_was_open)
    {
        if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options)))
        {
            int32_t dlg_width = settings_dlg->bounds().width;
            int32_t dlg_height = settings_dlg->bounds().height;
            int32_t new_x = (static_cast<int32_t>(width) - dlg_width) / 2;
            int32_t new_y = (static_cast<int32_t>(height) - dlg_height) / 2;
            settings_dlg->set_position(new_x, new_y);
            settings_dlg->set_ui_style(current_style);
            settings_dlg->set_resolution(width, height);
            settings_dlg->set_display_mode(fullscreen, borderless);
            settings_dlg->set_music_volume(music_vol);
            settings_dlg->set_sound_volume(sound_vol);
            settings_dlg->keep_open_after_apply();
            ui_.open_dialog(dialog_type::options);
        }
    }

    if (auto* panel = ui_.get_icon_panel())
        panel->set_screen_size(width, height);

    return true;
}

void game_state_manager::register_debug_probes()
{
    if (!debug_server::is_running()) return;

    debug_server::register_probe("client/status", [this]() -> nlohmann::json
    {
        return {
            {"state",           static_cast<int>(state_)},
            {"local_player_id", entities_.local_player_id()}
        };
    });

    debug_server::register_probe("entity/local_player", [this]() -> nlohmann::json
    {
        auto* e = entities_.get_local_player();
        if (!e) return {{"found", false}};
        const auto& t = e->transform();
        const auto& s = e->sprite();
        return {
            {"found",        true},
            {"id",           e->id()},
            {"x",            t.x},
            {"y",            t.y},
            {"facing",       static_cast<int>(t.facing)},
            {"gender",       s.gender},
            {"skin_color",   s.skin_color},
            {"hair_style",   s.hair_style},
            {"hair_color",   s.hair_color},
            {"weapon",       s.weapon},
            {"shield",       s.shield},
            {"body_armor",   s.body_armor},
            {"pants",        s.pants},
            {"helmet",       s.helmet},
            {"arm_armor",    s.arm_armor},
            {"boots",        s.boots},
            {"mantle",       s.mantle},
            {"alpha",        s.alpha}
        };
    });

    debug_server::register_probe("entity/list", [this]() -> nlohmann::json
    {
        nlohmann::json arr = nlohmann::json::array();
        entities_.for_each([&](const entity& e)
        {
            const auto& t = e.transform();
            const auto& s = e.sprite();
            arr.push_back({
                {"id",     e.id()},
                {"type",   static_cast<int>(e.type())},
                {"x",      t.x},
                {"y",      t.y},
                {"weapon", s.weapon},
                {"gender", s.gender}
            });
        });
        return arr;
    });

    debug_server::register_probe("render/last_frame", [this]() -> nlohmann::json
    {
        return entities_.last_render_diagnostic();
    });

    debug_server::register_action("sprite/info", [this](const nlohmann::json& args) -> nlohmann::json
    {
        uint16_t id = args.value("id", uint16_t(0));
        const sprite* spr = sprites_.get_sprite_by_id(id);
        if (!spr) return {{"found", false}, {"id", id}};
        nlohmann::json frames = nlohmann::json::array();
        for (uint32_t i = 0; i < spr->frame_count(); ++i)
        {
            const auto& f = spr->get_frame(i);
            frames.push_back({
                {"x",       f.source_rect.position.x},
                {"y",       f.source_rect.position.y},
                {"w",       f.source_rect.size.x},
                {"h",       f.source_rect.size.y},
                {"pivot_x", f.pivot_x},
                {"pivot_y", f.pivot_y}
            });
        }
        auto ck = spr->color_key();
        return {
            {"found",       true},
            {"id",          id},
            {"frame_count", spr->frame_count()},
            {"tex_w",       spr->bitmap_width()},
            {"tex_h",       spr->bitmap_height()},
            {"color_key",   {{"r", ck.r}, {"g", ck.g}, {"b", ck.b}, {"a", ck.a}}},
            {"frames",      frames}
        };
    });

    debug_server::register_action("sprite/save_png", [this](const nlohmann::json& args) -> nlohmann::json
    {
        uint16_t id = args.value("id", uint16_t(0));
        std::string path = args.value("path", "debug_sprite_" + std::to_string(id) + ".png");
        const sprite* spr = sprites_.get_sprite_by_id(id);
        if (!spr) return {{"ok", false}, {"error", "sprite not found"}};
        sf::Image img = spr->texture().copyToImage();
        bool saved = img.saveToFile(path);
        return {{"ok", saved}, {"path", path}};
    });

    debug_server::register_action("entity/set_sprite_field", [this](const nlohmann::json& args) -> nlohmann::json
    {
        uint32_t entity_id = args.value("entity_id", uint32_t(0));
        std::string field  = args.value("field", "");
        uint8_t value      = args.value("value", uint8_t(0));
        entity* e = entities_.get_entity(entity_id);
        if (!e) return {{"ok", false}, {"error", "entity not found"}};
        auto& s = e->sprite();
        if      (field == "weapon")     s.weapon     = value;
        else if (field == "shield")     s.shield     = value;
        else if (field == "body_armor") s.body_armor = value;
        else if (field == "pants")      s.pants      = value;
        else if (field == "helmet")     s.helmet     = value;
        else if (field == "arm_armor")  s.arm_armor  = value;
        else if (field == "boots")      s.boots      = value;
        else if (field == "mantle")     s.mantle     = value;
        else if (field == "gender")     s.gender     = value;
        else if (field == "skin_color") s.skin_color = value;
        else if (field == "hair_style") s.hair_style = value;
        else if (field == "hair_color") s.hair_color = value;
        else return {{"ok", false}, {"error", "unknown field: " + field}};
        entities_.load_character_sprites(*e, sprites_);
        return {{"ok", true}, {"field", field}, {"value", value}};
    });

    debug_server::register_deferred("player/move_to", [this](const nlohmann::json& args)
    {
        entities_.debug_move_local_player(args.value("x", 0), args.value("y", 0));
    });

    debug_server::register_deferred("player/set_direction", [this](const nlohmann::json& args)
    {
        auto* e = entities_.get_local_player();
        if (e)
            e->transform().facing = static_cast<direction>(args.value("direction", 1));
    });
}

} // namespace hb
