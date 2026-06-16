## ADDED Requirements

### Requirement: Desktop backend stack
The desktop targets (macOS, Windows, Linux) SHALL use GLFW for windowing and input and OpenGL 3.3 core profile for rendering, with Dear ImGui's `imgui_impl_glfw` and `imgui_impl_opengl3` backends.

#### Scenario: Backend selection at build time
- **WHEN** the project is configured for the `macos`, `windows`, or `linux` preset
- **THEN** GLFW SHALL be linked
- **AND** `imgui_impl_glfw.cpp` and `imgui_impl_opengl3.cpp` SHALL be compiled into the binary
- **AND** the iOS-specific Metal backend SHALL NOT be compiled

#### Scenario: GL context creation
- **WHEN** the desktop host starts
- **THEN** it SHALL request an OpenGL 3.3 core profile context with forward-compatibility flag set on macOS
- **AND** it SHALL fail to start with a clear error message if the requested context cannot be created

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
The project SHALL NOT introduce a runtime rendering-backend selector (no virtual `IRenderer` interface, no runtime switch between GL and Metal). Backend selection is a build-time decision driven by the CMake preset.

#### Scenario: No runtime branching on backend
- **WHEN** the codebase is grepped for runtime backend dispatch
- **THEN** there SHALL be no abstract renderer interface with multiple concrete implementations selected at runtime
- **AND** backend-specific code SHALL be excluded from compilation by the build system, not guarded by runtime checks

### Requirement: ImGui backend initialization order
The platform host SHALL initialize ImGui backends in the order required by Dear ImGui: create ImGui context first (done by `app::init` per app-shell), then init the platform backend (`ImGui_ImplGlfw_InitForOpenGL` / `ImGui_ImplMetal_Init`), then init the renderer backend (`ImGui_ImplOpenGL3_Init` / per-frame Metal pipeline setup).

#### Scenario: Correct shutdown order
- **WHEN** the application shuts down
- **THEN** the host SHALL shut down ImGui backends in reverse order: renderer backend, platform backend, then ImGui context destruction (the last performed by `app::shutdown`)
