## Why

The current icon implementation only works when running from the build directory with the `assets/` folder present. When packaging the application for distribution (e.g., installer, portable ZIP, app bundles), the icon file may not be at the expected relative path, causing the icon to disappear. Additionally, the current Windows-only implementation using `LoadImage()` with file paths doesn't work cross-platform and fails if the icon file isn't at the exact expected location.

This change ensures icons work reliably across all packaging and distribution scenarios, providing a professional appearance with consistent icon display in window title bars, taskbars, and file explorers regardless of how the application is packaged or where it's installed.

## What Changes

- **Replace file path-based icon loading** with embedded/compiled-in icon resources
- **Add cross-platform icon support** for Windows, macOS, and Linux distributions
- **Implement icon embedding** that works regardless of installation location
- **Update build system** to include icons as resources in the final binary/package
- **Add icon validation** to ensure icons appear correctly in all contexts (title bar, taskbar, file explorer)
- **Remove Windows-specific `LoadImage()`** calls in favor of GLFW's cross-platform `glfwSetWindowIcon()`
- **BREAKING**: Remove the current `assets/app_icon.ico` file loading via Windows API (will be replaced with embedded resource approach)

## Capabilities

### New Capabilities
- **application-icon-embedding**: Embed application icons directly into the executable/binary as resources rather than relying on external files
- **cross-platform-icon-support**: Consistent icon display across Windows, macOS, and Linux using appropriate platform-native approaches
- **packaged-icon-validation**: Verify icons work correctly in packaged/distributed application scenarios

### Modified Capabilities
- **desktop-window-management**: Current window creation and management will be updated to use embedded icons instead of file-based loading
- **application-packaging**: Build and packaging processes will include icon resource compilation/embedding

## Impact

**Affected code:**
- `apps/imgui-shell/platform/desktop/main.cpp`: Icon loading logic
- `apps/imgui-shell/platform/desktop/CMakeLists.txt`: Resource compilation/embedding
- `apps/imgui-shell/assets/`: Icon file formats and preparation
- Build system: Resource compilation steps for Windows (.rc), macOS (.icns), Linux (.png)

**APIs:**
- Windows: Replace `LoadImage()` with embedded resource loading
- GLFW: Use `glfwSetWindowIcon()` with embedded pixel data
- Cross-platform: Platform-specific resource embedding approaches

**Dependencies:**
- May need image conversion tools for creating platform-specific icon formats
- Build system updates for resource compilation
- Potential need for simple image loading library if not using GLFW's built-in capabilities

**Systems:**
- All packaging/distribution workflows (installers, app bundles, portable distributions)
- Development workflow for icon updates
- Cross-platform CI/CD pipelines for icon validation