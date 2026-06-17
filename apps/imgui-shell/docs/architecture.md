# imgui-shell architecture

> This is the **operational** architecture summary. For the *why* (rejected alternatives, risks, trade-offs), see [`openspec/changes/imgui-cross-platform-app/design.md`](../../../openspec/changes/imgui-cross-platform-app/design.md).

## Two-layer split

```
+-----------------------------------+
|             app/                  |   <- shared C++17 core, identical on every
|  init / frame / shutdown          |      platform; owns the ImGui context;
|  ImGui menu bar / About / Demo    |      contains all UI code
+-----------------------------------+
                ▲
                │ RenderContext& (platform-gated handle bag)
                ▼
+-----------------+ +---------------+
| platform/desktop| | platform/ios  |   <- per-platform host, thin: window /
|  main.cpp       | |  AppDelegate  |      view / event pump / backend setup;
|  vk_instance    | |  ViewController|     drives the lifecycle calls into app/.
|  vk_device      | |  main.mm      |      No UI logic.
|  vk_swapchain   | +---------------+
|  vk_frame       |
+-----------------+
        │                │
        ▼                ▼
   GLFW + Vulkan 1.2   Metal + UIKit
   (LunarG / MoltenVK) (CAMetalLayer)
```

## Lifecycle

The shared `app/` core does NOT own the main loop. It exposes three calls:

```cpp
app::init(RenderContext&);    // once, after the rendering backend is ready
app::frame(RenderContext&);   // every frame, between backend NewFrame and present
app::shutdown();              // once, before tearing down the rendering backend
```

The desktop host drives this from a `while (!glfwWindowShouldClose) { ... }` loop. The iOS host drives `app::frame()` from `MTKViewDelegate::drawInMTKView:`. Neither host is visible inside `app/` — the same `app/` source files compile unchanged on every target.

Why inversion-of-control: iOS apps don't own their main loop (UIKit / `UIApplicationMain` does), so a "the app runs the loop" design is dead-on-arrival there. This is the only structure that works for both platform families.

## ImGui backend wiring

Per the spec, backends are initialized in the order Dear ImGui requires:

```
app::init       →  ImGui::CreateContext()
host (desktop)  →  ImGui_ImplGlfw_InitForVulkan() then ImGui_ImplVulkan_Init()
host (iOS)      →  ImGui_ImplMetal_Init()
```

Shutdown reverses the order, and `vkDeviceWaitIdle` precedes any teardown on the desktop path.

## Where decisions live

- **Backend choice** (Vulkan/Metal split, why not OpenGL/D3D12/SDL): [`design.md`](../../../openspec/changes/imgui-cross-platform-app/design.md) §D3, §D4.
- **Build system** (CMake presets, FetchContent vs submodule): `design.md` §D2, §D6.
- **CI scope** (desktop only, iOS local-only): `design.md` §D7.

## Adding a new feature

For any UI feature, edit `app/App.cpp`'s `frame` (and optionally add new `app/*.cpp` files). The platform hosts almost never need to change.

For a new platform target, add a new `platform/<name>/` directory + a new CMake preset. Do not introduce a runtime renderer-selector — pick a backend at build time.

## Known constraints

- C++17 baseline (matches MoltenVK / iOS minimum deployment).
- Vulkan 1.2 baseline (highest fully supported by MoltenVK as of writing).
- Single-window, single-queue desktop host (no multi-viewport in v1 even though docking branch supports it).
- No persistent `imgui.ini` (intentional — see `App.cpp`).
