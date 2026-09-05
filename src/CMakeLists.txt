# Shared source library (used by client and tools)
add_library(helbreath_core STATIC
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
    graphics/paperdoll_renderer.cpp
    graphics/screen_transition.cpp
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
    world/ground_item.cpp
    world/map.cpp
    world/map_renderer.cpp
    world/weather_system.cpp
    world/world.cpp

    # Entity
    entity/entity.cpp
    entity/entity_manager.cpp
    entity/npc_animation_data.cpp

    # UI
    ui/ui_element.cpp
    ui/ui_system.cpp
    ui/dialog_base.cpp
    ui/mouse_interface.cpp
    ui/dialog_builder.cpp
    ui/render_strategy.cpp
    ui/managed_dialog.cpp
    ui/dialog_manager.cpp
    ui/cursor.cpp
    ui/status_log.cpp

    # UI Screens (sprite-based)
    ui/screens/screen_base.cpp
    ui/screens/screen_manager.cpp
    ui/screens/main_menu_screen.cpp
    ui/screens/login_screen.cpp
    ui/screens/character_select_screen.cpp
    ui/screens/character_create_screen.cpp
    ui/screens/connection_lost_screen.cpp
    ui/screens/reconnect_screen.cpp

    # UI Dialogs
    ui/dialogs/connection_dialog.cpp
    ui/dialogs/death_dialog.cpp
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
    ui/dialogs/quest_dialog.cpp
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
    ui/dialogs/fishing_dialog.cpp
    ui/dialogs/level_up_settings_dialog.cpp

    # Gameplay
    gameplay/combat.cpp
    gameplay/dialog_callbacks.cpp
    gameplay/effect_definition.cpp
    gameplay/effect_system.cpp
    gameplay/floating_text.cpp
    gameplay/input_handler.cpp
    gameplay/inventory.cpp
    gameplay/item.cpp
    gameplay/item_format.cpp
    gameplay/magic.cpp
    gameplay/guild_system.cpp
    gameplay/skills.cpp
    gameplay/ws_message_handler.cpp
    gameplay/ws_auth_handlers.cpp
    gameplay/ws_entity_handlers.cpp
    gameplay/ws_combat_handlers.cpp
    gameplay/ws_inventory_handlers.cpp
    gameplay/ws_guild_handlers.cpp
    gameplay/ws_quest_handlers.cpp
    gameplay/ws_party_handlers.cpp
    gameplay/ws_shop_handlers.cpp
    gameplay/ws_misc_handlers.cpp
    gameplay/game_state.cpp
    gameplay/chat_input_overlay.cpp

    # Platform
    platform/monitor.cpp

    # Chat
    chat/chat_system.cpp

    # Localization
    localization/localization.cpp

    # Debug
    debug/position_store.cpp
    debug/debug_overlay.cpp
    debug/debug_stats.cpp
    debug/mcp_server/debug_server.cpp
)

target_include_directories(helbreath_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenGL REQUIRED)

target_link_libraries(helbreath_core PUBLIC
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

# Main client executable
add_executable(helbreath_client main.cpp)
target_link_libraries(helbreath_client PRIVATE helbreath_core)

# Effect test tool
add_executable(effect_test_tool
    tools/effect_test_main.cpp
    tools/effect_test_app.cpp
)
target_link_libraries(effect_test_tool PRIVATE helbreath_core)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(helbreath_core PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    target_link_libraries(helbreath_core PUBLIC bcrypt)
endif()

# Linux-specific settings
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(helbreath_core PUBLIC X11::X11 X11::Xrandr)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(helbreath_core PRIVATE /W4 /MP)
else()
    target_compile_options(helbreath_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Hot-reload feature flag
if(HB_HOT_RELOAD)
    target_compile_definitions(helbreath_core PUBLIC HB_HOT_RELOAD_ENABLED)
endif()

# Debug overlay feature flag
# AUTO (default): enabled in Debug, disabled in Release
# ON: always enabled
# OFF: always disabled
if(HB_DEBUG_OVERLAY STREQUAL "ON")
    target_compile_definitions(helbreath_core PUBLIC HB_DEBUG_OVERLAY_ENABLED)
    message(STATUS "Debug overlay: ENABLED (always)")
elseif(HB_DEBUG_OVERLAY STREQUAL "OFF")
    message(STATUS "Debug overlay: DISABLED")
else()
    target_compile_definitions(helbreath_core PUBLIC
        $<$<CONFIG:Debug>:HB_DEBUG_OVERLAY_ENABLED>
    )
    message(STATUS "Debug overlay: AUTO (Debug builds only)")
endif()

# hb-patch - asset packager for CI/CD deployment
add_library(hb_patch_lib STATIC
    hb-patch/hasher.cpp
    hb-patch/compressor.cpp
    hb-patch/keygen.cpp
    hb-patch/manifest.cpp
    hb-patch/builder.cpp
)

target_include_directories(hb_patch_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/hb-patch)

target_link_libraries(hb_patch_lib PUBLIC
    unofficial-sodium::sodium
    nlohmann_json::nlohmann_json
    $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)

target_compile_features(hb_patch_lib PUBLIC cxx_std_23)

add_executable(hb-patch hb-patch/main.cpp)
target_link_libraries(hb-patch PRIVATE hb_patch_lib)

# Patcher executable
add_executable(helbreath_patcher
    patcher/main.cpp
    patcher/patcher_config.cpp
    patcher/manifest.cpp
    patcher/downloader.cpp
    patcher/file_ops.cpp
)

if(WIN32)
    target_sources(helbreath_patcher PRIVATE patcher/ui_win32.cpp)
    target_compile_definitions(helbreath_patcher PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_sources(helbreath_patcher PRIVATE patcher/ui_gtk.cpp)
    target_include_directories(helbreath_patcher PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(helbreath_patcher PRIVATE ${GTK3_LIBRARIES})
endif()

target_include_directories(helbreath_patcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(helbreath_patcher PRIVATE
    CURL::libcurl
    $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
    unofficial-sodium::sodium
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
set_target_properties(helbreath_patcher PROPERTIES OUTPUT_NAME "helbreath")
target_compile_features(helbreath_patcher PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(helbreath_patcher PRIVATE /W4 /MP)
else()
    target_compile_options(helbreath_patcher PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy DLLs and set debugger working directory
if(WIN32)
    foreach(target helbreath_client effect_test_tool helbreath_patcher hb-patch)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}>
                $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
        )
        set_target_properties(${target} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
        )
    endforeach()

    # Stage launcher exe + only its DLLs into bin/launcher/
    set(LAUNCHER_STAGING_DIR "${CMAKE_SOURCE_DIR}/bin/launcher")
    add_custom_command(TARGET helbreath_patcher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LAUNCHER_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:helbreath_patcher>
            "${LAUNCHER_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:helbreath_patcher>
            "${LAUNCHER_STAGING_DIR}"
        COMMAND_EXPAND_LISTS
    )
endif()

