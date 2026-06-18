# gui-theme Specification

## Purpose
TBD - created by archiving change add-gui-theme. Update Purpose after archive.
## Requirements
### Requirement: Curated theme applied at init
The application SHALL apply a curated `ImGuiStyle` ("imgui-shell dark") at `app::init` time, after `ImGui::StyleColorsDark()` is called, on every supported platform target (macOS, Windows, Linux, iOS). The theme SHALL override both the color palette (~30 of the 55+ `ImGuiCol_*` slots) and the structural metric fields (rounding, border sizes, padding, spacing). Slots and metrics not explicitly overridden SHALL inherit ImGui's `StyleColorsDark` defaults.

#### Scenario: Theme applied uniformly on every platform
- **WHEN** `app::init` runs on any supported platform
- **THEN** `ImGui::StyleColorsDark(&ImGui::GetStyle())` SHALL be called first to establish sane dark-theme defaults
- **AND** `app::applyTheme(ImGui::GetStyle())` SHALL be called immediately afterward
- **AND** the resulting `ImGui::GetStyle().Colors[ImGuiCol_WindowBg]` SHALL equal the theme's curated window-background color (not ImGui's default dark gray)
- **AND** `ImGui::GetStyle().WindowRounding` SHALL equal the theme's curated rounding value (not ImGui's default of `0.0f`)

#### Scenario: `applyTheme` runs after `StyleColorsDark`, never instead
- **WHEN** any future code in `app/` or `platform/` modifies the ImGui style
- **THEN** the order of operations SHALL be: `CreateContext` → `StyleColorsDark` → `applyTheme` → any additional overrides
- **AND** the host SHALL NOT call `applyTheme` from a context where `StyleColorsDark` has not first been called (`applyTheme` documents this prerequisite in its header comment)

### Requirement: Single-hue accent for active and hovered states
The theme palette SHALL use exactly one accent hue (in the cool blue / cyan family) across all `ImGuiCol_*` slots that represent active, hovered, highlighted, or selected states. The same hue SHALL be used at three brightness tiers (base / hover / active) for visual consistency.

#### Scenario: Active-state slots use the accent hue
- **WHEN** the codebase is inspected against the theme palette in `app/Theme.cpp`
- **THEN** the following slots SHALL share the same accent hue (RGB ratio identical, alpha may vary): `CheckMark`, `SliderGrab`, `ButtonHovered`, `ButtonActive`, `HeaderHovered`, `HeaderActive`, `TabHovered`, `ResizeGripHovered`, `ResizeGripActive`, `SeparatorHovered`, `SeparatorActive`, `TextSelectedBg`, `DragDropTarget`, `NavHighlight`
- **AND** no other hue family SHALL be introduced for emphasis states in v1

### Requirement: Tuned style metrics
The theme SHALL set explicit values for the following `ImGuiStyle` metric fields, all overriding ImGui defaults: `WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`, `IndentSpacing`, `ScrollbarSize`, `GrabMinSize`, `WindowRounding`, `ChildRounding`, `FrameRounding`, `GrabRounding`, `PopupRounding`, `ScrollbarRounding`, `TabRounding`, `WindowBorderSize`, `ChildBorderSize`, `PopupBorderSize`, `FrameBorderSize`, `TabBorderSize`. The exact numeric values are documented in `design.md` D2 and live in `app/Theme.cpp`.

#### Scenario: Metrics differ from ImGui defaults after applyTheme
- **WHEN** `app::applyTheme(style)` returns
- **THEN** `style.FrameRounding` SHALL NOT be `0.0f` (ImGui's default) — it SHALL equal the curated value
- **AND** `style.FrameBorderSize` SHALL be at least `1.0f` (ImGui's default is `0.0f` — borderless frames)
- **AND** `style.WindowPadding`, `style.FramePadding`, `style.ItemSpacing` SHALL each be at least 25% larger than ImGui's default values on at least one axis

### Requirement: Theme name visible in About dialog
The About dialog SHALL display the active theme name on a dedicated line, so the active theme is verifiable by inspection on every supported platform — matching the precedent set by `font-rendering` for the rasterizer name. The theme name string is set via the `IMGUI_SHELL_THEME_NAME` compile-time macro defined by the build system.

#### Scenario: About dialog reports the theme name
- **WHEN** a user opens `Help > About...` in any build
- **THEN** the dialog SHALL include a line of the form `"Theme: <name>"` where `<name>` matches `IMGUI_SHELL_THEME_NAME` (default value: `"imgui-shell dark"`)



### Requirement: Theme owns typography defaults (font family, size, text color)
The theme SHALL define three typography concepts in `app/Theme.h`:
- `kThemeTextColor` — a `constexpr ImVec4` used as the `ImGuiCol_Text` baseline (still a compile-time constant; this slot is also covered by the `colors` section of `theme.json` per `theme-persistence`).
- `kDefaultThemeFontFile` / `kDefaultThemeFontSizePx` — `constexpr` baked-in defaults for the font path and pixel size.
- `app::themeFontFile()` / `app::themeFontSizePx()` — accessor functions returning the CURRENT values (defaults initially; mutable via the matching setters when `theme-persistence` applies a `typography` override).
- `app::setThemeFontFile(std::string)` / `app::setThemeFontSizePx(float)` — setters used by `ThemeStorage`'s `readThemeFromConfig` when a valid `typography` override is parsed; out-of-band callers MAY also call them but are not expected to.

The font-rendering layer's font-atlas configuration SHALL source the font path + pixel size from the accessors (not the `constexpr` defaults directly), so `theme.json` overrides take effect. The theme's `ImGuiCol_Text` slot in `applyTheme` SHALL continue to be assigned from `kThemeTextColor` so the color and the baked-in typography defaults stay in sync within one source of truth.

#### Scenario: Font atlas uses theme accessors, not raw constants
- **WHEN** `app::init` configures the font atlas
- **THEN** `io.Fonts->AddFontFromFileTTF` SHALL be called with a path derived from `app::themeFontFile()` (NOT from `kDefaultThemeFontFile` directly) and a pixel size equal to `app::themeFontSizePx()` (NOT `kDefaultThemeFontSizePx` directly)
- **AND** when no `theme.json` override is active, those accessors return the baked-in default values
- **AND** the call SHALL NOT use the previously-existing `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition (which was removed by `add-gui-theme`)

#### Scenario: Text color in style matches theme constant
- **WHEN** `app::applyTheme(style)` returns
- **THEN** `style.Colors[ImGuiCol_Text]` SHALL be equal to `app::kThemeTextColor`
- **AND** changing the constant in `Theme.h` SHALL be sufficient to change the foreground text color throughout the application (no other code site sets `ImGuiCol_Text` to a literal value)

### Requirement: Theme code lives in the shared core
The theme implementation (`Theme.h` + `Theme.cpp`) SHALL live under `apps/imgui-shell/app/` and be part of the `imgui_shell_app` static library. It SHALL NOT contain any per-platform `#ifdef` blocks (matching the `app-shell` capability's "Identical source set across platforms" requirement); platform-uniform code only.

#### Scenario: No platform conditionals in Theme.cpp
- **WHEN** the codebase is grepped for `#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)` or `#if defined(IMGUI_SHELL_PLATFORM_IOS)` within `app/Theme.cpp`
- **THEN** zero matches SHALL be found

