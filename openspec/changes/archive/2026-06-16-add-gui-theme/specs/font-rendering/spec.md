## MODIFIED Requirements

### Requirement: Bundled default font
The application SHALL bundle exactly one TrueType font asset (`Inter-Regular.ttf` under `apps/imgui-shell/assets/fonts/`) and SHALL load that font in `app::init` at the pixel size specified by the theme (`app::kThemeFontSizePx` in `app/Theme.h`, default `14.0f`), replacing ImGui's compiled-in Proggy Clean fallback. The font asset path is sourced from the theme constant `app::kThemeFontFile`. The font SHALL appear unmodified at its canonical name to comply with the SIL Open Font License 1.1 reserved-font-name clause.

#### Scenario: Default font loaded on init
- **WHEN** `app::init` runs in a default build
- **THEN** `io.Fonts->AddFontFromFileTTF` SHALL be called once with the bundled font at the path derived from `app::kThemeFontFile` and a pixel size equal to `app::kThemeFontSizePx`
- **AND** the call SHALL succeed (return a non-null `ImFont*`)
- **AND** subsequent ImGui rendering SHALL use that font as the default

#### Scenario: Asset path is resolved per platform
- **WHEN** the binary runs on desktop
- **THEN** the bundled `Inter-Regular.ttf` SHALL be locatable at `<exe-dir>/assets/fonts/Inter-Regular.ttf` (copied by the build system next to the executable)
- **WHEN** the binary runs on iOS
- **THEN** the bundled `Inter-Regular.ttf` SHALL be locatable in the `.app` bundle's `Resources/` directory and SHALL be accessed via the iOS host's bundle-resource path

#### Scenario: Asset missing produces a clear error
- **WHEN** `app::init` cannot locate the bundled font file at the expected path
- **THEN** the binary SHALL print a clear error message identifying the missing asset path and SHALL fall back to `ImGui::GetIO().Fonts->AddFontDefault()` (Proggy Clean) so the application still launches
- **AND** the fallback path SHALL also be visible in the About dialog (e.g., `"FreeType (fallback font: Proggy)"`)

#### Scenario: Font size source is the theme, not a compile definition
- **WHEN** a contributor wants to change the default font pixel size
- **THEN** they SHALL edit `app::kThemeFontSizePx` in `app/Theme.h`
- **AND** there SHALL NOT be a `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition or CMake cache variable for font size (those were removed in favor of the theme constant)
