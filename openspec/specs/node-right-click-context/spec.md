# Node Right-click Context Menu

## Purpose
Provides node-specific context menu functionality via right-click, with proper z-order selection and hit testing.

## Requirements

### Requirement: Node right-click context menu
The system SHALL provide node-specific context menu accessible via right-click on nodes, with the menu appearing at the mouse cursor position and including node-specific operations.

#### Scenario: Right-click selects node and opens menu
- **WHEN** a user right-clicks on a node in the NodeGraphWidget
- **THEN** the clicked node SHALL be selected (z-order set to 1, hierarchical preservation applied)
- **AND** a context menu SHALL appear at the mouse cursor position
- **AND** the menu SHALL include node-specific options (Delete Node, Duplicate Node, Node Properties)

#### Scenario: Right-click hit testing uses top-most node
- **WHEN** multiple nodes overlap at the same screen position
- **AND** the user right-clicks on that position
- **THEN** the click SHALL target the top-most visible node (lowest z-order value)
- **AND** that node SHALL be selected and receive its context menu

#### Scenario: Canvas context menu remains available
- **WHEN** a user right-clicks on empty canvas area (not on any node)
- **THEN** the canvas context menu SHALL appear at the mouse cursor position
- **AND** SHALL include canvas-specific options (Reset View, placeholder items)
- **AND** SHALL not trigger node selection or z-order changes

#### Scenario: Node context menu priority
- **WHEN** a user right-clicks on a node
- **THEN** only the node context menu SHALL appear (canvas context menu SHALL not appear)
- **AND** SHALL use ImGui's standard popup menu styling