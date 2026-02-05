add_executable(helbreath_client
    main.cpp
    application.cpp

    # Core
    core/constants.hpp
    core/game_enums.hpp
    core/config.cpp

    # Graphics
    graphics/renderer.cpp
    graphics/text_renderer.cpp
    graphics/text_shader_cache.cpp
    graphics/menu_character_renderer.cpp
    graphics/color_utils.hpp

    # Input
    input/input.cpp

    # Audio
    audio/audio.cpp
    audio/sound_manager.cpp

    # Assets
    assets/pak_file.cpp
    assets/sprite.cpp
    assets/sprite_manager.cpp
    assets/tile_sprite_registry.cpp

    # Network
    network/packet.cpp
    network/connection.cpp
    network/websocket_connection.cpp
    network/network_system.cpp
    network/handlers/notify_handlers.cpp
    network/handlers/motion_handlers.cpp

    # World
    world/map.cpp
    world/map_renderer.cpp
    world/world.cpp

    # Entity
    entity/entity.cpp
    entity/entity_manager.cpp

    # UI
    ui/ui_element.cpp
    ui/ui_system.cpp
    ui/dialog_base.cpp
    ui/mouse_interface.cpp
    ui/dialog_builder.cpp
    ui/render_strategy.cpp
    ui/managed_dialog.cpp
    ui/dialog_manager.cpp
    ui/status_log.cpp

    # UI Screens (sprite-based)
    ui/screens/screen_base.cpp
    ui/screens/screen_manager.cpp
    ui/screens/main_menu_screen.cpp
    ui/screens/login_screen.cpp
    ui/screens/character_select_screen.cpp
    ui/screens/character_create_screen.cpp
    ui/screens/connection_lost_screen.cpp

    # UI Dialogs
    ui/dialogs/connection_dialog.cpp
    ui/dialogs/character_dialog.cpp
    ui/dialogs/inventory_dialog.cpp
    ui/dialogs/equipment_dialog.cpp
    ui/dialogs/spellbook_dialog.cpp
    ui/dialogs/skills_dialog.cpp
    ui/dialogs/shop_dialog.cpp
    ui/dialogs/bank_dialog.cpp
    ui/dialogs/chat_dialog.cpp
    ui/dialogs/party_dialog.cpp
    ui/dialogs/npc_dialog.cpp
    ui/dialogs/trade_dialog.cpp
    ui/dialogs/craft_dialog.cpp
    ui/dialogs/map_dialog.cpp
    ui/dialogs/repair_dialog.cpp
    ui/dialogs/help_dialog.cpp
    ui/dialogs/yaml_help_dialog.cpp
    ui/dialogs/login_dialog.cpp
    ui/dialogs/character_select_dialog.cpp
    ui/dialogs/character_create_dialog.cpp
    ui/dialogs/icon_panel_dialog.cpp
    ui/dialogs/yaml_icon_panel_dialog.cpp
    ui/dialogs/gauge_panel_dialog.cpp
    ui/dialogs/levelup_dialog.cpp
    ui/dialogs/system_menu_dialog.cpp

    # Gameplay
    gameplay/combat.cpp
    gameplay/floating_text.cpp
    gameplay/inventory.cpp
    gameplay/magic.cpp
    gameplay/skills.cpp
    gameplay/game_state.cpp

    # Chat
    chat/chat_system.cpp

    # Localization
    localization/localization.cpp

    # Debug
    debug/position_store.cpp
    debug/debug_overlay.cpp
    debug/debug_stats.cpp
)

target_include_directories(helbreath_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenGL REQUIRED)

target_link_libraries(helbreath_client PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
    OpenGL::GL
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    lz4::lz4
    unofficial-sodium::sodium
    ixwebsocket::ixwebsocket
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(helbreath_client PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    # Link Windows system libraries needed by ixwebsocket/mbedtls
    target_link_libraries(helbreath_client PRIVATE bcrypt)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(helbreath_client PRIVATE /W4 /MP)
else()
    target_compile_options(helbreath_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Hot-reload feature flag
if(HB_HOT_RELOAD)
    target_compile_definitions(helbreath_client PRIVATE HB_HOT_RELOAD_ENABLED)
endif()

# Debug overlay feature flag
# AUTO (default): enabled in Debug, disabled in Release
# ON: always enabled
# OFF: always disabled
if(HB_DEBUG_OVERLAY STREQUAL "ON")
    # Always enabled
    target_compile_definitions(helbreath_client PRIVATE HB_DEBUG_OVERLAY_ENABLED)
    message(STATUS "Debug overlay: ENABLED (always)")
elseif(HB_DEBUG_OVERLAY STREQUAL "OFF")
    # Always disabled
    message(STATUS "Debug overlay: DISABLED")
else()
    # AUTO mode - enabled in Debug builds only (using generator expression for multi-config)
    target_compile_definitions(helbreath_client PRIVATE
        $<$<CONFIG:Debug>:HB_DEBUG_OVERLAY_ENABLED>
    )
    message(STATUS "Debug overlay: AUTO (Debug builds only)")
endif()

# Copy DLLs to bin/ directory after build
if(WIN32)
    add_custom_command(TARGET helbreath_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:helbreath_client>
            $<TARGET_FILE_DIR:helbreath_client>
        COMMAND_EXPAND_LISTS
    )

    # Set Visual Studio debugger working directory to bin/
    set_target_properties(helbreath_client PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )
endif()

