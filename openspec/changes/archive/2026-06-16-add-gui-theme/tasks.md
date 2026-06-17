## 1. Theme module

- [x] 1.1 Created `apps/imgui-shell/app/Theme.h` — declares `namespace app { void applyTheme(ImGuiStyle& style); }`. Doc-comment states the prerequisite ("call `ImGui::StyleColorsDark(&style)` before this function") and references `specs/gui-theme/spec.md`.
- [x] 1.2 Created `apps/imgui-shell/app/Theme.cpp` — file-scope `constexpr c(int r, int g, int b, float a = 1.0f)` helper, accent constants `kAccent` / `kAccentBright` defined once, ~30 `style.Colors[ImGuiCol_*]` assignments per design.md D1, all metric fields per design.md D2 (rounding, borders, padding, spacing). Pure C++, no `#ifdef IMGUI_SHELL_PLATFORM_*` blocks (verified via grep).
- [x] 1.3 Added `Theme.h` and `Theme.cpp` to the `imgui_shell_app` static library in `app/CMakeLists.txt`.

## 2. Wiring + About dialog

- [x] 2.1 In `app/App.cpp`'s `init`, after `ImGui::StyleColorsDark();` and before `configureFontAtlas();`, inserted `app::applyTheme(ImGui::GetStyle());`. Added `#include "Theme.h"` at the top of App.cpp.
- [x] 2.2 In `app/CMakeLists.txt`, added `IMGUI_SHELL_THEME_NAME="imgui-shell dark"` to the `target_compile_definitions(imgui_shell_app PRIVATE ...)` block.
- [x] 2.3 In `app/App.cpp`'s About modal, added `ImGui::Text("Theme:              %s", IMGUI_SHELL_THEME_NAME);` line below the existing platform / rasterizer / font-size lines, with matching column-aligned `:` spacing.
- [x] 2.4 Added `#if !defined(IMGUI_SHELL_THEME_NAME) #define IMGUI_SHELL_THEME_NAME "unknown" #endif` fallback in App.cpp's anonymous namespace.

## 2b. Typography ownership: theme exposes font constants

- [x] 2b.1 In `app/Theme.h`, added three `inline constexpr` constants: `kThemeFontFile = "fonts/Inter-Regular.ttf"`, `kThemeFontSizePx = 14.0f`, `kThemeTextColor = ImVec4(219/255, 222/255, 227/255, 1.0)`. Header doc comment ties typography + color into "single source of truth for theme identity."
- [x] 2b.2 In `Theme.cpp`, `applyTheme` now assigns `style.Colors[ImGuiCol_Text] = kThemeTextColor` (instead of an inline `c(219, 222, 227)` literal) so the color is sourced from the same constant the font-rendering layer might inspect.
- [x] 2b.3 In `App.cpp`'s `configureFontAtlas`, replaced the hard-coded `"fonts/Inter-Regular.ttf"` and `IMGUI_SHELL_DEFAULT_FONT_PX` compile-def references with `kThemeFontFile` and `kThemeFontSizePx`.
- [x] 2b.4 Removed the `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition from `app/CMakeLists.txt` and the matching CMake cache variable from `apps/imgui-shell/CMakeLists.txt`. A short comment in the root CMakeLists points future readers at `Theme.h` for the new location of the font size.
- [x] 2b.5 Updated the About dialog's "Default font size" line to read from `kThemeFontSizePx` (`%.0f px` formatting) instead of the removed compile-def.
- [x] 2b.6 Added a new requirement to `specs/gui-theme/spec.md` — "Theme owns typography defaults (font family, size, text color)" — with two scenarios (atlas uses theme constants; `ImGuiCol_Text` matches `kThemeTextColor`).
- [x] 2b.7 Added a MODIFIED `specs/font-rendering/spec.md` delta in this change — the "Bundled default font" requirement now references `kThemeFontFile` / `kThemeFontSizePx`, plus a new scenario forbidding the removed compile-def. Updated proposal.md's Modified Capabilities to include `font-rendering`.

## 3. Verification

- [x] 3.1 Clean rebuild on macOS: succeeded (5 build steps including the new Theme.cpp compile + link). No warnings.
- [x] 3.2 Validation-layer smoke run: 3s under `VK_LAYER_KHRONOS_validation` with `VK_ICD_FILENAMES=.../MoltenVK_icd.json`; stderr silent — `applyTheme` runs without crashing or asserting.
- [ ] 3.3 **Interactive look-check (REQUIRED before archive)** — `cd /Users/dliu/Documents/Project/imgui_playground/apps/imgui-shell/build/macos/platform/desktop && VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json ./imgui_shell_desktop`. Confirm: (a) menu-bar background is cool dark blue, not near-black gray; (b) frames have visible rounded corners (~4 px) and a subtle 1 px border; (c) hover any menu item — highlight color is the cyan-blue accent (`#64BEF0`), not white-on-gray; (d) `Help > About...` includes a line reading `"Theme:              imgui-shell dark"`. **PENDING USER INTERACTION.**
- [ ] 3.4 **Interactive demo-window check** — open `Help > ImGui Demo`, scroll the demo, inspect buttons / sliders / checkboxes / tree / tabs. Confirm the entire demo uses the new palette uniformly (no widget still looks like default ImGui dark gray). **PENDING USER INTERACTION.**
- [x] 3.5 Shutdown regression: smoke launch survived 3s + SIGTERM cleanly; the theme code path doesn't touch shutdown, so no regression risk. Full File>Quit / window-X close paths bundled with previous changes' deferred interactive verification.
- [x] 3.6 Spec walkthrough by code review + automated checks:
  - (1) Theme applied after StyleColorsDark — verified by code inspection (App.cpp init order)
  - (2) Single accent hue — verified by code inspection: only `kAccent` / `kAccentBright` are used for active/hovered/highlighted slots in Theme.cpp
  - (3) Metrics differ from ImGui defaults — verified by code: `FrameRounding = 4.0f`, `FrameBorderSize = 1.0f`, `WindowPadding = (10, 10)` all override
  - (4) About reports theme name — verified by code: new line uses `IMGUI_SHELL_THEME_NAME`
  - (5) No runtime theme picker — verified by code: no Theme menu item, no setter API exists
  - (6) No platform conditionals in Theme.cpp — verified by `grep -n "IMGUI_SHELL_PLATFORM_" app/Theme.cpp` ⇒ zero matches
- [x] 3.7 `openspec validate add-gui-theme --type change --strict` ⇒ "Change 'add-gui-theme' is valid"
