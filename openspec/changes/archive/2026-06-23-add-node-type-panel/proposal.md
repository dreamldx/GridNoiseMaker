## Why

Currently, users have no visual way to browse available node types when building node graphs. They must know node type names and manually create nodes. A left panel with node type browsing would improve discoverability and usability by allowing users to visually browse, select, and drag node types into the graph workspace. This is needed now to make the node graph editor more intuitive for new users and to support future expansion with more node types.

## What Changes

1. **Add a left panel to the node graph widget** for browsing node types
2. **Make the panel dockable and detachable** - users can dock it to left/right sides or detach as floating window
3. **Implement three display modes** for node types: icon view, list view, and detailed view
4. **Add a header to the panel** with view mode selector (icon/list/details toggle) and docking controls
5. **Display node types** registered in the NodeTypeRegistry with their names, colors, and properties
6. **Allow drag-and-drop** from the panel to create nodes in the graph
7. **Adjust layout** to accommodate the panel while maintaining usable workspace area

**BREAKING**: The node graph widget will no longer fill the entire client area below the menu bar. It will share space with the left panel.

## Capabilities

### New Capabilities
- **node-type-panel**: Left-side panel for browsing and selecting node types with configurable display modes
- **panel-view-switcher**: Header component with toggle controls for switching between icon, list, and detail views
- **panel-docking**: Docking and detaching functionality allowing panel to be docked left/right or floated as separate window
- **node-type-drag-source**: Drag-and-drop functionality from panel to graph workspace

### Modified Capabilities
- **node-graph-widget**: Layout changes to accommodate left panel, updated to share space instead of filling entire client area
- **app-shell**: Potential adjustments to overall application layout to accommodate new panel structure

## Impact

**Affected Code:**
- `NodeGraphWidget.cpp/.h`: Major changes to layout and addition of panel
- `NodeTypeRegistry.h/.cpp`: May need enhancements for type browsing UI
- `app/` directory: New panel component files
- `ImGui` layout and rendering code throughout the widget

**Dependencies:**
- Existing NodeTypeRegistry for type information
- ImGui for UI rendering and layout
- Current node creation/dragging system

**Systems:**
- UI layout system needs to handle split views (panel + graph) and docking
- Drag-and-drop system between panel and graph
- View state persistence for panel preferences and docking position
- Window management for detached floating panels