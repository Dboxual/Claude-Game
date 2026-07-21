cmake_minimum_required(VERSION 3.21)
project(zion VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(zion
    src/main.cpp
    src/core/app.cpp
    src/core/log.cpp
    src/core/paths.cpp
    src/dev/dev_tools.cpp
    src/input/input.cpp
    src/settings/settings.cpp
    src/render/lighting.cpp
    src/render/sky.cpp
    src/render/renderer.cpp
    src/render/postfx.cpp
    src/world/terrain.cpp
    src/world/world.cpp
    src/world/interact.cpp
    src/world/day_night.cpp
    src/zone/zone_registry.cpp
    src/zone/zone_manager.cpp
    src/player/controller.cpp
    src/player/camera_rig.cpp
    src/fx/particles.cpp
    src/audio/audio.cpp
    src/save/save.cpp
    src/ui/ui.cpp
    src/ui/hud.cpp
    src/game/menus.cpp
    src/game/game.cpp
)

target_include_directories(zion PRIVATE src)
target_link_libraries(zion PRIVATE raylib nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(zion PRIVATE /W4 /permissive-)
    target_compile_definitions(zion PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    # Keep the console subsystem: smoke-test status lines must stay visible.
else()
    target_compile_options(zion PRIVATE -Wall -Wextra)
endif()

if(APPLE)
    # raylib (vcpkg) needs glfw and the Apple windowing/graphics frameworks
    # linked explicitly; without glfw the _glfw* symbols are undefined at link.
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(zion PRIVATE glfw
        "-framework IOKit" "-framework Cocoa" "-framework OpenGL" "-framework CoreVideo")
endif()
