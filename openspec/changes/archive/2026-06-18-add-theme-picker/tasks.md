## 1. ThemeStorage refactor — split read/write into selection + per-file helpers

- [x] 1.1 Rewrote `ThemeStorage.h` with the new public API surface: `readSelectedThemeName`, `writeSelectedThemeName`, `readThemeFile`, `writeThemeFile`, `migrateLegacyThemeFile`, `selectedThemeName` / `setSelectedThemeName` (cached accessor), `userThemesDir`, `userThemePath`, `listAvailableThemes` + `ThemeEntry`. Old `readThemeFromConfig` / `writeThemeToConfig` declarations removed.
- [x] 1.2 Factored the parse logic into a private `parseThemeBlocks(json, style, source)` helper in `ThemeStorage.cpp` plus a `serializeThemeBlocks(style)` mirror. Both are called from the new per-file APIs and the migration helper.
- [x] 1.3 `readSelectedThemeName()` opens `themeConfigPath()`; missing / unparseable / missing-field falls back to `"dark"`. One stderr warning on parse failure.
- [x] 1.4 `writeSelectedThemeName(name)` writes `{"_schema_version": 2, "selected_theme": "<name>"}` via the new `writeJsonAtomic(path, json)` helper. Updates the cached selection on success.
- [x] 1.5 `readThemeFile(path, style)` opens the path, parses JSON, calls `parseThemeBlocks`. Missing file = silent no-op (matches pre-change behavior).
- [x] 1.6 `writeThemeFile(path, style)` writes `serializeThemeBlocks(style)` to the path atomically. Parent dirs created via `std::filesystem::create_directories` inside `writeJsonAtomic`.
- [x] 1.7 `migrateLegacyThemeFile()` detects legacy (`!contains("_schema_version") && hasLegacyBlocks`). Writes `themes/custom.json` first; only on success rewrites `theme.json` to the modern shape. Logs one stderr line on completion.
- [x] 1.8 Added `userThemesDir()` (parent of `themeConfigPath()` + `/themes`) and `userThemePath(name)` accessors. Public in `ThemeStorage.h`.
- [x] 1.9 Removed `readThemeFromConfig` / `writeThemeToConfig`. Verified by grep — no references remain in `app/` source. Old definitions deleted.
- [x] 1.10 Clean build confirms type-system caught remaining callers (had to forward-include `<imgui.h>` in `App.h` for the new `applySelectedThemeToStyle(ImGuiStyle&)` declaration — added during the build-fix loop).

## 2. Bundled themes — `assets/themes/dark.json` + `light.json`

- [x] 2.1 Renamed `apps/imgui-shell/assets/themes/example-theme.json` → `dark.json` (the values already match `applyTheme`'s curated baseline). Also removed leftover `example-theme.json` from both `build/macos/.../assets/themes/` and `build/macos-stb/.../assets/themes/` since CMake's asset-copy step is additive.
- [x] 2.2 `dark.json`'s `_comment` updated to identify it as the curated baseline. No color / metric / typography values changed.
- [x] 2.3 Created `apps/imgui-shell/assets/themes/light.json`. Hand-tuned light-mode color palette: off-white backgrounds, near-black text, mid-gray borders, same cyan-blue accent (#64BEF0) used in dark for brand consistency. Metrics + typography blocks identical to `dark.json` (paddings, roundings, font stay the same across themes).
- [x] 2.4 Verified `<binary-dir>/platform/desktop/assets/themes/` contains exactly `dark.json` + `light.json` after a clean build (FreeType + stb-fallback both verified).

## 3. Theme listing — `app::listAvailableThemes()`

- [x] 3.1 Declared `struct ThemeEntry { name; path; fromUserDir; }` + `std::vector<ThemeEntry> listAvailableThemes()` in `ThemeStorage.h`.
- [x] 3.2 Implemented `listAvailableThemes()` via `std::map<string, ThemeEntry>` populated by two `directory_iterator` passes: bundled first (`resolveAssetPath("themes")`), then user dir (`userThemesDir()`). User-dir entries overwrite bundled by name. Result returned as alphabetically-sorted `std::vector` (`std::map` is sorted).
- [x] 3.3 Filter `path.extension() == ".json"` excludes `*.json.tmp.<pid>` atomic-write temp files automatically (their extension is `.<pid>`).
- [x] 3.4 Missing user dir is silent no-op. Missing bundled dir logs one warning; both missing logs the "no themes found" line.

## 4. App.cpp boot wiring — migrate + read selection + apply selected theme

- [x] 4.1 Declared `applySelectedThemeToStyle(ImGuiStyle&)`, `switchTheme(const std::string&)`, `resetSelectedTheme()` in `App.h`. Required forward-including `<imgui.h>` in `App.h`.
- [x] 4.2 `applySelectedThemeToStyle(style)` reads selection, enumerates `listAvailableThemes`, resolves by name. Falls back to `"dark"` (logging once) if the named theme isn't available. If `dark` is also unavailable, logs and leaves the in-memory style as-is (curated baseline from prior `applyTheme`).
- [x] 4.3 `switchTheme(name)` writes the new selection then calls `applySelectedThemeToStyle(ImGui::GetStyle())` — instant live repaint.
- [x] 4.4 `app::init` now runs `StyleColorsDark → applyTheme → migrateLegacyThemeFile → applySelectedThemeToStyle`, replacing the pre-change `readThemeFromConfig` step.
- [x] 4.5 `selectedThemeName()` cached accessor in `ThemeStorage.cpp` — written by `readSelectedThemeName` (via `setSelectedThemeName`) and updated by `switchTheme` / migration. Avoids per-frame disk reads from `Preferences.cpp`.

## 5. Preferences picker UI — Combo at top of Theme tab

- [x] 5.1 Added file-scope `g_themes` (`std::vector<app::ThemeEntry>`) + `g_themePickerIndex` (`int`) in `Preferences.cpp`. Refreshed by `refreshThemeList()` each frame the Theme tab renders.
- [x] 5.2 At the top of `renderThemeTab()`, before `BeginTable`, render `"Theme:"` label + `ImGui::Combo("##theme", ...)` with width 220. Combo items built as `const char*[]` from `g_themes[i].name.c_str()`.
- [x] 5.3 On Combo selection-change (`if (ImGui::Combo(...))` true), call `app::switchTheme(g_themes[g_themePickerIndex].name)`. The main shell repaints immediately.
- [x] 5.4 `refreshThemeList()` re-syncs `g_themePickerIndex` from `app::selectedThemeName()` (defaults to 0 if not found).
- [x] 5.5 Table size left as `ImVec2(0, -buttonRowHeight)` — the picker row above is drawn FIRST, so ImGui's cursor is already past it when `BeginTable` runs. Reserving extra `-pickerHeight` would over-shrink. Verified visually that the layout (picker → table → Reset button) renders without overlap.
- [ ] 5.6 (Optional polish) `*` suffix on user-dir-override entries — skipped for v1; the picker is clean without it.

## 6. Persist target retargeting — `persistTheme()` writes to selected theme's user file

- [x] 6.1 `persistTheme()` now calls `app::writeThemeFile(app::userThemePath(app::selectedThemeName()), ImGui::GetStyle())`.
- [x] 6.2 Verified by code review — every commit branch in `renderThemeRightPane` (the 5 `IsItemDeactivatedAfterEdit` calls + the font Combo's `if (Combo(...))` branch) routes through `persistTheme()`. No bypasses.
- [x] 6.3 Copy-on-write semantics fall out automatically: `writeThemeFile` creates parent dirs + the target file if absent; bundled `assets/themes/` is never targeted.

## 7. Reset semantic change — `resetSelectedTheme()`

- [x] 7.1 Implemented `app::resetSelectedTheme()` in `App.cpp`: looks up bundled path via `resolveAssetPath("themes/<selected>.json")`. If bundled doesn't exist, logs `Reset: '<name>' has no bundled origin; no action taken` and returns. Otherwise: `std::filesystem::remove(userThemePath(name))` + `readThemeFile(bundledPath, ImGui::GetStyle())`.
- [x] 7.2 Preferences.cpp's `Reset to defaults` button now calls `app::resetSelectedTheme()` (was `app::applyTheme + persistTheme`). Button label unchanged.
- [x] 7.3 Spec walkthrough: both scenarios (bundled-origin reset + user-only-theme no-op) match the implementation's branches.

## 8. iOS bundle list — `IMGUI_SHELL_IOS_THEMES`

- [x] 8.1 `platform/ios/CMakeLists.txt`'s `IMGUI_SHELL_IOS_THEMES` updated to include `dark.json` + `light.json` (replacing the old `example-theme.json` entry). `MACOSX_PACKAGE_LOCATION` still `Resources/themes`; both files will land in the .app bundle on a real iOS build.
- [ ] 8.2 iOS runtime verification deferred (still blocked on full Xcode availability per `add-typography-persistence` task 6.12). Source-level addition is in place.

## 9. Verification & spec validation

- [x] 9.1 macOS Vulkan + FreeType clean rebuild: zero warnings, zero errors.
- [x] 9.2 macOS stb-fallback rebuild (`build/macos-stb/`): zero warnings, zero errors.
- [x] 9.3 **Migration test (automated):** ran the binary against the existing legacy-shape `theme.json` (had top-level `colors` block, no `_schema_version`). Stderr printed exactly: `[imgui-shell] migrated legacy theme.json to themes/custom.json (selected_theme: custom)`. After 3s the app exited cleanly. Verified `~/.config/imgui-shell/themes/custom.json` contains the migrated `colors / metrics / typography` blob; `~/.config/imgui-shell/theme.json` is now `{"_schema_version": 2, "selected_theme": "custom"}`.
- [x] 9.4 **Selection persistence test (automated):** wrote `theme.json` with `selected_theme: "light"` and relaunched. App survived 3s with silent stderr — bundled `light.json` was found and applied successfully (no fallback-to-dark warning).
- [ ] 9.5 **Copy-on-write test (interactive — PENDING USER):** select bundled `dark` (no user-dir copy), edit any color, confirm `~/.config/imgui-shell/themes/dark.json` is created with the edit and the bundled file under `build/macos/platform/desktop/assets/themes/dark.json` stays byte-identical.
- [ ] 9.6 **Reset test (interactive — PENDING USER):** with user-dir `dark.json` in place, click `Reset to defaults`; confirm `~/.config/imgui-shell/themes/dark.json` is deleted and the main shell repaints with the bundled `dark` baseline.
- [ ] 9.7 **Picker switching test (interactive — PENDING USER):** open Preferences > Theme tab, pick `light` from the Combo. Main shell repaints with the light palette. `theme.json` updates to `selected_theme: "light"`. Close + relaunch; the light theme persists.
- [x] 9.8 `openspec validate add-theme-picker --type change --strict` ⇒ "Change 'add-theme-picker' is valid".
- [x] 9.9 Spec walkthrough — every requirement in the three delta spec files maps to specific code:
  - `theme-presets` ADDED — `ThemeStorage.cpp`'s `listAvailableThemes` / `userThemePath` / `userThemesDir` / `migrateLegacyThemeFile` + `App.cpp`'s `applySelectedThemeToStyle` / `switchTheme` / `resetSelectedTheme` + asset files `dark.json` / `light.json` + Preferences.cpp picker row.
  - `preferences-dialog` MODIFIED — picker row + retargeted `persistTheme` + `resetSelectedTheme` wiring in `Preferences.cpp`.
  - `theme-persistence` MODIFIED — new ThemeStorage API (`readThemeFile` / `writeThemeFile` / `read+writeSelectedThemeName` / `migrateLegacyThemeFile`) + bundled `dark.json` / `light.json` shipped in place of `example-theme.json`.
