## ADDED Requirements

### Requirement: Live-resize behavior on desktop
The desktop host SHALL render a full ImGui frame from inside the GLFW resize / refresh callbacks so the window contents re-lay-out live during a user-driven resize gesture, eliminating the OS-driven zoom/stretch artifact that occurs when the application's normal main loop is paused inside Cocoa's modal NSRunLoop. The callback path SHALL share the same render code, `FrameResources` pool, and `frameIndex` counter as the main loop — there is no second "minimal" render path.

#### Scenario: Resize / refresh callbacks render a full ImGui frame
- **WHEN** `glfwSetWindowRefreshCallback` or `glfwSetFramebufferSizeCallback` fires (including from inside macOS Cocoa's modal resize loop)
- **THEN** the desktop host SHALL invoke the same `renderOneFrame()` code path that the main loop uses
- **AND** the rendered frame SHALL include the full ImGui sequence: `ImGui_ImplVulkan_NewFrame` → `ImGui_ImplGlfw_NewFrame` → `app::frame(ctx)` (which calls `ImGui::NewFrame`, builds UI, calls `ImGui::Render`) → `ImGui_ImplVulkan_RenderDrawData(GetDrawData(), commandBuffer)` → submit → present
- **AND** the result SHALL be visible at the current framebuffer dimensions (the menu bar, About dialog, demo window all re-lay-out live during the drag)

#### Scenario: Swapchain recreate is gated on actual dimension change
- **WHEN** a resize / refresh callback fires
- **THEN** the host SHALL query the framebuffer size via `glfwGetFramebufferSize`
- **AND** SHALL invoke `vkDeviceWaitIdle` + `destroySwapchain` + `createSwapchain` ONLY if the queried dimensions differ from `swapchain.extent`
- **AND** when the dimensions are zero (minimized window) the callback SHALL bail out cleanly without recreating the swapchain or rendering a frame
- **AND** after a recreate, the host SHALL update `ImGui_ImplVulkan_SetMinImageCount` and the relevant `RenderContext` fields (`renderPass`, `minImageCount`, `imageCount`) so the subsequent `renderOneFrame` uses the new swapchain

#### Scenario: Single shared frame-resource pool between callback and main loop
- **WHEN** the callback and the main loop both submit frames
- **THEN** they SHALL share the same `FrameResources` pool
- **AND** they SHALL share the same `frameIndex` counter (passed by reference into `renderOneFrame`)
- **AND** the host SHALL NOT allocate a separate `ResizeFrameResources` or duplicate command pool / semaphores / fences for the callback path
- **AND** the host SHALL NOT require any thread-synchronization primitives between the callback and the main loop (both run on the main thread; the callback only fires from inside `glfwPollEvents` re-entered via Cocoa's modal loop)

#### Scenario: Callback is safe to call ImGui because it runs on the main thread between frames
- **WHEN** the resize / refresh callback executes
- **THEN** it SHALL be executing on the same thread that called `ImGui::CreateContext` (the main thread on macOS, in a re-entered NSRunLoop mode)
- **AND** ImGui SHALL be in a "between frames" state — no `ImGui::NewFrame` is currently in flight from any other code path — because `renderOneFrame` is atomic between `glfwPollEvents` calls in the main loop

#### Scenario: Application exits cleanly via every desktop close path
- **WHEN** the desktop user closes the application during or after a live resize (window-X, `Cmd+Q`, or `File > Quit`)
- **THEN** the binary SHALL return exit code 0 with no `abort()`, no `IM_ASSERT` failure, and no Vulkan validation error
- **AND** the shutdown SHALL unregister both callbacks (`glfwSetFramebufferSizeCallback(window, nullptr)` and `glfwSetWindowRefreshCallback(window, nullptr)`) and clear the file-scope state pointer BEFORE tearing down ImGui backends and Vulkan objects, so a stray late callback cannot fire into freed state

#### Scenario: Windows and Linux are unaffected
- **WHEN** the desktop host runs on Windows or Linux and a resize event arrives via the normal `glfwPollEvents` path (no modal-loop semantics)
- **THEN** the callback SHALL still execute and render one extra ImGui frame
- **AND** the user-visible effect SHALL be invisible (one extra frame per resize event is below perceptual threshold; the main loop's own next frame replaces it within ~16 ms)
