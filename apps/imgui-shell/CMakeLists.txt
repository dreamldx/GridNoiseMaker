cmake_minimum_required(VERSION 3.24)

project(imgui_shell
    VERSION 0.1.0
    DESCRIPTION "Cross-platform Dear ImGui shell (macOS / Windows / Linux / iOS)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Font rendering (see openspec change `add-font-antialiasing`).
option(IMGUI_SHELL_USE_FREETYPE
    "Use FreeType instead of stb_truetype for font rasterization"
    ON)
# Note: default font size used to live here as IMGUI_SHELL_DEFAULT_FONT_PX,
# but it's now part of the theme — see app/Theme.h kThemeFontSizePx.

# Derive IMGUI_SHELL_PLATFORM from the active toolchain / system.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IMGUI_SHELL_PLATFORM "ios" CACHE STRING "Target platform" FORCE)
    enable_language(OBJCXX)
elseif(APPLE)
    set(IMGUI_SHELL_PLATFORM "macos" CACHE STRING "Target platform" FORCE)
elseif(WIN32)
    set(IMGUI_SHELL_PLATFORM "windows" CACHE STRING "Target platform" FORCE)
elseif(UNIX)
    set(IMGUI_SHELL_PLATFORM "linux" CACHE STRING "Target platform" FORCE)
else()
    message(FATAL_ERROR "Unsupported platform; expected macOS, Windows, Linux, or iOS.")
endif()

set(IMGUI_SHELL_DESKTOP OFF)
set(IMGUI_SHELL_MOBILE OFF)
if(IMGUI_SHELL_PLATFORM MATCHES "^(macos|windows|linux)$")
    set(IMGUI_SHELL_DESKTOP ON)
elseif(IMGUI_SHELL_PLATFORM STREQUAL "ios")
    set(IMGUI_SHELL_MOBILE ON)
endif()

message(STATUS "imgui-shell: building for ${IMGUI_SHELL_PLATFORM} (desktop=${IMGUI_SHELL_DESKTOP}, mobile=${IMGUI_SHELL_MOBILE})")

# Common warning flags for our own targets (not ImGui).
if(MSVC)
    set(IMGUI_SHELL_WARNINGS /W4 /permissive-)
else()
    set(IMGUI_SHELL_WARNINGS -Wall -Wextra -Wpedantic)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)

add_subdirectory(app)

if(IMGUI_SHELL_DESKTOP)
    add_subdirectory(platform/desktop)
elseif(IMGUI_SHELL_MOBILE)
    add_subdirectory(platform/ios)
endif()
