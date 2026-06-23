# Z-Order Management for NodeGraphWidget - Technical Design

## Context

The NodeGraphWidget currently lacks proper visual hierarchy management. When multiple nodes overlap, there's no clear indication of selection state or intuitive click behavior. The existing implementation draws nodes in the order they appear in the data structure, which doesn't provide visual feedback for user interactions. This design implements a z-order system where lower values draw on top (inverse depth), with automatic reordering on selection and visual feedback.

Current constraints:
- NodeGraphWidget uses ImGui for rendering with immediate mode graphics
- Existing node data structure: `Node` with position, size, color, title fields
- Mouse interaction already handles dragging, panning, and zooming
- Right-click context menu already implemented

## Goals / Non-Goals

**Goals:**
1. Implement z-order tracking system with lower values drawing on top (0=UI, 1=selected, 2=default)
2. Automatic z-order reordering when nodes are selected via mouse click
3. Visual display of z-order value at bottom of each node
4. Maintain backward compatibility with existing node dragging and interaction
5. Preserve all existing functionality (pan, zoom, context menu)

**Non-Goals:**
1. Multiple simultaneous selected nodes (single selection only)
2. Manual z-order adjustment UI (beyond automatic selection-driven updates)
3. Z-order animation or smooth transitions
4. Z-order-based connection routing
5. 3D depth effects or perspective rendering

## Decisions

### 1. Z-Order Storage Approach
**Decision**: Add `zOrder` field to existing `Node` data structure
- **Rationale**: Simple, direct association with each node
- **Alternative Considered**: Separate z-order mapping (node ID → z-order)
  - Pros: Doesn't modify Node structure
  - Cons: More complex lookups, synchronization issues

### 2. Rendering Order Implementation
**Decision**: Sort nodes by z-order before rendering each frame
- **Rationale**: Simple implementation, clear separation of concerns
- **Alternative Considered**: Maintain sorted node list
  - Pros: No per-frame sorting overhead
  - Cons: Complexity in maintaining sorted order during updates

### 3. Selection-Driven Z-Order Update
**Decision**: Update z-order immediately on mouse click
- **Rationale**: Instant visual feedback, intuitive behavior
- **Alternative Considered**: Animation-based approach
  - Pros: Smoother visual transition
  - Cons: Complexity, delayed feedback

### 4. Z-Order Display Format
**Decision**: Simple text "Z: <value>" at node bottom
- **Rationale**: Clear, unobtrusive, debugging-friendly
- **Alternative Considered**: Visual indicators only (color, border)
  - Pros: Less visual clutter
  - Cons: Ambiguous values, less precise

### 5. Click Priority Resolution
**Decision**: Select top-most (lowest z-order) node at click position
- **Rationale**: Matches visual perception, intuitive behavior
- **Alternative Considered**: Click-through to all overlapping nodes
  - Pros: More control
  - Cons: Complex, unexpected behavior

## Risks / Trade-offs

**Risks:**
1. **Performance overhead from per-frame sorting**
   - Mitigation: Node count is typically small (<100), sorting cost negligible
2. **Visual clutter from z-order text display**
   - Mitigation: Small font size, positioned at bottom, optional debugging flag
3. **Breaking existing behavior for overlapping nodes**
   - Mitigation: Preserve existing behavior for non-overlapping nodes
4. **Z-order conflicts when multiple nodes have same value**
   - Mitigation: Stable sort preserves original order as tie-breaker

**Trade-offs:**
1. **Simplicity vs. Flexibility**: Simple z-order system chosen over complex layering system
2. **Immediate vs. Animated Updates**: Immediate updates chosen for responsiveness
3. **Text Display vs. Visual Only**: Text display chosen for clarity and debugging

## Migration Plan

1. **Phase 1**: Add z-order field to Node structure with default value 2
2. **Phase 2**: Implement rendering sort by z-order
3. **Phase 3**: Add mouse interaction to update z-order on selection
4. **Phase 4**: Add z-order text display at node bottom
5. **Phase 5**: Update existing specs to include z-order requirements

**Rollback Strategy**: Remove z-order field, revert sorting logic, maintain backward-compatible default behavior

## Open Questions

1. Should z-order values be persisted in saved/loaded graph files?
   - **Decision**: Yes, include in JSON serialization for consistency
2. Should there be a maximum z-order value?
   - **Decision**: No practical limit, but values >1000 unlikely
3. How to handle UI elements (grid, buttons) z-order?
   - **Decision**: Fixed z-order 0 for all UI elements
4. Should z-order affect node connection drawing?
   - **Decision**: No, connections drawn after nodes regardless of z-order