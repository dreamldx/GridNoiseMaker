## 1. Code change in app/App.cpp

- [x] 1.1 Removed `bool g_showDemoWindow = false;` from the anonymous namespace at the top of `apps/imgui-shell/app/App.cpp`.
- [x] 1.2 In `frame()`'s `Help` menu, changed the toggle MenuItem to `if (ImGui::MenuItem("ImGui Demo...")) { ImGui::OpenPopup("ImGui Demo"); }`. Label now has the trailing ellipsis per design.md D3.
- [x] 1.3 Removed the existing `if (g_showDemoWindow) { ImGui::ShowDemoWindow(&g_showDemoWindow); }` block from the bottom of `frame()`.
- [x] 1.4 Added the "ImGui Demo" modal body in `frame()`, placed immediately after the existing About modal block. Body uses `BeginPopupModal("ImGui Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)`, with two `TextWrapped` paragraphs (wrap-pos 380 px) explaining the constraint, plus a Close button matching the About modal's visual treatment.
- [x] 1.5 Confirmed by precise grep:
  - `grep -nE "ShowDemoWindow\(" app/*.cpp` ⇒ zero matches (no calls remain).
  - `grep -nE "g_showDemoWindow" app/*.cpp` ⇒ zero matches (static is gone).
  - The remaining textual references to `ImGui::ShowDemoWindow` in App.cpp are intentional (one comment documenting the architectural constraint, one user-facing TextWrapped string per spec scenario 4's "placeholder body" requirement).

## 2. Verification

- [x] 2.1 Clean rebuild on macOS: succeeded (3 build steps: App.cpp, libimgui_shell_app.a, link). No warnings, no link errors.
- [x] 2.2 Validation-layer smoke run: 3s under `VK_LAYER_KHRONOS_validation` with `VK_ICD_FILENAMES=.../MoltenVK_icd.json`. Stderr silent — the change is UI-only with no Vulkan implications; the new code path doesn't crash on init.
- [x] 2.3 Interactive: menu opens modal — verified by user: clicking `Help > ImGui Demo...` opens a modal popup centered on the main window with the placeholder text + Close button.
- [x] 2.4 Interactive: modal blocks background — verified by user: with the modal open, clicks on the menu bar / main window background are absorbed by the modal (focus capture works).
- [x] 2.5 Interactive: Close button dismisses — verified by user: clicking Close dismisses the popup cleanly; main UI becomes interactive again.
- [x] 2.6 Interactive: no separate demo window exists — verified by user: no residual ImGui Demo window appears anywhere; open/close cycle repeats cleanly.
- [x] 2.7 Spec walkthrough by code review: every scenario in `specs/ui-sample/spec.md` MODIFIED "ImGui demo modal" maps to a concrete code location — (1) `OpenPopup("ImGui Demo")` in the menu handler, `BeginPopupModal` with the correct flags; (2) Close button calls `CloseCurrentPopup`; (3) zero `ShowDemoWindow()` calls in `app/`, no `g_showDemoWindow` static; (4) placeholder body has two `TextWrapped` paragraphs matching the spec's described content.
- [x] 2.8 `openspec validate make-demo-modal --type change --strict` ⇒ "Change 'make-demo-modal' is valid"
