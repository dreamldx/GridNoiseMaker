## Context

NodeGraphWidget currently supports:
- Left-click selects node, brings to foreground (z-order 1) with hierarchical preservation
- Right-click on canvas shows context menu with "Reset View" and placeholder items
- Right-click on nodes does nothing
- Canvas context menu uses ImGui::BeginPopupContextWindow()

Current z-order system: when a node is selected via left-click, it gets z-order 1, other nodes preserve relative hierarchy (nodes with z-order ≤ selected node's original z-order shift down by 1, higher z-order nodes unchanged).

## Goals / Non-Goals

**Goals:**
- Right-click on node selects node and brings to foreground (z-order 1)
- Right-click on node opens context menu with node-specific options
- Right-click selection triggers same hierarchical z-order preservation as left-click
- Canvas right-click context menu remains unchanged
- Left-click selection and dragging remain unchanged

**Non-Goals:**
- Multi-node selection via right-click
- Node-specific menu implementations beyond basic placeholder (delete, duplicate, properties)
- Advanced context menu customization
- Keyboard shortcuts for context menu
- Tooltips or hover effects for context menu

## Decisions

**Decision 1: Right-click selection triggers same z-order update as left-click**
- **Chosen**: Right-click selection uses existing hierarchical z-order preservation algorithm
- **Alternative considered**: Right-click doesn't change z-order (only opens menu)
- **Rationale**: Consistent behavior - selection brings node to foreground regardless of click type

**Decision 2: Node context menu content**
- **Chosen**: Basic placeholder menu items (Delete Node, Duplicate Node, Node Properties)
- **Alternative considered**: No node-specific menu items initially
- **Alternative considered**: Full node-specific menu implementation
- **Rationale**: Minimum viable functionality. Can be extended later.

**Decision 3: Menu display priority**
- **Chosen**: Canvas context menu appears only when no node clicked
- **Alternative considered**: Both canvas and node menus could appear simultaneously
- **Rationale**: Clearer UX - one context menu type per click location

**Decision 4: Hit testing for right-click**
- **Chosen**: Use same hit testing as left-click (top-most node wins)
- **Alternative considered**: Different hit testing algorithm
- **Rationale**: Consistency with existing selection behavior

**Decision 5: Integration with existing input handling**
- **Chosen**: Add right-click handling to `NodeGraphWidget::handleInput()` method alongside left-click
- **Alternative considered**: Separate handler for right-click
- **Rationale**: Minimal code changes, reuse existing input logic

## Risks / Trade-offs

**Risk 1**: Conflict with existing canvas context menu
- **Mitigation**: Check if right-click hit a node before opening canvas context menu

**Risk 2**: Performance impact from extra hit testing
- **Mitigation**: Reuse existing hit testing infrastructure, no new loops

**Risk 3**: Confusion between left-click and right-click selection behavior
- **Mitigation**: Both trigger same z-order update, consistent visual feedback

**Risk 4**: Missing visual feedback for right-click selection
- **Mitigation**: Selected node gets z-order 1 and visual z-order display updates

**Risk 5**: Touchscreen compatibility (right-click not available)
- **Mitigation**: Not applicable to desktop ImGui app