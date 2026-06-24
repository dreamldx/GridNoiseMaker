# Node Graph Widget

## Purpose
TBD: Provide grid-based node graph visualization with pan/zoom capabilities for data processing workflows.

## Requirements

### Requirement: Node graph widget renders grid-based visualization
The application SHALL provide a node graph widget that displays a grid-based workspace with pan and zoom capabilities, draggable nodes, z-order management, and shares space with auxiliary panels when present.

#### Scenario: Node graph opens by default at application launch
- **WHEN** the application launches with default configuration
- **THEN** the node graph widget SHALL be visible
- **AND** SHALL occupy the available workspace area below the menu bar and to the right of any docked panels
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

### Requirement: View panning via two-finger trackpad swipe (macOS)
On macOS, the `NodeGraphWidget` canvas SHALL pan when the user performs a two-finger swipe gesture on a trackpad (or Magic Mouse), routing the gesture's scroll deltas to `ViewTransform::pan` via the existing `SimpleGridRenderer::pan` API. The gesture SHALL be active only when (a) the canvas window is hovered, (b) the Ctrl key is NOT held, and (c) the platform is macOS. On Windows and Linux, scroll events without the Ctrl modifier SHALL continue to be ignored by the canvas (the existing behavior is preserved). The Ctrl+wheel zoom path SHALL be unchanged on every platform.

#### Scenario: Two-finger swipe pans the canvas
- **WHEN** the user performs a two-finger swipe gesture inside the hovered `NodeGraphWidget` canvas on macOS, without holding Ctrl
- **THEN** the GLFW scroll callback SHALL deliver `(io.MouseWheelH, io.MouseWheel)` deltas to the per-frame `handleInput()`
- **AND** the dialog SHALL compute `delta = ImVec2(io.MouseWheelH * kTrackpadPanScalePx, io.MouseWheel * kTrackpadPanScalePx)` where `kTrackpadPanScalePx` is a file-scope constant (initially `12.0f`)
- **AND** SHALL call `m_gridRenderer->pan(delta)` exactly once per frame
- **AND** the visible canvas SHALL shift accordingly, with the direction matching the macOS system "natural scrolling" preference (GLFW honors the OS setting, no per-app override)

#### Scenario: Two-finger swipe supports both axes
- **WHEN** the user swipes diagonally (both axes contributing)
- **THEN** the pan delta SHALL include both horizontal (`io.MouseWheelH`) and vertical (`io.MouseWheel`) components in the single `m_gridRenderer->pan` call
- **AND** a pure-horizontal swipe SHALL pan horizontally with no vertical movement
- **AND** a pure-vertical swipe SHALL pan vertically with no horizontal movement

#### Scenario: Ctrl+wheel still zooms on macOS
- **WHEN** the user holds Ctrl while performing a wheel scroll or two-finger swipe on macOS
- **THEN** the new pan branch SHALL be skipped (the `!io.KeyCtrl` guard fails)
- **AND** the existing Ctrl+wheel zoom branch SHALL fire instead, calling `m_gridRenderer->zoom(io.MouseWheel, io.MousePos)`
- **AND** the canvas SHALL zoom about the cursor as before

#### Scenario: macOS-only — Windows and Linux untouched
- **WHEN** a Windows or Linux user performs the same scroll gesture without Ctrl
- **THEN** the new pan branch SHALL NOT execute (the `if constexpr (kIsMacOS)` guard excludes it at compile time on those targets)
- **AND** the canvas SHALL behave exactly as before (plain wheel without Ctrl is a no-op on the canvas)
- **AND** no `kTrackpadPanScalePx` constant SHALL be referenced by code paths reachable from non-macOS targets (it's defined unconditionally but only consumed inside the `if constexpr` branch, which the compiler dead-strips)

#### Scenario: Middle-mouse pan still works on macOS
- **WHEN** the user drags with the middle mouse button on macOS
- **THEN** the existing middle-mouse-pan branch SHALL fire (it runs before the new trackpad branch)
- **AND** the canvas SHALL pan via the same `m_gridRenderer->pan` API
- **AND** the trackpad branch SHALL NOT fire concurrently (the middle-mouse branch is independent — both branches contribute pan deltas to the same `pan()` call if both gestures somehow happen in the same frame, with no interference)

#### Scenario: Hint text reflects the gesture on macOS
- **WHEN** the on-canvas hint text is rendered on macOS
- **THEN** it SHALL read `"Pan: Middle Mouse / Two-finger swipe | Zoom: Ctrl+Mouse Wheel"` (or equivalent wording that mentions both pan affordances)
- **WHEN** the hint text is rendered on Windows or Linux
- **THEN** it SHALL continue to read `"Pan: Middle Mouse | Zoom: Ctrl+Mouse Wheel"` (the pre-change text, unchanged)

### Requirement: Node graph widget integrates with panels
The node graph widget SHALL integrate with auxiliary panels such as the node type panel, adapting its layout and maintaining functionality when panels are present.

#### Scenario: Layout adapts to docked panels
- **WHEN** a panel (e.g., node type panel) is docked alongside the node graph widget
- **THEN** the node graph workspace SHALL adjust its layout to share available space
- **AND** SHALL maintain all existing functionality (pan, zoom, node manipulation)
- **AND** SHALL preserve node positions and view state during layout adjustments

#### Scenario: Workspace maintains functionality with floating panels
- **WHEN** a panel is floating as a separate window
- **THEN** the node graph workspace SHALL occupy the full client area below the menu bar
- **AND** SHALL maintain all existing functionality
- **AND** SHALL support drag-and-drop interactions with floating panels

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
- **AND** z-order values SHALL remain unchanged

### Requirement: SimpleGridRenderer provides grid visualization
The system SHALL provide grid rendering with consistent spacing and appearance across zoom levels.

#### Scenario: Grid lines scale with zoom level
- **WHEN** the view zooms in or out
- **THEN** grid line spacing in screen pixels SHALL remain visually consistent
- **AND** grid density in world units SHALL adjust based on zoom level
- **AND** major grid lines SHALL appear at regular intervals

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
- **THEN** it SHALL include placeholder menu items labeled "Placeholder 1", "Placeholder 2"
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

### Requirement: Context menu integrates with existing input handling
The context menu SHALL integrate seamlessly with the existing node graph widget input handling system without breaking existing functionality.

#### Scenario: Context menu added to handleInput method
- **WHEN** the node graph widget processes input events
- **THEN** context menu handling SHALL be added to the existing `NodeGraphWidget::handleInput()` method
- **AND** SHALL check for right-click events within the same input processing loop
- **AND** SHALL use `ImGui::BeginPopupContextWindow()` for menu creation

#### Scenario: Existing functionality preserved
- **WHEN** the context menu is implemented
- **THEN** all existing functionality (pan, zoom, node dragging, z-order management) SHALL remain unchanged
- **AND** the "Reset View" button SHALL continue to function as before
- **AND** keyboard shortcuts SHALL not be affected

### Requirement: Node graph persistence operations
The node graph widget SHALL support persistence operations for saving and loading node state, extending its functionality to include file-based storage while maintaining all existing visualization and interaction capabilities.

#### Scenario: Node state includes persistence metadata
- **WHEN** nodes are created or modified
- **THEN** the node structure SHALL include fields for type information and properties
- **AND** SHALL support serialization of all visual properties (position, size, color, borderColor, title)
- **AND** SHALL support type-specific properties stored separately from visual properties

#### Scenario: Type-based property system
- **WHEN** the node graph widget is extended for persistence
- **THEN** it SHALL support a node type registry with default properties per type
- **AND** SHALL apply type-specific default visual properties (colors) to nodes
- **AND** SHALL maintain common visual properties for all node types

#### Scenario: JSON serialization integration
- **WHEN** persistence is implemented
- **THEN** the node graph widget SHALL integrate with JSON serialization using nlohmann/json
- **AND** SHALL serialize vector data as arrays (`[x, y]` for positions/sizes)
- **AND** SHALL serialize colors as RGBA arrays (`[r, g, b, a]`)
- **AND** SHALL serialize z-order values as integers

#### Scenario: Type validation during persistence operations
- **WHEN** the node graph widget loads nodes from JSON
- **THEN** it SHALL validate node type names against the NodeTypeRegistry
- **AND** SHALL skip nodes with unknown types
- **AND** SHALL collect statistics on skipped nodes for user feedback
- **AND** SHALL maintain existing functionality for all valid nodes
- **AND** SHALL preserve loaded z-order values
