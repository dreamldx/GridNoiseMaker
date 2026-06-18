# theme-persistence Specification

## Purpose
TBD - created by archiving change add-theme-persistence. Update Purpose after archive.
## Requirements
### Requirement: Theme persisted to per-OS config file
The application SHALL persist user-tunable theme values (`ImGuiStyle::Colors` slots + `ImGuiStyle` metric fields) to a JSON file at a per-OS platform-conventional path. The file SHALL be read at `app::init` time after `app::applyTheme`, so user-saved values override the baked-in curated theme. The file SHALL be written only via an explicit `app::writeThemeToConfig` API call — there SHALL be no background auto-save and no save-on-frame mechanism in v1.

#### Scenario: Read order — user overrides curated overrides ImGui defaults
- **WHEN** `app::init` runs in any build
- **THEN** the init sequence SHALL execute in exactly this order: `ImGui::StyleColorsDark(&style)` → `app::applyTheme(style)` → `app::readThemeFromConfig(style)`
- **AND** after the sequence, any slot the user has saved in the config file SHALL equal the saved value
- **AND** any slot the user has NOT saved but `applyTheme` sets SHALL equal the curated theme value
- **AND** any slot neither the user nor `applyTheme` touched SHALL equal the `StyleColorsDark` default (forward-compatible with new ImGui slots in future releases)

#### Scenario: Missing config file is a silent no-op
- **WHEN** `app::readThemeFromConfig` is called and the file does not exist at `app::themeConfigPath()`
- **THEN** the function SHALL return immediately without modifying `style`
- **AND** no error message SHALL be logged to stderr

### Requirement: Per-OS config path resolver
The application SHALL provide `app::themeConfigPath()` which returns an absolute filesystem path for the theme config file, selected per-platform via the `kIsDesktop` / `kIsMobile` (and new `kIsWindows`) constexpr traits from `app/Platform.h` — no `#ifdef IMGUI_SHELL_PLATFORM_*` blocks in source files.

#### Scenario: macOS / Linux path uses XDG_CONFIG_HOME with HOME fallback
- **WHEN** the application runs on macOS or Linux
- **THEN** `app::themeConfigPath()` SHALL return `${XDG_CONFIG_HOME}/imgui-shell/theme.json` if `XDG_CONFIG_HOME` is set and non-empty
- **AND** otherwise SHALL return `${HOME}/.config/imgui-shell/theme.json`
- **AND** if `HOME` is also unset, SHALL return `./.config/imgui-shell/theme.json` and log a warning about the missing env var

#### Scenario: Windows path uses APPDATA
- **WHEN** the application runs on Windows
- **THEN** `app::themeConfigPath()` SHALL return `${APPDATA}/imgui-shell/theme.json`
- **AND** if `APPDATA` is unset, SHALL return `./AppData/imgui-shell/theme.json` and log a warning

#### Scenario: iOS path uses app::g_documentsPath
- **WHEN** the application runs on iOS
- **THEN** `app::themeConfigPath()` SHALL return `${documentsPath}/theme.json` where `documentsPath` is the value last passed to `app::setDocumentsPath`
- **AND** if `app::setDocumentsPath` was never called, SHALL return `./theme.json` and log a warning

### Requirement: JSON schema and parsing rules
The theme config file SHALL be a single top-level JSON object with two optional keys: `colors` (object: `ImGuiCol_*` enum name without prefix → 4-element float array `[r, g, b, a]` in 0..1 range) and `metrics` (object: `ImGuiStyle` metric field name → either a single float or 2-element float array `[x, y]` depending on whether the field is scalar or `ImVec2`). Any key inside `colors` or `metrics` whose name begins with underscore (`_`) SHALL be silently ignored at load time — this provides a hook for inline documentation in hand-edited files.

#### Scenario: Colors are mapped via ImGui's canonical enum names
- **WHEN** `app::readThemeFromConfig` parses a `"colors"` key
- **THEN** each entry's key SHALL be matched against `ImGui::GetStyleColorName(i)` for `i` in `[0, ImGuiCol_COUNT)`
- **AND** known keys SHALL have their corresponding `style.Colors[i]` set to the parsed `ImVec4`
- **AND** unknown color keys SHALL each log one warning of the form `theme.json: unknown color key '<name>'; ignoring` and SHALL NOT modify `style`

#### Scenario: Metrics use a fixed set of supported field names
- **WHEN** `app::readThemeFromConfig` parses a `"metrics"` key
- **THEN** entries SHALL be matched against the supported metric field set (at minimum: `WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`, `IndentSpacing`, `ScrollbarSize`, `GrabMinSize`, `WindowRounding`, `ChildRounding`, `FrameRounding`, `GrabRounding`, `PopupRounding`, `ScrollbarRounding`, `TabRounding`, `WindowBorderSize`, `ChildBorderSize`, `PopupBorderSize`, `FrameBorderSize`, `TabBorderSize`)
- **AND** known metric keys SHALL have their corresponding field on `style` set to the parsed value
- **AND** unknown metric keys SHALL each log one warning of the form `theme.json: unknown metric key '<name>'; ignoring`

#### Scenario: Underscore-prefixed keys are silent
- **WHEN** the config file contains a key like `"_comment"`, `"_note"`, or any other underscore-prefixed name inside `colors` or `metrics`
- **THEN** the key SHALL be skipped without any log output
- **AND** the rest of the file SHALL load normally

#### Scenario: Malformed JSON logs once and leaves style untouched
- **WHEN** `app::readThemeFromConfig` cannot parse the file as valid JSON
- **THEN** the function SHALL log a single error to stderr identifying the parse-error location
- **AND** `style` SHALL be unchanged from its pre-call state
- **AND** the application SHALL continue running with the baked-in theme (no crash, no exit)

#### Scenario: Wrong-type values are skipped per key
- **WHEN** a color value is not a 4-element array of numbers, or a metric value is not a number (or 2-element array for `ImVec2` fields)
- **THEN** the offending key SHALL log one warning identifying the key + expected type
- **AND** the rest of the file SHALL continue loading

### Requirement: Atomic write via temp file + rename
The application SHALL provide `app::writeThemeToConfig(const ImGuiStyle&)` which writes the current `style` to the config path atomically — a mid-write crash MUST NOT leave a corrupt or partially-written file at the target path. The implementation SHALL write to a temp file in the same directory as the target, then rename over the target on completion.

#### Scenario: Successful write is atomic
- **WHEN** `app::writeThemeToConfig(style)` is called and the parent directory of `app::themeConfigPath()` exists or can be created
- **THEN** the function SHALL create any missing parent directories
- **AND** SHALL open a temp file at `<config-path>.tmp.<pid>` (same directory as the target) for writing
- **AND** SHALL serialize the `colors` + `metrics` objects to the temp file
- **AND** SHALL close the temp file
- **AND** SHALL invoke `std::filesystem::rename(<temp>, <target>)` to atomically replace the target
- **AND** after the call, `app::themeConfigPath()` SHALL contain the full serialized style

#### Scenario: Write failure does not corrupt existing file
- **WHEN** `app::writeThemeToConfig(style)` is called and the write fails partway (out of disk space, permission denied, etc.)
- **THEN** the existing file at `app::themeConfigPath()` (if any) SHALL be unchanged
- **AND** the temp file at `<target>.tmp.<pid>` MAY be left behind (cleanup is best-effort)
- **AND** the function SHALL log one error to stderr describing the failure

### Requirement: Shipped example theme file
The application SHALL bundle one example theme file at `apps/imgui-shell/assets/themes/example-theme.json` containing the same color + metric values that `app::applyTheme` sets, prefixed with an underscore-prefixed `_comment` key in each section explaining the schema. The file SHALL be copied next to the desktop binary by the existing post-build asset-copy step, and bundled into the iOS `.app` Resources/themes/ directory.

#### Scenario: Example exists at the expected build-output paths
- **WHEN** the desktop build completes
- **THEN** the path `<binary-dir>/platform/desktop/assets/themes/example-theme.json` SHALL exist
- **WHEN** the iOS build completes
- **THEN** the produced `.app` bundle SHALL contain `Resources/themes/example-theme.json`

#### Scenario: Example contents are loadable
- **WHEN** the example file is copied to `app::themeConfigPath()` and the application is launched
- **THEN** the application SHALL launch successfully
- **AND** the resulting `ImGuiStyle` SHALL be functionally equivalent to the post-`applyTheme` state (within float-precision tolerance)
- **AND** no warnings SHALL be logged about unknown keys

### Requirement: Documents-path setter for iOS
The application SHALL expose `app::setDocumentsPath(const char*)` so the iOS host can supply the bundle's NSDocumentDirectory path at startup. Desktop hosts SHALL NOT call this API (it is a no-op on desktop since `themeConfigPath` uses environment variables); the API exists in shared `app/` code so iOS uses it without `#ifdef` gating at the call site.

#### Scenario: iOS-only setter does not affect desktop path
- **WHEN** the desktop host runs without calling `app::setDocumentsPath`
- **THEN** `app::themeConfigPath()` SHALL return the XDG / APPDATA / HOME-based path per the platform branch
- **AND** the documents-path string SHALL be unused by the desktop path resolver

#### Scenario: setDocumentsPath is called before app::init on iOS (contract)
- **WHEN** the iOS host's `ViewController::viewDidLoad` runs (or equivalent entry point)
- **THEN** `app::setDocumentsPath([[NSBundle.mainBundle].resourcePath UTF8String])` (or the documents-dir-equivalent path) SHOULD be called before `app::init`
- **AND** if the call is missing, `app::themeConfigPath()` SHALL fall back to `./theme.json` and log a warning (per the iOS path scenario above)

