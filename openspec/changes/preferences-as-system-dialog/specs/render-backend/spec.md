## MODIFIED Requirements

### Requirement: Multi-viewport support on desktop
The desktop host SHALL enable ImGui's multi-viewport mode (`ImGuiConfigFlags_ViewportsEnable`) and SHALL wire the per-viewport platform + renderer callbacks so any ImGui window the user drags outside the main host becomes a real OS-level secondary window with its own GLFW window + Vulkan swapchain. The implementation SHALL re-use ImGui's bundled helpers (`ImGui_ImplVulkanH_*`, the default `Renderer_*` callbacks installed by `ImGui_ImplVulkan_Init`, and the GLFW multi-viewport hooks installed by `ImGui_ImplGlfw_InitForVulkan(window, true)`) rather than hand-rolling secondary swapchains. The iOS target SHALL NOT enable this flag �� iOS is single-window.

#### Scenario: ViewportsEnable flag is set on desktop only
- **WHEN** `app::init` runs on a desktop target
- **THEN** it SHALL set `ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable`
1 **WHEN** `app::init` runs on the iOS target
- **THEN** it SHALL NOT set the `ImGuiConfigFlags_ViewportsEnable` flag
- **AND** the gate SHALL be expressed via `if constexpr (app::kIsDesktop)` (no `#ifdef IMGUI_SHELL_PLATFORM_*` blocks in `app/` source files)

#### Scenario: ImGui default per-viewport callbacks are installed by standard init
- **WHEN** the desktop host calls `ImGui_ImplGlfw_InitForVulkan(window, /*install_callbacks=*/true)` and `ImGui_ImplVulkan_Init(&info)`
- **THEN** ImGui's default per-viewport callbacks (`Platform_CreateWindow`, `Platform_DestroyWindow`, `Platform_ShowWindow`, `Platform_SetWindowPos/Size/Title/Alpha`, `Renderer_CreateWindow`, `Renderer_DestroyWindow`, `Renderer_SetWindowSize`, `Renderer_RenderWindow`, `Renderer_SwapBuffers`) SHALL be registered on `ImGui::GetPlatformIO()` automatically by those init calls

#### Scenario: Multi-viewport submit + present happens after main viewport, before main present
– **WHEN** the desktop host's main loop renders a frame
- **THEN** the order of operations SHALL be: main-viewport render-pass + `ImGui_ImplVulkan_RenderDrawData` for the main draw data + end command buffer + `vkQueueSubmit` for the main viewport �� `ImGui::UpdatePlatformWindows()` �� `ImGui::RenderPlatformWindowsDefault()` �� `vkQueuePresentKHR` for the main viewport
- **AND** the host SHALL guard the `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` calls behind a check on `ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable` so the calls are no-ops on iOS even if the code is compiled there

#### Scenario: Secondary viewport swapchains are managed by ImGui, not by the host
- **WHEN** a secondary viewport is created, resized, or destroyed
- **THEN** the per-viewport `ImGui_ImplVulkanH_Window` (containing its own swapchain + image views + render pass + framebuffers + per-frame fences) SHALL be created / resized / destroyed by ImGui's `ImGui_ImplVulkanH_CreateOrResizeWindow` / `ImGui_ImplVulkanH_DestroyWindow` helpers via the renderer callbacks
- **AND** the host SHALL NOT call `desktop::createSwapchain` / `desktop::destroySwapchain` for any secondary viewport (those helpers are reserved for the main viewport only)
- **AND** the host's existing `desktop::Swapchain`, `desktop::FrameResources`, and `desktop::ResizeFrameResources` (any subset of which exist post-`add-gui-theme`) SHALL be used only for the main viewport

#### Scenario: Preferences dialog optionally uses native dialog instead of multi-viewport
- **WHEN** user opens the Preferences dialog via `Help > Preferences...` menu on desktop platforms
- **THEN** the dialog MAY open as either:
  - A secondary OS-level window using the multi-viewport capability (existing behavior)
  - A native system dialog using platform-native APIs (new preferred behavior)
– **AND** multi-viewport capability SHALL remain available for other ImGui windows that may be dragged outside the main host