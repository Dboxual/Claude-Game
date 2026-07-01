cmake_minimum_required(VERSION 3.21)
project(tacmove VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# The engine is split into one static library per layer. This is not just
# organization: it makes the platform boundaries real. tac_game and
# tac_renderer never receive SDL3's include paths, so gameplay or renderer
# code that tries to #include <SDL3/...> fails to compile. Porting to a new
# platform means adding a sibling of tac_platform_sdl, not editing the game.

# --- core: logging + config parsing (std library only) ---
add_library(tac_core STATIC
    src/engine/log.cpp
    src/engine/config.cpp
)
target_include_directories(tac_core PUBLIC src)

# --- gameplay: movement sim, world, HUD model (glm only, no SDL, no GL) ---
add_library(tac_game STATIC
    src/game/game.cpp
    src/game/player.cpp
    src/game/world.cpp
)
target_link_libraries(tac_game PUBLIC tac_core glm::glm)

# --- renderer: IRenderer + backends. The GL backend resolves its function
#     pointers through the window's proc loader, so no SDL here either. ---
add_library(tac_renderer STATIC
    src/renderer/renderer_factory.cpp
    src/renderer/common/font5x7.cpp
    src/renderer/opengl/gl_loader.cpp
    src/renderer/opengl/gl_renderer.cpp
    src/renderer/vulkan/vulkan_renderer.cpp
    src/renderer/metal/metal_renderer.cpp
    src/renderer/mobile/gles_renderer.cpp
)
target_link_libraries(tac_renderer PUBLIC tac_core glm::glm)

# --- platform: the only code in the project allowed to touch SDL3 ---
add_library(tac_platform_sdl STATIC
    src/platform/sdl/sdl_platform.cpp
    src/platform/sdl/sdl_window.cpp
    src/platform/sdl/sdl_input.cpp
    src/platform/sdl/sdl_filesystem.cpp
    src/platform/sdl/sdl_audio.cpp
)
target_link_libraries(tac_platform_sdl PUBLIC tac_core SDL3::SDL3)

# --- executable: the composition root ---
add_executable(tacmove src/main.cpp)
target_link_libraries(tacmove PRIVATE tac_game tac_renderer tac_platform_sdl)

foreach(target tac_core tac_game tac_renderer tac_platform_sdl tacmove)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()

# Ship the tunables next to the executable so the game finds them no matter
# where it is launched from (it also checks the working directory first).
add_custom_command(TARGET tacmove POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:tacmove>/config)
