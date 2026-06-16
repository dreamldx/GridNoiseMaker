## ADDED Requirements

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
Each platform host (desktop, iOS) SHALL own the platform-specific concerns and ONLY those concerns: window/view creation, event pump, render context setup, and driving the lifecycle calls into the shared core. Platform hosts MUST NOT contain UI logic.

#### Scenario: Desktop host drives the main loop
- **WHEN** the desktop target runs
- **THEN** a single `main()` entry point in `platform/desktop/` SHALL create the GLFW window, initialize the GL context, call `app::init`, run a frame loop calling `app::frame` until the window is closed, then call `app::shutdown`

#### Scenario: iOS host drives via CADisplayLink
- **WHEN** the iOS target runs
- **THEN** the iOS host SHALL host ImGui inside a `UIViewController` and drive `app::frame` from a `CADisplayLink` callback, with `app::init` called in `viewDidLoad` (or equivalent) after the Metal layer is ready, and `app::shutdown` called when the view controller is torn down

### Requirement: No business logic in v1
This change delivers the shell only. The shared application core SHALL NOT include feature-specific business logic beyond what the ui-sample capability requires.

#### Scenario: Shell-only deliverable
- **WHEN** v1 of this change is archived
- **THEN** the only UI present in `app/` is the sample UI defined by the ui-sample capability
