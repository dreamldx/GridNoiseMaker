## ADDED Requirements

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
