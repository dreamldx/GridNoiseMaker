# Node Graph Interaction Enhancements

## Purpose
Enhances mouse interaction handling in NodeGraphWidget to support z-order reordering on selection and improved click behavior.

## Requirements

### Requirement: Mouse-driven z-order interaction
The system SHALL handle mouse interactions (left-click and right-click) to trigger hierarchical z-order reordering when nodes are selected, preserving relative z-order relationships among nodes.

#### Scenario: Left-click selects and reorders
- **WHEN** a user left-clicks on a node in the NodeGraphWidget
- **THEN** the clicked node SHALL be selected
- **AND** the node's z-order SHALL be set to 1 (foreground)
- **AND** other nodes SHALL preserve their relative z-order hierarchy (nodes with original z-order less than selected node remain unchanged, nodes with original z-order greater than selected node shift down by 1)

#### Scenario: Right-click selects and opens menu
- **WHEN** a user right-clicks on a node in the NodeGraphWidget
- **THEN** the clicked node SHALL be selected
- **AND** the node's z-order SHALL be set to 1 (foreground)
- **AND** other nodes SHALL preserve their relative z-order hierarchy (nodes with original z-order less than selected node remain unchanged, nodes with original z-order greater than selected node shift down by 1)
- **AND** a context menu SHALL appear at the mouse cursor position with node-specific options (Delete Node, Duplicate Node, Node Properties)

#### Scenario: Click priority respects z-order
- **WHEN** multiple nodes overlap at the same screen position
- **AND** the user clicks (left-click or right-click) on that position
- **THEN** the click SHALL target the top-most visible node (lowest z-order value)
- **AND** the clicked node SHALL receive z-order 1
- **AND** hierarchical z-order preservation SHALL be applied to other nodes
- **AND** the visual z-order display SHALL update immediately

#### Scenario: Empty canvas click behavior
- **WHEN** a user clicks on empty canvas area (not on any node)
- **THEN** any currently selected node SHALL be deselected
- **AND** z-order values SHALL be preserved (hierarchy maintained)
- **AND** no node SHALL be selected (but may retain z-order value 1)

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