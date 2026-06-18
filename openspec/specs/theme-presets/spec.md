# theme-presets Specification

## Purpose
The theme-presets capability manages a catalog of bundled and user-created theme files, enabling runtime theme selection via a UI picker.

## Requirements
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
-C- **AND** loading `dark` SHALL parse the user-dir file
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
-( **THEN** `listAvailableThemes()` SHALL return only the bundled entries
- **AND** SHALL NOT log a warning

#### Scenario: Missing bundled dir falls back to user dir only
To be continued...