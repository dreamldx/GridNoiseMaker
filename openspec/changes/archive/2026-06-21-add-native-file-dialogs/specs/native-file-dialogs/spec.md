# Native File Dialogs

## Purpose
Provide native operating system file dialogs for save and load operations, replacing simple ImGui modal text inputs with platform-specific interfaces that offer better user experience and system integration.

## ADDED Requirements

### Requirement: Cross-platform file dialog abstraction
The system SHALL provide a cross-platform file dialog abstraction layer that exposes consistent save/open operations while delegating to platform-specific native implementations.

#### Scenario: Save dialog abstraction
- **WHEN** the application requests a save dialog
—
 **THEN** the abstraction layer SHALL present a native save dialog appropriate for the current platform
- **AND** SHALL return the selected file path or empty string if cancelled
- **AND** SHALL apply JSON file filter (`*.json`) on platforms that support file filtering

#### Scenario: Open dialog abstraction
- **WHEN** the application requests an open dialog
- **THEN** the abstraction layer SHALL present a native open dialog appropriate for the current platform
- **AND** SHALL return the selected file path or empty string if cancelled
- **AND** SHALL apply JSON file filter (`*.json`) on platforms that support file filtering

#### Scenario: Graceful fallback to ImGui modal
- **WHEN** native file dialog fails or is unavailable on current platform
- **THEN** the system SHALL fall back to ImGui modal text input
- **AND** SHALL log the failure for debugging purposes
- **AND** SHALL maintain application functionality

### Requirement: Platform-specific native dialog implementations
The system SHALL provide platform-specific implementations of file dialogs using each operating system's native APIs.

#### Scenario: Windows file dialogs
- **WHEN** running on Windows platform
- **THEN** save dialogs SHALL use `IFileSaveDialog` API (Common Item Dialog) for modern Windows experience with Unicode support
- **AND** open dialogs SHALL use `IFileOpenDialog` API (Common Item Dialog) for modern Windows experience with Unicode support
- **AND** SHALL include JSON file filter in dialog options
- **AND** SHALL handle COM initialization if required

#### Scenario: macOS file dialogs
- **WHEN** running on macOS platform
- **THEN** save dialogs SHALL use `NSSavePanel` Objective-C API
- **AND** open dialogs SHALL use `NSOpenPanel` Objective-C API
- **AND** SHALL include JSON file filter in allowed file types
- **AND** SHALL bridge C++ calls to Objective-C via `.mm` files

#### Scenario: Linux file dialogs
- **WHEN** running on Linux platform
- **THEN** file dialogs SHALL use GTK file chooser APIs (`gtk_file_chooser_dialog_new`)
- **AND** SHALL dynamically load GTK library at runtime via `dlopen`/`dlsym`
? **AND** SHALL fall back to ImGui modal if GTK unavailable
- **AND** SHALL include JSON file filter in dialog options

#### Scenario: iOS file picker
- **WHEN** running on iOS platform
- **THEN** file selection SHALL use `UIDocumentPickerViewController`
- **AND** SHALL respect iOS sandbox restrictions for file access
- **AND** SHALL present document picker modally within UIKit view hierarchy