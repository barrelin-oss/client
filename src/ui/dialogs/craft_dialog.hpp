#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <vector>
#include <string>
#include <array>
#include <optional>

namespace hb
{

// Crafting recipe structure
struct craft_recipe
{
    uint16_t result_item_id = 0;
    std::string result_name;
    uint8_t skill_required = 0; // Manufacturing skill level needed
    uint8_t success_rate = 0;   // Base success rate percentage

    struct ingredient
    {
        uint16_t item_id = 0;
        std::string name;
        int32_t quantity = 1;
    };
    std::vector<ingredient> ingredients;
};

// Craft dialog - for item manufacturing
class craft_dialog : public dialog
{
public:
    static constexpr int32_t max_ingredients = 6;
    static constexpr int32_t slot_size = 36;
    static constexpr int32_t slot_padding = 2;

    craft_dialog();
    ~craft_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Recipe management
    void set_recipes(std::vector<craft_recipe> recipes) { recipes_ = std::move(recipes); }
    void add_recipe(craft_recipe recipe) { recipes_.push_back(std::move(recipe)); }
    void clear_recipes()
    {
        recipes_.clear();
        selected_recipe_ = -1;
    }

    // Select a recipe
    void select_recipe(int32_t index);
    int32_t selected_recipe() const { return selected_recipe_; }

    // Ingredient slots (items player has placed)
    void set_ingredient(int32_t slot, const item* itm);
    void clear_ingredient(int32_t slot);
    void clear_ingredients();
    const item* get_ingredient(int32_t slot) const;

    // Manufacturing skill level
    void set_skill_level(uint8_t level) { skill_level_ = level; }
    uint8_t skill_level() const { return skill_level_; }

    // Crafting state
    void set_crafting(bool crafting) { is_crafting_ = crafting; }
    bool is_crafting() const { return is_crafting_; }
    void set_craft_progress(float progress) { craft_progress_ = progress; }

    // Result
    void set_last_result(bool success, std::string_view message);
    void clear_result() { has_result_ = false; }

    // Callbacks
    using craft_callback = std::function<void(int32_t recipe_index)>;
    using ingredient_callback = std::function<void(int32_t slot)>;

    void set_on_craft(craft_callback callback) { on_craft_ = std::move(callback); }
    void set_on_add_ingredient(ingredient_callback callback) { on_add_ingredient_ = std::move(callback); }
    void set_on_remove_ingredient(ingredient_callback callback) { on_remove_ingredient_ = std::move(callback); }

private:
    void render_recipe_list(renderer& rend, int32_t x, int32_t y);
    void render_ingredient_slots(renderer& rend, int32_t x, int32_t y);
    void render_result_preview(renderer& rend, int32_t x, int32_t y);
    std::optional<int32_t> slot_at(int32_t mx, int32_t my) const;
    std::optional<int32_t> recipe_at(int32_t mx, int32_t my) const;

    std::vector<craft_recipe> recipes_;
    int32_t selected_recipe_ = -1;
    int32_t scroll_offset_ = 0;
    static constexpr int32_t visible_recipes = 6;

    std::array<const item*, max_ingredients> ingredients_{};
    uint8_t skill_level_ = 0;

    bool is_crafting_ = false;
    float craft_progress_ = 0.0f;

    bool has_result_ = false;
    bool result_success_ = false;
    std::string result_message_;
    float result_display_time_ = 0.0f;

    std::optional<int32_t> hovered_slot_;
    std::optional<int32_t> hovered_recipe_;

    int32_t ingredient_area_x_ = 0;
    int32_t ingredient_area_y_ = 0;
    int32_t recipe_area_x_ = 0;
    int32_t recipe_area_y_ = 0;

    craft_callback on_craft_;
    ingredient_callback on_add_ingredient_;
    ingredient_callback on_remove_ingredient_;
};

} // namespace hb
