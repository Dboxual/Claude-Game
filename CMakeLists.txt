cmake_minimum_required(VERSION 3.21)
project(tacmove VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Three top-level pieces, enforced at build level:
#   src/shared/ -> tac_shared: simulation, config, logging. Compiled into BOTH
#                  the client and the dedicated server. std + glm only.
#   src/client/ -> the playable prototype (game loop/HUD/menu, platform
#                  layer, renderer backends). The only code that sees SDL/GL.
#   src/server/ -> tacmove_server: headless. Links tac_shared and nothing
#                  else, so it can never grow a graphics dependency silently.

# --- shared: movement sim, collision world, config parsing, logging ---
add_library(tac_shared STATIC
    src/shared/log.cpp
    src/shared/config.cpp
    src/shared/content.cpp
    src/shared/content_loader.cpp
    src/shared/melee.cpp
    src/shared/player.cpp
    src/shared/world.cpp
    src/shared/world_save.cpp
)
target_include_directories(tac_shared PUBLIC src)
target_link_libraries(tac_shared PUBLIC glm::glm)

# --- client game layer: sim orchestration, HUD/menu model, user settings.
#     No SDL, no graphics API (enforced: no such include paths here). ---
add_library(tac_client_game STATIC
    src/client/game.cpp
    src/client/settings.cpp
)
target_link_libraries(tac_client_game PUBLIC tac_shared)

# --- client renderer: IRenderer + backends. The GL backend resolves its
#     function pointers through the window's proc loader, so no SDL here. ---
add_library(tac_client_renderer STATIC
    src/client/renderer/renderer_factory.cpp
    src/client/renderer/common/font5x7.cpp
    src/client/renderer/opengl/gl_loader.cpp
    src/client/renderer/opengl/gl_renderer.cpp
    src/client/renderer/vulkan/vulkan_renderer.cpp
    src/client/renderer/metal/metal_renderer.cpp
    src/client/renderer/mobile/gles_renderer.cpp
)
target_link_libraries(tac_client_renderer PUBLIC tac_shared)

# --- client platform layer: the only code in the project that touches SDL3 ---
add_library(tac_client_platform_sdl STATIC
    src/client/platform/sdl/sdl_platform.cpp
    src/client/platform/sdl/sdl_window.cpp
    src/client/platform/sdl/sdl_input.cpp
    src/client/platform/sdl/sdl_filesystem.cpp
    src/client/platform/sdl/sdl_audio.cpp
)
target_link_libraries(tac_client_platform_sdl PUBLIC tac_shared SDL3::SDL3)

# --- game client ---
add_executable(tacmove src/client/main.cpp)
target_link_libraries(tacmove PRIVATE tac_client_game tac_client_renderer tac_client_platform_sdl)

# --- dedicated server: headless, command-line, tac_shared only ---
add_executable(tacmove_server
    src/server/main.cpp
    src/server/dedicated_server.cpp
)
target_link_libraries(tacmove_server PRIVATE tac_shared)

foreach(target tac_shared tac_client_game tac_client_renderer tac_client_platform_sdl tacmove tacmove_server)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        # Portable C stdio (sscanf in the config parsers) is fine; the
        # Microsoft-only _s variants are not a portability win here.
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()

# Ship the config directory (movement tunables) and the server content
# platform (server.cfg + content: weapon/entity defs, sounds) next to the
# binaries so both find them no matter where they are launched from.
add_custom_command(TARGET tacmove POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:tacmove>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/server $<TARGET_FILE_DIR:tacmove>/server)
