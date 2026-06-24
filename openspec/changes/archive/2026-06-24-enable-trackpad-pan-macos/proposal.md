## Why

On macOS, the `NodeGraphWidget` canvas treats every scroll event as a zoom — including the two-finger trackpad swipe, which is the most natural "pan around a canvas" gesture in the macOS ecosystem (Finder, Preview, Xcode, every native graphics editor uses two-finger swipe for pan). The current behavior is functionally usable (you can pan by dragging with middle-mouse or whatever the canvas's pan affordance is), but feels wrong to anyone who's spent five minutes on a Mac.

The discriminator already exists in the event stream: GLFW delivers macOS trackpad scrolls as `hasPreciseScrollingDeltas == YES` events with **fractional** delta values (e.g., `0.43`, `1.27`), whereas a physical mouse wheel produces **integer** ticks (`1.0`, `-2.0`). The signal lets us route the same `glfwSetScrollCallback` event to "pan" or "zoom" based on which input device produced it, without adding any new platform API surface.

## What Changes

- **NodeGraphWidget receives a per-axis pan delta from trackpad swipes on macOS.** The scroll callback (via ImGui's `io.MouseWheel` / `io.MouseWheelH`) is read at the top of the node graph's per-frame input handling. On macOS, if the magnitude of either delta is fractional (i.e., not an integer-rounded mouse-wheel tick), the event is interpreted as a **pan** request and forwarded to `ViewTransform`'s existing pan API instead of the zoom path.
- **Mouse wheel behavior is preserved on macOS.** Plugging a USB mouse into a Mac and scrolling the wheel still produces integer ticks → still triggers zoom (the existing behavior). The change only adds a route for the fractional/precision case.
- **Other platforms are completely untouched.** The new logic is gated by `if constexpr (kIsMacOS)` in `NodeGraphWidget`. Windows and Linux scroll callbacks continue to map straight to zoom regardless of precision. (Windows precision touchpads also emit fractional deltas, but we're deliberately not changing their behavior in this scope — that's a future change if anyone asks.)
- **Pan velocity scaling.** The trackpad delta is a small floating-point value in scroll-line-equivalent units. Multiply by a tunable `kTrackpadPanScalePx` (initially `~12.0f`) so a one-line-equivalent two-finger swipe pans the canvas by ~12 pixels (matches the visual feel of a native macOS scroll-view).
- **Pan direction.** Natural-scrolling convention: dragging two fingers down moves the canvas content down (i.e., pans the view UP). This matches the macOS system setting for scroll direction. We follow `io.MouseWheel` sign convention directly and let the OS-level natural-scroll preference apply.

Non-goals (called out to bound scope):
- **No pinch-to-zoom.** Trackpad pinch gestures are NOT delivered as scroll events on GLFW; supporting them needs a separate Cocoa callback. Future change.
- **No two-finger swipe panning anywhere else.** Only the `NodeGraphWidget` canvas. Other docked panels keep their default ImGui scroll-area behavior (which already accepts two-finger swipe for scrollbars).
- **No Windows / Linux behavior change.** Precision touchpad detection on those platforms is a separate problem.
- **No "kinetic" / inertia handling.** macOS sends inertia phases as continuation scroll events with decreasing magnitude — we'll just pan with whatever delta arrives each frame; no special inertia model.
- **No preferences-dialog toggle.** Pan-vs-zoom is determined by event source only; no user knob.

## Capabilities

### New Capabilities

(none — this change modifies an existing widget capability)

### Modified Capabilities

- `node-graph-widget`: adds a "macOS trackpad two-finger swipe pans the canvas" requirement covering the precision-delta detection, the per-axis pan dispatch, and the unchanged-on-other-platforms invariant.

## Impact

- **Code change:** ~30–50 LOC net, all in `apps/imgui-shell/app/NodeGraphWidget.cpp`.
  - A small `static bool isMacTrackpadScroll(ImVec2 wheel)` helper (or inline check) that returns true on macOS when either delta has a non-integer magnitude.
  - In the canvas's per-frame input handler, branch on the helper: if true → call the existing `ViewTransform`'s pan API with `delta * kTrackpadPanScalePx`; otherwise → existing zoom path.
  - One file-scope `constexpr float kTrackpadPanScalePx = 12.0f;` for tuning.
- **New dependencies:** none. Uses existing `io.MouseWheel` / `io.MouseWheelH` from the ImGui GLFW backend.
- **Spec change:** MODIFIED `openspec/specs/node-graph-widget/spec.md` only.
- **CI:** no workflow changes.
- **Risk surface:**
  - **A user with a Magic Mouse** (which presents as a trackpad to macOS) will also pan instead of zoom. Acceptable — Magic Mouse is fundamentally a trackpad with a curve, and the gesture is the same.
  - **Floating-point comparisons** — checking "is fractional" via `std::floor(x) != x` works but treats e.g. `1.0000001` (FP noise from a discrete tick) as fractional. Mitigation: compare against a small epsilon (`std::abs(x - std::round(x)) > 1e-3f`).
  - **Zoom path becoming unreachable** if some trackpad driver always delivers integer values (rare, but possible). Mitigation: pinch-to-zoom (out of scope, future change) and "user can still zoom via the existing zoom affordance" (toolbar button / keyboard shortcut, whichever the node graph has). The non-goal of pinch is acceptable for v1.
- **Backward compatibility:** new behavior only on macOS, only for fractional-delta scrolls — every existing mouse-wheel zoom path keeps working. Headless tests / CI on Linux/Windows runners see no behavior change.
