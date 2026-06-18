## ADDED Requirements

### Requirement: Preferences window opens from File menu
The application SHALL provide a `File > Preferences...` menu item that toggles a regular ImGui window titled `"Preferences"` containing a tab bar with three tabs: General, Theme, and About. The window is NOT a modal popup — the main shell remains interactive while it is open, so the user can see live preview of theme edits.

#### Scenario: Menu item opens the window
- **WHEN** a user clicks `File > Preferences...`
- **THEN** a file-scope `g_showPreferencesWindow` boolean SHALL be set to `true`
- **AND** on the same and subsequent frames `ImGui::Begin("Preferences", &g_showPreferencesWindow)` SHALL render the window
- **AND** the main shell remains interactive (menu bar, other ImGui content) while the Preferences window is open
- **AND** clicking the window's X (close button) sets `g_showPreferencesWindow` back to `false` via the `Begin` `p_open` parameter

#### Scenario: Menu-item label uses trailing ellipsis
- **WHEN** the File menu is rendered
- **THEN** the menu item label SHALL be exactly `"Preferences..."` (with trailing ellipsis per the `Help > About...` / `Help > ImGui Demo...` precedent)

#### Scenario: Preferences item visible on all platforms
- **WHEN** the File menu is rendered on any supported platform (macOS, Windows, Linux, iOS)
- **THEN** the `Preferences...` item SHALL be present
- **AND** on desktop platforms, it SHALL appear above the `Quit` item (a `Separator` between them is recommended but not required)
- **AND** on iOS, where `Quit` is hidden, `Preferences...` SHALL still render

#### Scenario: Window is multi-viewport friendly
- **WHEN** the user drags the Preferences window outside the main shell host's bounds
- **AND** `ImGuiConfigFlags_ViewportsEnable` is set (per `render-backend` "Multi-viewport support on desktop")
- **THEN** the window SHALL become a real OS-level secondary window with its own swapchain
- **AND** the master-detail editor inside SHALL continue rendering correctly at the new size

### Requirement: Tab bar — General / Theme / About in order
The Preferences window's body SHALL be `ImGui::BeginTabBar("PreferencesTabs")` containing three `ImGui::BeginTabItem` blocks in this exact order: General, Theme, About. The Theme tab SHALL be selected on first open (via `ImGuiTabItemFlags_SetSelected` on first frame). The Theme tab is the centerpiece — first impression on opening the window should be the editable surface.

#### Scenario: Three tabs render in fixed order
- **WHEN** the Preferences window opens
- **THEN** the tab bar SHALL render exactly three tab labels left-to-right: "General", "Theme", "About"
- **AND** the Theme tab SHALL be the active tab on first open
- **AND** the user MAY click any tab to switch; the selection persists across frames within a single window-open lifetime

### Requirement: General tab — read-only diagnostic info
The General tab SHALL display a flat list of read-only diagnostic information using `ImGui::Text` calls (no widgets, no editable fields). The information SHALL include at minimum: application name, application version, platform name, build configuration (Debug / Release), Dear ImGui version, Vulkan API version (queried from the active `VkPhysicalDevice` properties), GPU device name, active font rasterizer, and the absolute path to the theme config file (`app::themeConfigPath()`).

#### Scenario: General tab content is read-only
- **WHEN** the user activates the General tab
- **THEN** the body SHALL contain only `ImGui::Text` / `ImGui::TextWrapped` calls — no `Input*`, no `Slider*`, no `Button` widgets that mutate state
- **AND** each line SHALL follow a `"<label>: <value>"` pattern with consistent alignment

#### Scenario: Vulkan-info queries are cached
- **WHEN** the General tab is rendered for the first time in a session
- **THEN** Vulkan device info (API version + GPU name) SHALL be queried via `vkGetPhysicalDeviceProperties` and cached in file-scope statics
- **AND** subsequent renders within the same session SHALL use the cached values without re-querying

### Requirement: Theme tab — master-detail editor
The Theme tab SHALL provide a master-detail editor where the left pane is a `BeginChild`-scrolled selectable list of every editable theme item (grouped by `Separator + Text` headers: Colors / Metrics / Typography), and the right pane is a `BeginChild` showing a type-appropriate widget for the currently-selected item. The split SHALL be implemented via `ImGui::BeginTable("ThemeEditor", 2, ImGuiTableFlags_Resizable)` with initial column widths of approximately 35% (left) and 65% (right). All widgets in the right pane SHALL mutate `ImGui::GetStyle()` directly so changes are visible live in the main shell.

#### Scenario: Left pane lists every editable item
- **WHEN** the Theme tab is rendered
- **THEN** the left pane SHALL contain (in this order, separated by `Separator + Text` headers):
  - A "Colors" group: one `Selectable` per `ImGuiCol_*` slot from index 0 to `ImGuiCol_COUNT - 1`
  - A "Metrics" group: one `Selectable` per metric in the supported set (19 entries: `WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`, `IndentSpacing`, `ScrollbarSize`, `GrabMinSize`, `WindowRounding`, `ChildRounding`, `FrameRounding`, `GrabRounding`, `PopupRounding`, `ScrollbarRounding`, `TabRounding`, `WindowBorderSize`, `ChildBorderSize`, `PopupBorderSize`, `FrameBorderSize`, `TabBorderSize`)
  - A "Typography" group: two `Selectable` rows — `Font file` and `Font size`
- **AND** each `Selectable` SHALL display its label on the left and a current-value preview on the right (hex chip for colors, formatted number for metrics, current selection for typography)
- **AND** selection state SHALL be tracked in file-scope state and persist across frames

#### Scenario: Right pane widget matches selected item type
- **WHEN** a `Colors` entry is selected
- **THEN** the right pane SHALL render `ImGui::ColorEdit4("##color", &style.Colors[idx].x, ImGuiColorEditFlags_AlphaBar)`
- **WHEN** a scalar `Metrics` entry is selected
- **THEN** the right pane SHALL render `ImGui::SliderFloat` with the metric's documented range and step (see design.md D4)
- **WHEN** an `ImVec2` `Metrics` entry is selected (`WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`)
- **THEN** the right pane SHALL render `ImGui::DragFloat2` with the metric's documented range
- **WHEN** the Typography `Font file` entry is selected
- **THEN** the right pane SHALL render `ImGui::Combo` with at least the three bundled font options (`Inter Regular` → `fonts/Inter-Regular.ttf`, `JetBrains Mono Regular` → `fonts/JetBrainsMono-Regular.ttf`, `Lato Regular` → `fonts/Lato-Regular.ttf`) plus a `Custom...` entry that reveals an `ImGui::InputText` for a hand-entered path
- **WHEN** the Typography `Font size` entry is selected
- **THEN** the right pane SHALL render `ImGui::SliderFloat` with range `[6.0, 96.0]` and format `"%.1f px"`

#### Scenario: Typography changes display the "applies on next launch" caveat
- **WHEN** the Typography `Font file` or `Font size` widget is rendered in the right pane
- **THEN** a small inline note SHALL appear below the widget reading approximately `"Font changes apply on next launch. The current view still uses the previously-loaded font."`
- **AND** the note SHALL be rendered using `ImGui::TextDisabled` or `ImGui::TextColored` with a subdued color so it doesn't compete with the active widget

### Requirement: Autosave on edit commit + Reset
The Theme tab SHALL persist style and typography changes to disk automatically whenever a widget edit commits (mouse release for `ColorEdit4` / `SliderFloat` / `DragFloat2`, selection for `Combo`, Enter or focus-loss for `InputText`). The dialog SHALL render a single `Reset to defaults` button that reverts the in-memory style to the baked-in `app::applyTheme` defaults and immediately persists the result to disk. There is NO explicit Save button (autosave makes it redundant). There is NO Discard button (without a separate uncommitted in-memory state, there is nothing to discard).

#### Scenario: Color edit autosaves on commit
- **WHEN** the user opens a Color item, drags the alpha bar / channel sliders inside `ColorEdit4`, and releases the mouse
- **THEN** the dialog SHALL detect the commit via `ImGui::IsItemDeactivatedAfterEdit()` (or the Combo's bool return / `IsItemDeactivatedAfterEdit()` for InputText) and call `app::writeThemeToConfig(ImGui::GetStyle())` exactly once
- **AND** the on-disk `theme.json` SHALL reflect the new color in its `colors` block
- **AND** during the drag itself (before release) the file SHALL NOT be re-written each frame

#### Scenario: Slider autosaves on commit
- **WHEN** the user scrubs a `SliderFloat` (scalar metric or font size) or a `DragFloat2` (vec2 metric) and releases the mouse
- **THEN** `ImGui::IsItemDeactivatedAfterEdit()` SHALL be true exactly once and trigger one `writeThemeToConfig` call
- **AND** the in-memory style at the moment of release SHALL be what's written to disk

#### Scenario: Font Combo / custom-path InputText autosave
- **WHEN** the user picks a bundled font from the `Combo` widget
- **THEN** the `Combo` return-value-true SHALL trigger one `writeThemeToConfig` call carrying the new `themeFontFile()`
- **WHEN** the user finishes typing a custom path in the `Custom...` `InputText` (Enter or focus loss)
- **THEN** `ImGui::IsItemDeactivatedAfterEdit()` SHALL fire once and trigger one `writeThemeToConfig` call

#### Scenario: Reset reverts to baked-in defaults and persists
- **WHEN** the user clicks `Reset to defaults`
- **THEN** the dialog SHALL call `app::applyTheme(ImGui::GetStyle())` then immediately `app::writeThemeToConfig(ImGui::GetStyle())`
- **AND** the in-memory style SHALL be the curated `app/Theme.cpp` baseline
- **AND** the on-disk `theme.json` SHALL be overwritten with the baseline values

#### Scenario: No Save / Discard buttons rendered
- **WHEN** the Theme tab is open
- **THEN** the body SHALL NOT render a button labeled `Save`, `Discard`, or `Discard unsaved changes`
- **AND** the only button at the bottom of the Theme tab SHALL be `Reset to defaults`

### Requirement: About tab — credits and copyright
The About tab SHALL display a flat list of static text lines using `ImGui::Text` / `ImGui::TextWrapped`. The content SHALL include: application name + version, copyright placeholder, license placeholder, and a `Built with:` list of third-party dependencies with their name + license short-form.

#### Scenario: About tab lists every shipped dependency
- **WHEN** the About tab is rendered
- **THEN** the body SHALL include at minimum: Dear ImGui (`IMGUI_VERSION`), Inter font, JetBrains Mono font, Lato font, FreeType, GLFW, nlohmann/json, Vulkan loader / MoltenVK
- **AND** each entry SHALL include the dependency's license short-form (e.g., "MIT", "SIL OFL 1.1", "Apache 2.0", "FreeType License / GPLv2", "zlib")
- **AND** the body SHALL contain no widgets — text only

### Requirement: Window state — file-scope flag, no ini persistence
The dialog's open/closed state SHALL be tracked in a file-scope `bool g_showPreferencesWindow` initialized to `false`. The application SHALL NOT persist the dialog's position, size, or open/closed state across process restarts via ImGui's ini file (consistent with the existing `io.IniFilename = nullptr` setup).

#### Scenario: Window closed on launch
- **WHEN** the application starts
- **THEN** `g_showPreferencesWindow` SHALL be `false`
- **AND** the Preferences window SHALL NOT render until the user clicks `Help > Preferences...`

#### Scenario: No ini persistence
- **WHEN** the user closes and reopens the Preferences window
- **THEN** the window SHALL appear at ImGui's default position/size for a newly-opened window (centered or wherever ImGui chooses)
- **AND** the codebase SHALL NOT call any `Begin`/`SetNextWindow*` API that would persist position/size in an `imgui.ini` file
