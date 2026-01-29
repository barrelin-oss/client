cmake_minimum_required(VERSION 3.20)
project(helbreath_client VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies via vcpkg
find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)

add_subdirectory(src)
