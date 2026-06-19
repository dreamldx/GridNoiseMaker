# font-diagnostics Specification

## Purpose
TBD - created by syncing from change fix-font-rendering-issue. Update Purpose after archive.

## Requirements
### Requirement: Runtime font diagnostics overlay
The system SHALL provide an optional diagnostic overlay that displays real-time font rendering information when enabled via keyboard shortcut or configuration.

#### Scenario: Toggle diagnostic overlay with keyboard shortcut
. **WHEN** the user presses Ctrl+Shift+D (desktop) or three-finger tap (iOS)
- **THEN** the font diagnostic overlay SHALL toggle visibility
- **AND** SHALL display current font information including: font file name, pixel size, rasterizer type, atlas dimensions, glyph count
- **AND** SHALL update in real-time each frame

#### Scenario: Diagnostic overlay font rendering metrics
- **WHEN** the font diagnostic overlay is visible
- **THEN** it SHALL display metrics including: font loading time, atlas rebuild count, current glyph count, texture memory usage
- **AND** SHALL highlight any validation warnings or errors in red
- **AND** SHALL show platform-specific font path information

#### Scenario: Per-glyph diagnostic visualization
- **WHEN** the diagnostic overlay is in detailed mode (activated by clicking)
- **THEN** it SHALL display a grid of rendered glyphs with their Unicode codepoints
- **AND** SHALL highlight missing glyphs with red borders
.- **AND** SHALL show glyph bounding boxes and advance widths
- **AND** SHALL allow hovering to see detailed glyph metrics

### Requirement: Font error logging and persistence
The system SHALL log all font-related errors and warnings to both console and persistent log files with sufficient detail for debugging.

#### Scenario: Comprehensive font error logging
- **WHEN** any font loading, validation, or rendering error occurs
- **THEN** the system SHALL log the error with timestamp, error type, file path, platform, and technical details
- **AND** SHALL preserve the log across application restarts in `~/.config/imgui-shell/logs/font-errors.log`
- **AND** SHALL include stack trace or call chain information where available

#### Scenario: Font warning logging for non-critical issues
- **WHEN** a font operation succeeds but with warnings (e.g., missing glyphs, suboptimal hinting)
- **THEN** the system SHALL log the warning with clear distinction from errors
- **AND** SHALL include in the persistent log file
- **AND** SHALL be visible in diagnostic overlay when warnings mode is enabled

#### Scenario: Cross-platform log consistency
- **WHEN** font errors are logged on different platforms
- **THEN** the log format SHALL be identical across platforms
- **AND** platform-specific details SHALL be included in a standardized way
- **AND** log file location SHALL follow platform conventions (Windows: `%APPDATA%`, macOS/Linux: `~/.config`, iOS: app sandbox)

### Requirement: Font diagnostic API for programmatic access
The system SHALL expose a public API for querying font rendering state and diagnostics programmatically.

#### Scenario: Query current font information via API
- **WHEN** an external component calls `app::getCurrentFontInfo()`
- **THEN** the API SHALL return a structure containing: font file path, pixel size, rasterizer type, validation status, error count, warning count
- **AND** SHALL be callable from any thread (with appropriate synchronization)
- **AND** SHALL not trigger font atlas rebuild or other side effects

#### Scenario: Query font validation history via API
- **WHEN** an external component calls `app::getFontValidationHistory()`
- **THEN** the API SHALL return a list of recent font validation events with timestamps and outcomes
- **AND** SHALL include both successes and failures
- **AND** SHALL be limited to the last 100 events for memory efficiency

#### Scenario: Programmatic diagnostic overlay control
- **WHEN** an external component calls `app::setDiagnosticOverlayVisible(bool)`
- **THEN** the diagnostic overlay visibility SHALL be controlled programmatically
- **AND** SHALL respect the same keyboard shortcut toggle behavior
- **AND** SHALL not interfere with user-initiated toggles