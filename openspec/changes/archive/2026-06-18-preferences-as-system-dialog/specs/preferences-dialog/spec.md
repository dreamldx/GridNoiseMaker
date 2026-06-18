## MODIFIED Requirements

### Requirement: Preferences dialog window
The application SHALL provide a Preferences dialog window with three tabs (General, Theme, About) accessible via a `Help > Preferences...` menu item. The dialog SHALL be a native system dialog on desktop platforms (using platform-native dialog APIs instead of ImGui multi-viewport) and an inline floating panel on iOS.

#### Scenario: Opening the Preferences dialog
– **WHEN** user selects `Help > Preferences...` from the main menu
- **THEN** a Preferences dialog window SHALL open
- **AND** on desktop platforms, it SHALL be a native system dialog with platform-consistent appearance and behavior
- **AND** on iOS, it SHALL appear as a floating modal panel within the main window

#### Scenario: Dialog tabs structure
- **WHEN** the Preferences dialog is open
- **THEN** it SHALL display a tab bar with three tabs: General, Theme, About
- **AND** clicking a tab SHALL switch to that tab's content

#### Scenario: General tab content
- **WHEN** user is viewing the General tab
- **THEN** it SHALL display read-only information including:
- **AND** `IMGUI_VERSION`
- **AND** `IMGUI_SHELL_PLATFORM_NAME`
-i **AND** build configuration (Debug/Release)
- **AND** Vulkan API version (or Metal on iOS)
- **AND** GPU device name

#### Scenario: Theme tab master-detail editor
- **WHEN** user is viewing the Theme tab
- **THEN** it SHALL display a master-detail layout with selectable list of theme-editable items on the left and editing widgets on the right
- **AND** color slots SHALL be editable via `ImGui::ColorEdit4` widgets with live preview
- **AND** metric fields SHALL be editable via appropriate widgets (`ImGui::SliderFloat`, `ImGui::DragFloat`, or `ImGui::SliderInt`)

#### Scenario: About tab content
- **WHEN** user is viewing the About tab
- **THEN** it SHALL display static credits content including application name, version, and acknowledgments

#### Scenario: Theme persistence buttons
– **WHEN** user is viewing the Theme tab
- **THEN** it SHALL include Save, Reset to defaults, and Discard changes buttons at the bottom
- **AND** Save button SHALL call `writeThemeToConfig` with current `ImGui::GetStyle()`
- **AND** Reset button SHALL call `applyTheme` to restore curated defaults
- **AND** Discard button SHALL call `readThemeFromConfig` to reload saved file