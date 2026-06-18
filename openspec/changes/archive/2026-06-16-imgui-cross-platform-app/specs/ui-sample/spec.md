## ADDED Requirements

### Requirement: First-run sample UI
The application SHALL render a minimal sample UI on first launch that is visually identical across all supported platforms (macOS, Windows, Linux, iOS), enabling a contributor to verify the shell works by inspection.

#### Scenario: Sample UI renders on every platform
- **WHEN** the application is launched on any supported platform
- **THEN** an application window (or full-screen view on iOS) SHALL appear
- **AND** the window SHALL contain a top menu bar, a demo-window toggle, and an About dialog entry point

### Requirement: Menu bar
The sample UI SHALL include an ImGui main menu bar with at least two menus: `File` (containing `Quit` on desktop only) and `Help` (containing `About...` and `ImGui Demo`).

#### Scenario: File > Quit closes the desktop app
- **WHEN** a user selects `File > Quit` on a desktop platform
- **THEN** the application SHALL begin a clean shutdown (call `app::shutdown` and terminate)

#### Scenario: File > Quit is hidden on iOS
- **WHEN** the application runs on iOS
- **THEN** the `File > Quit` menu item SHALL NOT be shown (iOS apps are terminated by the OS, not by user-driven exit)

### Requirement: ImGui demo toggle
The sample UI SHALL provide a `Help > ImGui Demo` menu item that toggles Dear ImGui's built-in `ShowDemoWindow`. This serves both as a sanity check that the ImGui context is fully functional and as a discoverability aid for new contributors.

#### Scenario: Demo toggles open and closed
- **WHEN** a user clicks `Help > ImGui Demo` once
- **THEN** the ImGui demo window SHALL appear
- **WHEN** a user clicks `Help > ImGui Demo` a second time
- **THEN** the ImGui demo window SHALL be hidden

### Requirement: About dialog
The sample UI SHALL provide a `Help > About...` menu item that opens a modal dialog displaying the application name, the Dear ImGui version (via `IMGUI_VERSION`), and the platform name detected at compile time (e.g., "macOS", "Windows", "Linux", "iOS").

#### Scenario: About dialog reports correct platform
- **WHEN** a user opens the About dialog
- **THEN** the displayed platform name SHALL match the build's actual target platform
- **AND** the displayed ImGui version SHALL match the pinned ImGui revision's `IMGUI_VERSION` macro

### Requirement: No additional UI in v1
The sample UI SHALL NOT include any further panels, widgets, or business-domain UI beyond the menu bar, demo toggle, and About dialog. Additional UI is the responsibility of follow-on changes.

#### Scenario: Scope check
- **WHEN** v1 of this change is reviewed
- **THEN** the sample UI source contains only the menu bar, demo toggle, and About dialog code
