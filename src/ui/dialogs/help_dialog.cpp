#include "ui/dialogs/help_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <algorithm>

namespace hb {

constexpr const char* help_dialog::category_names[num_categories];

help_dialog::help_dialog()
    : dialog(dialog_type::help) {
    set_title("Help");
    set_bounds({100, 50, 450, 400});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);

    load_default_help();
}

void help_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void help_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Category tabs
    tab_area_x_ = x;
    tab_area_y_ = y;
    render_category_tabs(rend, x, y);
    y += 28;

    // Separator
    rend.draw_line(x, y, x + 430, y, sf::Color(80, 80, 100));
    y += 6;

    // Topic list (left side)
    int32_t list_width = 140;
    topic_area_x_ = x;
    topic_area_y_ = y;
    topic_area_w_ = list_width;
    topic_area_h_ = visible_topics * topic_row_height;
    render_topic_list(rend, x, y, list_width, topic_area_h_);

    // Content area (right side)
    int32_t content_x = x + list_width + 10;
    int32_t content_width = bounds_.width - list_width - 30;
    int32_t content_height = visible_content_lines * content_line_height;
    render_content_area(rend, content_x, y, content_width, content_height);

    // Scroll hint for content
    if (selected_topic_ >= 0) {
        auto filtered = get_filtered_topics();
        if (selected_topic_ < static_cast<int32_t>(filtered.size())) {
            const auto* topic = filtered[selected_topic_];
            if (topic->content_lines.size() > static_cast<size_t>(visible_content_lines)) {
                std::string hint = std::format("Use Up/Down to scroll ({}/{})",
                    content_scroll_ + 1,
                    static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines + 1);
                rend.draw_text(hint, content_x, y + content_height + 8, sf::Color(150, 150, 150), 10);
            }
        }
    }
}

void help_dialog::render_category_tabs(renderer& rend, int32_t x, int32_t y) {
    int32_t tab_width = 60;
    int32_t tab_height = 22;

    for (int i = 0; i < num_categories; ++i) {
        int32_t tab_x = x + i * (tab_width + 4);
        int actual_category = i - 1;  // -1 for "All"

        bool selected = current_category_ == actual_category;
        bool hovered = hovered_category_.has_value() && hovered_category_.value() == i;

        sf::Color bg_color;
        if (selected) {
            bg_color = sf::Color(60, 80, 100);
        } else if (hovered) {
            bg_color = sf::Color(50, 50, 65);
        } else {
            bg_color = sf::Color(40, 40, 50);
        }

        rend.draw_rect(tab_x, y, tab_width, tab_height, bg_color, true);
        if (selected) {
            rend.draw_rect(tab_x, y, tab_width, tab_height, sf::Color(100, 120, 150), false);
        }

        sf::Color text_color = selected ? sf::Color::White : sf::Color(180, 180, 180);
        int32_t text_x = tab_x + (tab_width - static_cast<int32_t>(strlen(category_names[i]) * 5)) / 2;
        rend.draw_text(category_names[i], text_x, y + 5, text_color, 10);
    }
}

void help_dialog::render_topic_list(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height) {
    // Background
    rend.draw_rect(x, y, width, height, sf::Color(30, 30, 40), true);
    rend.draw_rect(x, y, width, height, sf::Color(60, 60, 80), false);

    auto filtered = get_filtered_topics();

    for (int32_t i = 0; i < visible_topics; ++i) {
        int32_t topic_idx = topic_scroll_ + i;
        if (topic_idx >= static_cast<int32_t>(filtered.size())) break;

        const auto* topic = filtered[topic_idx];
        int32_t row_y = y + i * topic_row_height;

        bool hovered = hovered_topic_.has_value() && hovered_topic_.value() == topic_idx;
        bool selected = selected_topic_ == topic_idx;

        sf::Color bg_color;
        if (selected) {
            bg_color = sf::Color(50, 70, 90);
        } else if (hovered) {
            bg_color = sf::Color(40, 45, 55);
        } else {
            bg_color = sf::Color(35, 35, 45);
        }

        rend.draw_rect(x + 2, row_y + 1, width - 4, topic_row_height - 2, bg_color, true);

        sf::Color text_color = selected ? sf::Color::White : sf::Color(200, 200, 200);
        std::string title = topic->title;
        if (title.length() > 18) {
            title = title.substr(0, 15) + "...";
        }
        rend.draw_text(title, x + 6, row_y + 4, text_color, 11);
    }

    // Scroll indicator
    if (filtered.size() > static_cast<size_t>(visible_topics)) {
        int32_t scroll_x = x + width - 8;
        int32_t scroll_y = y + 2;
        int32_t scroll_height = height - 4;

        float visible_ratio = static_cast<float>(visible_topics) / filtered.size();
        int32_t thumb_height = std::max(15, static_cast<int32_t>(scroll_height * visible_ratio));

        float scroll_ratio = static_cast<float>(topic_scroll_) / (filtered.size() - visible_topics);
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) * scroll_ratio);

        rend.draw_rect(scroll_x, scroll_y, 4, scroll_height, sf::Color(25, 25, 35), true);
        rend.draw_rect(scroll_x, thumb_y, 4, thumb_height, sf::Color(70, 70, 90), true);
    }
}

void help_dialog::render_content_area(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height) {
    // Background
    rend.draw_rect(x, y, width, height, sf::Color(25, 28, 35), true);
    rend.draw_rect(x, y, width, height, sf::Color(60, 60, 80), false);

    auto filtered = get_filtered_topics();
    if (selected_topic_ < 0 || selected_topic_ >= static_cast<int32_t>(filtered.size())) {
        rend.draw_text("Select a topic from the list.", x + 10, y + 20, sf::Color(150, 150, 150));
        return;
    }

    const auto* topic = filtered[selected_topic_];

    // Title
    rend.draw_text(topic->title, x + 8, y + 6, sf::Color(200, 200, 255));
    rend.draw_line(x + 4, y + 22, x + width - 4, y + 22, sf::Color(50, 50, 70));

    // Content
    int32_t line_y = y + 28;
    int32_t end_line = std::min(content_scroll_ + visible_content_lines,
                                static_cast<int32_t>(topic->content_lines.size()));

    for (int32_t i = content_scroll_; i < end_line; ++i) {
        const auto& line = topic->content_lines[i];
        sf::Color line_color = sf::Color(220, 220, 220);

        // Check for formatting hints
        if (line.length() > 0 && line[0] == '*') {
            // Bullet point
            rend.draw_text(line, x + 8, line_y, sf::Color(180, 200, 180), 11);
        } else if (line.length() > 0 && line[0] == '#') {
            // Header style
            rend.draw_text(line.substr(1), x + 8, line_y, sf::Color(200, 180, 140), 11);
        } else {
            rend.draw_text(line, x + 8, line_y, line_color, 11);
        }
        line_y += content_line_height;
    }
}

std::vector<const help_topic*> help_dialog::get_filtered_topics() const {
    std::vector<const help_topic*> result;
    for (const auto& topic : topics_) {
        if (current_category_ == -1 || topic.category == static_cast<uint8_t>(current_category_)) {
            result.push_back(&topic);
        }
    }
    return result;
}

std::optional<int32_t> help_dialog::topic_at(int32_t mx, int32_t my) const {
    if (mx < topic_area_x_ || mx >= topic_area_x_ + topic_area_w_ ||
        my < topic_area_y_ || my >= topic_area_y_ + topic_area_h_) {
        return std::nullopt;
    }

    auto filtered = get_filtered_topics();
    int32_t row = (my - topic_area_y_) / topic_row_height;
    int32_t idx = topic_scroll_ + row;

    if (idx >= 0 && idx < static_cast<int32_t>(filtered.size())) {
        return idx;
    }
    return std::nullopt;
}

std::optional<int32_t> help_dialog::category_at(int32_t mx, int32_t my) const {
    int32_t tab_width = 60;
    int32_t tab_height = 22;

    if (my < tab_area_y_ || my >= tab_area_y_ + tab_height) {
        return std::nullopt;
    }

    for (int i = 0; i < num_categories; ++i) {
        int32_t tab_x = tab_area_x_ + i * (tab_width + 4);
        if (mx >= tab_x && mx < tab_x + tab_width) {
            return i;
        }
    }
    return std::nullopt;
}

bool help_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Category tab click
    auto cat = category_at(x, y);
    if (cat.has_value() && btn == sf::Mouse::Button::Left) {
        int actual_category = cat.value() - 1;  // -1 for "All"
        set_category(actual_category);
        selected_topic_ = -1;
        content_scroll_ = 0;
        return true;
    }

    // Topic click
    auto topic = topic_at(x, y);
    if (topic.has_value() && btn == sf::Mouse::Button::Left) {
        select_topic(topic.value());
        return true;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool help_dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_) return false;

    hovered_topic_ = topic_at(x, y);
    hovered_category_ = category_at(x, y);

    return dialog::handle_mouse_move(x, y);
}

bool help_dialog::handle_key_press(sf::Keyboard::Key key) {
    if (!visible_) return false;

    if (key == sf::Keyboard::Key::Down) {
        auto filtered = get_filtered_topics();
        if (selected_topic_ >= 0 && selected_topic_ < static_cast<int32_t>(filtered.size())) {
            const auto* topic = filtered[selected_topic_];
            int32_t max_scroll = std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines);
            content_scroll_ = std::min(content_scroll_ + 1, max_scroll);
        }
        return true;
    }

    if (key == sf::Keyboard::Key::Up) {
        content_scroll_ = std::max(0, content_scroll_ - 1);
        return true;
    }

    if (key == sf::Keyboard::Key::PageDown) {
        auto filtered = get_filtered_topics();
        if (selected_topic_ >= 0 && selected_topic_ < static_cast<int32_t>(filtered.size())) {
            const auto* topic = filtered[selected_topic_];
            int32_t max_scroll = std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines);
            content_scroll_ = std::min(content_scroll_ + visible_content_lines, max_scroll);
        }
        return true;
    }

    if (key == sf::Keyboard::Key::PageUp) {
        content_scroll_ = std::max(0, content_scroll_ - visible_content_lines);
        return true;
    }

    return false;
}

void help_dialog::select_topic(int32_t index) {
    auto filtered = get_filtered_topics();
    if (index >= 0 && index < static_cast<int32_t>(filtered.size())) {
        selected_topic_ = index;
        content_scroll_ = 0;
    }
}

void help_dialog::load_default_help() {
    topics_.clear();

    // Basics (category 0)
    topics_.push_back({
        "Getting Started",
        {
            "#Welcome to Helbreath!",
            "",
            "Helbreath is a classic MMORPG where you",
            "choose to fight for either Aresden or",
            "Elvine in a never-ending conflict.",
            "",
            "* Create a character and select a town",
            "* Learn the basics of combat",
            "* Join a guild and make allies",
            "* Participate in crusades for glory!"
        },
        0
    });

    topics_.push_back({
        "Movement",
        {
            "#Moving Your Character",
            "",
            "Left-click on the ground to walk to",
            "that location. Your character will",
            "automatically navigate around obstacles.",
            "",
            "* Click and hold to keep moving",
            "* Press shift to run",
            "* Press control+r to set run mode",
            "* Use arrow keys for manual control"
        },
        0
    });

    // Combat (category 1)
    topics_.push_back({
        "Basic Combat",
        {
            "#Combat Basics",
            "",
            "To attack an enemy, click on them while",
            "in combat mode (press F1 to toggle).",
            "",
            "* F1: Toggle combat mode",
            "* Left-click: Attack target",
            "* Ctrl+click: Force attack (PvP)",
            "",
            "Your damage is based on your weapon skill",
            "strength, and equipment."
        },
        1
    });

    topics_.push_back({
        "Super Attacks",
        {
            "#Super Attacks",
            "",
            "When your weapon skill reaches 100%,",
            "you unlock special super attacks:",
            "",
            "* Critical Strike: High damage",
            "* Stun Attack: Immobilizes enemy",
            "* Bleeding: Damage over time",
            "",
            "Super attacks consume stamina points.",
            "Use them strategically in combat!"
        },
        1
    });

    // Magic (category 2)
    topics_.push_back({
        "Casting Spells",
        {
            "#Using Magic",
            "",
            "Open the spellbook with 'M' to see",
            "your available spells.",
            "",
            "* Click a spell to prepare it",
            "* Click target to cast",
            "* Spells consume mana (MP)",
            "",
            "Higher intelligence increases magic",
            "damage and reduces mana costs."
        },
        2
    });

    topics_.push_back({
        "Spell Circles",
        {
            "#Magic Circles",
            "",
            "Spells are organized into circles:",
            "",
            "* Circle 1: Basic spells (Energy Bolt)",
            "* Circle 2: Fire/Ice magic",
            "* Circle 3: Area spells",
            "* Circle 4: Advanced combat magic",
            "* Circle 5: High-tier spells",
            "* Circle 6: Ultimate magic",
            "",
            "Higher circles require more magic skill."
        },
        2
    });

    // Items (category 3)
    topics_.push_back({
        "Equipment",
        {
            "#Using Equipment",
            "",
            "Open inventory with 'I' to see items.",
            "Double-click to equip or use items.",
            "",
            "Equipment slots:",
            "* Head, Body, Arms, Legs, Boots",
            "* Main hand, Off hand (or Two-hand)",
            "* Accessories (rings, necklace)",
            "",
            "Better equipment improves your stats."
        },
        3
    });

    topics_.push_back({
        "Item Grades",
        {
            "#Item Quality",
            "",
            "Items have different quality grades:",
            "",
            "* Normal (white): Basic stats",
            "* Magic (blue): +1 to +3 bonus",
            "* Rare (gold): Special effects",
            "* Unique (red): Powerful abilities",
            "",
            "Higher grade items are much rarer."
        },
        3
    });

    // Social (category 4)
    topics_.push_back({
        "Guilds",
        {
            "#Joining a Guild",
            "",
            "Guilds are player-run organizations.",
            "Benefits of joining a guild:",
            "",
            "* Guild chat channel",
            "* Coordinate in crusades",
            "* Share resources",
            "* Access guild warehouse",
            "",
            "Ask a guild master for an invitation."
        },
        4
    });

    // Shortcuts (category 5)
    topics_.push_back({
        "Hotkeys",
        {
            "#Keyboard Shortcuts",
            "",
            "* I - Inventory",
            "* C - Character stats",
            "* E - Equipment",
            "* K - Skills",
            "* M - Spellbook",
            "* P - Party",
            "* G - Guild",
            "* Enter - Chat",
            "* Escape - System menu",
            "* F1 - Toggle combat mode",
            "* F5-F8 - Quick spells"
        },
        5
    });

    topics_.push_back({
        "Chat Commands",
        {
            "#Chat Prefixes",
            "",
            "Type these before your message:",
            "",
            "* ! - Shout (area-wide)",
            "* @ - Whisper to player",
            "* # - Party chat",
            "* $ - Guild chat",
            "* % - Trade channel",
            "",
            "Example: @PlayerName Hello!"
        },
        5
    });
}

} // namespace hb
