#include "ui/dialogs/craft_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <algorithm>

namespace hb
{

craft_dialog::craft_dialog() : dialog(dialog_type::manufacture)
{
    set_title("Manufacturing");
    set_bounds({140, 80, 360, 400});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void craft_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);

    // Update result display timer
    if (has_result_)
    {
        result_display_time_ -= delta_time;
        if (result_display_time_ <= 0.0f)
        {
            has_result_ = false;
        }
    }
}

void craft_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Skill level display
    rend.draw_text(std::format("Manufacturing Skill: {}", skill_level_), x, y, sf::Color(200, 200, 100));
    y += 22;

    // Separator
    rend.draw_line(x, y, x + 340, y, sf::Color(80, 80, 100));
    y += 8;

    // Recipe list section
    rend.draw_text("Recipes:", x, y, sf::Color::White);
    y += 18;

    recipe_area_x_ = x;
    recipe_area_y_ = y;
    render_recipe_list(rend, x, y);
    y += visible_recipes * 24 + 10;

    // Separator
    rend.draw_line(x, y, x + 340, y, sf::Color(80, 80, 100));
    y += 8;

    // Ingredient section
    if (selected_recipe_ >= 0 && selected_recipe_ < static_cast<int32_t>(recipes_.size()))
    {
        const auto& recipe = recipes_[selected_recipe_];

        rend.draw_text("Required Materials:", x, y, sf::Color::White);
        y += 18;

        // Show required ingredients
        for (size_t i = 0; i < recipe.ingredients.size(); ++i)
        {
            const auto& ing = recipe.ingredients[i];
            sf::Color color = sf::Color(180, 180, 180);
            rend.draw_text(std::format("- {} x{}", ing.name, ing.quantity), x + 10, y, color);
            y += 16;
        }
        y += 8;

        rend.draw_text("Your Materials:", x, y, sf::Color::White);
        y += 18;

        ingredient_area_x_ = x;
        ingredient_area_y_ = y;
        render_ingredient_slots(rend, x, y);
        y += 2 * (slot_size + slot_padding) + 10;

        // Result preview
        render_result_preview(rend, x, y);
    }
    else
    {
        rend.draw_text("Select a recipe to begin crafting.", x, y, sf::Color(150, 150, 150));
        y += 20;
        ingredient_area_x_ = x;
        ingredient_area_y_ = y;
    }

    // Craft button
    int32_t btn_x = bounds_.x + bounds_.width / 2 - 50;
    int32_t btn_y = bounds_.y + bounds_.height - 45;

    sf::Color btn_color;
    if (is_crafting_)
    {
        btn_color = sf::Color(80, 80, 60);
    }
    else if (selected_recipe_ >= 0)
    {
        btn_color = sf::Color(60, 100, 60);
    }
    else
    {
        btn_color = sf::Color(60, 60, 60);
    }

    rend.draw_rect(btn_x, btn_y, 100, 28, btn_color, true);
    rend.draw_rect(btn_x, btn_y, 100, 28, sf::Color(100, 100, 120), false);
    rend.draw_text(is_crafting_ ? "Crafting..." : "Craft", btn_x + 28, btn_y + 6, sf::Color::White);

    // Crafting progress bar
    if (is_crafting_)
    {
        int32_t bar_x = btn_x;
        int32_t bar_y = btn_y + 32;
        int32_t bar_width = 100;
        int32_t bar_height = 8;

        rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(40, 40, 50), true);
        int32_t fill_width = static_cast<int32_t>(bar_width * craft_progress_);
        rend.draw_rect(bar_x, bar_y, fill_width, bar_height, sf::Color(100, 180, 100), true);
        rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(80, 80, 100), false);
    }

    // Result message
    if (has_result_)
    {
        sf::Color msg_color = result_success_ ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100);
        int32_t msg_x = bounds_.x + bounds_.width / 2 - static_cast<int32_t>(result_message_.length() * 3);
        int32_t msg_y = bounds_.y + bounds_.height - 20;
        rend.draw_text(result_message_, msg_x, msg_y, msg_color, 12);
    }
}

void craft_dialog::render_recipe_list(renderer& rend, int32_t x, int32_t y)
{
    int32_t recipe_width = 320;
    int32_t recipe_height = 22;

    // Scroll buttons if needed
    if (recipes_.size() > static_cast<size_t>(visible_recipes))
    {
        // Up arrow
        if (scroll_offset_ > 0)
        {
            rend.draw_text("^", x + recipe_width + 5, y, sf::Color(150, 150, 150));
        }
        // Down arrow
        if (scroll_offset_ + visible_recipes < static_cast<int32_t>(recipes_.size()))
        {
            rend.draw_text(
                "v", x + recipe_width + 5, y + (visible_recipes - 1) * recipe_height, sf::Color(150, 150, 150));
        }
    }

    for (int32_t i = 0; i < visible_recipes; ++i)
    {
        int32_t recipe_idx = scroll_offset_ + i;
        if (recipe_idx >= static_cast<int32_t>(recipes_.size()))
            break;

        const auto& recipe = recipes_[recipe_idx];
        int32_t row_y = y + i * recipe_height;

        bool hovered = hovered_recipe_.has_value() && hovered_recipe_.value() == recipe_idx;
        bool selected = selected_recipe_ == recipe_idx;

        sf::Color bg_color;
        if (selected)
        {
            bg_color = sf::Color(60, 80, 100);
        }
        else if (hovered)
        {
            bg_color = sf::Color(50, 50, 65);
        }
        else
        {
            bg_color = sf::Color(40, 40, 50);
        }

        rend.draw_rect(x, row_y, recipe_width, recipe_height - 2, bg_color, true);

        // Recipe name and success rate
        sf::Color text_color = skill_level_ >= recipe.skill_required ? sf::Color::White : sf::Color(150, 100, 100);
        rend.draw_text(recipe.result_name, x + 4, row_y + 3, text_color, 12);

        std::string rate_text = std::format("{}%", recipe.success_rate);
        rend.draw_text(rate_text, x + recipe_width - 40, row_y + 3, sf::Color(150, 200, 150), 12);
    }
}

void craft_dialog::render_ingredient_slots(renderer& rend, int32_t x, int32_t y)
{
    // 3 columns x 2 rows = 6 ingredient slots
    for (int32_t i = 0; i < max_ingredients; ++i)
    {
        int32_t row = i / 3;
        int32_t col = i % 3;
        int32_t slot_x = x + col * (slot_size + slot_padding + 40);
        int32_t slot_y = y + row * (slot_size + slot_padding);

        bool hovered = hovered_slot_.has_value() && hovered_slot_.value() == i;
        sf::Color bg = hovered ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);

        rend.draw_rect(slot_x, slot_y, slot_size, slot_size, bg, true);
        rend.draw_rect(slot_x, slot_y, slot_size, slot_size, sf::Color(70, 70, 90), false);

        if (ingredients_[i])
        {
            // Draw item indicator
            sf::Color item_color(150, 150, 150);
            rend.draw_rect(slot_x + 4, slot_y + 4, slot_size - 8, slot_size - 8, item_color, true);

            // Item amount
            if (ingredients_[i]->count > 1)
            {
                rend.draw_text(
                    std::to_string(ingredients_[i]->count), slot_x + 2, slot_y + slot_size - 14, sf::Color::White, 10);
            }
        }
    }
}

void craft_dialog::render_result_preview(renderer& rend, int32_t x, int32_t y)
{
    if (selected_recipe_ < 0 || selected_recipe_ >= static_cast<int32_t>(recipes_.size()))
        return;

    const auto& recipe = recipes_[selected_recipe_];

    rend.draw_text("Result:", x, y, sf::Color::White);
    y += 18;

    // Result item box
    rend.draw_rect(x, y, slot_size, slot_size, sf::Color(50, 60, 70), true);
    rend.draw_rect(x, y, slot_size, slot_size, sf::Color(80, 90, 100), false);

    // Placeholder for result item
    rend.draw_rect(x + 4, y + 4, slot_size - 8, slot_size - 8, sf::Color(180, 150, 100), true);

    // Result name
    rend.draw_text(recipe.result_name, x + slot_size + 8, y + 10, sf::Color(255, 220, 150));
}

std::optional<int32_t> craft_dialog::slot_at(int32_t mx, int32_t my) const
{
    for (int32_t i = 0; i < max_ingredients; ++i)
    {
        int32_t row = i / 3;
        int32_t col = i % 3;
        int32_t slot_x = ingredient_area_x_ + col * (slot_size + slot_padding + 40);
        int32_t slot_y = ingredient_area_y_ + row * (slot_size + slot_padding);

        ui_rect rect{slot_x, slot_y, slot_size, slot_size};
        if (rect.contains(mx, my))
        {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<int32_t> craft_dialog::recipe_at(int32_t mx, int32_t my) const
{
    int32_t recipe_width = 320;
    int32_t recipe_height = 22;

    for (int32_t i = 0; i < visible_recipes; ++i)
    {
        int32_t recipe_idx = scroll_offset_ + i;
        if (recipe_idx >= static_cast<int32_t>(recipes_.size()))
            break;

        int32_t row_y = recipe_area_y_ + i * recipe_height;
        ui_rect rect{recipe_area_x_, row_y, recipe_width, recipe_height - 2};

        if (rect.contains(mx, my))
        {
            return recipe_idx;
        }
    }
    return std::nullopt;
}

bool craft_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    // Check craft button
    int32_t btn_x = bounds_.x + bounds_.width / 2 - 50;
    int32_t btn_y = bounds_.y + bounds_.height - 45;
    ui_rect craft_btn{btn_x, btn_y, 100, 28};

    if (craft_btn.contains(x, y) && btn == sf::Mouse::Button::Left)
    {
        if (!is_crafting_ && selected_recipe_ >= 0 && on_craft_)
        {
            on_craft_(selected_recipe_);
        }
        return true;
    }

    // Check recipe selection
    auto recipe_idx = recipe_at(x, y);
    if (recipe_idx.has_value() && btn == sf::Mouse::Button::Left)
    {
        select_recipe(recipe_idx.value());
        return true;
    }

    // Check ingredient slots
    auto slot = slot_at(x, y);
    if (slot.has_value())
    {
        if (btn == sf::Mouse::Button::Left)
        {
            if (on_add_ingredient_)
            {
                on_add_ingredient_(slot.value());
            }
        }
        else if (btn == sf::Mouse::Button::Right)
        {
            if (on_remove_ingredient_ && ingredients_[slot.value()])
            {
                on_remove_ingredient_(slot.value());
            }
        }
        return true;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool craft_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_slot_ = slot_at(x, y);
    hovered_recipe_ = recipe_at(x, y);

    return dialog::handle_mouse_move(x, y);
}

void craft_dialog::select_recipe(int32_t index)
{
    if (index >= 0 && index < static_cast<int32_t>(recipes_.size()))
    {
        selected_recipe_ = index;
        clear_ingredients();
    }
}

void craft_dialog::set_ingredient(int32_t slot, const item* itm)
{
    if (slot >= 0 && slot < max_ingredients)
    {
        ingredients_[slot] = itm;
    }
}

void craft_dialog::clear_ingredient(int32_t slot)
{
    if (slot >= 0 && slot < max_ingredients)
    {
        ingredients_[slot] = nullptr;
    }
}

void craft_dialog::clear_ingredients()
{
    ingredients_.fill(nullptr);
}

const item* craft_dialog::get_ingredient(int32_t slot) const
{
    if (slot >= 0 && slot < max_ingredients)
    {
        return ingredients_[slot];
    }
    return nullptr;
}

void craft_dialog::set_last_result(bool success, std::string_view message)
{
    has_result_ = true;
    result_success_ = success;
    result_message_ = message;
    result_display_time_ = 3.0f; // Show for 3 seconds
    is_crafting_ = false;
    craft_progress_ = 0.0f;
}

} // namespace hb
