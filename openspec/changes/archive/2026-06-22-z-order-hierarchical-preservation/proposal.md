## Why

The current z-order implementation in NodeGraphWidget resets all unselected nodes to z-order 2 when a node is selected, destroying the relative ordering hierarchy between nodes. This makes it impossible to maintain custom z-order arrangements or layered compositions. The system should preserve hierarchical relationships during selection updates.

## What Changes

- Modify z-order update logic to preserve relative hierarchy when nodes are selected
- Change selection behavior: selected node gets z-order 1, other nodes maintain relative ordering (shifted down if needed)
- Update hit testing to consider the new hierarchical z-order system
- Ensure backward compatibility with existing saved files containing z-order values
- **BREAKING**: Changes z-order update behavior (preserves hierarchy instead of resetting to 2)

## Capabilities

### New Capabilities
- `z-order-hierarchy-preservation`: Preserves relative z-order hierarchy when nodes are selected or reordered

### Modified Capabilities
- `nodegraph-zorder`: Updates selection-driven z-order reordering to preserve hierarchy
- `nodegraph-interaction`: Updates mouse selection behavior to work with hierarchical z-order
- `node-graph-widget`: Updates z-order display and rendering to reflect hierarchical values

## Impact

- **Affected Code**: `NodeGraphWidget.cpp` (z-order update logic, selection handling), `NodeGraphWidget.h` (zOrder field)
- **APIs**: No external API changes
- **Dependencies**: nlohmann/json (serialization already supports zOrder field)
- **Systems**: Node graph visualization and interaction system
- **Breaking Changes**: Changes existing z-order update behavior (preserves hierarchy vs resetting to 2)