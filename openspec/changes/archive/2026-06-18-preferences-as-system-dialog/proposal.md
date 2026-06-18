## Why

The current Preferences dialog implementation uses ImGui's multi-viewport system to create a separate OS window on desktop platforms. While functional, this approach doesn't provide a true native system dialog experience. Native system dialogs offer better platform integration, consistent appearance with OS UI guidelines, and standard behaviors that users expect. Converting the Preferences dialog to a native system dialog will improve user experience and platform consistency.

## What Changes

1. Replace ImGui multi-viewport window implementation with platform-native dialog APIs
2. Maintain the same three-tab structure (General, Theme, About) but rendered within native dialog controls
3. Preserve all existing functionality: theme editing with live preview, Save/Reset/Discard buttons, diagnostic info display
4. Ensure iOS compatibility (likely using platform-specific native dialog APIs)
5. **BREAKING**: Remove dependency on ImGui multi-viewport for desktop Preferences dialog
6. Update dialog positioning/size persistence to use platform-native mechanisms

## Capabilities

### New Capabilities
- **platform-native-dialogs**: Capability for using native OS dialog APIs instead of ImGui windows for certain UI elements

### Modified Capabilities
- **preferences-dialog**: Requirements change from ImGui multi-viewport window to native system dialog implementation
- **render-backend**: Remove multi-viewport dependency for Preferences dialog (still needed for other potential multi-viewport uses)
- **ui-sample**: Update menu behavior to trigger native dialog instead of ImGui window

## Impact

**Affected Code:**
1. `apps/imgui-shell/app/Preferences.{h,cpp}`: Complete rewrite to use native dialog APIs
2. `apps/imgui-shell/app/App.cpp`: Update menu handler to open native dialog
3. Platform-specific implementations for Windows, macOS, Linux, iOS
4. Remove multi-viewport dependency for Preferences dialog (but keep for other potential uses)

**Dependencies:**
1. Platform-specific dialog APIs:
   - Windows: MessageBox, TaskDialog, or custom dialog using Win32 API
   - macOS: NSAlert or custom NSWindow
   - Linux: GTK dialog or platform-specific solution
   - iOS: UIAlertController or custom UIView

**Integration:**
1. Theme persistence must continue to work with native dialog
2. Live theme preview needs to update ImGui style even when editing in native dialog
3. Dialog position/size persistence needs new mechanism (not ImGui ini system)