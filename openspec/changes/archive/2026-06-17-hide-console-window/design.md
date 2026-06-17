## Context

Currently, the Windows desktop executable (`imgui_shell_desktop.exe`) is built as a console application. This causes a console window to appear alongside the GUI window when launched by double-clicking or from Windows Explorer. The console window is useful for developers to see debug output but creates a poor user experience for end-users.

The CMake configuration for Windows uses the Visual Studio 17 2022 generator with default settings, which creates a console application. The application uses GLFW for window management and Vulkan for rendering.

## Goals / Non-Goals

**Goals:**
1. Hide the console window when launching the Windows desktop application
2. Maintain ability to see debug output when needed (e.g., when launched from command line)
3. Only affect Windows platform; macOS and Linux behavior unchanged
4. Preserve existing build workflow and CMake presets

**Non-Goals:**
1. No changes to application GUI or rendering code
2. No platform abstraction layer for console handling
3. No changes to macOS or Linux build configurations
4. No complex runtime subsystem switching

## Decisions

### 1. Use Windows Subsystem Configuration
**Decision:** Configure the Windows executable to use `/SUBSYSTEM:WINDOWS` instead of `/SUBSYSTEM:CONSOLE`
**Rationale:** This is the standard Windows way to create GUI applications without a console window. It's a linker flag that can be set in CMake.
**Alternative considered:** Using `WinMain` entry point - would require significant code changes and doesn't solve the problem by itself.

### 2. CMake Target Property Configuration
**Decision:** Set `WIN32_EXECUTABLE` property on the desktop target when building for Windows
**Rationale:** CMake's `WIN32_EXECUTABLE` property automatically sets the correct linker flags for Windows GUI applications. It's platform-specific and only affects Windows builds.
**Alternative considered:** Manual linker flag configuration - more error-prone and harder to maintain.

### 3. Preserve Debug Output Capability
**Decision:** Keep application capable of outputting to stdout/stderr for debugging purposes
**Rationale:** Developers still need to see debug output. The application can still write to stdout/stderr; Windows will discard it unless launched from an existing console.
**Alternative considered:** Remove all stdout/stderr usage - would lose valuable debugging capability.

### 4. Separate Debug/Release Configuration
**Decision:** Only apply subsystem change to Release builds; keep Console subsystem for Debug builds
**Rationale:** Developers benefit from seeing console output during debugging. End-users only use Release builds.
**Alternative considered:** Apply to all builds - would make debugging harder.

## Risks / Trade-offs

**Risk:** Debug output lost when double-clicking executable → **Mitigation:** Add optional debug mode flag or log file output

**Risk:** Visual Studio debugger attachment may be affected → **Mitigation:** Test debugging workflow; ensure Debug builds keep console subsystem

**Risk:** Application may still allocate console if it calls console APIs → **Mitigation:** Audit code for console API usage; none found in current codebase

**Trade-off:** Simplicity vs. flexibility - simple subsystem change vs. complex runtime switching
