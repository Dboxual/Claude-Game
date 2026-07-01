cmake_minimum_required(VERSION 3.21)
project(tacmove VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(tacmove
    src/main.cpp
    src/gl_loader.cpp
    src/config.cpp
    src/world.cpp
    src/player.cpp
    src/renderer.cpp
    src/debug_text.cpp
)

target_link_libraries(tacmove PRIVATE SDL3::SDL3 glm::glm)

if(MSVC)
    target_compile_options(tacmove PRIVATE /W4)
else()
    target_compile_options(tacmove PRIVATE -Wall -Wextra)
endif()

# Ship the tunables next to the executable so the game finds them no matter
# where it is launched from (it also checks the working directory first).
add_custom_command(TARGET tacmove POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:tacmove>/config)
