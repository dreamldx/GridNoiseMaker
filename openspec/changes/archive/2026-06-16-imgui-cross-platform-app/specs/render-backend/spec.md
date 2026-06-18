## ADDED Requirements

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
The platform host SHALL initialize ImGui backends in the order required by Dear ImGui: create ImGui context first (done by `app::init` per app-shell), then init the platform backend (`ImGui_ImplGlfw_InitForVulkan` / `ImGui_ImplMetal_Init`), then init the renderer backend (`ImGui_ImplVulkan_Init` with a populated `ImGui_ImplVulkan_InitInfo` / per-frame Metal pipeline setup).

#### Scenario: ImGui descriptor pool allocation
- **WHEN** the desktop host initializes ImGui's Vulkan renderer backend
- **THEN** it SHALL pre-create a `VkDescriptorPool` sized for ImGui's font texture and per-frame image bindings (`maxSets >= 1000`, combined-image-sampler pool of the same size is sufficient per ImGui example)
- **AND** it SHALL pass that pool, the physical device, logical device, queue, queue family, render pass, and `MinImageCount`/`ImageCount` from the swapchain into `ImGui_ImplVulkan_InitInfo`

#### Scenario: Correct shutdown order
- **WHEN** the application shuts down
- **THEN** the host SHALL call `vkDeviceWaitIdle` before any teardown
- **AND** the host SHALL shut down ImGui backends in reverse order: renderer backend (`ImGui_ImplVulkan_Shutdown`), platform backend (`ImGui_ImplGlfw_Shutdown`), then ImGui context destruction (the last performed by `app::shutdown`)
- **AND** the host SHALL destroy Vulkan objects in reverse creation order (sync primitives → command pool → framebuffers → image views → swapchain → render pass → descriptor pool → device → surface → instance)
