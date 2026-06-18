## Why

The current `Help > ImGui Demo` menu item toggles a `bool g_showDemoWindow` and the per-frame body calls `ImGui::ShowDemoWindow(&g_showDemoWindow)` — a regular non-modal ImGui window. Now that `enable-multi-viewport` is archived, that demo window can be dragged outside the host as a real secondary OS window, which is a useful infrastructure demonstration but also a surface area we don't actually want to ship as a long-term UX. The user wants `Help > ImGui Demo` to behave as a **modal dialog** — clicking the menu item opens a focus-capturing popup the user can dismiss with a Close button, with no draggable / detachable window left around afterward.

This change converts the menu item to an `ImGui::OpenPopup` + `ImGui::BeginPopupModal` flow. ImGui's `ShowDemoWindow` calls `ImGui::Begin` internally to create its own top-level window; that Begin cannot nest inside a `BeginPopupModal` block (ImGui's window stack doesn't allow it). The modal body therefore shows a brief **placeholder** explaining the constraint, with a Close button to dismiss — chosen explicitly by the user as the desired UX over the more-complex alternative of reproducing the demo content inline.

## What Changes

- Replace the `g_showDemoWindow` flag-toggle pattern with an `OpenPopup("ImGui Demo")` call when `Help > ImGui Demo` is picked.
- Per-frame, render the popup via `ImGui::BeginPopupModal("ImGui Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)`. Body content:
  - A short paragraph (2–3 lines) explaining that `ImGui::ShowDemoWindow` opens its own top-level window and can't render inside a modal popup due to ImGui's window-stack architecture.
  - A pointer to where to find the demo source (a one-liner referencing `<imgui_demo.cpp>` in the fetched ImGui sources).
  - A `Close` button (matching the visual treatment of the existing About modal's Close button).
- Remove the `g_showDemoWindow` static and the `if (g_showDemoWindow) { ImGui::ShowDemoWindow(...) }` block from `frame()` — the demo window no longer renders anywhere in the app. `imgui_demo.cpp` itself stays compiled into the `imgui` static library (the ImGui spec already requires it; see `build-system` "ImGui compiled as a static library"), it just isn't referenced from any call site.
- Update the menu-item label from `"ImGui Demo"` (a toggle-style label) to `"ImGui Demo..."` (a dialog-opens-something label) per macOS HIG / common GUI conventions.
- Update `specs/ui-sample/spec.md`'s "ImGui demo toggle" requirement — the requirement name and scenarios are reworded to reflect the new modal behavior. The toggle-open-then-toggle-closed flow becomes open-modal-then-Close-button.

Non-goals (called out to bound scope):
- **No attempt to render the actual demo content inside the modal.** The placeholder is the entire body — that's the explicit user choice over alternatives. Reproducing demo content would require re-implementing ImGui's demo widgets, which is its own multi-thousand-LOC effort and pointless given `imgui_demo.cpp` exists upstream.
- **No re-introduction of the standalone demo window via a hidden flag.** The toggle is gone, not hidden.
- **No new menu items** elsewhere. Only the `Help > ImGui Demo` entry changes.
- **No CMake change.** `imgui_demo.cpp` continues to be compiled into the `imgui` static library per the `build-system` spec; we don't add a flag to exclude it.

## Capabilities

### New Capabilities
<!-- None — this change reshapes existing ui-sample behavior. -->

### Modified Capabilities

- `ui-sample`: The existing "ImGui demo toggle" requirement is reworded to "ImGui demo modal". The body no longer describes a togglable window; instead it describes an `OpenPopup` + `BeginPopupModal` flow. The two existing scenarios (`Demo toggles open and closed`) are replaced with two new scenarios: opening the modal from the menu, and dismissing it via Close button. The other `ui-sample` requirements (first-run sample UI, menu bar, About dialog, no additional UI in v1) are unchanged.

## Impact

- **Code change:** ~25 LOC net.
  - `apps/imgui-shell/app/App.cpp`'s `frame()` — replace the `g_showDemoWindow` toggle + `MenuItem("ImGui Demo", ..., &g_showDemoWindow)` + `ShowDemoWindow` block with `MenuItem("ImGui Demo...")` calling `OpenPopup` + a `BeginPopupModal` body with the placeholder text + Close button. Net is roughly a wash — about as much code as the current toggle path.
  - Remove the `bool g_showDemoWindow` static in the anonymous namespace.
- **Spec change:** one MODIFIED requirement in `openspec/specs/ui-sample/spec.md` (the "ImGui demo toggle" → "ImGui demo modal" rename + reworded scenarios). Other ui-sample requirements unchanged.
- **CI:** no workflow changes. ImGui demo source still compiles into the static lib; no dependency changes.
- **Risk:** minimal. The change is local to `app::frame()`; it removes a code path entirely, doesn't introduce a new abstraction. The only real "what could go wrong" is the placeholder text being unclear, which is fixable via a one-line edit any time.
- **Backward compatibility:** none required — this is an internal UX change. No external consumer; no persisted state for the demo window's open/closed status.

## Verification

The change is small enough that interactive verification is one minute of clicking:
1. Launch binary, click `Help > ImGui Demo...` — modal popup appears centered on the main window.
2. Confirm the modal **blocks interaction with the menu bar and other ImGui content** behind it (this is `BeginPopupModal`'s baseline behavior — focus capture + grayed-out background).
3. Confirm there is **no separate ImGui demo window** rendered anywhere.
4. Click `Close` — popup dismisses cleanly.
5. Confirm shutdown is unaffected (no crash, no validation error, exit code 0).
