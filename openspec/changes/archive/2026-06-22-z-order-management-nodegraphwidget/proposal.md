## Why

The current NodeGraphWidget lacks proper z-order management for visual hierarchy and node selection feedback. When multiple nodes overlap, there's no clear visual indication of selection state or intuitive click behavior. This change implements a z-order system where lower values draw on top (0 for UI, 1 for selected nodes), with mouse selection triggering automatic reordering to bring selected nodes to the foreground, and visual display of z-order values at the bottom of each node.

## What Changes

- Implement z-order tracking system in NodeGraphWidget with lower values drawing on top (inverse depth)
- Set default z-order: 0 for UI elements, 1 for selected node(s)
- Automatically reorder z-order when nodes are selected with mouse (selected node gets z-order 1)
- Display z-order value in bottom area of each node for debugging/visual feedback
- Add mouse interaction handling to trigger z-order updates on selection
- **BREAKING**: Node rendering order will change - previously drawn nodes may now be obscured by higher z-order nodes

## Capabilities

### New Capabilities
- `nodegraph-zorder`: Z-order management system for NodeGraphWidget including tracking, rendering order, and selection-driven updates
- `nodegraph-visual-feedback`: Visual display of z-order values at node bottom and selection highlighting
- `nodegraph-interaction`: Mouse-driven z-order reordering on node selection

### Modified Capabilities
- `nodegraph-rendering`: Modify rendering pipeline to respect z-order values with lower values drawing on top
- `nodegraph-selection`: Enhance selection handling to trigger z-order updates

## Impact

- **Affected Code**: NodeGraphWidget class, node rendering pipeline, mouse interaction handlers
- **APIs**: Internal z-order management API within NodeGraphWidget
- **Visual Changes**: Node rendering order will follow z-order values, selected nodes will visually appear on top
- **User Experience**: Clearer visual hierarchy, intuitive click behavior with automatic bring-to-front on selection