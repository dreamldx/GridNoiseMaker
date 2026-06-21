## ADDED Requirements

### Requirement: Embedded application icons
The build system SHALL embed application icon data directly into executables rather than relying on external icon files.

#### Scenario: Windows executable contains embedded icon resource
- **WHEN** the Windows version of imgui-shell is built
- **THEN** the PE executable contains an icon resource section with the application icon

#### Scenario: Icons work without external files
- **WHEN** the application is run from any location (install directory, portable drive, etc.)
- **THEN** the window icon appears in the title bar without requiring external icon files

#### Scenario: Multiple icon sizes embedded
- **WHEN** the application icon is embedded
- **THEN** multiple standard sizes (16x16, 32x32, 64x64) are included for proper display at different resolutions

### Requirement: Icon source file management
The build system SHALL convert source icon files to platform-specific formats during compilation.

#### Scenario: ICO file compilation on Windows
- **WHEN** building for Windows
- **THEN** the source `.ico` file is compiled into the executable using `windres` or equivalent resource compiler

#### Scenario: PNG to C array conversion for Linux
- **WHEN** building for Linux
- **THEN** source `.png` icon files are converted to C arrays embedded in the executable

#### Scenario: ICNS file generation for macOS
- **WHEN** building for macOS
- **THEN** source icon files are converted to `.icns` format and included in the app bundle

### Requirement: Build-time icon validation
The build system SHALL validate icon files during compilation to catch format issues early.

#### Scenario: Invalid icon format detection
- **WHEN** an invalid or malformed icon file is provided
- **THEN** the build fails with a descriptive error message

#### Scenario: Missing icon file detection
- **WHEN** the required icon source file is missing
- **THEN** the build fails with instructions to provide the icon file