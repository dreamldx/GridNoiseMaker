## Why

When launching the Windows desktop executable (`imgui_shell_desktop.exe`), a console window appears alongside the GUI window. This creates a poor user experience for a graphical application and makes the application look unprofessional. The console window is unnecessary for end-users who only need the GUI interface.

## What Changes

-BREAKING: Change the Windows desktop executable subsystem from console to Windows (hide console window)
- Add CMake configuration to build Windows executables with `/SUBSYSTEM:WINDOWS`
- Ensure the application still receives stdout/stderr for debugging when launched from command line
- Preserve the ability to see debug output when needed (optional debug mode)

## Capabilities

### New Capabilities
-smart-window-subsystem: Windows executable subsystem configuration that hides console window while preserving debug output capability
- `desktop-launch-behavior`: Control how the desktop application launches and handles stdout/stderr on Windows

### Modified Capabilities
- `build-system`: Modify Windows build preset to configure executable subsystem
2. `app-shell`: Potentially modify application initialization to handle subsystem differences

## Impact

- Affects Windows desktop build configuration in `CMakePresets.json` and CMakeLists.txt
- Changes the Visual Studio project properties for Windows executables
- May require modifications to how the application handles standard output/error streams
- Only affects Windows platform; macOS and Linux remain unchanged
