## Why

The `gui-theme` capability gives the shell a curated "imgui-shell dark" palette baked into `Theme.cpp` — but every tweak (different accent hue, wider padding, slightly larger font) requires editing source and rebuilding. The parked `add-preferences-dialog` change wants to expose runtime theme editing via a Preferences window, but that UI can't ship usefully without somewhere to persist the user's tweaks. This sub-change adds the persistence layer — a small JSON file at a per-OS config path that `app::init` reads after `applyTheme`, plus a `writeThemeToConfig()` API the future Preferences-window Save button will call.

This is sub-change **2 of 3** from the parked `add-preferences-dialog`. Sub-change 1 (`enable-multi-viewport`) is archived; this one adds persistence with no UI; sub-change 3 (`add-preferences-dialog`, still parked) wires the dialog on top.

## What Changes

- **Add `nlohmann/json` v3.11.3** to `cmake/Dependencies.cmake` via `FetchContent` (header-only, MIT-licensed — same pattern as ImGui + GLFW + FreeType already use). The dep is linked into `imgui_shell_app` (PUBLIC) since the persistence code lives in the shared core.
- **New `app/ThemeStorage.{h,cpp}`** with three public functions:
  - `std::string app::themeConfigPath()` — returns the absolute path to the theme.json file for the current platform. macOS / Linux: `${XDG_CONFIG_HOME:-~/.config}/imgui-shell/theme.json`. Windows: `%APPDATA%/imgui-shell/theme.json`. iOS: `<bundle-Documents>/theme.json` (sourced from a new `app::g_documentsPath` set by the iOS host).
  - `void app::readThemeFromConfig(ImGuiStyle& style)` — opens the file if it exists, parses the JSON, applies known `ImGuiCol_*` colors + metric values to `style`. Missing file → silent no-op. Malformed JSON → logs one error to stderr, leaves `style` untouched. Unknown `ImGuiCol_*` name or metric key → logs one warning per unknown key to stderr (developer-friendly, lenient continuation).
  - `void app::writeThemeToConfig(const ImGuiStyle& style)` — serializes the full set of `ImGuiCol_*` slots the `gui-theme` capability cares about (the same set `applyTheme` overrides) plus all the metric fields, atomically via temp-file-then-rename. Creates the parent directory if missing.
- **Wire `readThemeFromConfig` into `app::init`** immediately after `applyTheme(GetStyle())`. The order is: `StyleColorsDark` → `applyTheme` (baked defaults) → `readThemeFromConfig` (user overrides). Result: user-saved values take precedence; users with no saved file see the unchanged baked-in theme.
- **New `app::setDocumentsPath(const char*)` API** in `App.h` — sibling to the existing `app::setBundleResourcePath`. The iOS host calls it from `ViewController::viewDidLoad` with `NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, ...)[0]`. On desktop, this API is a no-op (the desktop `themeConfigPath` uses environment variables, not the bundle path). Sub-change-2-only: we don't yet WIRE the iOS host to call it — that's a one-liner that lands in `add-preferences-dialog` once we actually need iOS theme persistence.
- **Ship `apps/imgui-shell/assets/themes/example-theme.json`** with the same color + metric values that `Theme.cpp::applyTheme` sets, plus an opening top-of-file comment explaining the JSON schema. Contributors can copy this to their config path to see persistence working, or modify it to test custom themes before the Preferences UI lands.
- **No UI surface in this sub-change.** The Preferences dialog Save button is sub-change 3. Until then, `writeThemeToConfig` is reachable only via direct API call (e.g., from a test or a future hotkey); reading the config happens automatically on every launch.

Non-goals (called out to bound scope):
- **No Preferences dialog.** That's sub-change 3.
- **No multi-theme support.** One theme.json file, one persisted style. Switching to a "Light" preset is a future change.
- **No theme import/export.** No drag-and-drop, no command-line `--load-theme=path`.
- **No iOS runtime verification.** Code path exists with the `setDocumentsPath` plumbing, but the iOS host's actual call to it lands in `add-preferences-dialog` (where the Save button gives it a reason to exist). Until then, iOS uses baked-in `applyTheme` defaults only.
- **No JSON schema versioning.** The schema is flat enough that we can add fields without breaking old files (missing keys → keep in-memory values).
- **No background save / auto-write on theme mutation.** Save is explicit (via the future Preferences button); reads are init-only.
- **No undo/redo.** The persistence layer doesn't track history.

## Capabilities

### New Capabilities

- `theme-persistence`: The JSON schema for serializing `ImGuiStyle` (the slots + metrics the `gui-theme` capability owns), the per-OS path-resolver, read-on-init semantics, write-on-explicit-call semantics, and the unknown-key warning behavior. Owns "where the file lives, what it contains, how it's read, how it's written" — distinct from `gui-theme` (which owns "what the theme IS") and from the future `preferences-dialog` (which will own "the UI that triggers Save").

### Modified Capabilities

- `build-system`: New requirement — nlohmann/json is fetched via `FetchContent` in `cmake/Dependencies.cmake`, pinned to `v3.11.3`, and linked into `imgui_shell_app` PUBLIC.
- `app-shell`: A new minor scenario is added to the existing `Platform host responsibilities` requirement — on iOS, the host SHALL eventually call `app::setDocumentsPath` (the call site lands in `add-preferences-dialog`; this change just adds the API and notes the contract).

## Impact

- **Code change:** ~150–200 LOC net.
  - `apps/imgui-shell/app/ThemeStorage.h` — public API declarations (~30 LOC).
  - `apps/imgui-shell/app/ThemeStorage.cpp` — read/write/path-resolve implementation (~150 LOC, the bulk of the change).
  - `apps/imgui-shell/app/App.cpp` — one call to `readThemeFromConfig` in `init` + new `setDocumentsPath` setter (~10 LOC).
  - `apps/imgui-shell/app/App.h` — declaration of `setDocumentsPath` (~3 LOC).
  - `apps/imgui-shell/app/CMakeLists.txt` — add `ThemeStorage.{h,cpp}` to the library (~2 LOC).
  - `apps/imgui-shell/cmake/Dependencies.cmake` — add `FetchContent_Declare(nlohmann_json ...)` + `target_link_libraries(imgui_shell_app PUBLIC nlohmann_json::nlohmann_json)` (~10 LOC).
  - `apps/imgui-shell/assets/themes/example-theme.json` — bundled example with the baked-in values (~80 LOC of JSON).
- **New dependency:** `nlohmann/json` v3.11.3 (header-only, MIT, ~26000 LOC of header). Build time grows by ~10–20s on first configure; runtime adds nothing.
- **Spec change:** new `openspec/specs/theme-persistence/spec.md`; MODIFIED `build-system` (ADD a requirement for the JSON dep); MODIFIED `app-shell` (one new scenario in `Platform host responsibilities` for the iOS `setDocumentsPath` setter).
- **CI:** no workflow changes. nlohmann/json compiles in seconds on every preset.
- **Risk surface:**
  - **macOS Sandbox / Linux XDG path edge cases** — `~/.config/imgui-shell/` may not exist on first run; `themeConfigPath()` creates parents during `writeThemeToConfig`. Read path is tolerant of missing file.
  - **JSON parse errors corrupting the user's session** — `readThemeFromConfig` catches `nlohmann::json::parse_error`, logs once to stderr, leaves the in-memory `ImGuiStyle` untouched. The application continues with the baked-in theme.
  - **Concurrent writes** — out of scope. Single-process app; no second writer.
- **Backward compatibility:** new feature; no prior persisted state. Future schema changes can add fields freely (missing keys → keep in-memory values).
