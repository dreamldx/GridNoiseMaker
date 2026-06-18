## Status: ACTIVE — sub-change 3 of 3 from the original split

> This change was parked while the infrastructure prerequisites shipped as separate sub-changes. All three are now archived:
>
> 1. ✓ `2026-06-16-enable-multi-viewport` — secondary OS windows via ImGui multi-viewport
> 2. ✓ `2026-06-17-add-theme-persistence` — JSON I/O for `colors` + `metrics`
> 3. ✓ `2026-06-17-add-typography-persistence` — JSON `typography` section + font accessors + bundled JetBrainsMono / Lato
>
> This change is the final UI layer that ties them together: a `File > Preferences...` window with General / Theme / About tabs and a master-detail Theme editor that mutates `ImGui::GetStyle()` live and Saves to `theme.json` via the persistence layer. The historical content of the parked proposal is preserved further down for context, but the current scope is just **the UI**.

## Why

After eight archived changes — v1 shell, FreeType, shutdown ordering, live resize, dark theme, multi-viewport, demo-as-modal, theme persistence, and typography persistence — every aesthetic value in `ImGuiStyle` (and `themeFontFile` / `themeFontSizePx`) is *runtime-tweakable on disk* via `~/.config/imgui-shell/theme.json`. The shell can persist what the user picks. What it can't do yet is *let the user pick* — there's no UI. Anyone who wants to retune the accent color or try Lato has to find the JSON file, edit it by hand, save, and relaunch.

This change adds the missing UI surface: a Preferences window opened from the existing `File` menu. The window is built around a Theme tab with master-detail layout (list of editable items on the left, type-appropriate widget on the right) so the user can drive every persistable value with mouse clicks, see the result live in the main shell window, and Save when satisfied. A General tab carries diagnostic info (read-only), and an About tab carries copyright + credits.

## What Changes

- **New menu item.** `File > Preferences...` (with trailing ellipsis per the existing `Help > About...` / `Help > ImGui Demo...` convention) opens the Preferences window. Placed under `File` (above `Quit` on desktop) per cross-platform precedent — Preferences is an app-level "settings" affordance, distinct from Help's "about / demo / docs" cluster. The existing `Help > About...` modal popup is preserved unchanged — both surfaces coexist (the modal stays for the "verify-by-inspection" pattern; the tab shows full credits).
- **Preferences window structure.** A regular ImGui `Begin("Preferences", &g_showPreferencesWindow)` window (NOT a modal popup — the user benefits from being able to drag this outside the host as a secondary OS window via the multi-viewport infrastructure). Tab bar with three tabs:
  - **General** — read-only diagnostic info: app name + version, `IMGUI_VERSION`, `IMGUI_SHELL_PLATFORM_NAME`, build configuration (Debug / Release), Vulkan API version (from `VkInstance` query), GPU device name (from `VkPhysicalDevice` properties), font rasterizer (`IMGUI_SHELL_FONT_BACKEND`), config-file path (`app::themeConfigPath()`). One paragraph of text per major group; no widgets.
  - **Theme** — master-detail layout. Left pane is a `BeginChild` with a `Selectable` per editable item, grouped via `Separator + Text` headers ("Colors", "Metrics", "Typography"). Right pane (also `BeginChild`) renders the type-appropriate widget for whichever item is selected: `ColorEdit4` for an `ImGuiCol_*` slot, `SliderFloat` (range from spec) for a scalar metric, `DragFloat2` for an `ImVec2` metric, `Combo` of the three bundled fonts for `font_file`, `SliderFloat` (6–96) for `font_size_px`. Widgets mutate `ImGui::GetStyle()` directly for live preview; typography widgets call `setThemeFontFile` / `setThemeFontSizePx` directly (note: a font_file change takes effect at next launch per the existing typography-persistence relaunch contract — a small inline note in the right pane communicates this).
  - **About** — static content. App name, copyright placeholder ("© 2026 imgui-shell contributors"), license placeholder ("TBD — see project README"), Dear ImGui credit + `IMGUI_VERSION`, Inter font credit (with OFL pointer), JetBrains Mono credit, Lato credit, FreeType credit, GLFW credit, Vulkan credit. Small list of `ImGui::Text` lines; no widgets.
- **Autosave + Reset to defaults.** Every widget edit (color picker release, slider release, Combo selection, InputText commit) triggers `app::writeThemeToConfig(ImGui::GetStyle())` automatically. There is no Save button (redundant). There is no Discard button (no uncommitted state to discard). The Theme tab's only bottom-row affordance is a `Reset to defaults` button that re-applies `app::applyTheme` then writes the baseline to disk. **Load on startup** is already wired: `app::init` calls `app::readThemeFromConfig(ImGui::GetStyle())` after `applyTheme`, so the persisted theme is the boot state.
- **No undo/redo within the editor.** Reset to defaults is the only revert affordance.
- **Window state.** The Preferences window is **not** open by default — opens via the menu, closes via the X. The `g_showPreferencesWindow` flag persists across menu open/close cycles but resets to `false` at process restart (no ImGui ini for it — `io.IniFilename = nullptr` per the existing app-shell setup).

Non-goals (called out to bound scope):
- **No live font atlas rebuild.** Per the typography-persistence contract, font_file / font_size_px changes apply at next launch. The dialog communicates this inline; we don't implement runtime atlas rebuild here.
- **No theme presets / theme switcher.** Single editable theme.
- **No undo stack.** Discard + Reset are the only revert affordances.
- **No keyboard shortcuts.** Menu item only.
- **No iOS-specific UI accommodations.** The same `Begin` window opens on iOS, sized for a single iOS viewport. No floating / docked / popover variants.
- **No General-tab editable settings.** Read-only diagnostic info only.

## Capabilities

### New Capabilities

- `preferences-dialog`: The Preferences window's structure, tab content, master-detail Theme editor layout, Save / Discard / Reset semantics, and the `Help > Preferences...` menu item. Owns "what the user sees when they pick Preferences."

### Modified Capabilities

- `ui-sample`: The existing `File` menu gains one new item: `File > Preferences...` (above `Quit` on desktop; only entry on iOS). The existing `Help > About...` modal popup and `Help > ImGui Demo...` modal placeholder are unchanged.

## Impact

- **Code change:** ~300–400 LOC net.
  - `app/Preferences.{h,cpp}` — dialog UI (~250 LOC: 3 tab bodies + master-detail editor + Save/Discard/Reset).
  - `app/App.cpp` — `Help > Preferences...` menu item + per-frame dialog rendering (~20 LOC).
  - Maybe a small `app/PreferencesGeneral.{h,cpp}` if the General-tab Vulkan-info queries get bulky; otherwise inline.
- **New dependency:** none. nlohmann/json is already vendored; ImGui's standard widgets cover everything in the editor.
- **Spec change:** new `openspec/specs/preferences-dialog/spec.md`; MODIFIED `openspec/specs/ui-sample/spec.md` (Help menu gains a `Preferences...` item).
- **CI:** no workflow changes.
- **Risk surface:**
  - Live preview means mutating `ImGui::GetStyle()` while inside a `ColorEdit4` widget — entirely supported by ImGui, no issues.
  - Save while widgets are still mid-edit: each widget commits to `ImGui::GetStyle()` continuously, so the saved file reflects the in-memory state at click-time. No staging buffer.
  - Multi-viewport interaction: when the user drags the Preferences window outside the main shell, the master-detail layout reflows naturally. No special handling.
- **Backward compatibility:** none required. The dialog opt-in via menu; not opening it preserves the existing shell behavior.

## Historical context (preserved from the original parked proposal)

> The original parked proposal bundled this UI work with the multi-viewport infrastructure AND the JSON persistence into one large change. The split into three sub-changes (infrastructure first, then persistence, then UI) was the agreed delivery path. The infrastructure pieces shipped; this change is the user-facing layer that uses them.
