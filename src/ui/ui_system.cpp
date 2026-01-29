#include "ui/ui_system.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

// dialog implementation

dialog::dialog(dialog_type type)
    : type_(type) {
    // Dialogs start hidden - must be explicitly opened
    visible_ = false;
}

void dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;
    ui_panel::update(delta_time, inp);
}

void dialog::render(renderer& rend) {
    if (!visible_) return;

    ui_panel::render(rend);
    render_title_bar(rend);
}

bool dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check title bar for dragging
    if (draggable_ && btn == sf::Mouse::Button::Left) {
        ui_rect title_rect{bounds_.x, bounds_.y, bounds_.width, title_bar_height};
        if (title_rect.contains(x, y)) {
            // Check close button
            if (closeable_) {
                ui_rect close_rect{bounds_.x + bounds_.width - 20, bounds_.y + 4, 16, 16};
                if (close_rect.contains(x, y)) {
                    close();
                    return true;
                }
            }

            dragging_ = true;
            drag_offset_x_ = x - bounds_.x;
            drag_offset_y_ = y - bounds_.y;
            return true;
        }
    }

    return ui_panel::handle_mouse_down(x, y, btn);
}

bool dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (dragging_) {
        bounds_.x = x - drag_offset_x_;
        bounds_.y = y - drag_offset_y_;

        // Clamp to screen
        bounds_.x = std::clamp(bounds_.x, 0, static_cast<int32_t>(screen_width) - bounds_.width);
        bounds_.y = std::clamp(bounds_.y, 0, static_cast<int32_t>(screen_height) - bounds_.height);

        return true;
    }

    return ui_panel::handle_mouse_move(x, y);
}

void dialog::open() {
    set_visible(true);
}

void dialog::close() {
    set_visible(false);
    dragging_ = false;
    if (on_close_) {
        on_close_();
    }
}

void dialog::render_title_bar(renderer& rend) {
    // Title bar background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, title_bar_height,
                   sf::Color(60, 60, 80), true);
    rend.draw_line(bounds_.x, bounds_.y + title_bar_height,
                   bounds_.x + bounds_.width, bounds_.y + title_bar_height,
                   sf::Color(100, 100, 140));

    // Title text
    rend.draw_text(title_, bounds_.x + 8, bounds_.y + 4, sf::Color::White);

    // Close button
    if (closeable_) {
        int32_t close_x = bounds_.x + bounds_.width - 20;
        int32_t close_y = bounds_.y + 4;
        rend.draw_rect(close_x, close_y, 16, 16, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White);
    }
}

// ui_system implementation

void ui_system::initialize() {
    spdlog::info("UI system initialized");
}

void ui_system::shutdown() {
    dialogs_.clear();
    dialog_order_.clear();
    focused_ = nullptr;
    spdlog::info("UI system shutdown");
}

void ui_system::update(float delta_time, const input& inp) {
    for (auto* dlg : dialog_order_) {
        dlg->update(delta_time, inp);
    }
}

void ui_system::render(renderer& rend) {
    // Render dialogs in order (back to front)
    for (auto* dlg : dialog_order_) {
        if (dlg->is_open()) {
            dlg->render(rend);
        }
    }

    // Render tooltip
    if (tooltip_visible_ && !tooltip_text_.empty()) {
        int32_t tooltip_width = static_cast<int32_t>(tooltip_text_.length() * 7 + 8);
        int32_t tooltip_height = 20;

        rend.draw_rect(tooltip_x_, tooltip_y_, tooltip_width, tooltip_height,
                       sf::Color(40, 40, 50, 240), true);
        rend.draw_rect(tooltip_x_, tooltip_y_, tooltip_width, tooltip_height,
                       sf::Color(100, 100, 120), false);
        rend.draw_text(tooltip_text_, tooltip_x_ + 4, tooltip_y_ + 3, sf::Color::White);
    }
}

bool ui_system::handle_mouse_move(int32_t x, int32_t y) {
    // Process in reverse order (front to back)
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        if ((*it)->is_open() && (*it)->handle_mouse_move(x, y)) {
            return true;
        }
    }
    return false;
}

bool ui_system::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    // Check if clicking outside modal dialog
    if (is_modal_open()) {
        for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
            if ((*it)->is_open() && (*it)->modal()) {
                if (!(*it)->bounds().contains(x, y)) {
                    return true;  // Block input outside modal
                }
                break;
            }
        }
    }

    // Process dialogs
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        if ((*it)->is_open()) {
            if ((*it)->bounds().contains(x, y)) {
                bring_to_front(*it);
                (*it)->handle_mouse_down(x, y, btn);
                return true;
            }
        }
    }

    // Clear focus if clicking outside
    if (focused_) {
        focused_->set_focused(false);
        focused_ = nullptr;
    }

    return false;
}

bool ui_system::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) {
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        if ((*it)->is_open() && (*it)->handle_mouse_up(x, y, btn)) {
            return true;
        }
    }
    return false;
}

bool ui_system::handle_key_press(sf::Keyboard::Key key) {
    // Handle escape to close top dialog
    if (key == sf::Keyboard::Key::Escape) {
        for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
            if ((*it)->is_open() && (*it)->closeable()) {
                (*it)->close();
                return true;
            }
        }
    }

    if (focused_) {
        return focused_->handle_key_press(key);
    }

    return false;
}

bool ui_system::handle_text_input(char32_t unicode) {
    if (focused_) {
        return focused_->handle_text_input(unicode);
    }
    return false;
}

dialog* ui_system::get_dialog(dialog_type type) {
    auto it = dialogs_.find(type);
    if (it != dialogs_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ui_system::open_dialog(dialog_type type) {
    auto* dlg = get_dialog(type);
    if (dlg) {
        dlg->open();
        bring_to_front(dlg);
    }
}

void ui_system::close_dialog(dialog_type type) {
    auto* dlg = get_dialog(type);
    if (dlg) {
        dlg->close();
    }
}

void ui_system::toggle_dialog(dialog_type type) {
    auto* dlg = get_dialog(type);
    if (dlg) {
        if (dlg->is_open()) {
            dlg->close();
        } else {
            dlg->open();
            bring_to_front(dlg);
        }
    }
}

bool ui_system::is_dialog_open(dialog_type type) const {
    auto it = dialogs_.find(type);
    if (it != dialogs_.end()) {
        return it->second->is_open();
    }
    return false;
}

void ui_system::close_all_dialogs() {
    for (auto& [type, dlg] : dialogs_) {
        dlg->close();
    }
}

void ui_system::create_login_dialog() {
    auto dlg = std::make_unique<login_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::login] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_character_select_dialog() {
    auto dlg = std::make_unique<character_select_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::character_select] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_character_dialog() {
    auto dlg = std::make_unique<character_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::character_info] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_inventory_dialog() {
    auto dlg = std::make_unique<inventory_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::inventory] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_equipment_dialog() {
    auto dlg = std::make_unique<equipment_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::equipment] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_spellbook_dialog() {
    auto dlg = std::make_unique<spellbook_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::spellbook] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_skills_dialog() {
    auto dlg = std::make_unique<skills_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::skills] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_shop_dialog() {
    auto dlg = std::make_unique<shop_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::shop] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_bank_dialog() {
    auto dlg = std::make_unique<bank_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::bank] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_party_dialog() {
    auto dlg = std::make_unique<party_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::party] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_guild_dialog() {
    auto dlg = std::make_unique<guild_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::guild] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_npc_dialog() {
    auto dlg = std::make_unique<npc_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::npc_dialog] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_chat_dialog() {
    auto dlg = std::make_unique<chat_dialog>();
    dlg->set_draggable(false);
    dlg->set_closeable(false);

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::chat] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_system_menu_dialog() {
    auto dlg = std::make_unique<system_menu_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::system_menu] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_options_dialog() {
    auto dlg = std::make_unique<settings_dialog>();
    dlg->set_ui_style(style_);  // Initialize with current UI style
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::options] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_trade_dialog() {
    auto dlg = std::make_unique<trade_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::trade] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_craft_dialog() {
    auto dlg = std::make_unique<craft_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::manufacture] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_map_dialog() {
    auto dlg = std::make_unique<map_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::map] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_repair_dialog() {
    auto dlg = std::make_unique<repair_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::repair] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_help_dialog() {
    auto dlg = std::make_unique<help_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::help] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_message_box(std::string_view title, std::string_view message,
                                   std::function<void()> on_ok) {
    auto dlg = std::make_unique<dialog>(dialog_type::message_box);
    dlg->set_title(title);
    dlg->set_bounds({static_cast<int32_t>(screen_width) / 2 - 150,
                     static_cast<int32_t>(screen_height) / 2 - 60, 300, 120});
    dlg->set_modal(true);

    // Message
    auto msg_label = std::make_unique<ui_label>();
    msg_label->set_bounds({20, 40, 260, 40});
    msg_label->set_text(message);
    msg_label->set_alignment(ui_label::alignment::center);
    dlg->add_child(std::move(msg_label));

    // OK button
    auto ok_btn = std::make_unique<ui_button>();
    ok_btn->set_bounds({110, 80, 80, 28});
    ok_btn->set_text("OK");
    ok_btn->set_on_click([this, on_ok]() {
        close_dialog(dialog_type::message_box);
        if (on_ok) on_ok();
    });
    dlg->add_child(std::move(ok_btn));

    // Remove old message box if exists
    dialogs_.erase(dialog_type::message_box);
    dialog_order_.erase(
        std::remove_if(dialog_order_.begin(), dialog_order_.end(),
            [](dialog* d) { return d->type() == dialog_type::message_box; }),
        dialog_order_.end()
    );

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::message_box] = std::move(dlg);
    dialog_order_.push_back(ptr);
    ptr->open();
}

void ui_system::create_confirm_box(std::string_view title, std::string_view message,
                                   std::function<void(bool)> on_result) {
    auto dlg = std::make_unique<dialog>(dialog_type::confirm);
    dlg->set_title(title);
    dlg->set_bounds({static_cast<int32_t>(screen_width) / 2 - 150,
                     static_cast<int32_t>(screen_height) / 2 - 60, 300, 120});
    dlg->set_modal(true);

    // Message
    auto msg_label = std::make_unique<ui_label>();
    msg_label->set_bounds({20, 40, 260, 40});
    msg_label->set_text(message);
    msg_label->set_alignment(ui_label::alignment::center);
    dlg->add_child(std::move(msg_label));

    // Yes button
    auto yes_btn = std::make_unique<ui_button>();
    yes_btn->set_bounds({60, 80, 80, 28});
    yes_btn->set_text("Yes");
    yes_btn->set_on_click([this, on_result]() {
        close_dialog(dialog_type::confirm);
        if (on_result) on_result(true);
    });
    dlg->add_child(std::move(yes_btn));

    // No button
    auto no_btn = std::make_unique<ui_button>();
    no_btn->set_bounds({160, 80, 80, 28});
    no_btn->set_text("No");
    no_btn->set_on_click([this, on_result]() {
        close_dialog(dialog_type::confirm);
        if (on_result) on_result(false);
    });
    dlg->add_child(std::move(no_btn));

    // Remove old confirm box
    dialogs_.erase(dialog_type::confirm);
    dialog_order_.erase(
        std::remove_if(dialog_order_.begin(), dialog_order_.end(),
            [](dialog* d) { return d->type() == dialog_type::confirm; }),
        dialog_order_.end()
    );

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::confirm] = std::move(dlg);
    dialog_order_.push_back(ptr);
    ptr->open();
}

void ui_system::create_input_box(std::string_view title, std::string_view prompt,
                                 std::function<void(std::string_view)> on_submit) {
    auto dlg = std::make_unique<dialog>(dialog_type::input_box);
    dlg->set_title(title);
    dlg->set_bounds({static_cast<int32_t>(screen_width) / 2 - 150,
                     static_cast<int32_t>(screen_height) / 2 - 70, 300, 140});
    dlg->set_modal(true);

    // Prompt
    auto prompt_label = std::make_unique<ui_label>();
    prompt_label->set_bounds({20, 40, 260, 20});
    prompt_label->set_text(prompt);
    dlg->add_child(std::move(prompt_label));

    // Input
    auto text_input = std::make_unique<ui_text_input>();
    text_input->set_id("input_field");
    text_input->set_bounds({20, 65, 260, 24});
    dlg->add_child(std::move(text_input));

    // OK button
    auto ok_btn = std::make_unique<ui_button>();
    ok_btn->set_bounds({60, 100, 80, 28});
    ok_btn->set_text("OK");
    ok_btn->set_on_click([this, on_submit]() {
        auto* dlg = get_dialog(dialog_type::input_box);
        if (dlg) {
            if (auto* input = dlg->find_child("input_field")) {
                auto* text_input = static_cast<ui_text_input*>(input);
                if (on_submit) on_submit(text_input->text());
            }
        }
        close_dialog(dialog_type::input_box);
    });
    dlg->add_child(std::move(ok_btn));

    // Cancel button
    auto cancel_btn = std::make_unique<ui_button>();
    cancel_btn->set_bounds({160, 100, 80, 28});
    cancel_btn->set_text("Cancel");
    cancel_btn->set_on_click([this]() {
        close_dialog(dialog_type::input_box);
    });
    dlg->add_child(std::move(cancel_btn));

    // Remove old input box
    dialogs_.erase(dialog_type::input_box);
    dialog_order_.erase(
        std::remove_if(dialog_order_.begin(), dialog_order_.end(),
            [](dialog* d) { return d->type() == dialog_type::input_box; }),
        dialog_order_.end()
    );

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::input_box] = std::move(dlg);
    dialog_order_.push_back(ptr);
    ptr->open();
}

void ui_system::show_tooltip(std::string_view text, int32_t x, int32_t y) {
    tooltip_text_ = text;
    tooltip_x_ = x;
    tooltip_y_ = y;
    tooltip_visible_ = true;
}

void ui_system::hide_tooltip() {
    tooltip_visible_ = false;
}

void ui_system::set_focus(ui_element* element) {
    if (focused_) {
        focused_->set_focused(false);
    }
    focused_ = element;
    if (focused_) {
        focused_->set_focused(true);
    }
}

bool ui_system::is_modal_open() const {
    for (const auto& [type, dlg] : dialogs_) {
        if (dlg->is_open() && dlg->modal()) {
            return true;
        }
    }
    return false;
}

void ui_system::bring_to_front(dialog* dlg) {
    auto it = std::find(dialog_order_.begin(), dialog_order_.end(), dlg);
    if (it != dialog_order_.end()) {
        dialog_order_.erase(it);
        dialog_order_.push_back(dlg);
    }
}

void ui_system::create_character_create_dialog() {
    auto dlg = std::make_unique<character_create_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::character_create] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_icon_panel_dialog() {
    auto dlg = std::make_unique<icon_panel_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::icon_panel] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_gauge_panel_dialog() {
    auto dlg = std::make_unique<gauge_panel_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::gauge_panel] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_levelup_dialog() {
    auto dlg = std::make_unique<levelup_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::levelup] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

} // namespace hb
