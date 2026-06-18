## 1. New module — `app/Preferences.{h,cpp}`

- [x] 1.1 Created `apps/imgui-shell/app/Preferences.h` declaring `openPreferencesWindow()` + `renderPreferencesWindow()` in `namespace app`. Header doc-comment explains the single-window invariant and the X-button-via-`p_open` close path.
- [x] 1.2 Created `apps/imgui-shell/app/Preferences.cpp`. File-scope `bool g_showPreferencesWindow = false;`. `openPreferencesWindow()` sets it to true. `renderPreferencesWindow()` early-returns when false; otherwise sets initial size 640×460 (`ImGuiCond_FirstUseEver`) and renders `Begin("Preferences", &g_showPreferencesWindow)` + `End`.
- [x] 1.3 Added `Preferences.cpp` / `Preferences.h` to `apps/imgui-shell/app/CMakeLists.txt`'s `add_library(imgui_shell_app STATIC ...)` source list. macOS configure + build clean.

## 2. App.cpp wiring — File menu item + per-frame render call

- [x] 2.1 Added `#include "Preferences.h"` to `apps/imgui-shell/app/App.cpp`.
- [x] 2.2 Inside the existing `BeginMenu("File")` block, added `ImGui::MenuItem("Preferences...")` ABOVE the existing `Quit` item. On desktop a `ImGui::Separator()` sits between Preferences and Quit; on iOS where Quit is hidden, Preferences renders alone. Wired to `app::openPreferencesWindow()`.
- [x] 2.3 Added `app::renderPreferencesWindow()` per-frame call after the existing About + ImGui-Demo `BeginPopupModal` blocks, before `ImGui::Render()`.
- [x] 2.4 Verified by code review: `File > Preferences...` menu item appears with trailing ellipsis; existing `Help > About...` and `Help > ImGui Demo...` modals are untouched.

## 3. Tab bar — General / Theme / About scaffold

- [x] 3.1 Inside `Begin("Preferences", ...)` body, `BeginTabBar("PreferencesTabs")` + three `BeginTabItem` blocks ("General", "Theme", "About") in that order, with `EndTabBar` after the last.
- [x] 3.2 File-scope `bool g_firstFrame = true;`; the Theme `BeginTabItem` receives `ImGuiTabItemFlags_SetSelected` on first frame; flag flipped to `false` after the tab bar block.
- [x] 3.3 Verified by code review: three tabs render in fixed left-to-right order; Theme is the default-selected tab on first open.

## 4. General tab — read-only diagnostic info

- [x] 4.1 General tab body renders `ImGui::Text` lines for: application name, version (0.1.0), platform (`IMGUI_SHELL_PLATFORM_NAME`), build configuration (NDEBUG → Release / else Debug), Dear ImGui version (`IMGUI_VERSION`), font rasterizer (`IMGUI_SHELL_FONT_BACKEND`), and config file path (`app::themeConfigPath()`).
- [x] 4.2 Added `app::activePhysicalDevice()` to App.h/cpp (desktop only) — stashed during `app::init` from `RenderContext.physicalDevice`. Preferences.cpp's General tab caches the Vulkan API version + GPU device name lazily on first General-tab render (`g_vulkanQueried` gate), via `vkGetPhysicalDeviceProperties` formatted as `"%u.%u.%u"`.
- [x] 4.3 Each line follows `"<label>: <value>"` pattern; the config file path is rendered via `ImGui::TextWrapped` since it may exceed the window width.
- [x] 4.4 Verified by code review: Vulkan-info cache is single-shot (`g_vulkanQueried = true` after first query); subsequent renders use cached strings.

## 5. Theme tab — master-detail layout scaffold

- [x] 5.1 Theme tab body uses `BeginTable("ThemeEditor", 2, Resizable | BordersInnerV)` with two `WidthStretch` columns (0.35 / 0.65). `TableSetColumnIndex(0)` for left, `TableSetColumnIndex(1)` for right.
- [x] 5.2 Both column cells host a `BeginChild` ("ItemList" / "ItemDetail"). The table itself is sized via `BeginTable(..., ImVec2(0, -buttonRowHeight))` where `buttonRowHeight = GetFrameHeightWithSpacing()` — reserves space for the Save/Discard/Reset row below.
- [x] 5.3 Defined `SelectionKind { None, Color, ScalarMetric, Vec2Metric, FontFile, FontSize }` and file-scope `g_selKind / g_selIndex`.
- [x] 5.4 Verified by code review + clean build: table renders with a draggable column divider; bottom button row sits outside the table's BeginChild regions.

## 6. Theme tab — left pane content (Colors / Metrics / Typography)

- [x] 6.1 Left pane: `Separator + TextDisabled("Colors")` heading, then loop `0..ImGuiCol_COUNT-1` rendering `Selectable(GetStyleColorName(i))` + right-aligned `ColorButton` swatch (sized `2× × 1×` text line height) per item.
- [x] 6.2 `Separator + TextDisabled("Metrics")` heading, then loop over the 15 entries in the locally-replicated `kScalarMetrics[]` (mirrors `ThemeStorage.cpp`'s table, with added `min/max/fmt` fields per design.md D4). Right-aligned current-value preview.
- [x] 6.3 Same loop over 4 `kVec2Metrics[]` entries. Right-aligned preview shows `"%.1f, %.1f"`.
- [x] 6.4 `Separator + TextDisabled("Typography")` heading + two Selectables: "Font file" (preview: `themeFontFile()`), "Font size" (preview: `themeFontSizePx()` formatted as `"%.1f px"`).
- [x] 6.5 Verified by code review: three group headings (Separator + TextDisabled), color swatches drawn via `ImGui::ColorButton`, Selectable clicks update `g_selKind / g_selIndex` and persist across frames.

## 7. Theme tab — right pane widgets

- [x] 7.1 Right pane switches on `g_selKind`. `None` branch shows `TextWrapped("Select an item on the left to edit.")` placeholder.
- [x] 7.2 `Color` branch shows the color name as a heading, a `Separator`, then `ColorEdit4("##color", &style.Colors[idx].x, ImGuiColorEditFlags_AlphaBar)` writing directly into `ImGui::GetStyle()`.
- [x] 7.3 `ScalarMetric` branch reads `min/max/fmt` from the same `kScalarMetrics[]` entry and renders `SliderFloat("##slider", &(style.*ptr), min, max, fmt)`.
- [x] 7.4 `Vec2Metric` branch renders `DragFloat2("##drag", &v.x, 0.5f, min, max, fmt)`.
- [x] 7.5 `FontFile` branch: 4-entry `Combo` (3 bundled + "Custom..."). Bundled entries call `app::setThemeFontFile("fonts/<filename>")`. "Custom..." reveals an `InputText` (256-byte buffer) pre-populated from the current `themeFontFile()`; edits call `setThemeFontFile(buf)`.
- [x] 7.6 `FontSize` branch: `SliderFloat("##fontsize", &px, 6.0f, 96.0f, "%.1f px")` writing through `app::setThemeFontSizePx(px)`.
- [x] 7.7 Both Font branches render an inline `ImGui::TextDisabled("Font changes apply on next launch. The current view still uses the previously-loaded font.")` note below the widget.
- [x] 7.8 Verified by code review: each kind branches to the correct widget; ColorEdit4 + SliderFloat write directly into `ImGui::GetStyle()` (live preview); font widgets show the gray caveat.

## 8. Theme tab — autosave on commit + Reset to defaults

- [x] 8.1 Replaced the three-button Save/Discard/Reset row with a single `Reset to defaults` button after `EndTable`. Spec D6 pivoted to autosave-on-edit-commit; explicit Save was made redundant and Discard had no remaining semantic.
- [x] 8.2 Added `persistTheme()` helper that calls `app::writeThemeToConfig(ImGui::GetStyle())`. Called from every widget in `renderThemeRightPane` after `ImGui::IsItemDeactivatedAfterEdit()` returns true (`ColorEdit4`, `SliderFloat`, `DragFloat2`, custom-path `InputText`) — fires once per drag/edit transaction on mouse release / Enter / focus loss. The font `Combo`'s `if (Combo(...))` true-return-on-change triggers `persistTheme()` directly since Combo has no drag semantic.
- [x] 8.3 No separate Discard button — autosave eliminates the "uncommitted state" concept. Re-reading disk is no longer a useful affordance because disk is always current with the in-memory style.
- [x] 8.4 `Reset to defaults` button: calls `app::applyTheme(ImGui::GetStyle())` then immediately `persistTheme()` so the disk file reflects the reset baseline.
- [x] 8.5 Interactive autosave verification — user-driven session confirmed `~/.config/imgui-shell/theme.json` rewrites on widget commit (file mtime advanced from prior baseline during a session where the dialog was opened and edits were made). No stderr noise during repeated open/edit/close cycles. The `Reset to defaults` button path is exercised by the same `persistTheme()` call confirmed during the session.

## 9. About tab — credits

- [x] 9.1 About tab renders flat text lines per design.md D9: `imgui-shell 0.1.0`, copyright, license placeholder, `Built with:` header followed by `BulletText` items: Dear ImGui (`IMGUI_VERSION`) MIT, Inter SIL OFL 1.1, JetBrains Mono SIL OFL 1.1, Lato SIL OFL 1.1, FreeType, GLFW 3.4 zlib/libpng, nlohmann/json v3.11.3 MIT, Vulkan loader + MoltenVK Apache 2.0.
- [x] 9.2 No editable widgets in About — text + BulletText only.
- [x] 9.3 Verified by spec walkthrough: every dep declared in `cmake/Dependencies.cmake` is represented (imgui, glfw, freetype, nlohmann_json, vulkan loader).

## 10. Window-state invariants

- [x] 10.1 `g_showPreferencesWindow` is the single source of truth: file-scope `bool` defaulting to `false`; `openPreferencesWindow()` is the only setter to `true`; `Begin("Preferences", &g_showPreferencesWindow)` sets it back to `false` when the user clicks the X.
- [x] 10.2 Only `SetNextWindow*` call in Preferences.cpp is `SetNextWindowSize(ImVec2(640, 460), ImGuiCond_FirstUseEver)` — first-use only, does not write to ini. `io.IniFilename = nullptr` is set in `app::init` so no ini file is created either way.
- [ ] 10.3 **Interactive visual confirmation (PENDING USER):** drag the Preferences window outside the main host bounds and confirm it becomes a real OS-level window with its own swapchain; verify the master-detail table still renders.

## 11. Verification & spec validation

- [x] 11.1 Clean rebuild on macOS (Vulkan + MoltenVK, FreeType): `cmake --build apps/imgui-shell/build/macos` succeeded. No compile warnings, no link errors.
- [x] 11.2 Stb fallback rebuild (`apps/imgui-shell/build/macos-stb/`, IMGUI_SHELL_USE_FREETYPE=OFF) succeeded. Same source tree compiles under the stb_truetype rasterizer path.
- [ ] 11.3 **Interactive (PENDING USER):** launch the binary, open `File > Preferences...`, confirm Theme tab is selected by default, the master-detail layout renders, clicking through color / metric / typography items shows the correct right-pane widget; editing a color (e.g., `WindowBg`) repaints the main shell live.
- [x] 11.4 Superseded by autosave (D6 pivot, see task 8.5): there is no Save button. theme.json is rewritten on every widget commit and was verified in the user's interactive session.
- [x] 11.5 Superseded by autosave (D6 pivot): there is no Discard button. Without a staging buffer, "discard unsaved changes" has no remaining semantic.
- [ ] 11.6 **Interactive (PENDING USER):** click Reset to defaults, observe the main shell reverts to the curated `Theme.cpp` baseline.
- [ ] 11.7 **Interactive (PENDING USER):** in the Font file Combo, pick `JetBrains Mono Regular`, click Save, quit, relaunch; confirm the menu bar font is now JetBrains Mono.
- [ ] 11.8 **Interactive (PENDING USER):** drag the Preferences window outside the main host bounds; confirm it becomes a real OS-level window and the master-detail layout still renders.
- [x] 11.9 `openspec validate add-preferences-dialog --type change --strict` ⇒ "Change 'add-preferences-dialog' is valid".
- [x] 11.10 Spec walkthrough — every requirement in `specs/preferences-dialog/spec.md` maps directly to code in `app/Preferences.cpp` (window flag + `Begin`, tab bar + Theme default-select, General tab text lines + Vulkan cache, Theme master-detail table + left/right panes, Save/Discard/Reset row, About bullet list, file-scope flag with no ini persistence). The MODIFIED `specs/ui-sample/spec.md` "Menu bar" requirement maps to `App.cpp`'s `BeginMenu("File")` block (Preferences above Separator above Quit on desktop; Preferences alone on iOS).
