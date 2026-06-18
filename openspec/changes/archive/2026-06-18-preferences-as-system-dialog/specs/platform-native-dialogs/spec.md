## ADDED Requirements

### Requirement: Platform-native dialog support
The application SHALL use platform-native dialog APIs for system dialogs instead of ImGui windows. Native dialogs SHALL provide platform-consistent appearance and behavior while maintaining application functionality.

#### Scenario: Preferences dialog uses native API on Windows
- **WHEN** user opens Preferences dialog on Windows platform
/ **THEN** dialog SHALL be created using Win32 dialog APIs (not ImGui multi-viewport)
- **AND** dialog SHALL have native Windows appearance and behaviors

#### Scenario: Preferences dialog uses native API on macOS
- **WHEN** user opens Preferences dialog on macOS platform
- **THEN** dialog SHALL be created using Cocoa/AppKit dialog APIs
- **AND** dialog SHALL have native macOS appearance and behaviors

#### Scenario: Preferences dialog uses native API on Linux
- **WHEN** user opens Preferences dialog on Linux platform
- **THEN** dialog SHALL be created using GTK or platform-specific dialog APIs
- **AND** dialog SHALL have native Linux appearance and behaviors

#### Scenario: ImGui content rendered within native dialog
-[ **WHEN** native dialog is open
- **THEN** ImGui rendering SHALL be hosted within native dialog container
- **AND** all existing Preferences functionality SHALL work (tabs, theme editing, buttons)

#### Scenario: Theme live preview maintained
- **WHEN** user edits theme settings in native dialog
- **THEN** changes SHALL immediately update `ImGui::GetStyle()` for live preview
-d **AND** main application window SHALL reflect theme changes in real-time