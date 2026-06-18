## Why

`add-theme-persistence` (just archived) lets users persist colors + metrics tweaks to `~/.config/imgui-shell/theme.json` and have them re-applied on launch. But the typography — the font file (`Inter-Regular.ttf`) and pixel size (`14.0f`) — is still locked at compile time via `app::kThemeFontFile` / `app::kThemeFontSizePx` constexpr in `app/Theme.h`. A user who wants to try a different font or a different size has to edit a header and rebuild — even though the persistence layer is right there.

This change extends `theme.json` with a third top-level section, `typography`, alongside the existing `colors` and `metrics`. It also ships a couple of alternative fonts under `assets/fonts/` so the override has obvious targets. With this in place, a user can drop a different `.ttf` in their `~/.config/imgui-shell/` (or reference one of the bundled alternatives) and have it picked up at next launch — no rebuild required.

## What Changes

- **Convert typography constants from `constexpr` to mutable runtime defaults.** `kThemeFontFile` and `kThemeFontSizePx` in `app/Theme.h` change from `inline constexpr` to non-const variables exposed via accessor functions: `app::themeFontFile()` returns the current value (initialized to the previous Inter / 14 px defaults at startup), `app::setThemeFontFile(std::string)` / `app::setThemeFontSizePx(float)` mutate. `kThemeTextColor` stays `constexpr` (it's already covered by the `colors` JSON section under `ImGuiCol_Text`).
- **Extend `ThemeStorage` with a `typography` section.** The JSON schema gains a top-level `typography` object with optional keys `font_file` (string, relative to the assets path) and `font_size_px` (float). On read, valid values call the setters. On write, both values are serialized.
- **Wire `configureFontAtlas()` to read the live defaults.** Replace the two `kTheme*` constexpr references in `app/App.cpp::configureFontAtlas` with calls to the new accessors. The atlas is rebuilt from the current values each time `app::init` runs — which means a Save in the future Preferences dialog requires either an app relaunch or a font-atlas rebuild API. For v1, **relaunch is the contract** (matches the existing colors / metrics path — no live recomputation either; the persistence is a startup-state thing).
- **Ship two alternative fonts** under `apps/imgui-shell/assets/fonts/`: a monospace face (e.g., **JetBrains Mono Regular**, OFL) and an alternate sans (e.g., **Roboto Regular**, Apache 2.0). The example theme JSON gains a `typography` section that comments on the available alternatives.
- **Spec hygiene cleanup.** `font-rendering`'s "Font configuration is a build-time decision" requirement was already implicitly contradicted by `add-theme-persistence` (colors + metrics became runtime-persistable). This change REMOVES that requirement with a Reason/Migration note, replaced by the new theme-persistence-driven model. The `gui-theme` capability's "Theme is a build-time decision in v1" requirement gets the same treatment — runtime persistence is now the explicit story.
- **No iOS host wiring for typography-from-theme** this sub-change. Same constraint as `add-theme-persistence`: iOS reads its config from `Documents/` via `setDocumentsPath`; sub-change 3 (the parked `add-preferences-dialog`) is when the iOS host actually calls that setter. Until then, iOS keeps using the baked-in default Inter @ 14 px.

Non-goals (called out to bound scope):
- **No font picker UI.** That's the parked `add-preferences-dialog`. This change is JSON-only.
- **No live atlas rebuild on Save.** `writeThemeToConfig` writes the file; the new values apply on next launch. Live font swaps in mid-session would require destroying + recreating the ImGui font atlas + re-running `ImGui_ImplVulkan_CreateFontsTexture` + recreating the descriptor set — substantial scope; defer.
- **No font discovery (system fonts).** Only paths the user explicitly writes into `font_file` are honored; we don't enumerate system font directories.
- **No font fallback chain.** Single font, single size, single style. Multi-font merging is a future feature.
- **No subpixel rendering toggle in JSON.** Hinting / LCD configuration stays at compile time (the `font-rendering` "Default hinting configuration" requirement).

## Capabilities

### New Capabilities
<!-- None — extending existing capabilities. -->

### Modified Capabilities

- `theme-persistence`: The "JSON schema and parsing rules" requirement gets a new scenario for the `typography` section (parsing `font_file` string + `font_size_px` float; unknown / wrong-type values warn-and-continue same as colors / metrics). The "Theme persisted to per-OS config file" scenarios are unchanged in shape — the read order still puts user values on top of baked-in defaults.
- `font-rendering`: REMOVE the "Font configuration is a build-time decision" requirement (Reason: typography is now part of theme persistence; Migration: theme.json's `typography` section + `app::set*` accessors). MODIFY the "Bundled default font" requirement so its description acknowledges that the font file path is now overridable via theme persistence; the *default* (Inter-Regular at 14 px) is still bundled and used when no override exists.
- `gui-theme`: REMOVE the "Theme is a build-time decision in v1" requirement (Reason: contradicted by `add-theme-persistence`'s runtime persistence; the requirement was incorrectly preserved in that change's archive). The "Theme owns typography defaults" requirement stays — `app::themeFontFile()` / `themeFontSizePx()` are still defined in the theme module, just no longer `constexpr`.

## Impact

- **Code change:** ~70 LOC net.
  - `app/Theme.h` — accessors instead of `constexpr` for font file + size (~15 LOC).
  - `app/Theme.cpp` — backing storage for the mutable defaults + setter implementations (~20 LOC).
  - `app/ThemeStorage.cpp` — parse + write the `typography` section, including bounds-check on size (e.g., 6 px ≤ size ≤ 96 px) and a file-existence sanity check on the font path before calling the setter (~30 LOC).
  - `app/App.cpp` — update `configureFontAtlas` to use the accessors (~5 LOC).
- **New dependencies:** none. (Two new font asset files; no new code deps.)
- **New assets:** two .ttf files + their license files under `apps/imgui-shell/assets/fonts/`. ~500 KB each.
- **Spec change:** MODIFIED `theme-persistence` (one scenario added to the schema-and-parsing-rules requirement), MODIFIED `font-rendering` (REMOVE the build-time-decision requirement + MODIFY the bundled-default-font requirement), MODIFIED `gui-theme` (REMOVE the build-time-decision-in-v1 requirement).
- **CI:** no workflow changes.
- **Risk surface:** the main risk is users supplying a `font_file` path that doesn't exist or isn't a valid TTF. Mitigation: `readThemeFromConfig` checks `std::filesystem::exists(themeFontFile())` after the override is applied and logs a warning + reverts to the baked-in Inter default if the file doesn't exist. The atlas-load assertion that catches missing fonts also remains as a safety net.
- **Backward compatibility:** existing `theme.json` files without a `typography` section continue to work (the missing section is silently no-op, per the existing schema rules — underscore-prefix-ignored / missing-key-keeps-default semantics).
