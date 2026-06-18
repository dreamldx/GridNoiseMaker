## ADDED Requirements

### Requirement: CMake-based build
The project SHALL build with CMake ≥ 3.24 from a single top-level `CMakeLists.txt`. No alternate build system (Make-by-hand, Bazel, Xcode-only project) SHALL be the source of truth.

#### Scenario: Top-level configure succeeds on every supported preset
- **WHEN** a contributor runs `cmake --preset <macos|windows|linux|ios>` from the project root
- **THEN** CMake SHALL configure successfully without manual edits
- **AND** the configuration step SHALL fail with a clear message if the host platform cannot build the requested preset (e.g., requesting `ios` on Linux)

### Requirement: CMake presets per platform
The project SHALL ship a `CMakePresets.json` with at least four configure presets — `macos`, `windows`, `linux`, `ios` — and matching build presets. Each preset SHALL pin the generator, architecture (where relevant), and `CMAKE_BUILD_TYPE` defaults.

#### Scenario: One-command build per platform
- **WHEN** a contributor runs `cmake --preset <name> && cmake --build --preset <name>`
- **THEN** a runnable artifact (executable on desktop, `.app` bundle for iOS) SHALL be produced under the preset's build directory

#### Scenario: iOS preset uses Xcode generator
- **WHEN** the `ios` preset is configured
- **THEN** the generator SHALL be Xcode
- **AND** `CMAKE_SYSTEM_NAME` SHALL be `iOS`
- **AND** the desktop host target SHALL be excluded from the build

#### Scenario: Desktop presets exclude iOS host
- **WHEN** any desktop preset (`macos`, `windows`, `linux`) is configured
- **THEN** the iOS host target SHALL be excluded from the build
- **AND** UIKit / Metal framework links SHALL NOT be added

#### Scenario: Vulkan SDK discovery on desktop presets
- **WHEN** any desktop preset (`macos`, `windows`, `linux`) is configured
- **THEN** CMake SHALL locate a Vulkan installation via `find_package(Vulkan 1.2 REQUIRED)`
- **AND** the configuration step SHALL fail with a clear message if the SDK is not present (e.g., "LunarG Vulkan SDK ≥ 1.2 not found — install from https://vulkan.lunarg.com/ and set VULKAN_SDK")
- **AND** the resulting `Vulkan::Vulkan` imported target SHALL be linked into the desktop host binary

### Requirement: Pinned Dear ImGui dependency
The project SHALL depend on Dear ImGui via CMake `FetchContent` (preferred) or a git submodule, pinned to a specific commit SHA on the `docking` branch. The pin SHALL be visible in version control (in `CMakeLists.txt` or `.gitmodules`).

#### Scenario: Reproducible builds across machines
- **WHEN** two contributors check out the same commit and build
- **THEN** they SHALL link the same revision of Dear ImGui
- **AND** no network state outside the pin SHALL affect the ImGui version

#### Scenario: Upgrading ImGui is deliberate
- **WHEN** a contributor changes the ImGui pin
- **THEN** the change SHALL appear as a one-line diff in `CMakeLists.txt` (or a submodule SHA update) reviewable in a pull request

### Requirement: ImGui compiled as a static library
The project SHALL compile Dear ImGui sources (`imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui_demo.cpp`, and the selected backends) into a static library target consumed by the platform hosts. The project SHALL NOT depend on a system-installed ImGui.

#### Scenario: ImGui sources are part of the build
- **WHEN** the project is built from a clean checkout with no system ImGui present
- **THEN** the build SHALL succeed
- **AND** ImGui object files SHALL appear in the build directory

### Requirement: CI build matrix for desktop targets
The project SHALL provide a CI configuration that builds (compile + link, no run) the `macos`, `windows`, and `linux` presets on every push and pull request. The iOS preset MAY be excluded from CI in this change.

#### Scenario: Desktop build matrix runs on PR
- **WHEN** a pull request is opened
- **THEN** CI SHALL configure and build the three desktop presets
- **AND** any preset that fails to compile or link SHALL fail the CI job

#### Scenario: iOS documented as local-only
- **WHEN** a contributor wants to build the iOS target
- **THEN** the documented procedure SHALL be `cmake --preset ios && cmake --open` (or equivalent), followed by an Xcode build with the contributor's own signing identity
