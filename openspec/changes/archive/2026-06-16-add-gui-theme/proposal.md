## Why

The v1 shell calls `ImGui::StyleColorsDark()` and uses ImGui's default style metrics — a serviceable but generic look that announces "this is an ImGui example, not a curated product." With four archived changes worth of platform polish (Vulkan/Metal/iOS, FreeType, clean shutdown, live resize) behind the shell, the visible aesthetic is now the laggard. A purpose-tuned dark palette and a small set of customized `ImGuiStyle` metrics is a tiny code change that visibly elevates first impressions and gives the project a recognizable identity worth iterating on.

## What Changes

- Add `apps/imgui-shell/app/Theme.{h,cpp}` exposing a single public function `app::applyTheme(ImGuiStyle&)` that mutates the passed style in-place — both the `Colors[ImGuiCol_*]` array (~30 of the 55+ slots set explicitly; the rest left at ImGui defaults) and the metric fields (rounding, border sizes, padding/spacing, separator thickness).
- Call `app::applyTheme(ImGui::GetStyle())` from `app::init`, **after** `ImGui::StyleColorsDark()`. Calling Dark first establishes sane defaults for any slot we don't explicitly set; `applyTheme` then overrides what we care about.
- Bake one coherent "imgui-shell dark" look into v1 — no runtime theme switcher, no preset registry, no JSON/INI theme files. (Future change can add those if needed; out of scope here.)
- Specific design points (exact RGB values + metric numbers locked in design.md):
  - **Palette** — cool dark-blue base instead of ImGui's near-black gray; one accent hue (cool blue/cyan family) for active/highlight states; off-white text (not pure `#FFFFFF`); borders at low contrast so they don't compete with content.
  - **Metrics** — moderate corner rounding (4–6 px) for a modern feel; `FrameBorderSize = 1` for subtle widget delineation; slightly more generous `ItemSpacing` and `FramePadding` than ImGui defaults for breathing room; `ScrollbarSize` slightly thinner.
- Update the About dialog to display the theme name (`"imgui-shell dark"`) on a new line alongside the existing rasterizer / platform / version lines — same inspection-driven verification pattern used by previous changes.

Non-goals (called out to bound scope):
- No runtime theme switcher / `Help > Theme > ...` menu.
- No light theme. (Defer to a follow-up if anyone asks.)
- No external theme files (JSON / INI). Theme values are constexpr / static in `Theme.cpp`.
- No per-widget custom overrides — only the global `ImGuiStyle` is touched.
- No font-style changes (font selection is `font-rendering`'s capability; this change only touches colors + metrics).

## Capabilities

### New Capabilities

- `gui-theme`: The application's visual style — the `ImGuiStyle` color palette and metric values that `app::init` applies after `ImGui::StyleColorsDark()`. Owns what the imgui-shell-dark look is (palette + metrics) and the contract that it's applied uniformly across every platform target.

### Modified Capabilities

- `font-rendering`: The existing "Bundled default font" requirement is reworded so that the font asset path and pixel size are sourced from the new theme-owned constants in `app/Theme.h` (`kThemeFontFile`, `kThemeFontSizePx`) rather than a hard-coded path + the `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition. The compile-def + CMake cache var are removed; the theme is now the single source of truth for font family + size, while `font-rendering` remains the owner of HOW the font is loaded (FreeType vs stb, hinting, atlas wiring).


## Impact

- **Code change:** ~80 lines net — one new `app/Theme.h` (compact public API), one new `app/Theme.cpp` (the actual palette + metric values), three lines in `app/App.cpp`'s `init` to call `applyTheme`, one line added to the About dialog body, one line in `app/CMakeLists.txt` to add Theme.{h,cpp} to the library sources.
- **Spec change:** new `openspec/specs/gui-theme/spec.md` (ADDED) — owns palette, metrics, and the typography defaults (`kThemeFontFile`, `kThemeFontSizePx`, `kThemeTextColor`). MODIFIED `openspec/specs/font-rendering/spec.md` — the "Bundled default font" requirement now references the theme constants. `ui-sample` is untouched.
- **New deps:** none. ImGui already gives us everything we need.
- **CI:** no workflow changes. The theme is purely a runtime style application — no new build inputs.
- **Risk:** entirely cosmetic. The theme can be tuned indefinitely without changing the shape of `applyTheme`. Worst case is an aesthetic that ages badly; the fix is one diff in `Theme.cpp`.
- **Backward compatibility:** none required — this is v1's first theme decision, and no consumer depends on the default ImGui look.
