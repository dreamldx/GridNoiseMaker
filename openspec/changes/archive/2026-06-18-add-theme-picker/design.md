## Context

`add-preferences-dialog` shipped autosave: every widget edit immediately persists `colors / metrics / typography` to `<config-dir>/imgui-shell/theme.json`. There is exactly one editable theme. This change introduces *named themes* as a layer above that storage: a bundled catalog at `assets/themes/`, a writable user dir at `<config-dir>/imgui-shell/themes/`, a selection field in `theme.json`, and a picker in the Preferences Theme tab.

Two infrastructure pieces frame everything:
- **The existing `readThemeFromConfig` / `writeThemeToConfig` pair already does JSON parse / write of colors / metrics / typography.** Generalizing them to take an explicit file path is straightforward.
- **The desktop post-build asset-copy step copies `assets/` next to the binary.** Bundled themes piggyback on this — no new build-system work for desktop. iOS needs an explicit list (mirroring `IMGUI_SHELL_IOS_FONTS`).

Three constraints frame the design:
- **Bundled themes are read-only.** On desktop they sit next to the binary; on iOS they're inside the .app bundle. Edits MUST land in the user-writable config dir.
- **Selection persistence is per-user, not per-theme.** The selection state lives in `theme.json`, not inside any individual theme file.
- **Migration from the existing `theme.json` shape must not silently lose user customizations.** The old `colors / metrics / typography` blob is the user's existing custom theme; it has to land somewhere obvious (`themes/custom.json`) and remain the active selection so the user sees no visible change after migration.

## Goals / Non-Goals

**Goals:**
- Ship `assets/themes/dark.json` (curated baseline) + `assets/themes/light.json` (inverted) as the v1 bundled catalog.
- Picker UI in the Preferences Theme tab that lists `bundled ∪ user-dir` themes, with user-dir winning on name collision.
- Autosave-on-edit-commit writes to the *selected* theme's file in the user dir; first edit of a bundled theme triggers copy-on-write into user dir.
- Selection persists in `<config-dir>/imgui-shell/theme.json`'s `selected_theme` field and is applied at app startup.
- One-time migration of existing `theme.json`'s `colors / metrics / typography` blob into `themes/custom.json`, with `selected_theme: "custom"`.
- `Reset to defaults` resets the *selected* theme to its bundled values (deletes user-dir copy, reloads bundled).

**Non-Goals:**
- No "+ New theme" button, no rename/delete UI, no import/export UI (file-system-level only).
- No high-contrast / imgui-default in v1 — just dark + light.
- No live preview when hovering picker entries (commit-on-pick only).
- No theme inheritance / overrides (each theme is a flat blob; no "extends: dark").
- No per-theme typography decoupling (typography is part of each theme file, like today; the "applies on next launch" contract is unchanged).
- No "are you sure?" confirmation on picker switch (live preview means switching back is one click).

## Decisions

### D1: Storage layout — bundled + user dirs, selection in `theme.json`
```
apps/imgui-shell/assets/themes/         <-- READ-ONLY catalog (ships with app)
  dark.json
  light.json

<config-dir>/imgui-shell/
  theme.json                            <-- per-user; { "selected_theme": "dark" }
  themes/                               <-- USER-WRITABLE copies + edits
    custom.json                         <-- migrated from old theme.json (if any)
    dark.json                           <-- created on first edit of bundled dark
    ...
```

`theme.json` shape after this change:
```json
{
  "_schema_version": 2,
  "selected_theme": "dark"
}
```

The `_schema_version: 2` field is the migration signal — old files lack it. Reading: if absent and `colors` is present, migration runs.

- **Alternatives:**
  - **Single flat file**: keep `theme.json` containing all themes as a map (`{"themes": {"dark": {...}, "light": {...}}, "selected": "dark"}`). Rejected — one corrupt entry corrupts the whole file; atomic per-theme writes are easier with separate files.
  - **No bundled / user split**: copy bundled themes into user dir on first launch, then user dir is authoritative. Rejected — bundled themes become stale if the curated baseline updates in a future release; the user dir never picks up the new defaults. The "user dir overlay on bundled" model keeps the bundled catalog the source of truth and lets users opt into edits by editing.
  - **Selection in a separate `settings.json`**: cleaner separation but means two small files instead of one. `theme.json` is already the canonical "theme preferences" file; adding `selected_theme` to it preserves the name's meaning.

### D2: Theme file schema — identical to current `theme.json` content
Each `themes/<name>.json` file contains exactly what `theme.json` contains today minus the `selected_theme` field: top-level `colors / metrics / typography` blocks, identical schema, identical parsing rules, identical clamping. No nesting, no new fields.

This means:
- `readThemeFromConfig`'s existing parse logic is reused unchanged; the function just takes a path parameter.
- `writeThemeToConfig`'s existing serialize logic is reused unchanged; the function just takes a path parameter.
- The example `example-theme.json` shipping in `assets/themes/example-theme.json` can be repurposed as `assets/themes/dark.json` after stripping the `_comment` and confirming values match `applyTheme()`.

- **Alternatives:**
  - **New schema with `meta: {name, description}`**: tempting for UX (show theme description in picker tooltip) but adds parse complexity. Defer; v1 picker shows the filename stem.

### D3: Picker UI — `ImGui::Combo` at top of Theme tab, above the master-detail editor
A new horizontal row above the existing `BeginTable("ThemeEditor")`:
```cpp
ImGui::TextUnformatted("Theme:");
ImGui::SameLine();
ImGui::SetNextItemWidth(200);
if (ImGui::Combo("##theme", &selIdx, themeNames.data(), (int)themeNames.size())) {
    app::switchTheme(themeNames[selIdx]);
}
```
- Combo enumerates `theme-presets::listAvailableThemes()` (alphabetical, user-dir entries marked with a "*" suffix to indicate edits).
- Selecting a theme calls `app::switchTheme(name)` which (a) reads the theme file, (b) applies it to `ImGui::GetStyle()`, (c) writes the new selection to `theme.json`.
- The picker row reserves ~40 px of vertical space; the master-detail table below it shrinks proportionally (already uses `ImVec2(0, -buttonRowHeight)` — gets `ImVec2(0, -buttonRowHeight - pickerHeight)`).

- **Alternatives:**
  - **`ImGui::ListBox`**: takes more vertical space; not a good fit for what is essentially a single-line dropdown.
  - **Picker as its own sub-tab**: over-engineered for a single dropdown.
  - **Picker in a separate left column of the master-detail table**: muddies the existing 35/65 split.

### D4: Listing themes — `bundled ∪ user-dir`, user-dir wins on name collision
```
listAvailableThemes():
  result = {}
  for f in scandir(bundledThemesDir):
    if f.endswith(".json"):
      result[stem(f)] = {path: bundled/f, source: bundled}
  for f in scandir(userThemesDir):
    if f.endswith(".json"):
      result[stem(f)] = {path: user/f, source: user}     # overrides bundled
  return sorted(result.values(), by=name)
```

Empty user dir is a no-op (no `mkdir` until first write). Bundled dir always exists on a clean install. Missing bundled dir (theoretical asset-copy failure) → empty list → picker shows "(no themes)" placeholder and `applyTheme()` curated values remain.

- **Alternatives:**
  - **User dir wins by replacement, not overlay**: i.e., user's `dark.json` doesn't include `_schema_version` so it's incomplete. Rejected — flat replace is simpler than overlay; each theme file is a complete snapshot.
  - **Index file listing names**: an `index.json` mapping display-name to file path. Defer until users want display names different from filenames.

### D5: Edit flow — copy-on-write from bundled to user dir on first edit
Current behavior: every committed widget edit calls `persistTheme()` → `writeThemeToConfig(style)`.

New behavior:
```cpp
void persistTheme() {
  std::string name = app::selectedThemeName();    // from theme.json
  std::string path = app::userThemePath(name);    // <config>/imgui-shell/themes/<name>.json
  // Atomic write via temp+rename (existing writeThemeFile behavior).
  app::writeThemeFile(path, ImGui::GetStyle());
}
```

The first call for a bundled theme creates the user-dir file with the bundled values + the user's edit applied (since `ImGui::GetStyle()` already reflects the bundled values from the most recent `switchTheme` load). Subsequent calls overwrite the user-dir file.

After the first edit, `listAvailableThemes` finds the user-dir file and marks it with `*` to indicate it's user-edited. Bundled dir stays pristine.

- **Alternatives:**
  - **Track "dirty" in memory, only persist on close**: lost-edits trap if the process crashes; rejected (autosave's promise is "no surprise data loss").
  - **Show a "Modified" badge per theme**: marginal value; defer.

### D6: `Reset to defaults` — delete user copy, reload bundled
```cpp
void resetSelectedTheme() {
  std::string name = app::selectedThemeName();
  std::filesystem::path userPath = app::userThemePath(name);
  std::error_code ec;
  std::filesystem::remove(userPath, ec);          // no-op if not present
  // Reload from bundled (or applyTheme fallback if no bundled exists).
  app::applySelectedThemeToStyle(ImGui::GetStyle());
}
```

If the selected theme has no bundled origin (a user-only theme — only possible once a follow-up adds "+ New theme"), Reset is a no-op + one-line stderr warning. v1 ships without "+ New theme", so the only user-dir entries are copies of bundled themes — Reset always has a bundled fallback.

- **Alternatives:**
  - **Reset deletes the user-dir copy AND applies `applyTheme` curated defaults**: applyTheme is the curated dark baseline, not `light`'s baseline. Wrong semantic for a "reset the light theme to its bundled values" request. Rejected.

### D7: One-time migration of pre-existing `theme.json`
On `app::init`, after building `theme.json`'s path but before reading:
```cpp
void migrateLegacyThemeFile() {
  if (!exists(themeConfigPath())) return;
  json j = parseFile(themeConfigPath());
  if (j.contains("_schema_version") || !j.is_object()) return;   // already migrated / empty
  bool hasLegacyBlocks = j.contains("colors") || j.contains("metrics") || j.contains("typography");
  if (!hasLegacyBlocks) return;
  // Migration:
  std::string customPath = userThemesDir() + "/custom.json";
  ensureParentDirs(customPath);
  json customContent;
  if (j.contains("colors"))     customContent["colors"]     = j["colors"];
  if (j.contains("metrics"))    customContent["metrics"]    = j["metrics"];
  if (j.contains("typography")) customContent["typography"] = j["typography"];
  writeAtomic(customPath, customContent);                         // atomic temp+rename
  // Rewrite theme.json AFTER the copy succeeds:
  json newRoot = {{"_schema_version", 2}, {"selected_theme", "custom"}};
  writeAtomic(themeConfigPath(), newRoot);
  log("[imgui-shell] migrated legacy theme.json to themes/custom.json");
}
```

The order matters — write `custom.json` first; only if that succeeds, rewrite `theme.json`. Crash mid-migration leaves `theme.json` in the old shape; next launch retries.

- **Alternatives:**
  - **Force migration via a CLI flag / opt-in toggle**: over-engineered for a single-developer trajectory; auto-migrate.
  - **Keep both formats supported indefinitely**: doubles the parse paths forever. Rejected; v2 schema is the only schema going forward.

### D8: Bundled `light.json` — invert the curated dark
Mechanical inversion via the same `c(r,g,b)` helper used in `Theme.cpp`:
- Background tiers (`WindowBg / ChildBg / PopupBg / MenuBarBg / TitleBg*`) → off-white / light gray.
- `Text` → near-black (e.g., `c(28, 32, 38)`).
- `TextDisabled` → mid gray (e.g., `c(140, 145, 155)`).
- Accent (`kAccent`) → same cyan-blue (works on both backgrounds).
- Borders / separators → mid gray on light.
- Metrics block → identical to dark (paddings + roundings don't change with palette).
- Typography → identical (same font, same size).

The exact values are decided during implementation (`light.json` will be hand-tuned with the picker open so we can iterate live). Acceptable mediocrity for v1; future change can refine.

- **Alternatives:**
  - **Algorithmically invert dark via HSL inversion**: produces bad colors for the accent and disabled-text slots. Hand-tuned is small enough.

## Risks / Trade-offs

- **[Risk: a typo in `selected_theme` makes the picker render an empty selection]** → Mitigation: at startup, if `selected_theme` doesn't resolve to a known theme file, fall back to `dark` and log a warning. The picker shows `dark` as selected; user can pick something else.
- **[Risk: `assets/themes/` directory not copied next to the binary on a fresh build]** → Mitigation: the existing `apps/imgui-shell/CMakeLists.txt` post-build step copies `assets/` wholesale; verified during implementation via `ls build/macos/platform/desktop/assets/themes/`. iOS gets a dedicated `IMGUI_SHELL_IOS_THEMES` list mirroring `IMGUI_SHELL_IOS_FONTS`.
- **[Risk: user manually edits a `themes/*.json` and corrupts the JSON]** → Mitigation: the existing `parseFile` ignores invalid JSON with a one-line stderr warning; the corrupt theme is omitted from the picker until fixed. Selection falls back to `dark`.
- **[Risk: picker enumeration sees a partial file mid-write]** → Mitigation: `writeThemeFile` uses atomic temp+rename (inherited from `writeThemeToConfig`); the partial state lives only as `<name>.json.tmp.<pid>` which `listAvailableThemes` ignores (only `*.json` extension matches).
- **[Trade-off: per-theme files means more I/O on every edit]** → Acceptable: ~3-4 KB writes via atomic rename on a release-the-mouse cadence (max ~once / second under hand-edit). The previous single-file flow had the same cost.
- **[Trade-off: bundled vs user dir means two parse + scan operations on every Preferences open]** → Acceptable: handful of files; both dirs are small (≤10 files realistic, ≤100 absolute worst). Single `std::filesystem::directory_iterator` pass each.
- **[Trade-off: migration semantics — old `theme.json` becomes `themes/custom.json` with selection set to "custom"]** → A user who had hand-edited their theme.json sees the same colors after migration but the picker shows `custom` selected. They may not understand where their bundled-name `dark` went; the stderr log line is the only signal. Acceptable for single-developer pre-1.0 trajectory.

## Migration Plan

1. **On first launch with this change, `app::init` runs `migrateLegacyThemeFile()` BEFORE the read-selection-then-read-theme step.** This is the only side-effect at startup; subsequent launches see `_schema_version: 2` and skip migration.
2. **No code rollback path needed** — if the change is reverted, the user's `theme.json` (now selection-only) loses meaning; `colors / metrics / typography` blocks live in `themes/custom.json` which the reverted code ignores. User sees `app::applyTheme` curated defaults. Their data is preserved on disk but unused. Acceptable rollback shape.
3. **Bundled assets:** `assets/themes/dark.json` and `assets/themes/light.json` are added to the source tree. Desktop's existing `assets/` copy step picks them up. iOS adds the entries to `IMGUI_SHELL_IOS_THEMES` in `platform/ios/CMakeLists.txt`.

## Open Questions

- **Should `light.json` be hand-tuned or shipped as a near-finished v1?** Probably hand-tuned during implementation; if it's rough, mark it as "experimental" in stderr on first load? Defer the polish to a follow-up if needed.
- **Should the picker label themes by filename (`dark`) or by a friendly name (`Dark`)?** v1: filename stem. Friendly name needs schema D2 to grow a `meta.name` field; defer.
- **Should the user-dir copy be created lazily (only on first edit) or eagerly (on first switch)?** D5 says lazy — only on first persisting edit. Switching to a bundled theme and immediately switching away does NOT create a user-dir copy. This matches "user dir reflects user customizations" cleanly.
- **What's the picker behavior when the active theme file is deleted at runtime (e.g., user `rm`s `themes/custom.json` while the app is running)?** The in-memory style stays applied; the next persist call writes the file back. The next picker enumeration may show one fewer entry (if the bundled origin is also missing) or revert to bundled. Acceptable corner.
