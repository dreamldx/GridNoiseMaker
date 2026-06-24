## Context

`NodeGraphWidget::handleInput()` (in `apps/imgui-shell/app/NodeGraphWidget.cpp`) currently handles two canvas gestures:
- **Middle-mouse drag → pan** via `m_gridRenderer->pan(delta)`.
- **Ctrl + mouse wheel → zoom** about the cursor via `m_gridRenderer->zoom(io.MouseWheel, io.MousePos)`.

Plain mouse-wheel scroll (no Ctrl) currently does **nothing** — the wheel event is ignored. That's the gap this change fills on macOS: a two-finger trackpad swipe should pan the canvas, mapping into the same `ViewTransform::pan` path the middle-mouse drag uses.

Three constraints frame the design:
- **macOS-only.** The user explicitly asked for "don't touch other platforms." Wrap the new logic in `if constexpr (kIsMacOS)` — Windows / Linux see no change.
- **The existing Ctrl+wheel zoom path is preserved.** A user holding Ctrl on a Mac trackpad still zooms (the modifier wins). Without Ctrl, the same delta now pans.
- **Discriminator-free path on macOS.** Since plain-wheel without Ctrl is unused, we don't even need the fractional-vs-integer heuristic the proposal mentioned — ANY scroll delta on macOS without Ctrl can route to pan. This is simpler than the proposal's heuristic and has no false-positive Magic Mouse problem.

## Goals / Non-Goals

**Goals:**
- Two-finger trackpad swipe on the `NodeGraphWidget` canvas pans the view, on macOS only.
- Pan direction matches the OS-level natural-scroll preference (we just use the raw `io.MouseWheel*` sign — GLFW already honors the system setting).
- Both axes supported (`MouseWheelH` for horizontal swipes, `MouseWheel` for vertical).
- Existing middle-mouse pan and Ctrl+wheel zoom continue to work unchanged.

**Non-Goals:**
- No platform changes outside macOS.
- No pinch-to-zoom (would need a separate Cocoa-side gesture handler outside GLFW).
- No fractional-vs-integer wheel discrimination — the modifier (Ctrl) is the entire discriminator.
- No inertia / momentum modeling (the OS already delivers a tail of decreasing-magnitude continuation events; we just consume each one as it arrives).
- No tunable from the Preferences dialog.

## Decisions

### D1: macOS-only `if constexpr` branch in `handleInput()`
Inside `NodeGraphWidget::handleInput()`, add a new branch AFTER the middle-mouse-pan block and BEFORE the Ctrl+wheel-zoom block:

```cpp
if constexpr (kIsMacOS) {
    if (!io.KeyCtrl && ImGui::IsWindowHovered() &&
        (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f)) {
        const ImVec2 delta(
            io.MouseWheelH * kTrackpadPanScalePx,
            io.MouseWheel  * kTrackpadPanScalePx);
        m_gridRenderer->pan(delta);
    }
}
```

The `!io.KeyCtrl` guard hands the event to the existing zoom branch when Ctrl is held; otherwise the trackpad swipe consumes it as pan. The `IsWindowHovered` guard matches the existing pan / zoom branches.

- **Alternatives:**
  - **Use the fractional-delta heuristic** (proposal v1 idea): adds code complexity, and the Magic Mouse misclassification concern. With the modifier (Ctrl) as the discriminator, we don't need the heuristic.
  - **Replace Ctrl+wheel zoom entirely with pan, require ⌘+wheel for zoom on Mac:** changes existing behavior; needs a separate spec update. Out of scope for v1.

### D2: Pan scale factor
File-scope `constexpr float kTrackpadPanScalePx = 12.0f;` declared in the anonymous namespace of `NodeGraphWidget.cpp`. The 12 px figure matches the visual feel of a native `NSScrollView` and gives a comfortable "one notch = ~12 px world delta" baseline. The scale interacts with the current zoom level naturally (a zoomed-in canvas pans the same screen distance for the same gesture, just covering less world distance — the existing `ViewTransform::pan` already handles screen→world conversion).

If the scale feels wrong during interactive verification, bump it to `8` or `16`; trivial tweak in the same file.

- **Alternatives:**
  - **No scale (delta as-is):** GLFW's macOS trackpad deltas are in "lines" not pixels — typically values like `0.4`–`2.0` per frame. Without scaling, a vigorous two-finger swipe barely moves the canvas. Rejected.
  - **Scale-by-zoom:** `delta * (kBasePan / zoom)` so the world-space pan distance is constant across zoom levels. Not needed because `ViewTransform::pan` is already screen-space; the visual feel is "the canvas slides under my fingers" which is what users expect.

### D3: Update the hint text
The on-canvas hint at `NodeGraphWidget.cpp:329` currently reads:
```cpp
ImGui::Text("Pan: Middle Mouse | Zoom: Ctrl+Mouse Wheel");
```

On macOS, replace with:
```cpp
ImGui::Text("Pan: Middle Mouse / Two-finger swipe | Zoom: Ctrl+Mouse Wheel");
```

via a `kIsMacOS ? ... : ...` ternary (no new branch). One line change.

- **Alternatives:**
  - **Leave the hint alone:** users would discover the gesture by accident. Adding the hint costs almost nothing and avoids the discovery problem.

### D4: Both axes via `io.MouseWheel` + `io.MouseWheelH`
ImGui's GLFW backend forwards `glfwSetScrollCallback` events as `(xoffset, yoffset)` → `(io.MouseWheelH, io.MouseWheel)`. On macOS trackpads, horizontal two-finger swipes deliver `xoffset != 0`, vertical swipes deliver `yoffset != 0`, diagonal swipes deliver both. Mapping both to `ViewTransform::pan(ImVec2)` is one line.

The natural-scroll convention is handled at the OS level — GLFW respects the macOS system preference. If the user has "natural scrolling" off, the sign of the deltas flips, and the pan direction flips with it. We do nothing to override.

## Risks / Trade-offs

- **[Risk: Magic Mouse users lose plain-wheel-zoom — except plain wheel didn't zoom before either.]** → Non-issue. Both Magic Mouse and trackpad behave identically; both now pan when Ctrl is NOT held. Both still zoom on Ctrl+wheel.
- **[Risk: Two-finger swipe is consumed by overlapping ImGui scrollbars (e.g., docked panels) BEFORE reaching the node graph.]** → Acceptable: `ImGui::IsWindowHovered()` gates the new code path to canvas-focus only. If the cursor is over a panel, the panel's scroll behavior wins, which is what the user wants.
- **[Risk: GLFW on older macOS versions delivers two-finger swipe as continuous discrete events at very high rate (~120 Hz).]** → Each event arrives at `app::frame` rate and is integrated by `m_gridRenderer->pan` in screen-space; no integration math needed. The 12-px scale is per-event, not per-second, so a longer swipe naturally results in more pan distance.
- **[Trade-off: The hint text now says "Two-finger swipe" only on macOS, "Middle Mouse" only on Windows/Linux.]** → Different platforms see different hints. That's correct UX — Windows/Linux users would be confused by a Mac-specific gesture name.
- **[Trade-off: No discoverability for ⌘+wheel zoom on Mac (it doesn't exist; Ctrl+wheel is what works).]** → Existing inconsistency; ⌘ should arguably be the Mac modifier. Out of scope; a follow-up if anyone asks.

## Migration Plan

1. Land the change. macOS users immediately see the new pan gesture; Windows/Linux unchanged.
2. No rollback work needed — removing the macOS branch restores the pre-change behavior on Mac (trackpad swipe → no-op).
3. No assets, no schema, no IPC — just code.

## Open Questions

- **Is `12.0f` the right `kTrackpadPanScalePx`?** Pick by interactive feel. Tunable in one place if wrong.
- **Should the pan be scaled DOWN at high zoom levels to slow pan-when-zoomed-in?** Currently the canvas pans the same screen distance regardless of zoom — at 10x zoom, the user moves through less world space per gesture, which feels right ("I'm dragging the visible window"). No scaling needed unless interactive feel suggests otherwise.
- **Should this also map to Windows precision touchpads (which also deliver fractional deltas)?** Out of scope per user's explicit "macOS-only" decision. Follow-up if anyone asks.
