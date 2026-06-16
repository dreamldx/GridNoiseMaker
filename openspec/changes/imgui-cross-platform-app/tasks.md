## 1. Repo skeleton and dependency pinning

- [ ] 1.1 Create top-level directory `apps/imgui-shell/` with subfolders `app/`, `platform/desktop/`, `platform/ios/`, `cmake/`, `assets/`, `docs/`
- [ ] 1.2 Add root `apps/imgui-shell/CMakeLists.txt` requiring CMake ≥ 3.24, project name, C++17 standard, and `IMGUI_SHELL_PLATFORM` cache variable derived from preset
- [ ] 1.3 Add `apps/imgui-shell/CMakePresets.json` with configure + build presets `macos`, `windows`, `linux`, `ios` (Xcode generator + `CMAKE_SYSTEM_NAME=iOS` for the iOS preset)
- [ ] 1.4 Wire Dear ImGui via `FetchContent` (docking branch, pinned to a chosen SHA), declared in a `cmake/Dependencies.cmake` included from the root
- [ ] 1.5 Define an `imgui` static library target compiling `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui_demo.cpp` from the fetched source
- [ ] 1.6 Confirm `cmake --preset macos` (or whichever host you're on) configures successfully against the empty skeleton

## 2. Shared app core (capability: app-shell)

- [ ] 2.1 Create `app/RenderContext.h` defining a minimal struct passed from each host to the core (e.g., for desktop: GLFW window pointer + GL loader info; for iOS: Metal device + command queue + drawable provider) — implemented as a union/variant or two-field struct with platform-gated members
- [ ] 2.2 Create `app/App.h` declaring `app::init(RenderContext&)`, `app::frame(RenderContext&)`, `app::shutdown()`
- [ ] 2.3 Create `app/App.cpp` implementing the three lifecycle functions; `init` creates the ImGui context, `shutdown` destroys it, `frame` is currently empty (sample UI lands in task 5)
- [ ] 2.4 Add an internal `app::detail::ensureInitialized()` guard so calls before `init` or after `shutdown` assert in debug builds
- [ ] 2.5 Add an `imgui_shell_app` static library target in CMake exposing the `app/` sources, linked against `imgui`
- [ ] 2.6 Verify `imgui_shell_app` compiles on the host platform with no warnings under `-Wall -Wextra -Wpedantic` (or MSVC `/W4`)

## 3. Desktop host (capability: render-backend, desktop portion)

- [ ] 3.1 Add GLFW via `FetchContent` (or system find_package on Linux) in `cmake/Dependencies.cmake`, gated to desktop presets only
- [ ] 3.2 Compile the `imgui_impl_glfw.cpp` and `imgui_impl_opengl3.cpp` backend sources into a separate `imgui_backend_desktop` static library
- [ ] 3.3 Create `platform/desktop/main.cpp`: initialize GLFW with GL 3.3 core (+ `GLFW_OPENGL_FORWARD_COMPAT` on macOS), create window, create GL context, load GL via the bundled `imgui_impl_opengl3_loader.h` (no extra loader dep)
- [ ] 3.4 In `main.cpp`, after context creation: call `app::init`, then enter `while (!glfwWindowShouldClose(window))` loop — poll events, call `ImGui_ImplOpenGL3_NewFrame` / `ImGui_ImplGlfw_NewFrame` / `ImGui::NewFrame`, call `app::frame`, render ImGui draw data, swap buffers
- [ ] 3.5 On loop exit: call `app::shutdown`, then shut down the renderer backend, the platform backend, and destroy the window/terminate GLFW (correct reverse order per spec)
- [ ] 3.6 Define a CMake target `imgui_shell_desktop` linked against `imgui_shell_app`, `imgui_backend_desktop`, `imgui`, and platform GL libs; excluded from the `ios` preset
- [ ] 3.7 Build and run on the host platform — confirm a window opens, the loop runs, closing the window cleanly exits with no leak warnings

## 4. iOS host (capability: render-backend, iOS portion)

- [ ] 4.1 Compile the `imgui_impl_metal.mm` and `imgui_impl_osx.mm`-equivalent iOS backend sources (use the iOS-flavored Metal example from `examples/example_apple_metal/` as the reference) into `imgui_backend_ios` — only built under the `ios` preset
- [ ] 4.2 Create `platform/ios/AppDelegate.h/.mm` providing a minimal `UIApplicationDelegate` that installs the root view controller in `application:didFinishLaunchingWithOptions:`
- [ ] 4.3 Create `platform/ios/ViewController.h/.mm` hosting a `MTKView` (or a custom `UIView` backed by `CAMetalLayer`); on `viewDidLoad`, create `MTLDevice` + `MTLCommandQueue`, configure the metal layer, initialize ImGui Metal backend, then call `app::init`
- [ ] 4.4 Add `ViewController` per-frame method (e.g., `drawInMTKView:` or a `CADisplayLink` callback): begin Metal command buffer + render pass, call ImGui Metal `NewFrame`, call `app::frame`, render ImGui draw data via the Metal backend, commit + present
- [ ] 4.5 On `dealloc` / shutdown: call `app::shutdown`, tear down the Metal backend in reverse order
- [ ] 4.6 Create `platform/ios/main.mm` with the standard `UIApplicationMain` entry point referencing `AppDelegate`
- [ ] 4.7 Create `platform/ios/Info.plist.in` (templated for bundle id, display name) and configure CMake to substitute it at generate time
- [ ] 4.8 Define a CMake target `imgui_shell_ios` of type `MACOSX_BUNDLE` (with iOS-appropriate properties), linked against `imgui_shell_app`, `imgui_backend_ios`, `imgui`, UIKit, Metal, MetalKit, QuartzCore; excluded from desktop presets
- [ ] 4.9 Configure with `cmake --preset ios`, open the generated Xcode project, set a signing team, build to a connected device or simulator, confirm app launches

## 5. Sample UI (capability: ui-sample)

- [ ] 5.1 In `app/App.cpp`'s `frame`, render an ImGui main menu bar with `File` and `Help` menus
- [ ] 5.2 Add `File > Quit` item — only shown when a compile-time `IMGUI_SHELL_PLATFORM_DESKTOP` macro is defined (set by the desktop CMake target); on click, set a `g_requestQuit` flag the desktop host polls each frame to call `glfwSetWindowShouldClose`
- [ ] 5.3 Add `Help > ImGui Demo` toggle bound to a static `bool g_showDemo`; render `ImGui::ShowDemoWindow(&g_showDemo)` when true
- [ ] 5.4 Add `Help > About...` opening a modal `ImGui::BeginPopupModal`; display app name (`"imgui-shell"`), `IMGUI_VERSION` macro, and a platform string from a `kImguiShellPlatformName` constant set via CMake `target_compile_definitions` per target
- [ ] 5.5 Verify by eye on the host platform: menu bar visible, demo toggles, About dialog displays correct platform name and ImGui version

## 6. CI and docs

- [ ] 6.1 Add `.github/workflows/imgui-shell-build.yml` with a matrix of `macos-latest`, `ubuntu-latest`, `windows-latest`, each running `cmake --preset <name> && cmake --build --preset <name>` for the matching desktop preset
- [ ] 6.2 Install GLFW build prerequisites in the Ubuntu job (X11/Wayland dev packages: `xorg-dev`, `libwayland-dev`, `libxkbcommon-dev`)
- [ ] 6.3 Confirm the CI matrix passes for a PR-equivalent push on a feature branch
- [ ] 6.4 Write `apps/imgui-shell/README.md`: per-platform prerequisites, build commands, run instructions, iOS-specific Xcode + signing notes, and a "what this is / what this isn't" section
- [ ] 6.5 Add `docs/architecture.md` briefly describing the `app/` ↔ `platform/*` split and the lifecycle interface, linking back to design.md

## 7. Verification against specs

- [ ] 7.1 Walk through `specs/app-shell/spec.md` scenarios and confirm each is satisfied by the built artifact on at least one platform
- [ ] 7.2 Walk through `specs/render-backend/spec.md` scenarios — confirm GLFW absent from iOS binary, Metal absent from desktop binaries (check link map / `nm` output)
- [ ] 7.3 Walk through `specs/build-system/spec.md` scenarios on a clean checkout — `cmake --preset` succeeds for all four presets on supported hosts
- [ ] 7.4 Walk through `specs/ui-sample/spec.md` scenarios — menu bar, demo toggle, About dialog all behave as specified on every built platform
- [ ] 7.5 Run `openspec validate --change imgui-cross-platform-app --strict` and resolve any reported issues before archiving
