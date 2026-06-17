## Why

The shell currently lives inside a single GLFW window. Every ImGui sub-window the user opens (the demo, the future Preferences dialog) is clipped to that main window's bounds — drag it to the edge and it just disappears under the title bar. ImGui's docking branch (which we already pin) supports **multi-viewport** mode, where any ImGui window dragged outside the host can become a real OS-level secondary window with its own swapchain. This change wires the Vulkan + GLFW plumbing required for that mode on desktop, with no user-facing UI changes — purely infrastructure that the upcoming `add-preferences-dialog` change will use to make Preferences a real draggable-anywhere window.

The motivation is concrete: the parked `add-preferences-dialog` proposal targets a Preferences window that lives outside the main shell window, and `Help > ImGui Demo` already exercises sub-windows that benefit from being detachable. Doing the multi-viewport rework in isolation now keeps the Preferences change small later and lets us verify the infrastructure on its own.

## What Changes

- In `app::init`, set `io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable` on desktop only (gated by `app::kIsDesktop` from `Platform.h`). On iOS the flag stays off — iOS has a single-window OS and `ImGui_ImplGlfw` doesn't run there anyway.
- In `platform/desktop/main.cpp`, populate the multi-viewport-related fields on `ImGui_ImplVulkan_InitInfo` and register the per-viewport platform/render callbacks. ImGui's own `examples/example_glfw_vulkan/main.cpp` is the canonical reference; we follow its pattern — `ImGui_ImplVulkanH_Window` per viewport, `ImGui_ImplVulkanH_CreateOrResizeWindow` for swapchain creation, and the `Platform_*` callbacks ImGui calls when the user drags a window out.
- Update the per-frame submit to call `ImGui::UpdatePlatformWindows()` + `ImGui::RenderPlatformWindowsDefault()` after the main window's `RenderDrawData` + before the present, so secondary viewports get their own command buffers submitted each frame.
- Confirm interaction with the existing live-resize callback (`resizeCallbackWork`): the callback handles only the **main** GLFW window's resize. Secondary viewport resizes are handled by ImGui's per-viewport `Renderer_SetWindowSize` callback, which routes through `ImGui_ImplVulkanH_CreateOrResizeWindow`. No conflict.
- Confirm interaction with the existing shutdown order: secondary viewports SHALL be torn down BEFORE the main `ImGui_ImplVulkan_Shutdown` (which destroys per-viewport state ImGui owns) and BEFORE `app::shutdown` (which destroys the ImGui context). ImGui's `Shutdown` routines already cascade through the viewport list; our shutdown code path doesn't need explicit per-viewport teardown — but the spec scenario calls this out so a future contributor doesn't introduce a regression.

Non-goals (called out to bound scope):
- **No UI change visible to the user.** This sub-change adds zero new menus, dialogs, or widgets. The only observable difference: dragging the existing `Help > ImGui Demo` window outside the main window now produces a real OS window instead of clipping at the host's edge.
- **No JSON persistence** — that's `add-theme-persistence`, sub-change 2.
- **No Preferences dialog UI** — that's `add-preferences-dialog`, sub-change 3.
- **No iOS support for multi-viewport.** iOS is single-window; the `ViewportsEnable` flag is not set, the secondary-viewport code paths never run.
- **No new dependencies.** All needed APIs already ship in the pinned ImGui + GLFW versions.
- **No mouse-clipping / multi-monitor refinement** beyond what ImGui's defaults give us.

## Capabilities

### New Capabilities
<!-- None — this is an infrastructure rework scoped entirely within the render-backend capability. -->

### Modified Capabilities

- `render-backend`: A new requirement "Multi-viewport support on desktop" is added, specifying the ImGui flag, the callback registration pattern, the per-frame `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` call sites, and the shutdown ordering contract. iOS scenarios are unaffected (iOS doesn't enable the flag).

## Impact

- **Code change:** ~80–120 LOC net.
  - `apps/imgui-shell/app/App.cpp` — one conditional flag set in `init` (~5 LOC).
  - `apps/imgui-shell/platform/desktop/main.cpp` — multi-viewport callback registration on the `ImGui_ImplVulkan_InitInfo`, plus the new `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` calls in the main loop after `RenderDrawData` (~30–50 LOC). Possibly a couple of small helpers if the example code reads cleaner as named functions.
  - `apps/imgui-shell/platform/ios/...` — no change (multi-viewport is desktop-only).
- **New dependency:** none. All required APIs ship in the pinned ImGui 1.91.9-docking and GLFW 3.4.
- **Spec change:** one new requirement added to `openspec/specs/render-backend/spec.md` (ADDED Requirements delta). Other render-backend requirements unaffected.
- **CI:** no workflow changes. Multi-viewport doesn't add build deps or platform variants.
- **Risk surface:**
  - **Validation-layer noise from the secondary viewport swapchain** — ImGui's `ImGui_ImplVulkanH_CreateOrResizeWindow` does its own swapchain creation; our existing render-finished-semaphore-per-image pattern is independent from it. Validation should be silent if we follow ImGui's example precisely; we mirror it line-for-line and verify under `VK_LAYER_KHRONOS_validation` in the verification tasks.
  - **macOS Cocoa multi-window quirks** — dragging a window out crosses spaces / display boundaries; ImGui's GLFW backend handles this, but it's the riskiest interactive verification target.
  - **iOS won't compile if we accidentally enable a desktop-only path on the iOS preset** — gating via `app::kIsDesktop` (compile-time constant from `Platform.h`) prevents that.
- **Backward compatibility:** none required — the feature is purely additive. Existing single-window behavior continues to work; new behavior unlocks only when the user actively drags a window outside the main host.

## Relationship to the parked `add-preferences-dialog` change

This is the **first** of three sub-changes split out from the parked `openspec/changes/add-preferences-dialog/` proposal. The split is:

1. **`enable-multi-viewport`** (this change) — infrastructure.
2. **`add-theme-persistence`** (next, not yet scaffolded) — JSON I/O, no UI.
3. **`add-preferences-dialog`** (currently parked) — the actual Preferences window UI; depends on (1) and (2).

When all three are archived, the parked change directory can be removed.
