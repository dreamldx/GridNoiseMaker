## Context

ImGui's font atlas is a single texture rasterized at `ImGui::CreateContext` time inside `app::configureFontAtlas` and uploaded to GPU memory by the backend's `CreateFontsTexture` call. The atlas is immutable thereafter — the backend holds a `VkImage` / `MTLTexture` handle that's referenced from every draw command. Rebuilding requires releasing the GPU resource, clearing and re-rasterizing the atlas, then re-uploading.

Three constraints frame the design:
- **`app/` is platform-uniform.** No `#ifdef IMGUI_SHELL_PLATFORM_*` in `app/` source files (per the existing app-shell convention). Calling `ImGui_ImplVulkan_DestroyFontsTexture` directly from `app/Preferences.cpp` would break this — we'd need to include `imgui_impl_vulkan.h` which doesn't exist on iOS.
- **The renderer backend owns GPU sync.** `vkDeviceWaitIdle` (desktop) and Metal's `[commandQueue waitUntilCompleted]` (iOS) MUST run before texture destruction; only the platform layer has the device handle in scope.
- **Rebuild happens at slider-release time** — user-driven, ~1 rebuild per second worst case. The cost (atlas rasterize + GPU upload, ~5–20 ms) is acceptable as a one-shot stall, but NOT acceptable per-frame.

## Goals / Non-Goals

**Goals:**
- Font size / file changes apply within one frame of the widget commit.
- Implementation does NOT introduce `#ifdef` blocks in `app/`.
- Desktop (Vulkan) + iOS (Metal) both supported in the same change.
- No regressions on platforms that haven't registered a callback (headless tests, future platforms).

**Non-Goals:**
- No mid-frame async rebuild. The callback runs synchronously where the commit fires.
- No multi-size atlas pre-rasterization. One font, one size at a time.
- No font fallback chain expansion. Same single-font load.
- No live `font_file` swap optimization (e.g., caching atlases per file). Each rebuild starts from a clean atlas.

## Decisions

### D1: Platform-supplied callback registered into `app/`
```cpp
// app/App.h
namespace app {
    using RebuildFontAtlasFn = void (*)();
    void registerRebuildFontAtlasCallback(RebuildFontAtlasFn cb);
    void rebuildFontAtlas();           // call this from anywhere in app/
    void configureFontAtlas();         // newly public — was an anon-namespace static
}
```

`registerRebuildFontAtlasCallback` stores the callback in a file-scope `RebuildFontAtlasFn g_rebuildCb = nullptr;`. `rebuildFontAtlas()` is `if (g_rebuildCb) g_rebuildCb();` — silent no-op when unset.

**Why function pointer over `std::function`:** zero allocation, ABI-stable, no <functional> include needed in App.h. Callback never captures state — it just calls a fixed-name function in the platform's translation unit.

- **Alternatives:**
  - **Direct call from `app/`:** would require `#ifdef IMGUI_SHELL_PLATFORM_*` in App.cpp or Preferences.cpp to pick the right backend. Rejected.
  - **Two callbacks (pre + post):** `pre` does GPU wait + texture destroy; `post` does texture create. `app/` would call `pre → io.Fonts->Clear() → configureFontAtlas() → post`. Lets `app/` own the atlas-management half. Slightly cleaner separation but adds API surface; the single-callback approach with `configureFontAtlas` exposed is enough.
  - **Singleton class:** over-engineered.

### D2: Desktop callback in `platform/desktop/main.cpp`
```cpp
namespace {
    VkDevice g_device = VK_NULL_HANDLE;   // captured from RenderContext during init

    void rebuildFontAtlasDesktop() {
        vkDeviceWaitIdle(g_device);
        ImGui_ImplVulkan_DestroyFontsTexture();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        app::configureFontAtlas();
        ImGui_ImplVulkan_CreateFontsTexture();
    }
}

// In main() after app::init:
app::registerRebuildFontAtlasCallback(&rebuildFontAtlasDesktop);
```

Stash `VkDevice` in a file-scope static after the host creates it — it's stable for the process lifetime. Alternative: use `app::activePhysicalDevice` + add an `app::activeDevice()` accessor; deferred because the callback already needs to be in `platform/desktop/main.cpp` (no point routing through `app/`).

- **Alternatives:**
  - **Pass device via callback capture:** would require `std::function` (D1 rejected this).
  - **Re-query device per call:** unnecessary — it's set once.

### D3: iOS callback in `platform/ios/ViewController.mm`
```objc
static id<MTLDevice> g_metalDevice = nil;

static void rebuildFontAtlasIos() {
    // Metal doesn't have a single "wait idle" call; the existing
    // ViewController frame pattern serializes via the per-frame
    // command-buffer completion. Synchronizing here is best-effort:
    // call [commandQueue insertDebugCaptureBoundary] (no-op fence)
    // or rely on the next frame's commit picking up the new texture.
    ImGui_ImplMetal_DestroyFontsTexture();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    app::configureFontAtlas();
    ImGui_ImplMetal_CreateFontsTexture(g_metalDevice);
}

// In viewDidLoad after app::init:
app::registerRebuildFontAtlasCallback(&rebuildFontAtlasIos);
```

Metal's texture lifecycle is reference-counted via ARC; releasing the old texture happens once the last command buffer referencing it completes. Re-creating before the old one is released is safe (the old retained handle just gets dropped). No explicit wait needed in v1.

- **Alternatives:**
  - **`[commandQueue waitUntilCompleted]`** before destroy: requires keeping the most recent command buffer; not in scope for v1.
  - **Defer iOS rebuild entirely:** would leave font_size live-update broken on iOS. Rejected; ship both backends together.

### D4: Trigger point — `Preferences.cpp` font commit branches
```cpp
// In renderThemeRightPane, SelectionKind::FontSize:
if (ImGui::SliderFloat("##fontsize", &px, 6.0f, 96.0f, "%.1f px")) {
    app::setThemeFontSizePx(px);
}
if (ImGui::IsItemDeactivatedAfterEdit()) {
    persistTheme();
    app::rebuildFontAtlas();          // NEW
}
```

Symmetrically for `SelectionKind::FontFile`:
- Combo bundled-pick: `setThemeFontFile + persistTheme + rebuildFontAtlas`
- Custom-path InputText `IsItemDeactivatedAfterEdit`: `setThemeFontFile + persistTheme + rebuildFontAtlas`

Order matters: `persistTheme` must run before `rebuildFontAtlas` so the on-disk state is current at the moment the rebuild reads the new size/path. (Both currently read from `app::themeFontFile` / `app::themeFontSizePx` accessors, so order is moot for correctness — but the persist-first convention is the contract from `add-preferences-dialog`.)

- **Alternatives:**
  - **Rebuild on every NewFrame if a flag is set:** D6 below considers this.

### D5: `configureFontAtlas` becomes public
Currently `static` in `App.cpp`'s anonymous namespace. Move its declaration to `App.h`. The implementation stays in `App.cpp`'s file scope. Callers:
- `app::init` (existing, unchanged)
- `platform/desktop/main.cpp::rebuildFontAtlasDesktop`
- `platform/ios/ViewController.mm::rebuildFontAtlasIos`

No behavior change. The function reads `themeFontFile()` + `themeFontSizePx()` and adds the font. Safe to call after `Clear()`.

### D6: Deferred-to-next-frame (corrected from initial synchronous design)
**v1 ships the deferred-flag approach** (a correction during implementation). `app::rebuildFontAtlas()` sets a file-scope `bool g_fontRebuildPending = true;`. The next `app::frame` call drains the flag at the TOP, BEFORE `ImGui::NewFrame()`, by invoking the registered platform callback. The font visibly changes one frame after the user releases the slider — a single 16 ms delay, imperceptible in practice.

**Why synchronous-from-widget-body was wrong (caught during testing):**
ImGui sets `io.Fonts->Locked = true` inside `ImGui::NewFrame()` and clears it inside `ImGui::EndFrame()` (which `ImGui::Render()` calls). `ImFontAtlas::Clear()` asserts on the locked flag: `Assertion failed: (!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!"). The initial design's reasoning ("no draw commands have been submitted yet, so it's safe") missed the existence of this lock — the assertion fires immediately on the first widget commit. The fix is to defer the rebuild to between frames, where the atlas is unlocked.

**Order of operations in the deferred path:**
1. User commits a font-related widget (slider release / Combo pick / InputText commit).
2. Preferences.cpp calls `persistTheme()` then `app::rebuildFontAtlas()`.
3. `rebuildFontAtlas()` sets `g_fontRebuildPending = true;` and returns immediately. The current frame continues rendering with the OLD atlas.
4. Frame ends — `ImGui::Render()` is called, which sets `Locked = false`. Backend submits draw commands referencing the old font texture. GPU work proceeds.
5. Host's main loop begins the next frame: `ImGui_Impl{Vulkan,Glfw}_NewFrame()` runs.
6. `app::frame()` enters. It checks `g_fontRebuildPending`, finds it true, invokes `g_rebuildFontAtlasCb()`. This calls `vkDeviceWaitIdle` (drains the OLD-texture draws from the just-submitted previous frame), then destroys the old GPU texture, clears + rebuilds the atlas, creates the new GPU texture. `Locked` is still false here — we haven't called `ImGui::NewFrame()` yet.
7. `app::frame()` calls `ImGui::NewFrame()`. Locked = true. From here on the new atlas is in use; the rest of the frame renders text at the new size/font.

- **Alternatives:**
  - **Synchronous-from-widget-body (initial design):** REJECTED at implementation time due to the atlas-locked assertion.
  - **Drain at end of frame (after `Render()`):** also valid, since `Locked` is false after `Render`. Top-of-next-frame keeps the cause-and-effect more obvious in code ("next frame uses new font") and groups the rebuild with the other top-of-frame init (e.g. backend `NewFrame` calls happen just before).

### D7: Removed inline "applies on next launch" note
The two `ImGui::TextDisabled("Font changes apply on next launch. The current view still uses the previously-loaded font.")` lines (one in the FontFile branch, one in the FontSize branch of `renderThemeRightPane`) are deleted. With the rebuild in place, the note is wrong.

The note removal happens in the same commit as the rebuild wiring so the dialog never lies to the user.

- **Alternatives:**
  - **Keep the note but change wording to "Font changes apply immediately":** noisy. Better to remove it — users intuit from the live update that it works.

## Risks / Trade-offs

- **[Risk: `vkDeviceWaitIdle` from inside widget code stalls the renderer for the duration of one font-atlas rasterize.]** → Acceptable: rasterizing the bundled fonts at 14 px is ~5 ms on Apple Silicon, ~15 ms at 96 px. The stall is at slider-release-time, not during the drag. If profiling reveals user-visible jank, switch to deferred-flag (D6 alternative).
- **[Risk: GPU has a frame's draw commands referencing the old font texture STILL in-flight when destroy is called.]** → Mitigated by `vkDeviceWaitIdle` on desktop. On iOS, Metal's reference-counted texture lifecycle handles this implicitly (the old texture stays alive until the command buffer that references it completes; we just stop sending NEW draws against it). Verified by code review of `ImGui_ImplMetal_DestroyFontsTexture`.
- **[Risk: `ImFont*` pointers held anywhere outside the atlas become dangling after `io.Fonts->Clear()`.]** → ImGui internals (windows, draw lists) reference fonts via `io.Fonts->Fonts[0]` indirection, not raw `ImFont*`. After `Clear + configureFontAtlas`, `io.Fonts->Fonts[0]` is a fresh-but-equivalent `ImFont*`. Verified by reading `ImFontAtlas::Clear` source — it deletes the `ImFont*` array; new entries are repopulated by the next `AddFontFromFileTTF`. The default style's `Font` pointer (settable via `io.FontDefault`) is NOT touched by `Clear`, so we don't need to re-set it.
- **[Risk: Multi-viewport secondary windows on desktop hold their own ImGui contexts? (No — multi-viewport shares one context across all viewports.)]** → Verified: `ImGui_ImplVulkan_*Texture` operates on the single shared atlas; secondary OS windows pick up the new texture on their next frame.
- **[Trade-off: synchronously calling `vkDeviceWaitIdle` blocks the calling thread until the GPU is idle.]** → Acceptable at slider-release moment; would be unacceptable per-frame. Mitigation is "only do this on commit, not on every value change."
- **[Trade-off: iOS callback ships untested on real hardware.]** → Defer iOS verification per `add-typography-persistence` task 6.12 / `add-theme-picker` task 8.2; ship as code-review only.

## Migration Plan

1. Land code change. Existing builds keep working with no rebuild registered (the new APIs are additive); shipping platforms register the callback at init.
2. No rollback path needed — reverting the change removes the callback registration; `app::rebuildFontAtlas()` becomes a silent no-op; users see the pre-change "applies on next launch" behavior (which is what they had).

## Open Questions

- **Should `app::registerRebuildFontAtlasCallback` take ownership of multiple callbacks (e.g., for diagnostic logging)?** No — single callback is enough. If diagnostics are needed, they can be wrapped inside the platform's callback implementation.
- **What if the user changes font_file to a missing path?** `configureFontAtlas` already falls back to `AddFontDefault` (Proggy) and logs. After rebuild, the visible font becomes Proggy — visible feedback. Acceptable.
- **Should we cap the rebuild rate (e.g., debounce 200 ms)?** With commit-on-release-only, the natural rate is "one rebuild per slider grab" — already debounced by the user's hand. Skip explicit debounce.
