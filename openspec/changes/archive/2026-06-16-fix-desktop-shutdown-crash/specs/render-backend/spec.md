## MODIFIED Requirements

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
