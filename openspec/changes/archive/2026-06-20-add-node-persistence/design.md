## Context

The current node graph widget has a simple `Node` struct with basic properties (position, size, color, title) and supports dragging. However, it lacks persistence, type systems, and structured property storage. The project already uses nlohmann/json for theme persistence, providing a proven JSON serialization pattern to follow.

## Goals / Non-Goals

**Goals:**
1. **Persistence**: Save/load node graph state to JSON files with File menu integration
2. **Type system**: Support different node types with type-specific properties
3. **Common visual properties**: All nodes share basic visual attributes (color, size, border)
4. **Auto-load**: Load default project at application startup
5. **Extensible architecture**: Support adding new properties and types in the future

**Non-Goals:**
1. **Node connections/edges**: Focus on node properties, not graph topology
2. **Complex node editing UI**: Keep simple property display, not property editors
3. **Real-time collaboration**: Single-user file-based persistence only
4. **Version control**: No git integration or file history
5. **Custom node type creation UI**: Types defined in code, not user-creatable

## Decisions

### 1. JSON Schema Design
**Decision**: Use simple flat JSON structure with all node properties at the same level
```json
{
  "version": 1,
  "nodes": [
    {
      "id": "node1",
      "type": "input",
      "position": [100, 200],
      "size": [100, 60],
      "color": [45, 45, 45, 255],
      "borderColor": [100, 100, 100, 255],
      "title": "Input A",
      "properties": { "value": 5.0, "label": "Input A" }
    }
  ],
  "nodeTypes": {
    "input": { 
      "defaultColor": [45, 45, 45, 255],
      "defaultProperties": { "value": 0.0, "label": "Input" }
    },
    "processor": { 
      "defaultColor": [60, 60, 60, 255],
      "defaultProperties": { "operation": "add", "inputs": 2 }
    }
  }
}
```
**Rationale**: 
1. **Simplicity**: Matches existing Node struct organization, easier to serialize/deserialize
2. **Clarity**: All node properties visible at same level, no nested objects needed
3. **Consistency**: Uses arrays `[x, y]` for positions/sizes and `[r, g, b, a]` for colors, matching theme persistence patterns
4. **Practicality**: Works well for current scope, can be refactored later if needed

**Alternative considered**: Hierarchical structure with `visual` sub-object
- **Rejected**: Overly complex for current needs, adds nesting without clear benefit

### 2. Type System Architecture
**Decision**: Create `NodeType` registry with factory pattern for creating typed nodes
```cpp
class NodeType {
  std::string name;
  std::unordered_map<std::string, nlohmann::json> defaultProperties;
  std::string iconName;
  ImU32 defaultColor;
};

class NodeTypeRegistry {
  std::unordered_map<std::string, NodeType> types;
  Node createNode(const std::string& typeName);
};
```
**Rationale**: Centralized type management, easy to add new types, clean separation from node instances
**Alternative considered**: Inheritance hierarchy (InputNode, ProcessorNode classes) → more complex serialization

### 3. Property Storage
**Decision**: Use `nlohmann::json` for node properties with type-safe getters/setters
```cpp
class Node {
  nlohmann::json properties; // type-specific properties
  VisualProperties visual;    // common visual properties
  NodeType* type;             // reference to type definition
  
  template<typename T>
  T getProperty(const std::string& key, T defaultValue);
};
```
**Rationale**: Flexible property system, supports arbitrary types, integrates with JSON serialization
**Alternative considered**: Fixed property structs per node type → rigid, hard to extend

### bpkus
**Decision**: Extend existing `Node` struct with `visual` member and `properties` JSON field
**Rationale**: Minimal changes to existing code, backward compatible, leverages existing rendering
**Alternative considered**: Complete Node class rewrite → more disruptive, risk of breaking existing functionality

### 5. File Menu Integration
**Decision**: Add "Save Node Graph" and "Load Node Graph" to existing File menu in `App.cpp`
**Rationale**: Consistent with application UI patterns, avoids context menu clutter
**Alternative considered**: Separate "Node Graph" menu → adds menu complexity

### 6. Default Project Loading
**Decision**: Load from `assets/default_node_graph.json` if present, otherwise create default nodes
**Rationale**: Simple file-based default, can be overridden by users, no hardcoded defaults in code
**Alternative considered**: Hardcoded default nodes in code → less flexible

## Risks / Trade-offs

**[Risk] JSON schema evolution**: Future changes may break existing saved files
- **Mitigation**: Include version field, provide migration if needed

**[Risk] Performance with many nodes**: JSON serialization of large graphs could be slow
- **Mitigation**: Optimize with selective serialization, only save visible properties

**[Risk] Type registry initialization**: Circular dependencies with node creation
- **Mitigation**: Initialize registry before any node creation, use lazy loading if needed

**[Trade-off] Flexibility vs type safety**: JSON properties are flexible but lack compile-time checks
- **Acceptance**: Acceptable for this scope, runtime validation can be added later

**[Trade-off] Simple vs comprehensive**: Starting with basic properties leaves room for expansion
- **Acceptance**: Better to start simple and extend than over-engineer initially