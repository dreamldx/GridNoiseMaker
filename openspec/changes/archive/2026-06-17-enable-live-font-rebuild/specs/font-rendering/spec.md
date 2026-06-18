## ADDED Requirements

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
