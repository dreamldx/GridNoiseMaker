## Context

`platform/desktop/main.cpp` currently calls `app::shutdown()` (which invokes `ImGui::DestroyContext` → `ImGui::Shutdown`) *before* the ImGui backend `*_Shutdown()` calls. ImGui's own `Shutdown()` asserts on `BackendPlatformUserData != NULL` and `BackendRendererUserData != NULL`, so the assertion fires and `abort()` is called. The `render-backend` spec already requires the correct order; the implementation just doesn't follow it. The bug shipped because the previous apply session SIGTERM'd the binary at 3 seconds instead of driving an actual close, leaving the cleanup path entirely unexercised — exactly the failure mode the deferred `5.5` / `7.4` tasks warned about.

This design covers a small, mechanical change: reorder four function calls, add one defensive assert, tighten one spec scenario, and add an interactive close-path verification task that the previous change skipped.

## Goals / Non-Goals

**Goals:**
- Make every desktop exit path (`File > Quit`, window-X, OS-driven termination) return cleanly with exit code 0, no `abort()`, no ImGui assertion, no Vulkan validation message.
- Ensure that a future contributor who reorders the teardown wrong fails *at the host's call site* with a clear assertion message, not deep inside ImGui's guts.
- Close the test-coverage gap that allowed the bug to land: the new `render-backend` scenario explicitly requires both close paths to exit cleanly, and the task list includes an interactive verification step (no longer deferred).

**Non-Goals:**
- No conversion of the desktop teardown into RAII wrappers — the linear, explicit sequence is fine and readable; we just need it ordered correctly. (A later refactor can introduce RAII if the file grows.)
- No iOS host changes. The iOS `dealloc` path already calls `app::shutdown` after `ImGui_ImplMetal_Shutdown` in the right order; that path's verification remains pending full Xcode.
- No CI workflow changes. The interactive close test is local-only for now; promoting it to automated would require a headless driver (out of scope).
- No `app::frame`/`app::init` changes. The bug is purely in `platform/desktop/main.cpp` plus a defensive assert in `app/App.cpp::shutdown`.

## Decisions

### D1: Reorder the desktop teardown to match `render-backend` spec
The exact new sequence:

```cpp
// loop exits when glfwWindowShouldClose(window) || app::quitRequested()
vkDeviceWaitIdle(device.device);

// ImGui backends first (renderer, then platform) — matches spec, matches
// ImGui's own internal "Forgot to shutdown ... backend?" assertion.
ImGui_ImplVulkan_Shutdown();
ImGui_ImplGlfw_Shutdown();

// Now safe to destroy the ImGui context.
app::shutdown();

// Then Vulkan + GLFW objects in reverse creation order.
desktop::destroyFrameResources(device.device, frames);
vkDestroyDescriptorPool(device.device, imguiPool, nullptr);
desktop::destroySwapchain(device.device, swapchain);
desktop::destroyDevice(instance.handle, device);
desktop::destroyInstance(instance);
glfwDestroyWindow(window);
glfwTerminate();
```

The only conceptual change vs. the current buggy code is moving the two `*_Shutdown()` calls from below `app::shutdown()` to above it. Everything else stays.

- **Alternatives:**
  - **Move `ImGui::DestroyContext` out of `app::shutdown` and into the host:** Would let the host own all ImGui lifecycle, but violates the existing `app-shell` spec's "ImGui context ownership" requirement which says the *shared core* must own the context. Rejected.
  - **Add backend shutdowns inside `app::shutdown`:** Would couple `app/` to platform-specific backend headers (`imgui_impl_vulkan.h`, `imgui_impl_metal.h`), defeating the platform-agnostic source-set rule. Rejected.

### D2: Add a defensive assert in `app::shutdown` before destroying the context
At the top of `app::shutdown()`, before `ImGui::DestroyContext`:

```cpp
ImGuiIO& io = ImGui::GetIO();
assert(io.BackendRendererUserData == nullptr &&
       "Host called app::shutdown() before ImGui_ImplVulkan/Metal_Shutdown() — "
       "see specs/render-backend 'Correct shutdown order'");
assert(io.BackendPlatformUserData == nullptr &&
       "Host called app::shutdown() before ImGui_ImplGlfw_Shutdown() — "
       "see specs/render-backend 'Correct shutdown order'");
```

This trips the *host's* call site (with a spec reference in the message) instead of crashing inside ImGui's own internals with a less actionable backtrace. The two existing ImGui assertions still fire as a safety net in release-without-NDEBUG builds.

- **Alternatives:**
  - **Skip the defensive assert; trust ImGui's own:** Cheaper but the existing ImGui backtrace gives the contributor no pointer to the spec. The defensive assert costs us 4 lines and points the next reader at the exact rule.
  - **Make it `IM_ASSERT` instead of `assert`:** They're the same in practice; `assert` keeps `app/` from depending on an ImGui-specific macro at this layer. Keeping `assert`.

### D3: Tighten the `render-backend` "Correct shutdown order" requirement (MODIFIED)
The existing scenario is correct but doesn't expose *why* the order matters — a casual reader could think it's stylistic. Tighten it to mention the ImGui-internal assertion that enforces the order, and add a new scenario that explicitly requires both close paths to exit cleanly. Keeping the same requirement *name* so it merges into the existing capability cleanly.

- **Alternatives:**
  - **Add a brand-new requirement instead of modifying:** Splits the shutdown contract across two requirements, harder to find. Rejected.
  - **Just add a task without changing the spec:** Lets the spec stay silent on a rule that's load-bearing for correctness. Rejected.

### D4: Promote the previously-deferred interactive close-path verification into a concrete task
The archived `imgui-cross-platform-app` change had `5.5` and `7.4` marked `**DEFERRED:**`. The interactive close path is exactly what would have caught this bug. Add a single task on this change that requires running the binary, picking `File > Quit`, then re-running and closing via the window-X — and confirming exit code 0 + no abort in stderr. No CI automation; just a manual checklist item with a deterministic pass criterion.

- **Alternatives:**
  - **Headless driver / automated GUI test:** Out of scope for this change; would require dragging in something like `pyautogui` or an X virtual framebuffer + screenshot diff. Defer.
  - **Skip the task; rely on the new spec scenario alone:** The scenario is a contract; the task is the verification. Both are needed.

## Risks / Trade-offs

- **[Risk: defensive assert fires in unexpected legitimate cases]** → Mitigation: the assertion only checks the two `Backend*UserData` fields ImGui itself uses to track backend ownership. If a custom backend doesn't set these, the assertion will (correctly) flag that the host didn't call its `*_Shutdown` function — which is what we want.
- **[Risk: reorder breaks some untested code path]** → Mitigation: the spec explicitly requires the new order; the only path affected is the desktop teardown. iOS isn't touched.
- **[Risk: future contributors disable assertions in release builds and ship with the bug]** → Mitigation: ImGui's own assertions still fire in any build where its `IM_ASSERT` is non-empty (default). Even if both layers are disabled, the new spec scenario forces a manual close-test before archiving.
- **[Trade-off: the defensive assert adds a soft coupling between `app/` and the ImGui-backend invariant]** → Acceptable: `app/` already depends on ImGui (it owns the context); checking ImGui's own state doesn't add new dep surface.

## Migration Plan

Not applicable in the usual sense — this is a bug fix. Rollback path = revert this commit; the broken behavior comes back. There is no persisted state, no API change visible to downstream consumers, no data to migrate.

## Open Questions

- **Should we add a similar defensive assert to the iOS `dealloc` path?** The iOS host's `ViewController::dealloc` already calls `app::shutdown` after `ImGui_ImplMetal_Shutdown`, so the order is correct. Adding the assert there is symmetric and cheap; including it as part of this change is reasonable. *(Resolution: include it — the assert lives in `app::shutdown`, which is shared, so both platforms benefit automatically with no extra code.)*
