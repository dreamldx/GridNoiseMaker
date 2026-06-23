# Node Graph Interaction Enhancements

## Purpose
Enhances mouse interaction handling in NodeGraphWidget to support z-order reordering on selection and improved click behavior.

## ADDED Requirements

### Requirement: Mouse-driven z-order interaction
The system SHALL handle mouse interactions to trigger z-order reordering when nodes are selected.

#### Scenario: Left-click selects and reorders
- **WHEN** a user left-clicks on a node in the NodeGraphWidget
- **THEN** the clicked node SHALL be selected
- **AND** the node's z-order SHALL be set to 1 (foreground)
- **AND** any previously selected node's z-order SHALL revert to its previous value (default 2)

#### Scenario: Click priority respects z-order
- **WHEN** multiple nodes overlap at the same screen position
- **THEN** the click SHALL target the top-most visible node (lowest z-order value)
- **AND** the clicked node SHALL receive z-order 1
- **AND** the visual z-order display SHALL update immediately

#### Scenario: Empty canvas click behavior
- **WHEN** a user clicks on empty canvas area (not on any node)
- **THEN** any currently selected node SHALL be deselected
- **AND** the deselected node's z-order SHALL revert to default value 2
- **AND** no node SHALL have z-order 1

### Requirement: Integration with existing input handling
The z-order interaction system SHALL integrate seamlessly with existing NodeGraphWidget input handling.

#### Scenario: Preserve existing drag functionality
- **WHEN** the z-order system is implemented
- **THEN** existing node dragging functionality SHALL remain unchanged
- **AND** nodes SHALL continue to be draggable via left-click and drag
- **AND** drag operations SHALL not interfere with z-order updates

#### Scenario: Maintain pan/zoom functionality
- **WHEN** the z-order system is implemented
- **THEN** middle-click panning and Ctrl+wheel zooming SHALL continue to function as before
- **AND** view transformations SHALL not affect z-order calculations

#### Scenario: Context menu compatibility
- **WHEN** the z-order system is implemented
- **THEN** right-click context menu functionality SHALL remain available
- **AND** context menu operations SHALL not interfere with z-order state