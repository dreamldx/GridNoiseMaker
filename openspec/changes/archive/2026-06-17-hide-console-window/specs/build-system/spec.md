# build-system Specification

## MODIFIED Requirements

### Requirement: CMake presets per platform
The project SHALL ship a `CMakePresets.json` with at least four configure presets — `macos`, `windows`, `linux`, `ios` — and matching build presets. Each preset SHALL pin the generator, architecture (where relevant), and `CMAKE_BUILD_TYPE` defaults. The Windows preset SHALL configure the desktop executable as a Windows subsystem application in Release builds.

#### Scenario: One-command build per platform
- **WHEN** a contributor runs `cmake --preset <name> && cmake --build --preset <name>`
. **THEN** a runnable artifact (executable on desktop, `.app` bundle for iOS) SHALL be produced under the preset's build directory

#### Scenario: iOS preset uses Xcode generator
- **WHEN** the `ios` preset is configured
. **THEN** the generator SHALL be Xcode
. **AND** `CMAKE_SYSTEM_NAME` SHALL be `iOS`
. **AND** the desktop host target SHALL be excluded from the build

#### Scenario: Desktop presets exclude iOS host
- **WHEN** any desktop preset (`macos`, `windows`, `linux`) is configured
. **THEN** the iOS host target SHALL be excluded from the build
. **AND** UIKit / Metal framework links SHALL NOT be added

#### Scenario: Windows Release builds use Windows subsystem
- **WHEN** the Windows preset is configured with `CMAKE_BUILD_TYPE=Release`
. **THEN** the `imgui_shell_desktop` target SHALL be built as a Windows subsystem application (`/SUBSYSTEM:WINDOWS`)
. **AND** the executable SHALL NOT show a console window when launched by double-clicking

#### Scenario: Windows Debug builds use Console subsystem
1. **WHEN** the Windows preset is configured with `CMAKE_BUILD_TYPE=Debug`
. **THEN** the `imgui_shell_desktop` target SHALL be built as a Console subsystem application (`/SUBSYSTEM:CONSOLE`)
. **AND** developers SHALL see debug output in the console

## ADDED Requirements

### Requirement: Windows executable subsystem configuration
The Windows desktop executable SHALL have configurable subsystem based on build type, using CMake's `WIN32_EXECUTABLE` property to control console window visibility.

#### Scenario: WIN32_EXECUTABLE property configured
- **WHEN** CMake configures the Windows preset
. **THEN** the `imgui_shell_desktop` target SHALL have `WIN32_EXECUTABLE` property set based on `CMAKE_BUILD_TYPE`
. **AND** Release builds SHALL have `WIN32_EXECUTABLE=TRUE`
. **AND** Debug builds SHALL have `WIN32_EXECUTABLE=FALSE` (or unset)

#### Scenario: Visual Studio project reflects correct subsystem
- **WHEN** Visual Studio opens the generated Windows project
. **THEN** the `imgui_shell_desktop` project properties SHALL show the correct subsystem configuration
. **AND** Release configuration SHALL show "Windows" subsystem
. **AND** Debug configuration SHALL show "Console" subsystem
