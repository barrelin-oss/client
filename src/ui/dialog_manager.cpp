#include "ui/dialog_manager.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>

namespace hb {

// ============================================================================
// Helper functions for parsing colors and elements from JSON/YAML
// ============================================================================

namespace {

// Parse color from JSON node
sf::Color parse_color_json(const nlohmann::json& c) {
    return sf::Color(
        static_cast<uint8_t>(c.value("r", 255)),
        static_cast<uint8_t>(c.value("g", 255)),
        static_cast<uint8_t>(c.value("b", 255)),
        static_cast<uint8_t>(c.value("a", 255))
    );
}

// Parse color from YAML node
sf::Color parse_color_yaml(const YAML::Node& c) {
    return sf::Color(
        static_cast<uint8_t>(c["r"] ? c["r"].as<int>(255) : 255),
        static_cast<uint8_t>(c["g"] ? c["g"].as<int>(255) : 255),
        static_cast<uint8_t>(c["b"] ? c["b"].as<int>(255) : 255),
        static_cast<uint8_t>(c["a"] ? c["a"].as<int>(255) : 255)
    );
}

// Parse element type from string
element_type parse_element_type(const std::string& type_str) {
    if (type_str == "label") return element_type::label;
    if (type_str == "button") return element_type::button;
    if (type_str == "text_input") return element_type::text_input;
    if (type_str == "image") return element_type::image;
    if (type_str == "progress_bar") return element_type::progress_bar;
    if (type_str == "checkbox") return element_type::checkbox;
    if (type_str == "slider") return element_type::slider;
    if (type_str == "list_box") return element_type::list_box;
    if (type_str == "grid") return element_type::grid;
    if (type_str == "sprite") return element_type::sprite;
    if (type_str == "panel") return element_type::panel;
    if (type_str == "separator") return element_type::separator;
    // Custom HUD elements
    if (type_str == "custom") return element_type::custom;
    if (type_str == "hp_bar") return element_type::hp_bar;
    if (type_str == "mp_bar") return element_type::mp_bar;
    if (type_str == "sp_bar") return element_type::sp_bar;
    if (type_str == "exp_bar") return element_type::exp_bar;
    if (type_str == "location_text") return element_type::location_text;
    if (type_str == "combat_indicator") return element_type::combat_indicator;
    if (type_str == "super_attack") return element_type::super_attack;
    return element_type::label;  // default
}

// Parse element action from string
element_action parse_element_action(const std::string& action_str) {
    if (action_str == "open_dialog") return element_action::open_dialog;
    if (action_str == "close_dialog") return element_action::close_dialog;
    if (action_str == "close_self") return element_action::close_self;
    if (action_str == "toggle_dialog") return element_action::toggle_dialog;
    return element_action::none;
}

// Parse element from JSON
element_def parse_element_json(const nlohmann::json& elem_json) {
    element_def elem;

    elem.id = elem_json["id"].get<std::string>();
    elem.type = parse_element_type(elem_json.value("type", "label"));

    // Bounds
    if (elem_json.contains("bounds")) {
        const auto& b = elem_json["bounds"];
        elem.bounds.x = b.value("x", 0);
        elem.bounds.y = b.value("y", 0);
        elem.bounds.width = b.value("w", 0);
        elem.bounds.height = b.value("h", 0);
    }

    // Text
    elem.text = elem_json.value("text", "");

    // Colors
    if (elem_json.contains("text_color")) {
        elem.text_color = parse_color_json(elem_json["text_color"]);
    }
    if (elem_json.contains("background_color")) {
        elem.background_color = parse_color_json(elem_json["background_color"]);
    }
    if (elem_json.contains("border_color")) {
        elem.border_color = parse_color_json(elem_json["border_color"]);
    }
    if (elem_json.contains("hover_color")) {
        elem.hover_color = parse_color_json(elem_json["hover_color"]);
    }
    if (elem_json.contains("pressed_color")) {
        elem.pressed_color = parse_color_json(elem_json["pressed_color"]);
    }
    if (elem_json.contains("fill_color")) {
        elem.fill_color = parse_color_json(elem_json["fill_color"]);
    }

    // Sprite
    if (elem_json.contains("sprite_pak")) {
        elem.sprite_pak = elem_json["sprite_pak"].get<std::string>();
    }
    elem.sprite_index = elem_json.value("sprite_index", 0);
    elem.sprite_frame = elem_json.value("sprite_frame", 0);

    // Text input
    elem.max_chars = elem_json.value("max_chars", 256);
    elem.password_mode = elem_json.value("password_mode", false);

    // Grid
    elem.grid_cols = elem_json.value("grid_cols", 1);
    elem.grid_rows = elem_json.value("grid_rows", 1);
    elem.cell_width = elem_json.value("cell_width", 32);
    elem.cell_height = elem_json.value("cell_height", 32);
    elem.cell_padding = elem_json.value("cell_padding", 2);

    // Progress bar
    elem.show_value_text = elem_json.value("show_value_text", false);

    // Slider
    elem.min_value = elem_json.value("min_value", 0.0f);
    elem.max_value = elem_json.value("max_value", 1.0f);
    elem.step = elem_json.value("step", 0.01f);

    // Font size
    if (elem_json.contains("font_size")) {
        elem.font_size = elem_json["font_size"].get<uint32_t>();
    }

    // Tooltip
    elem.tooltip = elem_json.value("tooltip", "");

    // Custom properties
    if (elem_json.contains("properties") && elem_json["properties"].is_object()) {
        for (auto& [key, value] : elem_json["properties"].items()) {
            elem.properties[key] = value.get<std::string>();
        }
    }

    // Action system
    if (elem_json.contains("action")) {
        elem.action = parse_element_action(elem_json["action"].get<std::string>());
    }
    elem.action_target = elem_json.value("action_target", "");

    return elem;
}

// Parse element from YAML
element_def parse_element_yaml(const YAML::Node& elem_yaml) {
    element_def elem;

    elem.id = elem_yaml["id"].as<std::string>();
    elem.type = parse_element_type(elem_yaml["type"] ? elem_yaml["type"].as<std::string>("label") : "label");

    // Bounds
    if (elem_yaml["bounds"]) {
        const auto& b = elem_yaml["bounds"];
        elem.bounds.x = b["x"] ? b["x"].as<int32_t>(0) : 0;
        elem.bounds.y = b["y"] ? b["y"].as<int32_t>(0) : 0;
        elem.bounds.width = b["w"] ? b["w"].as<int32_t>(0) : 0;
        elem.bounds.height = b["h"] ? b["h"].as<int32_t>(0) : 0;
    }

    // Text
    elem.text = elem_yaml["text"] ? elem_yaml["text"].as<std::string>("") : "";

    // Colors
    if (elem_yaml["text_color"]) {
        elem.text_color = parse_color_yaml(elem_yaml["text_color"]);
    }
    if (elem_yaml["background_color"]) {
        elem.background_color = parse_color_yaml(elem_yaml["background_color"]);
    }
    if (elem_yaml["border_color"]) {
        elem.border_color = parse_color_yaml(elem_yaml["border_color"]);
    }
    if (elem_yaml["hover_color"]) {
        elem.hover_color = parse_color_yaml(elem_yaml["hover_color"]);
    }
    if (elem_yaml["pressed_color"]) {
        elem.pressed_color = parse_color_yaml(elem_yaml["pressed_color"]);
    }
    if (elem_yaml["fill_color"]) {
        elem.fill_color = parse_color_yaml(elem_yaml["fill_color"]);
    }

    // Sprite
    elem.sprite_pak = elem_yaml["sprite_pak"] ? elem_yaml["sprite_pak"].as<std::string>("") : "";
    elem.sprite_index = elem_yaml["sprite_index"] ? elem_yaml["sprite_index"].as<int32_t>(0) : 0;
    elem.sprite_frame = elem_yaml["sprite_frame"] ? elem_yaml["sprite_frame"].as<int32_t>(0) : 0;

    // Text input
    elem.max_chars = elem_yaml["max_chars"] ? elem_yaml["max_chars"].as<int32_t>(256) : 256;
    elem.password_mode = elem_yaml["password_mode"] ? elem_yaml["password_mode"].as<bool>(false) : false;

    // Grid
    elem.grid_cols = elem_yaml["grid_cols"] ? elem_yaml["grid_cols"].as<int32_t>(1) : 1;
    elem.grid_rows = elem_yaml["grid_rows"] ? elem_yaml["grid_rows"].as<int32_t>(1) : 1;
    elem.cell_width = elem_yaml["cell_width"] ? elem_yaml["cell_width"].as<int32_t>(32) : 32;
    elem.cell_height = elem_yaml["cell_height"] ? elem_yaml["cell_height"].as<int32_t>(32) : 32;
    elem.cell_padding = elem_yaml["cell_padding"] ? elem_yaml["cell_padding"].as<int32_t>(2) : 2;

    // Progress bar
    elem.show_value_text = elem_yaml["show_value_text"] ? elem_yaml["show_value_text"].as<bool>(false) : false;

    // Slider
    elem.min_value = elem_yaml["min_value"] ? elem_yaml["min_value"].as<float>(0.0f) : 0.0f;
    elem.max_value = elem_yaml["max_value"] ? elem_yaml["max_value"].as<float>(1.0f) : 1.0f;
    elem.step = elem_yaml["step"] ? elem_yaml["step"].as<float>(0.01f) : 0.01f;

    // Font size
    if (elem_yaml["font_size"]) {
        elem.font_size = elem_yaml["font_size"].as<uint32_t>();
    }

    // Tooltip
    elem.tooltip = elem_yaml["tooltip"] ? elem_yaml["tooltip"].as<std::string>("") : "";

    // Custom properties
    if (elem_yaml["properties"] && elem_yaml["properties"].IsMap()) {
        for (auto it = elem_yaml["properties"].begin(); it != elem_yaml["properties"].end(); ++it) {
            elem.properties[it->first.as<std::string>()] = it->second.as<std::string>();
        }
    }

    // Action system
    if (elem_yaml["action"]) {
        elem.action = parse_element_action(elem_yaml["action"].as<std::string>(""));
    }
    elem.action_target = elem_yaml["action_target"] ? elem_yaml["action_target"].as<std::string>("") : "";

    return elem;
}

// Parse dialog definition from JSON
dialog_definition parse_dialog_json(const nlohmann::json& dlg_json) {
    dialog_definition def;

    def.id = dlg_json["id"].get<std::string>();

    // Optional fields
    if (dlg_json.contains("title")) {
        def.title = dlg_json["title"].get<std::string>();
    }

    if (dlg_json.contains("bounds")) {
        const auto& b = dlg_json["bounds"];
        def.bounds.x = b.value("x", 0);
        def.bounds.y = b.value("y", 0);
        def.bounds.width = b.value("w", 200);
        def.bounds.height = b.value("h", 150);
    }

    def.modal = dlg_json.value("modal", false);
    def.draggable = dlg_json.value("draggable", true);
    def.closeable = dlg_json.value("closeable", true);
    def.has_border = dlg_json.value("has_border", true);
    def.has_title_bar = dlg_json.value("has_title_bar", true);
    def.centered = dlg_json.value("centered", false);
    def.always_on_top = dlg_json.value("always_on_top", false);
    def.right_click_closeable = dlg_json.value("right_click_closeable", true);

    // Colors
    if (dlg_json.contains("background_color")) {
        def.background_color = parse_color_json(dlg_json["background_color"]);
    }
    if (dlg_json.contains("border_color")) {
        def.border_color = parse_color_json(dlg_json["border_color"]);
    }
    if (dlg_json.contains("title_bar_color")) {
        def.title_bar_color = parse_color_json(dlg_json["title_bar_color"]);
    }
    if (dlg_json.contains("title_text_color")) {
        def.title_text_color = parse_color_json(dlg_json["title_text_color"]);
    }

    // Background sprite
    if (dlg_json.contains("background_sprite")) {
        const auto& spr = dlg_json["background_sprite"];
        def.background_sprite_pak = spr.value("pak", "");
        def.background_sprite_index = spr.value("index", 0);
        def.background_sprite_frame = spr.value("frame", 0);
    }

    // Parse elements
    if (dlg_json.contains("elements") && dlg_json["elements"].is_array()) {
        for (const auto& elem_json : dlg_json["elements"]) {
            if (!elem_json.contains("id")) {
                spdlog::warn("Element missing 'id' in dialog '{}', skipping", def.id);
                continue;
            }
            def.elements.push_back(parse_element_json(elem_json));
        }
    }

    return def;
}

// Parse dialog definition from YAML
dialog_definition parse_dialog_yaml(const YAML::Node& dlg_yaml) {
    dialog_definition def;

    def.id = dlg_yaml["id"].as<std::string>();

    // Optional fields
    if (dlg_yaml["title"]) {
        def.title = dlg_yaml["title"].as<std::string>();
    }

    if (dlg_yaml["bounds"]) {
        const auto& b = dlg_yaml["bounds"];
        def.bounds.x = b["x"] ? b["x"].as<int32_t>(0) : 0;
        def.bounds.y = b["y"] ? b["y"].as<int32_t>(0) : 0;
        def.bounds.width = b["w"] ? b["w"].as<int32_t>(200) : 200;
        def.bounds.height = b["h"] ? b["h"].as<int32_t>(150) : 150;
    }

    def.modal = dlg_yaml["modal"] ? dlg_yaml["modal"].as<bool>(false) : false;
    def.draggable = dlg_yaml["draggable"] ? dlg_yaml["draggable"].as<bool>(true) : true;
    def.closeable = dlg_yaml["closeable"] ? dlg_yaml["closeable"].as<bool>(true) : true;
    def.has_border = dlg_yaml["has_border"] ? dlg_yaml["has_border"].as<bool>(true) : true;
    def.has_title_bar = dlg_yaml["has_title_bar"] ? dlg_yaml["has_title_bar"].as<bool>(true) : true;
    def.centered = dlg_yaml["centered"] ? dlg_yaml["centered"].as<bool>(false) : false;
    def.always_on_top = dlg_yaml["always_on_top"] ? dlg_yaml["always_on_top"].as<bool>(false) : false;
    def.right_click_closeable = dlg_yaml["right_click_closeable"] ? dlg_yaml["right_click_closeable"].as<bool>(true) : true;

    // Colors
    if (dlg_yaml["background_color"]) {
        def.background_color = parse_color_yaml(dlg_yaml["background_color"]);
    }
    if (dlg_yaml["border_color"]) {
        def.border_color = parse_color_yaml(dlg_yaml["border_color"]);
    }
    if (dlg_yaml["title_bar_color"]) {
        def.title_bar_color = parse_color_yaml(dlg_yaml["title_bar_color"]);
    }
    if (dlg_yaml["title_text_color"]) {
        def.title_text_color = parse_color_yaml(dlg_yaml["title_text_color"]);
    }

    // Background sprite
    if (dlg_yaml["background_sprite"]) {
        const auto& spr = dlg_yaml["background_sprite"];
        def.background_sprite_pak = spr["pak"] ? spr["pak"].as<std::string>("") : "";
        def.background_sprite_index = spr["index"] ? spr["index"].as<int32_t>(0) : 0;
        def.background_sprite_frame = spr["frame"] ? spr["frame"].as<int32_t>(0) : 0;
    }

    // Parse elements
    if (dlg_yaml["elements"] && dlg_yaml["elements"].IsSequence()) {
        for (const auto& elem_yaml : dlg_yaml["elements"]) {
            if (!elem_yaml["id"]) {
                spdlog::warn("Element missing 'id' in dialog '{}', skipping", def.id);
                continue;
            }
            def.elements.push_back(parse_element_yaml(elem_yaml));
        }
    }

    return def;
}

} // anonymous namespace

// ============================================================================
// dialog_manager implementation
// ============================================================================

void dialog_manager::initialize(sprite_manager* sprites) {
    sprites_ = sprites;

#ifdef HB_HOT_RELOAD_ENABLED
    hot_reload_enabled_ = true;
    spdlog::info("Dialog manager initialized (hot-reload enabled)");
#else
    spdlog::info("Dialog manager initialized");
#endif
}

void dialog_manager::shutdown() {
    dialogs_.clear();
    dialog_order_.clear();
    definitions_.clear();
    sprites_ = nullptr;
    spdlog::info("Dialog manager shutdown");
}

void dialog_manager::register_definition(dialog_definition def) {
    std::string id = def.id;
    definitions_[std::move(id)] = std::move(def);
}

bool dialog_manager::load_definitions_from_yaml(const std::filesystem::path& file_path) {
    try {
        // Determine format based on file extension
        auto ext = file_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool is_yaml = (ext == ".yaml" || ext == ".yml");
        bool is_json = (ext == ".json");

        if (!is_yaml && !is_json) {
            spdlog::error("Unsupported file format for dialog definitions: {}", file_path.string());
            return false;
        }

        size_t count_before = definitions_.size();

        if (is_yaml) {
            // Load YAML file
            YAML::Node root = YAML::LoadFile(file_path.string());

            if (!root["dialogs"] || !root["dialogs"].IsSequence()) {
                spdlog::error("Dialog definitions file missing 'dialogs' array: {}", file_path.string());
                return false;
            }

            for (const auto& dlg_yaml : root["dialogs"]) {
                if (!dlg_yaml["id"]) {
                    spdlog::warn("Dialog definition missing 'id', skipping");
                    continue;
                }
                register_definition(parse_dialog_yaml(dlg_yaml));
            }
        } else {
            // Load JSON file
            std::ifstream file(file_path);
            if (!file.is_open()) {
                spdlog::error("Failed to open dialog definitions file: {}", file_path.string());
                return false;
            }

            nlohmann::json root = nlohmann::json::parse(file);

            if (!root.contains("dialogs") || !root["dialogs"].is_array()) {
                spdlog::error("Dialog definitions file missing 'dialogs' array: {}", file_path.string());
                return false;
            }

            for (const auto& dlg_json : root["dialogs"]) {
                if (!dlg_json.contains("id")) {
                    spdlog::warn("Dialog definition missing 'id', skipping");
                    continue;
                }
                register_definition(parse_dialog_json(dlg_json));
            }
        }

        // Store path and modification time for hot reload
        yaml_paths_.push_back(file_path);
        file_mod_times_[file_path] = std::filesystem::last_write_time(file_path);

        size_t count_loaded = definitions_.size() - count_before;
        spdlog::info("Loaded {} dialog definition(s) from {} (total: {})",
                     count_loaded, file_path.string(), definitions_.size());
        return true;

    } catch (const YAML::Exception& e) {
        spdlog::error("YAML parsing error in {}: {}", file_path.string(), e.what());
        return false;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("JSON parsing error in {}: {}", file_path.string(), e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Failed to load dialog definitions from {}: {}", file_path.string(), e.what());
        return false;
    }
}

void dialog_manager::load_definitions_from_directory(const std::filesystem::path& dir) {
    if (!std::filesystem::exists(dir)) {
        spdlog::warn("Dialog definitions directory not found: {}", dir.string());
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".json" || ext == ".yaml" || ext == ".yml") {
                load_definitions_from_yaml(entry.path());
            }
        }
    }
}

const dialog_definition* dialog_manager::get_definition(std::string_view id) const {
    auto it = definitions_.find(std::string(id));
    if (it != definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string_view> dialog_manager::list_definitions() const {
    std::vector<std::string_view> result;
    result.reserve(definitions_.size());
    for (const auto& [id, def] : definitions_) {
        result.push_back(id);
    }
    return result;
}

managed_dialog* dialog_manager::create_dialog(std::string_view definition_id) {
    // Find definition
    auto def_it = definitions_.find(std::string(definition_id));
    if (def_it == definitions_.end()) {
        spdlog::error("Dialog definition not found: {}", definition_id);
        return nullptr;
    }

    // Check if already created
    std::string id_str(definition_id);
    auto dlg_it = dialogs_.find(id_str);
    if (dlg_it != dialogs_.end()) {
        return dlg_it->second.get();
    }

    // Create new dialog
    auto dlg = std::make_unique<managed_dialog>(def_it->second);
    dlg->set_render_mode(default_mode_);
    dlg->set_sprite_manager(sprites_);

    // Set up action handler for YAML-based actions
    dlg->set_action_handler([this](element_action action, std::string_view target) {
        switch (action) {
            case element_action::open_dialog:
                open_dialog(target);
                break;
            case element_action::close_dialog:
            case element_action::close_self:
                close_dialog(target);
                break;
            case element_action::toggle_dialog:
                if (auto* dlg = find_dialog(target)) {
                    if (dlg->is_open()) {
                        dlg->close();
                    } else {
                        dlg->open();
                    }
                } else {
                    open_dialog(target);
                }
                break;
            default:
                break;
        }
    });

    managed_dialog* ptr = dlg.get();
    dialogs_[id_str] = std::move(dlg);

    // Insert respecting always_on_top ordering
    if (ptr->always_on_top()) {
        dialog_order_.push_back(ptr);
    } else {
        // Insert before the first always-on-top dialog
        auto insert_pos = std::find_if(dialog_order_.begin(), dialog_order_.end(),
            [](managed_dialog* d) { return d->always_on_top(); });
        dialog_order_.insert(insert_pos, ptr);
    }

    return ptr;
}

void dialog_manager::set_default_render_mode(render_mode mode) {
    default_mode_ = mode;

    // Update existing dialogs
    for (auto& [id, dlg] : dialogs_) {
        dlg->set_render_mode(mode);
    }
}

void dialog_manager::update(float delta_time, const input& inp) {
    // Check for file changes if hot-reload is enabled
    if (hot_reload_enabled_) {
        check_for_changes();
    }

    for (auto* dlg : dialog_order_) {
        if (dlg->is_open()) {
            dlg->update(delta_time, inp);
        }
    }
}

void dialog_manager::render(renderer& rend) {
    // Render in order (back to front)
    for (auto* dlg : dialog_order_) {
        if (dlg->is_open()) {
            dlg->render(rend);
        }
    }
}

managed_dialog* dialog_manager::find_dialog(std::string_view definition_id) {
    auto it = dialogs_.find(std::string(definition_id));
    if (it != dialogs_.end()) {
        return it->second.get();
    }
    return nullptr;
}

managed_dialog* dialog_manager::open_dialog(std::string_view definition_id) {
    managed_dialog* dlg = find_dialog(definition_id);
    if (!dlg) {
        // Check for special dialog types that need custom creation
        if (definition_id == "help_dialog") {
            dlg = create_help_dialog();
        } else if (definition_id == "icon_panel") {
            dlg = create_icon_panel_dialog();
        } else {
            dlg = create_dialog(definition_id);
        }
    }
    if (dlg) {
        dlg->open();
        bring_to_front(definition_id);
    }
    return dlg;
}

void dialog_manager::close_dialog(std::string_view definition_id) {
    if (auto* dlg = find_dialog(definition_id)) {
        dlg->close();
    }
}

void dialog_manager::close_all() {
    for (auto& [id, dlg] : dialogs_) {
        dlg->close();
    }
}

void dialog_manager::destroy_dialog(std::string_view definition_id) {
    std::string id_str(definition_id);

    auto it = dialogs_.find(id_str);
    if (it != dialogs_.end()) {
        // Remove from order
        dialog_order_.erase(
            std::remove(dialog_order_.begin(), dialog_order_.end(), it->second.get()),
            dialog_order_.end()
        );

        // Destroy
        dialogs_.erase(it);
    }
}

void dialog_manager::bring_to_front(std::string_view definition_id) {
    if (auto* dlg = find_dialog(definition_id)) {
        auto it = std::find(dialog_order_.begin(), dialog_order_.end(), dlg);
        if (it != dialog_order_.end()) {
            dialog_order_.erase(it);

            // Respect always_on_top: non-always-on-top dialogs go before
            // always-on-top dialogs, while always-on-top dialogs go to the end
            if (dlg->always_on_top()) {
                dialog_order_.push_back(dlg);
            } else {
                // Insert before the first always-on-top dialog
                auto insert_pos = std::find_if(dialog_order_.begin(), dialog_order_.end(),
                    [](managed_dialog* d) { return d->always_on_top(); });
                dialog_order_.insert(insert_pos, dlg);
            }
        }
    }
}

bool dialog_manager::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    // Handle in reverse order (front to back)
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        auto* dlg = *it;
        if (dlg->is_open()) {
            if (dlg->modal()) {
                // Modal dialog captures all input
                dlg->handle_mouse_down(x, y, btn);
                return true;
            }

            if (dlg->bounds().contains(x, y)) {
                bring_to_front(dlg->definition().id);
                dlg->handle_mouse_down(x, y, btn);
                return true;
            }
        }
    }
    return false;
}

bool dialog_manager::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) {
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        auto* dlg = *it;
        if (dlg->is_open() && dlg->handle_mouse_up(x, y, btn)) {
            return true;
        }
    }
    return false;
}

bool dialog_manager::handle_mouse_move(int32_t x, int32_t y) {
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        auto* dlg = *it;
        if (dlg->is_open() && dlg->handle_mouse_move(x, y)) {
            return true;
        }
    }
    return false;
}

bool dialog_manager::handle_mouse_wheel(int32_t x, int32_t y, int32_t delta) {
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        auto* dlg = *it;
        if (dlg->is_open() && dlg->bounds().contains(x, y)) {
            if (dlg->handle_mouse_wheel(x, y, delta)) {
                return true;
            }
        }
    }
    return false;
}

bool dialog_manager::handle_key_press(sf::Keyboard::Key key) {
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        auto* dlg = *it;
        if (dlg->is_open()) {
            if (dlg->handle_key_press(key)) {
                return true;
            }
            if (dlg->modal()) {
                return true;  // Modal blocks further processing
            }
        }
    }
    return false;
}

bool dialog_manager::handle_text_input(char32_t unicode) {
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it) {
        auto* dlg = *it;
        if (dlg->is_open() && dlg->handle_text_input(unicode)) {
            return true;
        }
    }
    return false;
}

bool dialog_manager::is_modal_open() const {
    for (const auto& [id, dlg] : dialogs_) {
        if (dlg->is_open() && dlg->modal()) {
            return true;
        }
    }
    return false;
}

bool dialog_manager::is_point_over_dialog(int32_t x, int32_t y) const {
    for (const auto& [id, dlg] : dialogs_) {
        if (dlg->is_open() && dlg->bounds().contains(x, y)) {
            return true;
        }
    }
    return false;
}

void dialog_manager::reload_definitions() {
    // Clear only definitions, keep dialogs and their state
    definitions_.clear();

    // Make a copy of paths since load_definitions_from_yaml will append to yaml_paths_
    auto paths_copy = yaml_paths_;
    yaml_paths_.clear();
    file_mod_times_.clear();

    // Reload all YAML files
    for (const auto& path : paths_copy) {
        load_definitions_from_yaml(path);
    }

    // Update existing dialogs with new definitions (preserves state and callbacks)
    for (auto& [id, dlg] : dialogs_) {
        auto def_it = definitions_.find(id);
        if (def_it != definitions_.end()) {
            dlg->update_definition(def_it->second);
            spdlog::info("Hot-reloaded dialog '{}'", id);
        }
    }

    spdlog::info("Reloaded {} dialog definitions", definitions_.size());
}

bool dialog_manager::check_for_changes() {
    auto now = std::chrono::steady_clock::now();

    // Only check at the configured interval
    if (now - last_poll_time_ < poll_interval_) {
        return false;
    }
    last_poll_time_ = now;

    // Check each tracked file for modifications
    bool any_changed = false;
    for (const auto& path : yaml_paths_) {
        try {
            if (!std::filesystem::exists(path)) {
                continue;
            }

            auto current_mod_time = std::filesystem::last_write_time(path);
            auto it = file_mod_times_.find(path);

            if (it != file_mod_times_.end() && current_mod_time != it->second) {
                any_changed = true;
                spdlog::info("Detected change in dialog definition file: {}", path.string());
                break;  // One change is enough to trigger reload
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // File might be temporarily locked by editor, ignore
            spdlog::debug("Could not check file {}: {}", path.string(), e.what());
        }
    }

    if (any_changed) {
        reload_definitions();
        return true;
    }

    return false;
}

yaml_help_dialog* dialog_manager::create_help_dialog() {
    // Check if already created
    auto existing = dialogs_.find("help_dialog");
    if (existing != dialogs_.end()) {
        return dynamic_cast<yaml_help_dialog*>(existing->second.get());
    }

    // Find the definition
    auto def_it = definitions_.find("help_dialog");
    if (def_it == definitions_.end()) {
        spdlog::error("Help dialog definition 'help_dialog' not found");
        return nullptr;
    }

    // Create the dialog
    auto dlg = std::make_unique<yaml_help_dialog>(def_it->second);
    dlg->set_render_mode(default_mode_);
    dlg->set_sprite_manager(sprites_);

    // Try to load help topics from the YAML file
    // Search through tracked YAML paths for the help_dialog.yaml file
    for (const auto& yaml_path : yaml_paths_) {
        try {
            YAML::Node root = YAML::LoadFile(yaml_path.string());

            // Check if this file has help_topics
            if (!root["help_topics"]) {
                continue;
            }

            // Load categories
            std::vector<help_category> categories;
            if (root["help_config"] && root["help_config"]["categories"]) {
                for (const auto& cat_yaml : root["help_config"]["categories"]) {
                    help_category cat;
                    cat.name = cat_yaml["name"] ? cat_yaml["name"].as<std::string>("") : "";
                    cat.id = cat_yaml["id"] ? cat_yaml["id"].as<int32_t>(0) : 0;
                    categories.push_back(std::move(cat));
                }
            }

            // Default categories if none specified
            if (categories.empty()) {
                categories = {
                    {"All", -1},
                    {"Basics", 0},
                    {"Combat", 1},
                    {"Magic", 2},
                    {"Items", 3},
                    {"Keys", 5}
                };
            }
            dlg->set_categories(std::move(categories));

            // Load topics
            std::vector<yaml_help_topic> topics;
            for (const auto& topic_yaml : root["help_topics"]) {
                yaml_help_topic topic;
                topic.title = topic_yaml["title"] ? topic_yaml["title"].as<std::string>("") : "";
                topic.category = topic_yaml["category"] ? topic_yaml["category"].as<int32_t>(0) : 0;

                if (topic_yaml["content"] && topic_yaml["content"].IsSequence()) {
                    for (const auto& line : topic_yaml["content"]) {
                        topic.content_lines.push_back(line.as<std::string>(""));
                    }
                }

                topics.push_back(std::move(topic));
            }
            dlg->set_topics(std::move(topics));

            spdlog::info("Loaded {} help topics from {}", topics.size(), yaml_path.string());
            break;  // Found and loaded, stop searching
        } catch (const YAML::Exception& e) {
            spdlog::debug("Could not load help topics from {}: {}", yaml_path.string(), e.what());
        }
    }

    // Set up action handler
    dlg->set_action_handler([this](element_action action, std::string_view target) {
        switch (action) {
            case element_action::open_dialog:
                open_dialog(target);
                break;
            case element_action::close_dialog:
            case element_action::close_self:
                close_dialog(target);
                break;
            case element_action::toggle_dialog:
                if (auto* d = find_dialog(target)) {
                    if (d->is_open()) {
                        d->close();
                    } else {
                        d->open();
                    }
                } else {
                    open_dialog(target);
                }
                break;
            default:
                break;
        }
    });

    yaml_help_dialog* ptr = dlg.get();
    dialogs_["help_dialog"] = std::move(dlg);

    // Insert respecting always_on_top ordering
    if (ptr->always_on_top()) {
        dialog_order_.push_back(ptr);
    } else {
        // Insert before the first always-on-top dialog
        auto insert_pos = std::find_if(dialog_order_.begin(), dialog_order_.end(),
            [](managed_dialog* d) { return d->always_on_top(); });
        dialog_order_.insert(insert_pos, ptr);
    }

    return ptr;
}

yaml_icon_panel_dialog* dialog_manager::create_icon_panel_dialog() {
    // Check if already created
    auto existing = dialogs_.find("icon_panel");
    if (existing != dialogs_.end()) {
        return dynamic_cast<yaml_icon_panel_dialog*>(existing->second.get());
    }

    // Find the definition
    auto def_it = definitions_.find("icon_panel");
    if (def_it == definitions_.end()) {
        spdlog::error("Icon panel definition 'icon_panel' not found");
        return nullptr;
    }

    // Create the dialog
    auto dlg = std::make_unique<yaml_icon_panel_dialog>(def_it->second);
    dlg->set_render_mode(default_mode_);
    dlg->set_sprite_manager(sprites_);

    // Note: Action handler is NOT set for icon panel because button actions
    // are handled by code callbacks (set_on_character, set_on_inventory, etc.)
    // This allows buttons to toggle legacy code-based dialogs.

    yaml_icon_panel_dialog* ptr = dlg.get();
    dialogs_["icon_panel"] = std::move(dlg);

    // Insert respecting always_on_top ordering
    if (ptr->always_on_top()) {
        dialog_order_.push_back(ptr);
    } else {
        // Insert before the first always-on-top dialog
        auto insert_pos = std::find_if(dialog_order_.begin(), dialog_order_.end(),
            [](managed_dialog* d) { return d->always_on_top(); });
        dialog_order_.insert(insert_pos, ptr);
    }

    spdlog::info("Created YAML-based icon panel dialog");
    return ptr;
}

} // namespace hb
