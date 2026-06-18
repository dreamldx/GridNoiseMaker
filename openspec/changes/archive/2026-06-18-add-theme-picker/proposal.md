## Why

After `add-preferences-dialog` shipped, the user has exactly one editable theme — every color / metric / typography tweak edits the same in-memory style and autosaves to a single on-disk file. To compare looks, switch between aesthetic moods (e.g., dark for night work, light for sunlit windows), or roll back to a known-good preset, there's no affordance. The user must edit values one-at-a-time and there's no way to bookmark a configuration.

This change introduces *named themes*: a small catalog of bundled theme files under `assets/themes/`, a picker in the Preferences Theme tab that lists them, persisted selection so the chosen theme survives relaunches, and autosave-into-the-selected-file semantics so per-theme edits stay with that theme.

## What Changes

- **Bundled themes catalog.** New `apps/imgui-shell/assets/themes/` directory ships with at least two theme files in the existing `theme.json` schema (colors / metrics / typography):
  - `dark.json` — the current curated baseline from `app::applyTheme`.
  - `light.json` — an inverted light-mode counterpart (backgrounds light, text dark, same accent hue).
  - Room to add `high-contrast.json` and `imgui-default.json` in follow-ups; v1 ships at least dark + light.
- **Theme-picker UI in Preferences.** A new `ImGui::Combo` (or List) widget at the TOP of the Theme tab, above the master-detail editor, labeled "Theme". Lists every available theme — both bundled and user copies (see below). Selecting an entry reads that theme's JSON, applies it to `ImGui::GetStyle()` live, and updates the selection state.
- **User theme dir.** Edits do not write back into the read-only bundle. Instead, the first edit of a bundled theme creates a writable copy at `<config-dir>/imgui-shell/themes/<name>.json` with the current values; subsequent edits autosave to that user copy. The picker lists `assets/themes/*.json` ∪ `<config-dir>/imgui-shell/themes/*.json` (user-dir copy wins on name collision).
- **Selection persistence.** A new `selected_theme: "<name>"` field at the top of `<config-dir>/imgui-shell/theme.json`. Existing `colors` / `metrics` / `typography` blocks are removed from `theme.json` — those values now live inside the per-theme files. **BREAKING (single-developer-acceptable):** an existing `theme.json` from `add-preferences-dialog` becomes effectively a stale snapshot; one-time migration on first launch copies its blob into `<config-dir>/imgui-shell/themes/custom.json`, sets `selected_theme: "custom"`, and strips the inline blocks from `theme.json`.
- **Apply on startup.** `app::init`'s existing `readThemeFromConfig(style)` call is generalized: read `selected_theme` from `theme.json`, look up that theme's file (user-dir first, fall back to bundled), apply its content via the existing JSON-parsing path. If no `theme.json` exists or `selected_theme` is missing, default to the bundled `dark`.
- **Reset to defaults — revised semantic.** The `Reset to defaults` button in the Preferences Theme tab now resets the **selected theme** to its bundled defaults: deletes the user-dir copy (if any) and reloads from the bundled `assets/themes/<name>.json`. If the selected theme has no bundled origin (a user-created theme), Reset does nothing (or warns — design.md decides).

Non-goals (called out to bound scope):
- **No "+ New theme" / duplicate-selected button.** v1 ships with the bundled set plus user-edit-copies-of-bundled. A button to fork a theme under a new name is a follow-up.
- **No theme rename / delete UI.** Users can rename / delete by hand-editing files; UI controls land later.
- **No theme import/export.** The JSON files in `<config-dir>/imgui-shell/themes/` ARE the export format — sharing a theme is `cp dark.json /path/to/other/user/`.
- **No live theme search / filter.** Combo is fine for a handful of entries; a search bar lands when the catalog grows.
- **No per-theme typography lockout.** Picking a theme replaces font_file + font_size_px; the same "applies on next launch" caveat from `add-typography-persistence` still applies (theme switch updates the on-disk state, but the visible font doesn't change until relaunch).

## Capabilities

### New Capabilities

- `theme-presets`: bundled theme catalog at `assets/themes/`, user theme dir at `<config-dir>/imgui-shell/themes/`, selection persistence, picker enumeration semantics (bundled ∪ user, user wins), and first-edit copy-on-write from bundled to user dir.

### Modified Capabilities

- `preferences-dialog`: Theme tab adds a top-row theme picker (Combo); autosave-on-edit-commit now targets `<config-dir>/imgui-shell/themes/<selected>.json` instead of `theme.json`; `Reset to defaults` now resets the *selected theme* to its bundled values (or no-op for user-only themes).
- `theme-persistence`: `theme.json` schema simplified — top-level `selected_theme: "<name>"` field is now the only meaningful content; `colors` / `metrics` / `typography` blocks are removed (those live in per-theme files). `readThemeFromConfig` is replaced by two functions: `readSelectedThemeName()` returning the selection (with default `"dark"`) and `readThemeFile(path, style)` that applies a specific theme file. The `writeThemeToConfig(style)` API is replaced by `writeThemeFile(path, style)` — the caller decides which file. One-time migration on first launch detects the old `theme.json` format and rewrites it.

## Impact

- **Code change:** ~200-300 LOC net.
  - `apps/imgui-shell/assets/themes/dark.json` + `light.json` — new bundled files (~3-4 KB each).
  - `app/ThemeStorage.{h,cpp}` — split readThemeFromConfig → readSelectedThemeName / readThemeFile; split writeThemeToConfig → writeThemeFile + writeSelectedThemeName; add a one-time-migration helper; add a theme-enumeration helper that scans both dirs.
  - `app/Preferences.cpp` — add the picker Combo at the top of the Theme tab; persistTheme() rewires to target the selected theme's file; Reset now deletes-user-copy + reloads-bundled.
  - `app/App.cpp` — `init()` boot path swaps the single readThemeFromConfig for the read-selection + read-theme-file pair.
  - `platform/ios/CMakeLists.txt` — adds `IMGUI_SHELL_IOS_THEMES` list mirroring the existing `IMGUI_SHELL_IOS_FONTS` pattern.
- **New dependencies:** none. nlohmann/json + std::filesystem already cover everything.
- **Spec change:** new `openspec/specs/theme-presets/spec.md`; MODIFIED `openspec/specs/preferences-dialog/spec.md` (Theme tab gains picker, autosave + reset retargeted); MODIFIED `openspec/specs/theme-persistence/spec.md` (selection-only schema + per-theme file path).
- **CI:** no workflow changes; the desktop post-build asset-copy step already copies `assets/` next to the binary so themes get picked up automatically.
- **Risk surface:**
  - **One-time migration of existing `theme.json`** — if the migration path has a bug, the user's customizations could vanish. Mitigation: migration copies the inline blob into `themes/custom.json` and only THEN strips the inline blocks from `theme.json`; on failure of the copy step the strip is skipped (so the next launch retries).
  - **Picker enumeration races** — if the user opens the Preferences dialog while a `themes/*.json` file is mid-write, enumeration could see a partial file. Mitigation: `writeThemeFile` already uses atomic temp+rename (inherited from the existing `writeThemeToConfig`); the partial state never has the target's filename.
  - **A bundled theme JSON gets out of sync with `ImGuiCol_COUNT`** — adding a new color slot in ImGui adds a slot that bundled themes don't fill, so the new slot falls back to whatever `applyTheme` set. That's the existing `readThemeFromConfig` "unknown key" semantic; no change.
- **Backward compatibility:** breaking — existing per-user `theme.json` files are auto-migrated on first launch; users with hand-edited files keep their changes inside the new `themes/custom.json`. Bundled curated default unchanged.
