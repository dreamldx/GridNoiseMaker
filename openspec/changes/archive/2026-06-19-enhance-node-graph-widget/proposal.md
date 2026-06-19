## Why

The node graph widget has been developed as a core visualization component but lacks formal specification. Creating a spec will document its existing capabilities (grid rendering, pan/zoom, draggable nodes, full-screen startup) and establish requirements for future enhancements.

## What Changes

- **Document existing node graph widget capabilities** with formal specification
- **Define requirements** for grid-based visualization, view transformation, and node interaction
- **Establish API contracts** for node graph widget interface and behavior
- **No breaking changes** - only documenting existing functionality

## Capabilities

### New Capabilities
- `node-graph-widget`: Core widget providing grid-based node visualization with pan/zoom, draggable nodes, and full-screen startup behavior

### Modified Capabilities
<!-- No existing specs need modification since this is documenting new functionality -->

## Impact

- **Affected code**: `apps/imgui-shell/app/NodeGraphWidget.h/.cpp`, `apps/imgui-shell/app/SimpleGridRenderer.h/.cpp`, `apps/imgui-shell/app/ViewTransform.h/.cpp`
- **APIs**: Node graph widget public interface, view transformation system
- **Dependencies**: ImGui rendering system, coordinate transformation utilities