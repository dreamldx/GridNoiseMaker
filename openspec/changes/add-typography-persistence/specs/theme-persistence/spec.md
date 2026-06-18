## MODIFIED Requirements

### Requirement: JSON schema and parsing rules
The theme config file SHALL be a single top-level JSON object with three optional keys: `colors` (object: `ImGuiCol_*` enum name without prefix → 4-element float array `[r, g, b, a]` in 0..1 range), `metrics` (object: `ImGuiStyle` metric field name → either a single float or 2-element float array `[x, y]` depending on whether the field is scalar or `ImVec2`), and `typography` (object: typography keys for font family + pixel size — see scenarios below). Any key inside `colors`, `metrics`, or `typography` whose name begins with underscore (`_`) SHALL be silently ignored at load time — this provides a hook for inline documentation in hand-edited files.

#### Scenario: Colors are mapped via ImGui's canonical enum names
- **WHEN** `app::readThemeFromConfig` parses a `"colors"` key
- **THEN** each entry's key SHALL be matched against `ImGui::GetStyleColorName(i)` for `i` in `[0, ImGuiCol_COUNT)`
- **AND** known keys SHALL have their corresponding `style.Colors[i]` set to the parsed `ImVec4`
- **AND** unknown color keys SHALL each log one warning of the form `theme.json: unknown color key '<name>'; ignoring` and SHALL NOT modify `style`

#### Scenario: Metrics use a fixed set of supported field names
- **WHEN** `app::readThemeFromConfig` parses a `"metrics"` key
- **THEN** entries SHALL be matched against the supported metric field set (at minimum: `WindowPadding`, `FramePadding`, `ItemSpacing`, `ItemInnerSpacing`, `IndentSpacing`, `ScrollbarSize`, `GrabMinSize`, `WindowRounding`, `ChildRounding`, `FrameRounding`, `GrabRounding`, `PopupRounding`, `ScrollbarRounding`, `TabRounding`, `WindowBorderSize`, `ChildBorderSize`, `PopupBorderSize`, `FrameBorderSize`, `TabBorderSize`)
- **AND** known metric keys SHALL have their corresponding field on `style` set to the parsed value
- **AND** unknown metric keys SHALL each log one warning of the form `theme.json: unknown metric key '<name>'; ignoring`

#### Scenario: Typography section overrides font family and size
- **WHEN** `app::readThemeFromConfig` parses a `"typography"` key
- **THEN** entries SHALL be matched against the supported typography key set: `font_file` (string) and `font_size_px` (number)
- **AND** if `font_file` is a non-empty string AND the resolved path (relative to assets dir, or absolute) `std::filesystem::exists`, the parser SHALL call `app::setThemeFontFile(<resolved-path>)`; if the file is missing the parser SHALL log one warning of the form `theme.json: typography.font_file '<path>' does not exist; reverting to default` and SHALL NOT call the setter
- **AND** if `font_size_px` is a number, the parser SHALL clamp it to `[6.0f, 96.0f]`, log a warning if clamping was needed, and call `app::setThemeFontSizePx(<clamped>)`
- **AND** unknown typography keys SHALL each log one warning of the form `theme.json: unknown typography key '<name>'; ignoring`
- **AND** typography overrides apply at NEXT launch only — they affect `configureFontAtlas`, which runs once during `app::init` and does not live-rebuild

#### Scenario: Underscore-prefixed keys are silent
- **WHEN** the config file contains a key like `"_comment"`, `"_note"`, or any other underscore-prefixed name inside `colors`, `metrics`, or `typography`
- **THEN** the key SHALL be skipped without any log output
- **AND** the rest of the file SHALL load normally

#### Scenario: Malformed JSON logs once and leaves style untouched
- **WHEN** `app::readThemeFromConfig` cannot parse the file as valid JSON
- **THEN** the function SHALL log a single error to stderr identifying the parse-error location
- **AND** `style` SHALL be unchanged from its pre-call state
- **AND** the typography setters SHALL NOT be called (defaults remain in effect)
- **AND** the application SHALL continue running with the baked-in theme (no crash, no exit)

#### Scenario: Wrong-type values are skipped per key
- **WHEN** a color value is not a 4-element array of numbers, or a metric value is not a number (or 2-element array for `ImVec2` fields), or a typography value is the wrong type for its key
- **THEN** the offending key SHALL log one warning identifying the key + expected type
- **AND** the rest of the file SHALL continue loading
