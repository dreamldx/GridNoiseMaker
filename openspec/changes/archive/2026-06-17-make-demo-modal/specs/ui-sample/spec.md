## REMOVED Requirements

### Requirement: ImGui demo toggle
**Reason**: Replaced by a modal-popup demo entry. The old toggle let the user open a draggable / detachable demo window; once `enable-multi-viewport` archived, that window could be torn off as a real OS secondary window — a behavior we don't want to ship long-term. The new flow opens an explanatory modal popup instead. See the new "ImGui demo modal" requirement below.

**Migration**: Application source change only — no persisted state, no external API, no consumer to migrate. `Help > ImGui Demo` (toggle) becomes `Help > ImGui Demo...` (action that opens a modal). The previous `g_showDemoWindow` static is removed entirely.

## ADDED Requirements

### Requirement: ImGui demo modal
The sample UI SHALL provide a `Help > ImGui Demo...` menu item that opens an ImGui modal popup (via `ImGui::OpenPopup` + `ImGui::BeginPopupModal`) when clicked. The modal's body SHALL display a short placeholder explaining that `ImGui::ShowDemoWindow` opens its own top-level window and therefore cannot be rendered inside an ImGui modal popup (due to ImGui's window-stack architecture), and SHALL point readers at `<imgui-src>/imgui_demo.cpp` for the actual demo source. The modal SHALL be dismissed via a `Close` button matching the visual treatment of the existing `Help > About...` modal's Close button. The menu-item label SHALL include a trailing ellipsis (`"ImGui Demo..."`) per common GUI conventions for "opens a dialog" actions.

#### Scenario: Menu opens the modal
- **WHEN** a user clicks `Help > ImGui Demo...`
- **THEN** `ImGui::OpenPopup("ImGui Demo")` SHALL be invoked in `app::frame`
- **AND** the popup SHALL render in the next frame via `ImGui::BeginPopupModal("ImGui Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)`
- **AND** the modal SHALL block keyboard / mouse interaction with the rest of the UI (the standard `BeginPopupModal` focus-capture behavior)
- **AND** the modal background SHALL be dimmed per the current theme's `ImGuiCol_ModalWindowDimBg` color (the same way the existing About modal dims its background)

#### Scenario: Modal dismissed via Close button
- **WHEN** the user clicks the `Close` button inside the ImGui Demo modal
- **THEN** `ImGui::CloseCurrentPopup()` SHALL be invoked
- **AND** the modal SHALL disappear in the next frame
- **AND** no demo window or any other ImGui window related to the demo SHALL remain rendered

#### Scenario: No togglable demo window exists in the application
- **WHEN** the application is running
- **THEN** `ImGui::ShowDemoWindow` SHALL NOT be called from any code path in `app/` or `platform/`
- **AND** the `g_showDemoWindow` static flag from the previous toggle-based implementation SHALL NOT exist
- **AND** `imgui_demo.cpp` SHALL still be compiled into the `imgui` static library (per the `build-system` "ImGui compiled as a static library" requirement) — it is reachable in the binary but unreferenced from application code

#### Scenario: Modal placeholder body contains expected guidance
- **WHEN** a user opens the ImGui Demo modal
- **THEN** the body SHALL contain text explaining that ImGui's demo content lives in `ImGui::ShowDemoWindow`, that ImGui's window stack does not allow `ImGui::Begin` to nest inside `BeginPopupModal`, and that the demo source is at `<imgui-src>/imgui_demo.cpp` (the path resolved via the existing `FetchContent` checkout)
- **AND** the body SHALL be at most ~5 lines of `ImGui::Text` / `ImGui::TextWrapped` calls (no widgets, no images, no embedded ImGui demo content)
