# Node Graph Persistence

## MODIFIED Requirements

### Requirement: Node graph state persistence
The system SHALL persist node graph state to JSON files and restore it from saved files, including node positions, visual properties, type information, and type-specific properties.

#### Scenario: Save node graph to JSON file
, - **WHEN** the user selects "Save Node Graph" from the File menu
- **THEN** the system SHALL present a native operating system file save dialog using platform-appropriate APIs
- **AND** SHALL save all node information to the selected JSON file
- **AND** SHALL include node positions, sizes, colors, titles, types, and type-specific properties
- **AND** SHALL use the simple flat JSON structure defined in the design document
- **AND** SHALL include a version field for future compatibility
- **AND** SHALL fall back to ImGui modal text input if native dialog unavailable

#### Scenario: Load node graph from JSON file
- **WHEN** the user selects "Load Node Graph" from the File menu
- **THEN** the system SHALL present a native operating system file open dialog using platform-appropriate APIs
- **AND** SHALL load node information from the selected JSON file
- **AND** SHALL restore node positions, sizes, colors, titles, types, and type-specific properties for nodes with registered types
- **AND** SHALL validate node type names against the NodeTypeRegistry
-

**AND** SHALL skip nodes with unknown type names and report them to the user
- **AND** SHALL handle missing or invalid fields with appropriate defaults
- **AND** SHALL validate the JSON structure and version compatibility
- **AND** SHALL fall back to ImGui modal text input if native dialog unavailable

#### Scenario: Auto-load default project at startup
- **WHEN** the application starts
- **THEN** the system SHALL automatically load the default node graph project
- **AND** SHALL look for `assets/default_node_graph.json` as the default project file
- **AND** SHALL create default nodes if the default project file is not found
- **AND** SHALL handle JSON parsing errors gracefully without crashing

#### Scenario: Node type system with type-based properties
- **WHEN** nodes are saved or loaded
- **THEN** the system SHALL preserve node type information
- **AND** SHALL store type-specific properties in the `properties` field
- **AND** SHALL apply type-specific default visual properties (colors)
- **AND** SHALL support different property schemas for different node types

#### Scenario: Common visual properties persistence
- **WHEN** nodes are saved or loaded
- **THEN** the system SHALL persist common visual properties (position, size, color, borderColor, title)
- **AND** SHALL serialize ImVec2 positions/sizes as arrays `[x, y]`
- **AND** SHALL serialize ImU32 colors as RGBA arrays `[r, g, b, a]`
- **AND** SHALL handle string titles with proper encoding

#### Scenario: Error handling for file operations
- **WHEN** a save operation fails (permissions, disk full, etc.)
- **THEN** the system SHALL display an error message to the user
- **AND** SHALL not lose the current node graph state
.

- **WHEN** a load operation fails (file not found, invalid JSON, etc.)
- **THEN** the system SHALL display an error message to the user
- **AND** SHALL maintain the existing node graph state or load defaults

#### Scenario: Unknown type handling during loading
- **WHEN** a JSON file contains nodes with type names not registered in NodeTypeRegistry
- **THEN** the system SHALL skip those nodes during loading
- **AND** SHALL display a dialog message listing all skipped node types and count
- **AND** SHALL continue loading all valid nodes with registered types
- **AND** SHALL not crash or reject the entire file due to unknown types