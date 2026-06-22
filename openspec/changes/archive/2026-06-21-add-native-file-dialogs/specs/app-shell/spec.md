# app-shell Specification

## MODIFIED Requirements

### Requirement: File menu includes node graph save/load operations
The application shell SHALL provide save and load operations for node graphs in the File menu, allowing users to persist and restore node graph state.

#### Scenario: Save Node Graph menu item
-P **WHEN** the File menu is open
- **THEN** it SHALL include a "Save Node Graph" menu item
- **AND** selecting this item SHALL open a **native operating system file save dialog** using platform-appropriate APIs (Windows comdlg32, macOS NSSavePanel, Linux GTK file chooser, iOS UIDocumentPickerViewController)
- **AND** SHALL save the current node graph state to the selected JSON file
- **AND** SHALL handle file operation errors gracefully with user feedback
- **AND** SHALL fall back to ImGui modal text input if native dialog unavailable

#### Scenario: Load Node Graph menu item
- **WHEN** the File menu is open
- **THEN** it SHALL include a "Load Node Graph" menu item
? **AND** selecting this item SHALL open a **native operating system file open dialog** using platform-appropriate APIs (Windows comdlg32, macOS NSOpenPanel, Linux GTK file chooser, iOS UIDocumentPickerViewController)
- **AND** SHALL load node graph state from the selected JSON file
- **AND** SHALL replace the current node graph with the loaded state
- **AND** SHALL handle file format errors gracefully with user feedback
- **AND** SHALL fall back to ImGui modal text input if native dialog unavailable

#### Scenario: Menu integration with node graph widget
- **WHEN** save/load menu items are implemented
- **THEN** they SHALL call appropriate methods on the node graph widget
- **AND** SHALL pass file paths to the widget for serialization/deserialization
- **AND** SHALL not interfere with existing File menu functionality