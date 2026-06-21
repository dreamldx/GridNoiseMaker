## Why

The current node graph widget provides visualization and interaction capabilities but lacks persistence. Node positions, properties, and graph state are lost when the application closes. Adding persistence enables users to save and restore their work, making the node graph a practical tool rather than just a demonstration.

## What Changes

- **Add JSON serialization** for node graph state (node positions, connections, properties, node types)
- **Create file-based persistence** system with save/load operations
- **Add node type system** with type-based customized properties and common visualization properties (e.g., different node types can have different properties but share common visual properties like color, size)
- **Add node property storage** starting with coordinates (x, y) with extensible architecture for type-based properties
- **Integrate with existing preferences system** using the same JSON library (nlohmann/json) already in the project
- **Add UI controls** for save/load operations in the File menu (no context menu items)
- **Add auto-load functionality** to load default project at application startup

## Capabilities

### New Capabilities
- `node-graph-persistence`: Save and restore node graph state including node positions, properties, types, and visualizations to/from JSON files, with auto-load of default project at startup

### Modified Capabilities
- `node-graph-widget`: Extend to include persistence operations and property storage
- `app-shell`: Add save/load menu items to File menu

## Impact

- **Affected code**: `apps/imgui-shell/app/NodeGraphWidget.h/.cpp`, `apps/imgui-shell/app/SimpleGridRenderer.h/.cpp`, `apps/imgui-shell/app/ViewTransform.h/.cpp`
- **Dependencies**: nlohmann/json (already used for theme persistence)
- **New files**: Node persistence serialization classes, JSON schema definitions
- **UI changes**: Add save/load options to File menu only (no context menu items)