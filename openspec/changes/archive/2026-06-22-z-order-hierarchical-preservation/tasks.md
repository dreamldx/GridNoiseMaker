## 1. Algorithm Implementation

- [x] 1.1 Analyze current z-order update logic in `NodeGraphWidget::updateNodeDragging`
- [x] 1.2 Implement hierarchical z-order preservation algorithm
- [x] 1.3 Handle edge case: selected node already has z-order 1 (idempotent)
- [x] 1.4 Handle edge case: empty canvas click resets all nodes to z-order 2
- [x] 1.5 Update z-order display text to show hierarchical values

## 2. Integration with Existing Systems

- [x] 2.1 Ensure hit testing respects hierarchical z-order (top-most/lowest z-order first)
- [x] 2.2 Verify stable sorting preserves original order for equal z-order values
- [x] 2.3 Test with existing node dragging functionality (z-order preserved during drag)
- [x] 2.4 Test with pan/zoom operations (z-order unchanged during view transformations)
- [x] 2.5 Test context menu compatibility with hierarchical z-order

## 3. Testing and Validation

- [x] 3.1 Create test scenario: A:2, B:1, C:3 → select A → A:1, B:2, C:3
- [x] 3.2 Create test scenario: A:2, B:1, C:3, D:2 → select C → C:1, A:2, B:1, D:3
- [x] 3.3 Test sequential selections maintain cumulative hierarchy
- [x] 3.4 Test with nodes having equal z-order values (stable sort behavior)
- [x] 3.5 Verify empty canvas click preserves z-order hierarchy (doesn't reset to 2)
- [x] 3.6 Test backward compatibility with saved files containing z-order values

## 4. Performance and Edge Cases

- [x] 4.1 Test with many nodes (>50) for performance impact
- [x] 4.2 Test with extreme z-order values (negative, very large)
- [x] 4.3 Verify integer overflow protection (32-bit signed ints)
- [x] 4.4 Document breaking changes: z-order update behavior changed (selection preserves hierarchy, empty canvas preserves hierarchy)