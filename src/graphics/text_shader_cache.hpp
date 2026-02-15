#pragma once

#include "graphics/text_style.hpp"
#include <SFML/Graphics/Shader.hpp>
#include <memory>
#include <unordered_map>
#include <string>

namespace hb
{

// Loads and caches GLSL fragment shaders for text effects.
// Each shader effect maps to a .frag file in assets/shaders/.
class text_shader_cache
{
public:
    text_shader_cache() = default;
    ~text_shader_cache() = default;

    // Load all shaders from assets/shaders/ directory
    // Returns true if at least one shader loaded successfully
    bool initialize();

    // Get shader for a given effect (nullptr if not loaded or not a shader effect)
    sf::Shader* get(text_effect effect);

    // Check if any shaders are available
    bool available() const { return !shaders_.empty(); }

private:
    bool load_shader(text_effect effect, const std::string& filename);

    std::unordered_map<uint8_t, std::unique_ptr<sf::Shader>> shaders_;
};

} // namespace hb
