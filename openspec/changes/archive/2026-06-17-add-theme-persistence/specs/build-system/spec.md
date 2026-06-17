## ADDED Requirements

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
