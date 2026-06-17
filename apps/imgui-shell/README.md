# imgui-shell

A cross-platform Dear ImGui application skeleton that targets **macOS, Windows, Linux, and iOS** from a single C++17 codebase. Built around `GLFW + Vulkan 1.2` on desktop and `Metal + UIKit` on iOS.

## What this is

- A buildable application shell — one menu bar, one demo toggle, one About dialog.
- A shared `app/` core (lifecycle interface `init` / `frame` / `shutdown`) that compiles unchanged on every supported platform.
- A thin per-platform host (`platform/desktop/` for the Vulkan + GLFW path, `platform/ios/` for the Metal + UIKit path).
- A CMake-based build system with one preset per platform (`macos`, `windows`, `linux`, `ios`).

## What this is not

- Not a production application — it has no business logic beyond the sample UI.
- Not a custom widget library — see the separate `add-node-graph-core` change for that work.
- Not a cross-platform abstraction layer — backend selection is a build-time decision; there is no runtime renderer-switching, no `IRenderer` interface.
- Not yet signed / notarized / store-uploaded — that's an explicit non-goal for this shell.

## Per-platform prerequisites

| Platform | Compiler | Required SDK | Notes |
|---|---|---|---|
| **macOS** ≥ 13 | Xcode 15+ Command Line Tools (Apple Clang 15) | [LunarG Vulkan SDK ≥ 1.2](https://vulkan.lunarg.com) — bundles MoltenVK | Apple Silicon and Intel both supported |
| **Linux** (Ubuntu 22.04+) | GCC 11+ or Clang 14+ | LunarG Vulkan SDK ≥ 1.2 (`apt install vulkan-sdk`), plus GLFW deps: `xorg-dev libwayland-dev libxkbcommon-dev` | |
| **Windows** 10+ | Visual Studio 2022 (17.x) | LunarG Vulkan SDK ≥ 1.2 | `VULKAN_SDK` env var set by the installer |
| **iOS** 15+ | **Full Xcode** (not just Command Line Tools) — required to drive the generated `imgui_shell_ios.xcodeproj` and sign | none (Metal is in the Apple SDK) | Requires an Apple Developer signing identity to run on a device |

In addition to the above, every target needs **CMake ≥ 3.24** and **Ninja** (or VS 2022's MSBuild on Windows).

## Building

The build is preset-driven. From `apps/imgui-shell/`:

```sh
# Desktop (any of macOS / Linux / Windows):
cmake --preset macos             # or linux / windows
cmake --build --preset macos     # or linux / windows
./build/macos/platform/desktop/imgui_shell_desktop
```

```sh
# iOS — Xcode-driven:
cmake --preset ios
open build/ios/imgui_shell.xcodeproj
# In Xcode: select a development team in the imgui_shell_ios target's "Signing & Capabilities", then Run.
```

### macOS runtime note: KosmicKrisp ICD

If you have multiple Vulkan drivers installed (e.g., the experimental `libvulkan_kosmickrisp.dylib` from a third-party tap), the loader may pick the wrong one and crash. Force MoltenVK explicitly:

```sh
export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json
./build/macos/platform/desktop/imgui_shell_desktop
```

The same applies in CI on macOS runners that bundle multiple ICDs.

### Windows launch behavior

Windows builds have different console behavior based on build type:
- **Debug builds**: Show a console window with debug output (default behavior)
- **Release builds**: Hide the console window for a cleaner GUI application experience

The console window in Debug builds is useful for developers to see debug output. Release builds use Windows subsystem (`/SUBSYSTEM:WINDOWS`) to hide the console window when launched by double-clicking or from Windows Explorer. Debug output is still available when launching Release builds from an existing command prompt.

## Project layout

```
apps/imgui-shell/
├── CMakeLists.txt            # root: project, platform detection, subdir wiring
├── CMakePresets.json         # presets: macos | windows | linux | ios
├── cmake/Dependencies.cmake  # FetchContent: ImGui (docking) + GLFW; find_package(Vulkan)
├── app/                      # shared core — compiles unchanged on all 4 targets
│   ├── App.{h,cpp}           #   lifecycle interface + sample UI
│   └── RenderContext.h       #   platform-gated handle bag
├── platform/desktop/         # GLFW + Vulkan host (macOS / Linux / Windows)
│   ├── main.cpp
│   ├── vk_instance.{h,cpp}
│   ├── vk_device.{h,cpp}
│   ├── vk_swapchain.{h,cpp}
│   └── vk_frame.{h,cpp}
└── platform/ios/             # Metal + UIKit host (iOS)
    ├── AppDelegate.{h,mm}
    ├── ViewController.{h,mm}
    ├── main.mm
    └── Info.plist.in
```

## CI

`.github/workflows/imgui-shell-build.yml` builds the three desktop presets on every PR. iOS is build-only locally (Xcode + signing identity required, not provisioned in CI for this change). See `openspec/changes/imgui-cross-platform-app/design.md` for the rationale.

## Where to learn more

- Architecture: [`docs/architecture.md`](docs/architecture.md)
- Full design rationale and rejected alternatives: [`openspec/changes/imgui-cross-platform-app/design.md`](../../openspec/changes/imgui-cross-platform-app/design.md)
- Spec deltas: [`openspec/changes/imgui-cross-platform-app/specs/`](../../openspec/changes/imgui-cross-platform-app/specs/)
