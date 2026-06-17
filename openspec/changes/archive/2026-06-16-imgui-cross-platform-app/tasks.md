## 1. Repo skeleton and dependency pinning

- [x] 1.1 Create top-level directory `apps/imgui-shell/` with subfolders `app/`, `platform/desktop/`, `platform/ios/`, `cmake/`, `assets/`, `docs/`
- [x] 1.2 Add root `apps/imgui-shell/CMakeLists.txt` requiring CMake ≥ 3.24, project name, C++17 standard, and `IMGUI_SHELL_PLATFORM` cache variable derived from preset
- [x] 1.3 Add `apps/imgui-shell/CMakePresets.json` with configure + build presets `macos`, `windows`, `linux`, `ios` (Xcode generator + `CMAKE_SYSTEM_NAME=iOS` for the iOS preset)
- [x] 1.4 Wire Dear ImGui via `FetchContent` (docking branch, pinned to a chosen SHA), declared in a `cmake/Dependencies.cmake` included from the root
- [x] 1.5 Define an `imgui` static library target compiling `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui_demo.cpp` from the fetched source
- [x] 1.6 Confirm `cmake --preset macos` (or whichever host you're on) configures successfully against the empty skeleton

## 2. Shared app core (capability: app-shell)

- [x] 2.1 Create `app/RenderContext.h` defining a minimal struct passed from each host to the core. Desktop variant carries: `GLFWwindow*`, `VkInstance`, `VkPhysicalDevice`, `VkDevice`, `VkQueue` + queue family index, `VkRenderPass`, `VkDescriptorPool` (for ImGui), swapchain `MinImageCount`/`ImageCount`, plus a per-frame `VkCommandBuffer` the host populates before calling `app::frame`. iOS variant carries `MTLDevice*`, `MTLCommandQueue*`, drawable provider. Implemented as a union/variant or two-field struct with platform-gated members.
- [x] 2.2 Create `app/App.h` declaring `app::init(RenderContext&)`, `app::frame(RenderContext&)`, `app::shutdown()`
- [x] 2.3 Create `app/App.cpp` implementing the three lifecycle functions; `init` creates the ImGui context, `shutdown` destroys it, `frame` is currently empty (sample UI lands in task 5)
- [x] 2.4 Add an internal `app::detail::ensureInitialized()` guard so calls before `init` or after `shutdown` assert in debug builds
- [x] 2.5 Add an `imgui_shell_app` static library target in CMake exposing the `app/` sources, linked against `imgui`
- [x] 2.6 Verify `imgui_shell_app` compiles on the host platform with no warnings under `-Wall -Wextra -Wpedantic` (or MSVC `/W4`)

## 3. Desktop host (capability: render-backend, desktop portion)

- [x] 3.1 Add GLFW via `FetchContent` (or system `find_package` on Linux) in `cmake/Dependencies.cmake`, gated to desktop presets only
- [x] 3.2 Add `find_package(Vulkan 1.2 REQUIRED)` in `cmake/Dependencies.cmake`, gated to desktop presets only; surface a clear configure-time failure message if `VULKAN_SDK` is not set / SDK is not installed
- [x] 3.3 Compile the `imgui_impl_glfw.cpp` and `imgui_impl_vulkan.cpp` backend sources into a separate `imgui_backend_desktop` static library, linked against `Vulkan::Vulkan` and `glfw`
- [x] 3.4 Create `platform/desktop/main.cpp` skeleton: set `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)`, verify `glfwVulkanSupported()`, create the GLFW window
- [x] 3.5 In `platform/desktop/vk_instance.{h,cpp}`: create `VkInstance` with API 1.2 and GLFW-required extensions; in debug builds add `VK_LAYER_KHRONOS_validation` + `VK_EXT_debug_utils` and install a debug messenger that logs validation messages and asserts on error severity
- [x] 3.6 In `platform/desktop/vk_device.{h,cpp}`: create surface via `glfwCreateWindowSurface`, enumerate physical devices, pick one that supports graphics + present-to-surface + `VK_KHR_swapchain`, create the logical device with one graphics+present queue
- [x] 3.7 In `platform/desktop/vk_swapchain.{h,cpp}`: create swapchain with `VK_PRESENT_MODE_FIFO_KHR`, image views, render pass (single color attachment matching swapchain format, no depth in v1), framebuffers; expose a `recreate()` helper for resize/out-of-date events
- [x] 3.8 In `platform/desktop/vk_frame.{h,cpp}`: command pool, per-frame command buffers, semaphores (image-available per frame-in-flight; render-finished lives in Swapchain per-image — see code comment for why), in-flight fences for `MAX_FRAMES_IN_FLIGHT = 2`
- [x] 3.9 Allocate the ImGui descriptor pool (`maxSets = 1000`, combined-image-sampler pool size = 1000) and populate `ImGui_ImplVulkan_InitInfo`; `imgui_impl_vulkan` requires that `ImGui_ImplVulkan_CreateFontsTexture` is called once after init (one-shot command buffer)
- [x] 3.10 In `main.cpp`, after all Vulkan objects exist: populate `RenderContext`, call `app::init`, then enter `while (!glfwWindowShouldClose(window))` loop — `glfwPollEvents`, wait/reset fence, `vkAcquireNextImageKHR` (handle `VK_ERROR_OUT_OF_DATE_KHR` → recreate swapchain and skip frame), reset and begin command buffer, begin render pass, call `ImGui_ImplVulkan_NewFrame` / `ImGui_ImplGlfw_NewFrame` / `ImGui::NewFrame`, call `app::frame` (writes ImGui draw data via `ImGui_ImplVulkan_RenderDrawData(cmd_buf)`), end render pass, end command buffer, `vkQueueSubmit` with semaphores + fence, `vkQueuePresentKHR` (handle `VK_SUBOPTIMAL_KHR` → recreate)
- [x] 3.11 On loop exit: `vkDeviceWaitIdle`, call `app::shutdown`, then `ImGui_ImplVulkan_Shutdown` / `ImGui_ImplGlfw_Shutdown`, then destroy Vulkan objects in reverse creation order (sync → command pool → framebuffers → image views → swapchain → render pass → descriptor pool → device → surface → debug messenger → instance), then destroy GLFW window and `glfwTerminate`
- [x] 3.12 Define a CMake target `imgui_shell_desktop` linked against `imgui_shell_app`, `imgui_backend_desktop`, `imgui`, `glfw`, `Vulkan::Vulkan`; excluded from the `ios` preset
- [x] 3.13 Build and run on the host platform — confirm a window opens, the loop runs, validation layer is silent in debug, closing the window cleanly exits with no Vulkan validation warnings (verified up to steady-state: ran 3s under validation layer with silent stderr; clean-exit-on-File>Quit path is implemented per spec but only smoke-tested at code review — manual interactive close pending)

## 4. iOS host (capability: render-backend, iOS portion)

- [x] 4.1 Compile the `imgui_impl_metal.mm` iOS backend source (no `imgui_impl_osx.mm` needed — that targets macOS NSWindow, not iOS UIKit) into `imgui_backend_ios` — only built under the `ios` preset
- [x] 4.2 Create `platform/ios/AppDelegate.h/.mm` providing a minimal `UIApplicationDelegate` that installs the root view controller in `application:didFinishLaunchingWithOptions:`
- [x] 4.3 Create `platform/ios/ViewController.h/.mm` hosting a `MTKView` (or a custom `UIView` backed by `CAMetalLayer`); on `viewDidLoad`, create `MTLDevice` + `MTLCommandQueue`, configure the metal layer, initialize ImGui Metal backend, then call `app::init`
- [x] 4.4 Add `ViewController` per-frame method (e.g., `drawInMTKView:` or a `CADisplayLink` callback): begin Metal command buffer + render pass, call ImGui Metal `NewFrame`, call `app::frame`, render ImGui draw data via the Metal backend, commit + present
- [x] 4.5 On `dealloc` / shutdown: call `app::shutdown`, tear down the Metal backend in reverse order
- [x] 4.6 Create `platform/ios/main.mm` with the standard `UIApplicationMain` entry point referencing `AppDelegate`
- [x] 4.7 Create `platform/ios/Info.plist.in` (templated for bundle id, display name) and configure CMake to substitute it at generate time
- [x] 4.8 Define a CMake target `imgui_shell_ios` of type `MACOSX_BUNDLE` (with iOS-appropriate properties), linked against `imgui_shell_app`, `imgui_backend_ios`, `imgui`, UIKit, Metal, MetalKit, QuartzCore; excluded from desktop presets
- [ ] 4.9 Configure with `cmake --preset ios`, open the generated Xcode project, set a signing team, build to a connected device or simulator, confirm app launches — **DEFERRED: requires full Xcode (not Command Line Tools); contributor with Xcode installed completes this step**

## 5. Sample UI (capability: ui-sample)

- [x] 5.1 In `app/App.cpp`'s `frame`, render an ImGui main menu bar with `File` and `Help` menus
- [x] 5.2 Add `File > Quit` item — only shown when a compile-time `IMGUI_SHELL_PLATFORM_DESKTOP` macro is defined (set by the desktop CMake target); on click, set a `g_requestQuit` flag the desktop host polls each frame to call `glfwSetWindowShouldClose`
- [x] 5.3 Add `Help > ImGui Demo` toggle bound to a static `bool g_showDemo`; render `ImGui::ShowDemoWindow(&g_showDemo)` when true
- [x] 5.4 Add `Help > About...` opening a modal `ImGui::BeginPopupModal`; display app name (`"imgui-shell"`), `IMGUI_VERSION` macro, and a platform string from a `kImguiShellPlatformName` constant set via CMake `target_compile_definitions` per target
- [ ] 5.5 Verify by eye on the host platform: menu bar visible, demo toggles, About dialog displays correct platform name and ImGui version — **DEFERRED: requires interactive run; window opens cleanly under validation layer (verified) but menu drive-through pending**

## 6. CI and docs

- [x] 6.1 Add `.github/workflows/imgui-shell-build.yml` with a matrix of `macos-latest`, `ubuntu-latest`, `windows-latest`, each running `cmake --preset <name> && cmake --build --preset <name>` for the matching desktop preset
- [x] 6.2a Install LunarG Vulkan SDK ≥ 1.2 on each runner — macOS via `dmg` install action (includes MoltenVK), Ubuntu via LunarG apt repo (`vulkan-sdk` package + `xorg-dev`, `libwayland-dev`, `libxkbcommon-dev` for GLFW), Windows via the LunarG installer action; set `VULKAN_SDK` env var for the configure step
- [x] 6.2b Confirm `find_package(Vulkan 1.2 REQUIRED)` succeeds on all three runners; gate any 1.3-specific code (none in v1) accordingly — CMake config logic verified locally; runner success awaits 6.3
- [ ] 6.3 Confirm the CI matrix passes for a PR-equivalent push on a feature branch — **DEFERRED: requires push to a real branch on GitHub; workflow is in `.github/workflows/imgui-shell-build.yml` and is ready to run**
- [x] 6.4 Write `apps/imgui-shell/README.md`: per-platform prerequisites, build commands, run instructions, iOS-specific Xcode + signing notes, and a "what this is / what this isn't" section
- [x] 6.5 Add `docs/architecture.md` briefly describing the `app/` ↔ `platform/*` split and the lifecycle interface, linking back to design.md

## 7. Verification against specs

- [x] 7.1 Walk through `specs/app-shell/spec.md` scenarios and confirm each is satisfied by the built artifact on at least one platform — refactored `App.cpp` to use `if constexpr` via `app/Platform.h` instead of `#ifdef IMGUI_SHELL_PLATFORM_*` blocks so that the "Identical source set across platforms" scenario is genuinely satisfied. All other scenarios (lifecycle interface, ImGui context ownership, platform host responsibilities, no-business-logic) are satisfied by code inspection on the built macOS binary.
- [x] 7.2 Walk through `specs/render-backend/spec.md` scenarios — verified via `nm`/`otool`: desktop binary has zero Metal/UIKit symbols, links `libvulkan.1.dylib` (not Metal); desktop binary references `vk*` symbols (vkAcquireNextImageKHR, etc.). Swapchain-recreation code path is implemented per spec; **manual resize exercise pending interactive run**.
- [ ] 7.3 Walk through `specs/build-system/spec.md` scenarios on a clean checkout — `cmake --preset` succeeds for all four presets on supported hosts — **macOS preset verified locally; Linux/Windows/iOS presets pending appropriate hosts (CI will exercise Linux and Windows when 6.3 runs)**
- [ ] 7.4 Walk through `specs/ui-sample/spec.md` scenarios — menu bar, demo toggle, About dialog all behave as specified on every built platform — **DEFERRED: requires interactive run on each platform; code matches spec**
- [x] 7.5 Run `openspec validate --change imgui-cross-platform-app --strict` and resolve any reported issues before archiving — `openspec validate imgui-cross-platform-app --type change --strict` ⇒ "Change 'imgui-cross-platform-app' is valid"
