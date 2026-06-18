# build-system Specification

## Purpose
TBD - created by archiving change imgui-cross-platform-app. Update Purpose after archive.
## Requirements
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
The project SHALL compile Dear ImGui sources (`imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui_demo.cpp`, the selected backends, and — when `IMGUI_SHELL_USE_FREETYPE=ON` (default) — `misc/freetype/imgui_freetype.cpp`) into a static library target consumed by the platform hosts. The project SHALL NOT depend on a system-installed ImGui.

#### Scenario: ImGui sources are part of the build
- **WHEN** the project is built from a clean checkout with no system ImGui present
- **THEN** the build SHALL succeed
- **AND** ImGui object files SHALL appear in the build directory

#### Scenario: FreeType backend object is present in default build
- **WHEN** the project is built in a default build (`IMGUI_SHELL_USE_FREETYPE=ON`)
- **THEN** an object file derived from `imgui_freetype.cpp` SHALL appear in the build directory under the `imgui` target's object outputs
- **AND** the `imgui` static library SHALL define the `IMGUI_ENABLE_FREETYPE` preprocessor symbol publicly so its consumers see the feature flag

### Requirement: CI build matrix for desktop targets
The project SHALL provide a CI configuration that builds (compile + link, no run) the `macos`, `windows`, and `linux` presets on every push and pull request. The iOS preset MAY be excluded from CI in this change.

#### Scenario: Desktop build matrix runs on PR
- **WHEN** a pull request is opened
- **THEN** CI SHALL configure and build the three desktop presets
- **AND** any preset that fails to compile or link SHALL fail the CI job

#### Scenario: iOS documented as local-only
- **WHEN** a contributor wants to build the iOS target
- **THEN** the documented procedure SHALL be `cmake --preset ios && cmake --open` (or equivalent), followed by an Xcode build with the contributor's own signing identity

### Requirement: FreeType dependency
The project SHALL depend on FreeType via CMake `FetchContent`, pinned to a specific upstream tag (`VER-2-13-3` at the time this change lands), available on every supported preset (`macos`, `windows`, `linux`, `ios`). FreeType's optional dependencies that this project does not use (zlib, bzip2, png, harfbuzz, brotli) SHALL be disabled at FetchContent configure time to minimize binary size and build complexity.

#### Scenario: FreeType is fetched and built on configure
- **WHEN** any supported preset is configured from a clean checkout in a default build (`IMGUI_SHELL_USE_FREETYPE=ON`)
- **THEN** CMake SHALL fetch FreeType at the pinned tag via `FetchContent`
- **AND** the resulting `freetype` target SHALL be linked into the `imgui` static library
- **AND** the FetchContent declaration SHALL set `FT_DISABLE_ZLIB`, `FT_DISABLE_BZIP2`, `FT_DISABLE_PNG`, `FT_DISABLE_HARFBUZZ`, `FT_DISABLE_BROTLI` to disable optional codepaths

#### Scenario: FreeType is skipped when the fallback option is selected
- **WHEN** the project is configured with `-DIMGUI_SHELL_USE_FREETYPE=OFF`
- **THEN** the FreeType `FetchContent` declaration SHALL NOT be evaluated (no clone, no build of FreeType)
- **AND** `imgui_freetype.cpp` SHALL NOT be added to the `imgui` target's source list

### Requirement: Bundled font asset is copied next to the desktop binary
The build SHALL copy `apps/imgui-shell/assets/fonts/Inter-Regular.ttf` (and its `OFL.txt` license file) into an `assets/fonts/` directory adjacent to the desktop executable on every desktop preset, so the binary can locate the font at a known relative path at runtime.

#### Scenario: Font asset present in the desktop build output
- **WHEN** a desktop preset is built
- **THEN** the path `<binary-dir>/platform/desktop/assets/fonts/Inter-Regular.ttf` SHALL exist
- **AND** the path `<binary-dir>/platform/desktop/assets/fonts/OFL.txt` SHALL exist
- **AND** the file contents SHALL match the source files byte-for-byte

#### Scenario: Font asset bundled inside the iOS .app
- **WHEN** the iOS preset is built
- **THEN** the produced `imgui_shell_ios.app` bundle SHALL contain `Resources/Inter-Regular.ttf` and `Resources/OFL.txt`

### Requirement: nlohmann/json dependency
The project SHALL depend on `nlohmann/json` via CMake `FetchContent`, pinned to a specific upstream tag (`v3.11.3` at the time this change lands), available on every supported preset (`macos`, `windows`, `linux`, `ios`). nlohmann/json is header-only; no compiled artifact is produced. The dep SHALL be linked into the `imgui_shell_app` static library with PUBLIC visibility so all `app/` and `platform/*` code that includes it transitively gets the include path.

#### Scenario: nlohmann/json is fetched and linked on configure
- **WHEN** any supported preset is configured from a clean checkout
- **THEN** CMake SHALL fetch `nlohmann/json` at the pinned tag via `FetchContent_Declare` + `FetchContent_MakeAvailable`
- **AND** the resulting `nlohmann_json::nlohmann_json` interface target SHALL be linked into `imgui_shell_app` PUBLIC
- **AND** the configure step SHALL succeed without manual intervention

#### Scenario: Header is reachable from app sources
- **WHEN** an `app/*.cpp` or `app/*.h` file includes `<nlohmann/json.hpp>`
- **THEN** the include SHALL resolve via the `nlohmann_json` interface target's PUBLIC include directory
- **AND** the project SHALL compile without an additional `target_include_directories` call

