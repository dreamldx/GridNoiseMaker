## Why

After `add-preferences-dialog` shipped autosave and the new `add-theme-picker` lets the user switch named themes live, the only knobs in the Preferences Theme tab that DON'T take effect immediately are `font_file` and `font_size_px`. The user just hit this — they changed the font size, expected to see it update, and didn't. The dialog has an inline note explaining the limitation, but the gap between editing a slider and seeing nothing happen is the kind of dead-end UX that breaks user trust.

The limitation was a deliberate non-goal of `add-typography-persistence`: ImGui's font atlas is GPU-resident, rasterized once at init, and rebuilding it at runtime requires coordinating with the renderer backend (vulkan/Metal must release the existing font texture, the atlas must clear-and-rebuild, the backend must upload a new texture). Cross-cutting work that was correctly deferred until the rest of the persistence loop existed. That loop now exists; this change closes it.

## What Changes

- **`app::rebuildFontAtlas()` callable from `Preferences.cpp`.** When a font_file or font_size_px widget commits via `IsItemDeactivatedAfterEdit`, the Preferences dialog SHALL call `app::rebuildFontAtlas()` after `persistTheme()`. The visible font updates within the same or next frame.
- **Platform-supplied rebuild callback.** To keep `app/` platform-uniform (no `#ifdef IMGUI_SHELL_PLATFORM_*` in source files), `app/` exposes a registration API: `void app::registerRebuildFontAtlasCallback(void(*)())`. The desktop and iOS hosts each register a function pointer during init. `app::rebuildFontAtlas()` invokes the registered callback; if no callback is registered (e.g., a headless test harness), it's a silent no-op.
- **Desktop (Vulkan) callback implementation** in `platform/desktop/main.cpp`. The function does:
  1. `vkDeviceWaitIdle(device)` — GPU must finish anything that references the old font texture.
  2. `ImGui_ImplVulkan_DestroyFontsTexture()` — release the existing GPU-side texture.
  3. `ImGuiIO& io = ImGui::GetIO(); io.Fonts->Clear();`
  4. `app::configureFontAtlas()` — re-add the font using the current `themeFontFile()` + `themeFontSizePx()`.
  5. `ImGui_ImplVulkan_CreateFontsTexture()` — upload the new atlas to the GPU.
- **iOS (Metal) callback implementation** in `platform/ios/ViewController.mm`. Same structure: `[commandQueue waitUntilCompleted]` (or equivalent), `ImGui_ImplMetal_DestroyFontsTexture()`, `io.Fonts->Clear()`, `app::configureFontAtlas()`, `ImGui_ImplMetal_CreateFontsTexture(device)`.
- **`app::configureFontAtlas()` becomes public.** Currently private static in `App.cpp`; the rebuild callbacks need to call it. Declaration moves to `App.h`. Behavior unchanged — reads current `themeFontFile()` + `themeFontSizePx()` and adds the font.
- **Preferences dialog inline note removed.** The `ImGui::TextDisabled("Font changes apply on next launch. The current view still uses the previously-loaded font.")` line in the Typography branches of `renderThemeRightPane` is deleted. Font changes now apply live.
- **Font Combo's `app::setThemeFontFile(...)` callsite also triggers rebuild.** Currently only commit-via-`IsItemDeactivatedAfterEdit` triggers persist+rebuild; the Combo's `if (Combo(...))` true-return path also needs the rebuild call.

Non-goals (called out to bound scope):
- **No mid-frame rebuild.** The callback runs synchronously inside `persistTheme`'s call path, which is inside an ImGui widget body. ImGui's rendering happens after `ImGui::Render()` at end-of-frame; calling `vkDeviceWaitIdle` mid-NewFrame is acceptable here because no draw command is in-flight yet (we're still building the command buffer for THIS frame). If profiling shows stutter, a follow-up can defer the rebuild to top-of-next-frame via a `g_fontRebuildPending` flag.
- **No mid-resize rebuild.** During a live-resize gesture, the desktop platform may already be calling `vkDeviceWaitIdle` between frames; concurrent rebuild + resize is a corner case we accept (the rebuild waits for GPU idle which is the same primitive resize uses).
- **No multi-font-size atlas.** Each rebuild rebuilds a SINGLE font at the current `themeFontSizePx()`. We don't pre-rasterize at multiple sizes for smooth scrubbing; that's a "dynamic font system" feature for a future ImGui upgrade.
- **No font-fallback chain.** Same single-font-load behavior as before; multi-font CJK / emoji is a separate feature.

## Capabilities

### New Capabilities

(none — this change extends existing capabilities)

### Modified Capabilities

- `font-rendering`: adds a "live font atlas rebuild" requirement; removes the implicit "atlas configured once at init" assumption from the existing "Bundled default font" requirement.
- `preferences-dialog`: removes the "Typography changes display the 'applies on next launch' caveat" scenario from the "Theme tab — master-detail editor" requirement; replaces it with "Typography changes apply immediately via atlas rebuild".
- `theme-persistence`: typography parsing comment / `_comment` strings updated to reflect the new "applies immediately" semantic; no parser behavior change.

## Impact

- **Code change:** ~80–120 LOC net.
  - `app/App.h` — add `configureFontAtlas` public declaration + `registerRebuildFontAtlasCallback` + `rebuildFontAtlas`.
  - `app/App.cpp` — move `configureFontAtlas` out of the anonymous namespace; add callback registration + dispatcher (~10 LOC).
  - `app/Preferences.cpp` — call `rebuildFontAtlas` after `persistTheme` in the font_size + font_file commit branches; also after the font Combo's `if (Combo(...))` true branch. Remove the "applies on next launch" note (~5 lines removed, 3 added).
  - `platform/desktop/main.cpp` — define `rebuildFontAtlasDesktop()` (~15 LOC) + register during init.
  - `platform/ios/ViewController.mm` — define `rebuildFontAtlasIos` (~15 LOC) + register during viewDidLoad.
- **New dependencies:** none. ImGui's vulkan + metal backends already expose `Destroy/CreateFontsTexture`.
- **Spec change:** MODIFIED `font-rendering`, `preferences-dialog`, `theme-persistence` — all delta-only (no new capability).
- **CI:** no workflow changes.
- **Risk surface:**
  - **`vkDeviceWaitIdle` during a frame is unusual** but safe — at the moment the callback fires, only `ImGui::NewFrame` + widget code has run for the current frame; the command buffer hasn't been submitted yet. The wait synchronizes against PREVIOUS frames' GPU work, which is exactly what we need.
  - **Texture handle invalidation.** ImGui caches the font texture ID on `io.Fonts->TexID` and inside `ImFont::ContainerAtlas`. Calling `Destroy` then `Create` reassigns this ID; ImGui code that's mid-call inside the rebuild callback path SHALL NOT cache the old ID (verified by reading the ImGui_ImplVulkan source — `CreateFontsTexture` updates `io.Fonts->SetTexID(...)` before returning).
  - **iOS-only verification gap.** iOS verification is still blocked on full Xcode (per the `add-typography-persistence` task 6.12 / `add-theme-picker` task 8.2). Source-level addition + macOS verification covers desktop; iOS Metal callback ships in the same change but is verified only by code review until Xcode is available.
- **Backward compatibility:** none required. The existing "applies on next launch" semantic was a known limitation, not a contract anyone could depend on. Headless test harnesses that don't register the callback see the original behavior (no rebuild) and keep working.
