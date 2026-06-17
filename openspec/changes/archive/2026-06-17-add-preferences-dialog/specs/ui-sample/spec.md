## MODIFIED Requirements

### Requirement: Menu bar
The sample UI SHALL include an ImGui main menu bar with at least two menus: `File` (containing `Quit` on desktop only) and `Help` (containing `About...`, `Preferences...`, and `ImGui Demo`).

#### Scenario: File > Quit closes the desktop app
- **WHEN** a user selects `File > Quit` on a desktop platform
- **THEN** the application SHALL begin a clean shutdown (call `app::shutdown` and terminate)

#### Scenario: File > Quit is hidden on iOS
- **WHEN** the application runs on iOS
- **THEN** the `File > Quit` menu item SHALL NOT be shown (iOS apps are terminated by the OS, not by user-driven exit)