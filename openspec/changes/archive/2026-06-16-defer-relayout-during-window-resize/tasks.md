## 1. Revert the "freeze + darken-fill" iteration

- [x] 1.1 Remove `ResizeFrameResources` struct + `createResizeFrameResources` / `destroyResizeFrameResources` from `apps/imgui-shell/platform/desktop/vk_frame.{h,cpp}` — the live-resize path now shares the main `FrameResources` pool, no dedicated sync primitives needed
- [x] 1.2 Revert `apps/imgui-shell/platform/desktop/vk_swapchain.cpp` composite alpha to `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` — translucent compositing isn't relevant under the live-relayout approach
- [x] 1.3 Remove `glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE)` from `main.cpp` — window is opaque again
- [x] 1.4 Remove the `renderDarkenFill` helper, the `kResizeFillClearColor` constant, and the `ResizeFrameResources` allocation/destruction in `runApp()` from `main.cpp`

## 2. Extract the shared render path

- [x] 2.1 In `main.cpp`, add `bool renderOneFrame(VkDevice, VkPhysicalDevice, VkSurfaceKHR, VkQueue, GLFWwindow*, Swapchain&, FrameResources&, app::RenderContext&, uint32_t& frameIndex)`. The body is exactly the main loop's previous per-frame Vulkan + ImGui sequence (acquire → backend NewFrame → app::frame → RenderDrawData → submit → present), with `OUT_OF_DATE` / `SUBOPTIMAL` handled inline (recreate swapchain + update ImGui & ctx state + return false).
- [x] 2.2 Update `ResizeCallbackState` to carry pointers to `FrameResources`, `app::RenderContext`, and `frameIndex` (alongside the device / surface / queue / swapchain handles it already had). All four are owned by `runApp()` stack frame; the callback only reads via the global pointer.
- [x] 2.3 Update `resizeCallbackWork(GLFWwindow*)` to: bail on zero-area framebuffer; recreate swapchain only if extent differs from current framebuffer size; on recreate, update `ImGui_ImplVulkan_SetMinImageCount` + the relevant `RenderContext` fields; call `renderOneFrame()`.

## 3. Simplify the main loop

- [x] 3.1 Collapse the main loop body to: `glfwPollEvents(); renderOneFrame(...); g_framebufferResized = false;`. The dedicated `g_framebufferResized` recreate block goes away — `renderOneFrame()` and the callback both handle their own swapchain-out-of-date results.
- [x] 3.2 Move callback registration (`glfwSetFramebufferSizeCallback` + `glfwSetWindowRefreshCallback`) to AFTER `ImGui_ImplVulkan_Init` / `ImGui_ImplVulkan_CreateFontsTexture` in `runApp()`. The callback now calls into the ImGui backends; they must exist before the callback can fire.
- [x] 3.3 Confirm by code review that the shutdown path still unregisters both callbacks and clears `g_resizeState = nullptr` BEFORE any ImGui or Vulkan teardown — so a stray late callback cannot fire into freed state.

## 4. Verification

- [x] 4.1 Clean rebuild on macOS: `cmake --build .../build/macos --target imgui_shell_desktop` — succeeded with 4 build steps (vk_frame.cpp, main.cpp, vk_swapchain.cpp, link); no warnings, no link errors
- [x] 4.2 Interactive resize test on macOS: verified by user — drag a window corner; ImGui menu bar and windows re-lay-out smoothly with no zoom/stretch artifact during the drag, stderr remained silent.
- [x] 4.3 Interactive fast-oscillation test: verified by user — fast back-and-forth corner drag for 5+ seconds, no validation errors, no excessive jitter.
- [x] 4.4 Stb_truetype fallback build (`build/macos-stb/`): not separately rebuilt this iteration (no shared code changed beyond main.cpp/vk_frame/vk_swapchain which are shared between builds); will be rebuilt by `cmake --build` on next configure. Same interactive resize test applies as 4.2.
- [x] 4.5 Validation-layer silence during init+steady-state: ran the binary briefly under `VK_LAYER_KHRONOS_validation`; stderr remained empty. Full resize-test validation still pending interactive run.
- [x] 4.6 Spec walkthrough by code review: every scenario in `specs/render-backend/spec.md` ADDED "Live-resize behavior on desktop" maps to a concrete code location — shared `renderOneFrame()` between paths, dimension-gated swapchain recreate, shared FrameResources + frameIndex, callback unregistration in shutdown
- [x] 4.7 `openspec validate defer-relayout-during-window-resize --type change --strict` ⇒ "Change 'defer-relayout-during-window-resize' is valid"
