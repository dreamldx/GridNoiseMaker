## Context

The imgui_playground application currently uses ImGui modal text inputs for file save/load operations in the File menu. While functional, these simple text inputs lack the native platform familiarity and features of OS file dialogs. The application already has a cross-platform architecture with platform-specific hosts (desktop: GLFW+Vulkan, iOS: Metal+UIKit) and a shared C++17 UI core.

## Goals / Non-Goals

**Goals:**
- Replace ImGui modal text inputs with native OS file dialogs for better user experience
- Maintain cross-platform compatibility across Windows, macOS, Linux, and iOS
- Keep existing JSON serialization/deserialization logic unchanged
- Preserve File menu integration and node graph persistence functionality
- Provide fallback to ImGui modals if native dialogs fail

**Non-Goals:**
- Not creating a fully-featured file manager or file browser
- Not supporting multiple file selection (single file only for node graph JSON)
- Not implementing custom file dialog UI themes
- Not changing the JSON file format or serialization logic
- Not modifying other aspects of the node graph persistence system

## Decisions

**1. Cross-platform abstraction layer**
- **Decision**: Create a `FileDialog` abstract base class with platform-specific implementations
- **Rationale**: Follows existing architecture pattern (RenderContext, Platform.h) for cross-platform concerns
- **Alternative considered**: Direct platform-specific calls in App.cpp with `#ifdef` - rejected as it violates "identical source across platforms" principle

**2. Platform-specific implementations**
- **Windows**: Use `IFileOpenDialog`/`IFileSaveDialog` (Common Item Dialog API) with COM initialization for modern Windows file dialogs with Unicode support
- **macOS**: Use `NSOpenPanel`/`NSSavePanel` via Objective-C++ bridge (`.mm` files)
- **Linux**: Use GTK file dialogs (`gtk_file_chooser_dialog_new`) via `dlopen` dynamic loading
3. **iOS**: Use `UIDocumentPickerViewController` for file selection
- **Rationale**: Each platform's native API provides best UX and system integration
- **Alternative considered**: Portable file dialog library (tinyfiledialogs, pfd) - rejected to avoid external dependencies and maintain control

**3. Fallback mechanism**
- **Decision**: If native dialog fails or unavailable, fall back to current ImGui modal
- **Rationale**: Ensures application remains functional even if platform-specific implementation has issues
- **Alternative considered**: Require native dialogs - rejected for robustness

**4. Linux dependency handling**
- **Decision**: Dynamically load GTK at runtime with `dlopen`/`dlsym`
-( **Rationale**: Avoids forcing GTK dependency on all Linux builds, allows graceful degradation
- **Alternative considered**: Static GTK dependency - rejected for flexibility and binary distribution

**5. API design**
- **Decision**: Synchronous blocking API (dialog opens, blocks UI until user action)
- **Rationale**: Matches current ImGui modal behavior, simpler integration
- **Alternative considered**: Async callbacks - rejected as overcomplication for current use case

**6. File filtering**
- **Decision**: Support JSON file filter (`*.json`) on all platforms
- **Rationale**: Consistent with node graph persistence file format
- **Alternative considered**: No filtering - rejected for better UX

## Risks / Trade-offs

**Risks:**
1. **Linux GTK dependency** → Mitigation: Dynamic loading with fallback to ImGui modal
2. **macOS Objective-C++ bridge complexity** → Mitigation: Follow existing iOS pattern in `.mm` files
3. **Windows COM initialization complexity** → Mitigation: Proper COM initialization with COINIT_APARTMENTTHREADED for Common Item Dialog API
4. **iOS sandbox restrictions** → Mitigation: Use document picker which respects sandbox

**Trade-offs:**
- **Complexity vs. UX**: Added platform-specific code for better user experience
-e **External dependency vs. self-contained**: Linux GTK dependency traded for native dialog support
- **Synchronous vs. async**: Simpler synchronous API at cost of UI blocking during dialog