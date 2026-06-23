## 1. Node Data Structure Updates

- [x] 1.1 Add `zOrder` field to `Node` struct with default value 2
- [x] 1.2 Update Node constructor to initialize zOrder
- [x] 1.3 Update JSON serialization to include zOrder field
- [x] 1.4 Update JSON deserialization to load zOrder (with default fallback)

## 2. Z-Order Rendering System

- [x] 2.1 Add z-order sorting to NodeGraphWidget rendering pipeline
- [x] 2.2 Implement stable sort by zOrder (lower values first)
- [x] 2.3 Ensure UI elements (grid, buttons) render with zOrder 0
- [x] 2.4 Test rendering order with overlapping nodes

## 3. Selection-Driven Z-Order Updates

- [x] 3.1 Modify mouse click handling to update zOrder on selection
- [x] 3.2 Set selected node zOrder to 1
- [x] 3.3 Revert previously selected node zOrder to 2
- [x] 3.4 Handle empty canvas clicks (deselect all, no zOrder 1)
- [x] 3.5 Implement click priority: select top-most (lowest zOrder) node

## 4. Visual Feedback Implementation

- [x] 4.1 Add z-order text display at bottom of each node
- [x] 4.2 Format text as "Z: <value>" (e.g., "Z: 1", "Z: 2")
- [x] 4.3 Ensure text updates immediately when zOrder changes
- [x] 4.4 Style text for readability (small font, contrast color)
- [x] 4.5 Add visual highlighting for selected node (border/color change)

## 5. Integration and Testing

- [x] 5.1 Ensure existing node dragging still works with zOrder system
- [x] 5.2 Verify pan/zoom functionality unaffected by zOrder
- [x] 5.3 Test context menu compatibility with zOrder updates
- [x] 5.4 Test with overlapping nodes at various zOrder values
- [x] 5.5 Verify backward compatibility with existing saved files
- [x] 5.6 Update documentation to reflect zOrder system changes

## 6. Edge Cases and Polish

- [x] 6.1 Handle drag operations (maintain zOrder during dragging)
- [ ] 6.2 Test with many nodes (>50) for performance impact
- [x] 6.3 Verify stable sort preserves original order for equal zOrder values
- [ ] 6.4 Add zOrder display toggle for debugging (optional feature)
- [x] 6.5 Validate all scenarios from specs are implemented