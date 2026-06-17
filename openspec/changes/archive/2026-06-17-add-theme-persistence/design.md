## Context

The shell needs to persist user theme tweaks somewhere so they survive process restarts. There are three classical options for "where": a fixed path next to the executable (works on desktop, ugly on iOS, requires write access to the install dir), per-user platform-conventional paths (XDG / APPDATA / iOS Documents), or a server-side store (overkill). Per-user platform-conventional paths is the only one that doesn't pick a fight with macOS / Linux / Windows sandboxing.

For the on-disk format, JSON is the right call here: small, human-readable, easy to hand-edit when the Preferences UI doesn't exist yet (sub-change 3), and we already have the `nlohmann/json` dependency vetted (it's the same lib used by the deferred node-graph-core design). Binary formats (CBOR, MessagePack) would shrink the file marginally for no real benefit at this scope.

This sub-change deliberately ships **without any UI**. The Preferences-window Save button lives in sub-change 3 — but the read path runs on every launch, so a user (or contributor) can drop a hand-crafted `theme.json` at the config path right now and see it applied. The shipped example asset (`assets/themes/example-theme.json`) is precisely for that smoke-test workflow.

## Goals / Non-Goals

**Goals:**
- Persist theme tweaks (colors + metrics) at a per-OS platform-conventional path so each user's choices survive restart.
- Apply user-saved values on top of the baked-in `applyTheme` defaults at startup — so users with no save file see the unchanged curated dark theme, and users with a save file see their tweaks.
- Provide a `writeThemeToConfig(const ImGuiStyle&)` API that the future Preferences-window Save button will call. The implementation must be atomic (temp file + rename) so a mid-write crash doesn't leave a corrupt file.
- Be lenient about unknown / removed keys in the saved file — log warnings, keep going. Old saved files SHOULD continue working after ImGui adds new color slots.
- Ship an `example-theme.json` so contributors see the schema before the Preferences UI exists.

**Non-Goals:**
- No UI surface. (Sub-change 3.)
- No multi-theme support. One persisted theme file per user.
- No theme import / export (no `--load-theme=foo.json`, no drag-and-drop).
- No background auto-save. Save is explicit, called by the future UI.
- No JSON schema versioning. The schema is flat and additive-by-design (missing keys keep in-memory values).
- No iOS host wiring this sub-change. The `setDocumentsPath` API exists; the iOS host's call to it lands when sub-change 3 needs Save to work on iOS.
- No undo / redo.

## Decisions

### D1: JSON schema — flat colors + metrics objects, ImGui enum names as keys
The file is a top-level JSON object with exactly two keys, both optional:

```json
{
  "colors": {
    "WindowBg":            [0.082, 0.094, 0.122, 1.0],
    "Text":                [0.860, 0.870, 0.890, 1.0],
    "CheckMark":           [0.392, 0.745, 0.941, 1.0],
    "_comment":            "Any keys starting with _ are ignored; this is documentation-only.",
    ...
  },
  "metrics": {
    "WindowRounding":      6.0,
    "FrameRounding":       4.0,
    "FrameBorderSize":     1.0,
    "WindowPadding":       [10.0, 10.0],
    "FramePadding":        [8.0, 5.0],
    "ItemSpacing":         [8.0, 6.0],
    ...
  }
}
```

- Color values are `[r, g, b, a]` arrays of 4 floats in 0..1 range (matches ImGui's internal representation).
- Metric values are either a single float (scalar fields like `WindowRounding`) or a 2-float `[x, y]` array (`ImVec2` fields like `WindowPadding`).
- Keys starting with underscore (`_comment`, `_note`, `_anything`) are silently ignored at load time and untouched at save time. Gives us a place to put inline documentation in the shipped example without polluting the data model.
- Color keys map to `ImGuiCol_*` enum values via `ImGui::GetStyleColorName(i)` — that function returns the canonical string name without the `ImGuiCol_` prefix (e.g., `"WindowBg"` not `"ImGuiCol_WindowBg"`). We use the exact strings ImGui produces so a future ImGui release adding new color slots is automatically compatible.
- Metric keys map to fields on `ImGuiStyle` via a hand-maintained switch / lookup table in `ThemeStorage.cpp`. We only handle the metrics `gui-theme` already cares about (the 19 fields `applyTheme` sets); unknown metric keys log a warning.

- **Alternatives:**
  - **Use integer enum values for color keys instead of strings:** Faster to parse but unreadable in a hand-edited file. Rejected — readability wins for a config file users will touch.
  - **Nested groups (e.g., `colors.background.WindowBg`):** More structure for no real gain. Rejected.

### D2: Per-OS config path resolver
Three platform branches, all in `themeConfigPath()`:

```
macOS / Linux:  ${XDG_CONFIG_HOME:-$HOME/.config}/imgui-shell/theme.json
Windows:        %APPDATA%/imgui-shell/theme.json
iOS:            g_documentsPath + "/theme.json"  (g_documentsPath set by setDocumentsPath)
```

The platform branch is selected via the existing `app::kIsDesktop` / `app::kIsMobile` traits from `Platform.h` (`if constexpr` — no `#ifdef` blocks in source files per `app-shell` spec). Within desktop, a second `if constexpr` on a new `kIsWindows` trait (added to `Platform.h` in this change) selects the Windows branch.

Environment variable resolution uses `std::getenv`. If `XDG_CONFIG_HOME` is unset or empty, fall back to `$HOME/.config`. If `HOME` is also unset (extremely rare), fall back to `./.config` and log a warning. On Windows, `%APPDATA%` is set on every login session; if absent we fall back to `./AppData` with a warning.

- **Alternatives:**
  - **Use Qt-style `QStandardPaths`-equivalent C++ library:** Adds a dep for one function. Rejected.
  - **Always store next to the executable:** Breaks on macOS app bundles (Contents/ is read-only) and iOS (sandboxed). Rejected.
  - **Single hard-coded path:** Same issues. Rejected.

### D3: Three error severities — missing (silent), malformed (log + bail), unknown key (warn + continue)
- **File doesn't exist:** No log. No error. `readThemeFromConfig` returns immediately; in-memory style is the baked-in `applyTheme` result.
- **File exists but JSON parse fails:** Log a single error message to stderr (`[imgui-shell] theme.json parse failed: <error>; skipping`), leave `style` untouched. The application continues with the baked-in theme.
- **File parses but a color key is not a recognized `ImGuiCol_*` name** (or a metric key isn't in our supported set): Log one warning per unknown key (`[imgui-shell] theme.json: unknown color/metric key 'WindowBgUnknownFoo'; ignoring`). Apply the known keys; ignore the unknown ones; keep going.
- **File parses but a color value isn't a 4-element array of floats** (or a metric value is the wrong type): Log a warning identifying the offending key. Skip that key. Keep going.

This gives developers a clear signal when they hand-edit a file wrong, while never silently nuking a user's tweaks because one slot name typo'd.

- **Alternatives:**
  - **Strict mode (refuse the whole file on any unknown key):** Hostile to forward-compat — adding new ImGui color slots upstream would break every existing saved file. Rejected.
  - **Silent mode (no warnings for unknown keys):** Cheap; harder to debug a "why isn't my edit taking effect" report. Rejected.

### D4: Atomic write via `std::filesystem::rename` of a temp file
`writeThemeToConfig` writes to `<path>.tmp.<pid>` first, then renames over `<path>` once the temp file is fully written and closed. On POSIX, `rename` is atomic with respect to readers (a concurrent reader sees either the old contents or the new, never a partial). On Windows, `MoveFileExW(... MOVEFILE_REPLACE_EXISTING)` provides the same guarantee — `std::filesystem::rename` uses it under the hood since C++17.

Parent directory creation: `std::filesystem::create_directories(path.parent_path())` runs before opening the temp file. Errors propagate as exceptions; the public API catches and logs.

- **Alternatives:**
  - **Write directly to the final path:** Crash-during-write leaves a half-empty file the next read can't parse. Rejected.
  - **POSIX-only `O_TMPFILE` + `linkat`:** Platform-specific; no Windows analog. Stick with the portable temp-rename pattern.

### D5: Read order — `StyleColorsDark` → `applyTheme` → `readThemeFromConfig`
The init sequence in `app::init` becomes:

```cpp
ImGui::CreateContext();
ImGui::StyleColorsDark();              // ImGui's defaults populate every slot
app::applyTheme(ImGui::GetStyle());    // gui-theme: curated overrides
app::readThemeFromConfig(ImGui::GetStyle()); // theme-persistence: user overrides
if constexpr (kIsDesktop) {
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
}
configureFontAtlas();
```

This ordering means:
- Slots ImGui added but neither `applyTheme` nor the user touched → ImGui's `StyleColorsDark` default.
- Slots `applyTheme` overrides but the user didn't touch → curated theme value.
- Slots the user touched → user's saved value.

No reordering needed in future ImGui upgrades; new slots automatically inherit `StyleColorsDark` defaults.

- **Alternatives:**
  - **Read user file *before* `applyTheme`:** User's saved values would be overwritten by the curated theme. Backward; rejected.

### D6: Shipped example at `assets/themes/example-theme.json`, copied next to the desktop binary
Placed under `apps/imgui-shell/assets/themes/example-theme.json`. The existing post-build `copy_directory` step (added by `add-font-antialiasing`) already copies the whole `assets/` directory next to the desktop binary, so the example file naturally appears at `<binary-dir>/platform/desktop/assets/themes/example-theme.json`. On iOS, the file is bundled into Resources/themes/ via a sibling `MACOSX_PACKAGE_LOCATION "Resources/themes"` source-file property in `platform/ios/CMakeLists.txt`.

Content: the same color + metric values that `Theme.cpp::applyTheme` sets, prefixed with a `_comment` field that briefly explains the schema. Maintaining this in sync with `Theme.cpp` is a deliberate manual step — they can diverge if `Theme.cpp` is tuned and the example isn't updated. A task in `tasks.md` reminds future-readers of the sync responsibility.

- **Alternatives:**
  - **Generate the example automatically at build time from `Theme.cpp`:** Would require either a code-generator step (extra build complexity) or a runtime "dump the current theme to JSON" call. Both are heavier than the manual-sync trade-off. Rejected for v1; revisit if drift becomes a real problem.
  - **Ship no example:** Users have no schema reference until sub-change 3's UI lands. Rejected — the user explicitly asked for an example.

## Risks / Trade-offs

- **[Risk: `Theme.cpp` and `example-theme.json` drift over time]** → Mitigation: documented manual-sync task in `tasks.md`; both files reference the same `gui-theme` capability + design.md D1 values so a careful reviewer can catch drift in PR review. Auto-generation deferred per D6 alternatives.
- **[Risk: `std::getenv` on macOS / Linux returns nullptr for unset env vars; null-deref crash if not checked]** → Mitigation: all `std::getenv` results are wrapped in `const char* v = std::getenv("XDG_CONFIG_HOME"); std::string str = v ? v : "";` pattern.
- **[Risk: Atomic rename across filesystems fails (e.g., `/tmp` on a tmpfs vs target on a real fs)]** → Mitigation: the temp file is placed at `<target>.tmp.<pid>`, in the SAME directory as the target. Same-directory rename is always atomic on every common FS.
- **[Risk: nlohmann/json silently coerces wrong types]** → Mitigation: we call `.is_array()`, `.is_number_float()`, etc. before accessing values; any mismatch falls through to the warn-and-continue path (D3).
- **[Risk: large saved file (e.g., user appends thousands of `_comment` entries) slows startup]** → Acceptable: the parse is O(n) and a few thousand keys is microseconds. If it becomes an issue we can size-cap the file (e.g., refuse files > 1 MB) in a follow-up.
- **[Risk: file at the config path is a directory (rare but possible if a user mkdir'd by mistake)]** → Mitigation: `std::filesystem::is_regular_file()` check before opening for read; if false, log a warning and treat as missing.
- **[Trade-off: the `setDocumentsPath` API exists in this sub-change but isn't called from anywhere yet]** → Acceptable. The API lands here because it's part of the persistence contract; the iOS host's call site is a one-liner that lands in sub-change 3 where the Save button gives it a reason to exist.

## Migration Plan

Not applicable. Greenfield persistence layer; no prior file format. Rollback = revert this change; existing saved files (if any) become unreferenced bytes on disk; subsequent launches use the baked-in theme.

## Open Questions

- **Should `_comment`-prefix ignore extend to top-level keys too?** Right now only inside `colors` and `metrics`. Probably yes for consistency; trivial follow-up edit if a user complains.
- **Should we cap the warning output (e.g., max 10 unknown-key warnings per launch)?** Defensible at high-noise scale; not load-bearing for v1.
- **iOS host `setDocumentsPath` call site** — committed to land in `add-preferences-dialog` (sub-change 3). If sub-change 3 is delayed, we may need a stub Save flow or a follow-up to get iOS persistence working in isolation.
