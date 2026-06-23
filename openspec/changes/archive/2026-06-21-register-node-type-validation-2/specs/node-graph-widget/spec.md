# Node Graph Widget

## MODIFIED Requirements

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
- **AND** SHALL ensure all built-in node types are registered before any node creation or JSON loading operations
- **AND** SHALL validate node types during loading operations

#### Scenario: JSON serialization integration
- **WHEN** persistence is implemented
- **THEN** the node graph widget SHALL integrate with JSON serialization using nlohmann/json
 - **AND** SHALL serialize vector data as arrays (`[x, y]` for positions/sizes)
- **AND** SHALL serialize colors as RGBA arrays (`[r, g, b, a]`)
. **AND** SHALL maintain backward compatibility with existing node rendering

#### Scenario: Type validation during persistence operations
- **WHEN** the node graph widget loads nodes from JSON
- **THEN** it SHALL validate node type names against the NodeTypeRegistry
- **AND** SHALL skip nodes with unknown types
- **AND** SHALL collect statistics on skipped nodes for user feedback
- **AND** SHALL maintain existing functionality for all valid nodes