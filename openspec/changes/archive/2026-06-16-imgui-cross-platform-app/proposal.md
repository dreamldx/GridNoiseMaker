## Why

We want a single GUI codebase that ships natively on macOS, Windows, Linux, and iOS without rewriting the UI per platform. Dear ImGui gives us immediate-mode UI with minimal runtime overhead, but the project needs an opinionated structure (backend selection, build system, platform shims, asset/lifecycle handling) so that "build for all four targets" is a one-command operation rather than four bespoke projects glued together.

## What Changes

- Add a new C++17 application skeleton centered on Dear ImGui (docking branch) with a shared `app/` core that owns the UI and lifecycle.
- Select a single rendering/windowing backend stack per platform family:
  - macOS / Windows / Linux desktop: GLFW + Vulkan 1.2 (native loader on Windows/Linux, MoltenVK on macOS).
  - iOS: Apple Metal + UIKit view controller host (no GLFW, no Vulkan; iOS has its own app lifecycle).
- Add a CMake-based build system with platform presets (`macos`, `windows`, `linux`, `ios`) and a vendored / FetchContent-managed Dear ImGui dependency pinned to a known revision.
- Provide a minimal sample window (menu bar, demo panel toggle, About dialog) so each platform target produces a runnable, visually identical app on first build.
- Add CI matrix definitions (build-only, not release signing) for the three desktop platforms; iOS build documented as a local Xcode-driven step because it requires a developer signing identity.
- Add docs covering: prerequisites per platform, how to build, how to run, and how the iOS target differs.

Non-goals (called out explicitly to bound scope):
- No application-specific business logic — this change delivers the shell only.
- No mobile-Android target.
- No automated code signing / TestFlight / notarization pipeline.
- No custom widget library on top of ImGui.

## Capabilities

### New Capabilities
- `app-shell`: The cross-platform application entry point, main loop, and lifecycle (init, per-frame tick, shutdown) that hosts the ImGui context on every supported platform.
- `render-backend`: The selection and abstraction of rendering/windowing backends per platform (GLFW+Vulkan 1.2 for desktop with MoltenVK on macOS, Metal+UIKit for iOS), including ImGui backend wiring.
- `build-system`: The CMake configuration, platform presets, and dependency management (Dear ImGui pinning) that produces a buildable artifact per target.
- `ui-sample`: The minimal first-run UI (menu bar, demo toggle, About dialog) that verifies the shell is wired correctly on each platform.

### Modified Capabilities
<!-- None — this is a greenfield project; no existing specs in openspec/specs/ to modify. -->

## Impact

- New top-level project directory under the repo (e.g., `apps/imgui-shell/`) — code, CMake, and platform-specific entry points.
- New third-party dependency: Dear ImGui (zlib/libpng-style MIT license — compatible).
- New build-time dependencies: CMake ≥ 3.24, a C++17 compiler per platform, GLFW (desktop), LunarG Vulkan SDK ≥ 1.2 on each desktop platform (bundles MoltenVK on macOS), Xcode toolchain (iOS/macOS).
- New CI surface: GitHub Actions (or equivalent) jobs for macOS / Windows / Linux build-only.
- No existing code is modified — this is additive.
- Developer ergonomics: contributors will need platform SDKs installed locally for the targets they build; documented in the new README.
