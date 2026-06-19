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
- **THEN** the binary SHALL perform comprehensive validation including file existence check and format verification
- **AND** SHALL print a detailed error message identifying the missing asset path, validation failure type, and suggested remediation
- **AND** SHALL fall back to `ImGui::GetIO().Fonts->AddFontDefault()` (Proggy Clean) so the application still launches
- **AND** the fallback path SHALL also be visible in the About dialog with error details (e.g., `"FreeType (fallback font: Proggy due to validation error: <details>)"`)

#### Scenario: Font size source is the theme accessor, not a compile definition
- **WHEN** a contributor wants to change the default font pixel size
- **THEN** they SHALL edit `app::kDefaultThemeFontSizePx` in `app/Theme.h` (the baked-in default that the accessor returns when no override is active)
- **AND** there SHALL NOT be a `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition or CMake cache variable for font size
- **AND** a user wanting to change ONLY their own copy SHALL edit `~/.config/imgui-shell/theme.json`'s `typography.font_size_px` rather than the source code

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



### Requirement: Font atlas can be rebuilt at runtime
The application SHALL provide `app::rebuildFontAtlas()` which clears and re-rasterizes the ImGui font atlas using the current `app::themeFontFile()` and `app::themeFontSizePx()` values, and coordinates with the active renderer backend to destroy and recreate the GPU-side font texture. Before rebuilding, the system SHALL validate the current font file using the two-stage validation architecture. If validation fails, the rebuild SHALL be aborted with detailed error logging and the existing atlas SHALL remain unchanged. After successful validation and rebuild, subsequent ImGui rendering SHALL use the newly-rasterized atlas (no relaunch required). The function SHALL be a silent no-op when called before `app::registerRebuildFontAtlasCallback` has been invoked.

#### Scenario: Rebuild call applies new font live
- **WHEN** `app::setThemeFontSizePx(24.0f)` runs followed by `app::rebuildFontAtlas()`
- **THEN** the system SHALL first validate the font file at the new size using two-stage validation
- **AND** if validation succeeds, `io.Fonts->Clear()` SHALL be invoked
- **AND** `app::configureFontAtlas()` SHALL be re-invoked, re-adding the font at the new size
- **AND** the renderer backend's font texture SHALL be recreated (`ImGui_ImplVulkan_CreateFontsTexture` on desktop, `ImGui_ImplMetal_CreateFontsTexture` on iOS)
- **AND** the very next ImGui frame SHALL render text at the new pixel size

#### Scenario: Rebuild aborted on validation failure
- **WHEN** `app::rebuildFontAtlas()` is called but font validation fails
- **THEN** the rebuild process SHALL be aborted before `io.Fonts->Clear()`
- **AND** a detailed validation error SHALL be logged
- **AND** the existing font atlas SHALL remain unchanged
- **AND** the About dialog SHALL indicate validation failure status

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
The application SHALL expose `app::registerRebuildFontAtlasCallback(void(*)())` in `app/App.h`. Each platform host SHALL call this API exactly once during its startup sequence (after `app::init` but before the first `app::frame`), passing a function pointer to a backend-specific rebuild implementation. The `app::configureFontAtlas()` function SHALL also be public in `app/App.h` (previously a file-scope static in `app/App.cpp`) so the registered callback can invoke it. Additionally, the system SHALL expose `app::registerFontErrorCallback(void(*)(FontError))` for platform hosts to receive font validation and rendering errors.

#### Scenario: Desktop host registers Vulkan callback
- **WHEN** the desktop host's `main()` runs after `app::init`
1 **THEN** the host SHALL define a function (e.g., `rebuildFontAtlasDesktop`) that captures the active `VkDevice` (e.g., via a file-scope static populated during render-context setup) and performs the `vkDeviceWaitIdle + ImGui_ImplVulkan_DestroyFontsTexture + io.Fonts->Clear + app::configureFontAtlas + ImGui_ImplVulkan_CreateFontsTexture` sequence
- **AND** the host SHALL call `app::registerRebuildFontAtlasCallback(&rebuildFontAtlasDesktop)` exactly once before entering the frame loop
- **AND** the host SHALL optionally call `app::registerFontErrorCallback()` to receive font validation errors

#### Scenario: iOS host registers Metal callback
- **WHEN** the iOS host's `viewDidLoad` runs after `app::init`
- **THEN** the host SHALL define a function (e.g., `rebuildFontAtlasIos`) that captures the active `id<MTLDevice>` and performs the `ImGui_ImplMetal_DestroyFontsTexture + io.Fonts->Clear + app::configureFontAtlas + ImGui_ImplMetal_CreateFontsTexture(device)` sequence
- **AND** the host SHALL call `app::registerRebuildFontAtlasCallback(&rebuildFontAtlasIos)` exactly once before entering the frame loop
- **AND** the host SHALL optionally call `app::registerFontErrorCallback()` to receive font validation errors

#### Scenario: app::configureFontAtlas is publicly callable
- **WHEN** `app/App.h` is read
- **THEN** it SHALL declare `void configureFontAtlas();` inside `namespace app`
- **AND** the implementation in `app/App.cpp` SHALL be in the `app::` namespace (no longer inside an anonymous namespace)
- **AND** calling `configureFontAtlas` AFTER `io.Fonts->Clear()` SHALL successfully re-add the font from the current `themeFontFile()` + `themeFontSizePx()`

