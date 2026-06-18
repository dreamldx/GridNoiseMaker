# font-rendering Specification

## Purpose
TBD - created by archiving change add-font-antialiasing. Update Purpose after archive.
## Requirements
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

### Requirement: Font atlas can be rebuilt at runtime
The application SHALL provide `app::rebuildFontAtlas()` which clears and re-rasterizes the ImGui font atlas using the current `app::themeFontFile()` and `app::themeFontSizePx()` values, and coordinates with the active renderer backend to destroy and recreate the GPU-side font texture. After the call returns, subsequent ImGui rendering SHALL use the newly-rasterized atlas (no relaunch required). The function SHALL be a silent no-op when called before `app::registerRebuildFontAtlasCallback` has been invoked.

#### Scenario: Rebuild call applies new font live
- **WHEN** `app::setThemeFontSizePx(24.0f)` runs followed by `app::rebuildFontAtlas()`
- **THEN** `io.Fonts->Clear()` SHALL be invoked
- **AND** `app::configureFontAtlas()` SHALL be re-invoked, re-adding the font at the new size
- **AND** the renderer backend's font texture SHALL be recreated (`ImGui_ImplVulkan_CreateFontsTexture` on desktop, `ImGui_ImplMetal_CreateFontsTexture` on iOS)
- **AND** the very next ImGui frame SHALL render text at the new pixel size

#### Scenario: Rebuild waits for GPU idle on desktop
- **WHEN** `app::rebuildFontAtlas()` is invoked on desktop
- **THEN** the registered callback SHALL call `vkDeviceWaitIdle(device)` BEFORE `ImGui_ImplVulkan_DestroyFontsTexture()`
- **AND** the old font texture SHALL NOT be released while any in-flight frame references it
- **AND** after `DestroyFontsTexture + io.Fonts->Clear + configureFontAtlas + CreateFontsTexture`, the new texture SHALL be uploaded to GPU memory and bound for subsequent draws

#### Scenario: Rebuild on iOS coordinates via Metal texture lifecycle
- **WHEN** `app::rebuildFontAtlas()` is invoked on iOS
- **THEN** the registered callback SHALL call `ImGui_ImplMetal_DestroyFontsTexture()` then `io.Fonts->Clear()` then `app::configureFontAtlas()` then `ImGui_ImplMetal_CreateFontsTexture(device)`
- **AND** Metal's reference-counted texture lifecycle SHALL keep the previous texture alive until any in-flight command buffer referencing it completes
- **AND** subsequent frames SHALL use the new texture

#### Scenario: Rebuild without callback is a no-op
- **WHEN** `app::rebuildFontAtlas()` is invoked before any host has called `app::registerRebuildFontAtlasCallback`
- **THEN** the function SHALL return immediately without modifying `io.Fonts`
- **AND** no error SHALL be logged
- **AND** the application SHALL continue running with the existing atlas

### Requirement: Platform supplies the rebuild callback during init
The application SHALL expose `app::registerRebuildFontAtlasCallback(void(*)())` in `app/App.h`. Each platform host SHALL call this API exactly once during its startup sequence (after `app::init` but before the first `app::frame`), passing a function pointer to a backend-specific rebuild implementation. The `app::configureFontAtlas()` function SHALL also be public in `app/App.h` (previously a file-scope static in `app/App.cpp`) so the registered callback can invoke it.

#### Scenario: Desktop host registers Vulkan callback
- **WHEN** the desktop host's `main()` runs after `app::init`
- **THEN** the host SHALL define a function (e.g., `rebuildFontAtlasDesktop`) that captures the active `VkDevice` (e.g., via a file-scope static populated during render-context setup) and performs the `vkDeviceWaitIdle + ImGui_ImplVulkan_DestroyFontsTexture + io.Fonts->Clear + app::configureFontAtlas + ImGui_ImplVulkan_CreateFontsTexture` sequence
- **AND** the host SHALL call `app::registerRebuildFontAtlasCallback(&rebuildFontAtlasDesktop)` exactly once before entering the frame loop

#### Scenario: iOS host registers Metal callback
- **WHEN** the iOS host's `viewDidLoad` runs after `app::init`
- **THEN** the host SHALL define a function (e.g., `rebuildFontAtlasIos`) that captures the active `id<MTLDevice>` and performs the `ImGui_ImplMetal_DestroyFontsTexture + io.Fonts->Clear + app::configureFontAtlas + ImGui_ImplMetal_CreateFontsTexture(device)` sequence
- **AND** the host SHALL call `app::registerRebuildFontAtlasCallback(&rebuildFontAtlasIos)` exactly once before entering the frame loop

#### Scenario: app::configureFontAtlas is publicly callable
- **WHEN** `app/App.h` is read
- **THEN** it SHALL declare `void configureFontAtlas();` inside `namespace app`
- **AND** the implementation in `app/App.cpp` SHALL be in the `app::` namespace (no longer inside an anonymous namespace)
- **AND** calling `configureFontAtlas` AFTER `io.Fonts->Clear()` SHALL successfully re-add the font from the current `themeFontFile()` + `themeFontSizePx()`

