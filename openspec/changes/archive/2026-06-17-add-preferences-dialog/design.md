## Context

This change adds a Preferences dialog to the imgui-shell application, enabling runtime theme editing and persistence. The dialog depends on two foundational changes:
1. **Multi-viewport support** (`enable-multi-viewport` change): Enables ImGui windows to become real OS-level secondary windows on desktop platforms
2. **Theme persistence** (`add-theme-persistence` change): Adds JSON read/write with per-OS config paths

The current application has a curated dark theme applied at `app::init` time (`applyTheme`), but all theme values are locked at build time. Users who want to tweak colors or metrics must edit `Theme.h` and rebuild. This change provides a user-facing Preferences window with three tabs (General, Theme, About) accessible via `Help > Preferences...`.

## Goals / Non-Goals

**Goals:**
1. Provide a Preferences dialog with three tabs: General (diagnostic info), Theme (master-detail editor), About (static credits)
2. Enable runtime theme editing with immediate visual feedback (live preview)
3. Persist theme changes via Save button using the theme-persistence infrastructure
4. Open as separate OS window on desktop (multi-viewport), inline panel on iOS
5. Add `Help > Preferences...` menu item to existing Help menu

**Non-Goals:**
1. Light theme or theme switching (single dark theme only)
2. Undo/redo within Theme tab (Discard button re-reads file)
3. Theme import/export (drag-drop JSON files)
4. Keyboard shortcuts for Preferences window
5. General-tab settings that modify behavior in v1 (diagnostic info only)
6. Multi-viewport on iOS (iOS Preferences is inline panel)
7. Font selection in Theme tab (font family/size remain theme constants)

## Decisions

**D1: Dialog structure and layout**
- Three tabs: General, Theme, About
- Theme tab uses master-detail layout: left pane selectable list, right pane editing widgets
- Buttons at bottom of Theme tab: Save, Reset to defaults, Discard changes
- Rationale: Clear separation of concerns, familiar UI pattern for preferences

**D2: Multi-viewport usage for desktop, inline for iOS**
- Desktop: Enable `ImGuiConfigFlags_ViewportsEnable`, dialog opens as secondary OS window
- iOS: No multi-viewport flag, dialog appears as floating modal panel
-L Rationale: Platform-appropriate user experience; iOS doesn't support multiple windows

**D3: Live preview implementation**
- Theme tab changes immediately update `ImGui::GetStyle()` 
- No separate "preview" mode or "apply" button needed
- Rationale: Direct feedback, matches expectations for theme editors

**D4: Integration with theme persistence**
- `readThemeFromConfig` called after `applyTheme` in `app::init` (user values override curated)
- Save button calls `writeThemeToConfig` with current style
  
Discard button re-reads saved file via `readThemeFromConfig`
- Rationale: Reuse existing infrastructure from `add-theme-persistence` change

**D5: Menu item placement**
- `Help > Preferences...` added to existing Help menu
-Keep existing `Help > About...` modal (both available)
- Rationale: Logical location, consistent with other applications

**D6: Dialog persistence**
- Use ImGui's standard ini file mechanism for position/size
-D Scope persistence to Preferences window only via `ImGuiWindowFlags`
- Rationale: Standard ImGui feature, preserves user's window placement

**D7: General tab content**
: Read-only diagnostic info: ImGui version, platform name, build config, Vulkan/Metal version, GPU name
- No editable settings in v1
- Rationale: Provides useful info without complexity

**D8: About tab content**
- Static credits: project info, Dear ImGui, Inter font (OFL link), FreeType
- Coexists with existing About modal
- Rationale: Full credits with more space than modal

## Risks / Trade-offs

**R1: Multi-viewport Vulkan complexity** → Mitigation: Follow ImGui's reference implementation, reuse existing render path
**R2: Performance impact of live style updates** → Mitigation: Style changes are cheap, only during editing
**R3: iOS inline panel vs desktop window inconsistency** → Mitigation: Platform-appropriate, documented behavior
**R4: JSON schema evolution** → Mitigation: Backward-compatible parsing, missing keys use defaults
**R5: Multiple About mechanisms (modal + tab)** → Mitigation: Both serve different purposes (quick verification vs full credits)

**Trade-offs:**
- **No undo/redo**: Simpler implementation, Discard button provides basic reset
- **No theme switching**: Single dark theme simplifies persistence and UI
- **No font editing**: Fonts remain theme constants, reduces scope
- **iOS inline panel**: Consistent with iOS single-window paradigm

## Open Questions

1. Should the Preferences dialog remember which tab was last active?
2. Should there be a "Defaults" button that resets to curated theme (not just file)?
3. Should the General tab eventually get editable settings? (Future change)
4. Should theme editing be limited to certain colors/metrics? (Currently all editable)
5. Should there be validation for color values (e.g., 0-1 range)? (ImGui widgets handle this)