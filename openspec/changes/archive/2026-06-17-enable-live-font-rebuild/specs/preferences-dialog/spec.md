## MODIFIED Requirements

### Requirement: Theme tab — master-detail editor
The Theme tab SHALL render a theme-picker row at the top (per the `theme-presets` "Picker UI" requirement), followed by a master-detail editor where the left pane is a `BeginChild`-scrolled selectable list of every editable theme item (grouped by `Separator + Text` headers: Colors / Metrics / Typography), and the right pane is a `BeginChild` showing a type-appropriate widget for the currently-selected item. The split SHALL be implemented via `ImGui::BeginTable("ThemeEditor", 2, ImGuiTableFlags_Resizable)` with initial column widths of approximately 35% (left) and 65% (right). The table's vertical size SHALL reserve space for both the picker row at the top AND the `Reset to defaults` button row at the bottom (`ImVec2(0, -(buttonRowHeight + pickerHeight))`). All widgets in the right pane SHALL mutate `ImGui::GetStyle()` directly so changes are visible live in the main shell.

#### Scenario: Layout includes theme picker row above the master-detail table
- **WHEN** the Theme tab is rendered
- **THEN** the topmost row SHALL contain a `"Theme:"` label and an `ImGui::Combo` widget listing themes from `app::listAvailableThemes()` (per the `theme-presets` capability)
- **AND** the master-detail `BeginTable` SHALL render below the picker row
- **AND** the `Reset to defaults` button SHALL render below the table
- **AND** the table's vertical size SHALL be `ImVec2(0, -(buttonRowHeight + pickerHeight))` (or equivalent) so neither the picker above nor the button below overlaps with the table

#### Scenario: Left pane lists every editable item
- **WHEN** the Theme tab is rendered
- **THEN** the left pane SHALL contain (in this order, separated by `Separator + Text` headers):
  - A "Colors" group: one `Selectable` per `ImGuiCol_*` slot from index 0 to `ImGuiCol_COUNT - 1`
  - A "Metrics" group: one `Selectable` per metric in the supported set (19 entries: `WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`, `IndentSpacing`, `ScrollbarSize`, `GrabMinSize`, `WindowRounding`, `ChildRounding`, `FrameRounding`, `GrabRounding`, `PopupRounding`, `ScrollbarRounding`, `TabRounding`, `WindowBorderSize`, `ChildBorderSize`, `PopupBorderSize`, `FrameBorderSize`, `TabBorderSize`)
  - A "Typography" group: two `Selectable` rows — `Font file` and `Font size`
- **AND** each `Selectable` SHALL display its label on the left and a current-value preview on the right (hex chip for colors, formatted number for metrics, current selection for typography)
- **AND** selection state SHALL be tracked in file-scope state and persist across frames

#### Scenario: Right pane widget matches selected item type
- **WHEN** a `Colors` entry is selected
- **THEN** the right pane SHALL render `ImGui::ColorEdit4("##color", &style.Colors[idx].x, ImGuiColorEditFlags_AlphaBar)`
- **WHEN** a scalar `Metrics` entry is selected
- **THEN** the right pane SHALL render `ImGui::SliderFloat` with the metric's documented range and step (see design.md D4 in the `add-preferences-dialog` change)
- **WHEN** an `ImVec2` `Metrics` entry is selected (`WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`)
- **THEN** the right pane SHALL render `ImGui::DragFloat2` with the metric's documented range
- **WHEN** the Typography `Font file` entry is selected
- **THEN** the right pane SHALL render `ImGui::Combo` with at least the three bundled font options (`Inter Regular` → `fonts/Inter-Regular.ttf`, `JetBrains Mono Regular` → `fonts/JetBrainsMono-Regular.ttf`, `Lato Regular` → `fonts/Lato-Regular.ttf`) plus a `Custom...` entry that reveals an `ImGui::InputText` for a hand-entered path
- **WHEN** the Typography `Font size` entry is selected
- **THEN** the right pane SHALL render `ImGui::SliderFloat` with range `[6.0, 96.0]` and format `"%.1f px"`

#### Scenario: Typography changes apply immediately (no caveat)
- **WHEN** the Typography `Font file` or `Font size` widget is rendered in the right pane
- **THEN** the dialog SHALL NOT render any "applies on next launch" caveat note (this scenario replaces the pre-change "applies on next launch" caveat)
- **AND** on widget commit (mouse release / Combo pick / InputText Enter or focus-loss), the dialog SHALL call `app::rebuildFontAtlas()` AFTER `persistTheme()` so the new font / size appears within one frame

## ADDED Requirements

### Requirement: Typography commits trigger live atlas rebuild
The Preferences dialog SHALL call `app::rebuildFontAtlas()` after `persistTheme()` for every widget commit that mutates `themeFontFile()` or `themeFontSizePx()`. The three affected widget paths in the Typography section of the Theme tab's right pane SHALL all invoke the rebuild: (a) the bundled-font `Combo` when its return value indicates a change, (b) the custom-path `InputText` when `IsItemDeactivatedAfterEdit` fires, (c) the font-size `SliderFloat` when `IsItemDeactivatedAfterEdit` fires.

#### Scenario: Font Combo selection rebuilds atlas
- **WHEN** the user picks a different entry from the Typography Font Combo
- **THEN** the dialog SHALL call `app::setThemeFontFile(<path>)` then `persistTheme()` then `app::rebuildFontAtlas()`
- **AND** the visible font in the main shell SHALL change within one frame

#### Scenario: Font-size slider release rebuilds atlas
- **WHEN** the user scrubs the `Font size` `SliderFloat` and releases the mouse
- **THEN** `ImGui::IsItemDeactivatedAfterEdit()` SHALL be true exactly once
- **AND** the dialog SHALL call `persistTheme()` then `app::rebuildFontAtlas()`
- **AND** the visible text in the main shell SHALL be rendered at the new size starting the next frame

#### Scenario: Custom font-path commit rebuilds atlas
- **WHEN** the user finishes editing the custom-path `InputText` (Enter or focus loss)
- **THEN** `ImGui::IsItemDeactivatedAfterEdit()` SHALL be true exactly once
- **AND** the dialog SHALL call `app::setThemeFontFile(buf)` then `persistTheme()` then `app::rebuildFontAtlas()`
- **AND** if the path is valid, the visible font SHALL change within one frame
- **AND** if the path is invalid (file not found), `configureFontAtlas` SHALL fall back to `AddFontDefault` (Proggy Clean), the rebuild SHALL still complete, and the visible font SHALL become Proggy as visible feedback (alongside the existing stderr warning)

#### Scenario: Mid-drag does NOT rebuild
- **WHEN** the user is actively dragging the Font size `SliderFloat` (mouse held down, value changing every frame)
- **THEN** `app::rebuildFontAtlas()` SHALL NOT be called per-frame
- **AND** the font atlas SHALL retain its pre-drag rasterization until the user releases the mouse
- **AND** the slider's display value SHALL update live (showing the chosen number), but the rendered TEXT around it SHALL still use the previous-size atlas until release
