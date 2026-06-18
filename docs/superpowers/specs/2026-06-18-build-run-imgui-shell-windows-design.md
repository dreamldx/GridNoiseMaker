# Design: Build and run imgui-shell on Windows
Date: 2026-06,18
Author: opencode

## Overview
Build and run the existing imgui-shell Dear ImGui application on Windows platform using the documented CMake preset system.

## Context
imgui-shell is a cross-platform Dear ImGui application skeleton targeting macOS, Windows, Linux, and iOS from a single C++17 codebase. It uses GLFW + Vulkan 1.2 on desktop platforms (MoltenVK on macOS) and Metal + UIKit on iOS.

## Architecture
The project uses a CMake preset system with per-platform configurations:
.
- **Windows preset**: Visual Studio 2022 generator, x64 architecture
- **Source structure**: Shared `app/` core with platform-specific `platform/desktop/` host
- **Dependencies**: ImGui (docking branch) + GLFW via FetchContent, Vulkan SDK
- **Output**: `imgui_shell_desktop.exe` in `build/windows/platform/desktop/`

## Components
1. **Build configuration**: Windows preset in `apps/imgui-shell/CMakePresets.json`
2. **CMakeLists.txt**: Root CMake file with project setup and dependency management
3. **app/**: Shared application core (lifecycle interface `init` / `frame` / `shutdown`)
4. **platform/desktop/**: Windows-specific Vulkan + GLFW host implementation
5. **cmake/Dependencies.cmake**: FetchContent configuration for ImGui and GLFW

## Prerequisites
- CMake ≥ 3.24
- Visual Studio 2022 with C++ tools
- LunarG Vulkan SDK ≥ 1.2 installed (`VULKAN_SDK` environment variable set)
- Windows 10+ operating system

## Build Process
1. **Configure**: `cmake --preset windows` from `apps/imgui-shell/` directory
2. **Build**: `cmake --build --preset windows`
3. **Run**: Execute `./build/windows/platform/desktop/imgui_shell_desktop.exe`

## Data Flow
1. CMake configures project using Windows preset (Visual Studio 2022 generator)
2. FetchContent pulls ImGui (docking branch) and GLFW dependencies
3. Visual Studio/MSBuild compiles C++17 source code
4. Executable links against Vulkan and GLFW libraries
5. Application initializes Vulkan renderer and GLFW windowing system
6. Shared app core renders Dear ImGui UI via platform-specific host

## Error Handling
- CMake errors if prerequisites missing (Vulkan SDK not found)
- Build failures reported by MSBuild with detailed compiler errors
- Runtime Vulkan validation layers catch graphics API errors
- Application logs to stdout/stderr for debugging
- Clean shutdown on window close or application termination

## Success Criteria
1. Application builds successfully without compilation errors
2. Executable runs and displays Dear ImGui UI with demo window
3. No Vulkan or GLFW runtime errors or crashes
4. Application closes cleanly when window is closed
5. Menu bar with "File" and "Help" items functions correctly

## Verification
Manual verification:
- Visual confirmation of rendered UI
- Check console output for error messages
- Validate window resize and interaction
- Test menu functionality (About dialog)

## Implementation Notes
- Uses existing build system (no modifications required)
- Follows documented process in `apps/imgui-shell/README.md`
- Windows preset configured for Visual Studio 2022 x64
- Debug build configuration by default
- All dependencies managed via CMake FetchContent or find_package