#include "ui/dialogs/character_create_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

character_create_dialog::character_create_dialog() : dialog(dialog_type::character_create)
{
    set_title("Create Character");
    set_bounds({static_cast<int32_t>(screen_width) / 2 - 250, static_cast<int32_t>(screen_height) / 2 - 200, 500, 400});
    set_closeable(true);
    set_modal(true);

    reset();
    create_ui();
}

void character_create_dialog::reset()
{
    char_data_ = character_data{};
    points_remaining_ = 10;
    clear_status();
}

void character_create_dialog::create_ui()
{
    int32_t left_col = 20;
    int32_t right_col = 260;
    int32_t y = 40;

    // Character name
    auto name_label = std::make_unique<ui_label>();
    name_label->set_bounds({left_col, y, 80, 20});
    name_label->set_text("Name:");
    add_child(std::move(name_label));

    auto name_input = std::make_unique<ui_text_input>();
    name_input->set_id("name_input");
    name_input->set_bounds({left_col + 60, y - 2, 160, 24});
    name_input->set_max_length(20);
    name_input->set_placeholder("Character name");
    name_input_ = name_input.get();
    add_child(std::move(name_input));

    y += 35;

    // Gender selection
    auto gender_label = std::make_unique<ui_label>();
    gender_label->set_bounds({left_col, y, 80, 20});
    gender_label->set_text("Gender:");
    add_child(std::move(gender_label));

    auto male_btn = std::make_unique<ui_button>();
    male_btn->set_id("male_btn");
    male_btn->set_bounds({left_col + 60, y - 2, 60, 24});
    male_btn->set_text("Male");
    male_btn->set_on_click([this]() { char_data_.gender = 0; });
    add_child(std::move(male_btn));

    auto female_btn = std::make_unique<ui_button>();
    female_btn->set_id("female_btn");
    female_btn->set_bounds({left_col + 130, y - 2, 60, 24});
    female_btn->set_text("Female");
    female_btn->set_on_click([this]() { char_data_.gender = 1; });
    add_child(std::move(female_btn));

    y += 35;

    // Class selection
    auto class_label = std::make_unique<ui_label>();
    class_label->set_bounds({left_col, y, 80, 20});
    class_label->set_text("Class:");
    add_child(std::move(class_label));

    auto warrior_btn = std::make_unique<ui_button>();
    warrior_btn->set_id("warrior_btn");
    warrior_btn->set_bounds({left_col + 60, y - 2, 70, 24});
    warrior_btn->set_text("Warrior");
    warrior_btn->set_on_click([this]() { char_data_.warrior = true; });
    add_child(std::move(warrior_btn));

    auto mage_btn = std::make_unique<ui_button>();
    mage_btn->set_id("mage_btn");
    mage_btn->set_bounds({left_col + 140, y - 2, 70, 24});
    mage_btn->set_text("Mage");
    mage_btn->set_on_click([this]() { char_data_.warrior = false; });
    add_child(std::move(mage_btn));

    y += 40;

    // Stats section title
    auto stats_title = std::make_unique<ui_label>();
    stats_title->set_bounds({left_col, y, 200, 20});
    stats_title->set_text("Stat Points");
    stats_title->set_text_color(sf::Color(200, 200, 255));
    add_child(std::move(stats_title));

    auto points_label = std::make_unique<ui_label>();
    points_label->set_id("points_label");
    points_label->set_bounds({left_col + 120, y, 100, 20});
    points_label->set_text("Remaining: 10");
    points_label->set_text_color(sf::Color(100, 255, 100));
    points_label_ = points_label.get();
    add_child(std::move(points_label));

    y += 25;

    // Create stat rows
    auto create_stat_row = [&](std::string_view name, int stat_idx, ui_label*& value_label)
    {
        auto label = std::make_unique<ui_label>();
        label->set_bounds({left_col, y, 50, 20});
        label->set_text(name);
        add_child(std::move(label));

        auto minus_btn = std::make_unique<ui_button>();
        minus_btn->set_bounds({left_col + 55, y - 2, 24, 24});
        minus_btn->set_text("-");
        minus_btn->set_on_click([this, stat_idx]() { remove_stat(stat_idx); });
        add_child(std::move(minus_btn));

        auto value = std::make_unique<ui_label>();
        value->set_bounds({left_col + 85, y, 30, 20});
        value->set_text("10");
        value->set_alignment(ui_label::alignment::center);
        value_label = value.get();
        add_child(std::move(value));

        auto plus_btn = std::make_unique<ui_button>();
        plus_btn->set_bounds({left_col + 120, y - 2, 24, 24});
        plus_btn->set_text("+");
        plus_btn->set_on_click([this, stat_idx]() { add_stat(stat_idx); });
        add_child(std::move(plus_btn));

        y += 28;
    };

    create_stat_row("STR", 0, str_value_label_);
    create_stat_row("VIT", 1, vit_value_label_);
    create_stat_row("DEX", 2, dex_value_label_);
    create_stat_row("INT", 3, int_value_label_);
    create_stat_row("MAG", 4, mag_value_label_);
    create_stat_row("CHA", 5, cha_value_label_);

    // Character preview area (right column)
    auto preview_label = std::make_unique<ui_label>();
    preview_label->set_bounds({right_col, 40, 200, 20});
    preview_label->set_text("Preview");
    preview_label->set_text_color(sf::Color(200, 200, 255));
    add_child(std::move(preview_label));

    // Appearance customization
    int32_t appear_y = 200;

    auto skin_label = std::make_unique<ui_label>();
    skin_label->set_bounds({right_col, appear_y, 80, 20});
    skin_label->set_text("Skin:");
    add_child(std::move(skin_label));

    for (int i = 0; i < 3; ++i)
    {
        auto skin_btn = std::make_unique<ui_button>();
        skin_btn->set_bounds({right_col + 50 + i * 35, appear_y - 2, 30, 24});
        skin_btn->set_text(std::to_string(i + 1));
        skin_btn->set_on_click([this, i]() { char_data_.skin_color = static_cast<uint8_t>(i); });
        add_child(std::move(skin_btn));
    }

    appear_y += 30;

    auto hair_label = std::make_unique<ui_label>();
    hair_label->set_bounds({right_col, appear_y, 80, 20});
    hair_label->set_text("Hair Style:");
    add_child(std::move(hair_label));

    auto hair_prev = std::make_unique<ui_button>();
    hair_prev->set_bounds({right_col + 80, appear_y - 2, 30, 24});
    hair_prev->set_text("<");
    hair_prev->set_on_click([this]() { char_data_.hair_style = (char_data_.hair_style + 7) % 8; });
    add_child(std::move(hair_prev));

    auto hair_next = std::make_unique<ui_button>();
    hair_next->set_bounds({right_col + 120, appear_y - 2, 30, 24});
    hair_next->set_text(">");
    hair_next->set_on_click([this]() { char_data_.hair_style = (char_data_.hair_style + 1) % 8; });
    add_child(std::move(hair_next));

    appear_y += 30;

    auto hcolor_label = std::make_unique<ui_label>();
    hcolor_label->set_bounds({right_col, appear_y, 80, 20});
    hcolor_label->set_text("Hair Color:");
    add_child(std::move(hcolor_label));

    for (int i = 0; i < 4; ++i)
    {
        auto color_btn = std::make_unique<ui_button>();
        color_btn->set_bounds({right_col + 80 + i * 35, appear_y - 2, 30, 24});
        color_btn->set_text(std::to_string(i + 1));
        color_btn->set_on_click([this, i]() { char_data_.hair_color = static_cast<uint8_t>(i); });
        add_child(std::move(color_btn));
    }

    // Buttons at bottom
    int32_t btn_y = 350;

    auto create_btn = std::make_unique<ui_button>();
    create_btn->set_id("create_btn");
    create_btn->set_bounds({130, btn_y, 100, 32});
    create_btn->set_text("Create");
    create_btn->set_on_click(
        [this]()
        {
            if (validate_character() && on_create_)
            {
                on_create_(char_data_);
            }
        });
    add_child(std::move(create_btn));

    auto cancel_btn = std::make_unique<ui_button>();
    cancel_btn->set_id("cancel_btn");
    cancel_btn->set_bounds({270, btn_y, 100, 32});
    cancel_btn->set_text("Cancel");
    cancel_btn->set_on_click(
        [this]()
        {
            if (on_cancel_)
            {
                on_cancel_();
            }
            close();
        });
    add_child(std::move(cancel_btn));
}

void character_create_dialog::update(float delta_time, const input& inp)
{
    if (!visible_)
        return;
    dialog::update(delta_time, inp);

    // Update name from input
    if (name_input_)
    {
        char_data_.name = std::string(name_input_->text());
    }
}

void character_create_dialog::render(renderer& rend)
{
    if (!visible_)
        return;
    dialog::render(rend);

    // Render character preview
    render_character_preview(rend);

    // Render status message
    if (!status_message_.empty())
    {
        sf::Color status_color = status_is_error_ ? sf::Color(255, 100, 100) : sf::Color(100, 255, 100);
        rend.draw_text(status_message_, bounds_.x + 20, bounds_.y + 320, status_color);
    }
}

void character_create_dialog::render_character_preview(renderer& rend)
{
    int32_t preview_x = bounds_.x + 280;
    int32_t preview_y = bounds_.y + 60;
    int32_t preview_w = 180;
    int32_t preview_h = 130;

    // Preview background
    rend.draw_rect(preview_x, preview_y, preview_w, preview_h, sf::Color(30, 30, 40), true);
    rend.draw_rect(preview_x, preview_y, preview_w, preview_h, sf::Color(60, 60, 80), false);

    // Placeholder character silhouette
    std::string gender_text = char_data_.gender == 0 ? "Male" : "Female";
    std::string class_text = char_data_.warrior ? "Warrior" : "Mage";

    rend.draw_text(gender_text, preview_x + 60, preview_y + 40, sf::Color(150, 150, 170));
    rend.draw_text(class_text, preview_x + 55, preview_y + 60, sf::Color(150, 150, 170));

    // Show current appearance settings
    std::string skin_text = "Skin: " + std::to_string(char_data_.skin_color + 1);
    std::string hair_text =
        "Hair: " + std::to_string(char_data_.hair_style + 1) + "/" + std::to_string(char_data_.hair_color + 1);

    rend.draw_text(skin_text, preview_x + 10, preview_y + 90, sf::Color(120, 120, 140), 11);
    rend.draw_text(hair_text, preview_x + 10, preview_y + 105, sf::Color(120, 120, 140), 11);
}

void character_create_dialog::update_stat_display()
{
    if (str_value_label_)
        str_value_label_->set_text(std::to_string(char_data_.strength));
    if (vit_value_label_)
        vit_value_label_->set_text(std::to_string(char_data_.vitality));
    if (dex_value_label_)
        dex_value_label_->set_text(std::to_string(char_data_.dexterity));
    if (int_value_label_)
        int_value_label_->set_text(std::to_string(char_data_.intelligence));
    if (mag_value_label_)
        mag_value_label_->set_text(std::to_string(char_data_.magic));
    if (cha_value_label_)
        cha_value_label_->set_text(std::to_string(char_data_.charisma));
}

void character_create_dialog::update_points_remaining()
{
    points_remaining_ = 10 - static_cast<uint16_t>(get_stat_points_used());
    if (points_label_)
    {
        points_label_->set_text("Remaining: " + std::to_string(points_remaining_));
        points_label_->set_text_color(points_remaining_ > 0 ? sf::Color(100, 255, 100) : sf::Color(200, 200, 200));
    }
}

uint16_t character_create_dialog::get_stat(int index) const
{
    switch (index)
    {
    case 0:
        return char_data_.strength;
    case 1:
        return char_data_.vitality;
    case 2:
        return char_data_.dexterity;
    case 3:
        return char_data_.intelligence;
    case 4:
        return char_data_.magic;
    case 5:
        return char_data_.charisma;
    default:
        return 10;
    }
}

void character_create_dialog::set_stat(int index, uint16_t value)
{
    switch (index)
    {
    case 0:
        char_data_.strength = value;
        break;
    case 1:
        char_data_.vitality = value;
        break;
    case 2:
        char_data_.dexterity = value;
        break;
    case 3:
        char_data_.intelligence = value;
        break;
    case 4:
        char_data_.magic = value;
        break;
    case 5:
        char_data_.charisma = value;
        break;
    }
}

uint16_t character_create_dialog::get_stat_points_used() const
{
    return (char_data_.strength - 10) + (char_data_.vitality - 10) + (char_data_.dexterity - 10) +
           (char_data_.intelligence - 10) + (char_data_.magic - 10) + (char_data_.charisma - 10);
}

void character_create_dialog::add_stat(int stat_index)
{
    if (points_remaining_ == 0)
    {
        set_status("No stat points remaining.", true);
        return;
    }

    uint16_t current = get_stat(stat_index);
    if (current >= max_stat)
    {
        set_status("Stat is at maximum.", true);
        return;
    }

    set_stat(stat_index, current + 1);
    update_stat_display();
    update_points_remaining();
    clear_status();
}

void character_create_dialog::remove_stat(int stat_index)
{
    uint16_t current = get_stat(stat_index);
    if (current <= min_stat)
    {
        set_status("Stat is at minimum.", true);
        return;
    }

    set_stat(stat_index, current - 1);
    update_stat_display();
    update_points_remaining();
    clear_status();
}

bool character_create_dialog::validate_character()
{
    if (char_data_.name.empty())
    {
        set_status("Please enter a character name.", true);
        return false;
    }

    if (char_data_.name.length() < 4)
    {
        set_status("Name must be at least 4 characters.", true);
        return false;
    }

    if (char_data_.name.length() > 20)
    {
        set_status("Name must be 20 characters or less.", true);
        return false;
    }

    // Check for valid characters (alphanumeric only)
    for (char c : char_data_.name)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)))
        {
            set_status("Name can only contain letters and numbers.", true);
            return false;
        }
    }

    if (points_remaining_ > 0)
    {
        set_status("Please allocate all stat points.", true);
        return false;
    }

    clear_status();
    return true;
}

void character_create_dialog::set_status(std::string_view message, bool error)
{
    status_message_ = message;
    status_is_error_ = error;
}

void character_create_dialog::clear_status()
{
    status_message_.clear();
    status_is_error_ = false;
}

} // namespace hb
