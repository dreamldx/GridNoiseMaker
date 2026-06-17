# imgui_playground

A playground for building **cross-platform [Dear ImGui](https://github.com/ocornut/imgui) apps** that target macOS, Windows, Linux, and iOS from one C++17 codebase. Planning, design, and implementation are coordinated through [OpenSpec](https://github.com/derek-johansson/openspec).

## What's in here

| Path | What it is |
|---|---|
| [`apps/imgui-shell/`](apps/imgui-shell/) | The cross-platform ImGui application shell. GLFW + Vulkan 1.2 on desktop (MoltenVK on macOS), Metal + UIKit on iOS. See its [`README.md`](apps/imgui-shell/README.md). |
| [`openspec/specs/`](openspec/specs/) | The current capability specs: `app-shell`, `build-system`, `render-backend`, `ui-sample`. These are the authoritative behavioral contracts. |
| [`openspec/changes/`](openspec/changes/) | Active in-flight changes. Archived changes go to `changes/archive/`. |
| [`docs/plans/`](docs/plans/) | Design docs for upcoming work that hasn't been promoted to an OpenSpec change yet. |
| [`.github/workflows/`](.github/workflows/) | CI: builds the three desktop presets on every PR. |
| `config.yaml` | OpenSpec configuration (schema = `spec-driven`). |

## Quick start (macOS / Linux / Windows)

You'll need **CMake ≥ 3.24**, the **LunarG Vulkan SDK ≥ 1.2** (from <https://vulkan.lunarg.com>), and a C++17 toolchain. iOS additionally needs full Xcode.

```sh
cd apps/imgui-shell
cmake --preset macos         # or linux / windows / ios
cmake --build --preset macos
./build/macos/platform/desktop/imgui_shell_desktop
```

For full per-platform prerequisites and the macOS-specific note about the KosmicKrisp Vulkan ICD, see [`apps/imgui-shell/README.md`](apps/imgui-shell/README.md).

## How the repo is organized

The shape mirrors the OpenSpec workflow: behavior is described in `openspec/specs/`, changes go through proposal → design → spec deltas → tasks → implementation → archive, and the implementation lives under `apps/`.

```
imgui_playground/
├── apps/imgui-shell/           code, CMake, platform hosts
├── openspec/
│   ├── specs/                  capability specs (current behavior)
│   └── changes/
│       ├── <active-change>/    proposal + design + spec deltas + tasks
│       └── archive/            shipped changes (dated)
├── docs/plans/                 brainstorming output ahead of changes
├── .github/workflows/          CI
└── config.yaml                 OpenSpec config
```

## Working with this repo

- **Browse what exists today:** read `openspec/specs/<capability>/spec.md`.
- **Propose new work:** in this repo we use the `opsx:*` slash commands (`/opsx:explore`, `/opsx:propose`, `/opsx:continue`, `/opsx:apply`, `/opsx:archive`). The underlying CLI is `openspec` — `openspec list`, `openspec status --change <name> --json`, `openspec validate <name> --type change --strict`, `openspec archive <name>`.
- **See what's been built:** check `openspec/changes/archive/` for shipped changes with their full design + tasks trail.

## Upcoming work

The next planned change is **`add-node-graph-core`** — a headless node-graph engine (data model, type system, registry, topology + dirty propagation, evaluator, JSON serialization, Catch2 tests). Design doc: [`docs/plans/2026-06-16-custom-widgets-design.md`](docs/plans/2026-06-16-custom-widgets-design.md). A subsequent `add-node-graph-editor` change will add the from-scratch ImGui canvas on top.

## License

TBD — to be confirmed before any external distribution.
