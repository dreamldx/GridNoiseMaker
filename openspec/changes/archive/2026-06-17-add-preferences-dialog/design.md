## Context

The Preferences dialog is the user-facing layer that closes the loop on three pieces of infrastructure already shipped: multi-viewport (so the dialog can be dragged outside the main shell), theme-persistence (so Save writes to disk), and typography-persistence (so font choice + size are part of the editable surface). The actual UI is intentionally narrow — one window, three tabs, one master-detail editor — and uses only standard ImGui widgets. No custom rendering, no new abstractions, no test framework. The interesting design work is in (a) the master-detail layout proportions, (b) the per-metric `SliderFloat` ranges, (c) how the typography "applies on next launch" caveat is surfaced inline, and (d) Save/Discard/Reset semantics.

Two concrete constraints frame everything else:
- **Live preview** — the user expects to see color/metric changes immediately in the main shell window. ImGui's `ColorEdit4` / `SliderFloat` widgets already write through to `ImGui::GetStyle()` continuously, so "live preview" is free.
- **Font changes are next-launch only.** This is the typography-persistence contract. The dialog has to communicate that without surprising the user.

## Goals / Non-Goals

**Goals:**
- Add `File > Preferences...` opening a regular ImGui window (not modal) with 3 tabs.
- Theme tab is the centerpiece: master-detail with left selectable list (Colors / Metrics / Typography sections) and a type-appropriate widget on the right.
- Save / Discard / Reset buttons with clear, distinct semantics.
- Multi-viewport friendly — dragging the Preferences window outside the host's bounds produces a real OS window.
- About tab credits every third-party dep with name + license.

**Non-Goals:**
- No undo stack. Discard re-reads from disk; Reset re-applies `applyTheme` defaults.
- No live font atlas rebuild. Font changes apply on next launch.
- No theme presets / no theme switcher.
- No keyboard shortcuts for the dialog.
- No General-tab editable settings (read-only diagnostics only).
- No "are you sure?" confirmation on Save / Discard / Reset.
- No search / filter for the editor list.
- No drag-and-drop of `.json` files onto the window.

## Decisions

### D1: Regular `ImGui::Begin` window, not modal
A modal popup would block interaction with the main shell while open — the wrong UX, because the whole point is **live preview** of theme changes against the main shell's running UI. A regular window lets the user keep both visible.

Window flags: `ImGuiWindowFlags_None` (i.e. all defaults including resize, move, dock — no NoSavedSettings, since the user may want size persistence across opens; though our app currently disables `io.IniFilename`, leaving the default is forward-compatible for when/if ini is re-enabled).

Multi-viewport-friendly by virtue of being a normal `Begin` window — the user can drag it outside the host bounds and ImGui's per-viewport platform/renderer callbacks take over.

- **Alternatives:**
  - **Modal popup:** kills live preview. Rejected.
  - **Custom render pass with vk-driven framebuffer for the dialog:** wildly over-engineered. Rejected.

### D2: Tab bar — General / Theme / About, in that order, Theme selected by default
`BeginTabBar("PreferencesTabs")` + 3 `BeginTabItem` blocks. Theme is selected first because it's the centerpiece — first impression on opening the window should be the editable surface. `BeginTabItem` accepts a default-open flag if needed (`ImGuiTabItemFlags_SetSelected` on first frame).

- **Alternatives:**
  - **Single-tab dialog with collapsing headers:** less spatial separation between unrelated sections; harder to ignore General + About if user just wants to edit. Rejected.
  - **Three separate windows:** clutters the workspace. Rejected.

### D3: Theme tab — master-detail with 35%/65% split, both panes are BeginChild
Layout: `BeginTable("ThemeEditor", 2, ImGuiTableFlags_Resizable)` with two columns. Initial widths: 35% left (selectable list) / 65% right (detail widget + group context). The user can drag the column divider. Inside each column, a `BeginChild` provides scrolling.

Left pane structure:
```
Colors                          (Separator + Text heading, gray TextDisabled style)
  WindowBg              [hex chip] (Selectable, ID = ImGuiCol_WindowBg)
  Text                  [hex chip] (Selectable)
  ...etc
Metrics                         (Separator + Text heading)
  WindowRounding        4.0 px   (Selectable, ID = metric idx)
  FrameRounding         4.0 px   (Selectable)
  ...etc
Typography                      (Separator + Text heading)
  Font file             Inter    (Selectable)
  Font size             14 px    (Selectable)
```

Each `Selectable` shows the label on the left and the current value (as a hex chip for colors, formatted number for metrics, current selection for typography) on the right via `ImGui::SameLine` + right-aligned text. Selection tracked via an enum-or-string + an int in a single state struct.

Right pane structure: switch on selected-item kind, render the type-appropriate widget. Always include a header label, the widget, a thin separator, and a "Resets just this item to default" small button.

- **Alternatives:**
  - **Inline editor on the selectable** (no right pane — click reveals the widget inline): cramped for `ColorEdit4`; harder to fit a typography combo. Rejected.
  - **Three sub-tabs inside Theme (Colors / Metrics / Typography):** doable but spends a tab-bar nesting level for what `Separator + heading` does for free. Rejected.

### D4: Metric `SliderFloat` ranges
Each scalar metric needs sensible `(min, max)` for its `SliderFloat`. Pulled from ImGui's own internal defaults + a sanity envelope:

| Field | Range | Step | Format |
|---|---|---|---|
| WindowPadding.x / .y | 0 – 20 | 0.5 | `%.1f` |
| FramePadding.x / .y | 0 – 16 | 0.5 | `%.1f` |
| ItemSpacing.x / .y | 0 – 20 | 0.5 | `%.1f` |
| ItemInnerSpacing.x / .y | 0 – 12 | 0.5 | `%.1f` |
| IndentSpacing | 0 – 40 | 1.0 | `%.0f` |
| ScrollbarSize | 2 – 32 | 0.5 | `%.1f` |
| GrabMinSize | 4 – 24 | 0.5 | `%.1f` |
| WindowRounding | 0 – 16 | 0.5 | `%.1f` |
| ChildRounding | 0 – 16 | 0.5 | `%.1f` |
| FrameRounding | 0 – 16 | 0.5 | `%.1f` |
| GrabRounding | 0 – 12 | 0.5 | `%.1f` |
| PopupRounding | 0 – 16 | 0.5 | `%.1f` |
| ScrollbarRounding | 0 – 16 | 0.5 | `%.1f` |
| TabRounding | 0 – 16 | 0.5 | `%.1f` |
| WindowBorderSize | 0 – 4 | 0.5 | `%.1f` |
| ChildBorderSize | 0 – 4 | 0.5 | `%.1f` |
| PopupBorderSize | 0 – 4 | 0.5 | `%.1f` |
| FrameBorderSize | 0 – 4 | 0.5 | `%.1f` |
| TabBorderSize | 0 – 4 | 0.5 | `%.1f` |

`ImVec2` fields get two `SliderFloat`s with `x` / `y` labels, OR a single `DragFloat2` — I'll use `DragFloat2` (less vertical space, fits the right pane cleanly).

- **Alternatives:**
  - **InputFloat with no slider:** lets user type extreme values; the implicit clamp on read still applies but the widget feels less guided. Rejected.
  - **DragFloat for scalars too:** sliders are visually clearer for bounded values like rounding/padding. Sticking with sliders.

### D5: Typography — `Combo` for font_file, `SliderFloat` for font_size_px
The `font_file` widget is an `ImGui::Combo` populated with the three bundled fonts plus a `(custom...)` entry that opens an `InputText` for a hand-entered path. The Combo entries:
1. `Inter Regular` → `fonts/Inter-Regular.ttf`
2. `JetBrains Mono Regular` → `fonts/JetBrainsMono-Regular.ttf`
3. `Lato Regular` → `fonts/Lato-Regular.ttf`
4. `Custom...` → reveals InputText below

If the current `themeFontFile()` doesn't match any bundled name, the Combo shows `(custom)` selected and the InputText is pre-filled with the current value.

`font_size_px` is a `SliderFloat` with range `[6.0, 96.0]` matching the persistence clamp; `step = 0.5`; format `"%.1f px"`.

**Inline "applies on next launch" note** beneath the typography widgets: `ImGui::TextColored(IM_COL32(...gray...), "Font changes apply on next launch. The current view still uses the previously-loaded font.");`

- **Alternatives:**
  - **Free InputText only:** user has to know the bundled font names. Rejected — the bundled options should be discoverable.
  - **File browser dialog:** overkill; future improvement if anyone asks.

### D6: Autosave on edit commit + Reset semantics
Every widget edit autosaves to disk; there is no Save button and no Discard button. One button at the bottom of the Theme tab: **Reset to defaults**.

- **Autosave** — after every ImGui widget call that mutates the in-memory style or typography, check `ImGui::IsItemDeactivatedAfterEdit()` (for `ColorEdit4`, `SliderFloat`, `DragFloat2`, `InputText` — fires once on mouse release / Enter / focus loss). For `Combo`, the function's bool return-on-change is the commit signal. On any commit, call `app::writeThemeToConfig(ImGui::GetStyle())`. The result: at most one disk write per "edit transaction" (a drag, a Combo pick, a text-field commit) — not per frame.
- **Reset to defaults** — calls `app::applyTheme(ImGui::GetStyle())` then immediately `app::writeThemeToConfig(ImGui::GetStyle())`. The disk file is overwritten with the curated baseline so the next launch picks up the reset state.

No "are you sure?" confirmation on Reset — the user can re-edit any widget to recover. The pairing of "instant persistence" + "Reset reverts to defaults" eliminates the staging/commit distinction that made Save/Discard meaningful in the original design.

**Why autosave over Save button:**
- The dialog is non-modal; the user expects their visible-live-preview changes to "just be saved" without an extra click.
- Without autosave, closing the window via the X is destructive (any edits since the last Save are lost). That trap is the kind of UX bug Save buttons exist to highlight, but here we'd rather remove the trap than highlight it.
- `theme.json` is small (~3 KB); writing it on commit is cheap and the existing `writeThemeToConfig` already does atomic temp+rename so a mid-write process crash leaves the previous file intact.

- **Alternatives:**
  - **Save button + autosave (belt-and-suspenders):** clutter, no real value.
  - **Debounced autosave (e.g., 500ms after last edit):** added complexity for marginal gain since commit-level writes are already once-per-transaction.
  - **Cancel-Save modal pattern with staging buffer:** doubles the state surface. Rejected — live preview wants the in-memory style to BE the editable state.

### D7: General tab — read-only diagnostics, no widgets
Just `ImGui::Text` lines. The Vulkan-specific bits (API version, GPU name) come from the existing `app::RenderContext` (which carries `VkPhysicalDevice` and `VkInstance`) — query `vkGetPhysicalDeviceProperties` once per dialog-render or cache via a one-shot lookup.

To avoid a Vulkan call per frame, cache the GPU name + API version in a file-scope static in `Preferences.cpp`, populated lazily on first General-tab open.

Lines shown:
- `imgui-shell` (bold via PushFont if we had bold — but we don't, so just Text)
- `Version: 0.1.0` (from CMake project version macro, exposed via target_compile_definitions)
- `Platform: <IMGUI_SHELL_PLATFORM_NAME>` (existing compile-def)
- `Build: Debug` (from `_DEBUG`/`NDEBUG`)
- `ImGui: <IMGUI_VERSION>` (existing)
- `Vulkan: 1.2 (<actual API version from physical device>)`
- `GPU: <vkGetPhysicalDeviceProperties.deviceName>`
- `Font rasterizer: <IMGUI_SHELL_FONT_BACKEND>` (existing)
- `Config file: <app::themeConfigPath()>` (existing accessor — shows where Save writes)

- **Alternatives:**
  - **Editable General settings (e.g., a checkbox to toggle validation layers):** validation layers are a build-time decision; runtime toggle is out of scope.

### D8: Window state — file-scope static, no ini persistence
`bool g_showPreferencesWindow = false;` at file scope in `Preferences.cpp`. Set to `true` when `File > Preferences...` is clicked; the dialog renders when `true`; closing the window via the X button sets it back to `false` (ImGui handles this via the `p_open` parameter to `Begin`). No persistence — process restart starts with the dialog closed. Aligns with the existing `io.IniFilename = nullptr` pattern.

### D9: About tab — flat list of credits, no widgets
Static `ImGui::Text` / `ImGui::TextWrapped` lines:
- `imgui-shell` + version
- `© 2026 imgui-shell contributors`
- License: `TBD — see project README` (placeholder until project license is locked)
- `Built with:`
  - Dear ImGui (`IMGUI_VERSION`) — MIT
  - Inter font (Rasmus Andersson) — SIL OFL 1.1
  - JetBrains Mono — SIL OFL 1.1
  - Lato (Łukasz Dziedzic) — SIL OFL 1.1
  - FreeType — FreeType License / GPLv2
  - GLFW (3.4) — zlib/libpng
  - nlohmann/json (v3.11.3) — MIT
  - Vulkan loader + MoltenVK — Apache 2.0

## Risks / Trade-offs

- **[Risk: master-detail layout breaks at very narrow widths]** → Mitigation: `BeginTable` with `Resizable` lets the user manage proportions. Min table width via `ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver)` ensures a reasonable default; below ~500 px the table column widths get cramped but it's still functional.
- **[Risk: Save while a widget is mid-drag captures a value the user didn't mean to keep]** → Acceptable: widgets commit continuously in ImGui's immediate-mode model. The user is responsible for clicking Save deliberately.
- **[Risk: Reset to defaults destroys hours of tweaking with no recovery]** → Acceptable trade-off (no confirmation per D6). Save discipline before Reset is the user's responsibility; the Discard alternative re-reads disk if they want to roll back to last-saved.
- **[Risk: The General tab's lazy Vulkan-properties caching uses a static and isn't thread-safe]** → ImGui is single-threaded; the cache is only ever touched from the same thread. Acceptable.
- **[Trade-off: typography font_file changes look broken because they don't take effect until relaunch]** → Mitigation: inline gray-text note next to the widgets explicitly states this. Future change can add live atlas rebuild.
- **[Trade-off: ~50 items in the left list scrolls instead of being categorized into sub-tabs]** → Acceptable. Separator headings group visually; the scroll is short.

## Migration Plan

Not applicable. New UI surface; no prior state to migrate. Rollback = revert this change; the dialog disappears, persistence still works via hand-edited JSON.

## Open Questions

- **Should the dialog have a "Cancel" button alongside Save?** The current design has Save / Discard / Reset. A literal Cancel button would close the window AND discard unsaved changes. The X button + Discard combination covers this two-step. Adding Cancel is minor; can be done post-archive if requested.
- **Should the font Combo show ALL .ttf files under `assets/fonts/`** dynamically, or just the three I hard-coded? Hard-coded is simpler and matches what `add-typography-persistence` shipped. Future: enumerate the directory.
- **Should the About tab display the current commit SHA?** Useful for support; requires a CMake-time generated header. Defer to a future change.
- **Should the General tab include a button to open the config file path in the OS file manager?** Convenient; small extra code via `system("open <path>")` on macOS, `explorer <path>` on Windows, `xdg-open <path>` on Linux. Maybe worth doing inline as a small bonus task. Decide during tasks.md.
