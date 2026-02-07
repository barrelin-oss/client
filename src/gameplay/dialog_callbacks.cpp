#include "gameplay/dialog_callbacks.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialog_manager.hpp"
#include "ui/managed_dialog.hpp"
#include "ui/dialogs/icon_panel_dialog.hpp"
#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include "core/config.hpp"
#include "debug/debug_stats.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void dialog_callbacks::initialize(game_state_manager& game)
{
    game_ = &game;
}

void dialog_callbacks::setup_callbacks()
{
    auto& ui = game_->ui();
    auto& network = game_->network();
    auto& inventory = game_->inventory();
    auto& magic = game_->magic();
    auto& sounds = game_->sounds();

    // Login is now handled by sprite-based screen_manager, not widget dialogs
    // The callbacks are set up in initialize() via screens_.get_login_screen()

    // Character select dialog
    if (auto* select_dlg = dynamic_cast<character_select_dialog*>(ui.get_dialog(dialog_type::character_select)))
    {
        select_dlg->set_on_enter([this](int32_t index) {
            const auto& chars = game_->characters();
            if (index >= 0 && index < static_cast<int32_t>(chars.size()))
            {
                int32_t character_id = chars[index].id;
                spdlog::info("Entering game with character '{}' (ID: {})", chars[index].name, character_id);
                game_->request_enter_game(character_id);
            }
        });

        select_dlg->set_on_create([this]() {
            spdlog::info("Opening character creation");
            game_->change_state(game_state::create_character);
        });

        select_dlg->set_on_delete([](int32_t index) {
            spdlog::info("Delete character requested: {}", index);
            // TODO: Implement delete character request
        });
    }

    // Character create dialog
    if (auto* create_dlg = dynamic_cast<character_create_dialog*>(ui.get_dialog(dialog_type::character_create)))
    {
        create_dlg->set_on_create([this](const character_create_dialog::character_data& data) {
            spdlog::info("Creating character: {} ({}, {})",
                data.name, data.warrior ? "Warrior" : "Mage", data.gender == 0 ? "Male" : "Female");
            game_->change_state(game_state::select_character);
        });

        create_dlg->set_on_cancel([this]() {
            game_->change_state(game_state::select_character);
        });
    }

    // Character dialog - stat point allocation
    if (auto* char_dlg = dynamic_cast<character_dialog*>(ui.get_dialog(dialog_type::character_info)))
    {
        char_dlg->set_on_add_stat([&network](int stat_index) {
            network.request_add_stat(stat_index);
        });
    }

    // Inventory dialog - item interactions
    if (auto* inv_dlg = dynamic_cast<inventory_dialog*>(ui.get_dialog(dialog_type::inventory)))
    {
        inv_dlg->set_on_item_click([](int32_t slot) {
            spdlog::debug("Inventory slot {} clicked", slot);
        });

        inv_dlg->set_on_item_right_click([&network](int32_t slot) {
            network.request_use_item(static_cast<uint8_t>(slot));
        });

        inv_dlg->set_on_item_double_click([&network](int32_t slot) {
            network.request_use_item(static_cast<uint8_t>(slot));
        });

        inv_dlg->set_on_item_drag([&network, &inventory](int32_t from_slot, int32_t to_slot) {
            inventory.move_item(from_slot, to_slot);
            network.request_move_item(from_slot, to_slot);
        });
    }

    // Equipment dialog - equip/unequip
    if (auto* equip_dlg = dynamic_cast<equipment_dialog*>(ui.get_dialog(dialog_type::equipment)))
    {
        equip_dlg->set_on_slot_click([](equip_slot slot) {
            spdlog::debug("Equipment slot {} clicked", static_cast<int>(slot));
        });

        equip_dlg->set_on_slot_right_click([&network](equip_slot slot) {
            network.request_unequip(static_cast<uint8_t>(slot));
        });
    }

    // Spellbook dialog - single click to cast
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(ui.get_dialog(dialog_type::spellbook)))
    {
        spell_dlg->set_on_spell_click([&magic](uint16_t spell_id) {
            spdlog::info("Casting spell {} from spellbook", spell_id);
            magic.set_pending_spell(spell_id);
        });
    }

    // Skills dialog - skill usage
    if (auto* skill_dlg = dynamic_cast<skills_dialog*>(ui.get_dialog(dialog_type::skills)))
    {
        skill_dlg->set_on_skill_click([](uint16_t skill_id) {
            spdlog::debug("Skill {} clicked", skill_id);
        });
    }

    // Chat dialog - no callbacks needed; input is handled by chat_input_overlay

    // Shop dialog - buy/sell
    if (auto* shop_dlg = dynamic_cast<shop_dialog*>(ui.get_dialog(dialog_type::shop)))
    {
        shop_dlg->set_on_buy([&network](size_t item_index, uint32_t quantity) {
            network.request_buy(static_cast<uint16_t>(item_index), static_cast<int32_t>(quantity));
        });
    }

    // Bank dialog - deposit/withdraw
    if (auto* bank_dlg = dynamic_cast<bank_dialog*>(ui.get_dialog(dialog_type::bank)))
    {
        bank_dlg->set_on_withdraw([&network](int32_t slot) {
            network.request_bank_withdraw(slot);
        });
    }

    // Party dialog - party actions
    if (auto* party_dlg = dynamic_cast<party_dialog*>(ui.get_dialog(dialog_type::party)))
    {
        party_dlg->set_on_invite([&network](std::string_view name) {
            network.request_party_invite(name);
        });

        party_dlg->set_on_leave([&network]() {
            network.request_party_leave();
        });
    }

    // NPC dialog - conversation responses
    if (auto* npc_dlg = dynamic_cast<npc_dialog*>(ui.get_dialog(dialog_type::npc_dialog)))
    {
        npc_dlg->set_on_option_select([&network](int32_t option_id) {
            network.send_npc_response(option_id);
        });
    }

    // Trade dialog - player trading
    if (auto* trade_dlg = dynamic_cast<trade_dialog*>(ui.get_dialog(dialog_type::trade)))
    {
        trade_dlg->set_on_add_item([&network](int32_t slot) {
            network.request_trade_add_item(slot, slot);
        });

        trade_dlg->set_on_remove_item([&network](int32_t slot) {
            network.request_trade_remove_item(slot);
        });

        trade_dlg->set_on_confirm([&network]() {
            network.request_trade_confirm();
        });

        trade_dlg->set_on_cancel([&network]() {
            network.request_trade_cancel();
        });

        trade_dlg->set_on_set_gold([&network](uint32_t amount) {
            network.request_trade_set_gold(amount);
        });
    }

    // Craft dialog - item manufacturing
    if (auto* craft_dlg = dynamic_cast<craft_dialog*>(ui.get_dialog(dialog_type::manufacture)))
    {
        craft_dlg->set_on_craft([&network](int32_t recipe_index) {
            network.request_craft_item(recipe_index);
        });

        craft_dlg->set_on_add_ingredient([](int32_t slot) {
            spdlog::debug("Adding ingredient to craft slot {}", slot);
        });

        craft_dlg->set_on_remove_ingredient([](int32_t slot) {
            spdlog::debug("Removing ingredient from craft slot {}", slot);
        });
    }

    // Repair dialog - item repair
    if (auto* repair_dlg = dynamic_cast<repair_dialog*>(ui.get_dialog(dialog_type::repair)))
    {
        repair_dlg->set_on_repair([&network](int32_t inventory_slot) {
            network.request_repair_item(inventory_slot);
        });

        repair_dlg->set_on_repair_all([&network]() {
            network.request_repair_all();
        });
    }

    // Map dialog - click on location
    if (auto* map_dlg = dynamic_cast<map_dialog*>(ui.get_dialog(dialog_type::map)))
    {
        map_dlg->set_on_click_location([](int32_t x, int32_t y) {
            spdlog::debug("Map clicked at ({}, {})", x, y);
        });
    }

    // Icon panel - bottom HUD buttons
    // First try YAML-based icon panel
    if (auto* yaml_dlg = dynamic_cast<yaml_icon_panel_dialog*>(ui.get_dialog(dialog_type::icon_panel)))
    {
        yaml_dlg->set_on_character([this]() {
            game_->ui().toggle_dialog(dialog_type::character_info);
        });

        yaml_dlg->set_on_inventory([this]() {
            game_->ui().toggle_dialog(dialog_type::inventory);
        });

        yaml_dlg->set_on_spellbook([this]() {
            game_->ui().toggle_dialog(dialog_type::spellbook);
        });

        yaml_dlg->set_on_skills([this]() {
            game_->ui().toggle_dialog(dialog_type::skills);
        });

        yaml_dlg->set_on_chat_history([this]() {
            game_->ui().toggle_dialog(dialog_type::chat);
        });

        yaml_dlg->set_on_system_menu([this]() {
            auto& dlg_mgr = game_->ui().dialogs();
            auto* sys_menu = dlg_mgr.find_dialog("system_menu");

            if (sys_menu)
            {
                if (sys_menu->is_open())
                {
                    sys_menu->close();
                }
                else
                {
                    sys_menu->open();
                }
            }
            else if (dlg_mgr.get_definition("system_menu"))
            {
                sys_menu = dlg_mgr.open_dialog("system_menu");
                if (sys_menu)
                {
                    setup_system_menu_buttons(sys_menu);
                }
            }
            else
            {
                game_->ui().toggle_dialog(dialog_type::system_menu);
            }
        });

        yaml_dlg->set_on_combat_indicator([this]() {
            game_->toggle_combat_mode();
            spdlog::debug("Combat mode toggled via click: {}", game_->is_combat_mode() ? "attack" : "peace");
        });

        yaml_dlg->set_on_button_sound([&sounds]() { sounds.play_ui_sound(14); });
    }
    // Fallback to code-based icon panel
    else if (auto* icon_dlg = dynamic_cast<icon_panel_dialog*>(ui.get_dialog(dialog_type::icon_panel)))
    {
        icon_dlg->set_ui_style(ui.style());
        icon_dlg->set_sprite_manager(&game_->sprites());

        icon_dlg->set_on_character([this]() {
            game_->ui().toggle_dialog(dialog_type::character_info);
        });

        icon_dlg->set_on_inventory([this]() {
            game_->ui().toggle_dialog(dialog_type::inventory);
        });

        icon_dlg->set_on_spellbook([this]() {
            game_->ui().toggle_dialog(dialog_type::spellbook);
        });

        icon_dlg->set_on_skills([this]() {
            game_->ui().toggle_dialog(dialog_type::skills);
        });

        icon_dlg->set_on_chat_history([this]() {
            game_->ui().toggle_dialog(dialog_type::chat);
        });

        icon_dlg->set_on_system_menu([this]() {
            auto& dlg_mgr = game_->ui().dialogs();
            auto* sys_menu = dlg_mgr.find_dialog("system_menu");

            if (sys_menu)
            {
                if (sys_menu->is_open())
                {
                    sys_menu->close();
                }
                else
                {
                    sys_menu->open();
                }
            }
            else
            {
                sys_menu = dlg_mgr.open_dialog("system_menu");
                if (sys_menu)
                {
                    setup_system_menu_buttons(sys_menu);
                }
            }
        });

        icon_dlg->set_on_combat_indicator([this]() {
            game_->toggle_combat_mode();
            spdlog::debug("Combat mode toggled via click: {}", game_->is_combat_mode() ? "attack" : "peace");
        });

        icon_dlg->set_on_button_sound([&sounds]() { sounds.play_ui_sound(14); });
    }

    // Legacy system menu dialog - keep for fallback
    if (auto* sys_dlg = dynamic_cast<system_menu_dialog*>(ui.get_dialog(dialog_type::system_menu)))
    {
        sys_dlg->set_on_settings([this]() {
            if (auto* settings = dynamic_cast<settings_dialog*>(game_->ui().get_dialog(dialog_type::options)))
            {
                settings->set_ui_style(game_->ui().style());

                // Center the dialog based on current renderer dimensions
                if (auto* rend = game_->get_renderer())
                {
                    auto b = settings->bounds();
                    int32_t new_x = (static_cast<int32_t>(rend->width()) - b.width) / 2;
                    int32_t new_y = (static_cast<int32_t>(rend->height()) - b.height) / 2;
                    settings->set_position(new_x, new_y);
                }
            }
            game_->ui().open_dialog(dialog_type::options);
        });

        sys_dlg->set_on_help([this]() {
            auto* help_dlg = game_->ui().dialogs().create_help_dialog();
            if (help_dlg)
            {
                help_dlg->open();
            }
            else
            {
                game_->ui().open_dialog(dialog_type::help);
            }
        });

        sys_dlg->set_on_logout([this]() {
            spdlog::info("Logout requested");
            game_->ws_connection().disconnect();
            game_->change_state(game_state::login);
        });

        sys_dlg->set_on_exit([this]() {
            spdlog::info("Exit requested");
            game_->change_state(game_state::quit);
        });
    }

    // Settings dialog - game configuration including UI style
    if (auto* settings_dlg = dynamic_cast<settings_dialog*>(ui.get_dialog(dialog_type::options)))
    {
        const auto& video = config::instance().video();
        settings_dlg->set_resolution(video.screen_width, video.screen_height);
        settings_dlg->set_display_mode(video.fullscreen, video.borderless);
        settings_dlg->set_monitor_index(video.monitor_index);
        settings_dlg->set_vsync(video.vsync);
        settings_dlg->set_framerate(video.framerate_limit);
        settings_dlg->set_remember_position(video.remember_position);
        settings_dlg->set_show_debug_stats(video.show_debug_stats);

        const auto& audio_cfg = config::instance().audio();
        settings_dlg->set_music_volume(audio_cfg.music_volume);
        settings_dlg->set_sound_volume(audio_cfg.sfx_volume);

        settings_dlg->set_on_style_change([this](ui_style style) {
            game_->set_ui_style(style);
            spdlog::info("UI style preview: {}", style == ui_style::classic ? "classic" : "modern");
        });

        settings_dlg->set_on_resolution_change([this](uint32_t width, uint32_t height,
                                                       bool fullscreen, bool borderless,
                                                       int32_t monitor_x, int32_t monitor_y) {
            const char* mode = fullscreen ? "fullscreen" : (borderless ? "borderless" : "windowed");
            spdlog::info("Resolution change requested: {}x{} {}", width, height, mode);
            if (game_->change_resolution(width, height, fullscreen, borderless, monitor_x, monitor_y))
            {
                spdlog::info("Resolution changed successfully");
            }
            else
            {
                spdlog::warn("Failed to change resolution");
            }
        });

        settings_dlg->set_on_framerate_change([this](uint32_t fps) {
            spdlog::info("Framerate limit changed to: {}", fps == 0 ? "unlimited" : std::to_string(fps));
            config::instance().video().framerate_limit = fps;
            if (auto* rend = game_->get_renderer(); rend && !config::instance().video().vsync)
            {
                rend->window().setFramerateLimit(fps);
            }
        });

        settings_dlg->set_on_vsync_change([this](bool vsync) {
            spdlog::info("VSync changed to: {}", vsync ? "enabled" : "disabled");
            auto& v = config::instance().video();
            v.vsync = vsync;
            if (auto* rend = game_->get_renderer())
            {
                rend->window().setVerticalSyncEnabled(vsync);
                rend->window().setFramerateLimit(vsync ? 0 : v.framerate_limit);
            }
        });

        settings_dlg->set_on_remember_position_change([](bool remember) {
            spdlog::info("Remember window position: {}", remember ? "enabled" : "disabled");
            config::instance().video().remember_position = remember;
        });

        settings_dlg->set_on_show_debug_stats_change([](bool show) {
            spdlog::info("Debug stats: {}", show ? "ON" : "OFF");
            config::instance().video().show_debug_stats = show;
            debug::debug_stats::instance().set_visible(show);
        });

        settings_dlg->set_type_to_chat(config::instance().game().type_to_chat);
        settings_dlg->set_on_type_to_chat_change([this](bool enabled) {
            spdlog::info("Type to chat: {}", enabled ? "ON" : "OFF");
            config::instance().game().type_to_chat = enabled;
            game_->chat_input().set_type_to_chat(enabled);
        });

        settings_dlg->set_on_music_volume_change([this](float volume) {
            if (auto* a = game_->get_audio())
            {
                a->set_music_volume(volume);
            }
            config::instance().audio().music_volume = volume;
        });

        settings_dlg->set_on_sound_volume_change([this](float volume) {
            if (auto* a = game_->get_audio())
            {
                a->set_sound_volume(volume);
            }
            config::instance().audio().sfx_volume = volume;
        });

        settings_dlg->set_on_apply([]() {
            spdlog::info("Settings applied");
            config::instance().save();
        });
    }
}

void dialog_callbacks::setup_system_menu_buttons(managed_dialog* sys_menu)
{
    sys_menu->on_button_click("btn_settings", [this]() {
        game_->ui().dialogs().close_dialog("system_menu");
        if (auto* settings = dynamic_cast<settings_dialog*>(game_->ui().get_dialog(dialog_type::options)))
        {
            settings->set_ui_style(game_->ui().style());

            // Center the dialog based on current renderer dimensions
            if (auto* rend = game_->get_renderer())
            {
                auto b = settings->bounds();
                int32_t new_x = (static_cast<int32_t>(rend->width()) - b.width) / 2;
                int32_t new_y = (static_cast<int32_t>(rend->height()) - b.height) / 2;
                settings->set_position(new_x, new_y);
            }
        }
        game_->ui().open_dialog(dialog_type::options);
    });
    sys_menu->on_button_click("btn_help", [this]() {
        game_->ui().dialogs().close_dialog("system_menu");
        auto* help_dlg = game_->ui().dialogs().create_help_dialog();
        if (help_dlg)
        {
            help_dlg->open();
        }
        else
        {
            game_->ui().open_dialog(dialog_type::help);
        }
    });
    sys_menu->on_button_click("btn_logout", [this]() {
        game_->ui().dialogs().close_dialog("system_menu");
        spdlog::info("Logout requested");
        game_->ws_connection().disconnect();
        game_->change_state(game_state::login);
    });
    sys_menu->on_button_click("btn_exit", [this]() {
        game_->ui().dialogs().close_dialog("system_menu");
        spdlog::info("Exit game requested");
        game_->change_state(game_state::quit);
    });
}

} // namespace hb
