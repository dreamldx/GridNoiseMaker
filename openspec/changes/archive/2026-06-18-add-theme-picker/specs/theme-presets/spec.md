## ADDED Requirements

### Requirement: Bundled theme catalog at `assets/themes/`
The application SHALL ship a read-only bundled-theme catalog under `apps/imgui-shell/assets/themes/`. Each file in the directory SHALL be a `*.json` file in the existing theme schema (top-level `colors` / `metrics` / `typography` blocks per the `theme-persistence` requirement "JSON schema and parsing rules"). The v1 catalog SHALL contain at least two files: `dark.json` (the curated baseline mirroring `app::applyTheme`) and `light.json` (an inverted light-mode variant with the same accent hue). On desktop the bundled files SHALL be copied next to the binary by the existing post-build `assets/` copy step; on iOS they SHALL be added to the `IMGUI_SHELL_IOS_THEMES` list in `platform/ios/CMakeLists.txt` (mirroring the existing `IMGUI_SHELL_IOS_FONTS` pattern).

#### Scenario: Bundled catalog at expected build-output paths
- **WHEN** the desktop build completes
- **THEN** the path `<binary-dir>/platform/desktop/assets/themes/dark.json` SHALL exist
- **AND** the path `<binary-dir>/platform/desktop/assets/themes/light.json` SHALL exist
- **WHEN** the iOS build completes
- **THEN** the produced `.app` bundle SHALL contain `Resources/themes/dark.json` and `Resources/themes/light.json`

#### Scenario: Bundled themes are loadable without warnings
- **WHEN** the application launches with the bundled `dark.json` selected
- **THEN** the file SHALL parse cleanly via the existing JSON parser
- **AND** no `unknown color key` / `unknown metric key` warnings SHALL be logged
- **AND** the resulting `ImGuiStyle` SHALL match `app::applyTheme`'s curated baseline within float-precision tolerance
- **WHEN** the application launches with the bundled `light.json` selected
- **THEN** the file SHALL parse cleanly with no warnings
- **AND** the background colors SHALL be light (off-white / light gray) and text colors near-black, while the accent (`SliderGrab`, `CheckMark`, etc.) SHALL remain the same cyan-blue used in `dark.json` to keep brand identity

### Requirement: User theme directory at `<config-dir>/imgui-shell/themes/`
The application SHALL maintain a user-writable theme directory at `<config-dir>/imgui-shell/themes/` where `<config-dir>` is resolved per the existing `theme-persistence` per-OS path resolver (XDG_CONFIG_HOME / APPDATA / NSDocumentDirectory). User-dir theme files SHALL use the same `colors / metrics / typography` schema as bundled files. The directory SHALL be created lazily — no `mkdir` on launch; it is created on the first `writeThemeFile` that targets it.

#### Scenario: User dir created lazily on first write
- **WHEN** the application launches and no user theme has ever been edited
- **THEN** `<config-dir>/imgui-shell/themes/` MAY NOT exist on disk
- **WHEN** the user edits a color and the autosave commit triggers `writeThemeFile`
- **THEN** the parent directory SHALL be created via `std::filesystem::create_directories` before the write
- **AND** the user-dir file at `<config-dir>/imgui-shell/themes/<selected-theme>.json` SHALL contain the serialized style

#### Scenario: User-dir overrides bundled for the same name
- **WHEN** both `<bundled>/dark.json` and `<user-dir>/dark.json` exist
- **THEN** the picker's `dark` entry SHALL resolve to the user-dir file
- **AND** loading `dark` SHALL parse the user-dir file
- **AND** writing `dark` SHALL write to the user-dir file (the bundled file SHALL NEVER be modified at runtime)

### Requirement: Theme enumeration — bundled ∪ user, user wins
The application SHALL expose `app::listAvailableThemes()` returning an alphabetically-sorted list of unique theme names enumerated from `assets/themes/*.json` (bundled) and `<config-dir>/imgui-shell/themes/*.json` (user). Names with `.json` extension SHALL be considered themes; other files SHALL be ignored. Hidden files (leading `.`) SHALL be excluded. The stem (filename without `.json`) SHALL be the theme name. Atomic-write temp files (matching `*.json.tmp.<pid>`) SHALL be excluded by the `.json` extension filter (their extension is `.<pid>`, not `.json`).

#### Scenario: Enumeration combines both dirs with user winning
- **WHEN** the bundled dir contains `dark.json`, `light.json` and the user dir contains `dark.json`, `winter.json`
- **THEN** `listAvailableThemes()` SHALL return exactly three entries in alphabetical order: `dark`, `light`, `winter`
- **AND** the `dark` entry SHALL resolve to the user-dir file (user wins on name collision)
- **AND** the `light` entry SHALL resolve to the bundled file
- **AND** the `winter` entry SHALL resolve to the user-dir file (no bundled origin)

#### Scenario: Missing user dir is not an error
- **WHEN** `<config-dir>/imgui-shell/themes/` does not exist
- **THEN** `listAvailableThemes()` SHALL return only the bundled entries
- **AND** SHALL NOT log a warning

#### Scenario: Missing bundled dir falls back to user dir only
- **WHEN** `assets/themes/` does not exist next to the binary (theoretical asset-copy failure)
- **THEN** `listAvailableThemes()` SHALL return only the user-dir entries
- **AND** SHALL log one warning of the form `bundled themes dir '<path>' not found; only user themes will be available`

#### Scenario: Both dirs empty falls back to baked-in default
- **WHEN** both `assets/themes/` and `<config-dir>/imgui-shell/themes/` are empty or absent
- **THEN** `listAvailableThemes()` SHALL return an empty list
- **AND** the application SHALL render with `app::applyTheme`'s curated baseline (the in-memory style after `applyTheme` runs at init)
- **AND** the picker UI SHALL show a `(no themes)` placeholder

### Requirement: Selection persistence in `theme.json`
The application SHALL persist the active theme name in a top-level `selected_theme` field of `<config-dir>/imgui-shell/theme.json`. The file SHALL also contain `_schema_version: 2` as a migration signal. At app startup, `app::init` SHALL call `app::readSelectedThemeName()` to retrieve the selection, then `app::applySelectedThemeToStyle(style)` to load and apply that theme's file content.

#### Scenario: theme.json contains selection only
- **WHEN** a user has selected the `dark` theme and the migration has run (or the file was freshly written by this change)
- **THEN** `<config-dir>/imgui-shell/theme.json` SHALL contain exactly: `{"_schema_version": 2, "selected_theme": "dark"}`
- **AND** SHALL NOT contain top-level `colors`, `metrics`, or `typography` blocks
- **AND** `colors / metrics / typography` SHALL be loaded from `<config-dir>/imgui-shell/themes/dark.json` (or `assets/themes/dark.json` if no user-dir copy exists)

#### Scenario: Missing or invalid selection falls back to `dark`
- **WHEN** `theme.json` does not exist, lacks the `selected_theme` field, or names a theme that's not present in `listAvailableThemes()`
- **THEN** the selection SHALL fall back to `"dark"`
- **AND** if `dark` itself is missing from both bundled and user dirs, the application SHALL render with `app::applyTheme`'s curated baseline
- **AND** the fallback path SHALL log exactly one warning per launch identifying which fallback was hit

### Requirement: Picker UI — `ImGui::Combo` at the top of the Theme tab
The Preferences Theme tab SHALL render a theme-picker row above the existing master-detail editor table. The picker row SHALL contain a `"Theme:"` label, an `ImGui::Combo` widget listing the names returned by `app::listAvailableThemes()`, and SHALL be visually separated from the master-detail table below via the table's existing vertical-size reservation (`ImVec2(0, -(buttonRowHeight + pickerHeight))`).

#### Scenario: Combo enumerates all available themes
- **WHEN** the Preferences window opens with the Theme tab active
- **THEN** the Combo widget at the top of the tab SHALL list every name returned by `app::listAvailableThemes()` in alphabetical order
- **AND** the currently-selected theme SHALL be the active Combo entry
- **AND** the Combo entry for a user-dir-override theme MAY append a visual indicator (e.g., `*` suffix) to distinguish it from a pristine bundled theme

#### Scenario: Selecting a theme loads it live
- **WHEN** the user picks a different theme from the Combo
- **THEN** the dialog SHALL call `app::switchTheme(<picked-name>)` which: (a) reads the picked theme's file content, (b) applies it to `ImGui::GetStyle()`, (c) writes the new `selected_theme` value to `theme.json`
- **AND** the main shell SHALL repaint immediately with the new colors / metrics
- **AND** subsequent autosave commits SHALL target the newly-selected theme's user-dir file

### Requirement: Copy-on-write — first edit creates user-dir copy
The application SHALL transparently create a user-dir copy of a bundled theme on the first widget-commit edit that targets it. The implementation SHALL be: `persistTheme()` always calls `writeThemeFile(<user-dir>/<selected>.json, style)`. The user-dir file is created if absent; subsequent edits overwrite it. The bundled file at `assets/themes/<selected>.json` SHALL NEVER be modified at runtime.

#### Scenario: First edit creates user-dir copy
- **WHEN** the user has selected `dark` (a bundled theme with no user-dir copy yet) and edits any widget that commits via `IsItemDeactivatedAfterEdit` or Combo return-true
- **THEN** `<config-dir>/imgui-shell/themes/dark.json` SHALL be created (parent dirs created as needed)
- **AND** the file SHALL contain the post-edit `ImGui::GetStyle()` serialized in the standard schema
- **AND** the bundled file at `<binary-dir>/platform/desktop/assets/themes/dark.json` SHALL be byte-identical to its pre-edit content

#### Scenario: Subsequent edits overwrite the user-dir file
- **WHEN** a user-dir copy already exists for the selected theme and the user makes another edit
- **THEN** the autosave call SHALL atomically overwrite the user-dir file via the existing temp+rename pattern
- **AND** the bundled file (if it exists) SHALL remain unchanged

#### Scenario: Edit on a user-only theme writes to user dir
- **WHEN** the user has selected a theme that exists only in the user dir (no bundled origin) and makes an edit
- **THEN** the write SHALL overwrite the existing user-dir file
- **AND** SHALL NOT attempt to create or write anything in `assets/themes/`

### Requirement: Reset to defaults — delete user copy, reload bundled
The Preferences Theme tab's `Reset to defaults` button SHALL reset the *selected* theme to its bundled baseline: delete the user-dir file for the selected theme (if any), then re-apply the bundled file (if any) to `ImGui::GetStyle()`. If the selected theme has no bundled origin (a user-only theme), Reset SHALL be a no-op and log one informational stderr line.

#### Scenario: Reset removes user copy and reloads bundled
- **WHEN** the user has selected `dark` (bundled), made edits creating `<user-dir>/dark.json`, and clicks `Reset to defaults`
- **THEN** the dialog SHALL call `std::filesystem::remove(<user-dir>/dark.json)` (silent if not present)
- **AND** SHALL parse `<bundled>/dark.json` and apply its content to `ImGui::GetStyle()` (replacing the in-memory edited state)
- **AND** the main shell SHALL repaint with the bundled `dark` values
- **AND** `theme.json` SHALL NOT change (the selection still points at `dark`)

#### Scenario: Reset on a user-only theme is a no-op
- **WHEN** the selected theme exists only in the user dir (no bundled file with the same name) and the user clicks `Reset to defaults`
- **THEN** the dialog SHALL log one stderr line: `Reset: '<name>' has no bundled origin; no action taken`
- **AND** the user-dir file SHALL NOT be deleted
- **AND** `ImGui::GetStyle()` SHALL be unchanged

### Requirement: One-time migration of legacy `theme.json`
On `app::init`, before reading the selection, the application SHALL detect a legacy-format `theme.json` (no `_schema_version: 2`, contains top-level `colors / metrics / typography`) and migrate it to the new layout: copy the inline blocks into `<config-dir>/imgui-shell/themes/custom.json`, then rewrite `theme.json` to `{"_schema_version": 2, "selected_theme": "custom"}`. The migration SHALL be ordered so the new `themes/custom.json` is written and renamed successfully BEFORE `theme.json` is overwritten — a mid-migration crash leaves the legacy file intact and the next launch retries.

#### Scenario: Legacy theme.json migrated to themes/custom.json
- **WHEN** the application launches with an existing `theme.json` containing `colors`, `metrics`, or `typography` blocks and lacking `_schema_version`
- **THEN** the migration SHALL create `<config-dir>/imgui-shell/themes/custom.json` containing the original `colors / metrics / typography` blocks
- **AND** the migration SHALL then rewrite `theme.json` to `{"_schema_version": 2, "selected_theme": "custom"}`
- **AND** the migration SHALL log exactly one stderr line: `migrated legacy theme.json to themes/custom.json (selected_theme: custom)`
- **AND** subsequent launches SHALL detect `_schema_version: 2` and skip migration

#### Scenario: Migration is atomic on the new file first
- **WHEN** the migration begins and the `writeThemeFile` for `themes/custom.json` fails partway (out of disk space, permission denied, etc.)
- **THEN** `theme.json` SHALL be left in its legacy shape (NOT rewritten)
- **AND** the next launch SHALL re-attempt the migration
- **AND** the user SHALL see the legacy theme values applied (via the legacy parse path, OR via the new path falling back to baked-in defaults if migration is purged in v3) — for v2, the legacy path is preserved as a fallback parser inside the same parse step

#### Scenario: Modern theme.json is left untouched
- **WHEN** `theme.json` already contains `_schema_version: 2` (regardless of whether `selected_theme` is set)
- **THEN** the migration step SHALL be a no-op (skip)
- **AND** no log line SHALL be emitted
