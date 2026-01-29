#include "ui/dialogs/login_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>

namespace hb {

login_dialog::login_dialog()
    : dialog(dialog_type::login) {
    set_title("Helbreath Login");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - 180,
        static_cast<int32_t>(screen_height) / 2 - 140,
        360, 280
    });
    set_closeable(false);
    set_draggable(false);
    set_modal(true);

    create_ui();
}

void login_dialog::create_ui() {
    // Server selection label
    auto server_label = std::make_unique<ui_label>();
    server_label->set_bounds({20, 40, 100, 20});
    server_label->set_text("Server:");
    add_child(std::move(server_label));

    // Server list
    auto server_list = std::make_unique<ui_list_box>();
    server_list->set_id("server_list");
    server_list->set_bounds({20, 60, 320, 60});
    server_list->set_item_height(20);
    server_list->set_on_select([this](int32_t index) {
        selected_server_ = index;
        if (on_server_select_) {
            on_server_select_(index);
        }
    });
    server_list_ = server_list.get();
    add_child(std::move(server_list));

    // Account label
    auto account_label = std::make_unique<ui_label>();
    account_label->set_bounds({20, 135, 80, 20});
    account_label->set_text("Account:");
    add_child(std::move(account_label));

    // Account input
    auto account_input = std::make_unique<ui_text_input>();
    account_input->set_id("account_input");
    account_input->set_bounds({100, 133, 240, 24});
    account_input->set_max_length(20);
    account_input->set_placeholder("Enter account name");
    account_input_ = account_input.get();
    add_child(std::move(account_input));

    // Password label
    auto password_label = std::make_unique<ui_label>();
    password_label->set_bounds({20, 170, 80, 20});
    password_label->set_text("Password:");
    add_child(std::move(password_label));

    // Password input
    auto password_input = std::make_unique<ui_text_input>();
    password_input->set_id("password_input");
    password_input->set_bounds({100, 168, 240, 24});
    password_input->set_max_length(20);
    password_input->set_password_mode(true);
    password_input->set_placeholder("Enter password");
    password_input_ = password_input.get();
    add_child(std::move(password_input));

    // Login button
    auto login_btn = std::make_unique<ui_button>();
    login_btn->set_id("login_button");
    login_btn->set_bounds({60, 210, 100, 32});
    login_btn->set_text("Login");
    login_btn->set_on_click([this]() { on_login_clicked(); });
    add_child(std::move(login_btn));

    // Create account button
    auto create_btn = std::make_unique<ui_button>();
    create_btn->set_id("create_button");
    create_btn->set_bounds({200, 210, 100, 32});
    create_btn->set_text("New Account");
    create_btn->set_on_click([this]() { on_create_clicked(); });
    add_child(std::move(create_btn));
}

void login_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;
    dialog::update(delta_time, inp);

    // Handle Enter key to submit login
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter)) {
        on_login_clicked();
    }

    // Tab between input fields
    if (inp.is_key_pressed(sf::Keyboard::Key::Tab)) {
        if (account_input_ && account_input_->focused()) {
            account_input_->set_focused(false);
            if (password_input_) password_input_->set_focused(true);
        } else if (password_input_ && password_input_->focused()) {
            password_input_->set_focused(false);
            if (account_input_) account_input_->set_focused(true);
        }
    }
}

void login_dialog::render(renderer& rend) {
    if (!visible_) return;
    dialog::render(rend);

    // Render status message if any
    if (!status_message_.empty()) {
        sf::Color status_color = status_is_error_ ? sf::Color(255, 100, 100) : sf::Color(100, 255, 100);
        rend.draw_text(status_message_,
            bounds_.x + 20, bounds_.y + 250,
            status_color);
    }
}

void login_dialog::add_server(std::string_view name, std::string_view host, uint16_t port) {
    servers_.push_back({std::string(name), std::string(host), port});
    if (server_list_) {
        server_list_->add_item(name);
        if (servers_.size() == 1) {
            server_list_->set_selected_index(0);
            selected_server_ = 0;
        }
    }
}

void login_dialog::clear_servers() {
    servers_.clear();
    selected_server_ = 0;
    if (server_list_) {
        server_list_->clear_items();
    }
}

void login_dialog::set_selected_server(int32_t index) {
    if (index >= 0 && index < static_cast<int32_t>(servers_.size())) {
        selected_server_ = index;
        if (server_list_) {
            server_list_->set_selected_index(index);
        }
    }
}

void login_dialog::set_status(std::string_view message, bool error) {
    status_message_ = message;
    status_is_error_ = error;
}

void login_dialog::clear_status() {
    status_message_.clear();
    status_is_error_ = false;
}

std::string_view login_dialog::account_text() const {
    return account_input_ ? account_input_->text() : "";
}

std::string_view login_dialog::password_text() const {
    return password_input_ ? password_input_->text() : "";
}

void login_dialog::set_account_text(std::string_view text) {
    if (account_input_) {
        account_input_->set_text(text);
    }
}

void login_dialog::clear_password() {
    if (password_input_) {
        password_input_->set_text("");
    }
}

void login_dialog::on_login_clicked() {
    if (!account_input_ || !password_input_) return;

    std::string_view account = account_input_->text();
    std::string_view password = password_input_->text();

    if (account.empty()) {
        set_status("Please enter your account name.", true);
        return;
    }

    if (password.empty()) {
        set_status("Please enter your password.", true);
        return;
    }

    if (servers_.empty()) {
        set_status("No server selected.", true);
        return;
    }

    clear_status();
    set_status("Connecting...", false);

    if (on_login_) {
        spdlog::info("Login requested: account={}", account);
        on_login_(account, password);
    }
}

void login_dialog::on_create_clicked() {
    if (!account_input_ || !password_input_) return;

    std::string_view account = account_input_->text();
    std::string_view password = password_input_->text();

    if (account.empty()) {
        set_status("Please enter an account name.", true);
        return;
    }

    if (password.empty()) {
        set_status("Please enter a password.", true);
        return;
    }

    if (account.length() < 4) {
        set_status("Account name must be at least 4 characters.", true);
        return;
    }

    if (password.length() < 4) {
        set_status("Password must be at least 4 characters.", true);
        return;
    }

    clear_status();
    set_status("Creating account...", false);

    if (on_create_account_) {
        spdlog::info("Create account requested: account={}", account);
        on_create_account_(account, password);
    }
}

} // namespace hb
