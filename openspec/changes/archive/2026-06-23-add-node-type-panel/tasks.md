# Implementation Tasks

## Phase 1: Core Infrastructure
1. **Enable ImGui docking support**
   - Modify `app::init()` to set ImGui docking flags
   - Configure ImGuiIO for docking and viewports
   - Test basic docking functionality

2. **Create panel preferences system**
   - Add panel state struct (visibility, docked, floating, view mode, size)
   - Add save/load functions for panel preferences
   - Integrate with existing preference system

## Phase 2: Node Type Panel Implementation
3. **Implement NodeTypePanel class**
   - Create `NodeTypePanel.h` and `NodeTypePanel.cpp`
   - Add constructor/destructor and basic initialization
   - Implement `render()` method for panel UI

4. **Add panel view modes**
   - Implement icon view mode (compact grid layout)
   - Implement list view mode (vertical list with names)
   - Implement detail view mode (expanded information view)
   - Add view mode switching logic

5. **Add panel header controls**
   - Add view mode toggle buttons (icon/list/detail)
   - Add docking controls (dock left/dock right/float)
   - Add panel close/minimize button
   - Style header appropriately

## Phase 3: Docking Integration
6. **Integrate panel with ImGui docking**
   - Use `ImGui::Begin()` with docking flags
   - Implement docking state management
   - Handle panel floating as separate window
   - Save/restore docking state

7. **Update NodeGraphWidget layout**
   - Modify `NodeGraphWidget::render()` to share space
   - Calculate available space based on panel state
   - Handle panel resizing and layout changes
   - Ensure graph functionality remains intact

## Phase 4: Drag-and-Drop Support
8. **Implement node type drag source**
   - Make node types draggable from panel
   - Create drag payload with node type information
   - Add visual drag feedback (ghost image, cursor)

9. **Implement node creation from drag**
   - Handle drop events in NodeGraphWidget
   - Convert screen coordinates to world coordinates
   - Create new nodes at drop position
   - Use NodeTypeRegistry for node creation

## Phase 5: Integration and Testing
10. **Integrate panel with NodeGraphWidget**
    - Add panel as member variable of NodeGraphWidget
    - Connect panel state to widget preferences
    - Ensure proper initialization order

11. **Test docking scenarios**
    - Test docking left/right
    - Test floating window functionality
    - Test panel resizing and persistence
    - Test with multiple panels (future extensibility)

12. **Test view mode switching**
    - Test icon/list/detail view modes
    - Verify visual fidelity in each mode
    - Test drag-and-drop from all view modes

13. **Test preferences persistence**
    - Test panel state persistence across sessions
    - Test docking position persistence
    - Test view mode persistence
    - Test panel size persistence

## Phase 6: Polish and Refinement
14. **Add keyboard shortcuts**
    - Add shortcuts for panel visibility toggle
    - Add shortcuts for view mode switching
    - Add shortcuts for docking controls

15. **Style and visual polish**
    - Ensure consistent styling with existing UI
    - Add appropriate spacing and margins
    - Test on different DPI displays
    - Verify accessibility (text contrast, sizing)

16. **Performance optimization**
    - Optimize panel rendering for many node types
    - Implement lazy loading for detailed views
    - Optimize drag-and-drop performance
    - Profile memory usage

## Phase 7: Documentation
17. **Update user documentation**
    - Document panel features and controls
    - Document keyboard shortcuts
    - Document preferences and persistence

18. **Update API documentation**
    - Document NodeTypePanel public API
    - Document integration points
    - Document extension points for future panels

## Critical Dependencies
- Must maintain backward compatibility with existing node creation
- Must not break existing NodeGraphWidget functionality
- Must work with all existing node types in NodeTypeRegistry
- Must integrate with existing preference system
- Must work on all supported platforms (Windows, macOS, Linux, iOS)

## Verification Checklist
- [ ] Panel displays all registered node types correctly
- [ ] All three view modes work and switch properly
- [ ] Panel docks correctly to left and right sides
- [ ] Panel floats as separate window
- [ ] Panel state persists across sessions
- [ ] Drag-and-drop creates nodes at correct position
- [ ] NodeGraphWidget adapts layout correctly
- [ ] Performance is acceptable with many node types
- [ ] UI is consistent with existing styling
- [ ] All existing tests pass
- [ ] Build succeeds on all platforms