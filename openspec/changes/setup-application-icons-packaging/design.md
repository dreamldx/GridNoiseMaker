## Context

The ImGui Shell application currently has a basic icon implementation that works only in specific development scenarios:
1. Windows-specific `LoadImage()` calls with hardcoded relative path `"assets/app_icon.ico"`
2. Icon file must be present in the `assets/` folder relative to the executable
3. No cross-platform support - macOS and Linux would not show icons
4. When packaged for distribution (installer, app bundle, portable ZIP), the relative path breaks

The application uses GLFW for window management and Vulkan for rendering. GLFW provides `glfwSetWindowIcon()` which accepts raw pixel data, offering a cross-platform path forward. However, we need to embed icon data into the executable rather than relying on external files.

## Goals / Non-Goals

**Goals:**
1. **Embedded icons**: Icons compiled directly into the executable as resources/data
2. **Cross-platform**: Consistent icon display on Windows, macOS, and Linux
3. **Packaging resilient**: Icons work regardless of installation location or packaging method
4. **Minimal dependencies**: Avoid adding complex image libraries; use simple approaches
5. **Build-time integration**: Icon embedding handled at compile time, not runtime file loading

**Non-Goals:**
1. **Dynamic icon loading**: Icons are fixed at build time, not swappable at runtime
2. **Multiple icon sizes**: Support only standard icon sizes needed for title bars/taskbars (16x16, 32x32, 64x64)
3. **Icon editing tools**: Not building icon editor GUI; using existing `.ico`/`.icns`/`.png` files
4. **Legacy Windows API support**: Dropping Windows-specific `LoadImage()` in favor of GLFW approach

## Decisions

### 1. Platform-Specific Resource Embedding
**Decision**: Use different approaches per platform rather than a single universal solution
- **Windows**: Compile `.ico` file into Windows resources using `.rc` file
- **macOS**: Bundle `.icns` file in app bundle resources directory
- **Linux**: Embed `.png` pixels directly in code or use XDG desktop entry

**Rationale**: Each platform has native expectations for icons:
- Windows expects icons in PE resource section for proper explorer/taskbar display
- macOS expects `.icns` in `Contents/Resources/` of app bundle
- Linux is flexible but `.png` embedded in code works everywhere

**Alternative considered**: Single `.png` embedded in all platforms
- **Rejected**: Windows needs `.ico` for proper multi-size support; macOS needs `.icns` for native integration

### 2. GLFW for Runtime Icon Setting
**Decision**: Use `glfwSetWindowIcon()` exclusively for setting window icons
**Rationale**: 
- GLFW already provides cross-platform window management
- `glfwSetWindowIcon()` accepts raw RGBA pixel data, works on all platforms
- Consistent code path instead of platform-specific `WM_SETICON`, `NSApplication`, etc.

**Alternative considered**: Platform-native APIs (`WM_SETICON`, `NSApplication`)
- **Rejected**: Would require platform-specific code; GLFW abstracts this already

### 3. Build-Time Icon Conversion
**Decision**: Convert source icon to platform formats at build time using simple tools
**Rationale**:
- `.ico` → Windows resources via `windres` (already in toolchain)
- `.png` → C array for Linux via custom Python script or `xxd`
- `.png` → `.icns` for macOS via `iconutil` or simple script

**Alternative considered**: Bundle raw image files and load at runtime
- **Rejected**: Breaks packaging resilience goal; adds runtime file dependencies

### 4. Simple Pixel Embedding for Linux
**Decision**: Embed `.png` pixel data as C array compiled into binary
**Rationale**:
- No external file dependencies
- Works with GLFW's `glfwSetWindowIcon()`
- Simple implementation: convert `.png` to `.c` file with `uint8_t icon_data[]`

**Alternative considered**: XDG desktop entry with icon path
- **Rejected**: Requires installation; doesn't work for portable applications

## Risks / Trade-offs

**[Risk] Increased build complexity** → Add icon conversion steps to CMake
- **Mitigation**: Keep scripts simple; document thoroughly

**[Risk] Icon size/format issues** → Different platforms expect different formats/sizes
- **Mitigation**: Standardize on 16x16, 32x32, 64x64 sizes; test on all platforms

**[Risk] GLFW icon limitations** → `glfwSetWindowIcon()` may not set taskbar/file explorer icons
- **Mitigation**: For Windows, also set via resource compilation for explorer integration

**[Trade-off] Build time vs runtime flexibility** → Icons fixed at build time
- **Acceptance**: Acceptable for this application; icons rarely change

**[Trade-off] Code complexity vs dependencies** → Custom build scripts vs image library
- **Acceptance**: Prefer simple scripts to avoid new dependencies

## Migration Plan

1. **Phase 1**: Implement Windows resource compilation
   - Create `.rc` file referencing `app_icon.ico`
   - Update CMake to compile resources
   - Test icon appears in explorer and taskbar

2. **Phase 2**: Implement GLFW icon setting for all platforms
   - Extract icon pixels from embedded resources/data
   - Use `glfwSetWindowIcon()` consistently
   - Remove Windows `LoadImage()` calls

3. **Phase 3**: Add macOS `.icns` support
   - Create `.icns` file generation in build
   - Configure app bundle resources

4. **Phase 4**: Add Linux embedded `.png` support
   - Create build-time `.png` to C array conversion
   - Test with GLFW

5. **Phase 5**: Validation and cleanup
   - Test all packaging scenarios
   - Remove old `assets/app_icon.ico` loading code
   - Update documentation

**Rollback**: Revert to current `LoadImage()` approach if issues arise

## Open Questions

1. **Best approach for `.png` to C array conversion**? Use Python with PIL/Pillow, or simple script?
2. **Should we support multiple icon sizes in embedded data**? GLFW can accept multiple images.
3. **How to handle high-DPI/retina icons**? Need 2x versions for macOS.
4. **Testing strategy for cross-platform icon display**? Manual testing or automated screenshots?