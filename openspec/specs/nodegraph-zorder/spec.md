# Node Graph Z-Order Management

## Purpose
Provides z-order management system for NodeGraphWidget with lower values drawing on top (inverse depth), automatic reordering on selection, and visual feedback.

## Requirements

### Requirement: Z-order tracking system
The system SHALL maintain z-order values for all nodes in the NodeGraphWidget, where lower z-order values draw on top (inverse depth perception). The z-order system SHALL support automatic reordering when nodes are selected and manual adjustment capabilities.

#### Scenario: Default z-order values
- **WHEN** nodes are created in the NodeGraphWidget
- **THEN** all nodes SHALL have default z-order value of 2
- **AND** UI elements SHALL have z-order value of 0
- **AND** selected node(s) SHALL have z-order value of 1

#### Scenario: Lower values draw on top
- **WHEN** nodes are rendered with different z-order values
- **THEN** nodes with lower z-order values SHALL be drawn on top of nodes with higher z-order values
- **AND** nodes with z-order 1 SHALL appear above nodes with z-order 2
- **AND** UI elements with z-order 0 SHALL appear above all nodes

#### Scenario: Z-order display at node bottom
- **WHEN** nodes are displayed in the NodeGraphWidget
- **THEN** each node SHALL display its current z-order value in the bottom area of the node
- **AND** the z-order display SHALL update in real-time as z-order values change
- **AND** the display SHALL use a small, non-intrusive text format

### Requirement: Selection-driven z-order reordering
The system SHALL automatically adjust z-order values when nodes are selected with the mouse (left-click or right-click), bringing selected nodes to the foreground.

#### Scenario: Mouse selection triggers z-order update
- **WHEN** a user selects a node by clicking it with the mouse (left-click or right-click)
- **THEN** the selected node SHALL receive z-order value of 1
- **AND** other nodes SHALL preserve their relative z-order hierarchy (nodes with original z-order less than selected node remain unchanged, nodes with original z-order greater than selected node shift down by 1)
- **AND** only one node SHALL have z-order 1 at any time (single selection)

#### Scenario: Z-order persistence during selection changes
- **WHEN** a user selects a different node
- **THEN** the newly selected node SHALL receive z-order 1
- **AND** all other nodes SHALL maintain their relative z-order hierarchy (following the same preservation rules as above)
- **AND** nodes SHALL not revert to default z-order 2 unless explicitly reset

#### Scenario: Click behavior respects z-order
- **WHEN** multiple nodes overlap at the same screen position
- **AND** the user clicks (left-click or right-click) on that position
- **THEN** the top-most visible node (lowest z-order value) SHALL be selected when clicked
- **AND** selection SHALL trigger automatic z-order reordering as described above

#### Scenario: Empty canvas click preserves z-order hierarchy
- **WHEN** a user clicks on empty canvas area (no node under cursor)
- **THEN** all nodes SHALL be deselected
- **AND** z-order values SHALL be preserved (not reset to default 2)
- **AND** no node SHALL be selected (but may have z-order value 1)

### Requirement: Z-order API for manual control
The system SHALL provide API methods for manual z-order adjustment and querying.

#### Scenario: Get current z-order value
- **WHEN** an application queries a node's z-order value
- **THEN** the system SHALL return the current z-order value for that node
- **AND** the returned value SHALL reflect any recent automatic reordering

#### Scenario: Set custom z-order value
- **WHEN** an application sets a custom z-order value for a node
- **THEN** the system SHALL update the node's z-order to the specified value
- **AND** the visual display at node bottom SHALL update to show the new value
- **AND** the rendering order SHALL immediately reflect the new z-order hierarchy