## Context

The `Help > ImGui Demo` menu item currently uses ImGui's stock pattern: a `MenuItem` bound to a `bool g_showDemoWindow`, and a per-frame `if (g_showDemoWindow) ImGui::ShowDemoWindow(&g_showDemoWindow)`. The demo is a regular top-level ImGui window — draggable, resizable, and (since `enable-multi-viewport` archived) detachable into a real OS-level secondary window.

The user wants this entry point to behave as a modal dialog instead — focus-capturing, dismiss-with-button, not detachable, not present in the window list when closed. ImGui's modal mechanism is `ImGui::OpenPopup` + `ImGui::BeginPopupModal`. The constraint that emerges immediately: ImGui's `ShowDemoWindow` internally calls `ImGui::Begin("Dear ImGui Demo")` to create its own top-level window — that `Begin` is a SIBLING, not a CHILD, of any surrounding `BeginPopupModal`. There is no supported way to nest the demo's window inside a popup. So the modal's body has to be hand-rolled content — not the real demo widgets.

The user has explicitly chosen this trade-off (modal frame, placeholder body, no demo content). This design captures the small set of decisions involved.

## Goals / Non-Goals

**Goals:**
- Replace the demo-window toggle with an `OpenPopup` + `BeginPopupModal` flow rooted in `Help > ImGui Demo...`.
- The modal body explains the architectural constraint and points at `<imgui_demo.cpp>` in the fetched ImGui sources for anyone who wants to see the actual demo code.
- Dismiss via a `Close` button matching the visual treatment of the existing About-modal Close button (so the two modals feel cohesive).
- Remove the now-dead `g_showDemoWindow` static entirely — no hidden flag, no debug toggle, no `#if 0` block.

**Non-Goals:**
- No attempt to render the real ImGui demo content inside the modal.
- No re-introduction of a togglable demo window via any code path.
- No new menu items, no new modals, no new spec capabilities.
- No removal of `imgui_demo.cpp` from the `imgui` static library (it stays compiled per the `build-system` spec — just unused).
- No keyboard shortcuts.

## Decisions

### D1: Use `ImGui::OpenPopup` + `ImGui::BeginPopupModal`, not a hand-rolled focus-capturing window
ImGui's modal popup mechanism is the supported API for blocking-focus dialogs. It handles all the things a hand-rolled `ImGui::Begin` with `NoMove`/`NoCollapse`/etc would have to reimplement: focus capture, dim-background overlay (via `ImGuiCol_ModalWindowDimBg` from the curated theme), dismiss-on-escape, click-outside behavior, stacking with other popups. We use the same call shape that the existing About modal already uses, line-for-line, to keep the codebase visually consistent.

- **Alternatives:**
  - **Hand-roll an `ImGui::Begin` with `ImGuiWindowFlags_Modal`** — `ImGuiWindowFlags_Modal` is a flag in ImGui's docking branch but it's documented as "use BeginPopupModal instead." Rejected.
  - **Use a custom `BeginChild` with manual focus capture** — reinvents the wheel. Rejected.

### D2: Accept that ShowDemoWindow can't render inside the modal — the body is a placeholder
The constraint is fundamental to ImGui's window stack: `BeginPopupModal` opens an ImGui window (with `ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal`), and `ShowDemoWindow` opens its OWN top-level window. They live at the same level of ImGui's window tree, not nested. Calling `ShowDemoWindow` from inside the popup's body would render the demo as a separate window beside the modal — which is the exact opposite of what we want.

The placeholder body in the modal therefore explains the constraint in plain prose. Concrete content:

```
ImGui's demo content lives in ImGui::ShowDemoWindow, which opens its own
top-level window. ImGui's window stack does not allow Begin() to nest inside
BeginPopupModal(), so the demo content cannot be rendered inside this modal.

If you want to inspect or experiment with the demo, see
<imgui-src>/imgui_demo.cpp — that file is compiled into the imgui static
library but is no longer called from this application.
```

A `Close` button below the text dismisses the modal.

- **Alternatives:**
  - **Re-implement the demo widgets manually inside the popup** — ImGui's demo is several thousand lines of widget showcases. Pointless work; the canonical demo lives upstream.
  - **Render `ShowDemoWindow` outside the modal but only when the modal is open** — would produce a non-modal floating demo window alongside a modal frame. Confusing UX; rejected.
  - **Stub body with no explanation** — would leave a future reader wondering why the modal is empty. The placeholder text serves as both UX and code documentation.

### D3: Menu-item label changes from `"ImGui Demo"` to `"ImGui Demo..."`
The macOS Human Interface Guidelines (and most desktop GUI conventions, including Windows and most Linux DEs) use a trailing `...` to indicate a menu item opens a dialog rather than performing an immediate action. The current label `"ImGui Demo"` implied a toggle. The new label `"ImGui Demo..."` matches the convention used by the existing `"About..."` item.

- **Alternatives:**
  - **Rename to `"About ImGui Demo..."`** — wordier without adding clarity. Rejected.
  - **Keep `"ImGui Demo"` (no ellipsis)** — confusable with a toggle; doesn't match the existing `"About..."` precedent. Rejected.

### D4: `g_showDemoWindow` static is removed entirely
The flag is gone from `app/App.cpp`'s anonymous namespace. No hidden debug toggle, no `#if 0` preservation, no command-line override. The "demo as a real togglable window" UX is genuinely retired; if a contributor needs it back, they revert this change or open a follow-up.

`imgui_demo.cpp` itself stays compiled into the `imgui` static library per the `build-system` spec's "ImGui compiled as a static library" requirement — that requirement explicitly lists `imgui_demo.cpp` as one of the always-compiled sources, and we're not modifying it. Net result: the symbol `ImGui::ShowDemoWindow` exists in the binary; nothing calls it.

### D5: `BeginPopupModal` flags = `AlwaysAutoResize | NoSavedSettings`
Matches the existing About modal exactly. `AlwaysAutoResize` sizes the popup to its content (about 350×120 px for this body); `NoSavedSettings` prevents ImGui from writing the popup's position/size to its persistent ini file (which we have disabled anyway via `io.IniFilename = nullptr`, but the flag makes the intent explicit).

- **Alternatives:**
  - **Fixed size via `SetNextWindowSize`** — auto-resize handles the text width well enough; manual sizing is one more value to tune.
  - **Without `NoSavedSettings`** — currently a no-op (`IniFilename == nullptr`), but adding it future-proofs against someone re-enabling the ini file.

## Risks / Trade-offs

- **[Risk: a future contributor wants the real demo back and assumes the code is just disabled]** → Mitigation: the placeholder text in the modal body explicitly explains where the demo lives (`imgui_demo.cpp`). A `git log -p` on the change also makes the removal visible.
- **[Risk: the placeholder text reads as a bug-report ("why is there no demo?")]** → Mitigation: phrasing is explanatory, not apologetic. The text is one paragraph of code documentation in the user's face — feature, not bug.
- **[Risk: the modal feels useless from a UX standpoint]** → Acknowledged. The user explicitly chose this over the more-complex alternatives. The modal is now the demonstration of `BeginPopupModal` in our codebase — which has more pedagogical value than yet another draggable ImGui window.
- **[Trade-off: removing the demo window means we lose a multi-viewport visual test target]** → Acceptable: `enable-multi-viewport` archived with its 3 interactive tests verified. Future verification of multi-viewport will use the upcoming Preferences window (`add-preferences-dialog`, parked).

## Migration Plan

Not applicable. The change removes a UX surface; there's no persisted state for the previous toggle (the `g_showDemoWindow` static was reset to `false` on every launch via the file-scope initialization). No rollback ceremony needed beyond reverting the commit.

## Open Questions

- **Exact wording of the placeholder text** — picked above as a reasonable first draft; locks into the spec scenario as illustrative content. If the user wants different phrasing, it's a one-line edit in `app/App.cpp` and a matching update to the scenario's example.
- **Should `Help > ImGui Demo...` be removed entirely instead of opening a placeholder modal?** Defensible — if the demo content can't appear, why keep the menu entry? But the user's explicit pick was "modal with placeholder," so keep it. Worth revisiting if the modal feels truly useless after a week of use.
