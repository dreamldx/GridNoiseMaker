## Context

The NodeGraphWidget currently implements a simple z-order system where:
- All nodes have default z-order value of 2
- Selected node gets z-order value of 1  
- When a new node is selected, all other nodes are reset to z-order 2
- Lower z-order values draw on top (inverse depth)

This destroys any custom z-order arrangements users might create. For example, if nodes are arranged as A:2, B:1, C:3 (B on top, C on bottom), selecting A would reset B and C to 2, losing the B-on-top, C-on-bottom relationship.

## Goals / Non-Goals

**Goals:**
1. Preserve relative z-order hierarchy when nodes are selected
2. Selected node always gets z-order 1 (top-most position)
3. Other nodes maintain their relative ordering (shifted down if needed)
4. Backward compatibility with existing saved files
5. Efficient algorithm for updating z-order values

**Non-Goals:**
1. Multiple simultaneous node selection
2. Manual z-order assignment UI
3. Z-order values outside integer range
4. Changing the "lower values draw on top" rendering rule

## Decisions

**Decision 1: Hierarchical shift algorithm**
- **Chosen**: When node X is selected with original z-order value Zx:
  1. Set X.zOrder = 1
  2. For all other nodes Y with original z-order value Zy:
     - If Zy ≤ Zx: Y.zOrder = Zy + 1 (shift down to make room for selected node at position 1)
     - If Zy > Zx: Y.zOrder = Zy (unchanged)
- **Alternative considered**: Global renumbering (sort all nodes and assign sequential values)
- **Rationale**: Preserves relative hierarchy while keeping z-order values close to original. More intuitive than global renumbering for users who set custom z-order values.

**Decision 2: Handling z-order 1 collisions**
- **Chosen**: If selected node already has z-order 1, no changes to other nodes
- **Alternative considered**: Always shift all nodes down
- **Rationale**: If user clicks same node twice, should be idempotent. No need to disturb other nodes.

**Decision 3: Z-order range handling**
- **Chosen**: Use 32-bit signed integers, allow any integer value
- **Alternative considered**: Limit to positive values only
- **Rationale**: More flexible for future use cases. Negative values could represent "background" nodes.

**Decision 4: Empty canvas click behavior**
- **Chosen**: When clicking empty canvas, deselect all nodes but preserve z-order hierarchy
- **Alternative considered**: Reset all nodes to z-order 2 (loses hierarchy)
- **Alternative considered**: Shift nodes to remove z-order 1 position (complex)
- **Rationale**: Preserves user's custom z-order arrangements. Simple implementation. z-order 1 becomes a regular value when no node is selected.

## Risks / Trade-offs

**Risk 1**: Integer overflow with many hierarchical shifts
- **Mitigation**: Use 32-bit integers, extremely unlikely to overflow in practice (would require 2 billion+ nodes)

**Risk 2**: Performance with many nodes (O(n) update)
- **Mitigation**: Acceptable for typical node counts (<1000). Current implementation already O(n) for sorting.

**Risk 3**: Confusing behavior when nodes have equal z-order values
- **Mitigation**: Stable sort preserves original order for equal z-order values (existing behavior)

**Risk 4**: Nodes may have z-order 1 without being selected
- **Mitigation**: z-order 1 is just a regular value when no node is selected. Visually indistinguishable from other values.