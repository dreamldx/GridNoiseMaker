## Why

The node type validation functionality was recently implemented as part of the `register-node-type-validation` change. The dialog implementation is embedded in `App.cpp` and the NodeTypeRegistry is embedded within `NodeGraphWidget.cpp`. This creates tight coupling and makes the code harder to maintain. Separating these components into dedicated files improves modularity, testability, and code organization.

## What Changes

- Create `DialogManager.h` and `DialogManager.cpp` to handle UI dialogs, moving the unknown node types dialog out of `App.cpp`
- Create `NodeTypeRegistry.h` and `NodeTypeRegistry.cpp` to separate the registry from `NodeGraphWidget.cpp`
- Update `App.cpp` to use the new DialogManager for unknown node type notifications
- Update `NodeGraphWidget.cpp` to reference the external NodeTypeRegistry instead of its internal implementation
- Maintain backward compatibility with existing functionality

## Capabilities

### New Capabilities
- `dialog-management`: Centralized dialog handling for UI notifications
- `type-registry-modularity`: Separated node type registry for better code organization

### Modified Capabilities
<!-- Existing capabilities whose REQUIREMENTS are changing (not just implementation).
     Only list here if spec-level behavior changes. Each needs a delta spec file.
     Use existing spec names from openspec/specs/. Leave empty if no requirement changes. -->

## Impact

- **App.cpp**: Will use DialogManager instead of directly managing dialog state
- **NodeGraphWidget.cpp**: Will reference external NodeTypeRegistry instead of internal implementation
- **New files**: DialogManager.h/.cpp, NodeTypeRegistry.h/.cpp
- **Dependencies**: No new dependencies required
- **Testing**: Can now test DialogManager and NodeTypeRegistry independently