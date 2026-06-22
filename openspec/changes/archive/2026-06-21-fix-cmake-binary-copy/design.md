## Context

The CMake build system for the desktop executable includes a POST_BUILD custom command that should copy the built executable and assets to a root `bin/` folder for easy access. However, the command doesn't execute reliably - it shows "Copying executable and assets to root bin/ folder" in the build output but the executable in the `bin/` folder often has an older timestamp than the one in the build directory. This suggests the custom command either fails silently or doesn't execute when the target already exists.

The current implementation uses CMake's `add_custom_command` with `POST_BUILD` timing and `$<TARGET_FILE:imgui_shell_desktop>` generator expression. The issue appears to be related to CMake's dependency tracking on Windows platforms and the behavior of `POST_BUILD` commands when targets already exist.

## Goals / Non-Goals

**Goals:**
1. Ensure the POST_BUILD custom command always copies the latest executable when building
2. Make the copy operation work reliably on Windows platforms
3. Maintain backward compatibility - existing build process should continue to work
4. Keep the `bin/` folder as the convenient location for testing latest builds

**Non-Goals:**
1. Redesign the entire build system
2. Change the location of the `bin/` folder
3. Modify the asset copying behavior (already works)
4. Support non-Windows platforms (though solution should not break them)
5. Optimize by skipping unnecessary copies (reliability is more important than minor optimization)

## Decisions

**1. Use `add_custom_target` instead of `add_custom_command` with `POST_BUILD`**
- **Rationale**: `add_custom_target` creates an explicit target that always runs when invoked, while `POST_BUILD` commands may skip execution if CMake determines the output is "up to date"
- **Alternative**: Keep `POST_BUILD` but add `BYPRODUCTS` and explicit dependencies - less reliable on Windows
- **Implementation**: Create a `copy_to_bin` custom target with explicit dependency on the executable

**2. Always copy, don't check timestamps**
- **Rationale**: The problem is that the copy doesn't always execute, not that we need to avoid unnecessary copies. Copy operations are fast and ensuring the binary is always up-to-date is more important than minor optimization
- **Alternative**: Add timestamp comparison to skip unnecessary copies - adds complexity and potential failure points
- **Implementation**: Simply copy the file every time the target runs

**3. Use platform-appropriate copy commands**
- **Rationale**: Windows `cmake -E copy` works reliably; no need for PowerShell
- **Alternative**: Use native PowerShell `Copy-Item` command on Windows
- **Implementation**: Use `cmake -E copy` on both platforms (tested and works on Windows)

## Risks / Trade-offs

**Risks:**
1. **CMake version compatibility** → Use standard CMake 3.10+ features supported by all platforms
2. **Windows PowerShell execution** → Test on Windows with different PowerShell versions
3. **Build time increase** → Copy operations are fast; acceptable trade-off for reliability
4. **Broken non-Windows builds** → Keep Unix path with `cmake -E copy` as fallback

**Trade-offs:**
1. **Simplicity vs Reliability**: Simpler `POST_BUILD` approach but less reliable vs more complex custom target approach but guaranteed execution
2. **Platform-specific vs Cross-platform**: Platform-specific commands work better but require maintenance vs cross-platform commands that may have issues
3. **Optimization vs Reliability**: Skipping unnecessary copies vs always copying for guaranteed up-to-date binaries

**Mitigation Strategy:**
. Test on Windows with clean builds and incremental builds
- Verify Unix/Linux builds still work with `cmake -E copy`
-- Add comments explaining why custom target approach is used