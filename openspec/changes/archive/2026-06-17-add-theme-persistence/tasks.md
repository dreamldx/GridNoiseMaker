## 1. Dependency: nlohmann/json

- [x] 1.1 In `apps/imgui-shell/cmake/Dependencies.cmake`, added a `FetchContent_Declare(nlohmann_json ... GIT_TAG v3.11.3 GIT_SHALLOW TRUE)` block plus `FetchContent_MakeAvailable(nlohmann_json)`. Pre-set `JSON_BuildTests OFF` + `JSON_Install OFF` to skip upstream tests/installs.
- [x] 1.2 In `apps/imgui-shell/app/CMakeLists.txt`, `target_link_libraries(imgui_shell_app PUBLIC imgui nlohmann_json::nlohmann_json)`.
- [x] 1.3 Configure reported the fetch step succeeding (14.2s configure including the fetch); build completed cleanly. ThemeStorage.cpp compiles and links against the nlohmann/json target.

## 2. Platform.h gets `kIsWindows` / `kIsMacOS` / `kIsLinux` traits

- [x] 2.1 In `apps/imgui-shell/app/Platform.h`, added `kIsWindows` / `kIsMacOS` / `kIsLinux` constexpr bools inside the existing `IMGUI_SHELL_PLATFORM_DESKTOP` branch, refined via `_WIN32` / `__APPLE__` / else predefines. iOS branch sets all three to `false`. Header-only — the rule "no `#ifdef IMGUI_SHELL_PLATFORM_*` in source files" remains intact.
- [x] 2.2 No CMake change needed — `_WIN32` / `__APPLE__` are compiler-provided.

## 3. ThemeStorage module

- [x] 3.1 Created `apps/imgui-shell/app/ThemeStorage.h` declaring `themeConfigPath()`, `readThemeFromConfig`, `writeThemeToConfig` with the prerequisite documented in the header comment.
- [x] 3.2 Created `apps/imgui-shell/app/ThemeStorage.cpp` with all required includes (Platform.h, App.h for setDocumentsPath decl, nlohmann/json, filesystem, fstream, cstdio, cstdlib, plus `process.h` on Windows / `unistd.h` elsewhere gated via `_WIN32` predef per design.md D4).
- [x] 3.3 `themeConfigPath()` implemented via `if constexpr` over `kIsMobile` / `kIsWindows` / desktop-POSIX. Null-safe `envOrEmpty()` helper for getenv. XDG_CONFIG_HOME → HOME-based fallback → `./.config` warning on macOS/Linux; APPDATA → `./AppData` warning on Windows; `g_documentsPath` → `./theme.json` warning on iOS.
- [x] 3.4 `readThemeFromConfig` implemented per spec scenarios — `is_regular_file` check + silent return on missing; `nlohmann::json::parse(allow_exceptions=false)` + `is_discarded()` check + one error log + bail on malformed; root must be object check; iterate `colors` (matching `GetStyleColorName`) and `metrics` (matching scalar + ImVec2 tables); underscore-prefix-ignored; unknown keys log one warning each; wrong-type values log one warning each.
- [x] 3.5 `writeThemeToConfig` implemented atomically — `create_directories(parent_path)`, write to `<target>.tmp.<pid>` (using `_getpid()` on Windows via `<process.h>`, `getpid()` elsewhere via `<unistd.h>`), `std::filesystem::rename` to swap atomically, try/catch around the whole sequence with single-error log on failure.
- [x] 3.6 Two static metric-name tables (`kScalarMetrics[15]` scalar floats, `kVec2Metrics[4]` ImVec2 fields) using member pointers — single source of truth used by both read and write paths.
- [x] 3.7 `ThemeStorage.h` and `ThemeStorage.cpp` added to the `imgui_shell_app` static library sources in `app/CMakeLists.txt`.

## 4. App.cpp wiring

- [x] 4.1 Added `#include "ThemeStorage.h"` at the top of `App.cpp`.
- [x] 4.2 In `app::init()`, inserted `app::readThemeFromConfig(ImGui::GetStyle());` after `applyTheme(...)` and before the multi-viewport flag set / `configureFontAtlas()`. Init sequence is now: `CreateContext` → `StyleColorsDark` → `applyTheme` → `readThemeFromConfig` → `kIsDesktop ? ViewportsEnable` → `configureFontAtlas`.
- [x] 4.3 In `App.h`, declared `void setDocumentsPath(const char* path);` next to the existing `setBundleResourcePath` declaration with explanatory doc-comment.
- [x] 4.4–4.5 Combined: `g_documentsPath` static and `setDocumentsPath` implementation both live in `ThemeStorage.cpp`'s anonymous namespace (cleaner than splitting across App.cpp — the only consumer is `themeConfigPath`, which is also in ThemeStorage.cpp). App.h declares the symbol; ThemeStorage.cpp owns both ends.

## 5. Example theme asset

- [x] 5.1 Created `apps/imgui-shell/assets/themes/example-theme.json` with the same color + metric values that `Theme.cpp::applyTheme` sets. Two `_comment` entries (one each in `colors` and `metrics`) explain the schema.
- [x] 5.2 Existing post-build `copy_directory ${CMAKE_SOURCE_DIR}/assets → $<TARGET_FILE_DIR:imgui_shell_desktop>/assets` step picked up the new `themes/` subdirectory automatically (verified: `build/macos/platform/desktop/assets/themes/example-theme.json` exists).
- [x] 5.3 In `platform/ios/CMakeLists.txt`, added a `IMGUI_SHELL_IOS_THEMES` source-files-properties block bundling `example-theme.json` into `Resources/themes` of the iOS `.app`.
- [x] 5.4 Manual-sync responsibility (`Theme.cpp` ↔ `example-theme.json`) is documented in design.md D6 + the open-question list. Auto-generation deferred. **Caught during verification:** the initial example used 4 obsolete ImGuiCol_* names (`NavHighlight`, `TabActive`, `TabUnfocused`, `TabUnfocusedActive`) — ImGui 1.91.9-docking renamed these to `NavCursor`, `TabSelected`, `TabDimmed`, `TabDimmedSelected`. Theme.cpp's enum-literal references still work via ImGui's backward-compat aliases; the example.json names were updated to match what `ImGui::GetStyleColorName` returns now.

## 6. Verification

- [x] 6.1 Clean rebuild on macOS: succeeded (11 build steps; nlohmann/json fetched + integrated; ThemeStorage.cpp compiled; no warnings, no link errors).
- [x] 6.2 Stb fallback rebuild (`build/macos-stb/`): succeeded.
- [x] 6.3 Validation-layer smoke (no config file): launched binary for 3s under validation; stderr silent — `readThemeFromConfig` returned immediately because `is_regular_file(~/.config/imgui-shell/theme.json)` returned false.
- [x] 6.4 Load test (corrected example as config): copied `assets/themes/example-theme.json` → `~/.config/imgui-shell/theme.json`, relaunched, 3s under validation. After the obsolete-ImGuiCol-name fix, stderr is fully silent — the load round-trips cleanly through `applyTheme` → `readThemeFromConfig` overrides → identical result.
- [ ] 6.5 **Interactive override test (REQUIRED before archive)**: edit `~/.config/imgui-shell/theme.json` to set `WindowBg` to `[1.0, 0.0, 0.0, 1.0]`. Relaunch. Confirm the menu-bar background is RED — proves `readThemeFromConfig` runs AFTER `applyTheme` and user values take precedence. **PENDING USER INTERACTION (visual confirmation only).**
- [x] 6.6 Unknown-key warning test: injected `"WindowBgFake": [1,0,0,1]` into the config file's `colors` object. Launched; stderr contained exactly one line `[imgui-shell] theme.json: unknown color key 'WindowBgFake'; ignoring`. The rest of the file loaded; binary survived 3s cleanly.
- [x] 6.7 Malformed-JSON test: wrote `{ "colors": ` (truncated) to the config file. Launched; stderr contained exactly one line `[imgui-shell] theme.json parse failed at '...'; skipping`; app continued for 3s with the baked-in theme; clean shutdown.
- [ ] 6.8 Atomic-write smoke test (round-trip): requires temporarily wiring `writeThemeToConfig` to a debug hotkey or test main. Code path exists and follows design.md D4 exactly (temp file + rename); not exercised in this apply session. **DEFERRED — will be exercised end-to-end when `add-preferences-dialog`'s Save button calls `writeThemeToConfig`.**
- [x] 6.9 Walked through `specs/theme-persistence/spec.md` scenarios — each maps to a concrete code location verified above (read-order via App.cpp init sequence; per-OS path via `themeConfigPath`'s `if constexpr` branches; missing/malformed/unknown handled per scenario 6.3/6.7/6.6; JSON schema rules via the metric tables + `GetStyleColorName` mapping; atomic write via temp + rename in `writeThemeToConfig`; example file at correct paths; setDocumentsPath desktop no-op semantics).
- [x] 6.10 `openspec validate add-theme-persistence --type change --strict` ⇒ "Change 'add-theme-persistence' is valid".
- [ ] 6.11 iOS bundle verification — **DEFERRED: requires full Xcode**; code path exists, runtime verification waits for sub-change 3 (`add-preferences-dialog`) and Xcode availability.
