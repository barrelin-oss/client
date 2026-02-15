#include "graphics/text_shader_cache.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

bool text_shader_cache::initialize()
{
    if (!sf::Shader::isAvailable())
    {
        spdlog::warn("Shaders not available on this system - GPU text effects disabled");
        return false;
    }

    // Load each shader effect
    bool any_loaded = false;
    any_loaded |= load_shader(text_effect::dissolve, "assets/shaders/dissolve.frag");
    any_loaded |= load_shader(text_effect::chromatic, "assets/shaders/chromatic.frag");
    any_loaded |= load_shader(text_effect::glow, "assets/shaders/glow.frag");
    any_loaded |= load_shader(text_effect::distortion, "assets/shaders/distortion.frag");
    any_loaded |= load_shader(text_effect::scanlines, "assets/shaders/scanlines.frag");

    if (any_loaded)
    {
        spdlog::info("Text shader cache: loaded {} shaders", shaders_.size());
    }
    else
    {
        spdlog::warn("Text shader cache: no shaders loaded");
    }

    return any_loaded;
}

sf::Shader* text_shader_cache::get(text_effect effect)
{
    auto it = shaders_.find(static_cast<uint8_t>(effect));
    if (it != shaders_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

bool text_shader_cache::load_shader(text_effect effect, const std::string& filename)
{
    auto shader = std::make_unique<sf::Shader>();
    if (!shader->loadFromFile(filename, sf::Shader::Type::Fragment))
    {
        spdlog::warn("Failed to load shader: {}", filename);
        return false;
    }

    spdlog::debug("Loaded shader: {}", filename);
    shaders_[static_cast<uint8_t>(effect)] = std::move(shader);
    return true;
}

} // namespace hb
