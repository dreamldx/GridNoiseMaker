# Node Graph Widget Headerless and Non-Movable Design

## Overview
Make the Node Graph canvas window headerless and non-movable while preserving docking functionality.

## Current State Analysis
The Node Graph widget has two main windows:
1. **Main dock space window**: Created in `NodeGraphWidget::render()` with flags `ImGuiWindowFlags_NoTitleBar` and `ImGuiWindowFlags_NoMove`
2. **Node Graph canvas window**: Created in `NodeGraphWidget::renderGraphCanvas()` with flag `ImGuiWindowFlags_NoTitleBar` but missing `ImGuiWindowFlags_NoMove`

The canvas window currently:
- Has no title bar (header already hidden)
- Can be moved within the dock space
- Can be docked/undocked via docking system

## Requirements
- Hide header: Already satisfied with `ImGuiWindowFlags_NoTitleBar`
- Make non-movable: Add `ImGuiWindowFlags_NoMove` flag

## Design
**File:** `apps/imgui-shell/app/NodeGraphWidget.cpp`
**Function:** `renderGraphCanvas()`
**Location:** Lines 283-291 (window flags definition)

### Current Code:
```cpp
ImGuiWindowFlags window_flags = 
    ImGuiWindowFlags_NoTitleBar | 
    // Remove NoResize and NoMove to allow docking
    ImGuiWindowFlags_NoScrollbar | 
    ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus;
```

### Modified Code:
```cpp
ImGuiWindowFlags window_flags = 
    ImGuiWindowFlags_NoTitleBar | 
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar | 
    ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus;
```

## Implementation Details
1. **Scope:** Single line change - add `ImGuiWindowFlags_NoMove` flag
2. **Compatibility:** Maintains all existing docking functionality
3. **Behavior:** Window cannot be moved as a floating window within dock space, but can still be docked/undocked via docking system

## Verification
- Build should succeed with `cd apps/imgui-shell && cmake --build --preset windows`
- Application should run without errors
- Node Graph window should be fixed in position but remain dockable

## Notes
- The header is already hidden via `NoTitleBar` flag
- The `NoMove` flag only affects floating window movement within dock space
- Docking functionality is preserved via the dock space system

## Implementation Status
✅ **Implemented:** `ImGuiWindowFlags_NoMove` flag added to `renderGraphCanvas()` window flags.

**Date:** 2026-06-23
**Commit:** 7bc7e16 (fix(node-graph): make Node Graph window non-movable by adding NoMove flag)