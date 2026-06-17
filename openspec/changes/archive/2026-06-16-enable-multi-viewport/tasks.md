## 1. Enable the flag (shared app core)

- [x] 1.1 In `apps/imgui-shell/app/App.cpp`'s `init()`, immediately after `app::applyTheme(ImGui::GetStyle())` and before `configureFontAtlas()`, added `if constexpr (kIsDesktop) { ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; }` with a comment referencing the spec. Desktop-only by compile-time constant.
- [x] 1.2 Confirmed by `grep -nE '#if(def)? +(defined\()?IMGUI_SHELL_PLATFORM' app/*.cpp` ⇒ zero matches in `.cpp` files. The pre-existing matches in `Platform.h` and `RenderContext.h` are intentional (platform-traits header and platform-gated struct layout); the spec rule applies to source files only.

## 2. Wire the per-frame multi-viewport calls (desktop host)

- [x] 2.1 In `apps/imgui-shell/platform/desktop/main.cpp`'s `renderOneFrame()`, immediately after `vkEndCommandBuffer(f.commandBuffer)` and before the `VkSemaphore renderFinished = ...` line, added `if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { ImGui::UpdatePlatformWindows(); ImGui::RenderPlatformWindowsDefault(); }` with a comment explaining the secondary-viewport contract.
- [x] 2.2 Confirmed by code review that the main viewport's `vkQueueSubmit` + `vkQueuePresentKHR` still run AFTER the multi-viewport block. ImGui's secondary-viewport submit/present is internal to `RenderPlatformWindowsDefault`; the main viewport remains owned by our existing submit/present code.
- [x] 2.3 Confirmed `ImGui_ImplGlfw_InitForVulkan(window, /*install_callbacks=*/true)` already passes `true` at the existing call site — the flag is required for ImGui's GLFW backend to install its multi-viewport platform callbacks. No change needed.
- [x] 2.4 Confirmed `ImGui_ImplVulkan_InitInfo.MinImageCount` and `ImageCount` are already populated from the main swapchain at the existing call site. No change needed.

## 3. Verification

- [x] 3.1 Clean rebuild on macOS: succeeded (4 build steps: App.cpp, libimgui_shell_app.a, main.cpp, link). No warnings, no link errors.
- [x] 3.2 Validation-layer smoke run (no secondary viewports yet): launched the binary for 3s under `VK_LAYER_KHRONOS_validation` with `VK_ICD_FILENAMES=.../MoltenVK_icd.json`. Stderr silent. With no secondary viewport open, `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` are no-ops; the steady-state path is unchanged.
- [x] 3.3 Interactive secondary-window test: verified by user — demo window dragged outside main host's bounds became a real OS-level secondary window with its own title bar / X / Dock entry; demo content rendered correctly inside; dragging back inside re-merged cleanly.
- [x] 3.4 Interactive multi-secondary-window test: verified by user — demo dragged out + About modal open coexist cleanly; no crashes, no validation errors, stderr silent.
- [x] 3.5 Interactive close-path test with secondary viewports: verified by user — with at least one secondary viewport open, closing via main window's red X / File > Quit exits cleanly (exit code 0, silent stderr, no IM_ASSERT). Defensive `app::shutdown` assertions still pass when ImGui has secondary-viewport state to tear down.
- [x] 3.6 Stb_truetype fallback build (`build/macos-stb/`): rebuilt cleanly with the same source. Multi-viewport is independent of font backend; the interactive test from 3.3 applies identically once you eyeball this binary too.
- [x] 3.7 Spec walkthrough by code review: every scenario in `specs/render-backend/spec.md` ADDED "Multi-viewport support on desktop" maps to a concrete code location — `if constexpr (kIsDesktop)` gate, default callbacks unchanged (relying on ImGui's `Init` to install them), `UpdatePlatformWindows` + `RenderPlatformWindowsDefault` between main `EndCommandBuffer` and main submit, no `desktop::createSwapchain` calls for secondary viewports, no resize callback bound to secondary windows, no explicit per-viewport shutdown code, `InitInfo.MinImageCount`/`ImageCount` populated from main swapchain
- [x] 3.8 `openspec validate enable-multi-viewport --type change --strict` ⇒ "Change 'enable-multi-viewport' is valid"
