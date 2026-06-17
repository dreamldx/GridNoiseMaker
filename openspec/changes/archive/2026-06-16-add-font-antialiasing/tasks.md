## 1. Dependencies and CMake wiring

- [x] 1.1 Add a CMake cache option `IMGUI_SHELL_USE_FREETYPE` (default `ON`) at the top of `apps/imgui-shell/CMakeLists.txt`
- [x] 1.2 In `cmake/Dependencies.cmake`, guarded on `IMGUI_SHELL_USE_FREETYPE`: `FetchContent_Declare(freetype ... GIT_TAG VER-2-13-3 GIT_SHALLOW TRUE)` after setting `FT_DISABLE_ZLIB`, `FT_DISABLE_BZIP2`, `FT_DISABLE_PNG`, `FT_DISABLE_HARFBUZZ`, `FT_DISABLE_BROTLI` and `SKIP_INSTALL_ALL` to `ON`; call `FetchContent_MakeAvailable(freetype)`
- [x] 1.3 When `IMGUI_SHELL_USE_FREETYPE=ON`, append `${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp` to the `imgui` static library's source list and link `freetype` into it via `target_link_libraries(imgui PUBLIC freetype)`
- [x] 1.4 When `IMGUI_SHELL_USE_FREETYPE=ON`, `target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)` so downstream code sees the feature flag
- [x] 1.5 Verify on the host: `cmake --preset macos` reports the FreeType FetchContent step succeeding; toggling `-DIMGUI_SHELL_USE_FREETYPE=OFF` skips the fetch (verified — `build/macos` fetched + linked freetype; `build/macos-stb` did not)

## 2. Bundled font asset

- [x] 2.1 Create `apps/imgui-shell/assets/fonts/` directory
- [x] 2.2 Add `Inter-Regular.ttf` (downloaded from the upstream `rsms/inter` v4.0 release, `extras/ttf/Inter-Regular.ttf`) at its unmodified canonical filename — required by SIL OFL reserved-font-name clause
- [x] 2.3 Add `Inter-OFL.txt` (the upstream SIL OFL 1.1 license file accompanying the font) into the same directory
- [x] 2.4 In `apps/imgui-shell/platform/desktop/CMakeLists.txt`, copy `${CMAKE_SOURCE_DIR}/assets/` next to the desktop binary via a post-build `cmake -E copy_directory` step (build output verified: `build/macos/platform/desktop/assets/fonts/Inter-Regular.ttf` exists)
- [x] 2.5 In `apps/imgui-shell/platform/ios/CMakeLists.txt`, add the font + license file to the `imgui_shell_ios` target with `MACOSX_PACKAGE_LOCATION "Resources/fonts"` so they end up under `Resources/fonts/` inside the `.app` bundle
- [x] 2.6 Add `IMGUI_SHELL_ASSETS_DIR` compile definition on `imgui_shell_app` (PRIVATE, desktop only — iOS uses the runtime `setBundleResourcePath` API instead)

## 3. App code: atlas configuration

- [x] 3.1 Add a `IMGUI_SHELL_FONT_BACKEND` compile def on `imgui_shell_app` (`"FreeType"` when `IMGUI_SHELL_USE_FREETYPE=ON`, else `"stb_truetype"`)
- [x] 3.2 Add a `IMGUI_SHELL_DEFAULT_FONT_PX` compile def on `imgui_shell_app` (default `14`, exposed as a CMake cache var so a contributor can override per-build)
- [x] 3.3 In `app/App.cpp`'s `init` — after `ImGui::CreateContext`, before `ImGui::StyleColorsDark` — when `IMGUI_ENABLE_FREETYPE` is defined: set `io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();`
- [x] 3.4 In `app/App.cpp`'s `init`, after the FontBuilderIO wiring: build the font path via `resolveAssetPath("fonts/Inter-Regular.ttf")`, call `io.Fonts->AddFontFromFileTTF(path, IMGUI_SHELL_DEFAULT_FONT_PX, &cfg)` where `cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting`. On nullptr return, log a clear stderr message and fall back to `io.Fonts->AddFontDefault()`; remember the fallback in `g_fontFallback` so the About dialog can report it
- [x] 3.5 Added pure-C++ `app::resolveAssetPath` helper (uses `IMGUI_SHELL_ASSETS_DIR` compile-def on desktop, runtime `g_bundleResourcePath` on iOS) and a public `app::setBundleResourcePath(const char*)` API the iOS host calls
- [x] 3.6 Update `app/App.cpp`'s About modal to render two new lines: `"Font rasterizer: %s%s"` (with fallback annotation) and `"Default font size: %d px"`

## 4. iOS host wiring (defers Xcode verification)

- [x] 4.1 In `platform/ios/ViewController.mm` `viewDidLoad`, call `app::setBundleResourcePath([[NSBundle mainBundle].resourcePath UTF8String])` before `app::init`
- [x] 4.2 iOS CMakeLists.txt uses the `MACOSX_PACKAGE_LOCATION "Resources/fonts"` pattern on the font source files so assets bundle correctly — **VERIFICATION DEFERRED: requires full Xcode**

## 5. Verification

- [x] 5.1 Clean rebuild on macOS: `cmake --build --preset macos --target imgui_shell_desktop` succeeded (83 build steps, no warnings, FreeType static lib appears, assets copied to `build/macos/platform/desktop/assets/fonts/`)
- [x] 5.2 Ran the desktop binary 3+ seconds under validation layer (`VK_ICD_FILENAMES=.../MoltenVK_icd.json`); stderr silent — font loaded without falling back
- [ ] 5.3 Interactive smoke check: open `Help > About...`, confirm `"Font rasterizer: FreeType"` is visible and the displayed pixel size matches `IMGUI_SHELL_DEFAULT_FONT_PX` — **DEFERRED: requires interactive run; code wired per spec, both binaries available at `build/macos/...` and `build/macos-stb/...` for side-by-side eyeball test**
- [x] 5.4 Reconfigured with `-DIMGUI_SHELL_USE_FREETYPE=OFF` in `build/macos-stb/`, rebuilt, ran; `nm` confirms ZERO `FT_*` symbols in the stb binary while the default build has `_FT_Init_FreeType` and friends; both binaries run silently for 3s under validation layer
- [x] 5.5 Walked through `specs/font-rendering/spec.md` scenarios — all backend-selection / asset-loading / hinting / About-dialog / no-runtime-picker scenarios are satisfied in the FreeType build; fallback scenarios satisfied in the stb build. Asset-missing scenario verified by code path (fprintf + AddFontDefault + g_fontFallback flag) but only smoke-tested at code review
- [x] 5.6 Walked through `specs/build-system/spec.md` delta scenarios: FreeType FetchContent succeeds + skipped on OFF (verified); desktop output has fonts at the spec-required path (verified); iOS bundle layout matches spec (code matches; Xcode verification deferred); MODIFIED "ImGui compiled as a static library" — `imgui_freetype.cpp` object present in build dir when FreeType is ON
- [x] 5.7 `openspec validate add-font-antialiasing --type change --strict` ⇒ "Change 'add-font-antialiasing' is valid"
- [ ] 5.8 iOS bundle verification — **DEFERRED: requires full Xcode**; documented in the archive notes
