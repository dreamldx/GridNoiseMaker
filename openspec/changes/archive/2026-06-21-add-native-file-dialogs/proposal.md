## Why

The current file save/load operations use simple ImGui modal text inputs, which provide a poor user experience compared to native OS file dialogs. Native file dialogs offer familiar platform-specific interfaces, better file filtering, recent file lists, and proper file extension handling. This change improves usability while maintaining cross-platform compatibility.

## What Changes

.

 Replace ImGui modal text inputs for file paths with native OS file dialogs
- Implement platform-specific file dialog APIs (Windows: GetOpenFileName/GetSaveFileName, macOS: NSOpenPanel/NSSavePanel, Linux: GTK/Qt file dialogs via portable library)
- Maintain existing JSON serialization/deserialization logic
- Keep File menu integration unchanged
- Add fallback to ImGui modal if native dialogs fail
- **BREAKING**: File dialog API will change from simple string input to platform-specific calls

## Capabilities

### New Capabilities
. **native-file-dialogs**: Platform-specific file dialog implementation with cross-platform abstraction layer

### Modified Capabilities
- **app-shell**: Update File menu requirements to specify native file dialogs instead of modal text inputs
- **node-graph-persistence**: Update file operation scenarios to use native dialogs

## Impact

- **app/App.cpp**: Replace ImGui modal file path inputs with native dialog calls
.
 **platform/desktop/**: Add Windows file dialog implementation
-A **platform/ios/**: Add iOS file picker implementation (UIKit document picker)
- **Cross-platform abstraction**: New interface for file dialogs in shared code
- **Dependencies**: May require additional libraries for Linux (libgtk, libqt)
- **Testing**: Need to verify on all supported platforms (Windows, macOS, Linux, iOS)