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
#include <spdlog/spdlog.h>
#include <cmath>
#include <array>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/debug_overlay.hpp"
#endif

#include "debug/debug_stats.hpp"

namespace hb {

namespace {

// Monster/NPC PAK loading table
// Matches legacy Game.cpp MakeSprite() calls: MakeSprite("slm", 1220 + 7*8*0, 40, TRUE)
// Each entry: {pak_name, sprite_id_start, sprite_count}
// Formula: sprite_id = 1220 + (type - 10) * 56 + action * 8 + (dir - 1)
struct monster_pak_entry
{
    const char* pak_name;
    uint16_t sprite_id;
    uint16_t sprite_count;
};

static constexpr uint16_t npc_base = 1220;
static constexpr uint16_t npc_stride = 56;  // 7 actions * 8 directions

// All monster/NPC PAK files from legacy loading (Game.cpp lines 3836-3930)
static constexpr std::array monster_paks =
{
    monster_pak_entry{"slm",           static_cast<uint16_t>(npc_base + npc_stride * 0),  40},  // Slime (Type: 10)
    monster_pak_entry{"ske",           static_cast<uint16_t>(npc_base + npc_stride * 1),  40},  // Skeleton (Type: 11)
    monster_pak_entry{"Gol",           static_cast<uint16_t>(npc_base + npc_stride * 2),  40},  // Stone-Golem (Type: 12)
    monster_pak_entry{"Cyc",           static_cast<uint16_t>(npc_base + npc_stride * 3),  40},  // Cyclops (Type: 13)
    monster_pak_entry{"Orc",           static_cast<uint16_t>(npc_base + npc_stride * 4),  40},  // Orc (Type: 14)
    monster_pak_entry{"Shopkpr",       static_cast<uint16_t>(npc_base + npc_stride * 5),   8},  // ShopKeeper (Type: 15)
    monster_pak_entry{"Ant",           static_cast<uint16_t>(npc_base + npc_stride * 6),  40},  // Giant-Ant (Type: 16)
    monster_pak_entry{"Scp",           static_cast<uint16_t>(npc_base + npc_stride * 7),  40},  // Scorpion (Type: 17)
    monster_pak_entry{"Zom",           static_cast<uint16_t>(npc_base + npc_stride * 8),  40},  // Zombie (Type: 18)
    monster_pak_entry{"Gandlf",        static_cast<uint16_t>(npc_base + npc_stride * 9),   8},  // Gandalf (Type: 19)
    monster_pak_entry{"Howard",        static_cast<uint16_t>(npc_base + npc_stride * 10),  8},  // Howard (Type: 20)
    monster_pak_entry{"Guard",         static_cast<uint16_t>(npc_base + npc_stride * 11), 40},  // Guard (Type: 21)
    monster_pak_entry{"Amp",           static_cast<uint16_t>(npc_base + npc_stride * 12), 40},  // Amphis (Type: 22)
    monster_pak_entry{"Cla",           static_cast<uint16_t>(npc_base + npc_stride * 13), 40},  // Clay-Golem (Type: 23)
    monster_pak_entry{"tom",           static_cast<uint16_t>(npc_base + npc_stride * 14),  8},  // Tom (Type: 24)
    monster_pak_entry{"William",       static_cast<uint16_t>(npc_base + npc_stride * 15),  8},  // William (Type: 25)
    monster_pak_entry{"Kennedy",       static_cast<uint16_t>(npc_base + npc_stride * 16),  8},  // Kennedy (Type: 26)
    monster_pak_entry{"Helb",          static_cast<uint16_t>(npc_base + npc_stride * 17), 40},  // Hellbound (Type: 27)
    monster_pak_entry{"Troll",         static_cast<uint16_t>(npc_base + npc_stride * 18), 40},  // Troll (Type: 28)
    monster_pak_entry{"Orge",          static_cast<uint16_t>(npc_base + npc_stride * 19), 40},  // Ogre (Type: 29)
    monster_pak_entry{"Liche",         static_cast<uint16_t>(npc_base + npc_stride * 20), 40},  // Liche (Type: 30)
    monster_pak_entry{"Demon",         static_cast<uint16_t>(npc_base + npc_stride * 21), 40},  // Demon (Type: 31)
    monster_pak_entry{"Unicorn",       static_cast<uint16_t>(npc_base + npc_stride * 22), 40},  // Unicorn (Type: 32)
    monster_pak_entry{"WereWolf",      static_cast<uint16_t>(npc_base + npc_stride * 23), 40},  // WereWolf (Type: 33)
    monster_pak_entry{"Dummy",         static_cast<uint16_t>(npc_base + npc_stride * 24), 40},  // Dummy (Type: 34)
    // Type 35 (Energy-Ball) uses Effect5.pak - skipped, handled separately if needed
    monster_pak_entry{"GT-Arrow",      static_cast<uint16_t>(npc_base + npc_stride * 26), 40},  // Arrow-GuardTower (Type: 36)
    monster_pak_entry{"GT-Cannon",     static_cast<uint16_t>(npc_base + npc_stride * 27), 40},  // Cannon-GuardTower (Type: 37)
    monster_pak_entry{"ManaCollector", static_cast<uint16_t>(npc_base + npc_stride * 28), 40},  // Mana Collector (Type: 38)
    monster_pak_entry{"Detector",      static_cast<uint16_t>(npc_base + npc_stride * 29), 40},  // Detector (Type: 39)
    monster_pak_entry{"ESG",           static_cast<uint16_t>(npc_base + npc_stride * 30), 40},  // ESG (Type: 40)
    monster_pak_entry{"GMG",           static_cast<uint16_t>(npc_base + npc_stride * 31), 40},  // GMG (Type: 41)
    monster_pak_entry{"ManaStone",     static_cast<uint16_t>(npc_base + npc_stride * 32), 40},  // ManaStone (Type: 42)
    monster_pak_entry{"LWB",           static_cast<uint16_t>(npc_base + npc_stride * 33), 40},  // Light War Beetle (Type: 43)
    monster_pak_entry{"GHK",           static_cast<uint16_t>(npc_base + npc_stride * 34), 40},  // God's Hand Knight (Type: 44)
    monster_pak_entry{"GHKABS",        static_cast<uint16_t>(npc_base + npc_stride * 35), 40},  // GHK + Battle Steed (Type: 45)
    monster_pak_entry{"TK",            static_cast<uint16_t>(npc_base + npc_stride * 36), 40},  // Temple Knight (Type: 46)
    monster_pak_entry{"BG",            static_cast<uint16_t>(npc_base + npc_stride * 37), 40},  // Battle Golem (Type: 47)
    monster_pak_entry{"Stalker",       static_cast<uint16_t>(npc_base + npc_stride * 38), 40},  // Stalker (Type: 48)
    monster_pak_entry{"Hellclaw",      static_cast<uint16_t>(npc_base + npc_stride * 39), 40},  // Hellclaw (Type: 49)
    monster_pak_entry{"Tigerworm",     static_cast<uint16_t>(npc_base + npc_stride * 40), 40},  // Tigerworm (Type: 50)
    monster_pak_entry{"Catapult",      static_cast<uint16_t>(npc_base + npc_stride * 41), 40},  // Catapult (Type: 51)
    monster_pak_entry{"Gagoyle",       static_cast<uint16_t>(npc_base + npc_stride * 42), 40},  // Gargoyle (Type: 52)
    monster_pak_entry{"Beholder",      static_cast<uint16_t>(npc_base + npc_stride * 43), 40},  // Beholder (Type: 53)
    monster_pak_entry{"DarkElf",       static_cast<uint16_t>(npc_base + npc_stride * 44), 40},  // Dark-Elf (Type: 54)
    monster_pak_entry{"Bunny",         static_cast<uint16_t>(npc_base + npc_stride * 45), 40},  // Bunny (Type: 55)
    monster_pak_entry{"Cat",           static_cast<uint16_t>(npc_base + npc_stride * 46), 40},  // Cat (Type: 56)
    monster_pak_entry{"GiantFrog",     static_cast<uint16_t>(npc_base + npc_stride * 47), 40},  // GiantFrog (Type: 57)
    monster_pak_entry{"MTGiant",       static_cast<uint16_t>(npc_base + npc_stride * 48), 40},  // Mountain Giant (Type: 58)
    monster_pak_entry{"Ettin",         static_cast<uint16_t>(npc_base + npc_stride * 49), 40},  // Ettin (Type: 59)
    monster_pak_entry{"CanPlant",      static_cast<uint16_t>(npc_base + npc_stride * 50), 40},  // Cannibal Plant (Type: 60)
    monster_pak_entry{"Rudolph",       static_cast<uint16_t>(npc_base + npc_stride * 51), 40},  // Rudolph (Type: 61)
    monster_pak_entry{"DireBoar",      static_cast<uint16_t>(npc_base + npc_stride * 52), 40},  // Dire Boar (Type: 62)
    monster_pak_entry{"frost",         static_cast<uint16_t>(npc_base + npc_stride * 53), 40},  // Frost (Type: 63)
    monster_pak_entry{"Crop",          static_cast<uint16_t>(npc_base + npc_stride * 54), 40},  // Crop (Type: 64)
    monster_pak_entry{"IceGolem",      static_cast<uint16_t>(npc_base + npc_stride * 55), 40},  // Ice Golem (Type: 65)
    monster_pak_entry{"Wyvern",        static_cast<uint16_t>(npc_base + npc_stride * 56), 40},  // Wyvern (Type: 66)
    monster_pak_entry{"McGaffin",      static_cast<uint16_t>(npc_base + npc_stride * 57), 16},  // McGaffin (Type: 67)
    monster_pak_entry{"Perry",         static_cast<uint16_t>(npc_base + npc_stride * 58), 16},  // Perry (Type: 68)
    monster_pak_entry{"Devlin",        static_cast<uint16_t>(npc_base + npc_stride * 59), 16},  // Devlin (Type: 69)
};

} // anonymous namespace

bool game_state_manager::initialize(renderer& rend, audio& aud) {
    audio_ = &aud;
    renderer_ = &rend;

    // Synchronous critical initialization
    if (!network_.initialize()) {
        spdlog::error("Failed to initialize network");
        return false;
    }
    entities_.initialize();
    sprites_.initialize("assets/");

    // Apply debug stats visibility from config
    debug::debug_stats::instance().set_visible(config::instance().video().show_debug_stats);

    // Queue async initialization steps
    init_steps_.clear();
    init_step_index_ = 0;

    init_steps_.push_back({"Loading interface sprites...", [this]() {
        if (sprites_.load_pak("interface", "sprites/interface.pak")) {
            spdlog::info("Loaded interface.pak");
            sprites_.store_sprite_at_id(0, "interface", 0);
        }
    }});

    init_steps_.push_back({"Loading dialog sprites...", [this]() {
        if (sprites_.load_pak("interface2", "sprites/interface2.pak"))
            spdlog::info("Loaded interface2.pak");
    }});
    init_steps_.push_back({"Loading dialog sprites...", [this]() {
        if (sprites_.load_pak("New-Dialog", "sprites/New-Dialog.pak")) {
            spdlog::info("Loaded New-Dialog.pak");
            sprites_.store_sprite_at_id(51, "New-Dialog", 0);
            sprites_.store_sprite_at_id(52, "New-Dialog", 1);
            sprites_.store_sprite_at_id(54, "New-Dialog", 2);
        }
    }});
    init_steps_.push_back({"Loading dialog sprites...", [this]() {
        if (sprites_.load_pak("LoginDialog", "sprites/LoginDialog.pak")) {
            spdlog::info("Loaded LoginDialog.pak");
            sprites_.store_sprite_at_id(53, "LoginDialog", 0);
        }
    }});
    init_steps_.push_back({"Loading dialog sprites...", [this]() {
        if (sprites_.load_pak("GameDialog", "sprites/GameDialog.pak")) {
            spdlog::info("Loaded GameDialog.pak");
            sprites_.store_sprite_at_id(57, "GameDialog", 8);
            sprites_.store_sprite_at_id(58, "GameDialog", 9);
            sprites_.store_sprite_at_id(60, "GameDialog", 0);
            sprites_.store_sprite_at_id(61, "GameDialog", 1);
            sprites_.store_sprite_at_id(62, "GameDialog", 2);
            sprites_.store_sprite_at_id(63, "GameDialog", 3);
        }
    }});
    init_steps_.push_back({"Loading dialog sprites...", [this]() {
        if (sprites_.load_pak("DialogText", "sprites/DialogText.pak")) {
            spdlog::info("Loaded DialogText.pak");
            sprites_.store_sprite_at_id(71, "DialogText", 1);
        }
    }});

    init_steps_.push_back({"Initializing UI...", [this]() {
        ui_.set_sprite_manager(&sprites_);
        ui_.initialize();
    }});

    init_steps_.push_back({"Initializing game systems...", [this]() { inventory_.initialize(); }});
    init_steps_.push_back({"Initializing game systems...", [this]() { magic_.initialize(); }});
    init_steps_.push_back({"Initializing game systems...", [this]() { skills_.initialize(); }});
    init_steps_.push_back({"Initializing game systems...", [this]() {
        combat_.initialize(&entities_, &magic_, &skills_, &sounds_, &inventory_);
    }});

    init_steps_.push_back({"Initializing effects...", [this]() {
        effects_.initialize(sprites_, sounds_, world_);
        magic_.set_effect_system(&effects_);
    }});

    init_steps_.push_back({"Setting up network...", [this]() {
        setup_network_handlers();
        action_queue_.initialize(*this);
        input_handler_.initialize(*this);
        dialog_callbacks_.initialize(*this);
        ws_handler_.initialize(*this);
    }});

    // Each equipment PAK is its own step for fine-grained progress
    for (const auto& entry : menu_character_renderer::get_pak_load_list()) {
        init_steps_.push_back({"Loading character sprites...", [this, entry]() {
            std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";
            if (!sprites_.load_pak(entry.pak_name, pak_path)) {
                spdlog::debug("Optional equipment PAK not found: {}", pak_path);
                return;
            }
            for (uint32_t i = 0; i < entry.sprite_count; ++i) {
                uint16_t global_id = static_cast<uint16_t>(entry.sprite_id + i);
                sprites_.store_sprite_at_id(global_id, entry.pak_name, i);
            }
        }});
    }
    init_steps_.push_back({"Loading character sprites...", [this]() {
        menu_char_renderer_.set_initialized();
        spdlog::info("Menu character renderer initialized");
    }});

    // Each monster PAK is its own step for fine-grained progress
    for (const auto& entry : monster_paks) {
        init_steps_.push_back({"Loading monster sprites...", [this, entry]() {
            std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";
            if (!sprites_.load_pak(entry.pak_name, pak_path)) {
                spdlog::debug("Optional monster PAK not found: {}", pak_path);
                return;
            }
            for (uint16_t i = 0; i < entry.sprite_count; ++i) {
                uint16_t global_id = static_cast<uint16_t>(entry.sprite_id + i);
                sprites_.store_sprite_at_id(global_id, entry.pak_name, i);
            }
        }});
    }

    init_steps_.push_back({"Initializing screens...", [this]() {
        screens_.initialize();
        screens_.get_main_menu_screen().set_on_start([this]() {
            change_state(game_state::login);
        });
        screens_.get_main_menu_screen().set_on_quit([this]() {
            change_state(game_state::quit);
        });
        screens_.get_main_menu_screen().set_on_settings([this]() {
            if (auto* settings = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options)))
            {
                settings->set_ui_style(ui_.style());
            }
            ui_.open_dialog(dialog_type::options);
        });
        screens_.get_login_screen().set_on_login([this](const std::string& account, const std::string& password) {
            spdlog::info("Login attempt: {} (pass length: {})", account, password.length());
            attempt_login(account, password);
        });
        screens_.get_login_screen().set_on_cancel([this]() {
            change_state(game_state::main_menu);
        });
        screens_.get_character_select_screen().set_on_select([this](int32_t index) {
            if (index >= 0 && index < static_cast<int32_t>(characters_.size())) {
                int32_t character_id = characters_[index].id;
                spdlog::info("Entering game with character '{}' (ID: {})", characters_[index].name, character_id);
                ws_handler_.request_enter_game(character_id);
            }
        });
        screens_.get_character_select_screen().set_on_create([this]() {
            spdlog::info("Opening character creation");
            change_state(game_state::create_character);
        });
        screens_.get_character_select_screen().set_on_delete([this](int32_t index) {
            if (index >= 0 && index < static_cast<int32_t>(characters_.size()))
            {
                int32_t character_id = characters_[index].id;
                spdlog::info("Deleting character '{}' (ID: {})", characters_[index].name, character_id);
                ws_handler_.request_delete_character(character_id);
            }
        });
        screens_.get_character_select_screen().set_on_logout([this]() {
            spdlog::info("Logging out to main menu");
            ws_connection_.disconnect();
            change_state(game_state::main_menu);
        });
        screens_.get_character_create_screen().set_on_create([this](const character_create_data& data) {
            spdlog::info("Creating character: name='{}' gender={} stats={}/{}/{}/{}/{}/{}",
                         data.name, data.gender, data.strength, data.vitality, data.dexterity,
                         data.intelligence, data.magic, data.charisma);
            ws_handler_.request_create_character(data);
        });
        screens_.get_character_create_screen().set_on_cancel([this]() {
            spdlog::info("Canceling character creation, returning to character select");
            change_state(game_state::select_character);
        });
        screens_.get_connection_lost_screen().set_on_timeout([this]() {
            spdlog::info("Connection lost timeout, returning to main menu");
            ws_connection_.disconnect();
            change_state(game_state::main_menu);
        });
        screens_.get_character_select_screen().set_character_renderer(&menu_char_renderer_);
        screens_.get_character_create_screen().set_character_renderer(&menu_char_renderer_);
        auto play_ui_sound = [this]() { sounds_.play_ui_sound(14); };
        screens_.get_main_menu_screen().set_on_button_sound(play_ui_sound);
        screens_.get_login_screen().set_on_button_sound(play_ui_sound);
        screens_.get_character_select_screen().set_on_button_sound(play_ui_sound);
        screens_.get_character_create_screen().set_on_button_sound(play_ui_sound);
    }});

    // Each dialog is its own step for fine-grained progress
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_character_select_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_character_create_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_character_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_inventory_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_equipment_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_spellbook_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_skills_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_chat_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_shop_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_bank_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_party_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_guild_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_npc_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_trade_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_craft_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_map_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_repair_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_help_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_system_menu_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_options_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() {
        ui_.load_dialog_definitions_from_directory("assets/ui/dialogs");
    }});
    init_steps_.push_back({"Creating dialogs...", [this]() {
        ui_.create_icon_panel_dialog();
        if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
            yaml_dlg->set_screen_size(renderer_->width(), renderer_->height());
        } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
            icon_dlg->set_screen_size(renderer_->width(), renderer_->height());
        }
    }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_gauge_panel_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { ui_.create_levelup_dialog(); }});
    init_steps_.push_back({"Creating dialogs...", [this]() { dialog_callbacks_.setup_callbacks(); }});

    // Wire chat input overlay
    init_steps_.push_back({"Setting up chat...", [this]() {
        chat_input_.set_on_send([this](std::string_view content, std::string_view channel,
                                       std::string_view recipient) {
            // Send via WebSocket
            ws_handler_.send_chat_message(content, channel, recipient);

            // Local echo
            chat_message msg;
            if (channel == "shout") msg.type = chat_type::shout;
            else if (channel == "whisper") msg.type = chat_type::whisper;
            else if (channel == "guild") msg.type = chat_type::guild;
            else if (channel == "party") msg.type = chat_type::party;
            else if (channel == "faction") msg.type = chat_type::faction;
            else if (channel == "gm") msg.type = chat_type::gm;
            else if (channel == "trade") msg.type = chat_type::trade;
            else msg.type = chat_type::normal;

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

        chat_input_.set_on_command([this](std::string_view command) {
            ws_handler_.send_chat_message(command, "command", "");
        });

        chat_input_.set_type_to_chat(config::instance().game().type_to_chat);
    }});

    // Tile sprites: register core mappings + discover tile PAKs, then queue each load
    // initialize_core and discover are fast (no I/O), so run them now at queue time
    tile_registry_.initialize_core(sprites_, "sprites/");
    auto tile_paks = tile_registry_.discover_tile_pak_list();
    for (auto& tp : tile_paks) {
        init_steps_.push_back({"Loading tile sprites...", [this, entry = std::move(tp)]() {
            tile_registry_.load_tile_pak(entry);
        }});
    }

    // World initialization (depends on tile registry being populated)
    init_steps_.push_back({"Initializing world...", [this]() {
        if (tile_registry_.registered_count() > 0)
        {
            if (!world_.initialize(tile_registry_))
                spdlog::warn("World initialization failed - terrain rendering may not work");
            const auto& video = config::instance().video();
            world_.set_screen_size(video.screen_width, video.screen_height);
            spdlog::info("Tile sprite registry: {} mappings", tile_registry_.registered_count());
        }
        else
        {
            spdlog::warn("Tile sprite registry empty - terrain rendering disabled");
        }
    }});

    init_steps_.push_back({"Initializing audio...", [this]() {
        if (sounds_.initialize(*audio_))
        {
            sounds_.set_sfx_enabled(config::instance().audio().sfx_enabled);
            sounds_.set_music_enabled(config::instance().audio().music_enabled);
            spdlog::info("Sound manager initialized with sfx={}, music={}",
                         sounds_.is_sfx_enabled(), sounds_.is_music_enabled());
            entities_.set_sound_manager(&sounds_);
        }
        else
        {
            spdlog::warn("Sound manager initialization failed - audio may not work");
        }
    }});

    init_steps_.push_back({"Finalizing...", [this]() {
        world_.set_events({
            .on_map_changed = [this](std::string_view /*old_map*/, std::string_view new_map) {
                sounds_.start_bgm(new_map, static_cast<int>(world_.weather()));
                if (state_ == game_state::playing && renderer_)
                {
                    transition_.set_show_label(false);
                    transition_.randomize_type();
                    transition_.start_reveal(renderer_->width(), renderer_->height());
                }
            },
            .on_weather_changed = [this](weather_type w) {
                int weather_int = static_cast<int>(w);
                if (weather_int >= 4 && weather_int <= 6)
                {
                    sounds_.start_bgm(world_.current_map_name(), weather_int);
                }
            },
            .on_time_changed = nullptr
        });
    }});

    init_steps_.push_back({"Ready!", [this]() {
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

bool game_state_manager::has_pending_init_steps() const {
    return init_step_index_ < init_steps_.size();
}

std::pair<float, std::string> game_state_manager::run_next_init_step() {
    if (init_step_index_ >= init_steps_.size())
        return {1.0f, "Ready!"};

    auto& step = init_steps_[init_step_index_];
    std::string msg = step.message;
    step.action();
    ++init_step_index_;

    float progress = static_cast<float>(init_step_index_) / static_cast<float>(init_steps_.size());
    return {progress, msg};
}

void game_state_manager::update(float delta_time, const input& inp) {
    // Handle state transitions
    if (state_transition_) {
        exit_state(state_);
        state_ = pending_state_;
        enter_state(state_);
        state_transition_ = false;
    }

    // Update network
    network_.update();
    ws_connection_.update(delta_time);

    // Poll for WebSocket messages on main thread
    while (auto msg = ws_connection_.receive()) {
        ws_handler_.handle_message(*msg);
    }

    // Handle pending connect event
    if (ws_connection_.poll_connect_event()) {
        spdlog::info("WebSocket connected (polled on main thread)");
        std::string username, password;
        if (ws_handler_.consume_pending_login(username, password)) {
            spdlog::info("Sending login request for user: {}", username);
            json login_msg = make_login_request(username, password);
            ws_connection_.send(login_msg);
        }
    }

    // Handle pending disconnect event
    std::string disconnect_reason;
    if (ws_connection_.poll_disconnect_event(disconnect_reason)) {
        spdlog::warn("WebSocket disconnected (polled on main thread): {}", disconnect_reason);
        ws_handler_.clear_session();
        characters_.clear();

        if (state_ != game_state::main_menu && state_ != game_state::connection_lost) {
            screens_.get_connection_lost_screen().set_reason(
                disconnect_reason.empty() ? "Server disconnected" : disconnect_reason);
            change_state(game_state::connection_lost);
        }
    }

    // Update sprite memory management
    sprites_.update_memory(delta_time);

#ifdef HB_DEBUG_OVERLAY_ENABLED
    auto& debug_overlay = debug::debug_overlay::instance();
    debug_overlay.update(delta_time, inp);
    bool debug_consumed_input = debug_overlay.consumed_mouse_input() ||
                                 debug_overlay.consumed_keyboard_input();
#else
    bool debug_consumed_input = false;
#endif

    if (!debug_consumed_input) {
        switch (state_) {
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

void game_state_manager::render(renderer& rend) {
    switch (state_) {
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

void game_state_manager::change_state(game_state new_state) {
    if (state_ == new_state) return;

    spdlog::info("State change requested: {} -> {}",
        static_cast<int>(state_), static_cast<int>(new_state));

    pending_state_ = new_state;
    state_transition_ = true;
}

void game_state_manager::enter_state(game_state state) {
    spdlog::debug("Entering state: {}", static_cast<int>(state));

    switch (state) {
        case game_state::main_menu:
            ui_.close_all_dialogs();
            screens_.change_screen(screen_type::main_menu);
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
            transition_.set_show_label(false);
            transition_.randomize_type();
            transition_.start_reveal(renderer_->width(), renderer_->height());
            ui_.open_dialog(dialog_type::icon_panel);
            if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
                yaml_dlg->set_screen_size(renderer_->width(), renderer_->height());
            } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
                icon_dlg->set_screen_size(renderer_->width(), renderer_->height());
            }
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

void game_state_manager::exit_state(game_state state) {
    spdlog::debug("Exiting state: {}", static_cast<int>(state));

    switch (state) {
        case game_state::playing:
            sounds_.stop_bgm();
            clear_game_data();
            break;

        default:
            break;
    }
}

void game_state_manager::clear_game_data() {
    spdlog::info("Clearing game data (map, entities, combat, inventory)");

    entities_.remove_all_entities();
    entities_.set_local_player(invalid_entity_id);
    world_.unload_map();
    world_.set_cinematic_mode(false);
    world_.set_global_render_mode(false);
    world_.set_zoom_mode_enabled(false);

    // Clear extracted subsystems
    action_queue_.clear();
    input_handler_.clear();
    ws_handler_.clear();

    // Clear visual effects, status log, and floating text
    effects_.clear();
    status_log_.clear();
    floating_text_.clear();
    quests_.clear();
    spell_hotbar_.fill(0);
}

void game_state_manager::start_map_transition(std::function<void()> on_midpoint)
{
    if (renderer_)
    {
        transition_.start_full(renderer_->width(), renderer_->height(), std::move(on_midpoint));
    }
}

void game_state_manager::update_main_menu(float delta_time, const input& inp) {
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_login(float delta_time, const input& inp) {
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_character_select(float delta_time, const input& inp) {
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_character_create(float delta_time, const input& inp) {
    screens_.update_mouse_position(inp);
    if (!ui_.is_modal_open()) {
        screens_.update(delta_time, inp);
    }
}

void game_state_manager::update_loading(float delta_time, const input& inp) {
    (void)inp;

    loading_progress_ += delta_time * 0.5f;
    if (loading_progress_ >= 1.0f) {
        loading_progress_ = 1.0f;
        change_state(game_state::playing);
    }
}

void game_state_manager::update_playing(float delta_time, const input& inp) {
    // Map display-space mouse to scene-space for game world interactions
    auto [scene_mx, scene_my] = renderer_->display_to_scene(inp.mouse_x(), inp.mouse_y());

    // Convert scene-space mouse position to view coordinates for entity hover detection
    auto [mouse_world_x, mouse_world_y] = world_.screen_to_world(scene_mx, scene_my);
    input_handler_.set_mouse_position(
        mouse_world_x - world_.camera_x(),
        mouse_world_y - world_.camera_y());

    // Update blocked movement cooldown
    action_queue_.update_cooldown(delta_time);

    // Chat input overlay runs first - when active it consumes keyboard input.
    // Skip overlay activation when a dialog has text focus (e.g. chat search).
    bool chat_active = false;
    if (!ui_.has_text_focus())
        chat_active = chat_input_.update(delta_time, inp);

    // Handle game input only when chat overlay is not active
    if (!chat_active && !ui_.has_text_focus())
        input_handler_.handle_input(inp);

    // Track alt key state for super attack indicator
    bool alt_held = inp.is_key_down(sf::Keyboard::Key::LAlt) ||
                    inp.is_key_down(sf::Keyboard::Key::RAlt);
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_alt_held(alt_held);
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        icon_dlg->set_alt_held(alt_held);
    }

    // Mouse wheel zoom (use scene-space coordinates for anchor)
    if (world_.is_zoom_mode_enabled()) {
        int32_t wheel = inp.wheel_delta();
        if (wheel != 0) {
            if (world_.is_cinematic_mode()) {
                world_.adjust_zoom(static_cast<float>(-wheel) * 0.1f, scene_mx, scene_my);
            } else {
                world_.adjust_zoom(static_cast<float>(-wheel) * 0.1f);
            }
        }
    }

    // Update world
    world_.update(delta_time);

    // Update entities
    entities_.update(delta_time, world_, input_handler_.is_combat_mode());

    // Process queued actions
    action_queue_.process_pending();

    // Update camera to follow local player
    if (entity* player = local_player()) {
        const auto& transform = player->transform();
        world_.set_player_position(transform.x, transform.y);
        sounds_.set_listener_position(transform.x, transform.y);
    }

    // Update combat
    combat_.update(delta_time);
    magic_.update_effects(delta_time);
    effects_.update(delta_time);
    skills_.update_cooldowns(delta_time);

    // Update HUD
    update_icon_panel();

    // Update debug stats
    auto& debug_stats = debug::debug_stats::instance();
    debug_stats.update(delta_time);
    if (debug_stats.visible()) {
        int32_t cam_x = world_.camera_x();
        int32_t cam_y = world_.camera_y();
        debug_stats.set_camera_bounds(
            cam_x, cam_y,
            cam_x + static_cast<int32_t>(renderer_->width()),
            cam_y + static_cast<int32_t>(renderer_->height())
        );

        if (entity* player = local_player()) {
            const auto& transform = player->transform();
            debug_stats.set_player_position(
                transform.tile_x, transform.tile_y,
                transform.x, transform.y
            );

            // Player movement state
            auto dir_to_str = [](direction d) -> const char* {
                switch (d) {
                    case direction::north:      return "N";
                    case direction::north_east:  return "NE";
                    case direction::east:        return "E";
                    case direction::south_east:  return "SE";
                    case direction::south:       return "S";
                    case direction::south_west:  return "SW";
                    case direction::west:        return "W";
                    case direction::north_west:  return "NW";
                    default:                     return "?";
                }
            };
            bool running = player->has_movement() && player->movement().running;
            debug_stats.set_player_movement(
                dir_to_str(transform.facing),
                transform.moving,
                transform.move_progress,
                running
            );

            // Player stats
            if (player->has_stats()) {
                const auto& stats = player->stats();
                debug_stats.set_player_stats(
                    stats.hp, stats.max_hp,
                    stats.mp, stats.max_mp,
                    stats.level
                );
            }
        }

        debug_stats.set_entity_count(static_cast<int32_t>(entities_.entity_count()));
        debug_stats.set_map_name(std::string(world_.current_map_name()));
        debug_stats.set_network_connected(ws_connection_.is_connected());
        debug_stats.set_ping(ws_connection_.ping_ms());
        debug_stats.set_network_message_totals(
            ws_connection_.messages_received(), ws_connection_.messages_sent());
        debug_stats.set_sprite_cache_count(static_cast<int32_t>(sprites_.cached_sprite_count()));
        debug_stats.set_pak_files_loaded(static_cast<int32_t>(sprites_.loaded_pak_count()));

        // World state
        debug_stats.set_zoom_level(world_.zoom_level());
        debug_stats.set_chunk_count(static_cast<int32_t>(world_.chunk_count()));

        auto weather_to_str = [](weather_type w) -> const char* {
            switch (w) {
                case weather_type::clear: return "Clear";
                case weather_type::rain:  return "Rain";
                case weather_type::snow:  return "Snow";
                case weather_type::storm: return "Storm";
                default:                  return "Unknown";
            }
        };
        auto time_to_str = [](time_of_day t) -> const char* {
            switch (t) {
                case time_of_day::dawn:      return "Dawn";
                case time_of_day::morning:   return "Morning";
                case time_of_day::noon:      return "Noon";
                case time_of_day::afternoon: return "Afternoon";
                case time_of_day::dusk:      return "Dusk";
                case time_of_day::night:     return "Night";
                case time_of_day::midnight:  return "Midnight";
                default:                     return "Unknown";
            }
        };
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

        entity* hovered = entities_.get_entity_at_screen_pos(
            input_handler_.mouse_x(), input_handler_.mouse_y(), cam_x, cam_y);
        if (hovered && hovered->has_name()) {
            debug_stats.set_hovered_entity(hovered->name().name + " (ID:" + std::to_string(hovered->id()) + ")");
        } else {
            debug_stats.set_hovered_entity("");
        }

        // Audio
        if (audio_) {
            debug_stats.set_active_sounds(static_cast<int32_t>(audio_->active_sound_count()));
        }
        debug_stats.set_bgm_track(std::string(sounds_.current_bgm_track()));

        // UI
        debug_stats.set_open_dialog_count(static_cast<int32_t>(ui_.open_dialog_count()));

        debug_stats.set_game_state("Playing");
        debug_stats.set_combat_mode(input_handler_.is_combat_mode(), input_handler_.is_safe_attack_mode());
    }

    // Update status log and floating text
    status_log_.update(delta_time);
    floating_text_.update(delta_time);
}

void game_state_manager::render_main_menu(renderer& rend) {
    if (sprites_.has_sprite_at_id(52)) {
        screens_.render(rend, sprites_);
    } else {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("HELBREATH", screen_width / 2 - 60, 150, sf::Color::White, 24);
        rend.draw_text("Click to Start (sprites not loaded)", screen_width / 2 - 100, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_login(renderer& rend) {
    if (sprites_.has_sprite_at_id(53)) {
        screens_.render(rend, sprites_);
    } else {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("LOGIN (sprites not loaded)", screen_width / 2 - 80, 100, sf::Color::White, 16);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_character_select(renderer& rend) {
    if (sprites_.has_sprite_at_id(57)) {
        screens_.render(rend, sprites_);
    } else {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("CHARACTER SELECT (sprites not loaded)", screen_width / 2 - 120, 100, sf::Color::White);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_character_create(renderer& rend) {
    if (sprites_.has_sprite_at_id(58)) {
        screens_.render(rend, sprites_);
    } else {
        rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 40), true);
        rend.draw_text("CHARACTER CREATE (sprites not loaded)", screen_width / 2 - 120, 100, sf::Color::White);
        rend.draw_text("Press ESC to go back", screen_width / 2 - 70, 300, sf::Color(150, 150, 150));
    }
}

void game_state_manager::render_loading(renderer& rend) {
    rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(10, 10, 20), true);

    int32_t bar_width = 400;
    int32_t bar_height = 20;
    int32_t bar_x = (screen_width - bar_width) / 2;
    int32_t bar_y = screen_height / 2;

    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(40, 40, 60), true);
    rend.draw_rect(bar_x, bar_y, static_cast<int32_t>(bar_width * loading_progress_), bar_height,
                   sf::Color(100, 150, 200), true);
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(80, 80, 120), false);
    rend.draw_text(loading_message_, bar_x, bar_y - 30, sf::Color::White);
}

void game_state_manager::render_playing(renderer& rend) {
    rend.begin_scene();  // Scaled: redirect to scene_rt_. Others: no-op.

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
        const sf::IntRect* bounds_for_row = (have_player_bounds && player_tile_y <= row)
            ? &player_bounds : nullptr;
        world_.render_objects_row(rend, row, bounds_for_row);
    }

    // Draw any remaining entities past the last visible row
    while (idx < sorted.size())
    {
        entities_.render_single_entity(rend, sprites_, *sorted[idx], cam_x, cam_y, mouse_x, mouse_y);
        ++idx;
    }

    effects_.render(rend, cam_x, cam_y);
    world_.reset_zoom_view(rend);

    floating_text_.render(rend, cam_x, cam_y);

    // Provide camera info for fog overlay styles that need tile alignment
    rend.set_fog_camera_info(cam_x, cam_y, world_.zoom_level());

    rend.end_scene();    // Scaled: composite. Extended: fog overlay. Special: no-op.

    rend.begin_ui();     // Switch to native resolution for UI overlay

    // Update rendering stats
    auto& ds = debug::debug_stats::instance();
    int32_t visible_tiles = (range.end_x - range.start_x) * (range.end_y - range.start_y);
    ds.set_tiles_rendered(visible_tiles);
    ds.set_sprites_rendered(static_cast<int32_t>(sorted.size()));
    ds.set_objects_rendered(world_.objects_rendered());

    ds.render(rend);
    status_log_.render(rend, static_cast<int32_t>(rend.display_width()),
                       static_cast<int32_t>(rend.display_height()), 70);

    // Chat input overlay (always on top of game, above icon panel)
    chat_input_.render(rend, static_cast<int32_t>(rend.display_width()),
                       static_cast<int32_t>(rend.display_height()));
}

void game_state_manager::setup_network_handlers() {
    notify_handler_.initialize(*this);
    motion_handler_.initialize(*this);

    network_.register_handler(msg_response_log,
        [this](packet_reader& r) { handle_login_response(r); });

    network_.register_handler(msg_response_enter_game,
        [this](packet_reader& r) { handle_enter_game(r); });

    network_.register_handler(msg_response_player_data,
        [this](packet_reader& r) { handle_player_data(r); });

    network_.register_handler(msg_notify,
        [this](packet_reader& r) {
            auto subtype = r.read_u16();
            if (!subtype) return;
            notify_handler_.dispatch(*subtype, r);
        });

    network_.register_handler(msg_response_motion,
        [this](packet_reader& r) { motion_handler_.handle_motion_response(r); });

    network_.register_handler(msg_event_motion,
        [this](packet_reader& r) { motion_handler_.handle_motion_event(r); });

    network_.register_handler(msg_event_common,
        [this](packet_reader& r) { motion_handler_.handle_common_event(r); });

    network_.register_handler(msg_player_character_contents,
        [this](packet_reader& r) { handle_character_list(r); });
}

void game_state_manager::handle_login_response(packet_reader& reader) {
    auto result_type = reader.read_u16();
    if (!result_type) return;

    if (*result_type == login_response::confirm) {
        spdlog::info("Login successful");
        network_.request_character_list();
    } else if (*result_type == login_response::password_mismatch) {
        show_error("Invalid password");
    } else if (*result_type == login_response::not_existing_account) {
        show_error("Account does not exist");
    } else if (*result_type == login_response::account_locked) {
        show_error("Account is locked");
    } else if (*result_type == login_response::service_not_available) {
        show_error("Service not available");
    } else {
        show_error("Login failed");
    }
}

void game_state_manager::handle_character_list(packet_reader& reader) {
    auto count = reader.read_u8();
    if (!count) return;

    characters_.clear();
    for (uint8_t i = 0; i < *count; ++i) {
        character_info info;
        if (auto name = reader.read_string(20)) {
            info.name = *name;
        }
        if (auto level = reader.read_u16()) {
            info.level = *level;
        }
        characters_.push_back(info);
    }

    change_state(game_state::select_character);
}

void game_state_manager::handle_enter_game(packet_reader& reader) {
    auto result = reader.read_u8();
    if (!result || *result != 0) {
        show_error("Failed to enter game");
        return;
    }

    auto host = reader.read_string(64);
    auto port = reader.read_u16();

    if (host && port) {
        if (network_.connect_game_server(*host, *port)) {
            change_state(game_state::loading);
        } else {
            show_error("Failed to connect to game server");
        }
    }
}

void game_state_manager::handle_player_data(packet_reader& reader) {
    (void)reader;
}

void game_state_manager::handle_map_data(packet_reader& reader) {
    (void)reader;
}

void game_state_manager::set_local_player_id(entity_id id) {
    entities_.set_local_player(id);
}

entity* game_state_manager::local_player() {
    return entities_.local_player();
}

const entity* game_state_manager::local_player() const {
    return entities_.local_player();
}

void game_state_manager::set_characters(std::vector<character_info> characters) {
    characters_ = std::move(characters);

    if (state_ == game_state::select_character)
    {
        refresh_character_select_screen();
    }
}

void game_state_manager::select_character(size_t index) {
    if (index < characters_.size()) {
        selected_character_ = index;
    }
}

void game_state_manager::set_login_result(uint16_t result) {
    if (result == login_response::confirm) {
        spdlog::info("Login successful");
    } else {
        spdlog::warn("Login failed: 0x{:04X}", result);
    }
}

void game_state_manager::refresh_character_select_screen() {
    std::vector<char_slot_info> slot_chars;
    for (const auto& ch : characters_) {
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

void game_state_manager::set_character_create_result(uint16_t result) {
    if (result == login_response::new_character_created) {
        spdlog::info("Character created successfully");
        network_.request_character_list();
    } else if (result == login_response::already_existing_character) {
        spdlog::warn("Character name already exists");
        show_error("Character name already exists");
    } else {
        spdlog::warn("Character creation failed: 0x{:04X}", result);
        show_error("Character creation failed");
    }
}

void game_state_manager::show_message(std::string_view message) {
    (void)message;
    ui_.show_connection_dialog(nullptr);
}

void game_state_manager::show_error(std::string_view error) {
    ui_.show_error_dialog(error, nullptr);
}

void game_state_manager::update_icon_panel() {
    entity* player = local_player();
    if (!player) return;

    bool combat_mode = input_handler_.is_combat_mode();
    bool safe_attack_mode = input_handler_.is_safe_attack_mode();

    auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel));
    if (yaml_dlg) {
        if (player->has_stats()) {
            const auto& stats = player->stats();
            yaml_dlg->set_hp(stats.hp, stats.max_hp);
            yaml_dlg->set_mp(stats.mp, stats.max_mp);
            yaml_dlg->set_sp(stats.sp, stats.max_sp);
            int64_t exp_for_next_level = static_cast<int64_t>(stats.level) * 1000;
            yaml_dlg->set_experience(stats.experience, exp_for_next_level, stats.level);
        }
        const auto& transform = player->transform();
        yaml_dlg->set_position(transform.tile_x, transform.tile_y);
        yaml_dlg->set_map_name(world_.current_map_name());
        if (player->has_combat()) {
            yaml_dlg->set_poisoned(player->combat().poisoned);
        }
        bool weapon_mastered = false;
        if (const item* weapon = inventory_.get_equipped(equip_slot::right_hand)) {
            weapon_skill ws = combat_.get_weapon_skill(weapon->type_id);
            weapon_mastered = skills_.is_skill_mastered(static_cast<uint16_t>(ws));
        }
        yaml_dlg->set_super_attack_available(weapon_mastered);
        yaml_dlg->set_combat_mode(combat_mode);
        yaml_dlg->set_safe_attack_mode(safe_attack_mode);
        return;
    }

    auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel));
    if (!icon_dlg) return;

    if (player->has_stats()) {
        const auto& stats = player->stats();
        icon_dlg->set_hp(stats.hp, stats.max_hp);
        icon_dlg->set_mp(stats.mp, stats.max_mp);
        icon_dlg->set_sp(stats.sp, stats.max_sp);
        int64_t exp_for_next_level = static_cast<int64_t>(stats.level) * 1000;
        icon_dlg->set_experience(stats.experience, exp_for_next_level, stats.level);
    }
    const auto& transform = player->transform();
    icon_dlg->set_position(transform.tile_x, transform.tile_y);
    icon_dlg->set_map_name(world_.current_map_name());
    if (player->has_combat()) {
        icon_dlg->set_poisoned(player->combat().poisoned);
    }
    bool weapon_mastered = false;
    if (const item* weapon = inventory_.get_equipped(equip_slot::right_hand)) {
        weapon_skill ws = combat_.get_weapon_skill(weapon->type_id);
        weapon_mastered = skills_.is_skill_mastered(static_cast<uint16_t>(ws));
    }
    icon_dlg->set_super_attack_available(weapon_mastered);
    icon_dlg->set_combat_mode(combat_mode);
    icon_dlg->set_safe_attack_mode(safe_attack_mode);
}

void game_state_manager::set_ui_style(ui_style style) {
    ui_.set_style(style);

    render_mode mode = (style == ui_style::modern) ? render_mode::modern : render_mode::classic;

    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_render_mode(mode);
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
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
    std::sort(learned.begin(), learned.end(), [](const spell* a, const spell* b) {
        if (a->circle != b->circle) return a->circle < b->circle;
        return a->id < b->id;
    });

    size_t slot = 0;
    for (const auto* sp : learned)
    {
        if (slot >= spell_hotbar_.size()) break;
        spell_hotbar_[slot++] = sp->id;
    }

    spdlog::debug("Auto-populated spell hotbar with {} spells", slot);
}

void game_state_manager::attempt_login(const std::string& username, const std::string& password) {
    auto& net_cfg = config::instance().network();
    std::string ws_url = "ws://" + net_cfg.login_server_host + ":" + std::to_string(net_cfg.login_server_port);

    spdlog::info("Connecting to server: {}", ws_url);
    spdlog::info("attempt_login called with username='{}' password_len={}", username, password.length());

    // Store credentials in ws_handler for after connection
    ws_handler_.set_pending_login(username, password);
    spdlog::info("Stored pending credentials: username='{}' password_len={}", username, password.length());

    // If already connected, send login request directly
    if (ws_connection_.is_connected()) {
        spdlog::info("Already connected, sending login request directly for user: {}", username);
        json login_msg = make_login_request(username, password);
        ws_connection_.send(login_msg);
        return;
    }

    if (!ws_connection_.connect(ws_url)) {
        show_error("Failed to connect to server");
        return;
    }

    ui_.show_connection_dialog([this]() {
        ws_connection_.disconnect();
        ws_handler_.clear_session();
        change_state(game_state::main_menu);
    });
}

void game_state_manager::request_pickup(int32_t tile_x, int32_t tile_y) {
    spdlog::info("Requesting pickup at ({}, {})", tile_x, tile_y);

    if (entity* player = local_player()) {
        player->set_action(object_action::get_item);
    }

    json msg = make_player_pickup_request(tile_x, tile_y);
    ws_connection_.send(msg);
}

void game_state_manager::set_view_radius(int16_t radius_x, int16_t radius_y, bool sees_all) {
    view_radius_x_ = radius_x;
    view_radius_y_ = radius_y;
    sees_all_ = sees_all;

    if (!renderer_) return;

    if (sees_all)
    {
        // Player sees everything - use full display as fair zone
        renderer_->set_internal_resolution(renderer_->display_width(), renderer_->display_height());
    }
    else
    {
        // Convert tile radius to pixel dimensions: diameter * tile_size
        uint32_t w = static_cast<uint32_t>(radius_x) * 2 * tile_width;
        uint32_t h = static_cast<uint32_t>(radius_y) * 2 * tile_width;
        renderer_->set_internal_resolution(w, h);
    }

    world_.set_screen_size(renderer_->scene_width(), renderer_->scene_height());
}

void game_state_manager::send_view_range() {
    // Send the interaction area (what the player can see/target).
    // Special: display resolution. Scaled/Extended: fair zone (internal resolution).
    uint32_t w = renderer_ ? renderer_->interaction_width() : config::instance().video().screen_width;
    uint32_t h = renderer_ ? renderer_->interaction_height() : config::instance().video().screen_height;
    json msg = make_set_view_range_request(w, h);
    ws_connection_.send(msg);
    spdlog::info("Sent view range: {}x{}", w, h);
}

bool game_state_manager::change_resolution(uint32_t width, uint32_t height, bool fullscreen,
                                           bool borderless, int32_t monitor_x, int32_t monitor_y) {
    if (!renderer_) {
        spdlog::error("Cannot change resolution: renderer not initialized");
        return false;
    }

    bool settings_was_open = false;
    ui_style current_style = ui_style::classic;
    float music_vol = 1.0f;
    float sound_vol = 1.0f;

    if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
        settings_was_open = settings_dlg->visible();
        if (settings_was_open) {
            current_style = settings_dlg->get_ui_style();
            music_vol = settings_dlg->get_music_volume();
            sound_vol = settings_dlg->get_sound_volume();
            settings_dlg->close();
        }
    }

    if (!renderer_->set_resolution(width, height, fullscreen, borderless, monitor_x, monitor_y)) {
        return false;
    }

    auto& video = config::instance().video();
    video.screen_width = width;
    video.screen_height = height;
    video.fullscreen = fullscreen;
    video.borderless = borderless;

    world_.set_screen_size(renderer_->scene_width(), renderer_->scene_height());

    if (state_ == game_state::playing) {
        send_view_range();
    }

    if (settings_was_open) {
        if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui_.get_dialog(dialog_type::options))) {
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

    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        yaml_dlg->set_screen_size(width, height);
    } else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui_.get_dialog(dialog_type::icon_panel))) {
        icon_dlg->set_screen_size(width, height);
    }

    return true;
}

} // namespace hb
