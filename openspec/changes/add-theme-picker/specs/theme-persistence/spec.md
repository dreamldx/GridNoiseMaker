## MODIFIED Requirements

### Requirement: Theme persisted to per-OS config file
The application SHALL persist theme state in a per-user config tree under a per-OS platform-conventional path. The tree SHALL contain two parts: (a) a small `theme.json` recording which theme is selected (top-level `selected_theme: "<name>"` + `_schema_version: 2`), and (b) a `themes/` subdirectory holding per-theme JSON files in the existing `colors / metrics / typography` schema. At `app::init` time, after `app::applyTheme(style)`, the application SHALL read the selected theme name from `theme.json` (with fallback to `"dark"`) and apply the corresponding theme file to `style`. There SHALL be no inline `colors / metrics / typography` blocks at the top level of `theme.json`.

#### Scenario: Read order — selected theme overrides curated overrides ImGui defaults
- **WHEN** `app::init` runs in any build
- **THEN** the init sequence SHALL execute in exactly this order: `ImGui::StyleColorsDark(&style)` → `app::applyTheme(style)` → `app::migrateLegacyThemeFile()` (no-op if already migrated) → `app::applySelectedThemeToStyle(style)`
- **AND** after the sequence, any slot the selected theme file defines SHALL equal that file's value
- **AND** any slot the selected theme file does NOT define but `applyTheme` sets SHALL equal the curated theme value
- **AND** any slot neither the selected theme file nor `applyTheme` touched SHALL equal the `StyleColorsDark` default

#### Scenario: Missing theme.json is a silent no-op
- **WHEN** `app::readSelectedThemeName()` is called and `theme.json` does not exist at `app::themeConfigPath()`
- **THEN** the function SHALL return the default `"dark"` without modifying `style`
- **AND** no error SHALL be logged
- **AND** `app::applySelectedThemeToStyle(style)` SHALL then load the `dark` theme file (bundled or user) if available, or be a no-op if neither exists

### Requirement: JSON schema and parsing rules
The `<config-dir>/imgui-shell/theme.json` file SHALL be a single top-level JSON object containing at least these two fields: `_schema_version` (integer, currently `2`) and `selected_theme` (string, the name of an available theme without `.json` extension). Per-theme files at `<config-dir>/imgui-shell/themes/<name>.json` and `assets/themes/<name>.json` SHALL contain top-level `colors`, `metrics`, and `typography` blocks in the existing schema. Any key whose name begins with underscore (`_`) SHALL be silently ignored at load time inside any of these blocks — this provides a hook for inline documentation in hand-edited files.

#### Scenario: theme.json selection-only schema
- **WHEN** `app::readSelectedThemeName()` parses a modern `theme.json`
- **THEN** the file SHALL be a JSON object containing `_schema_version: 2` and `selected_theme: "<string>"`
- **AND** the function SHALL return the value of `selected_theme`
- **AND** any other top-level fields (including legacy `colors`, `metrics`, `typography`) SHALL be ignored silently (the modern parser does NOT process them; the migration step at init handles legacy files)

#### Scenario: Per-theme file uses the existing schema
- **WHEN** `app::readThemeFile(path, style)` parses a per-theme JSON file
- **THEN** entries under `colors` SHALL be matched against `ImGui::GetStyleColorName(i)` for `i` in `[0, ImGuiCol_COUNT)` (unknown keys log one warning each, exactly as in the pre-change schema)
- **AND** entries under `metrics` SHALL be matched against the supported metric field set (15 scalar + 4 vec2, identical to pre-change)
- **AND** entries under `typography` SHALL be matched against `font_file` (string) + `font_size_px` (number, clamped to `[6.0, 96.0]`)
- **AND** wrong-type values SHALL log one warning per key and be skipped (existing behavior)

#### Scenario: Underscore-prefixed keys are silent everywhere
- **WHEN** any of `theme.json`, `<bundled>/<name>.json`, or `<user-dir>/<name>.json` contains an underscore-prefixed key
- **THEN** the key SHALL be skipped without log output
- **AND** the rest of the file SHALL load normally

#### Scenario: Malformed JSON in any file logs once and leaves style untouched
- **WHEN** any parser (`readSelectedThemeName` / `readThemeFile`) encounters invalid JSON
- **THEN** the function SHALL log a single error to stderr identifying the file path and parse-error location
- **AND** `style` (if applicable) SHALL be unchanged from its pre-call state
- **AND** the application SHALL continue running with the baked-in theme

### Requirement: Atomic write via temp file + rename
The application SHALL provide `app::writeThemeFile(const std::string& path, const ImGuiStyle& style)` which writes the current `style` to the given path atomically — a mid-write crash MUST NOT leave a corrupt or partially-written file at the target path. The implementation SHALL write to a temp file in the same directory as the target (`<target>.tmp.<pid>`), then rename over the target on completion. The application SHALL ALSO provide `app::writeSelectedThemeName(const std::string& name)` which atomically rewrites `theme.json` to the modern `{"_schema_version": 2, "selected_theme": "<name>"}` shape, again via temp+rename. The pre-change `app::writeThemeToConfig(style)` API SHALL be removed; callers MUST use `writeThemeFile` directly.

#### Scenario: writeThemeFile is atomic
- **WHEN** `app::writeThemeFile(path, style)` is called and the parent directory of `path` exists or can be created
- **THEN** the function SHALL create any missing parent directories via `std::filesystem::create_directories`
- **AND** SHALL open a temp file at `<path>.tmp.<pid>` (same directory as the target) for writing
- **AND** SHALL serialize the `colors / metrics / typography` blocks to the temp file
- **AND** SHALL close the temp file
- **AND** SHALL invoke `std::filesystem::rename(<temp>, <path>)` to atomically replace the target
- **AND** after the call, the file at `path` SHALL contain the full serialized style

#### Scenario: writeSelectedThemeName is atomic
- **WHEN** `app::writeSelectedThemeName("<name>")` is called
- **THEN** it SHALL write `{"_schema_version": 2, "selected_theme": "<name>"}` to `<themeConfigPath()>.tmp.<pid>` and rename over `themeConfigPath()`
- **AND** the file SHALL be valid parsable JSON after the call

#### Scenario: Write failure does not corrupt existing file
- **WHEN** any of the atomic-write APIs fails partway (out of disk space, permission denied, etc.)
- **THEN** the existing file at the target path (if any) SHALL be unchanged
- **AND** the temp file at `<target>.tmp.<pid>` MAY be left behind (cleanup is best-effort)
- **AND** the function SHALL log one error to stderr describing the failure

#### Scenario: writeThemeToConfig is no longer exposed
- **WHEN** application code is searched for `writeThemeToConfig`
- **THEN** there SHALL be no references in `app/` source files (the symbol has been removed in favor of `writeThemeFile`)
- **AND** the new caller pattern SHALL be `app::writeThemeFile(app::userThemePath(app::selectedThemeName()), style)`

### Requirement: Shipped example theme file
The application SHALL bundle theme files at `apps/imgui-shell/assets/themes/`. The v1 catalog SHALL contain at least `dark.json` (mirroring `app::applyTheme`'s curated baseline) and `light.json` (an inverted light-mode counterpart). Each bundled file SHALL be valid against the per-theme schema (`colors / metrics / typography`). The desktop build's existing post-build `assets/` copy step SHALL ensure these files appear next to the binary at `<binary-dir>/platform/desktop/assets/themes/*.json`; the iOS build SHALL include them via a new `IMGUI_SHELL_IOS_THEMES` list in `platform/ios/CMakeLists.txt` (mirroring the existing `IMGUI_SHELL_IOS_FONTS` pattern). The pre-change `example-theme.json` SHALL be replaced by these named bundled themes — `example-theme.json` is no longer shipped.

#### Scenario: Bundled themes exist at the expected build-output paths
- **WHEN** the desktop build completes
- **THEN** the paths `<binary-dir>/platform/desktop/assets/themes/dark.json` and `<binary-dir>/platform/desktop/assets/themes/light.json` SHALL exist
- **WHEN** the iOS build completes
- **THEN** the produced `.app` bundle SHALL contain `Resources/themes/dark.json` and `Resources/themes/light.json`
- **AND** the bundle SHALL NOT contain `Resources/themes/example-theme.json` (the pre-change file is removed)

#### Scenario: Bundled themes are loadable and produce expected visuals
- **WHEN** the bundled `dark.json` is selected and applied at startup
- **THEN** the application SHALL launch successfully
- **AND** the resulting `ImGuiStyle` SHALL be functionally equivalent to `applyTheme`'s curated state (within float-precision tolerance)
- **AND** no warnings SHALL be logged about unknown keys
- **WHEN** the bundled `light.json` is selected
- **THEN** the application SHALL launch successfully
- **AND** the main shell SHALL render with light backgrounds and dark text
- **AND** the accent (`SliderGrab`, `CheckMark`, etc.) SHALL remain the same cyan-blue used by `dark.json` for brand consistency
