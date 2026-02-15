#pragma once

#include "ui/dialog_definition.hpp"
#include "ui/managed_dialog.hpp"
#include "ui/render_strategy.hpp"
#include "ui/dialogs/yaml_help_dialog.hpp"
#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hb
{

class sprite_manager;
class renderer;
class input;

// Central coordinator for the new dialog system
// Manages dialog definitions, creation, and lifecycle
class dialog_manager
{
public:
    dialog_manager() = default;
    ~dialog_manager() = default;

    dialog_manager(const dialog_manager&) = delete;
    dialog_manager& operator=(const dialog_manager&) = delete;

    // === Initialization ===

    void initialize(sprite_manager* sprites);
    void shutdown();

    // === Definition management ===

    // Register a dialog definition (from code)
    void register_definition(dialog_definition def);

    // Load definitions from YAML file
    bool load_definitions_from_yaml(const std::filesystem::path& yaml_path);

    // Load all YAML files from a directory
    void load_definitions_from_directory(const std::filesystem::path& dir);

    // Get a registered definition
    const dialog_definition* get_definition(std::string_view id) const;

    // List all registered definition IDs
    std::vector<std::string_view> list_definitions() const;

    // === Dialog creation ===

    // Create a managed dialog from a registered definition
    managed_dialog* create_dialog(std::string_view definition_id);

    // Create a custom dialog subclass from a registered definition
    template<typename T, typename... Args> T* create_custom_dialog(std::string_view definition_id, Args&&... args);

    // Create the YAML-based help dialog with topics loaded from YAML
    yaml_help_dialog* create_help_dialog();

    // Create the YAML-based icon panel (main HUD)
    yaml_icon_panel_dialog* create_icon_panel_dialog();

    // === Render mode ===

    // Set default render mode for new dialogs
    void set_default_render_mode(render_mode mode);
    render_mode default_render_mode() const { return default_mode_; }

    // === Update/render ===

    void update(float delta_time, const input& inp);
    void render(renderer& rend);

    // === Dialog management ===

    // Find a created dialog by definition ID
    managed_dialog* find_dialog(std::string_view definition_id);

    // Open a dialog (creates if not exists)
    managed_dialog* open_dialog(std::string_view definition_id);

    // Close a dialog
    void close_dialog(std::string_view definition_id);

    // Close all managed dialogs
    void close_all();

    // Destroy a dialog (removes from list)
    void destroy_dialog(std::string_view definition_id);

    // Bring dialog to front
    void bring_to_front(std::string_view definition_id);

    // === Input handling ===

    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn);
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn);
    bool handle_mouse_move(int32_t x, int32_t y);
    bool handle_mouse_wheel(int32_t x, int32_t y, int32_t delta);
    bool handle_key_press(sf::Keyboard::Key key);
    bool handle_text_input(char32_t unicode);

    // Check if any modal dialog is open
    bool is_modal_open() const;

    // Check if a screen point is over any open dialog
    bool is_point_over_dialog(int32_t x, int32_t y) const;

    // Find the topmost open dialog at a screen point (z-order aware)
    managed_dialog* find_topmost_at(int32_t x, int32_t y) const;

    // === Development/hot reload ===

    // Enable/disable automatic hot-reload (polls for file changes)
    void set_hot_reload_enabled(bool enabled) { hot_reload_enabled_ = enabled; }
    bool hot_reload_enabled() const { return hot_reload_enabled_; }

    // Set polling interval (default 200ms)
    void set_hot_reload_interval(std::chrono::milliseconds interval) { poll_interval_ = interval; }

    // Manually reload all definitions
    void reload_definitions();

    // Check for file changes and reload if needed (called automatically from update() if enabled)
    bool check_for_changes();

private:
    // All registered definitions
    std::map<std::string, dialog_definition, std::less<>> definitions_;

    // Created dialogs (keyed by definition ID for easy lookup)
    std::map<std::string, std::unique_ptr<managed_dialog>, std::less<>> dialogs_;

    // Z-order for rendering (back to front)
    std::vector<managed_dialog*> dialog_order_;

    // Services
    sprite_manager* sprites_ = nullptr;
    render_mode default_mode_ = render_mode::modern;

    // Paths for hot reload
    std::vector<std::filesystem::path> yaml_paths_;

    // Hot-reload state
    bool hot_reload_enabled_ = false;
    std::chrono::milliseconds poll_interval_{200};
    std::chrono::steady_clock::time_point last_poll_time_;
    std::map<std::filesystem::path, std::filesystem::file_time_type> file_mod_times_;
};

// === Template implementation ===

template<typename T, typename... Args>
T* dialog_manager::create_custom_dialog(std::string_view definition_id, Args&&... args)
{
    // Find definition
    auto def_it = definitions_.find(std::string(definition_id));
    if (def_it == definitions_.end())
    {
        return nullptr;
    }

    // Check if already created
    std::string id_str(definition_id);
    auto dlg_it = dialogs_.find(id_str);
    if (dlg_it != dialogs_.end())
    {
        // Return existing dialog (downcasted)
        return dynamic_cast<T*>(dlg_it->second.get());
    }

    // Create custom dialog with definition
    auto dlg = std::make_unique<T>(def_it->second, std::forward<Args>(args)...);
    dlg->set_render_mode(default_mode_);
    dlg->set_sprite_manager(sprites_);

    T* ptr = dlg.get();
    dialogs_[id_str] = std::move(dlg);
    dialog_order_.push_back(ptr);

    return ptr;
}

} // namespace hb
