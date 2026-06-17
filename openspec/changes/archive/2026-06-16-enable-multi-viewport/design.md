## Context

ImGui's docking branch (which we pin at `v1.91.9-docking`) exposes a **multi-viewport** mode behind `ImGuiConfigFlags_ViewportsEnable`. When enabled, ImGui detects when the user drags a window outside the main host's bounds and instructs the platform backend to create a real OS-level secondary window for that ImGui window. The renderer backend (in our case, `ImGui_ImplVulkan`) creates a per-viewport swapchain to render into. The host application's job is to register the platform-side callbacks (window create/destroy/move/resize) and call two specific ImGui functions in the main loop after rendering the primary viewport.

The reference implementation lives in ImGui's `examples/example_glfw_vulkan/main.cpp`. We follow it line-for-line where possible — the API surface is large, the invariants are subtle (e.g., the `ImGui_ImplVulkanH_Window` struct ImGui's renderer keeps per viewport is NOT the same as our existing `desktop::Swapchain`), and divergence from the reference doubles the risk for no benefit.

The existing single-window infrastructure (our `desktop::Instance` / `desktop::Device` / `desktop::Swapchain` / `desktop::FrameResources` / `desktop::renderOneFrame`) is preserved unchanged — it remains the path the **main viewport** uses. Multi-viewport adds a parallel code path that ImGui owns and we plumb into, not a replacement.

## Goals / Non-Goals

**Goals:**
- Enable ImGui multi-viewport mode on desktop (macOS / Windows / Linux) so any ImGui window the user drags outside the main host becomes a real OS window with its own swapchain.
- Mirror ImGui's `example_glfw_vulkan` reference for the per-viewport platform/renderer callback wiring — keep divergence to zero.
- Keep the existing main-loop render path (`renderOneFrame`, `glfwSet*Callback`, live-resize behavior) untouched; multi-viewport is additive.
- Preserve the shutdown-order contract from `fix-desktop-shutdown-crash` — ImGui backend shutdowns BEFORE `app::shutdown`, and ImGui's internal viewport-list teardown happens inside those shutdowns automatically.
- No iOS impact — iOS doesn't enable the flag and doesn't compile any of the new code paths.

**Non-Goals:**
- No UI changes. No new menus, no new dialogs. (Sub-changes 2 and 3 add those.)
- No JSON persistence of viewport positions or window state. (Sub-change 2.)
- No support for non-default present modes on secondary viewports.
- No MSAA / depth on secondary viewports.
- No per-monitor DPI scaling refinements beyond what ImGui's defaults give us.
- No hand-rolling of multi-viewport callbacks — we use ImGui's bundled helpers (`ImGui_ImplVulkanH_*`).
- No mouse cursor virtualization across viewports beyond ImGui's `ConfigFlags_ViewportsEnable` defaults.

## Decisions

### D1: Use ImGui's bundled multi-viewport helpers (`ImGui_ImplVulkanH_*`), not a hand-rolled implementation
ImGui ships `ImGui_ImplVulkanH_Window` (a struct bundling swapchain + image views + render pass + framebuffers + per-frame fences for one viewport), `ImGui_ImplVulkanH_CreateOrResizeWindow` (creates / recreates that struct), and the matching `ImGui_ImplVulkanH_DestroyWindow`. These are designed for exactly the multi-viewport callback flow. Re-using them means our secondary-viewport code is `~30 LOC of callback boilerplate` instead of `~300 LOC of swapchain plumbing`.

- **Alternatives:**
  - **Hand-roll secondary swapchains using our existing `desktop::createSwapchain` / `destroySwapchain`:** Duplicates code that ImGui already maintains. Rejected — diverges from the reference, harder to keep in sync with future ImGui releases.
  - **Use Vulkan dynamic rendering (no render pass) for secondary viewports:** Would deviate from the main viewport's render-pass-based path. Heterogeneous render strategies multiply review surface. Rejected for v1.

### D2: Per-viewport `ImGui_ImplVulkanH_Window` lifecycle managed entirely through ImGui's callbacks
The renderer-side callbacks ImGui invokes when a secondary viewport is created / resized / destroyed look like this (paraphrased from the example):

```
ImGui::GetPlatformIO().Renderer_CreateWindow  = ImGui_ImplVulkan_CreateWindow;   // ImGui-provided default
ImGui::GetPlatformIO().Renderer_DestroyWindow = ImGui_ImplVulkan_DestroyWindow;
ImGui::GetPlatformIO().Renderer_SetWindowSize = ImGui_ImplVulkan_SetWindowSize;
ImGui::GetPlatformIO().Renderer_RenderWindow  = ImGui_ImplVulkan_RenderWindow;
ImGui::GetPlatformIO().Renderer_SwapBuffers   = ImGui_ImplVulkan_SwapBuffers;
```

These defaults already live inside `imgui_impl_vulkan.cpp`; calling `ImGui_ImplVulkan_Init` with proper `ImGui_ImplVulkan_InitInfo` is enough to install them. The same is true for the platform-side callbacks via `ImGui_ImplGlfw_InitForVulkan(window, true)` — the `true` flag installs the GLFW platform callbacks for multi-viewport (and we already pass `true`, from the previous app-shell change).

So the actual code we add is minimal:
- Set the `ConfigFlags_ViewportsEnable` flag in `app::init` (desktop-gated).
- Possibly populate `ImGui_ImplVulkan_InitInfo.MinAllocationSize` and the `Allocator` field (we leave both at defaults).
- In the main loop, after `ImGui::Render()` and `ImGui_ImplVulkan_RenderDrawData(...)`, call `ImGui::UpdatePlatformWindows()` followed by `ImGui::RenderPlatformWindowsDefault()`. The first one walks the viewport list and calls the platform callbacks; the second walks it again and calls the renderer callbacks.

- **Alternatives:**
  - **Override the default callbacks with ours:** Would let us re-use `desktop::Swapchain` per viewport. Tempting but contradicts D1; we'd be reinventing the wheel.

### D3: `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` slot in BETWEEN main-viewport draw-data submission and `vkQueuePresentKHR`
Looking at our existing `renderOneFrame` (extracted in `defer-relayout-during-window-resize`):

```cpp
// 1. Acquire main swapchain image
// 2. Begin command buffer + render pass
// 3. ImGui_Impl* NewFrame + app::frame (builds ImGui draw data) + ImGui::Render
// 4. ImGui_ImplVulkan_RenderDrawData(main draw_data, f.commandBuffer)
// 5. End render pass + end command buffer
// 6. Submit + present
```

The multi-viewport calls land between step 4 and step 5 — actually between step 5 and step 6, in ImGui's reference:

```cpp
// ... step 5: end render pass + end command buffer ...

// Multi-viewport pass:
ImGuiIO& io = ImGui::GetIO();
if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

// ... step 6: submit + present main ...
```

`RenderPlatformWindowsDefault` internally submits + presents each secondary viewport. We don't manage those secondary swapchains' command buffers / semaphores / fences — ImGui's `ImGui_ImplVulkan_RenderWindow` handles all of that per viewport via the `ImGui_ImplVulkanH_Window` ImGui owns.

- **Alternatives:**
  - **Call the multi-viewport functions BEFORE rendering the main viewport:** Wrong order; ImGui's reference rendering order has main viewport submit + present last so the user sees the main window updated even if a secondary viewport had a transient hiccup.

### D4: Interaction with the existing live-resize callback is zero
The live-resize callback (`resizeCallbackWork` from `defer-relayout-during-window-resize`) is registered on **only** the main GLFW window. When ImGui creates a secondary GLFW window via the platform callback, ImGui's own GLFW backend installs the appropriate callbacks (size, refresh, close, focus) on that secondary window — and the secondary window's resize is handled by `Renderer_SetWindowSize` → `ImGui_ImplVulkanH_CreateOrResizeWindow` → secondary swapchain recreate. The main window's `resizeCallbackWork` never fires for secondary windows because it's bound to the main `GLFWwindow*` only.

- **Alternatives:**
  - **Install our `resizeCallbackWork` on every secondary window too:** Would conflict with ImGui's own GLFW callback wiring. Rejected.

### D5: Shutdown order is unchanged — secondary viewports are torn down inside `ImGui_ImplVulkan_Shutdown`
The shutdown-order rule from `fix-desktop-shutdown-crash` stays: `vkDeviceWaitIdle` → `ImGui_ImplVulkan_Shutdown` → `ImGui_ImplGlfw_Shutdown` → `app::shutdown` → reverse-order Vulkan teardown. `ImGui_ImplVulkan_Shutdown` walks the viewport list internally and calls `ImGui_ImplVulkan_DestroyWindow` for each secondary viewport before tearing down its own state. Same for `ImGui_ImplGlfw_Shutdown` and the secondary GLFW windows. We don't have to do anything explicit.

The defensive assertions we added in `app::shutdown` (checking `BackendRendererUserData == nullptr` and `BackendPlatformUserData == nullptr`) continue to work — both flags are cleared inside the respective Shutdown calls regardless of how many viewports were alive.

- **Alternatives:**
  - **Explicitly walk viewports and call `DestroyWindow` ourselves before `Shutdown`:** Duplicates ImGui's internal teardown. Rejected.

### D6: Desktop-only gate via `app::kIsDesktop` from `Platform.h`
The flag set in `app::init` is guarded by `if constexpr (kIsDesktop) { io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; }` — same pattern the existing `app/App.cpp` uses to keep the source set platform-uniform per `app-shell` spec. On iOS, the flag is never set, so the secondary-viewport code paths never run.

`platform/ios/` is untouched. The iOS host uses `ImGui_ImplMetal`, not `ImGui_ImplVulkan`, so even if a viewport flag did get set, the Metal backend's multi-viewport callbacks would handle it — but for v1 iOS, multi-viewport is explicitly off.

- **Alternatives:**
  - **`#ifdef IMGUI_SHELL_PLATFORM_DESKTOP`:** Would violate the no-`#ifdef`-in-app/ source files rule. `if constexpr` keeps the source uniform.

## Risks / Trade-offs

- **[Risk: validation-layer warnings from secondary-viewport swapchain creation]** → Mitigation: ImGui's `ImGui_ImplVulkanH_CreateOrResizeWindow` is the reference implementation; if it had validation issues we'd be hearing about them upstream. We verify under `VK_LAYER_KHRONOS_validation` interactively.
- **[Risk: macOS Cocoa multi-window quirks — secondary windows in a different space, on a different monitor, in full-screen mode]** → Mitigation: ImGui's GLFW backend handles these via standard GLFW APIs (`glfwGetWindowMonitor`, `glfwGetWindowFrameSize`, etc.). We don't intercept any of it.
- **[Risk: a secondary viewport's swapchain recreate happens during Cocoa's modal resize loop — our `resizeCallbackWork` runs concurrently]** → Acceptable: the callback is bound to the main window only, and the modal loop only fires for the window being dragged. Secondary windows are dragged independently; cross-window interleaving isn't a real scenario.
- **[Risk: image-count / `MinImageCount` mismatch between the main swapchain and secondary swapchains]** → Mitigation: ImGui's `ImGui_ImplVulkanH_CreateOrResizeWindow` picks each secondary viewport's image count from `ImGui_ImplVulkan_InitInfo.MinImageCount` (which we already set from the main swapchain). The main and secondary counts are de-facto consistent.
- **[Trade-off: we relinquish control of secondary swapchain creation to ImGui]** → Acceptable per D1. If we ever need fine control (e.g., to force a specific present mode on secondary viewports), we override the `Renderer_CreateWindow` callback to point at our own helper. Documented as a follow-up if needed.

## Migration Plan

Not applicable in the strict sense. Rollback = revert this change; the flag stays unset, the secondary-viewport code paths never run, ImGui clips windows at the host's bounds as today. No persisted state, no data migration.

## Open Questions

- **Should we keep ImGui's default mouse-cursor behavior across viewports?** ImGui's docking branch has an `ImGuiConfigFlags_DpiEnableScaleViewports` flag for HiDPI multi-monitor setups. Default is off; we leave it off. Revisit if testing reveals cursor / DPI issues on multi-monitor configurations.
- **Should secondary viewports get the same `IMGUI_SHELL_THEME_NAME` compile-def?** They share the global ImGui context, so they automatically use the curated theme. No special handling needed — but worth confirming in interactive testing.
- **macOS notarization implications of multiple windows?** Out of scope (no notarization in v1) but flag for the eventual signing change.
