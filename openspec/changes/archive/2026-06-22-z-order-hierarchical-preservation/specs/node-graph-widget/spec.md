# Node Graph Widget

## MODIFIED Requirements

### Requirement: Node graph widget renders grid-based visualization
The application SHALL provide a node graph widget that displays a grid-based workspace with pan and zoom capabilities, draggable nodes, hierarchical z-order management, and fills the available viewport area at startup.

#### Scenario: Node graph opens by default at application launch
- **WHEN** the application launches with default configuration
- **THEN** the node graph widget SHALL be visible
- **AND** SHALL fill the entire client area below the menu bar
- **AND** SHALL display a grid background with three test nodes (Input, Process, Output)
- **AND** all nodes SHALL have initial z-order value of 2
- **AND** UI elements SHALL have z-order value of 0

#### Scenario: View panning via middle mouse drag
- **WHEN** the user holds the middle mouse button and drags within the node graph widget
- **THEN** the view SHALL pan in the direction of mouse movement
- **AND** the grid and nodes SHALL maintain their relative positions during panning
- **AND** z-order values SHALL remain unchanged during panning

#### Scenario: View zooming via Ctrl+Mouse wheel
- **WHEN** the user holds the Ctrl key and scrolls the mouse wheel within the node graph widget
- **THEN** the view SHALL zoom in or out centered at the mouse cursor position
- **AND** SHALL respect zoom constraints (MIN_ZOOM = 0.01, MAX_ZOOM = 100)
- **AND** grid lines SHALL scale proportionally with zoom level
- **AND** z-order values SHALL remain unchanged during zooming

#### Scenario: Node dragging via left mouse button
- **WHEN** the user clicks and drags a node with the left mouse button
- **THEN** the node SHALL move in world space following mouse movement
- **AND** SHALL convert screen delta to world delta using current zoom level
- **AND** SHALL allow only one node to be dragged at a time
- **AND** top-most (lowest z-order) node under cursor wins selection
- **AND** the dragged node SHALL maintain its z-order value during dragging

#### Scenario: Reset view functionality
- **WHEN** the user clicks the "Reset View" button in the node graph widget
- **THEN** the view SHALL return to default zoom level (1.0) and centered position
- **AND** grid lines SHALL reset to default spacing
- **AND** z-order values SHALL remain unchanged

#### Scenario: Node selection updates z-order
- **WHEN** a user selects a node by clicking it
- **THEN** the selected node SHALL receive z-order value of 1
- **AND** the node's z-order value SHALL be displayed at the bottom of the node
- **AND** other nodes SHALL preserve their relative z-order hierarchy (nodes with original z-order less than selected node remain unchanged, nodes with original z-order greater than selected node shift down by 1)