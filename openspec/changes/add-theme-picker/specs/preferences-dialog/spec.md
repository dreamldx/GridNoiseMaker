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

#### Scenario: Typography changes display the "applies on next launch" caveat
- **WHEN** the Typography `Font file` or `Font size` widget is rendered in the right pane
- **THEN** a small inline note SHALL appear below the widget reading approximately `"Font changes apply on next launch. The current view still uses the previously-loaded font."`
- **AND** the note SHALL be rendered using `ImGui::TextDisabled` or `ImGui::TextColored` with a subdued color so it doesn't compete with the active widget

### Requirement: Autosave on edit commit + Reset
The Theme tab SHALL persist style and typography changes to disk automatically whenever a widget edit commits (mouse release for `ColorEdit4` / `SliderFloat` / `DragFloat2`, selection for `Combo`, Enter or focus-loss for `InputText`). The autosave target SHALL be the user-dir file for the currently-selected theme — i.e., `<config-dir>/imgui-shell/themes/<selected>.json` (per the `theme-presets` "Copy-on-write" requirement). The dialog SHALL render a single `Reset to defaults` button that resets the *selected* theme to its bundled baseline (per the `theme-presets` "Reset to defaults" requirement). There is NO explicit Save button (autosave makes it redundant). There is NO Discard button (without a separate uncommitted in-memory state, there is nothing to discard).

#### Scenario: Color edit autosaves to the selected theme's user-dir file
- **WHEN** the user opens a Color item, drags the alpha bar / channel sliders inside `ColorEdit4`, and releases the mouse
- **THEN** the dialog SHALL detect the commit via `ImGui::IsItemDeactivatedAfterEdit()` and call `app::persistTheme()` exactly once
- **AND** `persistTheme` SHALL resolve the target file to `<config-dir>/imgui-shell/themes/<selected-theme>.json` via `app::userThemePath(app::selectedThemeName())`
- **AND** the file SHALL be written atomically (temp + rename) with the new color in its `colors` block
- **AND** during the drag itself (before release) the file SHALL NOT be re-written each frame

#### Scenario: Slider autosaves on commit
- **WHEN** the user scrubs a `SliderFloat` (scalar metric or font size) or a `DragFloat2` (vec2 metric) and releases the mouse
- **THEN** `ImGui::IsItemDeactivatedAfterEdit()` SHALL be true exactly once and trigger one `persistTheme` call
- **AND** the in-memory style at the moment of release SHALL be what's written to the selected theme's user-dir file

#### Scenario: Font Combo / custom-path InputText autosave
- **WHEN** the user picks a bundled font from the `Combo` widget
- **THEN** the `Combo` return-value-true SHALL trigger one `persistTheme` call carrying the new `themeFontFile()`
- **WHEN** the user finishes typing a custom path in the `Custom...` `InputText` (Enter or focus loss)
- **THEN** `ImGui::IsItemDeactivatedAfterEdit()` SHALL fire once and trigger one `persistTheme` call

#### Scenario: Reset reverts the selected theme to its bundled values and persists
- **WHEN** the user clicks `Reset to defaults` while a bundled-origin theme is selected
- **THEN** the dialog SHALL call `app::resetSelectedTheme()` which: (a) deletes the user-dir copy at `<config-dir>/imgui-shell/themes/<selected>.json` (silent if not present), (b) re-applies the bundled `assets/themes/<selected>.json` content to `ImGui::GetStyle()`
- **AND** the in-memory style SHALL match the bundled baseline for the selected theme
- **AND** `theme.json` SHALL NOT change (selection still points at the same theme)

#### Scenario: Reset on a user-only theme is a no-op with stderr log
- **WHEN** the selected theme has no bundled origin (a user-only entry without a matching file in `assets/themes/`) and the user clicks `Reset to defaults`
- **THEN** `resetSelectedTheme` SHALL log exactly one stderr line of the form `Reset: '<name>' has no bundled origin; no action taken`
- **AND** the user-dir file SHALL NOT be deleted
- **AND** `ImGui::GetStyle()` SHALL be unchanged

#### Scenario: No Save / Discard buttons rendered
- **WHEN** the Theme tab is open
- **THEN** the body SHALL NOT render a button labeled `Save`, `Discard`, or `Discard unsaved changes`
- **AND** the only button at the bottom of the Theme tab SHALL be `Reset to defaults`
