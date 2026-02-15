#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hb
{

// Forward declarations
struct rect;

// Simple rect for dialog definitions (avoiding circular dependencies)
struct ui_bounds
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    bool contains(int32_t px, int32_t py) const { return px >= x && px < x + width && py >= y && py < y + height; }
};

// UI element types supported by the dialog system
enum class element_type
{
    label,        // Static text
    button,       // Clickable button
    text_input,   // Text entry field
    image,        // Static image/sprite
    progress_bar, // Progress/gauge bar
    checkbox,     // Toggle checkbox
    slider,       // Value slider
    list_box,     // Scrollable list
    grid,         // Grid of cells (for inventory, etc.)
    sprite,       // PAK sprite with frame
    panel,        // Container panel
    separator,    // Visual separator line

    // === Custom HUD elements (rendered by code, positioned by YAML) ===
    custom,           // Generic custom element (subclass handles rendering)
    hp_bar,           // HP bar with poison support
    mp_bar,           // MP bar
    sp_bar,           // Stamina bar
    exp_bar,          // Experience bar
    location_text,    // Map name and coordinates display
    combat_indicator, // Safe/PK mode indicator
    super_attack      // Super attack counter
};

// Action types for YAML-based button actions
enum class element_action
{
    none,         // No action (use code callback only)
    open_dialog,  // Open another dialog
    close_dialog, // Close a specific dialog
    close_self,   // Close the containing dialog
    toggle_dialog // Toggle another dialog open/closed
};

// Element state flags
struct element_state
{
    bool visible = true;
    bool enabled = true;
    bool focused = false;
    bool hovered = false;
    bool pressed = false;

    // Dynamic values
    std::string text;            // Current text (for labels, buttons, inputs)
    float progress = 0.0f;       // Progress value (0.0-1.0) for progress bars
    bool checked = false;        // Checkbox state
    float slider_value = 0.0f;   // Slider value (0.0-1.0)
    int32_t selected_index = -1; // List/grid selection

    // Grid-specific state
    std::vector<int32_t> grid_data; // Data for each cell
};

// Definition for a single UI element
struct element_def
{
    std::string id; // Unique identifier for callbacks/access
    element_type type = element_type::label;
    ui_bounds bounds; // Position and size relative to dialog

    // Text content
    std::string text;
    std::optional<sf::Color> text_color;
    std::optional<uint32_t> font_size;

    // Sprite rendering (for classic mode)
    std::string sprite_pak;   // PAK file name (e.g., "GameDialog")
    int32_t sprite_index = 0; // Sprite index in PAK
    int32_t sprite_frame = 0; // Frame within sprite

    // Visual styling
    std::optional<sf::Color> background_color;
    std::optional<sf::Color> border_color;
    std::optional<sf::Color> hover_color;
    std::optional<sf::Color> pressed_color;
    std::optional<sf::Color> disabled_color;

    // Input constraints
    int32_t max_chars = 256;    // For text_input
    bool password_mode = false; // Mask input characters

    // Grid configuration
    int32_t grid_cols = 1;
    int32_t grid_rows = 1;
    int32_t cell_width = 32;
    int32_t cell_height = 32;
    int32_t cell_padding = 2;

    // Progress bar configuration
    std::optional<sf::Color> fill_color;
    bool show_value_text = false;

    // Slider configuration
    float min_value = 0.0f;
    float max_value = 1.0f;
    float step = 0.01f;

    // Extended properties (for custom data)
    std::map<std::string, std::string> properties;

    // Tooltip
    std::string tooltip;

    // Action system (for buttons)
    element_action action = element_action::none;
    std::string action_target; // Dialog ID for open/close/toggle actions
};

// Definition for a complete dialog
struct dialog_definition
{
    std::string id;    // Unique identifier
    std::string title; // Window title
    ui_bounds bounds;  // Position and size

    // Dialog behavior
    bool modal = false;                // Blocks input to other dialogs
    bool draggable = true;             // Can be dragged by title bar
    bool closeable = true;             // Has close button
    bool has_border = true;            // Draw border
    bool has_title_bar = true;         // Show title bar
    bool centered = false;             // Center on screen on open
    bool always_on_top = false;        // Always render on top of other dialogs
    bool right_click_closeable = true; // Right-click closes this dialog

    // Visual styling
    std::optional<sf::Color> background_color;
    std::optional<sf::Color> border_color;
    std::optional<sf::Color> title_bar_color;
    std::optional<sf::Color> title_text_color;

    // Sprite-based background (for classic mode)
    std::string background_sprite_pak;
    int32_t background_sprite_index = 0;
    int32_t background_sprite_frame = 0;

    // Child elements
    std::vector<element_def> elements;

    // Find element by ID
    const element_def* find_element(std::string_view element_id) const
    {
        for (const auto& elem : elements)
        {
            if (elem.id == element_id)
            {
                return &elem;
            }
        }
        return nullptr;
    }

    element_def* find_element(std::string_view element_id)
    {
        for (auto& elem : elements)
        {
            if (elem.id == element_id)
            {
                return &elem;
            }
        }
        return nullptr;
    }
};

} // namespace hb
