@echo off
REM Windows build script for imgui-shell
REM Run this script from the imgui_playground directory

echo Building imgui-shell for Windows...

REM Check if we're in the right directory
if not exist "apps\imgui-shell\CMakePresets.json" (
    echo Error: Run this script from the imgui_playground directory
    pause
    exit /b 1
)

REM Set Vulkan SDK path if not already set
if "%VULKAN_SDK%"=="" (
    echo Warning: VULKAN_SDK environment variable not set
    echo Try setting it to your Vulkan SDK path, e.g.:
    echo set VULKAN_SDK=D:\VulkanSDK\1.4.350.0
)

REM Configure with CMake presets
echo Configuring Windows build...
cd apps\imgui-shell
cmake --preset windows

if errorlevel 1 (
    echo CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo Building Windows executable...
cmake --build --preset windows

if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

echo Build successful!
echo Executable: apps\imgui-shell\build\windows\platform\desktop\Debug\imgui_shell_desktop.exe
echo.
echo Note: Debug builds show console window, Release builds hide it
echo To build Release: cmake --preset windows -D CMAKE_BUILD_TYPE=Release && cmake --build --preset windows --config Release
echo.
echo To run:
echo 1. Ensure Vulkan runtime is installed
echo 2. Run: apps\imgui-shell\build\windows\platform\desktop\Debug\imgui_shell_desktop.exe
echo.
pause