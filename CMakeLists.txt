cmake_minimum_required(VERSION 3.20)
project(helbreath_client VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(HB_HOT_RELOAD "Enable hot-reload for dialog YAML/JSON files" ON)

# Debug overlay option: AUTO (default), ON, or OFF
# AUTO = enabled in Debug builds only
set(HB_DEBUG_OVERLAY "AUTO" CACHE STRING "Enable debug positioning overlay (AUTO=Debug only, ON=always, OFF=never)")
set_property(CACHE HB_DEBUG_OVERLAY PROPERTY STRINGS "AUTO" "ON" "OFF")

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories - all binaries go to bin/ (for all configurations)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin)

# Dependencies via vcpkg
find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_subdirectory(src)

# Set helbreath_client as the default startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT helbreath_client)
