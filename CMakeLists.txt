cmake_minimum_required(VERSION 3.20)
project(GravityEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Find Raylib (local install first, fallback to FetchContent)
find_package(raylib QUIET)

if (NOT raylib_FOUND)
    message(STATUS "raylib not found locally, fetching via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
else()
    message(STATUS "Found local raylib: ${raylib_DIR}")
endif()

# Project Sources and Headers
set(SOURCES
    src/main.cpp
    src/GravityEngine.cpp
    src/Renderer.cpp
)

set(HEADERS
    include/Vec2.hpp
    include/Body.hpp
    include/GravityEngine.hpp
    include/Renderer.hpp
)

add_executable(GravityEngine ${SOURCES} ${HEADERS})

target_include_directories(GravityEngine PRIVATE include)

target_link_libraries(GravityEngine PRIVATE raylib)

if (WIN32 AND MINGW)
    target_link_libraries(GravityEngine PRIVATE opengl32 gdi32 winmm)
endif()
