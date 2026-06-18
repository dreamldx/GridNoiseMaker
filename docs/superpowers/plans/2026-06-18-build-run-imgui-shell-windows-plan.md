# Build and run imgui-shell on Windows Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`.

- [ ]`) syntax for tracking.

**Goal:** Build and run the existing imgui-shell Dear ImGui application on Windows platform using the documented CMake preset system.

**Architecture:** Use CMake preset system with Windows configuration (Visual Studio 2022 generator), build dependencies via FetchContent (ImGui, GLFW), link against Vulkan SDK, execute resulting binary.

**Tech Stack:** CMake ≥ 3.24, Visual Studio 2022, LunarG Vulkan SDK ≥ 1.2, C++17

---

## File Structure
- **Source**: `apps/imgui-shell/` (existing codebase)
- **Configuration**: `apps/imgui-shell/CMakePresets.json` (Windows preset)
- **Dependencies**: `apps/imgui-shell/cmake/Dependencies.cmake` (ImGui + GLFW)
- **Output**: `apps/imgui-shell/build/windows/platform/desktop/imgui_shell_desktop.exe`

### Task 1: Verify prerequisites

**Files:**
- Check: System environment (Vulkan SDK)
- Check: `apps/imgui-shell/CMakePresets.json` (Windows preset configuration)

- [ ] **Step 1: Verify CMake version**

```bash
cmake --version
```
Expected: Output showing CMake ≥ 3.24

- [ ] **Step 2: Verify Visual Studio 2022 availability**

```bash
where msbuild
```
Expected: Path to MSBuild.exe (e.g., `C:\Program Files\Microsoft Visual Studio\2022\...\MSBuild.exe`)

- [ ] **Step 3: Verify Vulkan SDK environment variable**

```bash
echo $env:VULKAN_SDK
```
Expected: Path to Vulkan SDK installation (e.g., `C:\VulkanSDK\1.2.xxx.x`)

- [ ] **Step 4: Verify Vulkan SDK files exist**

```bash
Test-Path -LiteralPath "$env:VULKAN_SDK\Include\vulkan\vulkan.h"
```
Expected: `True`

- [ ] **Step 5: Commit verification status**

```bash
git status
git add docs/superpowers/plans/2026-06-18-build-run-imgui-shell-windows-plan.md
git commit -m "chore: verification checklist for Windows build prerequisites"
```

### Task 2: Configure project with Windows preset

**Files:**
- Read: `apps/imgui-shell/CMakePresets.json` (preset configuration)
- Create: `apps/imgui-shell/build/windows/` (configured build directory)

-y [ ] **Step 1: Navigate to imgui-shell directory**

```bash
cd apps/imgui-shell
```

- [ ] **Step 2: Run CMake configure with Windows preset**

```bash
cmake --preset windows
```
Expected: Configuration completes successfully with output ending in `-- Configuring done` and `-- Generating done`

- [ ] **Step 3: Verify build directory created**

```bash
Test-Path -LiteralPath "build/windows/CMakeCache.txt"
```
Expected: `True`

- [ ] **Step 4: Commit configuration**

```bash
git status
git commit -m "chore: configure imgui-shell with Windows preset"
```

### Task 3: Build project

**Files:**
- Build: All source files in `apps/imgui-shell/`
- Output: `apps/imgui-shell/build/windows/platform/desktop/imgui_shell_desktop.exe`

- [ ] **Step 1: Run CMake build with Windows preset**

```bash
cmake --build --preset windows
```
Expected: Build completes successfully with output ending in `Build succeeded.`

- [ ] **Step 2: Verify executable created**

```bash
Test-Path -LiteralPath "build/windows/platform/desktop/imgui_shell_desktop.exe"
```
Expected: `True`

- [ ] **Step 3: Check executable size**

```bash
Get-Item "build/windows/platform/desktop/imgui_shell_desktop.exe" | Select-Object Length
```
Expected: File size > 100KB (actual size may vary)

- [ ] **Step 4: Commit build**

```bash
git status
git commit -m "chore: build imgui-shell for Windows"
```

### Task 4: Run application and verify

**Files:**
- Execute: `apps/imgui-shell/build/windows/platform/desktop/imgui_shell_desktop.exe`

- [ ] **Step 1: Run the executable**

```bash
cd build/windows/platform/desktop
.\imgui_shell_desktop.exe
```
Expected: Application launches with Dear ImGui window showing demo UI. Window can be closed normally.

- [ ] **Step 2: Verify UI elements present**

Visual verification:
1. Window title contains "Dear ImGui"
2. Menu bar with "File" and "Help" items visible
3. Demo window with checkboxes, buttons, sliders visible
4. "About" dialog accessible from Help menu

- [ ] **Step 3: Test clean shutdown**

Close window by clicking X button or using Alt+F4. Application should exit without crash dialog.

- [ ] **Step 4: Commit verification**

```bash
cd ..\..\..\..
git status
git commit -m "chore: verify imgui-shell runs successfully on Windows"
```

### Task 5: Final validation

**Files:**
- Review: All build artifacts and logs

- [ ] **Step 1: Verify no build warnings in logs**

Check build output for any warning messages that indicate potential issues.

- [ ] **Step 2: Check for Vulkan validation layer errors**

If running with `VK_LOADER_DEBUG=all` or similar environment variables, ensure no validation errors appear.

. [ ] **Step 3: Create summary report**

Document build time, executable size, and any observations.

- [ ] **Step 4: Final commit**

```bash
git status
git commit -m "docs: Windows build and run validation complete"
```

## Self-Review

**Spec coverage:**
- [x] Prerequisites verification ✓ (Task 1)
- [x] CMake configuration ✓ (Task 2)
- [x] Build process ✓ (Task 3)
- [x] Application execution ✓ (Task 4)
- [x] Success validation ✓ (Task 5)

**Placeholder scan:** No placeholders found. All steps contain exact commands and expected outputs.

**Type consistency:** Not applicable for this build/run plan (no code modifications).

Plan is complete and ready for execution.