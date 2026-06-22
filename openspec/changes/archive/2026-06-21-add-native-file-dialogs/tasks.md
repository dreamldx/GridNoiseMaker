## 1. Cross-platform File Dialog Abstraction Layer

- [x] 1.1 Create `FileDialog` abstract base class in shared code (`apps/imgui-shell/app/`)
.
- [x] 1.2 Define interface with `openFileDialog()` and `saveFileDialog()` pure virtual methods

 - [x] 1.3 Add platform detection/selection mechanism similar to existing `RenderContext` pattern
- [x] 1.4 Create factory function to return platform-specific `FileDialog` implementation
- [x] 1.5 Add JSON file filter support (`*.json`) in abstract interface
- [x] 1.6 Implement fallback mechanism that returns to ImGui modal text input

## 2. Platform-Specific Implementations

- [x] 2.1 Windows: Create `WindowsFileDialog` class in `platform/desktop/`
- [x] 2.2 Windows: Implement Common Item Dialog API (`IFileOpenDialog`/`IFileSaveDialog`) with Unicode support
- [x] 2.3 Windows: Handle COM initialization and dialog configuration
- [x] 2.4 Windows: Add JSON filter (`*.json`) to file dialog options
- [x] 2.5 macOS: Create `MacOSFileDialog` class in `platform/desktop/` (Objective-C++ `.mm` file)
- [x] 2.6 macOS: Implement `NSOpenPanel`/`NSSavePanel` bridge with C++ wrapper
- [x] 2.7 macOS: Add JSON file type to `allowedFileTypes` property
- [x] 2.8 macOS: Handle modal presentation and result callback
- [x] 2.9 Linux: Create `LinuxFileDialog` class in `platform/desktop/`
- [x] 2.10 Linux: Implement dynamic GTK loading with `dlopen`/`dlsym`
- [x] 2.11 Linux: Create `gtk_file_chooser_dialog_new` wrapper with fallback detection
- [x] 2.12 Linux: Add JSON filter support in GTK file chooser
- [x] 2.13 Linux: Ensure graceful degradation when GTK unavailable (fallback to ImGui)
- [x] 2.14 iOS: Create `iOSFileDialog` class in `platform/ios/`
- [x] 2.15 iOS: Implement `UIDocumentPickerViewController` integration
- [x] 2.16 iOS: Handle sandbox restrictions and document access permissions
- [x] 2.17 iOS: Maintain UIKit view hierarchy integration

## 3. Application Integration

- [x] 3.1 Update `App.cpp` to use `FileDialog` abstraction instead of ImGui modal text input
- [x] 3.2 Replace `ImGui::InputText` with `fileDialog->saveFileDialog()` in save operation
- [x] 3.3 Replace `ImGui::InputText` with `fileDialog->openFileDialog()` in load operation
- [x] 3.4 Update File menu handlers to call new dialog APIs
- [x] 3.5 Remove obsolete ImGui modal popup code for file dialogs
- [x] 3.6 Maintain backward compatibility for error handling and user feedback
- [x] 3.7 Keep existing JSON serialization logic unchanged (node graph widget calls remain same)
- [x] 3.8 Update any references to `g_fileDialogPath` or modal state variables

## 4. Build System and Dependencies

- [x] 4.1 Windows: Update `CMakeLists.txt` to link against `shlwapi.lib` for Common Item Dialog API
- [x] 4.2 Windows: Add Windows headers includes with COM support
- [x] 4.3 macOS: Update `CMakeLists.txt` to compile Objective-C++ `.mm` files
- [x] 4.4 macOS: Add `Foundation` and `AppKit` framework dependencies
- [ ] 4.5 Linux: Update `CMakeLists.txt` to detect GTK availability (optional dependency)
- [ ] 4.6 Linux: Add `dlfcn.h` includes and dynamic loading utilities
- [ ] 4.7 iOS: Update iOS `CMakeLists.txt` to include new file dialog implementation
- [ ] 4.8 iOS: Ensure UIKit framework dependency maintained

## 5. Testing and Verification

. [x] 5.1 Windows: Test save and open dialogs on Windows with JSON filter
- [x] 5.2 Windows: Verify Unicode file path handling
- [ ] 5.3 macOS: Test file dialogs on macOS with JSON file type filtering
- [ ] 5.4 macOS: Verify Objective-C++ bridge works correctly
- [ ] 5.5 Linux: Test with GTK available (Ubuntu/Debian desktop)
- [ ] 5.6 Linux: Test fallback behavior when GTK unavailable (headless/server)
- [ ] 5.7 iOS: Test document picker on iOS simulator or device
- [ ] 5.8 Cross-platform: Verify consistent behavior across all supported platforms
- [ ] 5.9 Cross-platform: Test fallback mechanism when native dialogs fail
- [x] 5.10 Integration: Verify File menu save/load operations work correctly
- [x] 5.11 Integration: Test node graph persistence unchanged (JSON serialization unchanged)
- [x] 5.12 Integration: Validate error handling and user feedback remains functional