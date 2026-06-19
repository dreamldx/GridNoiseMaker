## Why

Font rendering issues have been reported where fonts fail to load or render correctly across different platforms and configurations. The current font-rendering system has edge cases where font loading can fail silently or produce poor visual quality, affecting user experience and application reliability. This needs to be addressed now because font rendering is a fundamental UI component that directly impacts usability.

## What Changes

- **Add comprehensive font loading error handling**: Improve error reporting and fallback mechanisms when font files are missing or corrupted
- **Enhance font validation**: Add validation for font file integrity and compatibility before attempting to load
- **Improve font atlas rebuild reliability**: Fix race conditions and edge cases in font atlas rebuild during runtime font changes
- **Add font rendering diagnostics**: Provide better debugging information for font-related issues in logs and UI
- **Standardize font loading across platforms**: Ensure consistent font loading behavior between desktop (Vulkan/GLFW) and iOS (Metal/UIKit) targets

## Capabilities

### New Capabilities
- `font-validation`: Comprehensive font file validation and error reporting
- `font-diagnostics`: Runtime font rendering diagnostics and debugging tools

### Modified Capabilities
- `font-rendering`: Enhanced error handling, improved font atlas rebuild reliability, and standardized cross-platform behavior

## Impact

- **Code**: Changes to `app/App.cpp`, `app/App.h`, `app/Theme.cpp`, `app/ThemeStorage.cpp`, `app/Preferences.cpp`, and platform-specific files
- **APIs**: No breaking API changes; enhanced error reporting in existing functions
- **Dependencies**: No new external dependencies required
- **Systems**: Improved font rendering reliability affects all UI components across all supported platforms