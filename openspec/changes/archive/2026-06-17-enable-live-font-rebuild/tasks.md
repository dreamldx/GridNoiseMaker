## 1. App.h / App.cpp — public API surface

- [x] 1.1 Added `configureFontAtlas` + `RebuildFontAtlasFn` + `registerRebuildFontAtlasCallback` + `rebuildFontAtlas` declarations to `apps/imgui-shell/app/App.h` (inside `namespace app`). Header doc-comment explains the platform-callback split.
- [x] 1.2 Moved `configureFontAtlas()` out of `App.cpp`'s anonymous namespace into the `app::` namespace (linkage changed from anon-namespace-static to namespace-scoped). Existing call inside `app::init` continues to resolve.
- [x] 1.3 Added `RebuildFontAtlasFn g_rebuildFontAtlasCb = nullptr;` in App.cpp's anon namespace.
- [x] 1.4 Implemented `app::registerRebuildFontAtlasCallback(cb)` as `g_rebuildFontAtlasCb = cb;`.
- [x] 1.5 Implemented `app::rebuildFontAtlas()` as `if (g_rebuildFontAtlasCb) g_rebuildFontAtlasCb();` — silent no-op when unregistered (matches the "no callback" spec scenario).
- [x] 1.6 Clean build of `imgui_shell_app` confirms no symbol-visibility regression; the existing `app::init` call site still resolves.

## 2. Desktop callback — `rebuildFontAtlasDesktop` in `platform/desktop/main.cpp`

- [x] 2.1 Added file-scope `VkDevice g_imguiDevice = VK_NULL_HANDLE;` in main.cpp's anon namespace. Populated immediately after `ImGui_ImplVulkan_Init` (so it's set before any callback could fire).
- [x] 2.2 Defined `rebuildFontAtlasDesktop()` in main.cpp's anon namespace performing the sequence: `vkDeviceWaitIdle(g_imguiDevice) → ImGui_ImplVulkan_DestroyFontsTexture() → io.Fonts->Clear() → app::configureFontAtlas() → ImGui_ImplVulkan_CreateFontsTexture()`.
- [x] 2.3 In `main()` right after `ImGui_ImplVulkan_CreateFontsTexture()`, set `g_imguiDevice = device.device;` then `app::registerRebuildFontAtlasCallback(&rebuildFontAtlasDesktop);`.
- [x] 2.4 Clean build confirms `ImGui_ImplVulkan_DestroyFontsTexture` + `CreateFontsTexture` resolve against the linked ImGui Vulkan backend.

## 3. iOS callback — `rebuildFontAtlasIos` in `platform/ios/ViewController.mm`

- [x] 3.1 Added file-scope `static id<MTLDevice> g_metalDevice = nil;` at the top of `ViewController.mm` (outside the `@implementation` block). Populated in `viewDidLoad` from `_device`.
- [x] 3.2 Defined `static void rebuildFontAtlasIos()` performing the sequence: `ImGui_ImplMetal_DestroyFontsTexture() → io.Fonts->Clear() → app::configureFontAtlas() → ImGui_ImplMetal_CreateFontsTexture(g_metalDevice)`. Comment notes Metal's ref-counted texture lifecycle handles in-flight references without an explicit wait.
- [x] 3.3 In `viewDidLoad` right after `app::init(_ctx)` returns and `_appInitialized = YES`, set `g_metalDevice = _device;` then `app::registerRebuildFontAtlasCallback(&rebuildFontAtlasIos);`.
- [ ] 3.4 iOS runtime verification deferred (full Xcode unavailable). Source-level addition lands as code review only.

## 4. Preferences.cpp — call `rebuildFontAtlas()` after `persistTheme()` for font commits

- [x] 4.1 Located the three font-commit paths in `renderThemeRightPane`'s `SelectionKind::FontFile` + `SelectionKind::FontSize` branches.
- [x] 4.2 Combo bundled-pick branch: appended `app::rebuildFontAtlas();` after the existing `persistTheme()` call in the `if (comboIdx < customSentinel)` body.
- [x] 4.3 Custom-path InputText branch: changed `if (ImGui::IsItemDeactivatedAfterEdit()) persistTheme();` to `{ persistTheme(); app::rebuildFontAtlas(); }`.
- [x] 4.4 Font-size SliderFloat branch: same transform on the `IsItemDeactivatedAfterEdit` block — appended `app::rebuildFontAtlas();` after `persistTheme();`.

## 5. Remove "applies on next launch" inline notes

- [x] 5.1 Deleted the `ImGui::Spacing()` + `ImGui::TextDisabled("Font changes apply on next launch. ...")` lines from the `SelectionKind::FontFile` branch.
- [x] 5.2 Deleted the same pair from the `SelectionKind::FontSize` branch.
- [x] 5.3 Grep verified no remaining `applies on next launch` / `previously-loaded font` strings anywhere in `apps/imgui-shell/`.

## 6. Verification & spec validation

- [x] 6.1 Clean rebuild on macOS Vulkan + FreeType: zero warnings, zero errors. `build/macos/platform/desktop/imgui_shell_desktop` produced.
- [x] 6.2 stb fallback rebuild (`build/macos-stb/`): zero warnings, zero errors.
- [x] 6.3 Headless smoke (3s): silent stderr, clean SIGTERM exit. Boot path (migration + theme picker + autosave from prior changes) unaffected by the new font-rebuild wiring.
- [x] 6.4 Interactive live font-size + font-file test confirmed working across multiple user-driven sessions (clean exit 0, theme files updating in `~/.config/imgui-shell/themes/`). The deferred-flag fix landed after the initial synchronous-rebuild assertion crash; subsequent sessions exercised the live-rebuild path successfully.
- [x] 6.5 Combined with 6.4 — font-file Combo switches between Inter / JetBrains Mono / Lato landed live during the same sessions.
- [ ] 6.6 Custom-path InputText test — NOT explicitly verified in this archive session (no invalid-path test reproduced). The code path mirrors the bundled-Combo branch, so behavior should follow; flagged for future verification.
- [ ] 6.7 Mid-drag stability — NOT explicitly verified in this archive session. Implementation only calls `rebuildFontAtlas()` on `IsItemDeactivatedAfterEdit` (slider release), not on every value change, so the design enforces the property; flagged for future eyeball.
- [x] 6.8 "Applies on next launch" note removed — verified by code review: both `ImGui::TextDisabled("Font changes apply on next launch...")` lines were deleted in section 5; no remaining instances grep-verified.
- [x] 6.9 `openspec validate enable-live-font-rebuild --type change --strict` ⇒ "Change 'enable-live-font-rebuild' is valid".
- [x] 6.10 Spec walkthrough — every requirement maps to specific code:
  - `font-rendering` ADDED "Font atlas can be rebuilt at runtime" → `app::rebuildFontAtlas` in App.cpp + the desktop / iOS rebuild callbacks.
  - `font-rendering` ADDED "Platform supplies the rebuild callback during init" → `app::registerRebuildFontAtlasCallback` + the host registration calls in `main()` (desktop) and `viewDidLoad` (iOS).
  - `preferences-dialog` MODIFIED "Theme tab — master-detail editor" — the "applies on next launch" caveat scenario is replaced; the new "Typography changes apply immediately" scenario maps to the Preferences.cpp font-commit branches (which now call `rebuildFontAtlas()` after `persistTheme()`).
  - `preferences-dialog` ADDED "Typography commits trigger live atlas rebuild" → the three `app::rebuildFontAtlas()` call sites in `renderThemeRightPane` (Combo bundled pick, custom-path InputText commit, font-size SliderFloat release).
