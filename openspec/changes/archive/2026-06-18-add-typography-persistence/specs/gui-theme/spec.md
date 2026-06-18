## REMOVED Requirements

### Requirement: Theme is a build-time decision in v1
**Reason**: Stale since `add-theme-persistence` (archived 2026-06-17) enabled runtime persistence of color + metric values via `~/.config/imgui-shell/theme.json`. The "no runtime theme selection, no runtime color editing" framing in this requirement is no longer accurate — runtime overrides of every value `applyTheme` sets are now supported by the `theme-persistence` capability. The requirement was preserved through `add-theme-persistence`'s archive in oversight; removing it here is overdue spec hygiene.

**Migration**: Runtime theme persistence is now the supported story. The baked-in curated `applyTheme` values remain the *defaults* (and are what users see on first launch with no `theme.json` present); user-saved values override at next launch. A `Help > Theme > ...` menu is still NOT a v1 goal — that's the parked `add-preferences-dialog` change's surface. For now, runtime editing happens by hand-editing `theme.json` directly; the file-based mechanism is enough for v1 without a UI on top.

## MODIFIED Requirements

### Requirement: Theme owns typography defaults (font family, size, text color)
The theme SHALL define three typography concepts in `app/Theme.h`:
- `kThemeTextColor` — a `constexpr ImVec4` used as the `ImGuiCol_Text` baseline (still a compile-time constant; this slot is also covered by the `colors` section of `theme.json` per `theme-persistence`).
- `kDefaultThemeFontFile` / `kDefaultThemeFontSizePx` — `constexpr` baked-in defaults for the font path and pixel size.
- `app::themeFontFile()` / `app::themeFontSizePx()` — accessor functions returning the CURRENT values (defaults initially; mutable via the matching setters when `theme-persistence` applies a `typography` override).
- `app::setThemeFontFile(std::string)` / `app::setThemeFontSizePx(float)` — setters used by `ThemeStorage`'s `readThemeFromConfig` when a valid `typography` override is parsed; out-of-band callers MAY also call them but are not expected to.

The font-rendering layer's font-atlas configuration SHALL source the font path + pixel size from the accessors (not the `constexpr` defaults directly), so `theme.json` overrides take effect. The theme's `ImGuiCol_Text` slot in `applyTheme` SHALL continue to be assigned from `kThemeTextColor` so the color and the baked-in typography defaults stay in sync within one source of truth.

#### Scenario: Font atlas uses theme accessors, not raw constants
- **WHEN** `app::init` configures the font atlas
- **THEN** `io.Fonts->AddFontFromFileTTF` SHALL be called with a path derived from `app::themeFontFile()` (NOT from `kDefaultThemeFontFile` directly) and a pixel size equal to `app::themeFontSizePx()` (NOT `kDefaultThemeFontSizePx` directly)
- **AND** when no `theme.json` override is active, those accessors return the baked-in default values
- **AND** the call SHALL NOT use the previously-existing `IMGUI_SHELL_DEFAULT_FONT_PX` compile definition (which was removed by `add-gui-theme`)

#### Scenario: Text color in style matches theme constant
- **WHEN** `app::applyTheme(style)` returns
- **THEN** `style.Colors[ImGuiCol_Text]` SHALL be equal to `app::kThemeTextColor`
- **AND** changing the constant in `Theme.h` SHALL be sufficient to change the foreground text color throughout the application as a *default* (no other code site sets `ImGuiCol_Text` to a literal value); `theme.json`'s `colors.Text` MAY override this default at next launch
