## REMOVED Requirements

### Requirement: Font configuration is a build-time decision
**Reason**: Stale since `add-theme-persistence` (archived 2026-06-17) enabled runtime persistence of color + metric values. Typography (font file + pixel size) is now also runtime-overridable via the `typography` section of `theme.json` per the MODIFIED "JSON schema and parsing rules" requirement in `theme-persistence` (this change). The blanket "build-time decision" framing is no longer accurate; preserving it would contradict shipped behavior.

**Migration**: The hinting / rasterizer flags (`ImGuiFreeTypeBuilderFlags_LightHinting`, the choice between FreeType and stb_truetype, the `IMGUI_SHELL_USE_FREETYPE` CMake option) remain build-time decisions and are covered by the `font-rendering` "Default hinting configuration" and "FreeType-based rasterization is the default" requirements, which stay in force. What changed is only the *font file* and *font pixel size* — those move from build-time-only to "build-time default, user-overridable via `theme.json` `typography` section."

## MODIFIED Requirements

### Requirement: Bundled default font
The application SHALL bundle `Inter-Regular.ttf` (under `apps/imgui-shell/assets/fonts/`) AND zero or more alternative TrueType font assets (currently `JetBrainsMono-Regular.ttf` and `Lato-Regular.ttf`) under the same directory. In `app::init` the application SHALL load the font identified by `app::themeFontFile()` at the pixel size returned by `app::themeFontSizePx()` — both default to Inter / 14 px when no `theme.json` override is present, and MAY be overridden by `theme.json`'s `typography` section (see `theme-persistence` spec). All bundled fonts SHALL appear unmodified at their canonical names to comply with their respective licenses (SIL OFL 1.1 reserved-font-name clauses for Inter, JetBrains Mono, and Lato).

#### Scenario: Default font loaded on init
- **WHEN** `app::init` runs in a default build with no `theme.json` `typography` override
- **THEN** `app::themeFontFile()` SHALL return the asset-relative path `"fonts/Inter-Regular.ttf"` (the default constant)
- **AND** `app::themeFontSizePx()` SHALL return `14.0f`
- **AND** `io.Fonts->AddFontFromFileTTF` SHALL be called with the resolved path of `themeFontFile()` and a pixel size equal to `themeFontSizePx()`
- **AND** the call SHALL succeed (return a non-null `ImFont*`)
- **AND** subsequent ImGui rendering SHALL use that font as the default

#### Scenario: Override via theme.json applies on next launch
- **WHEN** the user's `theme.json` contains a `typography.font_file` pointing at a different bundled font (e.g., `"fonts/JetBrainsMono-Regular.ttf"`) and/or a different `typography.font_size_px`
- **THEN** at the NEXT launch of the application, `app::themeFontFile()` and `app::themeFontSizePx()` SHALL return the overridden values
- **AND** `configureFontAtlas` SHALL load the overridden font at the overridden size
- **AND** there is no mid-session live-rebuild — values apply at next `app::init` only

#### Scenario: Asset path is resolved per platform
- **WHEN** the binary runs on desktop
- **THEN** any bundled font (Inter / JetBrains Mono / Lato) SHALL be locatable at `<exe-dir>/assets/fonts/<filename>.ttf` (copied by the build system next to the executable)
- **WHEN** the binary runs on iOS
- **THEN** any bundled font SHALL be locatable in the `.app` bundle's `Resources/fonts/` directory

#### Scenario: Asset missing produces a clear error
- **WHEN** `app::init` cannot locate the font file returned by `app::themeFontFile()` at the expected path (whether the default or an override)
- **THEN** the binary SHALL print a clear error message identifying the missing asset path and SHALL fall back to `ImGui::GetIO().Fonts->AddFontDefault()` (Proggy Clean) so the application still launches
- **AND** the fallback path SHALL also be visible in the About dialog (e.g., `"FreeType (fallback font: Proggy)"`)

#### Scenario: Font size source is the theme accessor, not a compile definition
- **WHEN** a contributor wants to change the default font pixel size
- **THEN** they SHALL edit `app::kDefaultThemeFontSizePx` in `app/Theme.h` (the baked-in default that the accessor returns when no override is active)
- **AND** there SHALL NOT be a `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition or CMake cache variable for font size
- **AND** a user wanting to change ONLY their own copy SHALL edit `~/.config/imgui-shell/theme.json`'s `typography.font_size_px` rather than the source code
