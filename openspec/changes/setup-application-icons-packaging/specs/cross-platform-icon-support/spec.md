## ADDED Requirements

### Requirement: Consistent icon display across platforms
The application SHALL display icons consistently on Windows, macOS, and Linux.

#### Scenario: Windows title bar icon
- **WHEN** the application runs on Windows
- **THEN** the window title bar displays the application icon

#### Scenario: macOS dock and title bar icon
- **WHEN** the application runs on macOS
- **THEN** both the dock and window title bar display the application icon

#### Scenario: Linux window icon
- **WHEN** the application runs on Linux
- **THEN** the window title bar displays the application icon

### Requirement: Platform-appropriate icon formats
The build system SHALL use platform-native icon formats for best integration.

#### Scenario: Windows uses ICO format
- **WHEN** building for Windows
- **THEN** icons are provided in `.ico` format with multiple size variants

#### Scenario: macOS uses ICNS format
- **WHEN** building for macOS
- **THEN** icons are provided in `.icns` format within the app bundle

#### Scenario: Linux uses embedded PNG
- **WHEN** building for Linux
- **THEN** icons are embedded as PNG pixel data in the executable

### Requirement: GLFW-based icon setting
The application SHALL use GLFW's `glfwSetWindowIcon()` for cross-platform icon display.

#### Scenario: GLFW icon setting on all platforms
- **WHEN** the application window is created
- **THEN** `glfwSetWindowIcon()` is called with embedded icon data regardless of platform

#### Scenario: Consistent icon appearance
- **WHEN** comparing Windows, macOS, and Linux versions
- **THEN** the icon appears identical in window title bars across all platforms

### Requirement: High-DPI/retina icon support
The icon system SHALL support high-resolution displays.

#### Scenario: Retina displays on macOS
- **WHEN** running on macOS with retina display
- **THEN** high-resolution (2x) icons are used for crisp appearance

#### Scenario: High-DPI scaling on Windows
- **WHEN** running on Windows with high-DPI scaling
- **THEN** appropriate icon sizes are selected for the current DPI setting