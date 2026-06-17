## Context

Live window resize on macOS is handled by Cocoa via a **modal NSRunLoop** (`NSEventTrackingRunLoopMode`) that takes over the calling thread for the entire duration of the drag gesture. Until the user releases the mouse, the application's normal main loop is suspended — `glfwPollEvents()` does not return, so we never see `glfwGetFramebufferSize` changing, never recreate the swapchain, never render a fresh frame. The OS, lacking new frames from us, takes the last presented drawable and stretches it across the changing window dimensions, producing the visible "zoom" artifact the user reported.

GLFW does provide two callbacks that DO fire from inside Cocoa's modal loop on macOS: `glfwSetWindowRefreshCallback` and `glfwSetFramebufferSizeCallback`. The path to a smooth live-resize is therefore: in those callbacks, recreate the swapchain at the current framebuffer size (if it has changed) and **render one full ImGui frame** so the OS receives a fresh-content image at the right dimensions every Cocoa event.

An earlier iteration of this change took the opposite approach — a Vulkan-only "darken-fill" clear during the drag, deferring ImGui re-layout until the drag finished. That was implemented end-to-end (compiled, smoke-tested) and rejected after thinking about UX: a frozen darkened window is less informative than a live re-rendered one, and the implementation cost was the same order of magnitude. The code paths from that iteration (`ResizeFrameResources`, `renderDarkenFill`, transparent framebuffer, post-multiplied alpha) have been reverted; this design captures the second-pass direction.

## Goals / Non-Goals

**Goals:**
- Eliminate the OS-driven zoom/stretch artifact during live window resize on macOS by rendering a fresh ImGui frame at the current size every time Cocoa fires a resize / refresh callback.
- Share a single `renderOneFrame()` code path between the main loop and the callbacks — no parallel "minimal Vulkan clear" implementation to keep in sync.
- Share the `FrameResources` per-frame-in-flight pool across the main loop and the callback path, advancing a single `frameIndex` counter, so no dedicated resize-only sync primitives are required.
- Behave harmlessly on Windows and Linux — the callbacks fire there too; on those platforms the main loop is also running, so the callback's frame is one extra rendered frame per resize event, invisible.

**Non-Goals:**
- No animation, easing, or transition effects on resize start/end.
- No alternative "freeze + dark fill" mode (rejected in design; not configurable at runtime).
- No multi-threaded rendering. The callback runs on the main thread in a re-entered run-loop mode, which is single-threaded with respect to the original main loop.
- No iOS change. iOS apps don't have user-driven window resize.
- No `glfwSetWindowSizeLimits` / aspect-ratio constraints.

## Decisions

### D1: Share one `renderOneFrame()` between main loop and callbacks
The main loop body extracts into `renderOneFrame(VkDevice, VkPhysicalDevice, VkSurfaceKHR, VkQueue, GLFWwindow*, Swapchain&, FrameResources&, app::RenderContext&, uint32_t& frameIndex)`. Both the main loop and the resize/refresh callbacks call it. Identical behavior in both paths — `ImGui_ImplVulkan_NewFrame` → `ImGui_ImplGlfw_NewFrame` → `app::frame` → `ImGui_ImplVulkan_RenderDrawData` → submit → present. Returns `bool` (true if a frame was presented; false if the swapchain was recreated due to `OUT_OF_DATE` and the call should be retried by the next loop iteration).

- **Alternatives:**
  - **Separate "live-resize render" function:** Would duplicate every per-frame Vulkan operation. Maintenance hazard. Rejected.
  - **Inline the body in both places:** Same duplication concern. Rejected.

### D2: Callbacks safe to call ImGui because they run on main thread, between frames
The hard rule from the earlier "darken-fill" iteration — *"callback MUST NOT touch ImGui or app::*"* — is dropped. It was needed when we wanted the callback path to be Vulkan-only; now that the callback IS the same render path as the main loop, calling ImGui from it is required.

The safety argument: Cocoa's modal loop is a re-entered NSRunLoop mode on the **main thread** (the same thread that called `ImGui::CreateContext`), and ImGui's "between frames" invariant holds whenever the callback fires — the main loop body is exactly one atomic `renderOneFrame()` between `glfwPollEvents()` calls, so no re-entrant ImGui frame can overlap.

- **Alternatives:**
  - **Run the callback on a worker thread:** Violates ImGui's single-threaded model and adds threading complexity. Rejected.
  - **Defer ImGui rendering by setting a flag and waking the main loop later:** Doesn't help — the main loop is paused during the modal Cocoa loop, can't be "woken" without ending the drag. Rejected.

### D3: Share `FrameResources` + `frameIndex` between paths
The callback uses the same `FrameResources` pool as the main loop and advances the same `frameIndex` counter (passed by reference into `renderOneFrame`). No dedicated `ResizeFrameResources` struct. Because callback and main loop never run concurrently (single thread, re-entered run loop), the fence/semaphore reuse semantics are exactly the same as ordinary back-to-back main-loop frames. No new synchronization primitives.

- **Alternatives:**
  - **Keep dedicated `ResizeFrameResources`:** Would be defensible if the callback ran on a different thread, but D2 establishes it does not. Strictly more code for no functional gain. Rejected.

### D4: Recreate swapchain inside the callback when dimensions change; otherwise just render
The callback queries `glfwGetFramebufferSize`, compares to `swapchain.extent`, and only invokes `vkDeviceWaitIdle` + `destroySwapchain` + `createSwapchain` if they differ. This handles three cases:
- **First event of a drag**: dimensions just changed, swapchain stale → recreate, then render.
- **Same-size refresh events**: dimensions match, no recreate → just render.
- **Zero-size (minimized)**: bail out cleanly without recreate or render.

After a recreate, the callback also updates `ImGui_ImplVulkan_SetMinImageCount` and `RenderContext` fields so subsequent renders use the new swapchain's image count.

### D5: Composite alpha back to `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`
The transparent-framebuffer / post-multiplied-alpha hooks from the previous iteration are removed. The window is opaque again — this is the right default for a normal app window, and there's no longer a translucent overlay to support.

### D6: Main loop simplifies to `glfwPollEvents()` + `renderOneFrame()`
The previous "swapchain-recreated-during-modal-loop, skip-recreate-but-update-ImGui-state" block goes away. `renderOneFrame` handles its own `OUT_OF_DATE` / `SUBOPTIMAL` results inline. `g_framebufferResized` is still set by the callback for diagnostic purposes but no longer drives a dedicated branch in the main loop — clearing it each iteration is sufficient.

## Risks / Trade-offs

- **[Risk: ImGui re-entrancy from the callback]** → Mitigation: D2's safety argument (main thread, between frames). Validated by code review of the call-graph: `glfwPollEvents` is the only place from which Cocoa's modal loop is entered, and `renderOneFrame` always completes before the next `glfwPollEvents`.
- **[Risk: a runaway resize gesture submits dozens of frames per second]** → Acceptable. Each frame is identical work to a normal-loop frame; the per-frame-in-flight pool handles throttling. Worst case is GPU saturation, which is visible as input lag during a fast wiggle — not a functional defect.
- **[Risk: GLFW window-refresh callback fires for non-resize events (e.g., expose)]** → Acceptable — the callback just renders an extra ImGui frame. The next normal-loop frame overwrites it within ms.
- **[Risk: Vulkan validation flags semaphore reuse between callback frames and main-loop frames]** → Mitigation: per-image `renderFinishedSemaphores` (existing structure from a prior bug fix) means each acquired image has its own signal semaphore. Callback and main loop alternate cleanly through the per-image pool.
- **[Trade-off: the callback can submit frames faster than the main loop's natural cadence during a drag]** → Acceptable. Frame submission is rate-limited by the fence in the per-frame-in-flight pool; the GPU/display-engine block presents that exceed `MAX_FRAMES_IN_FLIGHT`.

## Migration Plan

Not applicable in the strict sense — this is a UX fix. Rollback = revert this change; the OS-driven zoom artifact comes back. No persisted state, no API change visible to `app/` consumers.

The earlier "freeze + darken-fill" iteration's code paths are removed by this change. If they're ever needed again (e.g., for a UX experiment), the git history of this change directory has the full diff.

## Open Questions

- **Should `app::frame` know it's being rendered from inside a resize?** Some app code might want to suppress expensive computations during a drag. Out of scope for v1; trivially addable later via a `bool app::isResizing()` setter the callback can flip on entry and clear on exit. Defer until there's a consumer.
- **Should the callback rate-limit itself?** Cocoa can fire callbacks at very high frequency during a fast drag. We currently render every event; an alternative is to coalesce events and render at e.g. 60 Hz max. Acceptable to defer — the main concern (zoom artifact) is solved by even one fresh frame per event, and the fence pool prevents work piling up.
