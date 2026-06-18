## Why

ImGui's default font rasterizer (`stb_truetype`) produces serviceable text but is noticeably soft at small sizes and non-integer DPI scales — the App > About dialog and menu bar are the first surface a contributor sees, so blurry text is the wrong first impression. ImGui ships an opt-in **FreeType** backend (`misc/freetype/imgui_freetype.cpp`) that delivers sharper glyph outlines, real hinting, and subpixel-accurate rendering for the cost of one extra build-time dependency. Switching now (while the shell is still small) is far cheaper than retrofitting later.

## What Changes

- Add **FreeType** as a build-time dependency, fetched via CMake `FetchContent` and pinned by tag, available on all four platform targets (desktop and iOS — same atlas code runs everywhere).
- Compile `imgui_freetype.cpp` from the existing fetched ImGui sources into the existing `imgui` static library (no new top-level target).
- In `app::init`, before any font is uploaded, switch the ImGui font atlas builder to FreeType via `io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType()` (or the equivalent post-1.91 API), and apply default builder flags (`ImGuiFreeTypeBuilderFlags_LightHinting` as the conservative starting point).
- Load one bundled default font (e.g., a stripped Roboto / Inter variant under `apps/imgui-shell/assets/fonts/`) at a sensible default pixel size; the v1 default ImGui font remains as a fallback if asset loading fails.
- Expose a small, documented config surface on the host (compile-time defaults; not a runtime settings panel) so contributors can choose hinting mode (`Default`, `Light`, `Mono`, `None`) and toggle subpixel rendering.
- Update the About dialog to show the active font-rasterizer backend (`"FreeType X.Y.Z"` vs `"stb_truetype"`) so the change is visible-by-inspection per `ui-sample` style.

Non-goals (called out to bound scope):
- No runtime font picker / no settings panel.
- No multi-font merging (icon fonts, CJK ranges) — separate change.
- No SDF-based rendering — overkill at this scope.
- No color emoji.

## Capabilities

### New Capabilities

- `font-rendering`: How the application loads and rasterizes fonts (rasterizer choice, builder flags, default font, fallback behavior). Owns the FreeType-vs-stb_truetype decision and the contract on which is active at runtime.

### Modified Capabilities

- `build-system`: New requirement — FreeType is fetched and linked on every supported preset; `imgui_freetype.cpp` is compiled into the `imgui` static library.

## Impact

- **New third-party dependency:** [FreeType](https://gitlab.freedesktop.org/freetype/freetype) (FreeType License or GPLv2 — dual-licensed, MIT-compatible under the FreeType License). Adds ~600 KB to the static link on desktop; smaller on iOS (system FreeType may be link-time available but we'll vendor for consistency).
- **Code changes:** `cmake/Dependencies.cmake` (FetchContent + add `imgui_freetype.cpp` to the `imgui` target), `app/App.cpp` (configure atlas builder + load default font), `apps/imgui-shell/assets/fonts/` (new asset directory and one bundled `.ttf`).
- **Spec changes:** new `openspec/specs/font-rendering/spec.md`; modified `openspec/specs/build-system/spec.md` (new "FreeType dependency" requirement).
- **CI:** no new install steps — FreeType is vendored via FetchContent, same as ImGui and GLFW. Build time grows slightly (~5–10s) on first configure.
- **Backward compatibility:** none required — this is the v1 shell, no prior font config exists.
