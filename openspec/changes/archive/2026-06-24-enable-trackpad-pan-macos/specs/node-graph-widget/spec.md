## ADDED Requirements

### Requirement: View panning via two-finger trackpad swipe (macOS)
On macOS, the `NodeGraphWidget` canvas SHALL pan when the user performs a two-finger swipe gesture on a trackpad (or Magic Mouse), routing the gesture's scroll deltas to `ViewTransform::pan` via the existing `SimpleGridRenderer::pan` API. The gesture SHALL be active only when (a) the canvas window is hovered, (b) the Ctrl key is NOT held, and (c) the platform is macOS. On Windows and Linux, scroll events without the Ctrl modifier SHALL continue to be ignored by the canvas (the existing behavior is preserved). The Ctrl+wheel zoom path SHALL be unchanged on every platform.

#### Scenario: Two-finger swipe pans the canvas
- **WHEN** the user performs a two-finger swipe gesture inside the hovered `NodeGraphWidget` canvas on macOS, without holding Ctrl
- **THEN** the GLFW scroll callback SHALL deliver `(io.MouseWheelH, io.MouseWheel)` deltas to the per-frame `handleInput()`
- **AND** the dialog SHALL compute `delta = ImVec2(io.MouseWheelH * kTrackpadPanScalePx, io.MouseWheel * kTrackpadPanScalePx)` where `kTrackpadPanScalePx` is a file-scope constant (initially `12.0f`)
- **AND** SHALL call `m_gridRenderer->pan(delta)` exactly once per frame
- **AND** the visible canvas SHALL shift accordingly, with the direction matching the macOS system "natural scrolling" preference (GLFW honors the OS setting, no per-app override)

#### Scenario: Two-finger swipe supports both axes
- **WHEN** the user swipes diagonally (both axes contributing)
- **THEN** the pan delta SHALL include both horizontal (`io.MouseWheelH`) and vertical (`io.MouseWheel`) components in the single `m_gridRenderer->pan` call
- **AND** a pure-horizontal swipe SHALL pan horizontally with no vertical movement
- **AND** a pure-vertical swipe SHALL pan vertically with no horizontal movement

#### Scenario: Ctrl+wheel still zooms on macOS
- **WHEN** the user holds Ctrl while performing a wheel scroll or two-finger swipe on macOS
- **THEN** the new pan branch SHALL be skipped (the `!io.KeyCtrl` guard fails)
- **AND** the existing Ctrl+wheel zoom branch SHALL fire instead, calling `m_gridRenderer->zoom(io.MouseWheel, io.MousePos)`
- **AND** the canvas SHALL zoom about the cursor as before

#### Scenario: macOS-only — Windows and Linux untouched
- **WHEN** a Windows or Linux user performs the same scroll gesture without Ctrl
- **THEN** the new pan branch SHALL NOT execute (the `if constexpr (kIsMacOS)` guard excludes it at compile time on those targets)
- **AND** the canvas SHALL behave exactly as before (plain wheel without Ctrl is a no-op on the canvas)
- **AND** no `kTrackpadPanScalePx` constant SHALL be referenced by code paths reachable from non-macOS targets (it's defined unconditionally but only consumed inside the `if constexpr` branch, which the compiler dead-strips)

#### Scenario: Middle-mouse pan still works on macOS
- **WHEN** the user drags with the middle mouse button on macOS
- **THEN** the existing middle-mouse-pan branch SHALL fire (it runs before the new trackpad branch)
- **AND** the canvas SHALL pan via the same `m_gridRenderer->pan` API
- **AND** the trackpad branch SHALL NOT fire concurrently (the middle-mouse branch is independent — both branches contribute pan deltas to the same `pan()` call if both gestures somehow happen in the same frame, with no interference)

#### Scenario: Hint text reflects the gesture on macOS
- **WHEN** the on-canvas hint text is rendered on macOS
- **THEN** it SHALL read `"Pan: Middle Mouse / Two-finger swipe | Zoom: Ctrl+Mouse Wheel"` (or equivalent wording that mentions both pan affordances)
- **WHEN** the hint text is rendered on Windows or Linux
- **THEN** it SHALL continue to read `"Pan: Middle Mouse | Zoom: Ctrl+Mouse Wheel"` (the pre-change text, unchanged)
