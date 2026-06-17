## MODIFIED Requirements

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
