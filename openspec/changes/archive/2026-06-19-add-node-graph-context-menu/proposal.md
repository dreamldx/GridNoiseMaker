## Why

The node graph widget currently lacks context-sensitive interaction methods. Adding a right-click context menu provides intuitive access to common operations without cluttering the main interface with buttons or requiring keyboard shortcuts.

## What Changes

- **Add right-click context menu** to node graph widget with menu items accessible via right mouse button
- **Include Reset View option** in context menu to provide alternative access to reset functionality
- **Add 5 placeholder menu items** separated by divider lines for future functionality expansion
- **No breaking changes** - adding new functionality without modifying existing behavior

## Capabilities

### New Capabilities
- `node-graph-context-menu`: Context-sensitive right-click menu for node graph widget with Reset View option and placeholder items

### Modified Capabilities
.

## Impact

- **Affected code**: `apps/imgui-shell/app/NodeGraphWidget.h/.cpp` (add context menu handling)
- **APIs**: No new public APIs, internal menu handling only
- **Dependencies**: ImGui menu system, existing node graph widget infrastructure