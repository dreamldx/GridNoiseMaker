## Context

The v1 shell uses ImGui's stock `StyleColorsDark()` and stock `ImGuiStyle` metrics — fine for a working example but generic. The shell now has FreeType-sharpened Inter text, a clean Vulkan-driven render path, and live resize that re-lays-out smoothly; the visual palette is the only remaining "this looks like an unfinished example" tell.

This design locks the actual numbers (RGB triples for each `ImGuiCol_*` slot we set, and metric values for rounding / borders / padding / spacing). Implementation is small (~80 LOC across two new files plus a few-line wire-up); the work that matters here is the aesthetic curation. Sources for the palette are the standard "modern dark UI" conventions: cool dark-blue base instead of pure gray, single accent hue used for active / hovered states, off-white text to reduce eye strain at small body sizes, subtle low-contrast borders. Reference points considered (and adapted, not copied): Visual Studio Code Dark+, GitHub Dark dimmed, the Catppuccin Mocha palette.

The theme is a build-time decision in v1; runtime switching, light variants, and external `.ini`/JSON theme files are explicit non-goals (callable as follow-up changes if anyone asks).

## Goals / Non-Goals

**Goals:**
- Replace ImGui's stock dark look with a curated "imgui-shell dark" palette + metric set that's coherent (single hue family for backgrounds, single accent for emphasis, deliberate contrast hierarchy).
- Keep the implementation behind a single function `app::applyTheme(ImGuiStyle&)` so the rest of `app/` (and future code) doesn't care where the theme comes from.
- Apply uniformly on every supported platform (desktop + iOS) via the shared `app/` core — no per-platform divergence.
- Make the theme verifiable by inspection: the About dialog gains a new line displaying the active theme name.
- Stay strictly within ImGui's existing style surface — no custom render passes, no shaders, no draw-list intercepts.

**Non-Goals:**
- No runtime theme switcher / `Help > Theme > ...` menu.
- No light theme variant.
- No external theme files (JSON, INI, etc.). The palette is `constexpr` / static in `Theme.cpp`.
- No per-widget custom overrides.
- No font / typography changes (that's `font-rendering`'s capability).
- No animated transitions between theme states.

## Decisions

### D1: Palette — cool dark-blue base with a single cyan accent
The full set of `ImGuiCol_*` slots we explicitly set, with their (R, G, B, A) values in normalized 0.0–1.0 floats. Slots not listed below remain at the values `ImGui::StyleColorsDark()` established (called before `applyTheme`).

```
// Background tiers — cool dark blue, three levels
WindowBg              = (0.082, 0.094, 0.122, 1.000)   // base
ChildBg               = (0.082, 0.094, 0.122, 1.000)   // match base
PopupBg               = (0.110, 0.122, 0.153, 1.000)   // one step lighter, slight elevation
FrameBg               = (0.137, 0.153, 0.188, 1.000)   // button/input default
FrameBgHovered        = (0.180, 0.200, 0.240, 1.000)
FrameBgActive         = (0.220, 0.244, 0.290, 1.000)
TitleBg               = (0.067, 0.078, 0.102, 1.000)   // slightly darker than base
TitleBgActive         = (0.110, 0.137, 0.180, 1.000)   // accent-tinged on focus
TitleBgCollapsed      = (0.067, 0.078, 0.102, 0.750)
MenuBarBg             = (0.094, 0.106, 0.137, 1.000)
ScrollbarBg           = (0.082, 0.094, 0.122, 0.000)   // transparent over WindowBg
TabUnfocused          = (0.094, 0.106, 0.137, 1.000)
TabUnfocusedActive    = (0.137, 0.153, 0.188, 1.000)
Tab                   = (0.094, 0.106, 0.137, 1.000)
TabActive             = (0.180, 0.200, 0.240, 1.000)

// Accent — cyan-blue family, three brightness tiers
ScrollbarGrab         = (0.235, 0.275, 0.345, 1.000)
ScrollbarGrabHovered  = (0.295, 0.345, 0.420, 1.000)
ScrollbarGrabActive   = (0.345, 0.420, 0.510, 1.000)
CheckMark             = (0.392, 0.745, 0.941, 1.000)   // accent base
SliderGrab            = (0.392, 0.745, 0.941, 1.000)
SliderGrabActive      = (0.490, 0.835, 0.980, 1.000)
Button                = (0.180, 0.200, 0.240, 1.000)   // frame-default-style
ButtonHovered         = (0.392, 0.745, 0.941, 0.500)   // accent at 50%
ButtonActive          = (0.392, 0.745, 0.941, 0.800)
Header                = (0.180, 0.200, 0.240, 1.000)
HeaderHovered         = (0.392, 0.745, 0.941, 0.350)
HeaderActive          = (0.392, 0.745, 0.941, 0.600)
TabHovered            = (0.392, 0.745, 0.941, 0.500)
ResizeGrip            = (0.235, 0.275, 0.345, 0.800)
ResizeGripHovered     = (0.392, 0.745, 0.941, 0.700)
ResizeGripActive      = (0.490, 0.835, 0.980, 0.900)
SeparatorHovered      = (0.392, 0.745, 0.941, 0.600)
SeparatorActive       = (0.490, 0.835, 0.980, 0.900)
TextSelectedBg        = (0.392, 0.745, 0.941, 0.350)
DragDropTarget        = (0.392, 0.745, 0.941, 0.900)
NavHighlight          = (0.392, 0.745, 0.941, 1.000)

// Lines & subtle structure
Border                = (0.196, 0.220, 0.275, 1.000)
BorderShadow          = (0.000, 0.000, 0.000, 0.000)   // disabled
Separator             = (0.196, 0.220, 0.275, 1.000)

// Text — off-white, not pure white
Text                  = (0.860, 0.870, 0.890, 1.000)
TextDisabled          = (0.450, 0.470, 0.510, 1.000)

// Overlay
ModalWindowDimBg      = (0.000, 0.000, 0.000, 0.600)
NavWindowingDimBg     = (0.000, 0.000, 0.000, 0.300)
NavWindowingHighlight = (0.392, 0.745, 0.941, 0.700)
```

The accent `(0.392, 0.745, 0.941)` (≈ `#64BEF0`) is the only hue used for "this is active/hovered/highlighted" — keeping the visual language tight. Background tiers all sit on the same hue family (cool blue-gray) so the eye reads a coherent surface; the accent then pops without competing.

- **Alternatives:**
  - **Warm dark palette (browns / orange accent)**: harder to read at small body sizes; rejected.
  - **Multiple accent hues** (e.g., green for success, red for error, blue for info): premature — we don't have semantic colors yet; the shell is just menu bar + demo + About.
  - **Pure neutral grayscale**: feasible but bland; the accent gives the look a brand identity at zero cost.

### D2: Metrics — moderate rounding, subtle borders, slightly generous padding

```cpp
style.WindowPadding      = ImVec2(10.0f, 10.0f);
style.FramePadding       = ImVec2( 8.0f,  5.0f);
style.ItemSpacing        = ImVec2( 8.0f,  6.0f);
style.ItemInnerSpacing   = ImVec2( 6.0f,  4.0f);
style.IndentSpacing      = 20.0f;
style.ScrollbarSize      = 12.0f;
style.GrabMinSize        = 10.0f;

style.WindowRounding     = 6.0f;
style.ChildRounding      = 4.0f;
style.FrameRounding      = 4.0f;
style.GrabRounding       = 3.0f;
style.PopupRounding      = 6.0f;
style.ScrollbarRounding  = 8.0f;
style.TabRounding        = 4.0f;

style.WindowBorderSize   = 1.0f;
style.ChildBorderSize    = 1.0f;
style.PopupBorderSize    = 1.0f;
style.FrameBorderSize    = 1.0f;
style.TabBorderSize      = 0.0f;
style.SeparatorTextBorderSize = 2.0f;
```

The choices: rounding ≈ 4–6 px reads as "modern but not playful"; `FrameBorderSize = 1` plus a low-contrast `Border` color gives subtle delineation without visual noise; `TabBorderSize = 0` keeps tabs clean; padding/spacing values are about 25% more generous than ImGui defaults — enough to feel "designed" without wasting screen space.

- **Alternatives:**
  - **Sharp corners (rounding = 0)**: a fine alternative aesthetic; rejected because the bundled Inter font already has soft glyphs, and rounded frames pair better.
  - **Heavy rounding (8–10 px)**: gets cartoonish at our default body size (14 px). Tested mentally and rejected.
  - **No borders** (`*BorderSize = 0`): cleaner but loses widget separation at small sizes.

### D3: Theme name exposed via `IMGUI_SHELL_THEME_NAME` compile-def
Same pattern as `IMGUI_SHELL_FONT_BACKEND` and `IMGUI_SHELL_PLATFORM_NAME`. Defined in `app/CMakeLists.txt` as `IMGUI_SHELL_THEME_NAME="imgui-shell dark"`. The About dialog in `app/App.cpp` reads it via the macro and adds one new line: `ImGui::Text("Theme: %s", IMGUI_SHELL_THEME_NAME);`. No other consumer.

- **Alternatives:**
  - **Return the name from `app::applyTheme`**: would require the caller to remember the string. Less ergonomic.
  - **Hard-code the string in App.cpp's About body**: works but lets the theme name and theme code drift.

### D4: `applyTheme` is called AFTER `ImGui::StyleColorsDark()`, not instead of
`StyleColorsDark` sets *every* slot in `ImGuiStyle::Colors[]` plus reasonable metric defaults. Our `applyTheme` then overrides only the slots / metrics we care about, leaving the rest at sane dark-theme defaults. This is more robust than calling `applyTheme` standalone — any new `ImGuiCol_*` slot ImGui adds in a future release inherits the dark default automatically; the worst case is "new slot looks dark and generic" rather than "new slot looks broken / out of palette".

### D5: `app::applyTheme(ImGuiStyle&)` takes a non-const reference, not the global directly
Signature: `void applyTheme(ImGuiStyle& style)`. Caller passes `ImGui::GetStyle()`. Reasons: (a) the function is trivially testable without an ImGui context; (b) symmetry with how ImGui's own `StyleColorsDark(ImGuiStyle* dst)` works (it takes an explicit destination); (c) future "preview the theme in a sub-window" experiments could call `applyTheme` on a local `ImGuiStyle` copy.

### D6: Color literals via a tiny constexpr helper, not raw `ImVec4` everywhere
Two-line helper:
```cpp
constexpr ImVec4 c(int r, int g, int b, float a = 1.0f) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}
```
Used in the actual `Theme.cpp` so the palette table reads as 8-bit RGB (more familiar than 0.0–1.0 floats) plus alpha. The table reads `c(21, 24, 31)` instead of `ImVec4(0.082f, 0.094f, 0.122f, 1.0f)` — easier to eyeball, easier to compare against hex tools, easier to tweak.

## Risks / Trade-offs

- **[Risk: the chosen palette ages poorly or doesn't fit the project's eventual direction]** → Acceptable: changing the palette is a single-file edit, no API breakage, no spec rewrite (the spec specifies the *contract* — there is one applied theme — not the exact RGB values, which are listed as illustrative).
- **[Risk: contributors hate the choice and want to swap themes mid-session]** → Mitigation: documented as a future-change candidate. The single-function abstraction (`applyTheme`) is exactly the seam needed to add a registry/switcher later without rewriting.
- **[Risk: accent color has insufficient contrast against background for accessibility]** → Tested by inspection: accent `#64BEF0` vs background `#15181F` has a WCAG contrast ratio ~8:1 — well above the 4.5:1 AA threshold. Text `#DBDEE3` vs `WindowBg` is ~12:1.
- **[Risk: ImGui adds a new `ImGuiCol_*` slot in a future version that doesn't fit our palette]** → Mitigation: D4 calls `StyleColorsDark` first, so any new slot inherits ImGui's dark default. Worst case is one out-of-palette widget until we add the slot to `applyTheme`.
- **[Trade-off: bundling exact RGB values into a header file means a tweak is a code change, not a config change]** → Acceptable in v1 (non-goal: external theme files). When/if someone needs runtime tweakability, the next change adds a JSON loader.

## Migration Plan

Not applicable — first theme decision. Rollback = revert this change; the default `StyleColorsDark()` look comes back. No persisted state, no API change, no data migration. The `applyTheme` function disappears cleanly.

## Open Questions

- **Should the accent color be exposed as a public `app::kAccentColor` constant?** A future custom widget (node-graph editor, etc.) might want to draw an active outline in the accent hue and shouldn't re-derive it from `ImGui::GetStyle().Colors[ImGuiCol_CheckMark]`. Lean yes, but not blocking — easy to add when the first consumer appears.
- **Should the metrics use a "compact" vs "comfortable" toggle?** Some users prefer denser UIs. Out of scope for v1; if the request comes up, the right shape is a CMake cache var `IMGUI_SHELL_THEME_DENSITY` that selects between two `applyTheme` variants.
- **Theme name string** — locked at `"imgui-shell dark"` here. If the project gets a real name later, this string updates in one place.
