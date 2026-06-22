# Build System Reliability

## Purpose
Ensures build artifacts are reliably deployed to designated output locations with consistent copy operations and up-to-date outputs.

## Requirements

### Requirement: Reliable build artifact deployment
The build system SHALL reliably deploy built artifacts to designated output locations, ensuring that copy operations execute consistently and produce up-to-date outputs.

#### Scenario: Desktop executable copied to bin folder
- **WHEN** the desktop executable is built
- **THEN** the build system SHALL copy the executable to the root `bin/` folder
- **AND** SHALL ensure the copied executable has the same timestamp as the built executable

#### Scenario: Assets copied alongside executable
- **WHEN** the desktop executable is built
- **THEN** the build system SHALL copy required assets to the `bin/assets/` folder
- **AND** SHALL ensure asset files match source files byte-for-byte
- **AND** SHALL maintain directory structure consistency

#### Scenario: Copy operations execute on every build
- **WHEN** a rebuild occurs with no source changes
- **THEN** copy operations SHALL still execute to ensure outputs are up-to-date

#### Scenario: Platform-appropriate copy commands
- **WHEN** building on Windows
- **THEN** copy operations SHALL use reliable copy commands (`cmake -E copy` or PowerShell `Copy-Item`)
- **WHEN** building on Unix platforms (macOS, Linux)
- **THEN** copy operations SHALL use cross-platform commands (`cmake -E copy`)