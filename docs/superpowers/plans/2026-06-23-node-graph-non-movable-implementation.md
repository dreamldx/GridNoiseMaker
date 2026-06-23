# Node Graph Widget Non-Movable Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `ImGuiWindowFlags_NoMove` flag to the Node Graph canvas window to make it non-movable while preserving docking functionality.

**Architecture:** Single-line modification to `NodeGraphWidget::renderGraphCanvas()` to add the `NoMove` flag to existing window flags.

**Tech Stack:** C++, ImGui, nlohmann/json

---

### Task 1: Modify Node Graph Widget Window Flags

**Files:**
- Modify: `apps/imgui-shell/app/NodeGraphWidget.cpp:283-291`

**Current Code:**
```cpp
ImGuiWindowFlags window_flags = 
    ImGuiWindowFlags_NoTitleBar | 
    // Remove NoResize and NoMove to allow docking
    ImGuiWindowFlags_NoScrollbar | 
    ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus;
```

- [ ] **Step 1: Read the current file to verify context**

```bash
cd apps/imgui-shell
grep -n "renderGraphCanvas" app/NodeGraphWidget.cpp
grep -n "window_flags" app/NodeGraphWidget.cpp -A 3 -B 3
```

- [ ] **Step 2: Modify the window flags to add NoMove**

Edit `apps/imgui-shell/app/NodeGraphWidget.cpp:283-291`:

```cpp
ImGuiWindowFlags window_flags = 
    ImGuiWindowFlags_NoTitleBar | 
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar | 
    ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus;
```

- [ ] **Step 3: Verify the modification**

```bash
cd apps/imgui-shell
grep -n "ImGuiWindowFlags_NoMove" app/NodeGraphWidget.cpp -B 2 -A 2
```

Expected output:
```
282:    ImGuiWindowFlags window_flags = 
283:        ImGuiWindowFlags_NoTitleBar | 
284:        ImGuiWindowFlags_NoMove |
285:        ImGuiWindowFlags_NoScrollbar | 
```

- [ ] **Step 4: Build the application**

```bash
cd apps/imgui-shell
cmake --build --preset windows
```

Expected: Build succeeds with no errors

- [ ] **Step 5: Commit changes**

```bash
git add apps/imgui-shell/app/NodeGraphWidget.cpp
git commit -m "fix(node-graph): make Node Graph window non-movable by adding NoMove flag"
```

---

### Task 2: Verification and Documentation

**Files:**
- Modify: `docs/superpowers/specs/2026-06-23-node-graph-non-movable-design.md`

- [ ] **Step 1: Update documentation to reflect completion**

Edit `docs/superpowers/specs/2026-06-23-node-graph-non-movable-design.md`:

Add at the end of the file:

```markdown
## Implementation Status
✅ **Implemented:** `ImGuiWindowFlags_NoMove` flag added to `renderGraphCanvas()` window flags.

**Date:** 2026-06-23
**Commit:** [Commit hash will be added here]
```

- [ ] **Step 2: Run basic verification**

```bash
cd apps/imgui-shell
./build/windows/bin/imgui_shell.exe
```

Expected: Application launches successfully (close after verification)

- [ ] **Step 3: Commit documentation update**

```bash
git add docs/superpowers/specs/2026-06-23-node-graph-non-movable-design.md
git commit -m "docs(node-graph): update design doc with implementation status"
```

- [ ] **Step 4: Final status check**

```bash
git status
git log --oneline -3
```

Expected: Clean working tree, last 3 commits include both implementation and documentation commits.