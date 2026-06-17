## Why

> Note: the change name `defer-relayout-during-window-resize` is now a historical misnomer — see "What Changes" below for the actual behavior. Renaming the change directory would orphan the in-flight planning artifacts in this slot; the name is preserved for traceability.

On macOS, the system runs a **modal Cocoa event loop during live window resize** — while the user drags a window edge, our `while (!glfwWindowShouldClose) { ... }` body is paused. The OS keeps presenting the last drawable and stretches it to fit the new window dimensions, which produces a visible zoom/stretch artifact where ImGui content visibly enlarges and distorts during the drag, then snaps back to correct layout once the gesture ends. The user has flagged this as the worst-feeling rough edge in the v1 shell.

The desired behavior: **re-layout and render fresh ImGui content live during the drag**, every time Cocoa fires the resize / refresh callback, so the menu bar and About dialog move smoothly with the window instead of stretching. The earlier "freeze + darken-fill" exploration in this change was implemented and rejected after consideration — a frozen darkened window is less informative than a live re-rendered one and the implementation cost was the same order of magnitude.

## What Changes

- Extract the main loop's per-frame render code into a `renderOneFrame()` helper that does the full path: acquire image → backend `NewFrame` → `ImGui::NewFrame` (inside `app::frame`) → build UI → `ImGui::Render` → `ImGui_ImplVulkan_RenderDrawData` → submit → present.
- Register **`glfwSetWindowRefreshCallback`** on the desktop window. GLFW invokes this callback synchronously from inside Cocoa's modal resize loop (which is otherwise opaque to us). The callback recreates the swapchain at the new framebuffer size (if dimensions changed) and calls `renderOneFrame()` so the user sees fresh ImGui content at the correct size every Cocoa event.
- Update the existing **framebuffer-size callback** (`onResize`) to do the same recreate-then-render path, so the first event of a drag also produces a fresh frame rather than the OS-stretched one.
- Both the main loop and the callbacks share a single `frameIndex` counter (passed by reference into `renderOneFrame()`), so the `FrameResources` per-frame-in-flight pool is advanced consistently across both code paths.
- Keep the swapchain composite alpha at `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` (the v1 default) — no transparent-framebuffer hint, no post-multiplied alpha. The fresh ImGui frame at the new size is opaque just like normal rendering.
- The main loop body collapses to two calls: `glfwPollEvents()` + `renderOneFrame()`. The previous dedicated "swapchain-recreated-during-modal-loop" handling block goes away; `renderOneFrame()` and the callback both handle `OUT_OF_DATE` / `SUBOPTIMAL` results inline.

Non-goals (called out to bound scope):
- No animation, easing, or transition effects during the resize. Just live re-layout.
- No new threading model. The callback runs on the main thread (Cocoa's modal loop is a re-entered NSRunLoop on the main thread), and ImGui's single-threaded invariants are preserved because each call to `renderOneFrame()` is atomic between Cocoa events — ImGui is always "between frames" when the callback fires.
- No iOS change — iOS apps don't have user-driven window resize.
- No Windows / Linux behavior change in spirit. The callback fires on those platforms too, but their main loop is also running, so the callback's `renderOneFrame()` is just one more frame, harmless.

## Capabilities

### New Capabilities
<!-- None — this is a UX bug fix scoped entirely within the render-backend capability. -->

### Modified Capabilities

- `render-backend`: A new requirement "Live-resize behavior on desktop" specifies that the GLFW resize/refresh callbacks SHALL invoke the full ImGui render path (not a stripped-down clear), the swapchain recreate is gated on actual dimension change, and the main loop and callback share the same frame-resource pool via a shared `frameIndex`.

## Impact

- **Code change:** ~120 lines net in `apps/imgui-shell/platform/desktop/main.cpp` (extracted `renderOneFrame()`, expanded `ResizeCallbackState` to carry frames + ctx + frameIndex pointers, callback body shrunk to recreate-if-needed + `renderOneFrame()`). `vk_frame.{h,cpp}` and `vk_swapchain.cpp` revert their earlier "freeze approach" additions (`ResizeFrameResources` removed; `compositeAlpha` back to `OPAQUE`). Net: about the same total LOC as v1; about 60 LOC removed vs. the freeze-approach checkpoint.
- **Spec change:** `render-backend` gains a new `ADDED Requirements` block describing the live-relayout contract; replaces the earlier (in-flight, never archived) "darken-fill via Vulkan-only callback path" wording.
- **CI:** no workflow changes; live resize is interactive-only verification.
- **Risk:** the callback now calls into ImGui from Cocoa's modal-loop thread context. This is safe because (a) Cocoa's modal loop is a re-entered run-loop mode on the main thread (same thread that called `ImGui::CreateContext`), and (b) ImGui is in a "between frames" state every time the callback fires — the main loop body is one atomic `renderOneFrame()` between `glfwPollEvents()` calls, so no re-entrant ImGui frame can overlap.
- **Backward compatibility:** none required — there's no prior shipped behavior to preserve.
