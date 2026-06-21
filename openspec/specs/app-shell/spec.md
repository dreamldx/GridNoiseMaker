# app-shell Specification

## Purpose
TBD - created by archiving change imgui-cross-platform-app. Update Purpose after archive.
## Requirements
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

#### Scenario: No double-init across hot reload or platform handoff
- **WHEN** `app::shutdown` has been called
- **THEN** any subsequent `app::frame` call SHALL be a no-op or assertion failure (defined behavior, not undefined)

### Requirement: Platform host responsibilities
Each platform host (desktop, iOS) SHALL own the platform-specific concerns and ONLY those concerns: window/view creation, event pump, render context setup, supplying the iOS-only `documentsPath` for per-user config storage, and driving the lifecycle calls into the shared core. Platform hosts MUST NOT contain UI logic.

#### Scenario: Desktop host drives the main loop
- **WHEN** the desktop target runs
- **THEN** a single `main()` entry point in `platform/desktop/` SHALL create the GLFW window (with `GLFW_CLIENT_API = GLFW_NO_API`), bring up the Vulkan instance / surface / device / swapchain / render pass / framebuffers / sync primitives, call `app::init` once the render context is fully populated, run a frame loop calling `app::frame` (acquire → record → submit → present) until the window is closed, then call `app::shutdown` followed by `vkDeviceWaitIdle` and reverse-order Vulkan teardown

#### Scenario: iOS host drives via CADisplayLink
- **WHEN** the iOS target runs
- **THEN** the iOS host SHALL host ImGui inside a `UIViewController` and drive `app::frame` from a `CADisplayLink` callback, with `app::init` called in `viewDidLoad` (or equivalent) after the Metal layer is ready, and `app::shutdown` called when the view controller is torn down

#### Scenario: iOS host supplies the documents path before init
- **WHEN** the iOS target starts (in `ViewController::viewDidLoad` or equivalent, before `app::init` is invoked)
- **THEN** the iOS host SHALL call `app::setDocumentsPath` with the result of `NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0]` (or equivalent UTF-8 path string) so the shared `app/` code can resolve the theme config path inside the iOS Documents directory
- **AND** desktop hosts SHALL NOT call `app::setDocumentsPath` (it is iOS-only; on desktop the theme path resolves via XDG_CONFIG_HOME / APPDATA env vars instead)

### Requirement: No business logic in v1
This change delivers the shell only. The shared application core SHALL NOT include feature-specific business logic beyond what the ui-sample capability requires.

#### Scenario: Shell-only deliverable
- **WHEN** v1 of this change is archived
.
**THEN** the only UI present in `app/` is the sample UI defined by the ui-sample capability

### Requirement: File menu includes node graph save/load operations
The application shell SHALL provide save and load operations for node graphs in the File menu, allowing users to persist and restore node graph state.

#### Scenario: Save Node Graph menu item
- **WHEN** the File menu is open
- **THEN** it SHALL include a "Save Node Graph" menu item
- **AND** selecting this item SHALL open a file save dialog
- **AND** SHALL save the current node graph state to the selected JSON file
- **AND** SHALL handle file operation errors gracefully with user feedback

#### Scenario: Load Node Graph menu item
- **WHEN** the File menu is open
- **THEN** it SHALL include a "Load Node Graph" menu item
- **AND** selecting this item SHALL open a file open dialog
- **AND** SHALL load node graph state from the selected JSON file
- **AND** SHALL replace the current node graph with the loaded state
- **AND** SHALL handle file format errors gracefully with user feedback

#### Scenario: Menu integration with node graph widget
- **WHEN** save/load menu items are implemented
- **THEN** they SHALL call appropriate methods on the node graph widget
- **AND** SHALL pass file paths to the widget for serialization/deserialization
- **AND** SHALL not interfere with existing File menu functionality

