## MODIFIED Requirements

### Requirement: Shared application core
The system SHALL provide a shared application core, compilable as C++17, that contains all UI code and exposes a lifecycle interface (`init`, `frame`, `shutdown`) consumed by every platform host. The same `app/` source files MUST compile unchanged on macOS, Windows, Linux, and iOS.

#### Scenario: Identical source set across platforms
- **WHEN** the project is built for any supported target (macOS, Windows, Linux, iOS)
- **THEN** the files under `app/` are compiled by the desktop and iOS targets without per-platform `#ifdef` blocks inside `app/` source files

#### Scenario: Lifecycle interface
- **WHEN** a platform host initializes the application
- **THEN** the host SHALL call `app::init(RenderContext&)` exactly once after the rendering backend is ready
- **AND** the host SHALL call `app::frame(RenderContext&)` once per frame before swapping buffers / presenting the drawable
- **AND** the host SHALL call `app::shutdown()` exactly once before tearing down the rendering backend

### Requirement: ImGui context ownership
The shared application core SHALL own the Dear ImGui context: creating it during `init`, building per-frame UI inside `frame`, and destroying it during `shutdown`. Platform hosts MUST NOT create or destroy the ImGui context themselves.

#### Scenario: Single context lifecycle
- **WHEN** the application starts
- **THEN** exactly one `ImGui::CreateContext()` call SHALL occur, inside `app::init`
- **AND** the matching `ImGui::DestroyContext()` SHALL occur inside `app::shutdown`
- **AND** the ImGui configuration SHALL enable docking and multiple viewport support for dockable UI components

#### Scenario: No double-init across hot reload or platform handoff
- **WHEN** `app::shutdown` has been called
- **THEN** any subsequent `app::frame` call SHALL be a no-op or assertion failure (defined behavior, not undefined)

### Requirement: Platform separation
Each platform host (desktop, iOS) SHALL own the platform-specific concerns and ONLY those concerns: window/view creation, event pump, render context setup, supplying the iOS-only `documentsPath` for per-user config storage, and driving the lifecycle calls into the shared core. Platform hosts MUST NOT contain UI logic.

#### Scenario: Desktop platform host responsibilities
- **WHEN** building for a desktop platform (macOS, Windows, Linux)
- **THEN** a single `main()` entry point in `platform/desktop/` SHALL create the GLFW window (with `GLFW_CLIENT_API = GLFW_NO_API`), bring up the Vulkan instance / surface / device / swapchain / render pass / framebuffers / sync primitives, call `app::init` once the render context is fully populated, run a frame loop calling `app::frame` (acquire → record → submit → present) until the window is closed, then call `app::shutdown` followed by `vkDeviceWaitIdle` and reverse-order Vulkan teardown

#### Scenario: iOS platform host responsibilities
- **WHEN** building for iOS
- **THEN** the `platform/ios/` host SHALL create an `MTKView` (MetalKit view) inside a `UIViewController`, implement `MTKViewDelegate` callbacks (`drawInMTKView:` for per-frame calls, `mtkView:drawableSizeWillChange:` for resize), call `app::init` after the Metal device and command queue are ready, and forward all UIApplication lifecycle events (including entering background) to the shared core via a future extension point

#### Scenario: Application state persistence
- **WHEN** the application runs on desktop
- **THEN** user preferences (including panel layout, docking state, and UI configuration) SHALL be stored in a platform-appropriate location
- **AND** SHALL be restored on subsequent application launches