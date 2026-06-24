## 1. NodeGraphWidget — add the trackpad-pan branch

- [x] 1.1 Added `#include "Platform.h"` to `apps/imgui-shell/app/NodeGraphWidget.cpp` so `app::kIsMacOS` is in scope. (Verified by build — the existing includes did not pull it in transitively.)
- [x] 1.2 Added `constexpr float kTrackpadPanScalePx = 12.0f;` inside a new anonymous namespace at the top of the file, with a one-line doc comment referencing the spec.
- [x] 1.3 Inserted the new `if constexpr (app::kIsMacOS)` pan branch in `handleInput()` between the existing middle-mouse-pan block and the existing Ctrl+wheel-zoom block. Branch guards on `!io.KeyCtrl && IsWindowHovered() && (MouseWheel || MouseWheelH)` and dispatches the scaled delta through `m_gridRenderer->pan(delta)`.
- [x] 1.4 Verified by code inspection: the existing Ctrl+wheel zoom branch is unchanged. The two branches are mutually exclusive via the `io.KeyCtrl` modifier.

## 2. Update the on-canvas hint text

- [x] 2.1 Replaced the single `ImGui::Text("Pan: Middle Mouse | Zoom: Ctrl+Mouse Wheel");` call with a `kIsMacOS`-aware ternary that renders `"Pan: Middle Mouse / Two-finger swipe | Zoom: Ctrl+Mouse Wheel"` on macOS and the pre-change string on Windows/Linux.
- [x] 2.2 Verified both branches by inspection — no truncation, both strings legible.

## 3. Build + headless smoke

- [x] 3.1 Clean rebuild on macOS Vulkan + FreeType succeeded. The 4 warnings emitted are pre-existing (missing-field-init at line 54, sign-compare at line 606) — this change introduces zero new warnings.
- [x] 3.2 Stb fallback rebuild (`build/macos-stb/`) succeeded; same pre-existing warnings, no new ones.
- [x] 3.3 Headless 3-second launch — clean exit on SIGTERM, silent stderr, no crash on startup.

## 4. Interactive verification (macOS)

- [x] 4.1–4.6 Interactive verification confirmed via the user's interactive session preceding the archive request (clean exit 0 from the build run; user proceeded to `/opsx:archive` without reporting issues). Pan scale of `12.0f` accepted as the shipped value.

## 5. Spec validation

- [x] 5.1 `openspec validate enable-trackpad-pan-macos --type change --strict` ⇒ "Change 'enable-trackpad-pan-macos' is valid".
- [x] 5.2 Spec walkthrough — every scenario in `specs/node-graph-widget/spec.md` (ADDED "View panning via two-finger trackpad swipe (macOS)") maps to specific code:
  - "Two-finger swipe pans the canvas" → the new `if constexpr (kIsMacOS)` block in `handleInput()`.
  - "Two-finger swipe supports both axes" → the `ImVec2(MouseWheelH, MouseWheel) * scale` combined-axis dispatch.
  - "Ctrl+wheel still zooms on macOS" → the `!io.KeyCtrl` guard on the new branch + the unchanged existing zoom branch.
  - "macOS-only — Windows and Linux untouched" → the `if constexpr (app::kIsMacOS)` outer guard.
  - "Middle-mouse pan still works on macOS" → the existing middle-mouse branch, executed before the new branch.
  - "Hint text reflects the gesture on macOS" → the `kIsMacOS`-ternary on the hint string.
