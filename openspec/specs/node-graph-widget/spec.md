# Node Graph Widget

## Purpose
TBD: Provide grid-based node graph visualization with pan/zoom capabilities for data processing workflows.

## Requirements

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

### Requirement: Node graph widget provides context menu
The node graph widget SHALL provide a right-click context menu accessible anywhere in the canvas area, offering common operations and placeholder items for future functionality.

#### Scenario: Context menu opens on right-click
- **WHEN** the user right-clicks (ImGuiMouseButton_Right) anywhere within the node graph widget canvas area
- **THEN** a context menu SHALL appear at the mouse cursor position
- **AND** SHALL be displayed using ImGui's standard popup menu styling
- **AND** SHALL close automatically when another menu item is selected or when clicking outside the menu

#### Scenario: Context menu includes Reset View option
- **WHEN** the context menu is open
- **THEN** it SHALL include a "Reset View" menu item
- **AND** selecting "Reset View" SHALL reset the grid view to default zoom level (1.0) and centered position
- **AND** SHALL have the same effect as clicking the "Reset View" button in the widget

#### Scenario: Context menu includes placeholder items with dividers
- **WHEN** the context menu is open
- **THEN** it SHALL include 5 placeholder menu items labeled "Menu Item 1", "Menu Item 2", ..., "Menu Item 5"
- **AND** SHALL include separator lines (`ImGui::Separator`) between groups of menu items
- **AND** placeholder items SHALL be visually distinct from functional items

#### Scenario: Context menu respects input priority
- **WHEN** the user interacts with the node graph widget
- **THEN** right-click SHALL not interfere with existing left-click (node dragging) or middle-click (view panning) functionality
- **AND** SHALL only trigger when no other mouse operation is active

### Requirement: Context menu integrates with existing input handling
The context menu SHALL integrate seamlessly with the existing node graph widget input handling system without breaking existing functionality.

#### Scenario: Context menu added to handleInput method
- **WHEN** the node graph widget processes input events
- **THEN** context menu handling SHALL be added to the existing `NodeGraphWidget::handleInput()` method
- **AND** SHALL check for right-click events within the same input processing loop
- **AND** SHALL use `ImGui::BeginPopupContextWindow()` for menu creation

#### Scenario: Existing functionality preserved
- **WHEN** the context menu is implemented
- **THEN** all existing functionality (pan, zoom, node dragging) SHALL remain unchanged
- **AND** the "Reset View" button SHALL continue to function as before
- **AND** keyboard shortcuts SHALL not be affected
