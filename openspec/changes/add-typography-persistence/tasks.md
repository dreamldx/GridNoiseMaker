## 1. Theme.h / Theme.cpp — constexpr defaults + runtime accessors

- [x] 1.1 In `app/Theme.h`, renamed `kThemeFontFile` → `kDefaultThemeFontFile`, `kThemeFontSizePx` → `kDefaultThemeFontSizePx` (both stay `inline constexpr`). `kThemeTextColor` unchanged. Added four function declarations: `themeFontFile()`, `themeFontSizePx()`, `setThemeFontFile(std::string)`, `setThemeFontSizePx(float)`. Header doc-comment explains the defaults-vs-current split.
- [x] 1.2 In `app/Theme.cpp`, added file-scope `g_themeFontFile` / `g_themeFontSizePx` initialized to the defaults. Accessor and setter implementations: setter passes empty string / NaN → revert to default.
- [x] 1.3 Verified by grep: no references to the renamed `kThemeFontFile` / `kThemeFontSizePx` remain in `apps/imgui-shell/`; only the new `kDefault*` names + accessor calls exist.

## 2. App.cpp wiring — configureFontAtlas reads accessors

- [x] 2.1 In `configureFontAtlas`, replaced `resolveAssetPath(kThemeFontFile)` with a block that first reads `app::themeFontFile()`, detects whether the value is absolute (`/...` or `...:\...`), and either uses it as-is or passes through `resolveAssetPath`.
- [x] 2.2 Size argument changed from `kThemeFontSizePx` to `app::themeFontSizePx()`.
- [x] 2.3 About dialog's "Default font size" line uses `app::themeFontSizePx()` so an override is visible by inspection.
- [x] 2.4 **Bonus**: promoted `resolveAssetPath` from App.cpp's anonymous namespace to a public `app::` API (declaration in App.h, definition outside the anonymous block in App.cpp) so ThemeStorage.cpp can call it without duplicating the resolution logic.

## 3. ThemeStorage — typography parse + write

- [x] 3.1 Added a `typography` block to `readThemeFromConfig` immediately after the `metrics` block. Iterates the object, skips underscore-prefixed keys silently, recognizes `font_file` (string) + `font_size_px` (number), warns once per unknown key.
- [x] 3.2 `font_file` parsing: resolves to absolute via `resolveAssetPath` (or honors absolute paths as-is), existence-checks the resolved form, logs `theme.json: typography.font_file '<path>' does not exist; reverting to default` if missing. **Critical fix:** stores the user's ORIGINAL (relative) input via `setThemeFontFile(raw)` — not the resolved form — so `configureFontAtlas` doesn't double-prefix the assets dir.
- [x] 3.3 `font_size_px` parsing: clamps to `[6.0f, 96.0f]` via `std::clamp`; logs the clamp transition if it occurred.
- [x] 3.4 Extended `writeThemeToConfig` with a `typography` object containing `font_file` (current accessor value) + `font_size_px` (current accessor value).

## 4. Asset fetching — JetBrains Mono Regular + Lato Regular

- [x] 4.1 Downloaded `JetBrainsMono-Regular.ttf` (270 KB) from `github.com/JetBrains/JetBrainsMono/raw/master/fonts/ttf/`. Confirmed via `file`: TrueType Font data.
- [x] 4.2 Downloaded `JetBrainsMono-OFL.txt` (4.4 KB) from the same repo root.
- [x] 4.3 Downloaded `Lato-Regular.ttf` (657 KB) from `github.com/google/fonts/raw/main/ofl/lato/`. **Substituted Lato for the originally-planned Roboto** — Google Fonts now ships Roboto as a variable font only; Lato is a clean static-Regular OFL-licensed alternate sans that serves the same role. Spec deltas updated to match.
- [x] 4.4 Downloaded `Lato-OFL.txt` (4.4 KB) from the same path.
- [x] 4.5 Copied all four files into `apps/imgui-shell/assets/fonts/`. Existing desktop post-build asset-copy step picks them up automatically (verified in `build/macos/platform/desktop/assets/fonts/`).
- [x] 4.6 Updated `platform/ios/CMakeLists.txt`'s `IMGUI_SHELL_IOS_FONTS` list to include the new font + license files. iOS bundle will contain Resources/fonts/ with all six entries.

## 5. Example theme.json — typography section

- [x] 5.1 Added a top-level `typography` object to `example-theme.json` with `_comment` listing all three bundled fonts (`Inter-Regular.ttf` default, `JetBrainsMono-Regular.ttf`, `Lato-Regular.ttf`), `font_file` set to `"fonts/Inter-Regular.ttf"`, `font_size_px` set to `14.0`. Section is PRESENT (not absent) so it serves as inline schema documentation.

## 6. Verification

- [x] 6.1 Clean rebuild on macOS: succeeded (3 build steps after the path-fix rebuild). No compile warnings, no link errors.
- [ ] 6.2 Stb fallback rebuild (`build/macos-stb/`) — not separately rebuilt this session; same source tree, same expected behavior.
- [x] 6.3 Validation-layer smoke (default config): launched the binary for 3s under validation with the example theme.json copied to the config path. Stderr silent — accessors return defaults, font loads correctly.
- [x] 6.4 **Automated default-behavior test**: launched the binary with the example theme.json (typography defaults). 3s clean, stderr silent. Spec scenario "Default font loaded on init" verified.
- [x] 6.5 **Automated JetBrainsMono override test**: edited config to set `font_file = "fonts/JetBrainsMono-Regular.ttf"` and `font_size_px = 13.0`. Relaunched. 3s clean, stderr silent. **Caught the double-prefix path-resolution bug here** — initial implementation stored the resolved path in ThemeStorage's setter, causing configureFontAtlas to resolve a second time. Fixed by storing the user's original (relative) input; existence check still uses the resolved form. Visual confirmation of the actual font swap is interactive (task 6.5 visual eyeball — see below).
- [x] 6.6 **Automated missing-font warning test**: set `font_file = "fonts/DoesNotExist.ttf"`. Stderr contained exactly one line: `theme.json: typography.font_file 'assets/fonts/DoesNotExist.ttf' does not exist; reverting to default`. App survived 3s. Font stayed as default Inter.
- [x] 6.7 **Automated size-clamp warning test**: set `font_size_px = 200.0`. Stderr contained exactly one line: `theme.json: typography.font_size_px 200 out of range [6, 96]; clamping to 96`. App survived 3s. (Visual confirmation that text is huge is interactive — see below.)
- [ ] 6.5-visual / 6.7-visual **Interactive visual confirmation (REQUIRED before archive)**: launch the binary with JetBrainsMono override active. Confirm the menu bar font is the monospace JetBrains Mono (different glyph shapes than Inter). Then bump `font_size_px` to e.g. 28 and relaunch — confirm visibly larger text. **PENDING USER INTERACTION.**
- [x] 6.8 Spec walkthrough — `theme-persistence` MODIFIED "JSON schema and parsing rules" scenario "Typography section overrides font family and size" maps directly to the parser block; unknown-key path verified by code review (any key other than `font_file` / `font_size_px` / `_<prefix>` logs one warning).
- [x] 6.9 Spec walkthrough — `font-rendering` REMOVED "Font configuration is a build-time decision" (delta valid); MODIFIED "Bundled default font" scenarios all map to code (accessors in configureFontAtlas, three bundled fonts at expected paths, asset-missing fallback unchanged from prior change). `IMGUI_SHELL_DEFAULT_FONT_PX` compile-def confirmed absent: `grep -nE "IMGUI_SHELL_DEFAULT_FONT_PX" apps/imgui-shell/ → no matches`.
- [x] 6.10 Spec walkthrough — `gui-theme` REMOVED "Theme is a build-time decision in v1" (delta valid); MODIFIED "Theme owns typography defaults" reflects the accessor split (constexpr defaults + mutable runtime values).
- [x] 6.11 `openspec validate add-typography-persistence --type change --strict` ⇒ "Change 'add-typography-persistence' is valid".
- [ ] 6.12 iOS bundle verification — **DEFERRED: requires full Xcode**. Six font / license files added to `IMGUI_SHELL_IOS_FONTS`; runtime verification waits for sub-change 3 + Xcode.
