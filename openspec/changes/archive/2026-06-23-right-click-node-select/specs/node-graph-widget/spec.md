# Node Graph Widget

## MODIFIED Requirements

### Requirement: Node graph widget provides context menu
The node graph widget SHALL provide right-click context menus accessible anywhere in the canvas area, offering canvas-specific and node-specific operations.

#### Scenario: Canvas context menu opens on right-click
- **WHEN** the user right-clicks (ImGuiMouseButton_Right) on empty canvas area within the node graph widget
- **THEN** a context menu SHALL appear at the mouse cursor position
- **AND** SHALL include canvas-specific options (Reset View, placeholder items)
- **AND** SHALL be displayed using ImGui's standard popup menu styling
- **AND** SHALL close automatically when another menu item is selected or when clicking outside the menu

#### Scenario: Node context menu opens on right-click
- **WHEN** the user right-clicks (ImGuiMouseButton_Right) on a node within the node graph widget
- **THEN** the clicked node SHALL be selected and brought to foreground (z-order 1)
- **AND** a node context menu SHALL appear at the mouse cursor position
- **AND** SHALL include node-specific options (Delete Node, Duplicate Node, Node Properties)
- **AND** SHALL be displayed using ImGui's standard popup menu styling
- **AND** SHALL close automatically when another menu item is selected or when clicking outside the menu

#### Scenario: Context menu includes Reset View option
- **WHEN** the canvas context menu is open
- **THEN** it SHALL include a "Reset View" menu item
- **AND** selecting "Reset View" SHALL reset the grid view to default zoom level (1.0) and centered position
 - **AND** SHALL have the same effect as clicking the "Reset View" button in the widget

#### Scenario: Canvas context menu includes placeholder items with dividers
- **WHEN** the canvas context menu is open
- **THEN** it SHALL include 5 placeholder menu items labeled "Menu Item 1", "Menu Item 2", ..., "Menu Item 5"
- **AND** SHALL include separator lines (`ImGui::Separator`) between groups of menu items
- **AND** placeholder items SHALL be visually distinct from functional items

#### Scenario: Node context menu includes node-specific items
- **WHEN** the node context menu is open
- **THEN** it SHALL include "Delete Node", "Duplicate Node", and "Node Properties" menu items
- **AND** SHALL include separator lines (`ImGui::Separator`) between groups of menu items

#### Scenario: Context menu respects input priority
- **WHEN** the user interacts with the node graph widget
- **THEN** right-click SHALL not interfere with existing left-click (node dragging) or middle-click (view panning) functionality
- **AND** SHALL only trigger when no other mouse operation is active