# ui-sample Specification

## Purpose
TBD - created by archiving change imgui-cross-platform-app. Update Purpose after archive.
## Requirements
### Requirement: First-run sample UI
The application SHALL render a minimal sample UI on first launch that is visually identical across all supported platforms (macOS, Windows, Linux, iOS), enabling a contributor to verify the shell works by inspection.

#### Scenario: Sample UI renders on every platform
- **WHEN** the application is launched on any supported platform
- **THEN** an application window (or full-screen view on iOS) SHALL appear
- **AND** the window SHALL contain a top menu bar, a demo-window toggle, and an About dialog entry point

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

### Requirement: ImGui demo modal
The sample UI SHALL provide a `Help > ImGui Demo...` menu item that opens an ImGui modal popup (via `ImGui::OpenPopup` + `ImGui::BeginPopupModal`) when clicked. The modal's body SHALL display a short placeholder explaining that `ImGui::ShowDemoWindow` opens its own top-level window and therefore cannot be rendered inside an ImGui modal popup (due to ImGui's window-stack architecture), and SHALL point readers at `<imgui-src>/imgui_demo.cpp` for the actual demo source. The modal SHALL be dismissed via a `Close` button matching the visual treatment of the existing `Help > About...` modal's Close button. The menu-item label SHALL include a trailing ellipsis (`"ImGui Demo..."`) per common GUI conventions for "opens a dialog" actions.

#### Scenario: Menu opens the modal
- **WHEN** a user clicks `Help > ImGui Demo...`
- **THEN** `ImGui::OpenPopup("ImGui Demo")` SHALL be invoked in `app::frame`
- **AND** the popup SHALL render in the next frame via `ImGui::BeginPopupModal("ImGui Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)`
- **AND** the modal SHALL block keyboard / mouse interaction with the rest of the UI (the standard `BeginPopupModal` focus-capture behavior)
- **AND** the modal background SHALL be dimmed per the current theme's `ImGuiCol_ModalWindowDimBg` color (the same way the existing About modal dims its background)

#### Scenario: Modal dismissed via Close button
- **WHEN** the user clicks the `Close` button inside the ImGui Demo modal
- **THEN** `ImGui::CloseCurrentPopup()` SHALL be invoked
- **AND** the modal SHALL disappear in the next frame
- **AND** no demo window or any other ImGui window related to the demo SHALL remain rendered

#### Scenario: No togglable demo window exists in the application
- **WHEN** the application is running
- **THEN** `ImGui::ShowDemoWindow` SHALL NOT be called from any code path in `app/` or `platform/`
- **AND** the `g_showDemoWindow` static flag from the previous toggle-based implementation SHALL NOT exist
- **AND** `imgui_demo.cpp` SHALL still be compiled into the `imgui` static library (per the `build-system` "ImGui compiled as a static library" requirement) — it is reachable in the binary but unreferenced from application code

#### Scenario: Modal placeholder body contains expected guidance
- **WHEN** a user opens the ImGui Demo modal
- **THEN** the body SHALL contain text explaining that ImGui's demo content lives in `ImGui::ShowDemoWindow`, that ImGui's window stack does not allow `ImGui::Begin` to nest inside `BeginPopupModal`, and that the demo source is at `<imgui-src>/imgui_demo.cpp` (the path resolved via the existing `FetchContent` checkout)
- **AND** the body SHALL be at most ~5 lines of `ImGui::Text` / `ImGui::TextWrapped` calls (no widgets, no images, no embedded ImGui demo content)

