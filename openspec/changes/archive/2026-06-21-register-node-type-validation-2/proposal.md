## Why

Currently, the node graph system allows loading nodes with unknown type names from JSON files, which leads to inconsistent behavior where nodes appear as generic "default" nodes without their intended properties or visual styling. This creates a poor user experience when loading saved projects that contain node types not registered in the current session, and silently discards important type-specific data.

## What Changes

- **Node type registration at startup**: Ensure all built-in node types are registered before any node creation or JSON loading
- **Type name validation**: Verify node type names during JSON loading against the registry
- **Unknown type handling**: Skip nodes with unknown types and collect them for user feedback
- **Error notification**: Show a dialog message listing all skipped nodes with their type names
- **No silent failures**: Replace silent fallback to "default" nodes with explicit error feedback

## Capabilities

### New Capabilities
- **node-type-validation**: Capability for validating node types during JSON loading and reporting unknown types to users

### Modified Capabilities
- **node-graph-persistence**: Requirement to validate node types during loading and handle unknown types with user feedback instead of silent fallback
- **node-graph-widget**: Requirement to register built-in node types at startup and maintain type registry state

## Impact

**Affected Code:**
- `apps/imgui-shell/app/NodeGraphWidget.cpp`: NodeGraphWidget constructor and `fromJson()` method
- `apps/imgui-shell/app/NodeGraphWidget.h`: NodeTypeRegistry class interface
- `apps/imgui-shell/app/App.cpp`: Application initialization and error dialog handling

**Dependencies:**
- ImGui for dialog rendering
- nlohmann/json for JSON parsing
- Existing node type registry system

**No Breaking Changes**: The change maintains backward compatibility by skipping unknown nodes rather than rejecting entire files, and provides clear user feedback instead of silent behavior.