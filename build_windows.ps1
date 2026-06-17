# PowerShell build script for imgui-shell on Windows
# Run this script from the imgui_playground directory

Write-Host "Building imgui-shell for Windows..." -ForegroundColor Green

# Check if we're in the right directory
if (-not (Test-Path "apps\imgui-shell\CMakePresets.json")) {
    Write-Host "Error: Run this script from the imgui_playground directory" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Check Vulkan SDK
if (-not $env:VULKAN_SDK) {
    Write-Host "Warning: VULKAN_SDK environment variable not set" -ForegroundColor Yellow
    Write-Host "Try setting it to your Vulkan SDK path, e.g.:" -ForegroundColor Yellow
    Write-Host "`$env:VULKAN_SDK = 'D:\VulkanSDK\1.4.350.0'" -ForegroundColor Yellow
    Write-Host ""
}

# Configure with CMake presets
Write-Host "Configuring Windows build..." -ForegroundColor Cyan
Set-Location apps\imgui-shell
cmake --preset windows

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Build the project
Write-Host "Building Windows executable..." -ForegroundColor Cyan
cmake --build --preset windows

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable: apps\imgui-shell\build\windows\platform\desktop\Debug\imgui_shell_desktop.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: Debug builds show console window, Release builds hide it" -ForegroundColor Cyan
Write-Host "To build Release: cmake --preset windows -D CMAKE_BUILD_TYPE=Release && cmake --build --preset windows --config Release" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run:" -ForegroundColor Yellow
Write-Host "1. Ensure Vulkan runtime is installed" -ForegroundColor Yellow
Write-Host "2. Run: apps\imgui-shell\build\windows\platform\desktop\Debug\imgui_shell_desktop.exe" -ForegroundColor Yellow
Write-Host ""
Read-Host "Press Enter to exit"