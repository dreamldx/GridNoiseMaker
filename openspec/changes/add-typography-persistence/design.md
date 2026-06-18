## Context

`gui-theme` (archived 2026-06-16) introduced typography defaults via `inline constexpr` constants in `app/Theme.h`: `kThemeFontFile = "fonts/Inter-Regular.ttf"` and `kThemeFontSizePx = 14.0f`. Those constants were the right shape for v1 (single source of truth, no runtime state), but they prevent the very thing `add-theme-persistence` enabled for colors and metrics — the ability for a user to edit `~/.config/imgui-shell/theme.json` and see their tweak on next launch.

This change converts the two constants to mutable runtime defaults exposed via accessors, threads them through `ThemeStorage`'s JSON read/write paths, and ships two alternative fonts so the override has obvious targets. It also cleans up two stale spec requirements that `add-theme-persistence` left behind in conflict: `font-rendering`'s "Font configuration is a build-time decision" and `gui-theme`'s "Theme is a build-time decision in v1." Both were rendered factually wrong the moment `add-theme-persistence` allowed runtime persistence; removing them here is overdue spec hygiene.

The atlas rebuild semantics deliberately match the existing colors/metrics path: persistence is a **startup-state** mechanism, not a live-mutation mechanism. The future `add-preferences-dialog` (sub-change 3, parked) will likely need a live atlas-rebuild API for the font picker, but that's its problem; here we keep the contract simple — "values apply on next launch."

## Goals / Non-Goals

**Goals:**
- Make the font file path and font pixel size overridable via `theme.json` so users can switch fonts or change sizes without rebuilding.
- Ship two reasonable alternative fonts (one monospace, one alternate sans) so the override has concrete targets.
- Validate user-supplied values: missing font file → warn + revert; size out of sane bounds → warn + clamp.
- Clean up the two spec requirements that `add-theme-persistence` left in conflict (`font-rendering` and `gui-theme` build-time-decision requirements).
- Preserve backward compatibility — existing `theme.json` files without a `typography` section behave exactly as before.

**Non-Goals:**
- No font picker UI (sub-change 3, parked).
- No live atlas rebuild on Save (defer; matches colors/metrics contract).
- No system-font enumeration / no font discovery.
- No font fallback chain.
- No subpixel rendering / hinting toggles in JSON (those stay compile-time in `font-rendering`).
- No iOS host wiring for typography overrides this change.
- No localization-specific font selection.

## Decisions

### D1: Convert `kThemeFontFile` / `kThemeFontSizePx` from `constexpr` to runtime defaults via accessors
`app/Theme.h` exposes four new symbols:

```cpp
namespace app {

// Default values, used when no theme.json override is present.
constexpr const char* kDefaultThemeFontFile   = "fonts/Inter-Regular.ttf";
constexpr float       kDefaultThemeFontSizePx = 14.0f;

// Accessors return the current runtime value (initialized from the defaults
// at static-init time; mutated by setters below).
const std::string& themeFontFile();
float              themeFontSizePx();

// Setters used by ThemeStorage when applying theme.json overrides.
// Pass an empty string / NaN to revert to the default.
void setThemeFontFile(std::string path);
void setThemeFontSizePx(float px);

} // namespace app
```

The accessors return references / values from file-scope statics in `Theme.cpp`. `kDefaultThemeFontFile` / `kDefaultThemeFontSizePx` remain `constexpr` so they can be referenced as fallbacks without going through the accessor.

`kThemeTextColor` stays `constexpr` unchanged — it's the `ImGuiCol_Text` slot, already covered by the existing `colors` JSON section under `add-theme-persistence`. Nothing new for that.

- **Alternatives:**
  - **Globally-visible non-const variables** (`extern std::string g_themeFontFile;`): less encapsulation, easier to mutate from anywhere by accident. Rejected.
  - **Pass typography state through `RenderContext`** instead of file-scope statics: thematically consistent with `RenderContext` being the bag of "things app/ needs to know," but RenderContext is owned by the platform host and re-populated each frame — wrong lifetime for theme state. Rejected.

### D2: JSON schema — new `typography` object alongside `colors` + `metrics`

```json
{
  "colors":  { ... },
  "metrics": { ... },
  "typography": {
    "_comment":     "font_file is relative to the assets/ root. font_size_px in [6, 96].",
    "font_file":    "fonts/JetBrainsMono-Regular.ttf",
    "font_size_px": 16
  }
}
```

Both keys are optional. Missing key → keep current value (which is either the baked-in default or whatever was set earlier in the read flow). Same underscore-prefix-ignored rule as `colors` / `metrics`.

`font_file` is **relative to the resolved assets directory**, NOT relative to the `theme.json` file's location. This matches how the existing `app::resolveAssetPath` builds font paths in `configureFontAtlas`. Absolute paths in `font_file` are accepted as-is (e.g., a user pointing at a system font they downloaded).

`font_size_px` must be a number. Clamped to `[6.0f, 96.0f]` after parsing — anything outside warns and clamps rather than rejecting.

- **Alternatives:**
  - **Top-level `font_file` / `font_size_px` keys** (no nesting in `typography`): flatter but mixes unrelated namespaces. Rejected.
  - **Combined `font` string parsed CSS-style** (`"16px JetBrainsMono"`): magic format; harder to validate. Rejected.

### D3: Validation — file existence + size bounds + revert-on-fail
When `ThemeStorage::readThemeFromConfig` encounters a `typography` section:

1. Read `font_size_px` → if numeric, clamp to `[6.0f, 96.0f]`, call `setThemeFontSizePx`. If out of range BEFORE clamp, log one warning identifying the input + clamped result.
2. Read `font_file` → if non-empty string, resolve relative to assets dir, check `std::filesystem::exists`. If missing → log one warning + leave the default in place (don't call the setter). If present → call `setThemeFontFile`.
3. If both keys are present and `font_file` failed (missing file), the size override still applies. They're independent.

The existence check happens INSIDE `readThemeFromConfig` so by the time `configureFontAtlas` runs, the accessors return validated values. configureFontAtlas itself can stay simple.

- **Alternatives:**
  - **No existence check; trust the user:** delegates the error surface to ImGui's `AddFontFromFileTTF` assertion, which already handles missing fonts with our fallback to Proggy. Acceptable but the warning is clearer up front. Keeping the check.
  - **Hard-error on missing font (refuse the whole load):** punishes a typo by nuking the rest of the user's theme. Same anti-pattern we already rejected for `colors`/`metrics`. Rejected.

### D4: Atlas rebuild semantics — relaunch-only in v1
`configureFontAtlas` runs once in `app::init`. It reads the current accessor values and builds the atlas from them. If a future API (or sub-change 3's Save button) wants live font swaps, it needs to:

1. Call `vkDeviceWaitIdle` (font atlas textures may be in flight on the GPU).
2. Destroy `ImGui_ImplVulkan`'s font texture (via `ImGui_ImplVulkan_DestroyFontsTexture` / equivalent).
3. Clear and rebuild `io.Fonts->Clear()` + `AddFontFromFileTTF(...)` + `Build()`.
4. Re-call `ImGui_ImplVulkan_CreateFontsTexture()`.

That's a ~30 LOC sequence with real Vulkan synchronization concerns — large enough to spec separately. We don't do it here. The contract is: **save → relaunch → see new typography.** Documented in the spec scenarios.

- **Alternatives:**
  - **Live rebuild now**: substantial extra scope + Vulkan correctness work. Defer.
  - **Restart-the-app dialog after Save**: UX overhead; the user can just close and reopen. Not needed.

### D5: Alternative fonts shipped — JetBrains Mono Regular + Roboto Regular
- **JetBrains Mono Regular** (SIL OFL 1.1) — monospace family with explicit "designed for coding" provenance. Reasonable second monospace option for the playground.
- **Roboto Regular** (Apache 2.0) — Google's classic sans-serif. Different visual character from Inter; gives the user a concrete A/B option.

Both fetched from upstream releases at apply time, committed under `assets/fonts/`. License files (`JetBrainsMono-OFL.txt` and `Roboto-Apache-2.0.txt`) committed alongside. The example `theme.json`'s `typography` `_comment` lists the three bundled options with their filenames.

- **Alternatives:**
  - **Ship many fonts**: bloats the repo + binary for marginal gain. Two alternatives is enough to demonstrate the override; users can add more themselves.
  - **No alternatives, user supplies their own**: makes the feature feel hypothetical; the bundled options make it concrete. Worth the ~600 KB.

### D6: Spec-hygiene cleanup — REMOVE two stale requirements
- `font-rendering`'s "Font configuration is a build-time decision" — already factually wrong since `add-theme-persistence` allowed runtime persistence of colors + metrics. REMOVED here with Reason + Migration.
- `gui-theme`'s "Theme is a build-time decision in v1" — same issue. REMOVED here with the same Reason + Migration.

Both were preserved through `add-theme-persistence`'s archive (oversight at the time). Removing them is overdue. The migration story: theme persistence is now the supported runtime story; the build-time defaults are still where the baked-in values live.

### D7: Path resolution for `font_file` — relative to assets dir, absolute paths allowed
When `ThemeStorage` reads `font_file`, it:

1. If the string is empty → no-op.
2. If the string starts with `/` (POSIX) or contains `:\` (Windows) → treat as absolute; check `std::filesystem::exists` on the literal path.
3. Otherwise → join with `app::resolveAssetPath("")` (which produces the assets root) → check existence.

The accessor `themeFontFile()` returns the RESOLVED absolute path the setter received, so `configureFontAtlas` can pass it directly to `AddFontFromFileTTF` without further resolution.

- **Alternatives:**
  - **Relative-to-theme.json**: users might intuitively expect this. But the assets directory is where actual font files live; making the user trace through `~/.config/imgui-shell/` to find fonts is worse UX.
  - **Path validation via `std::filesystem::is_regular_file`** (stricter than `exists`): more correct but `exists` is sufficient for v1 — the FreeType / stb assertion catches not-a-font cases.

## Risks / Trade-offs

- **[Risk: user references a font that exists but isn't a valid TTF]** → Mitigation: existence check passes, then `AddFontFromFileTTF` returns nullptr and our existing `g_fontFallback` path kicks in (Proggy + warning in About). Acceptable; pre-existing safety net.
- **[Risk: user sets font_size_px way out of bounds, e.g., 1000]** → Mitigation: clamp to `[6, 96]` with one warning. Out-of-bounds attempts are bounded; no infinite-loop or memory-blowup risk.
- **[Risk: alternative font assets balloon the binary]** → Acceptable. ~600 KB total for two fonts; matches Inter's bundle cost.
- **[Risk: live-reload expectation — user saves and expects immediate effect]** → Mitigation: spec scenario explicitly documents "relaunch to apply"; the future Preferences dialog UI will need to communicate this if a Save button exists without a live-rebuild API.
- **[Risk: removing the build-time-decision requirements is a meaningful spec contract shift]** → Mitigation: explicit REMOVED + Reason + Migration in the delta. Future readers see the trail.

## Migration Plan

Not applicable in the runtime sense — additive. Old `theme.json` files without `typography` continue to work; the missing section is a silent no-op (same as missing `colors` / `metrics`).

Spec-side, the two REMOVED requirements are noted as such with Reason + Migration text in this change's delta. Anyone reading the archived `gui-theme` or `font-rendering` specs after this change will see the requirement is gone with a pointer at the new persistence story.

## Open Questions

- **Should we also persist `font_size_px` in the Theme.cpp baked-in defaults?** Currently `kDefaultThemeFontSizePx = 14.0f` is the only place. If someone wants a different baked-in default (say, 16 px), they edit `Theme.h`. That's fine.
- **Should the example `theme.json` include a working `typography` section by default?** I'm inclined toward YES — the section being PRESENT in the example (with values matching the baked-in defaults) is documentation. Versus an absent section being a "you can add this" hint. Going with PRESENT.
- **Should `font_size_px` accept fractional values like `13.5`?** Yes — float, not int. ImGui handles fractional sizes.
- **Should we offer a "fonts/" listing API for the future Preferences dialog to populate a font picker?** Out of scope; sub-change 3's problem.
