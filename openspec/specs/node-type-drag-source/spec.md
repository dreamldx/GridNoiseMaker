# Node Type Drag Source Specification

## Purpose
TBD: Enable drag-and-drop creation of nodes from node type panel to graph workspace

## Requirements

### Requirement: Node type drag source enables drag-and-drop creation
The node type panel SHALL function as a drag source, allowing users to drag node types from the panel into the graph workspace to create new nodes.

#### Scenario: Node types are draggable from panel
- **WHEN** the user drags a node type from the panel
- **THEN** visual drag feedback SHALL be shown (e.g., ghost image, cursor change)
- **AND** the dragged node type SHALL be identifiable during the drag operation
- **AND** the panel SHALL indicate which node type is being dragged

#### Scenario: Drag creates nodes in graph workspace
- **WHEN** the user drops a node type into the graph workspace
- **THEN** a new node of that type SHALL be created at the drop position
- **AND** the new node SHALL have default properties for its type
- **AND** SHALL use the node type's default visual properties (color, size)

#### Scenario: Drag respects workspace coordinates
- **WHEN** a node type is dropped into the graph workspace
- **THEN** the new node's position SHALL be converted from screen coordinates to world coordinates
- **AND** SHALL use the current view transform (zoom/pan) for accurate placement
- **AND** SHALL appear at the visual drop location in the workspace

#### Scenario: Drag provides visual feedback
- **WHEN** a node type is being dragged over the graph workspace
- **THEN** visual feedback SHALL indicate valid drop targets
- **AND** SHALL show where the new node would be created
- **AND** SHALL prevent dropping in invalid areas (e.g., outside workspace)

#### Scenario: Drag works with all view modes
- **WHEN** node types are displayed in any view mode (icon, list, detail)
- **THEN** all displayed node types SHALL be draggable
- **AND** drag behavior SHALL be consistent across view modes
- **AND** drag SHALL work regardless of panel docking state (docked or floating)