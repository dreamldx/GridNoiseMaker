# imgui_playground — System Design

> Status: living document. Generated from a full read of the source tree on
> 2026-06-20. For the *operational* one-pager see
> [`apps/imgui-shell/docs/architecture.md`](../apps/imgui-shell/docs/architecture.md);
> for the *why* behind individual decisions see the per-change `design.md`
> files under `openspec/changes/`. This document is the consolidated technical
> overview of the whole application as it stands today.

---

## 1. Purpose & scope

`imgui_playground` is a cross-platform [Dear ImGui](https://github.com/ocornut/imgui)
application shell that builds from a single C++17 source set onto **macOS,
Windows, Linux, and iOS**. It demonstrates a clean separation between a
**platform-agnostic UI core** and **thin per-platform rendering hosts**, and
carries three substantive feature subsystems on top of the bare shell:

| Subsystem | What it does |
|---|---|
| **App shell** | Menu bar, About / Demo modals, ImGui context lifecycle. |
| **Node-graph widget** | A from-scratch pan/zoom canvas with draggable, typed nodes and versioned JSON persistence. |
| **Theme system** | A fully editable ImGui style (colors / metrics / typography) with per-theme files, a live editor, and atomic persistence. |
| **Preferences window** | A tabbed window exposing build info, the theme editor, and credits. |

The project is coordinated through [OpenSpec](https://github.com/derek-johansson/openspec):
behavior is described in `openspec/specs/<capability>/spec.md`, and the source
code references those specs directly in comments.

### 1.1 Development Workflow
Changes follow a structured OpenSpec workflow:
1. **Change creation**: `openspec new-change <name>` creates proposal → design → specs → tasks
2. **Implementation**: Complete tasks with verification (build/test commands)
3. **Spec sync**: Delta specs merged to main specifications after implementation
4. **Archive**: Completed changes moved to dated archive directories
5. **Commit convention**: Follows Conventional Commits format: `<type>(<scope>): <description>`

Recent completed change: **add-node-persistence** (2026-06-20) added JSON serialization,
File menu integration, NodeType system, and auto-load capabilities.

### Non-goals

- Not a production app — no domain logic beyond the sample UI.
- No runtime renderer switching — the graphics backend is a **build-time**
  decision (Vulkan on desktop, Metal on iOS); there is deliberately no
  `IRenderer` abstraction.
- Not signed / notarized / store-uploaded.

---

## 2. High-level architecture

The system is a strict **two-layer split** joined by a single value type,
`RenderContext`.

```
┌───────────────────────────────────────────────────────────────┐
│                            app/  (shared C++17 core)            │
│                                                                 │
│   App.cpp ── owns ImGui context; builds menu/modals each frame  │
│     │                                                           │
│     ├── NodeGraphWidget ── SimpleGridRenderer ── ViewTransform  │
│     ├── Preferences  (window + theme editor UI)                 │
│     ├── Theme        (baked-in style + typography accessors)    │
│     └── ThemeStorage (per-OS JSON persistence, migration)       │
│                                                                 │
│   Identical source on every target. Contains ALL UI code.       │
└───────────────────────────────────────────────────────────────┘
                  ▲                              ▲
                  │  app::init / frame / shutdown│
                  │  RenderContext& (handle bag) │
                  ▼                              ▼
   ┌─────────────────────────┐      ┌────────────────────────────┐
   │   platform/desktop/      │      │        platform/ios/        │
   │   main.cpp (run loop)    │      │   AppDelegate / ViewController│
   │   vk_instance/device/    │      │   main.mm                   │
   │   swapchain/frame        │      │                             │
   │   GLFW + Vulkan 1.2       │      │   Metal + UIKit (MTKView)   │
   └─────────────────────────┘      └────────────────────────────┘
            │                                     │
   LunarG loader / MoltenVK            Apple Metal / CAMetalLayer
```

### 2.1 Why inversion of control

The core does **not** own the main loop. It exposes three calls:

```cpp
app::init(RenderContext&);   // once, after the backend is ready
app::frame(RenderContext&);  // every frame, between backend NewFrame and present
app::shutdown();             // once, before tearing down the backend
```

The desktop host drives these from a `while (!glfwWindowShouldClose)` loop; the
iOS host drives `app::frame()` from `MTKViewDelegate::drawInMTKView:`. iOS apps
*cannot* own their main loop (UIKit/`UIApplicationMain` does), so an
"app-runs-the-loop" design is dead on arrival there. IoC is the only structure
that works for both platform families.

### 2.2 The platform seam: `RenderContext` and `Platform.h`

- **`app/RenderContext.h`** is a compile-time-gated "handle bag". On desktop it
  carries Vulkan handles (`VkInstance`, `VkPhysicalDevice`, `VkDevice`, queue,
  render pass, descriptor pool, per-frame command buffer); on iOS it carries
  opaque `void*` Metal handles (so the header is valid in both `.cpp` and `.mm`
  translation units). The host populates it before each `app::` call.
- **`app/Platform.h`** centralizes the only `#ifdef` the core needs, exposing
  `constexpr bool kIsDesktop / kIsMobile / kIsWindows / kIsMacOS / kIsLinux`.
  Core code branches on these `constexpr` traits (often `if constexpr`) instead
  of scattering preprocessor conditionals — this is what lets `app/` stay
  "identical source across platforms".

---

## 3. Module reference (`app/`)

| File | Responsibility |
|---|---|
| `App.{h,cpp}` | Lifecycle (`init`/`frame`/`shutdown`), main menu bar, About/Demo/file-dialog modals, font-atlas rebuild plumbing, theme orchestration, node-graph hosting. |
| `Platform.h` | Compile-time platform traits. |
| `RenderContext.h` | Platform-gated handle bag passed across the seam. |
| `Theme.{h,cpp}` | The baked-in "imgui-shell dark" style + typography runtime accessors. |
| `ThemeStorage.{h,cpp}` | JSON persistence: selection file, per-theme files, listing, legacy migration, atomic writes. |
| `Preferences.{h,cpp}` | The Preferences window and its General/Theme/About tabs; the theme editor UI. |
| `NodeGraphWidget.{h,cpp}` | The node canvas: types, nodes, JSON load/save, render + input. |
| `SimpleGridRenderer.{h,cpp}` | Zoom-adaptive infinite grid; owns the `ViewTransform`. |
| `ViewTransform.{h,cpp}` | 2D pan/zoom world↔screen mapping (no matrix/glm dependency). |

### 3.1 App core (`App.cpp`)

`App.cpp` is the heart. It holds a small amount of file-scope state behind an
anonymous namespace:

- A lifecycle `State` enum (`Uninitialized → Running → ShutDown`) asserted at
  every entry point.
- `g_quitRequested` (desktop File ▸ Quit), `g_fontFallback` (whether the
  built-in Proggy font is in use).
- The node-graph widget (`std::unique_ptr<NodeGraphWidget>`) and the
  save/load file-dialog flags.
- The **deferred font-rebuild** flag and the platform-supplied rebuild
  callback (see §7).

`frame()` each tick: drains a pending font rebuild → `ImGui::NewFrame()` →
builds the menu bar → opens/draws the About, Demo, Save, and Load modals →
renders the Preferences window → renders the node-graph widget →
`ImGui::Render()`. A notable ImGui idiom enforced here is **flag-then-open** for
menu→modal transitions (calling `OpenPopup` from inside `BeginMenu` computes the
wrong ID-stack scope, so the modal would never open).

### 3.2 Node-graph subsystem

```
NodeGraphWidget ──owns──> SimpleGridRenderer ──owns──> ViewTransform
       │                                                    ▲
       │ m_nodes: vector<Node>  (world-space rects)         │ world↔screen
       │ NodeTypeRegistry (singleton): name → NodeType      │
       │ m_filePathBuffer: char[] (save/load modal input)    │
       └── render(): grid → nodes → toolbar → context menu ─┘
```

- **`Node`** — a rectangle in **world coordinates** (`position`/`size` are
  pre-zoom), plus colors, a title, a `type` tag, and a free-form
  `nlohmann::json` property bag. `dragging` is transient per-frame UI state and
  is *not* serialized.
.

 **`NodeType` / `NodeTypeRegistry`** — a process-wide Meyers singleton mapping
  type names ("input", "processor", "output") to a default color + default
  property template. `createNode()` stamps a node from a template; unknown
  types degrade to a bare "default" node. The registry uses `std::unordered_map<std::string, NodeType>` with `default_properties` as `nlohmann::json` for type-specific configuration.
- **Rendering** is immediate-mode: every node's world rect is converted to
  screen space through the current `ViewTransform` at draw time, so a single
  pan/zoom affects the grid and all nodes uniformly.
- **Input** (all gated on the canvas window being hovered): middle-drag pans,
  `Ctrl`+wheel zooms about the cursor, right-click opens the context menu, and
  left-press latches **one** node (top-most hit, back-to-front) to drag until
  release. The drag is *latched on press* deliberately so a fast mouse motion
  that outruns the node rect doesn't cancel the drag.
- **Persistence** is versioned (`"version": 1`) and self-describing — a saved
  file carries both a `nodes` array and a `nodeTypes` object so loading can
  re-register types. All load/save paths swallow exceptions and report success
  as a `bool`, so a malformed file never crashes the app.

### 3.3 Coordinate system (`ViewTransform`)

The view is a uniform scale (zoom) plus translation, expressed as a single pair
of formulas (no matrix machinery):

```
screen = zoom * (world − viewOffset) + viewportCenter + viewportPos
world  = (screen − viewportPos − viewportCenter) / zoom + viewOffset
```

`viewOffset` is the world point shown at the canvas center. Zoom is clamped to
`[0.01, 100]`. Zoom-about-cursor is implemented by recording the world point
under the cursor, changing the zoom, then shifting `viewOffset` by however much
that point moved — keeping it pinned. `snapToGrid`/`shouldSnap` exist and are
correct but are not yet wired up (reserved for a planned node-drag snap feature).

### 3.4 Theme system

Two cooperating files:

- **`Theme.cpp`** holds the **baked-in** "imgui-shell dark" palette and metrics
  (`applyTheme()`), plus the **runtime typography accessors** (`themeFontFile`,
  `themeFontSizePx`, `themePopupMenuMargin` and their setters). The palette is a
  cool dark-blue scheme with a cyan accent at three brightness tiers; an 8-bit
  RGB helper `c(r,g,b,a)` keeps the table reading like hex. `applyTheme()`
  *requires* the caller to have called `ImGui::StyleColorsDark()` first, so that
  any future `ImGuiCol_` slot ImGui adds still looks reasonable.
- **`ThemeStorage.cpp`** is the persistence layer (see §6).

The orchestration glue lives in `App.cpp`:
`applySelectedThemeToStyle()` (resolve selection → load file, falling back to
"dark" then to the baked-in theme), `switchTheme()` (write selection + reapply +
rebuild atlas), and `resetSelectedTheme()` (delete the user-dir copy and reload
the bundled file).

### 3.5 Preferences window

A single **non-modal** ImGui window toggled by a file-scope flag, with three
tabs:

- **General** — app identity, build config, Dear ImGui version, and (desktop)
  Vulkan API version + GPU name, queried once and cached.
- **Theme** — a picker combo plus a two-column master-detail editor. The left
  pane lists every `ImGuiCol_` color (with swatches), every scalar/vec2 metric,
  the custom popup-menu margin, and typography. The right pane edits the
  selected item directly into the live `ImGuiStyle`. Edits **autosave on
  commit** (`IsItemDeactivatedAfterEdit` → `persistTheme()`), writing to the
  selected theme's *user-dir* file (first edit of a bundled theme transparently
  creates a user-dir copy; the bundled file is never written). Font/size/margin
  changes additionally trigger a font-atlas rebuild.
- **About** — version and third-party license credits.

---

## 4. Rendering hosts (`platform/`)

### 4.1 Desktop (GLFW + Vulkan 1.2)

The desktop host is a hand-rolled, minimal Vulkan renderer split into focused
units:

| File | Responsibility |
|---|---|
| `main.cpp` | `runApp()` lifecycle + the per-frame `renderOneFrame()`; live-resize callbacks; font-rebuild callback; Windows embedded icon. |
| `vk_instance.{h,cpp}` | `VkInstance`, portability enumeration, debug-utils validation (debug builds). |
| `vk_device.{h,cpp}` | Surface + physical/logical device + a single graphics+present queue. |
| `vk_swapchain.{h,cpp}` | Swapchain, image views, single-color render pass, framebuffers; recreate/destroy. |
| `vk_frame.{h,cpp}` | Per-frame-in-flight command buffer / semaphore / fence. |
| `vk_check.h` | `VK_CHECK` macro → throw with a stringified `VkResult`. |

**Frame structure** (`renderOneFrame`): wait on the in-flight fence → acquire
next image → begin command buffer + render pass (clear) → backend `NewFrame`s →
`app::frame()` (which calls `ImGui::NewFrame` + `ImGui::Render`) →
`ImGui_ImplVulkan_RenderDrawData` → end pass → (if multi-viewport enabled)
`UpdatePlatformWindows` + `RenderPlatformWindowsDefault` → submit → present.
`VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` on acquire or present triggers a
swapchain recreate.

**Synchronization model** is textbook double-buffering with one subtlety:
**render-finished semaphores live in the swapchain (one per image), not in the
per-frame set**, because a binary semaphore signaled by `vkQueuePresentKHR` may
remain in use until the image is re-acquired. `MAX_FRAMES_IN_FLIGHT == 2`.

**Live resize**: `renderOneFrame` is also invoked from the GLFW
framebuffer-size / refresh callbacks (`resizeCallbackWork`). On macOS, a Cocoa
modal resize loop would otherwise freeze the window mid-drag; re-entering a full
frame from the callback (safe, because the main loop is paused inside
`glfwPollEvents` and ImGui is between frames) keeps it laying out live.

**Multi-viewport**: enabled on desktop only (`ImGuiConfigFlags_ViewportsEnable`),
so dragging an ImGui sub-window outside the host spawns a real OS window.

**Validation**: in debug builds, `VK_LAYER_KHRONOS_validation` + a debug
messenger that logs to stderr and `std::abort()`s on error severity.

### 4.2 iOS (Metal + UIKit)

| File | Responsibility |
|---|---|
| `main.mm` | `UIApplicationMain` entry. |
| `AppDelegate.{h,mm}` | Creates the key window + root `ViewController`. |
| `ViewController.{h,mm}` | Owns `MTLDevice`/`MTLCommandQueue`/`MTKView`; drives `app::frame` from `drawInMTKView:`; wires the Metal font-rebuild callback. |

iOS has **no swapchain to manage** — `MTKView` handles drawables and resize.
`drawInMTKView:` updates `io.DisplaySize` / framebuffer scale each frame, calls
`ImGui_ImplMetal_NewFrame` → `app::frame` → encodes draw data → presents. Before
`app::init`, the host calls `setBundleResourcePath` (for bundled assets) so the
font loader can resolve paths at runtime.

### 4.3 Backend init / shutdown ordering (a hard contract)

Dear ImGui requires a strict order, asserted in both directions:

```
init:      app::init → CreateContext
           host      → ImplGlfw_InitForVulkan → ImplVulkan_Init   (desktop)
                     → ImplMetal_Init                              (iOS)
shutdown:  host      → ImplVulkan_Shutdown → ImplGlfw_Shutdown
           app::shutdown → DestroyContext
```

`app::shutdown()` defensively asserts `io.BackendRendererUserData == nullptr`
and `io.BackendPlatformUserData == nullptr` so a host that tears down in the
wrong order fails *here*, with a spec reference, rather than deep inside ImGui.
On desktop, `vkDeviceWaitIdle` precedes all teardown.

---

## 5. Control flow & lifecycle (sequence)

```
main()                         (platform/desktop/main.cpp)
 └─ runApp()
     ├─ glfwInit / createWindow / set icon / center / focus
     ├─ createInstance → createDevice → createSwapchain
     │   → createFrameResources → createImGuiDescriptorPool
     ├─ populate RenderContext
     ├─ app::init(ctx)
     │   ├─ capture Vulkan handles; create NodeGraphWidget
     │   ├─ auto-load assets/default_node_graph.json (if present)
     │   ├─ ImGui::CreateContext; flags (docking, nav, [desktop] viewports)
     │   ├─ StyleColorsDark → applyTheme → migrateLegacyThemeFile
     │   │   → applySelectedThemeToStyle → loadPreferences
     │   └─ configureFontAtlas
     ├─ ImplGlfw_InitForVulkan → ImplVulkan_Init → CreateFontsTexture
     ├─ registerRebuildFontAtlasCallback(&rebuildFontAtlasDesktop)
     ├─ wire resize callbacks
     ├─ while (!shouldClose && !quitRequested):
     │     glfwPollEvents
     │     renderOneFrame  ── calls app::frame(ctx) ──┐
     │                                                ▼
     │                          [drain font rebuild] NewFrame → menu/modals
     │                          → Preferences → NodeGraph → Render
     └─ teardown (reverse order): waitIdle → clear callbacks
         → ImplVulkan_Shutdown → ImplGlfw_Shutdown → app::shutdown
         → destroy frames/pool/swapchain/device/instance/window
```

---

## 6. Persistence

Three independent JSON stores, all under the per-OS user config root:

```
macOS / Linux: ${XDG_CONFIG_HOME:-$HOME/.config}/imgui-shell/
Windows:       %APPDATA%/imgui-shell/
iOS:           <Documents>/
```

| File | Owner | Contents |
|---|---|---|
| `theme.json` | `ThemeStorage` | `{ "_schema_version": 2, "selected_theme": "<name>" }` — selection only. |
| `themes/<name>.json` | `ThemeStorage` | A theme's `colors` / `metrics` / `typography` blocks. |
| `preferences.json` | `Preferences` | `{ "_schema_version": 1, "context_menu_margin": <px> }`. |
| `<path>.json` (node graph) | `NodeGraphWidget` | Versioned graph (`nodes` + `nodeTypes`). User-chosen path via Save/Load dialogs; `assets/default_node_graph.json` auto-loads at startup. |

**Key persistence properties:**

- **Atomic writes** everywhere: serialize to `‹target›.tmp.‹pid›`, then
  `rename` over the target. Parent dirs are created on demand.
- **Bundled vs user themes**: themes are discovered by scanning *both*
  `assets/themes/` (bundled, read-only) and the user `themes/` dir; on a name
  collision the **user copy wins**. Editing a bundled theme creates a user copy;
  the bundled original is never mutated. "Reset to defaults" deletes the user
  copy and reloads the bundled file.
- **Color encoding** differs by store: theme files store `ImGuiCol_` colors as
  4-element **float** arrays (0..1); node-graph files store node colors as
  4-element **integer** `[r,g,b,a]` arrays unpacked from the packed `ImU32`.
- **Schema metric tables** (`kScalarMetrics` / `kVec2Metrics`) are the single
  source of truth shared by read and write paths in `ThemeStorage.cpp`, and
  mirrored (with UI ranges/format) in `Preferences.cpp`.
- **Migration**: a pre-`_schema_version` `theme.json` carrying inline
  colors/metrics/typography is migrated once into `themes/custom.json` and the
  selection rewritten to `"custom"`.
- **Robustness**: malformed/missing files degrade silently (theme selection
  falls back to `"dark"`); parse errors log a single stderr line.

---

## 7. Font subsystem

Fonts are the one place where the platform-agnostic core must reach back into
the platform's GPU texture lifecycle, solved with a registered callback.

- **`configureFontAtlas()`** (in `App.cpp`, public) rasterizes the configured
  TTF (`themeFontFile()` at `themeFontSizePx()`) into the ImGui atlas, using
  FreeType with light hinting when `IMGUI_ENABLE_FREETYPE` is set. If the file
  is missing or fails to rasterize, it falls back to ImGui's built-in Proggy
  bitmap font (and sets `g_fontFallback`, surfaced in the About dialog).
- **Runtime rebuild** is **deferred**: ImGui locks the font atlas between
  `NewFrame` and `Render`, and `Fonts->Clear()` asserts inside that window.
  `rebuildFontAtlas()` therefore only sets `g_fontRebuildPending`; `app::frame`
  drains it at the *top* of the next frame, before `NewFrame` locks the atlas.
- **The platform supplies the GPU half** of the rebuild via
  `registerRebuildFontAtlasCallback`. Desktop's `rebuildFontAtlasDesktop`:
  `vkDeviceWaitIdle` → `DestroyFontsTexture` → `Fonts->Clear` →
  `configureFontAtlas` → `CreateFontsTexture`. iOS's `rebuildFontAtlasIos` does
  the Metal equivalent (no explicit GPU wait — Metal's refcounted textures stay
  alive until in-flight command buffers complete). This is what makes
  Preferences typography edits take effect live.

Three OFL-licensed fonts ship under `assets/fonts/`: Inter (default), JetBrains
Mono, and Lato.

---

## 8. Build system

CMake ≥ 3.24, preset-driven (`CMakePresets.json`: `macos | windows | linux |
ios`).

- The root `CMakeLists.txt` derives `IMGUI_SHELL_PLATFORM` from the toolchain,
  sets `IMGUI_SHELL_DESKTOP` / `IMGUI_SHELL_MOBILE`, and adds only the matching
  `platform/` subdir. C++17 is enforced (`CMAKE_CXX_EXTENSIONS OFF`).
- `cmake/Dependencies.cmake` uses `FetchContent` for ImGui (docking branch) +
  GLFW and `find_package(Vulkan)` for the system SDK.
- `IMGUI_SHELL_USE_FREETYPE` (default ON) selects FreeType over stb_truetype.
- Compile-time identity defines (`IMGUI_SHELL_PLATFORM_NAME`,
  `…_FONT_BACKEND`, `…_THEME_NAME`, `IMGUI_SHELL_ASSETS_DIR`) feed the About
  dialog and asset resolution.
- CI (`.github/workflows/`) builds the three desktop presets on every PR; iOS is
  build-only locally (requires full Xcode + a signing identity).

**Asset resolution** is itself platform-gated: on desktop, `resolveAssetPath`
joins the compile-time `IMGUI_SHELL_ASSETS_DIR`; on iOS it joins the runtime
`NSBundle.mainBundle.resourcePath` supplied via `setBundleResourcePath`.

---

## 9. Cross-cutting design decisions

| Decision | Rationale |
|---|---|
| Inversion of control (host owns the loop) | Only structure that works for both desktop and iOS. |
| Build-time backend selection (no `IRenderer`) | Avoids a runtime abstraction layer with no second consumer; each host stays direct and debuggable. |
| `constexpr` platform traits over scattered `#ifdef` | Keeps `app/` source identical across targets; branches compile away. |
| Deferred font-atlas rebuild | ImGui asserts on atlas mutation between NewFrame/Render. |
| Atomic temp-file + rename writes | No half-written config on crash/power-loss. |
| User-dir copy-on-edit for themes | Bundled defaults stay pristine; "Reset" is a delete + reload. |
| Per-image (not per-frame) present semaphores | A present-signaled binary semaphore can be in use until re-acquire. |
| `nlohmann/json` exceptions → `bool` at the boundary | A bad file degrades gracefully instead of crashing. |
| Simple flat JSON structure (not hierarchical) | All node properties at same level for human readability and simplicity. |
| Arrays for vector data (`[x, y]`, `[r, g, b, a]`) | Consistent serialization pattern matching ImGui's packed color format. |
| Type-safe property getters/setters with templates | Runtime type checking while maintaining compile-time safety for common types. |
| File menu integration only (no context menu) | Consistent UI pattern with existing theme/preferences persistence. |
| Auto-load from `assets/default_node_graph.json` | Zero-configuration startup experience with fallback to programmatic defaults. |
| Node type registry with default properties | Type-specific configuration separate from visual properties, extensible for future features. |

### Known constraints

- C++17 baseline (matches MoltenVK / iOS minimum deployment).
- Vulkan 1.2 baseline (highest fully supported by MoltenVK at time of writing).
- Single-window, single-queue desktop host.
- No persistent `imgui.ini` (`io.IniFilename = nullptr`, intentional).
- The node graph has no edges/connections yet — nodes are independent
  rectangles; the type registry and property bags are forward-looking
  scaffolding for a future evaluation engine.
-
 Grid spacing is discontinuous at the zoom-out thresholds (a visible "pop").
-TODO File dialogs use simple ImGui modal inputs (not native file pickers).
- Node `dragging` state is runtime-only, not persisted in JSON.
- JSON schema version 1 with no backward compatibility handling yet.
. Default project auto-load depends on `assets/` directory accessibility.

---

## 9.1 JSON Persistence Schema Details

Node graph JSON uses a simple flat structure (version 1):

```json
{
  "version": 1,
  "nodes": [
    {
      "position": [x, y],
      "size": [width, height], 
      "color": [r, g, b, a],
      "borderColor": [r, g, b, a],
      "title": "Node Title",
      "type": "input",
      "properties": {
        "custom_property": "value"
      }
    }
  ],
  "nodeTypes": {
    "input": {
      "defaultColor": [r, g, b, a],
      "defaultProperties": {"frequency": 440}
    }
  }
}
```

**Design choices:**
- **Flat structure**: All properties at same level for readability and simplicity
  
- **Array format**: `[x, y]` for vectors, `[r, g, b, a]` for colors matches ImGui's packed `ImU32`
  
. **Self-describing**: Includes `nodeTypes` so files can be loaded independently
  
. **Type-safe access**: Template methods `getProperty<T>()` with runtime fallbacks
  
. **Error resilience**: Exceptions caught at file boundary, returns `bool` success
  
. **Auto-load**: `assets/default_node_graph.json` loads at startup if present

## 10. Directory map

```
imgui_playground/
├── apps/imgui-shell/
│   ├── CMakeLists.txt, CMakePresets.json
│   ├── cmake/Dependencies.cmake
│   ├── app/                     ← shared C++17 core (all UI)
│   │   ├── App.{h,cpp}
│   │   ├── Platform.h, RenderContext.h
│   │   ├── Theme.{h,cpp}, ThemeStorage.{h,cpp}
│   │   ├── Preferences.{h,cpp}
│   │   ├── NodeGraphWidget.{h,cpp}
│   │   ├── SimpleGridRenderer.{h,cpp}
│   │   └── ViewTransform.{h,cpp}
│   ├── platform/desktop/        ← GLFW + Vulkan host
│   │   ├── main.cpp, vk_{instance,device,swapchain,frame}.{h,cpp}
│   │   ├── vk_check.h, resource.h
│   ├── platform/ios/            ← Metal + UIKit host
│   │   ├── main.mm, AppDelegate.{h,mm}, ViewController.{h,mm}
│   ├── assets/                  ← fonts/, themes/, icons
│   └── docs/architecture.md
├── openspec/
│   ├── specs/<capability>/spec.md   ← authoritative behavioral contracts
│   └── changes/                     ← in-flight + archived changes (design.md, tasks)
├── docs/                        ← this document, design plans
└── README.md, AGENTS.md
```

### Spec index (authoritative contracts)

`app-shell`, `render-backend`, `gui-theme`, `theme-persistence`,
`theme-presets`, `preferences-dialog`, `font-rendering`, `font-validation`,
`font-diagnostics`, `node-graph-widget`, `node-graph-persistence`,
`desktop-launch-behavior`, `smart-window-subsystem`, `ui-sample`. Source
comments cite these by path — start there when changing behavior.
