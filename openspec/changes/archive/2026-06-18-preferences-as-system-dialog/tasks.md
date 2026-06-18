## 1. Platform Abstraction Layer

- [x] 1.1 Create `NativeDialog` abstract base class in `apps/imgui-shell/app/NativeDialog.h`
.
- [x] 1.2 Define platform-agnostic interface for dialog creation, showing, hiding, and content rendering
- [x] 1.3 Create `Win32NativeDialog` class for Windows platform implementation
- [x] 1.4 Create platform detection and factory method to instantiate correct dialog type

## 2. Windows Implementation

-

[x] 2.1 Implement `Win32NativeDialog::create()` using Win32 dialog APIs
- [x] 2.2 Create Win32 dialog window with appropriate styles (WS_OVERLAPPEDWINDOW, etc.)
- [x] 2.3 Integrate ImGui rendering within Win32 dialog (using GLFW for window management)
.

[~] 2.4 Handle dialog message loop and event forwarding to ImGui (basic message loop implemented, runtime testing blocked by font issue)
- [ ] 2.5 Implement dialog positioning/size persistence using Windows Registry or ini file

## 3. Preferences Dialog Integration

- [x] 3.1 Update `Preferences` class to use `NativeDialog` instead of ImGui window
- [ ] 3.2 Modify `Preferences::render()` to delegate to native dialog
- [ ] 3.3 Ensure all three tabs (General, Theme, About) render correctly in native dialog
- [ ] 3.4 Test theme editing widgets (ColorEdit4, SliderFloat, etc.) in native dialog context
- [ ] 3.5 Verify Save/Reset/Discard buttons work with native dialog

## 4. Theme Live Preview Integration

1. [x] 4.1 Implement callback system between native dialog and main ImGui context
- [ ] 4.2 Ensure theme changes in dialog immediately update `ImGui::GetStyle()`
- [ ] 4.3 Test live preview updates main application window in real-time
-F [ ] 4.4 Verify theme persistence (`writeThemeToConfig`, `readThemeFromConfig`) works with native dialog

## 5. macOS Implementation

- [ ] 5.1 Create `CocoaNativeDialog` class for macOS platform
- [ ] 5.2 Implement using Cocoa/AppKit APIs (NSWindow, NSView)
- [ ] 5.3 Host ImGui rendering within NSWindow/NSView
- [ ] 5.4 Handle macOS-specific dialog behaviors and appearance
- [ ] 5.5 Implement persistence using NSUserDefaults

## 6. Linux Implementation

- [ ] 6.1 Create `GtkNativeDialog` class for Linux platform
- [ ] 6.2 Implement using GTK3 or GTK4 dialog APIs
- [ ] 6.3 Host ImGui rendering within GtkWindow
- [ ] 6.4 Handle Linux-specific dialog behaviors
- [ ] 6.5 Implement persistence using GSettings or config file

## 7. iOS Implementation Strategy

- [ ] 7.1 Evaluate iOS dialog options: UIAlertController vs custom UIView
- [ ] 7.2 Determine if native dialog or inline panel is better for iOS
- [ ] 7.3 Create `UIKitNativeDialog` class or update existing iOS panel approach
- [ ] 7.4 Implement iOS-specific dialog/panel rendering

## 8. Multi-Viewport Compatibility

- [ ] 8.1 Keep `ImGuiConfigFlags_ViewportsEnable` enabled for other potential uses
- [ ] 8.2 Update `render-backend` implementation to handle mixed mode (native dialogs + multi-viewport)
- [ ] 8.3 Test that other ImGui windows can still use multi-viewport when dragged outside
- [ ] 8.4 Update documentation to clarify multi-viewport is optional for dialogs

## 9. Build System Updates

- [x] 9.1 Add `NativeDialog.{h,cpp}` and platform-specific files to `app/CMakeLists.txt`
- [ ] 9.2 Add platform-specific dependencies (Windows API, Cocoa, GTK) to build system
- [ ] 9.3 Ensure cross-platform compilation works (guard platform-specific code)
- [ ] 9.4 Test build on all target platforms: Windows, macOS, Linux, iOS

## 10. Testing and Verification

- [ ] 10.1 Test Preferences dialog opens/closes via `Help > Preferences...` menu on each platform
- [ ] 10.2 Verify native dialog appearance matches platform UI guidelines
- [ ] 10.3 Test all dialog functionality: tabs, theme editing, buttons, persistence
- [ ] 10.4 Verify backward compatibility (existing theme.json files still work)
- [ ] 10.5 Test mixed mode: native Preferences dialog + multi-viewport for other windows
- [ ] 10.6 Run `openspec validate preferences-as-system-dialog --type change --strict`