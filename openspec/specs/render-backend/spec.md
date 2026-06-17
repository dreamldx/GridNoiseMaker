# render-backend Specification

## Purpose
TBD - created by archiving change imgui-cross-platform-app. Update Purpose after archive.
## Requirements
### Requirement: Desktop backend stack
The desktop targets (macOS, Windows, Linux) SHALL use GLFW for windowing and input and Vulkan 1.2 for rendering, with Dear ImGui's `imgui_impl_glfw` and `imgui_impl_vulkan` backends. On macOS, the Vulkan implementation SHALL be provided by MoltenVK (bundled with the LunarG Vulkan SDK); on Windows and Linux, the native Vulkan loader SHALL be used.

#### Scenario: Backend selection at build time
- **WHEN** the project is configured for the `macos`, `windows`, or `linux` preset
- **THEN** GLFW SHALL be linked
- **AND** the Vulkan loader library SHALL be linked (`vulkan-1` on Windows, `libvulkan` on Linux, MoltenVK's loader on macOS)
- **AND** `imgui_impl_glfw.cpp` and `imgui_impl_vulkan.cpp` SHALL be compiled into the binary
- **AND** the iOS-specific Metal backend SHALL NOT be compiled

#### Scenario: GLFW window created without a default client API
- **WHEN** the desktop host creates its window
- **THEN** it SHALL set the GLFW window hint `GLFW_CLIENT_API` to `GLFW_NO_API` before calling `glfwCreateWindow` (so GLFW does not create an OpenGL context)
- **AND** it SHALL verify Vulkan support via `glfwVulkanSupported()` and fail fast with a clear error message if support is absent

#### Scenario: Vulkan instance creation
- **WHEN** the desktop host starts
- **THEN** it SHALL create a `VkInstance` with API version 1.2, the GLFW-required instance extensions (from `glfwGetRequiredInstanceExtensions`), and (in debug builds only) `VK_LAYER_KHRONOS_validation` plus `VK_EXT_debug_utils`
- **AND** it SHALL fail to start with a clear error message if instance creation fails

#### Scenario: Surface, device, and swapchain initialization
- **WHEN** the desktop host has a `VkInstance`
- **THEN** it SHALL create a `VkSurfaceKHR` via `glfwCreateWindowSurface`
- **AND** it SHALL select a physical device that supports graphics, presentation to the surface, and the `VK_KHR_swapchain` extension
- **AND** it SHALL create a logical device with one graphics+present queue
- **AND** it SHALL create a swapchain with at least two images, a present mode of `VK_PRESENT_MODE_FIFO_KHR` (guaranteed available), image views, a render pass, and matching framebuffers
- **AND** it SHALL recreate the swapchain (and dependent framebuffers/image views) when `VK_ERROR_OUT_OF_DATE_KHR` or `VK_SUBOPTIMAL_KHR` is observed, or on a window-resize event

#### Scenario: Per-frame synchronization and command recording
- **WHEN** the desktop host enters its main loop
- **THEN** it SHALL maintain at least `MAX_FRAMES_IN_FLIGHT = 2` of per-frame resources (command buffer, image-available semaphore, render-finished semaphore, in-flight fence)
- **AND** each frame SHALL: wait on the in-flight fence, `vkAcquireNextImageKHR`, reset and record a command buffer, submit with semaphores + fence, then `vkQueuePresentKHR`

### Requirement: iOS backend stack
The iOS target SHALL use UIKit for view hosting and Apple Metal for rendering, with Dear ImGui's `imgui_impl_metal` backend driven from a Metal view inside a `UIViewController`.

#### Scenario: Backend selection at build time
- **WHEN** the project is configured for the `ios` preset
- **THEN** `imgui_impl_metal.mm` SHALL be compiled into the binary
- **AND** GLFW SHALL NOT be linked
- **AND** the desktop GL backend sources SHALL NOT be compiled

#### Scenario: Metal device and command queue ownership
- **WHEN** the iOS host initializes
- **THEN** it SHALL create exactly one `MTLDevice` and one `MTLCommandQueue`, owned by the view controller
- **AND** the `CAMetalLayer` of the hosting view SHALL be configured with that device before `app::init` is called

### Requirement: Single rendering path per platform family
The project SHALL NOT introduce a runtime rendering-backend selector (no virtual `IRenderer` interface, no runtime switch between Vulkan and Metal). Backend selection is a build-time decision driven by the CMake preset.

#### Scenario: No runtime branching on backend
- **WHEN** the codebase is grepped for runtime backend dispatch
- **THEN** there SHALL be no abstract renderer interface with multiple concrete implementations selected at runtime
- **AND** backend-specific code SHALL be excluded from compilation by the build system, not guarded by runtime checks

### Requirement: ImGui backend initialization order
The platform host SHALL initialize ImGui backends in the order required by Dear ImGui: create ImGui context first (done by `app::init` per app-shell), then init the platform backend (`ImGui_ImplGlfw_InitForVulkan` / `ImGui_ImplMetal_Init`), then init the renderer backend (`ImGui_ImplVulkan_Init` with a populated `ImGui_ImplVulkan_InitInfo` / per-frame Metal pipeline setup). At shutdown, the order SHALL be exactly reversed; the renderer and platform backends MUST release their `BackendRendererUserData` / `BackendPlatformUserData` (by calling their respective `*_Shutdown()` functions) BEFORE `ImGui::DestroyContext` is invoked, because `ImGui::Shutdown()` itself asserts on those fields being null.

#### Scenario: ImGui descriptor pool allocation
- **WHEN** the desktop host initializes ImGui's Vulkan renderer backend
- **THEN** it SHALL pre-create a `VkDescriptorPool` sized for ImGui's font texture and per-frame image bindings (`maxSets >= 1000`, combined-image-sampler pool of the same size is sufficient per ImGui example)
- **AND** it SHALL pass that pool, the physical device, logical device, queue, queue family, render pass, and `MinImageCount`/`ImageCount` from the swapchain into `ImGui_ImplVulkan_InitInfo`

#### Scenario: Correct shutdown order
- **WHEN** the application shuts down
- **THEN** the host SHALL call `vkDeviceWaitIdle` before any teardown
- **AND** the host SHALL shut down ImGui backends in reverse order: renderer backend (`ImGui_ImplVulkan_Shutdown` / `ImGui_ImplMetal_Shutdown`) first, then platform backend (`ImGui_ImplGlfw_Shutdown` on desktop; iOS has no separate platform-backend shutdown), and only THEN ImGui context destruction (the last performed by `app::shutdown` via `ImGui::DestroyContext`)
- **AND** the host SHALL destroy Vulkan objects in reverse creation order (sync primitives → command pool → framebuffers → image views → swapchain → render pass → descriptor pool → device → surface → instance)
- **AND** invoking `app::shutdown` before either `*_Shutdown()` call SHALL trigger an assertion at `app::shutdown`'s call site (checking `ImGui::GetIO().BackendRendererUserData == nullptr` and `BackendPlatformUserData == nullptr`) with an assertion message pointing at this requirement — so the failure surfaces at the host's call site rather than deep inside `ImGui::Shutdown`

#### Scenario: Application exits cleanly via every desktop close path
- **WHEN** the desktop user closes the application via the window-X (or `Cmd+Q` on macOS)
- **THEN** the binary SHALL return exit code 0
- **AND** stderr SHALL contain no `abort()` message, no ImGui `IM_ASSERT` failure, and no Vulkan validation message at error severity
- **WHEN** the desktop user picks `File > Quit` from the in-app menu
- **THEN** the binary SHALL return exit code 0 with the same silence guarantees as above
- **WHEN** the desktop host receives a `SIGTERM` while the main loop is running
- **THEN** the binary MAY exit with a non-zero status (signal-driven termination), but if it does run any cleanup it SHALL do so in the order required by the "Correct shutdown order" scenario above (i.e., never call `ImGui::DestroyContext` while a backend's user-data is still bound)

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

### Requirement: Multi-viewport support on desktop
The desktop host SHALL enable ImGui's multi-viewport mode (`ImGuiConfigFlags_ViewportsEnable`) and SHALL wire the per-viewport platform + renderer callbacks so any ImGui window the user drags outside the main host becomes a real OS-level secondary window with its own GLFW window + Vulkan swapchain. The implementation SHALL re-use ImGui's bundled helpers (`ImGui_ImplVulkanH_*`, the default `Renderer_*` callbacks installed by `ImGui_ImplVulkan_Init`, and the GLFW multi-viewport hooks installed by `ImGui_ImplGlfw_InitForVulkan(window, true)`) rather than hand-rolling secondary swapchains. The iOS target SHALL NOT enable this flag — iOS is single-window.

#### Scenario: ViewportsEnable flag is set on desktop only
- **WHEN** `app::init` runs on a desktop target
- **THEN** it SHALL set `ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable`
- **WHEN** `app::init` runs on the iOS target
- **THEN** it SHALL NOT set the `ImGuiConfigFlags_ViewportsEnable` flag
- **AND** the gate SHALL be expressed via `if constexpr (app::kIsDesktop)` (no `#ifdef IMGUI_SHELL_PLATFORM_*` blocks in `app/` source files)

#### Scenario: ImGui default per-viewport callbacks are installed by standard init
- **WHEN** the desktop host calls `ImGui_ImplGlfw_InitForVulkan(window, /*install_callbacks=*/true)` and `ImGui_ImplVulkan_Init(&info)`
- **THEN** ImGui's default per-viewport callbacks (`Platform_CreateWindow`, `Platform_DestroyWindow`, `Platform_ShowWindow`, `Platform_SetWindowPos/Size/Title/Alpha`, `Renderer_CreateWindow`, `Renderer_DestroyWindow`, `Renderer_SetWindowSize`, `Renderer_RenderWindow`, `Renderer_SwapBuffers`) SHALL be registered on `ImGui::GetPlatformIO()` automatically by those init calls
- **AND** the host SHALL NOT override or replace any of these defaults in v1

#### Scenario: Multi-viewport submit + present happens after main viewport, before main present
- **WHEN** the desktop host's main loop renders a frame
- **THEN** the order of operations SHALL be: main-viewport render-pass + `ImGui_ImplVulkan_RenderDrawData` for the main draw data + end command buffer + `vkQueueSubmit` for the main viewport → `ImGui::UpdatePlatformWindows()` → `ImGui::RenderPlatformWindowsDefault()` → `vkQueuePresentKHR` for the main viewport
- **AND** the host SHALL guard the `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` calls behind a check on `ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable` so the calls are no-ops on iOS even if the code is compiled there

#### Scenario: Secondary viewport swapchains are managed by ImGui, not by the host
- **WHEN** a secondary viewport is created, resized, or destroyed
- **THEN** the per-viewport `ImGui_ImplVulkanH_Window` (containing its own swapchain + image views + render pass + framebuffers + per-frame fences) SHALL be created / resized / destroyed by ImGui's `ImGui_ImplVulkanH_CreateOrResizeWindow` / `ImGui_ImplVulkanH_DestroyWindow` helpers via the renderer callbacks
- **AND** the host SHALL NOT call `desktop::createSwapchain` / `desktop::destroySwapchain` for any secondary viewport (those helpers are reserved for the main viewport only)
- **AND** the host's existing `desktop::Swapchain`, `desktop::FrameResources`, and `desktop::ResizeFrameResources` (any subset of which exist post-`add-gui-theme`) SHALL be used only for the main viewport

#### Scenario: Live-resize callback does not fire for secondary viewports
- **WHEN** a user resizes a secondary OS-level viewport window
- **THEN** the resize SHALL be handled by ImGui's `Renderer_SetWindowSize` callback (which routes through `ImGui_ImplVulkanH_CreateOrResizeWindow`)
- **AND** the host's `glfwSetFramebufferSizeCallback` / `glfwSetWindowRefreshCallback` (registered on the main `GLFWwindow*` only) SHALL NOT fire for secondary viewports

#### Scenario: Shutdown teardown of secondary viewports is delegated to ImGui's *_Shutdown calls
- **WHEN** the desktop host shuts down with one or more secondary viewports still alive
- **THEN** `ImGui_ImplVulkan_Shutdown()` SHALL walk the viewport list and call `ImGui_ImplVulkan_DestroyWindow` for each secondary viewport before clearing `ImGui::GetIO().BackendRendererUserData`
- **AND** `ImGui_ImplGlfw_Shutdown()` SHALL likewise destroy each secondary GLFW window before clearing `BackendPlatformUserData`
- **AND** the host SHALL NOT add explicit per-viewport teardown code; the shutdown order from `render-backend` "Correct shutdown order" (vkDeviceWaitIdle → ImGui_ImplVulkan_Shutdown → ImGui_ImplGlfw_Shutdown → app::shutdown → Vulkan teardown) SHALL be unchanged
- **AND** the defensive assertions in `app::shutdown` (`io.BackendRendererUserData == nullptr` and `BackendPlatformUserData == nullptr`) SHALL pass after the ImGui shutdowns run, regardless of viewport count

#### Scenario: Image count consistency between main and secondary swapchains
- **WHEN** `ImGui_ImplVulkan_Init` is called
- **THEN** the host SHALL populate `ImGui_ImplVulkan_InitInfo.MinImageCount` and `ImageCount` from the main swapchain
- **AND** secondary viewports created subsequently via `ImGui_ImplVulkanH_CreateOrResizeWindow` SHALL inherit those counts from `InitInfo`, so the main viewport and secondary viewports run with consistent frames-in-flight semantics

#### Scenario: Preferences dialog uses multi-viewport on desktop
- **WHEN** user opens the Preferences dialog via `Help > Preferences...` menu on desktop platforms
- **THEN** the dialog SHALL open as a secondary OS-level window using the multi-viewport capability
- **AND** the dialog SHALL be movable and resizable independently from the main window

