## 1. Input Handling Implementation

- [x] 1.1 Update NodeGraphWidget::handleInput to detect right-click on nodes
- [x] 1.2 Add right-click hit testing using same logic as left-click (top-most node)
- [x] 1.3 Trigger node selection on right-click (set zOrder=1, apply hierarchical preservation)
- [x] 1.4 Ensure canvas context menu appears only when right-click misses all nodes
- [x] 1.5 Test right-click selection with overlapping nodes (hit testing priority)

## 2. Context Menu Implementation

- [x] 2.1 Create node-specific context menu function in NodeGraphWidget
- [x] 2.2 Add placeholder menu items (Different Node, Duplicate Node, Node Properties)
- [x] 2.3 Add separators between menu item groups
- [x] 2.4 Ensure node context menu appears at mouse cursor position
- [x] 2.5 Test menu closes automatically when clicking outside or selecting items

## 3. Z-order Integration

- [x] 3.1 Verify right-click selection triggers same hierarchical preservation as left-click
- [x] 3.2 Test z-order display updates after right-click selection
- [x] 3.3 Test sequential selections (left-right-left) maintain z-order hierarchy
- [x] 3.4 Test with nodes having equal z-order values (stable sort behavior)

## 4. Integration Testing

- [x] 4.1 Test right-click selection with existing left-click dragging functionality
- [x] 4.2 Test with pan/zoom operations (middle-click drag, Ctrl+wheel zoom)
- [x] 4.3 Verify canvas context menu still works when right-click misses nodes
- [x] 4.4 Test backward compatibility (saved files with z-order values)

## 5. Documentation and Verification

- [x] 5.1 Update comments in NodeGraphWidget.cpp for right-click behavior
- [x] 5.2 Verify no breaking changes to existing APIs
- [x] 5.3 Build and run application to test complete functionality