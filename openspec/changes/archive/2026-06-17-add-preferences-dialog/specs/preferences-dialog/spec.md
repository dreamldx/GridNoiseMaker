## ADDED Requirements

### Requirement: Preferences dialog window
The application SHALL provide a Preferences dialog window with three tabs (General, Theme, About) accessible via a `Help > Preferences...` menu item. The dialog SHALL be a separate OS window on desktop platforms (using ImGui multi-viewport mode) and an inline floating panel on iOS.

#### Scenario: Opening the Preferences dialog
. **WHEN** user selects `Help > Preferences...` from the main menu
- **THEN** a Preferences dialog window SHALL open
- **AND** on desktop platforms, it SHALL be a separate OS window that can be moved and resized independently
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
- **AND** build configuration (Debug/Release)
- **AND** Vulkan API version (or Metal on iOS)
- **AND** GPU device name

#### Scenario: Theme tab master-detail editor
- **WHEN** user is viewing the Theme tab
- **THEN** it SHALL display a two-pane master-detail layout
- **AND** the left pane SHALL contain a selectable list of all theme-editable items (color slots first, then metric fields)
- **AND** the right pane SHALL display appropriate editing widgets for the selected item:
- **AND** for colors: `ImGui::ColorEdit4` widget
- **AND** for metrics: `ImGui::SliderFloat`, `ImGui::DragFloat`, or `ImGui::SliderInt` depending on the metric type

#### Scenario: Theme tab interaction and persistence
- **WHEN** user makes changes in the Theme tab
- **THEN** changes SHALL immediately update `ImGui::GetStyle()` for live preview
- **AND** the tab SHALL provide three buttons at the bottom:
- **AND** `Save` button writes current style to the config file via `writeThemeToConfig`
- **AND** `Reset to theme defaults` button calls `applyTheme` again
- **AND** `Discard unsaved changes` button re-reads the saved file via `readThemeFromConfig`

#### Scenario: About tab content
- **WHEN** user is viewing the About tab
- **THEN** it SHALL display static content including:
- **AND** project name, version, year, and license placeholder
- **AND** Dear ImGui credit
- **AND** Inter font credit with OFL link
- **AND** FreeType credit

#### Scenario: Dialog persistence
- **WHEN** user closes the Preferences dialog
- **THEN** its last-known position and size SHALL be persisted via ImGui's standard ini file mechanism
- **AND** the dialog SHALL reopen at the same position and size when next opened