## Status: PARKED — split into three changes

> This change has been split into three follow-on changes per the scope discussion at the bottom of this proposal. The work is being delivered as:
>
> 1. **`enable-multi-viewport`** — render-backend rework so ImGui windows can become real OS-level secondary windows (Vulkan multi-viewport callbacks, GLFW + swapchain per viewport).
> 2. **`add-theme-persistence`** — JSON I/O for the curated theme (nlohmann/json dep, per-OS config paths, read-on-init, write-on-Save).
> 3. **`add-preferences-dialog`** — the actual Preferences window UI (tab bar, master-detail Theme editor, About tab). Depends on (1) and (2).
>
> The proposal below is preserved as historical context for the end-state target; the three sub-changes carve up its content. When all three are archived, this directory can be removed or archived-as-superseded.

## Why

> **Original scope warning (preserved for context).** This change as originally scoped was the biggest yet for `imgui-shell` — three substantive pieces of work: (1) enabling ImGui's multi-viewport mode so the Preferences window can be a real OS-level secondary window, (2) adding JSON persistence (with platform-specific config paths), and (3) building the Preferences UI itself (tab bar, master-detail theme editor with ~45 widgets, About tab). Each piece touches a different capability. The split into three follow-on changes above is the agreed delivery path.

The shell now has FreeType-sharpened text, a curated dark theme, a live-resize render path, and a clean Vulkan teardown — but every aesthetic choice (font size, theme colors, metrics) is locked at build time. Contributors who want to tune the palette or try a different size have to edit `Theme.h` and rebuild. A Preferences window with a tab-based layout (`General` / `Theme` / `About`), opened from `Help > Preferences...`, gives that audience the obvious affordance: a separate OS window they can drag aside while the main shell window shows the result live. The Theme tab is the centerpiece — a master-detail layout (list of style slots on the left, edit widget on the right) lets users tweak any of the ~30 colors and ~15 metric fields and see the change immediately; a Save button persists the current style to a per-OS config path so the tweak survives restart.

## What Changes

- **Multi-viewport Vulkan in `render-backend`.** Enable `io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable`, populate `ImGui_ImplVulkan_InitInfo.UseDynamicRendering` (or the multi-viewport-specific callbacks per ImGui's example), and wire the per-viewport `RenderWindow`/`Platform_CreateWindow` callbacks so ImGui windows the user drags outside the main window become real GLFW/Vulkan secondary windows. On iOS this feature is unavailable (single-window OS); the Preferences dialog falls back to a floating in-main-window panel via the same `ImGui::Begin("Preferences")` code — the multi-viewport flag is just absent on iOS so the same window goes inline.
- **New `Help > Preferences...` menu item** in `app/App.cpp`, opens the Preferences window. Closing the window (X button or Close button in any tab) closes it but persists last-known position/size via ImGui's standard ini (re-enable ImGui's persistent ini file from the current "disabled" state, scoped to the Preferences window only via `ImGuiWindowFlags`).
- **Tab bar with three tabs.**
  - **General** — minimal in v1: shows `IMGUI_VERSION`, `IMGUI_SHELL_PLATFORM_NAME`, build configuration (Debug/Release), Vulkan API version, GPU device name. Read-only. Stub for future general settings.
  - **Theme** — master-detail: left pane is an `ImGui::Selectable` list of all theme-editable items (color slots first, then metric fields, grouped with `ImGui::Separator`). Right pane shows an `ImGui::ColorEdit4` for the selected color, or `ImGui::SliderFloat` / `ImGui::DragFloat` / `ImGui::SliderInt` for metrics. Changes mutate `ImGui::GetStyle()` immediately for live preview. Three buttons below the detail pane: `Save` (writes current style to the config file), `Reset to theme defaults` (calls `applyTheme(ImGui::GetStyle())` again), `Discard unsaved changes` (re-reads the saved file).
  - **About** — copyright info (project name, version, year, license placeholder), Dear ImGui credit, Inter font credit (with OFL link), FreeType credit. Static content. Coexists with the existing `Help > About...` modal popup; both remain available — the modal stays for the "verifiable at a glance" pattern this codebase has used to confirm builds, the tab gives more room for full credits.
- **JSON theme persistence.**
  - Add `nlohmann/json` (header-only, MIT) via `FetchContent` in `cmake/Dependencies.cmake`, pinned to tag `v3.11.3`.
  - Theme persistence path is per-platform:
    - macOS / Linux: `${XDG_CONFIG_HOME:-~/.config}/imgui-shell/theme.json`
    - Windows: `%APPDATA%/imgui-shell/theme.json`
    - iOS: `<NSDocumentDirectory>/theme.json` (read via the existing `app::setBundleResourcePath` plumbing, extended with a sibling `setDocumentsPath`)
  - Schema: a JSON object with `colors` (object: ImGuiCol_* enum name → `[r, g, b, a]`) and `metrics` (object: metric field name → number). Backwards-compatible parsing — missing keys silently keep the in-memory value (whatever `applyTheme` set).
  - Read on `app::init`, AFTER `applyTheme` runs, so user-saved values override the baked-in theme. Missing/malformed file → silently keep `applyTheme`'s baked-in values.
- **Spec change: gui-theme's "Theme is a build-time decision" requirement is REMOVED.** Replaced with a new requirement allowing runtime in-session edits + persistence via the Preferences dialog. The "no `Help > Theme > ...` menu" sub-clause specifically is dropped — replaced by `Help > Preferences...` opening the dialog.

Non-goals (called out to bound scope):
- No light theme. Persistence stores one theme; switching to a "Light" preset is a follow-up change.
- No undo/redo within the Theme tab. Discard re-reads the file; that's the only "undo".
- No theme import/export (drag-and-drop a `.json` file). Just the auto-load + Save button.
- No keyboard shortcuts for the Preferences window. Menu item only.
- No General-tab settings that actually modify behavior in v1 — just diagnostic info.
- No multi-viewport on iOS — iOS Preferences is inline.
- No font selection in the Theme tab (font family/size are theme constants in `Theme.h` from the previous change; exposing them as runtime-tweakable is a separate follow-up).

## Capabilities

### New Capabilities

- `preferences-dialog`: The Preferences window's structure — three tabs (General / Theme / About), the master-detail Theme tab layout, the `Help > Preferences...` menu item that opens it, the close/save/discard buttons. Owns the dialog's behavior and what's editable in each tab.
- `theme-persistence`: The JSON schema, per-platform config-file location, read-on-init and write-on-Save semantics, missing-file / malformed-file fallback behavior. Owns the persistence side of theme tweaking — distinct from the dialog's UI, so the persistence can be reused later (e.g., command-line `--reset-theme` flag).

### Modified Capabilities

- `gui-theme`: The "Theme is a build-time decision in v1" requirement is REMOVED — runtime editing is now allowed via the Preferences dialog, and the "no `Help > Theme > ...` menu" restriction is dropped. The other gui-theme requirements (curated theme applied at init, single-hue accent, tuned metrics, theme name in About, theme code lives in shared core, theme owns typography defaults) are unchanged.
- `render-backend`: A new requirement enables ImGui's multi-viewport mode on desktop and wires the per-viewport GLFW + Vulkan swapchain callbacks. The existing desktop scenarios (instance creation, surface/device/swapchain init, frames-in-flight) gain a note that they describe the **primary** viewport; secondary viewports follow ImGui's `ImGui_ImplVulkan_*` viewport callbacks pattern. iOS scenarios unchanged (no multi-viewport).
- `ui-sample`: One new `Help > Preferences...` menu item is added to the existing `Help` menu. The existing `Help > About...` modal is unchanged (kept per user's pick).

## Impact

- **New code:** ~500–700 LOC net.
  - `app/Preferences.{h,cpp}` — dialog UI (~250 LOC), tab content rendering, master-detail theme editor
  - `app/ThemeStorage.{h,cpp}` — JSON read/write, per-platform path resolution (~150 LOC)
  - `platform/desktop/main.cpp` — ImGui multi-viewport wiring (~80 LOC)
  - `platform/ios/ViewController.mm` — `app::setDocumentsPath` plumbing (~20 LOC)
  - `app/App.cpp` — menu item + viewport flag + theme-storage init/save call sites (~30 LOC)
- **New dependency:** `nlohmann/json` v3.11.3 (header-only, MIT, vendored via FetchContent — same pattern as ImGui + GLFW + FreeType).
- **Spec change:** new `openspec/specs/preferences-dialog/spec.md` + new `openspec/specs/theme-persistence/spec.md`. MODIFIED `gui-theme` (one removed requirement), MODIFIED `render-backend` (one new requirement + scenario notes), MODIFIED `ui-sample` (new menu item).
- **CI:** no workflow changes. nlohmann/json builds in seconds; multi-viewport doesn't add build complexity.
- **Risk surface:** multi-viewport Vulkan is the highest-risk piece. ImGui ships a reference implementation in `examples/example_glfw_vulkan/`; we'd mirror its `SetupVulkanWindow` / `FrameRender` / `FramePresent` viewport callback pattern. The original v1 render path already gets us most of the way; the new code is the per-viewport callbacks, not a rebuild from scratch. Still, this is the first change touching the render-backend in a substantive way since `fix-desktop-shutdown-crash`.
- **Backward compatibility:** users with an old build see no theme.json file → preferences open with baked-in `Theme.cpp` defaults — graceful. Users who save a theme then upgrade the binary → missing keys in their saved file fall back to baked-in defaults — graceful. No data migration needed.

## Open scope question

Given the size (3 capabilities × multi-week-ish work), it may be cleaner to split this change into:
1. `enable-multi-viewport` — just the render-backend rework, no dialog
2. `add-theme-persistence` — JSON I/O + apply-on-load, no UI
3. `add-preferences-dialog` — the UI itself, depending on the above

The current proposal treats it as one change. If `/opsx:apply` later proves too large to ship as one, we can carve it.
