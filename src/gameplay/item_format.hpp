#pragma once

#include "gameplay/item.hpp"
#include <SFML/Graphics/Color.hpp>
#include <string>
#include <vector>

namespace hb
{

struct item_info_line
{
    std::string text;
    sf::Color color = sf::Color::White;
};

// Build display lines for an item (name, effects, durability, total count)
std::vector<item_info_line> build_item_info(const item& itm, int32_t total_count = 0);

// Get name color based on item properties
sf::Color item_name_color(const item& itm);

} // namespace hb
