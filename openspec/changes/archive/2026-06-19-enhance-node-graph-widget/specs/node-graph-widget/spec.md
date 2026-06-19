## ADDED Requirements

### Requirement: Node graph widget renders grid-based visualization
The application SHALL provide a node graph widget that displays a grid-based workspace with pan and zoom capabilities, draggable nodes, and fills the available viewport area at startup.

#### Scenario: Node graph opens by default at application launch
- **WHEN** the application launches with default configuration
- **THEN** the node graph widget SHALL be visible
- **AND** SHALL fill the entire client area below the menu bar
- **AND** SHALL display a grid background with three test nodes (Input, Process, Output)

#### Scenario: View panning via middle mouse drag
- **WHEN** the user holds the middle mouse button and drags within the node graph widget
- **THEN** the view SHALL pan in the direction of mouse movement
- **AND** the grid and nodes SHALL maintain their relative positions during panning

#### Scenario: View zooming via Ctrl+Mouse wheel
- **WHEN** the user holds the Ctrl key and scrolls the mouse wheel within the node graph widget
- **THEN** the view SHALL zoom in or out centered at the mouse cursor position
- **AND** SHALL respect zoom constraints (MIN_ZOOM = 0.01, MAX_ZOOM = 100)
- **AND** grid lines SHALL scale proportionally with zoom level

#### Scenario: Node dragging via left mouse button
- **WHEN** the user clicks and drags a node with the left mouse button
- **THEN** the node SHALL move in world space following mouse movement
- **AND** SHALL convert screen delta to world delta using current zoom level
- **AND** SHALL allow only one node to be dragged at a time
- **AND** top-most (last drawn) node under cursor wins selection

#### Scenario: Reset view functionality
- **WHEN** the user clicks the "Reset View" button in the node graph widget
- **THEN** the view SHALL return to default zoom level (1.0) and centered position
- **AND** grid lines SHALL reset to default spacing

### Requirement: ViewTransform provides coordinate system conversion
The system SHALL provide a ViewTransform class that maps between world space and screen space with uniform scaling (zoom) and translation.

#### Scenario: World-to-screen coordinate conversion
- **WHEN** a world position is converted to screen coordinates via `worldToScreen()`
- **THEN** the returned screen position SHALL account for current zoom level and view offset
- **AND** SHALL consider viewport position and size

#### Scenario: Screen-to-world coordinate conversion
- **WHEN** a screen position is converted to world coordinates via `screenToWorld()`
- **THEN** the returned world position SHALL invert the zoom and translation applied by `worldToScreen()`
- **AND** SHALL maintain mathematical consistency with `worldToScreen()`

#### Scenario: Coordinate stability during window movement
- **WHEN** the main application window is moved while node graph is visible
- **THEN** node positions SHALL remain stable in world coordinates
- **AND** screen coordinates SHALL update correctly based on new viewport position

### Requirement: SimpleGridRenderer provides grid visualization
The system SHALL provide grid rendering with consistent spacing and appearance across zoom levels.

#### Scenario: Grid lines scale with zoom level
- **WHEN** the view zooms in or out
- **THEN** grid line spacing in screen pixels SHALL remain visually consistent
- **AND** grid density in world units SHALL adjust based on zoom level
- **AND** major grid lines SHALL appear at regular intervals

#### Scenario: Grid renders within canvas bounds
- **WHEN** the node graph widget draws its grid
- **THEN** grid lines SHALL be clipped to the canvas area
- **AND** SHALL not extend beyond the widget boundaries
- **AND** SHALL use appropriate colors for visibility against background