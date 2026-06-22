# build-system Specification

## MODIFIED Requirements

### Requirement: Bundled font asset is copied next to the desktop binary
The build SHALL copy `apps/imgui-shell/assets/fonts/Inter-Regular.ttf` (and its `OFL.txt` license file) into an `assets/fonts/` directory adjacent to the desktop executable on every desktop preset, so the binary can locate the font at a known relative path at runtime.

#### Scenario: Font asset present in the desktop build output
-p **WHEN** a desktop preset is built
- **THEN** the path `<binary-dir>/platform/desktop/assets/fonts/Inter-Regular.ttf` SHALL exist
- **AND** the path `<binary-dir>/platform/desktop/assets/fonts/OFL.txt` SHALL exist
- **AND** the file contents SHALL match the source files byte-for-byte

#### Scenario: Font asset bundled inside the iOS .app
- **WHEN** the iOS preset is built
- **THEN** the produced `imgui_shell_ios.app` bundle SHALL contain `Resources/Inter-Regular.ttf` and `Resources/OFL.txt`

#### Scenario: Executable reliably copied to bin folder
- **WHEN** a desktop preset is built
- **THEN** the executable SHALL be copied to the root `bin/` folder
- **AND** the copy operation SHALL execute reliably on every build
,
 **AND** the copied executable SHALL be up-to-date with the built executable