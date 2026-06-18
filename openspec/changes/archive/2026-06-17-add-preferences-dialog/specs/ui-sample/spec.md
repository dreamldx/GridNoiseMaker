## MODIFIED Requirements

### Requirement: Menu bar
The sample UI SHALL include an ImGui main menu bar with at least two menus: `File` (containing `Preferences...` on all platforms and `Quit` on desktop only) and `Help` (containing `About...` and `ImGui Demo`).

#### Scenario: File > Quit closes the desktop app
- **WHEN** a user selects `File > Quit` on a desktop platform
- **THEN** the application SHALL begin a clean shutdown (call `app::shutdown` and terminate)

#### Scenario: File > Quit is hidden on iOS
- **WHEN** the application runs on iOS
- **THEN** the `File > Quit` menu item SHALL NOT be shown (iOS apps are terminated by the OS, not by user-driven exit)

#### Scenario: File menu contains Preferences item
- **WHEN** the user opens the `File` menu on any platform
- **THEN** a `Preferences...` menu item SHALL be present (with trailing ellipsis per the `Help > About...` / `Help > ImGui Demo...` precedent)
- **AND** on desktop platforms, `Preferences...` SHALL appear above the `Quit` item
- **AND** on iOS (where `Quit` is hidden), `Preferences...` SHALL still render

#### Scenario: File > Preferences... opens the Preferences window
- **WHEN** the user clicks `File > Preferences...`
- **THEN** the file-scope `g_showPreferencesWindow` boolean defined in the `preferences-dialog` capability SHALL be set to `true`
- **AND** in the same and subsequent frames the Preferences window SHALL render via `ImGui::Begin("Preferences", &g_showPreferencesWindow)` (per the `preferences-dialog` "Preferences window opens from File menu" requirement)
- **AND** the existing `Help > About...` modal popup and `Help > ImGui Demo...` modal popup SHALL continue to function unchanged
