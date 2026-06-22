# Build System Reliability

## ADDED Requirements

### Requirement: Reliable build artifact deployment
The build system SHALL reliably deploy built artifacts to designated output locations, ensuring that copy operations execute consistently and produce up-to-date outputs.

#### Scenario: Desktop executable copied to bin folder
-p **WHEN** the desktop executable is built
- **THEN** the build system SHALL copy the executable to the root `bin/` folder
- **AND** SHALL ensure the copied executable has the same timestamp as the built executable
. **AND** SHALL log the copy operation status for debugging

#### Scenario: Assets copied alongside executable
- **WHEN** the desktop executable is built
- **THEN** the build system SHALL copy required assets to the `bin/assets/` folder
- **AND** SHALL ensure asset files match source files byte-for-byte
- **AND** SHALL maintain directory structure consistency

#### Scenario: Copy operations execute on every build
- **WHEN** a rebuild occurs with no source changes
- **THEN** copy operations SHALL still execute to verify outputs are up-to-date
- **AND** SHALL skip actual file copying if source and destination are identical

#### Scenario: Platform-appropriate copy commands
- **WHEN** building on Windows
- **THEN** copy operations SHALL use Windows-appropriate commands (PowerShell `Copy-Item`)
- **WHEN** building on Unix platforms (macOS, Linux)
- **THEN** copy operations SHALL use cross-platform commands (`cmake -E copy`)