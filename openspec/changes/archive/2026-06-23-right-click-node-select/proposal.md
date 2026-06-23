## Why

Current NodeGraphWidget has a context menu accessible via right-click on empty canvas area, but right-clicking on a node does nothing. This creates inconsistency where right-click only works on canvas but not on nodes. Users expect right-click on nodes to show context menu for node-specific operations (delete, duplicate, properties) and also select/bring the node to foreground.

## What Changes

- Right-clicking on a node selects the node (sets z-order to 1) and brings it to foreground
- Right-clicking on a node opens context menu with node-specific options
- Right-click on canvas continues to show canvas context menu (no change)
- Hierarchy z-order preservation applies to right-click selection same as left-click
- Existing left-click selection and dragging functionality remains unchanged

## Capabilities

### New Capabilities
- `node-right-click-context`: Right-click context menu support for nodes with selection

### Modified Capabilities
- `nodegraph-interaction`: Add right-click selection scenario to mouse-driven interaction requirements
- `nodegraph-zorder`: Right-click selection triggers same z-order hierarchy preservation as left-click
- `node-graph-widget`: Add node-specific context menu scenario

## Impact

- NodeGraphWidget.cpp: Update `handleInput()` method to handle right-click on nodes
- Context menu system: Extend to support node-specific menu items (delete, duplicate, properties)
- Z-order management: Right-click selection triggers same hierarchical preservation algorithm as left-click
- No API changes, no dependencies affected