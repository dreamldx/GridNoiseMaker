## 1. Reorder the desktop teardown

- [x] 1.1 In `apps/imgui-shell/platform/desktop/main.cpp`, move the two `ImGui_ImplVulkan_Shutdown();` / `ImGui_ImplGlfw_Shutdown();` calls so they execute *before* `app::shutdown();`. Final order per design.md D1: `vkDeviceWaitIdle` → `ImGui_ImplVulkan_Shutdown` → `ImGui_ImplGlfw_Shutdown` → `app::shutdown` → reverse-order Vulkan teardown → GLFW window destroy + terminate.
- [x] 1.2 Confirmed by code review that no other paths in `main.cpp` destroy the ImGui context out-of-order — the only `app::shutdown` call is in the loop-exit teardown; the `try/catch` in `main()` does not run any cleanup (process exits with non-zero on exception, OS reclaims resources)

## 2. Defensive guard in app::shutdown

- [x] 2.1 In `apps/imgui-shell/app/App.cpp`'s `shutdown()`, added two assertions on `ImGui::GetIO().BackendRendererUserData == nullptr` and `BackendPlatformUserData == nullptr` with spec-referencing messages (`see specs/render-backend 'Correct shutdown order'`) so the failure points at the host's call site rather than ImGui's internals
- [x] 2.2 Both assertions live in `app/App.cpp` (shared core); iOS host gets the same protection automatically with no platform-specific copy

## 3. Verification

- [x] 3.1 Clean rebuild on macOS (FreeType build): `cmake --build .../build/macos --target imgui_shell_desktop` — succeeded, no warnings; reorder is mechanical so build impact is minimal
- [ ] 3.2 **Interactive close-path test (window-X)** — **PENDING USER INTERACTION**: `cd /Users/dliu/Documents/Project/imgui_playground/apps/imgui-shell/build/macos/platform/desktop && VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json ./imgui_shell_desktop`, then click the red X (or `Cmd+Q`). Confirm `echo $?` is `0`, no `abort` / `Assertion failed` / Vulkan validation in stderr. *(CLI verification blocked: `osascript` keystroke needs Accessibility permission; Apple Event quit not delivered to non-bundle binary.)*
- [ ] 3.3 **Interactive close-path test (File > Quit)** — **PENDING USER INTERACTION**: same launch command as 3.2; pick `File > Quit` from the in-app menu; confirm same clean-exit guarantees
- [x] 3.4 Regression check on the stb_truetype fallback build: `cmake --build .../build/macos-stb --target imgui_shell_desktop` — succeeded, builds cleanly with the same reorder and defensive asserts (font backend is independent of shutdown order, so the fix applies uniformly)
- [ ] 3.5 **Defensive-assert smoke test** — **PENDING USER INTERACTION**: temporarily revert task 1.1 (move `app::shutdown()` back above the two `*_Shutdown()` calls), rebuild, run, close interactively → confirm the new `app::shutdown` assertion fires with the spec-referencing message *before* ImGui's own "Forgot to shutdown..." assert. Re-apply the fix and rebuild. *(Same close-path-drive limitation as 3.2/3.3.)*
- [x] 3.6 Walked through `specs/render-backend/spec.md` MODIFIED "ImGui backend initialization order" scenarios — code matches the new ordering and assertion text matches the spec's "assertion at app::shutdown's call site" requirement; the new "Application exits cleanly via every desktop close path" scenario is verifiable once 3.2/3.3 run
- [x] 3.7 `openspec validate fix-desktop-shutdown-crash --type change --strict` ⇒ "Change 'fix-desktop-shutdown-crash' is valid"
