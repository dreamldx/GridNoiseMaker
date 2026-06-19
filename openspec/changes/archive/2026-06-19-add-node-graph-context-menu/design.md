## Context

The node graph widget currently provides a grid-based visualization with three interaction methods:
1. Middle mouse drag for panning view
2. Ctrl+mouse wheel for zooming  
3. Left mouse drag for moving nodes
4. "Reset View" button at top-left corner

The widget uses ImGui for rendering and input handling, with a `SimpleGridRenderer` for grid visualization and `ViewTransform` for coordinate conversion. Adding a context menu provides alternative access to common operations without requiring the mouse to move to the top button area.

## Goals / Non-Goals

**Goals:**
- Add right-click context menu accessible anywhere in node graph canvas
- Include "Reset View" menu item duplicating existing button functionality
- Add 5 placeholder menu items separated by dividers for future expansion
- Maintain existing keyboard/mouse interactions unchanged
- Use standard ImGui menu system for consistency

**Non-Goals:**
- Context menu for individual nodes (canvas-level only)
- Keyboard shortcuts for menu items (future enhancement)
- Customizable menu items (future enhancement)
- Menu item icons (keep simple text-only)

## Decisions

**1. Menu Trigger: Right-click on empty canvas**
- **Decision**: Trigger menu on right-click (ImGuiMouseButton_Right) when window is hovered
- **Rationale**: Standard convention for context menus, avoids conflict with existing left-click (node drag) and middle-click (pan)
- **Alternative considered**: Shift+right-click or other modifier - rejected for simplicity

**2. Menu Positioning: At mouse cursor position**
- **Decision**: Open menu at current mouse cursor position using ImGui standard positioning
- **Rationale**: Provides immediate visual feedback, follows user expectation for context menus
- **Alternative considered**: Fixed position at center - rejected as less intuitive

**3. Menu Implementation: Inline ImGui::BeginPopupContextWindow**
- **Decision**: Use `ImGui::BeginPopupContextWindow()` for window-level context menu
- **Rationale**: Built-in ImGui functionality handles positioning, modality, and input focus
- **Alternative considered**: Custom popup implementation - rejected as unnecessary complexity

**4. Placeholder Items: Generic labels with dividers**
- **Decision**: Use "Menu Item 1", "Menu Item 2", etc. with `ImGui::Separator()` between groups
- **Rationale**: Clearly indicates functionality for future implementation while maintaining visual structure
- **Alternative considered**: Disabled actual functionality - rejected as could be confusing

**5. Integration: Extend handleInput() method**
- **Decision**: Add context menu handling to existing `NodeGraphWidget::handleInput()` method
- **Rationale**: Centralizes all input handling logic in one place, follows existing pattern
- **Alternative considered**: Separate method - rejected as would fragment input logic

## Risks / Trade-offs

**Risk 1**: Right-click could conflict with future node-specific context menus
- **Mitigation**: Current implementation is canvas-level only; node-specific menus would require different trigger logic (e.g., right-click on node)

**Risk 2**: Menu positioning could be outside window bounds on edges
- **Mitigation**: ImGui's `BeginPopupContextWindow` handles boundary checking automatically

**Risk 3**: Added complexity to input handling
- **Mitigation**: Simple conditional check for right-click, minimal additional code

**Trade-off**: Reset View appears in both button and menu
- **Acceptable**: Provides multiple access methods without removing existing functionality