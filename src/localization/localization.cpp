#include "localization/localization.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace hb
{

localization& localization::instance()
{
    static localization inst;
    return inst;
}

bool localization::initialize(language default_lang)
{
    strings_.clear();
    available_languages_.clear();
    callbacks_.clear();
    current_language_ = default_lang;

    // Add built-in English strings as fallback
    string_map english_strings;

    // Common UI strings
    english_strings["ui.ok"] = "OK";
    english_strings["ui.cancel"] = "Cancel";
    english_strings["ui.yes"] = "Yes";
    english_strings["ui.no"] = "No";
    english_strings["ui.close"] = "Close";
    english_strings["ui.apply"] = "Apply";
    english_strings["ui.save"] = "Save";
    english_strings["ui.load"] = "Load";
    english_strings["ui.delete"] = "Delete";
    english_strings["ui.back"] = "Back";
    english_strings["ui.next"] = "Next";
    english_strings["ui.confirm"] = "Confirm";

    // Login screen
    english_strings["login.title"] = "Helbreath Login";
    english_strings["login.account"] = "Account";
    english_strings["login.password"] = "Password";
    english_strings["login.login"] = "Login";
    english_strings["login.create_account"] = "Create Account";
    english_strings["login.connecting"] = "Connecting...";
    english_strings["login.failed"] = "Login failed";
    english_strings["login.invalid_credentials"] = "Invalid account or password";
    english_strings["login.server_full"] = "Server is full";
    english_strings["login.banned"] = "Account is banned";
    english_strings["login.already_logged_in"] = "Account is already logged in";

    // Character selection
    english_strings["charsel.title"] = "Select Character";
    english_strings["charsel.create"] = "Create Character";
    english_strings["charsel.delete"] = "Delete Character";
    english_strings["charsel.enter"] = "Enter Game";
    english_strings["charsel.empty_slot"] = "Empty Slot";
    english_strings["charsel.level"] = "Level";
    english_strings["charsel.confirm_delete"] = "Are you sure you want to delete this character?";

    // Character creation
    english_strings["charcreate.title"] = "Create Character";
    english_strings["charcreate.name"] = "Name";
    english_strings["charcreate.gender"] = "Gender";
    english_strings["charcreate.male"] = "Male";
    english_strings["charcreate.female"] = "Female";
    english_strings["charcreate.hair_style"] = "Hair Style";
    english_strings["charcreate.hair_color"] = "Hair Color";
    english_strings["charcreate.skin_color"] = "Skin Color";
    english_strings["charcreate.strength"] = "Strength";
    english_strings["charcreate.vitality"] = "Vitality";
    english_strings["charcreate.dexterity"] = "Dexterity";
    english_strings["charcreate.intelligence"] = "Intelligence";
    english_strings["charcreate.magic"] = "Magic";
    english_strings["charcreate.charisma"] = "Charisma";
    english_strings["charcreate.points_remaining"] = "Points Remaining: {0}";

    // In-game UI
    english_strings["game.inventory"] = "Inventory";
    english_strings["game.equipment"] = "Equipment";
    english_strings["game.skills"] = "Skills";
    english_strings["game.spells"] = "Spells";
    english_strings["game.status"] = "Status";
    english_strings["game.guild"] = "Guild";
    english_strings["game.party"] = "Party";
    english_strings["game.options"] = "Options";
    english_strings["game.quit"] = "Quit";
    english_strings["game.logout"] = "Log Out";
    english_strings["game.help"] = "Help";

    // Stats
    english_strings["stat.hp"] = "HP";
    english_strings["stat.mp"] = "MP";
    english_strings["stat.sp"] = "SP";
    english_strings["stat.exp"] = "EXP";
    english_strings["stat.level"] = "Level";
    english_strings["stat.str"] = "STR";
    english_strings["stat.vit"] = "VIT";
    english_strings["stat.dex"] = "DEX";
    english_strings["stat.int"] = "INT";
    english_strings["stat.mag"] = "MAG";
    english_strings["stat.chr"] = "CHR";

    // Combat
    english_strings["combat.attack"] = "Attack";
    english_strings["combat.magic_attack"] = "Magic Attack";
    english_strings["combat.defense"] = "Defense";
    english_strings["combat.hit_ratio"] = "Hit Ratio";
    english_strings["combat.critical"] = "Critical";
    english_strings["combat.damage"] = "{0} damage";
    english_strings["combat.miss"] = "Miss";
    english_strings["combat.blocked"] = "Blocked";
    english_strings["combat.died"] = "You have died";
    english_strings["combat.killed"] = "You killed {0}";

    // Chat
    english_strings["chat.normal"] = "[Normal]";
    english_strings["chat.shout"] = "[Shout]";
    english_strings["chat.whisper"] = "[Whisper]";
    english_strings["chat.guild"] = "[Guild]";
    english_strings["chat.party"] = "[Party]";
    english_strings["chat.system"] = "[System]";
    english_strings["chat.trade"] = "[Trade]";
    english_strings["chat.global"] = "[Global]";

    // Items
    english_strings["item.weight"] = "Weight: {0}";
    english_strings["item.durability"] = "Durability: {0}/{1}";
    english_strings["item.required_level"] = "Required Level: {0}";
    english_strings["item.required_str"] = "Required STR: {0}";
    english_strings["item.required_dex"] = "Required DEX: {0}";
    english_strings["item.required_int"] = "Required INT: {0}";
    english_strings["item.equipped"] = "Equipped";
    english_strings["item.cannot_equip"] = "Cannot equip this item";

    // Trade
    english_strings["trade.title"] = "Trade";
    english_strings["trade.offer"] = "Offer";
    english_strings["trade.accept"] = "Accept";
    english_strings["trade.decline"] = "Decline";
    english_strings["trade.gold"] = "Gold: {0}";

    // Guild
    english_strings["guild.name"] = "Guild Name";
    english_strings["guild.leader"] = "Guild Master";
    english_strings["guild.members"] = "Members";
    english_strings["guild.create"] = "Create Guild";
    english_strings["guild.join"] = "Join Guild";
    english_strings["guild.leave"] = "Leave Guild";
    english_strings["guild.disband"] = "Disband Guild";

    // Party
    english_strings["party.create"] = "Create Party";
    english_strings["party.join"] = "Join Party";
    english_strings["party.leave"] = "Leave Party";
    english_strings["party.invite"] = "Invite to Party";
    english_strings["party.kick"] = "Kick from Party";

    // System messages
    english_strings["system.disconnected"] = "Disconnected from server";
    english_strings["system.connection_lost"] = "Connection lost";
    english_strings["system.reconnecting"] = "Reconnecting...";
    english_strings["system.server_shutdown"] = "Server is shutting down";
    english_strings["system.maintenance"] = "Server maintenance in progress";

    // Options
    english_strings["options.title"] = "Options";
    english_strings["options.video"] = "Video";
    english_strings["options.audio"] = "Audio";
    english_strings["options.controls"] = "Controls";
    english_strings["options.game"] = "Game";
    english_strings["options.fullscreen"] = "Fullscreen";
    english_strings["options.resolution"] = "Resolution";
    english_strings["options.master_volume"] = "Master Volume";
    english_strings["options.music_volume"] = "Music Volume";
    english_strings["options.sfx_volume"] = "SFX Volume";
    english_strings["options.language"] = "Language";

    // Error messages
    english_strings["error.generic"] = "An error occurred";
    english_strings["error.network"] = "Network error";
    english_strings["error.file_not_found"] = "File not found";
    english_strings["error.invalid_data"] = "Invalid data";

    strings_[language::english] = std::move(english_strings);
    available_languages_.push_back(language::english);

    spdlog::info("Localization initialized with {} built-in strings", strings_[language::english].size());
    return true;
}

void localization::shutdown()
{
    strings_.clear();
    available_languages_.clear();
    callbacks_.clear();
}

bool localization::load_language(language lang, std::string_view path)
{
    std::string path_str{path};
    std::ifstream file{path_str};
    if (!file)
    {
        spdlog::warn("Could not load language file: {}", path);
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    return load_language_from_string(lang, content);
}

bool localization::load_language_from_string(language lang, std::string_view json_content)
{
    try
    {
        auto json = nlohmann::json::parse(json_content);

        string_map strings;

        // Recursively flatten nested JSON into dot-notation keys
        std::function<void(const nlohmann::json&, const std::string&)> flatten =
            [&strings, &flatten](const nlohmann::json& obj, const std::string& prefix)
        {
            for (auto& [key, value] : obj.items())
            {
                std::string full_key = prefix.empty() ? key : prefix + "." + key;
                if (value.is_object())
                {
                    flatten(value, full_key);
                }
                else if (value.is_string())
                {
                    strings[full_key] = value.get<std::string>();
                }
            }
        };

        flatten(json, "");

        strings_[lang] = std::move(strings);

        // Add to available languages if not already present
        if (std::find(available_languages_.begin(), available_languages_.end(), lang) == available_languages_.end())
        {
            available_languages_.push_back(lang);
        }

        spdlog::info("Loaded {} strings for language {}", strings_[lang].size(), get_language_code(lang));
        return true;
    }
    catch (const nlohmann::json::exception& e)
    {
        spdlog::error("Failed to parse language JSON: {}", e.what());
        return false;
    }
}

void localization::set_language(language lang)
{
    if (strings_.find(lang) == strings_.end())
    {
        spdlog::warn("Language {} not loaded, keeping current language", get_language_code(lang));
        return;
    }

    if (current_language_ != lang)
    {
        current_language_ = lang;
        spdlog::info("Language changed to {}", get_language_code(lang));

        // Notify callbacks
        for (auto& callback : callbacks_)
        {
            if (callback)
            {
                callback(lang);
            }
        }
    }
}

std::string_view localization::get(std::string_view key) const
{
    // Try current language first
    auto lang_it = strings_.find(current_language_);
    if (lang_it != strings_.end())
    {
        auto str_it = lang_it->second.find(std::string(key));
        if (str_it != lang_it->second.end())
        {
            return str_it->second;
        }
    }

    // Fall back to English
    if (current_language_ != language::english)
    {
        auto en_it = strings_.find(language::english);
        if (en_it != strings_.end())
        {
            auto str_it = en_it->second.find(std::string(key));
            if (str_it != en_it->second.end())
            {
                return str_it->second;
            }
        }
    }

    // Return empty if not found
    return empty_string_;
}

std::string_view localization::get_or(std::string_view key, std::string_view fallback) const
{
    auto result = get(key);
    return result.empty() ? fallback : result;
}

bool localization::has_key(std::string_view key) const
{
    auto lang_it = strings_.find(current_language_);
    if (lang_it != strings_.end())
    {
        return lang_it->second.count(std::string(key)) > 0;
    }
    return false;
}

std::vector<std::string_view> localization::get_all_keys() const
{
    std::vector<std::string_view> keys;

    auto lang_it = strings_.find(current_language_);
    if (lang_it != strings_.end())
    {
        keys.reserve(lang_it->second.size());
        for (const auto& [key, value] : lang_it->second)
        {
            keys.push_back(key);
        }
    }

    return keys;
}

void localization::on_language_changed(language_change_callback callback)
{
    if (callback)
    {
        callbacks_.push_back(std::move(callback));
    }
}

std::string localization::format_impl(std::string_view format_str, std::vector<std::string> args) const
{
    std::string result(format_str);

    for (size_t i = 0; i < args.size(); ++i)
    {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos)
        {
            result.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }

    return result;
}

} // namespace hb
