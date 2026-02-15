#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <string>
#include <string_view>

namespace hb
{

class login_dialog : public dialog
{
public:
    login_dialog();
    ~login_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;

    // Callbacks
    using login_callback = std::function<void(std::string_view account, std::string_view password)>;
    using create_account_callback = std::function<void(std::string_view account, std::string_view password)>;
    using server_select_callback = std::function<void(int32_t server_index)>;

    void set_on_login(login_callback callback) { on_login_ = std::move(callback); }
    void set_on_create_account(create_account_callback callback) { on_create_account_ = std::move(callback); }
    void set_on_server_select(server_select_callback callback) { on_server_select_ = std::move(callback); }

    // Server list
    void add_server(std::string_view name, std::string_view host, uint16_t port);
    void clear_servers();
    void set_selected_server(int32_t index);

    // Status message
    void set_status(std::string_view message, bool error = false);
    void clear_status();

    // Access input fields
    std::string_view account_text() const;
    std::string_view password_text() const;
    void set_account_text(std::string_view text);
    void clear_password();

private:
    void create_ui();
    void on_login_clicked();
    void on_create_clicked();

    struct server_info
    {
        std::string name;
        std::string host;
        uint16_t port;
    };

    std::vector<server_info> servers_;
    int32_t selected_server_ = 0;
    std::string status_message_;
    bool status_is_error_ = false;

    login_callback on_login_;
    create_account_callback on_create_account_;
    server_select_callback on_server_select_;

    // UI elements (stored for direct access)
    ui_text_input* account_input_ = nullptr;
    ui_text_input* password_input_ = nullptr;
    ui_list_box* server_list_ = nullptr;
};

} // namespace hb
