## Why

The desktop binary aborts on every exit path (`File > Quit` and the OS window-close button), with the following crash signature:

```
0   libsystem_kernel.dylib   __pthread_kill + 8
1   libsystem_pthread.dylib  pthread_kill + 288
2   libsystem_c.dylib        abort + 180
3   libsystem_c.dylib        __assert_rtn + 284
4   imgui_shell_desktop      ImGui::Shutdown() + 160
5   imgui_shell_desktop      ImGui::DestroyContext(ImGuiContext*) + 68
6   imgui_shell_desktop      app::shutdown() + 112
```

The assertion is `ImGui::Shutdown()`'s built-in check `IM_ASSERT(g.IO.BackendRendererUserData == NULL && "Forgot to shutdown Renderer backend?")` (and the matching one for the platform backend). It fires because `platform/desktop/main.cpp` currently calls `app::shutdown()` (which destroys the ImGui context) **before** the ImGui backend shutdowns, exactly the opposite of what `specs/render-backend/spec.md` requires:

> the host SHALL shut down ImGui backends in reverse order: renderer backend (`ImGui_ImplVulkan_Shutdown`), platform backend (`ImGui_ImplGlfw_Shutdown`), then ImGui context destruction (the last performed by `app::shutdown`)

The bug shipped because the previous apply session smoke-tested the binary by `SIGTERM`ing it at 3 seconds rather than driving an actual close — the cleanup path was never exercised. Two `**DEFERRED:**` tasks (`5.5` and `7.4` in the now-archived `imgui-cross-platform-app` change) called this risk out explicitly, but it took an interactive run to trigger it.

## What Changes

- Reorder the desktop teardown sequence in `platform/desktop/main.cpp` so the ImGui backend shutdowns happen *before* `app::shutdown()`. The new order:
  1. `glfwSetWindowShouldClose(window, GLFW_TRUE)` (no-op if already true) so we have one canonical exit path
  2. `vkDeviceWaitIdle(device.device)`
  3. `ImGui_ImplVulkan_Shutdown()`
  4. `ImGui_ImplGlfw_Shutdown()`
  5. `app::shutdown()` — destroys the ImGui context (now safe; backends already released their `BackendRendererUserData`/`BackendPlatformUserData`)
  6. Destroy Vulkan objects in reverse creation order (unchanged)
  7. Destroy GLFW window + `glfwTerminate`
- Add a defensive ImGui-side guard: in `app::shutdown()`, assert that no backend is still bound (`IM_ASSERT(ImGui::GetIO().BackendRendererUserData == NULL && BackendPlatformUserData == NULL)`) so a future host that gets the order wrong fails immediately at the host's call site instead of deep inside `ImGui::Shutdown`.
- Tighten the `render-backend` spec scenario "Correct shutdown order" to (a) call out the specific `ImGui::Shutdown` assertion that enforces the ordering, and (b) add a new scenario that requires the binary to exit cleanly via *both* the window-close button and the in-app `File > Quit` item — closing the test-coverage gap that let this ship.
- Promote the previously-deferred `5.5` / `7.4` interactive close-path verification from the archived `imgui-cross-platform-app` change into a concrete task on this change, so it's actually run before this change archives.

Non-goals (called out to bound scope):
- No refactor of the desktop teardown into RAII wrappers — the linear sequence is fine; we just need it in the right order.
- No change to the iOS host (its `dealloc` path already follows reverse order; verification of that remains pending full Xcode).
- No new abstraction over backend lifecycle.

## Capabilities

### New Capabilities
<!-- None — this is a bug fix with no new capability surface. -->

### Modified Capabilities

- `render-backend`: The "Correct shutdown order" requirement is tightened with (a) an explicit reference to ImGui's internal assertion that enforces the ordering and (b) a new scenario requiring both desktop close paths (window-X and `File > Quit`) to exit with no abort / no validation messages.

## Impact

- **Code change:** ~10 lines in `apps/imgui-shell/platform/desktop/main.cpp` (reorder of the teardown block) and ~3 lines in `apps/imgui-shell/app/App.cpp` (defensive assert in `app::shutdown`). No file additions, no dependency changes.
- **Spec change:** one modified requirement + one new scenario in `openspec/specs/render-backend/spec.md`.
- **CI:** no workflow changes; the existing build matrix continues to verify compile + link. The new "exit cleanly on close" scenario is verifiable only interactively for now (same constraint as `5.5`).
- **Risk:** the fix is mechanical — moving four calls to their correct positions per a spec we already wrote. The defensive assert may surprise a future contributor who skips backend shutdown intentionally, but the assertion message will tell them exactly which spec they're violating.
- **Backward compatibility:** none required — this fixes a regression introduced by the same change in v1; no consumer depends on the broken behavior.
