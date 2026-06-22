## Why

The CMake POST_BUILD custom command for the desktop executable fails to reliably copy the built binary and assets to the root `bin/` folder. The build output shows "Copying executable and assets to root bin/ folder" but the executable in the `bin/` folder often has an older timestamp than the one in the build directory, indicating the copy operation either fails silently or doesn't execute when the target is already present. This creates confusion during development and testing when users expect the `bin/` folder to contain the latest built executable.

## What Changes

- Fix the CMake custom command to always copy the latest executable when building
- Ensure the copy operation executes reliably on Windows platforms
- Add proper dependency tracking so CMake detects when the executable needs updating
- Add error reporting or logging when copy operations fail
- Maintain existing functionality: copy executable and assets to root `bin/` folder for easy access

## Capabilities

### New Capabilities
- `build-system-reliability`: Ensure build system operations execute reliably and consistently across rebuilds

### Modified Capabilities
-
 `build-system`: Update requirement for POST_BUILD custom commands to execute reliably and copy outputs correctly

## Impact

**Affected CMake files:**
- `apps/imgui-shell/platform/desktop/CMakeLists.txt` - Contains the problematic POST_BUILD custom command
- Potentially other CMakeLists.txt files with similar copy operations

**Build process:**
- Desktop executable build and deployment workflow
- Developer experience when testing latest builds from `bin/` folder
- Cross-platform consistency (Windows path handling vs Unix)

**Dependencies:**
- CMake generator expressions (`$<TARGET_FILE:...>`)
- Windows path conventions and command execution
- File timestamp comparison for change detection