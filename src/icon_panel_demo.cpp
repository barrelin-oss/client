// Icon Panel Demo - Minimal test for classic rendering
// Build: cmake --build build --config Release --target icon_panel_demo
// Run from client directory: build\src\Release\icon_panel_demo.exe

#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "ui/dialog_manager.hpp"
#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include <spdlog/spdlog.h>
#include <SFML/Graphics.hpp>

using namespace hb;

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("=== Icon Panel Demo ===");

    // 1. Create window and renderer
    renderer rend;
    if (!rend.initialize(640, 480, false)) {
        spdlog::error("Failed to initialize renderer");
        return 1;
    }
    spdlog::info("Renderer initialized");

    // 2. Initialize sprite manager and load GameDialog.pak
    sprite_manager sprites;
    sprites.initialize("assets/");

    if (sprites.load_pak("GameDialog", "sprites/GameDialog.pak")) {
        spdlog::info("Loaded GameDialog.pak");

        // Check what's in sprite index 6 (icon panel)
        const sprite* icon_spr = sprites.get_sprite("GameDialog", 6);
        if (icon_spr) {
            spdlog::info("GameDialog sprite 6: frame_count={}", icon_spr->frame_count());
        } else {
            spdlog::error("GameDialog sprite 6 not found!");
        }
    } else {
        spdlog::error("Failed to load GameDialog.pak");
    }

    // 3. Create dialog manager and load YAML definitions
    dialog_manager dlg_mgr;
    dlg_mgr.initialize(&sprites);
    dlg_mgr.load_definitions_from_directory("assets/ui/dialogs");
    spdlog::info("Dialog manager initialized");

    // 4. Check if icon_panel definition was loaded
    auto* def = dlg_mgr.get_definition("icon_panel");
    if (def) {
        spdlog::info("Found icon_panel definition:");
        spdlog::info("  - background_sprite_pak: '{}'", def->background_sprite_pak);
        spdlog::info("  - background_sprite_index: {}", def->background_sprite_index);
        spdlog::info("  - background_sprite_frame: {}", def->background_sprite_frame);
    } else {
        spdlog::error("icon_panel definition NOT found!");
    }

    // 5. Create the icon panel dialog
    yaml_icon_panel_dialog* icon_panel = dlg_mgr.create_icon_panel_dialog();
    if (icon_panel) {
        spdlog::info("Created icon panel dialog");
        spdlog::info("  - sprites() = {}", icon_panel->sprites() != nullptr);
        icon_panel->open();

        // Set some test values
        icon_panel->set_hp(75, 100);
        icon_panel->set_mp(50, 80);
        icon_panel->set_sp(30, 60);
        icon_panel->set_map_name("TestMap");
        icon_panel->set_position(123, 456);
    } else {
        spdlog::error("Failed to create icon panel dialog!");
    }

    // 6. Create input handler
    input inp;

    // 7. Main loop
    spdlog::info("Entering main loop...");
    sf::Clock clock;

    while (rend.is_open()) {
        float dt = clock.restart().asSeconds();

        // Process events
        while (auto event = rend.window().pollEvent()) {
            inp.process_event(*event);
        }

        if (inp.is_key_pressed(sf::Keyboard::Key::Escape) || inp.should_close()) {
            break;
        }

        // Toggle classic/modern with 'C' key
        static bool classic_mode = true;
        if (inp.is_key_pressed(sf::Keyboard::Key::C)) {
            classic_mode = !classic_mode;
            spdlog::info("Render mode: {}", classic_mode ? "CLASSIC" : "MODERN");
            // Note: The yaml_icon_panel_dialog checks sprites_ to decide rendering
        }

        // Update
        dlg_mgr.update(dt, inp);

        // Render
        rend.begin_frame();

        // Draw a dark background
        rend.draw_rect(0, 0, 640, 480, sf::Color(30, 30, 40), true);

        // Draw some text
        rend.draw_text("Icon Panel Demo - Press ESC to exit", 10, 10, sf::Color::White);
        rend.draw_text("Press C to toggle classic/modern (check log)", 10, 30, sf::Color::Yellow);

        // Render dialogs
        dlg_mgr.render(rend);

        rend.end_frame();

        // End frame for input (reset pressed/released states)
        inp.end_frame();
    }

    spdlog::info("Demo finished");
    dlg_mgr.shutdown();
    rend.shutdown();

    return 0;
}
