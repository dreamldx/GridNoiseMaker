## Context

The shell currently renders text through ImGui's default `stb_truetype`-based atlas builder. This produces grayscale-antialiased glyphs that are serviceable but visibly soft, especially at non-integer DPI scales (Retina macOS, common Windows fractional-scaling setups) and at the small pixel sizes that menu bars and About-dialog body text occupy. The recently-archived `imgui-cross-platform-app` change pinned ImGui at `v1.91.9-docking`, which has a stable opt-in **FreeType** font builder under `misc/freetype/imgui_freetype.{h,cpp}`. The architecture is already friendly to swapping the builder — font atlas configuration lives in `app::init` inside the shared `app/` core, so the swap costs at most a handful of lines + one new dependency.

Constraints we must respect:
- C++17 baseline, four platform targets (macOS arm64/x86_64, Linux x86_64, Windows x86_64, iOS arm64) — the chosen FreeType must build cleanly on all four.
- No new runtime configuration UI in v1 (per proposal Non-Goals).
- Single-source-set rule from `app-shell` spec: no per-platform `#ifdef` blocks in `app/` source files.

## Goals / Non-Goals

**Goals:**
- Replace the default font rasterizer with FreeType in `app::init` such that ImGui menu / About / demo text renders visibly sharper, with the same single line of host-side change wiring up the atlas builder.
- Vendor one bundled default font under `apps/imgui-shell/assets/fonts/` so the look is identical on every platform (no reliance on host fonts).
- Keep the build system change minimal: one new `FetchContent_Declare` + adding `imgui_freetype.cpp` to the existing `imgui` static library target. No new top-level CMake target.
- Make the active rasterizer visible in the About dialog so the change is verifiable by inspection.

**Non-Goals:**
- No runtime font selector / settings panel.
- No multi-font merging (icon fonts, CJK ranges, color emoji) — separable follow-on change.
- No SDF / MSDF rendering — overkill at this scope.
- No font configuration knobs exposed to `app/` callers beyond a default — hinting mode is a compile-time default decided here, not user-tunable.

## Decisions

### D1: Use ImGui's built-in `imgui_freetype.cpp` rather than a hand-rolled wrapper
ImGui ships a maintained FreeType backend under `misc/freetype/` in the same repo we already fetch via `FetchContent`. It tracks the upstream `ImFontAtlasBuilderIO` interface as ImGui evolves, handles per-glyph subpixel positioning, exposes builder flags for hinting, and is the configuration the ImGui maintainer recommends. Writing our own is gratuitous.
- **Alternatives:**
  - **Hand-rolled FreeType glue:** Reinvents tested code, ties us to ImGui internals we don't control.
  - **HarfBuzz + custom shaping:** Out of scope; we don't need complex script shaping in v1.

### D2: Fetch FreeType via CMake `FetchContent`, pinned to a tag, on all four platforms
Match D2 from the prior `imgui-cross-platform-app` design: `FetchContent` keeps the source tree clean and pins the version in `cmake/Dependencies.cmake`. Pin to a known tag — `VER-2-13-3` is the latest as of mid-2026 and builds with CMake out-of-the-box. Disable FreeType's optional dependencies we don't need (`FT_DISABLE_ZLIB`, `FT_DISABLE_BZIP2`, `FT_DISABLE_PNG`, `FT_DISABLE_HARFBUZZ`, `FT_DISABLE_BROTLI`) to keep the binary lean — we only need TTF / OTF rasterization, no subsetting / compressed-PNG glyphs.
- **Alternatives:**
  - **System `find_package(Freetype)`:** Different versions per Linux distro / brew install; reproducibility suffers. Acceptable as a fallback path on Linux if a contributor opts in via CMake cache, but not the default.
  - **vcpkg / Conan:** Adds a tool to the contributor prerequisites; we rejected this for ImGui in the prior design and the same rationale applies here.

### D3: Bundle Inter as the default font (`assets/fonts/Inter-Regular.ttf`)
- Inter (by Rasmus Andersson) is designed specifically for screen rendering at small sizes — exactly our use case.
- Licensed under the SIL Open Font License 1.1 (permissive, allows redistribution as part of any program with no per-instance attribution requirement). License file is committed alongside.
- One regular weight (no bold/italic) for v1 — keeps the bundle small (~300 KB).
- **Alternatives:**
  - **Roboto Regular:** Equally good; Apache 2.0. Picked Inter because it's narrower at small body sizes and slightly more legible at 13–15 px (typical menu sizes).
  - **System font:** No — defeats the "identical look across platforms" goal and complicates the build.
  - **Ship no font; rely on ImGui's default Proggy Clean:** Stays bitmap-y; defeats the whole purpose of this change.

### D4: Default builder flags = `ImGuiFreeTypeBuilderFlags_LightHinting`
- `LightHinting` enables FreeType's "light" hinting (vertical only), which produces crisp text without the heavy horizontal distortion of "Default" (full) hinting. This is the same default Mozilla / Chromium use for screen text on macOS / Linux.
- We do NOT enable `LCD` (subpixel RGB) by default — it requires gamma-correct compositing the swapchain may not be configured for, and on macOS Retina the per-channel benefit is invisible. Leaving as a documented opt-in.
- We do NOT enable `MonoHinting` (1-bit) — kills antialiasing.
- **Alternatives:**
  - **Default (full hinting):** Sharper at integer sizes, worse at fractional DPI.
  - **None (no hinting):** Softer; FreeType's whole point is the hinting.
  - **LCD subpixel:** Beautiful on correctly-configured displays, broken-looking otherwise. Defer.

### D5: Atlas configuration owned by `app::init` (no per-host code path)
The atlas builder + default-font loading happens once, inside `app::init`, before any platform backend uses the atlas. This keeps `platform/desktop/` and `platform/ios/` unchanged. The font asset is loaded via `io.Fonts->AddFontFromFileTTF` with a path resolved relative to the executable's CMake-defined `IMGUI_SHELL_ASSETS_DIR` macro (desktop: build-output `assets/` directory copied next to the binary; iOS: assets baked into the `.app` bundle's `Resources/`).
- **Alternatives:**
  - **Per-platform font path resolution in `platform/*/`:** Pushes asset path logic out of the shared core for no benefit; `app/` can resolve paths via a constexpr / compile-def cleanly.
  - **Embed the .ttf as a C array via xxd:** No source-tree friendly, harder to swap fonts.

### D6: About dialog reflects active rasterizer via a compile-time tag
We set `target_compile_definitions(imgui_shell_app PRIVATE IMGUI_SHELL_FONT_BACKEND="FreeType")` (or `"stb_truetype"` if the build opts out via a `-DIMGUI_SHELL_USE_FREETYPE=OFF` cache option). `App.cpp`'s About dialog displays this string. Inspection at runtime confirms the build wired up the right backend.

### D7: Keep `stb_truetype` reachable via CMake option, default OFF
A single CMake option `IMGUI_SHELL_USE_FREETYPE` (default ON) lets a contributor disable FreeType for diagnosis or build-time comparison without surgery. When OFF, the build skips the FetchContent and `imgui_freetype.cpp` compile and falls back to ImGui's default atlas builder.

## Risks / Trade-offs

- **[FreeType build time grows ~5–10s on first configure]** → Mitigation: FetchContent caches across builds; clean-rebuild cost only.
- **[FreeType has a dual-license (FreeType License / GPLv2); we redistribute under the FreeType License]** → Mitigation: include the upstream `docs/FTL.TXT` and our top-level NOTICE on first commit that references both this and Inter's OFL.
- **[Inter SIL OFL has a reserved-font-name clause for "Inter"]** → Mitigation: ship the file unmodified at its canonical name (`Inter-Regular.ttf`); do not subset and rename.
- **[Static-link bloat: ~600 KB FreeType, ~300 KB Inter]** → Acceptable for a tool/utility shell; will revisit if we hit a real binary-size SLA.
- **[FreeType's hinting flags interact subtly with HiDPI display scaling]** → Mitigation: `LightHinting` is the safest default; spec scenarios call out exact configuration.
- **[ImGui FreeType API changed between 1.89 and 1.90+; we pin 1.91.9-docking]** → Pin is fixed; if/when we bump ImGui we re-verify the FreeType atlas init call. Documented in the spec.
- **[Asset path resolution differs desktop vs iOS]** → Mitigation: D5 puts the path logic behind a single compile-def; the .cpp stays uniform.

## Migration Plan

Not applicable in the strict sense — this change is additive over the v1 shell. Rollback = set `-DIMGUI_SHELL_USE_FREETYPE=OFF` and rebuild; the default ImGui atlas builder + bundled `ImGui::AddFontDefault` Proggy is reachable in one configure step. No data migration. No persisted state changes.

## Open Questions

- **Font size default:** 14 px feels right for macOS Retina; needs an eyes-on check on Windows non-Retina before locking the value into the spec. Will likely add a small `IMGUI_SHELL_DEFAULT_FONT_PX` compile-def with a documented default.
- **Should the demo window get a separate bold weight (Inter-Bold)?** Deferred — out of scope for v1; can be a follow-on.
- **iOS asset bundling path:** Confirmed in concept (CMake `RESOURCE` property on the bundle target) but unverified locally until full Xcode is available. Documented in tasks.
