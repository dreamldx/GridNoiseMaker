## ADDED Requirements

### Requirement: FreeType-based rasterization is the default
The application SHALL use FreeType (via Dear ImGui's bundled `misc/freetype/imgui_freetype.cpp` backend) as the font rasterizer on every supported platform target (macOS, Windows, Linux, iOS) in the default build configuration.

#### Scenario: Default-build atlas builder is FreeType
- **WHEN** the project is configured with default CMake options on any supported preset
- **THEN** the build SHALL define `IMGUI_ENABLE_FREETYPE`
- **AND** `app::init` SHALL set `io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType()` before the first font is added to the atlas
- **AND** the resulting `imgui_freetype.cpp` SHALL be present in the link map of the desktop binary

#### Scenario: Build-time fallback to stb_truetype
- **WHEN** the project is configured with `-DIMGUI_SHELL_USE_FREETYPE=OFF`
- **THEN** `IMGUI_ENABLE_FREETYPE` SHALL NOT be defined
- **AND** `imgui_freetype.cpp` SHALL NOT be compiled or linked
- **AND** `app::init` SHALL leave `FontBuilderIO` at its default (`nullptr`), causing ImGui to use its built-in `stb_truetype` builder

### Requirement: Bundled default font
The application SHALL bundle exactly one TrueType font asset (`Inter-Regular.ttf` under `apps/imgui-shell/assets/fonts/`) and SHALL load that font in `app::init` at the default pixel size, replacing ImGui's compiled-in Proggy Clean fallback. The font SHALL appear unmodified at its canonical name to comply with the SIL Open Font License 1.1 reserved-font-name clause.

#### Scenario: Default font loaded on init
- **WHEN** `app::init` runs in a default build
- **THEN** `io.Fonts->AddFontFromFileTTF` SHALL be called once with the bundled `Inter-Regular.ttf` and a pixel size of `IMGUI_SHELL_DEFAULT_FONT_PX` (default 14)
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

### Requirement: Default hinting configuration
The application SHALL apply `ImGuiFreeTypeBuilderFlags_LightHinting` as the default builder flag for the bundled font, producing crisp vertically-hinted glyphs without horizontal distortion. LCD subpixel rendering (`ImGuiFreeTypeBuilderFlags_LCD`) SHALL NOT be enabled by default; mono hinting (`ImGuiFreeTypeBuilderFlags_MonoHinting`) SHALL NOT be enabled by default.

#### Scenario: Light hinting is on for the bundled font
- **WHEN** the bundled font is added to the atlas via `AddFontFromFileTTF`
- **THEN** the `ImFontConfig` SHALL have `FontBuilderFlags` set to `ImGuiFreeTypeBuilderFlags_LightHinting`
- **AND** no LCD / Mono flag SHALL be set on that config

### Requirement: Active rasterizer visible in About dialog
The About dialog SHALL display the active font rasterizer (`"FreeType"` or `"stb_truetype"`) so the build configuration is verifiable by inspection on every supported platform.

#### Scenario: About dialog reports FreeType in default build
- **WHEN** a user opens `Help > About...` in a default build
- **THEN** the dialog SHALL include a line that reads `"Font rasterizer: FreeType"` (or equivalent labeled string)

#### Scenario: About dialog reports stb_truetype in fallback build
- **WHEN** a user opens `Help > About...` in a build configured with `-DIMGUI_SHELL_USE_FREETYPE=OFF`
- **THEN** the dialog SHALL include a line that reads `"Font rasterizer: stb_truetype"`

### Requirement: Font configuration is a build-time decision
The application SHALL NOT expose runtime font rasterizer selection, runtime hinting toggles, or a runtime font-picker UI in v1. All font configuration (rasterizer choice, hinting flags, default font, pixel size) SHALL be a build-time decision driven by CMake cache variables and compile-time macros.

#### Scenario: No runtime font picker in v1
- **WHEN** the codebase is grepped for runtime font-changing UI
- **THEN** there SHALL be no menu item, settings panel, or hotkey that selects a different font, rasterizer, or hinting mode at runtime
