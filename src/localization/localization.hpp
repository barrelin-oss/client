#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>

namespace hb
{

// Supported languages
enum class language : uint8_t
{
    english = 0,
    korean = 1,
    japanese = 2,
    chinese_simplified = 3,
    chinese_traditional = 4
};

// Get language code string
inline std::string_view get_language_code(language lang)
{
    switch (lang)
    {
    case language::english:
        return "en";
    case language::korean:
        return "ko";
    case language::japanese:
        return "ja";
    case language::chinese_simplified:
        return "zh_cn";
    case language::chinese_traditional:
        return "zh_tw";
    default:
        return "en";
    }
}

// Get language display name
inline std::string_view get_language_name(language lang)
{
    switch (lang)
    {
    case language::english:
        return "English";
    case language::korean:
        return "한국어";
    case language::japanese:
        return "日本語";
    case language::chinese_simplified:
        return "简体中文";
    case language::chinese_traditional:
        return "繁體中文";
    default:
        return "English";
    }
}

// Localization callback for language change
using language_change_callback = std::function<void(language)>;

class localization
{
public:
    localization() = default;
    ~localization() = default;

    // Initialize with default language
    bool initialize(language default_lang = language::english);
    void shutdown();

    // Load language strings from JSON file
    bool load_language(language lang, std::string_view path);
    bool load_language_from_string(language lang, std::string_view json_content);

    // Set current language
    void set_language(language lang);
    language current_language() const { return current_language_; }

    // Get localized string by key
    std::string_view get(std::string_view key) const;

    // Get with fallback to another key
    std::string_view get_or(std::string_view key, std::string_view fallback) const;

    // Check if key exists
    bool has_key(std::string_view key) const;

    // Format string with arguments (simple positional replacement)
    // Format: "Hello {0}, you have {1} messages"
    template<typename... Args> std::string format(std::string_view key, Args&&... args) const
    {
        std::string_view base = get(key);
        if (base.empty())
            return std::string(key);
        return format_impl(base, {to_string(std::forward<Args>(args))...});
    }

    // Get all keys for current language
    std::vector<std::string_view> get_all_keys() const;

    // Language change notification
    void on_language_changed(language_change_callback callback);

    // Get available languages
    const std::vector<language>& available_languages() const { return available_languages_; }

    // Singleton access (optional, for convenience macro)
    static localization& instance();

private:
    std::string format_impl(std::string_view format_str, std::vector<std::string> args) const;

    template<typename T> static std::string to_string(T&& value)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
        {
            return std::forward<T>(value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>)
        {
            return std::string(value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, const char*>)
        {
            return std::string(value);
        }
        else if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
        {
            return std::to_string(value);
        }
        else
        {
            return std::string(value);
        }
    }

    // String storage per language
    using string_map = std::unordered_map<std::string, std::string>;
    std::unordered_map<language, string_map> strings_;

    language current_language_ = language::english;
    std::vector<language> available_languages_;
    std::vector<language_change_callback> callbacks_;

    // Empty string for missing keys
    inline static const std::string empty_string_;
};

// Convenience macro for getting localized strings
#define _(key) hb::localization::instance().get(key)
#define _F(key, ...) hb::localization::instance().format(key, __VA_ARGS__)

} // namespace hb
