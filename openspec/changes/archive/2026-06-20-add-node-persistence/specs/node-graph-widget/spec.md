# Node Graph Widget

## ADDED Requirements

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
- **AND** SHALL maintain backward compatibility with existing node rendering