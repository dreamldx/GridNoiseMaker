## ADDED Requirements

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

## MODIFIED Requirements

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
