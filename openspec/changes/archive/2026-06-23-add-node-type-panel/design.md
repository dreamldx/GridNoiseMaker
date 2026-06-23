## Context

The NodeGraphWidget currently fills the entire client area below the menu bar with a grid-based workspace. Users create nodes by knowing type names, with no visual browsing interface. The NodeTypeRegistry contains registered node types with names, colors, and properties, but this information is not exposed in the UI. ImGui provides docking functionality that can be leveraged for dockable panels.

## Goals / Non-Goals

**Goals:**
1. Provide visual browsing of available node types through a dockable left panel
2. Support multiple display modes: icon view (compact), list view (textual), detail view (full info)
3. Enable drag-and-drop creation of nodes from panel to graph workspace
4. Allow panel to be docked to left/right sides or detached as floating window
5. Maintain backward compatibility for existing node creation methods
6. Preserve user preferences for panel state (docking position, view mode, size)

**Non-Goals:**
1. Creating a full node editor/designer within the panel
2. Supporting panel docking to top/bottom edges (only left/right for workspace layout)
3. Implementing custom docking system (use ImGui's built-in docking)
4. Real-time node type registration/unregistration UI
5. Complex panel customization/theming beyond basic view modes

## Decisions

### 1. Use ImGui Docking System
**Decision:** Leverage ImGui's built-in docking system for panel docking/detaching
**Rationale:** ImGui already provides mature docking functionality that handles layout, split views, and floating windows. Implementing custom docking would be complex and error-prone.
**Alternatives Considered:**
- Custom docking implementation: Too much work, would need to handle many edge cases
- Fixed panel only: Doesn't meet requirement for detachable panels

### 2. Panel as Separate Window within NodeGraphWidget
**Decision:** Implement panel as separate ImGui window within NodeGraphWidget render method
**Rationale:** Keeps panel code colocated with graph widget while allowing ImGui to manage docking. Panel state (visibility, position) can be managed alongside graph state.
**Alternatives Considered:**
- Separate widget class: Would need complex communication with NodeGraphWidget
- Global application panel: Wouldn't be specific to node graph context

### 3. Three View Modes with Header Toggle
**Decision:** Implement icon, list, and detail views with header toggle buttons
**Rationale:** Provides flexibility for different user preferences and screen sizes. Icon view for visual browsing, list view for quick scanning, detail view for learning.
**Alternatives Considered:**
- Single view mode: Less flexible
- Collapsible sections: Too complex for initial implementation

### 4. Drag-and-Drop using ImGui Payload System
**Decision:** Use ImGui's drag-and-drop payload system for node creation
**Rationale:** ImGui provides built-in drag-and-drop with visual feedback. Node type information can be serialized in payload.
**Alternatives Considered:**
- Click-to-create: Less intuitive, would need position selection
- Double-click: Similar issues with position selection

### 5. Panel State Persistence in Preferences
**Decision:** Store panel state (docked/floating, view mode, size) in application preferences
**Rationale:** Users expect UI state to be preserved between sessions. Preferences system already exists for theme settings.
**Alternatives Considered:**
- Session-only persistence: Would reset on restart
- Config file per-workspace: Overly complex for simple panel state

### 6. Default Docked Left, Resizable Split
**Decision:** Default panel state is docked left with resizable splitter
**Rationale:** Left docking is conventional for tool panels. Resizable splitter allows users to adjust workspace vs panel size.
**Alternatives Considered:**
- Right docking: Less common for creation panels
- Fixed width: Less flexible for different screen sizes

## Risks / Trade-offs

**Risk 1: Docking complexity** → Use ImGui's stable docking API, test edge cases
**Risk 2: Performance with many node types** → Virtual scrolling for list views, limit icon size
**Risk 3: Layout conflicts with existing UI** → Careful integration testing with current NodeGraphWidget
**Risk 4: Drag-and-drop coordinate conversion** → Use existing ViewTransform system for world-space placement
**Risk 5: State persistence conflicts** → Clear migration path if schema changes

**Trade-offs:**
- Using ImGui docking ties us to ImGui's docking model but saves implementation time
- Three view modes increase complexity but improve usability
- Storing state in preferences centralizes persistence but couples to preferences system