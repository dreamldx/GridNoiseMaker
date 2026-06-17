## 1. Prerequisites (Dependent Changes)

- [x] 1.1 Verify `enable-multi-viewport` change is complete (multi-viewport support in render-backend) ✅ IMPLEMENTED
- [x] 1.2 Verify `add-theme-persistence` change is complete (theme JSON read/write, per-OS config paths) ✅ IMPLEMENTED
- [x] 1.3 Ensure `nlohmann/json` dependency is available via FetchContent in cmake/Dependencies.cmake ✅ IMPLEMENTED

## 2. Preferences Dialog UI Implementation

- [x] 2.1 Create `apps/imgui-shell/app/Preferences.h` with class declaration and public API
- [x] 2.2 Create `apps/imgui-shell/app/Preferences.cpp` with dialog implementation
- [x] 2.3 Implement `Preferences::render()` method with tab bar (General, Theme, About)
- [x] 2.4 Implement General tab with read-only diagnostic info
- [x] 2.5 Implement Theme tab with master-detail layout (left: selectable list, right: editing widgets)
- [x] 2.6 Implement About tab with static credits content
- [x] 2.7 Add Save, Reset to defaults, Discard changes buttons to Theme tab
- [x] 2.8 Wire button actions: Save calls `writeThemeToConfig`, Reset calls `applyTheme`, Discard calls `readThemeFromConfig`
- [x] 2.9 Add `Help > Preferences...` menu item to App.cpp menu bar
- [x] 2.10 Add dialog open/close state management (show/hide based on menu selection)

## 3. Theme Editor Widgets

- [x] 3.1 Create selectable list of all theme-editable items (color slots first, then metric fields)
- [x] 3.2 Implement `ImGui::ColorEdit4` widget for color editing with live preview
- [x] 3.3 Implement appropriate widgets for metric types: `ImGui::SliderFloat`, `ImGui::DragFloat`, or `ImGui::SliderInt`
- [x] 3.4 Ensure theme changes immediately update `ImGui::GetStyle()` for live preview
- [x] 3.5 Group color slots and metric fields with `ImGui::Separator` for visual organization

## 4. Multi-Viewport Integration

- [x] 4.1 Enable `ImGuiConfigFlags_ViewportsEnable` in desktop platform initialization
- [x] 4.2 Configure Preferences dialog to open as secondary OS window on desktop
- [x] 4.3 Configure Preferences dialog to appear as inline floating panel on iOS (no multi-viewport)
- [x] 4.4 Add `ImGuiWindowFlags` for dialog persistence (position/size saved via ini file)
- [x] 4.5 Scope ini persistence to Preferences window only (not main window)

## 5. Integration with Theme Persistence

  - [x] 5.1 Call `readThemeFromConfig` after `applyTheme` in `app::init` (ensuring user values override curated)
- [x] 5.2 Wire Save button to call `writeThemeToConfig` with current `ImGui::GetStyle()`
  - [x] 5.3 Wire Discard button to call `readThemeFromConfig` to reload saved file
- [x] 5.4 Wire Reset button to call `applyTheme` to restore curated defaults
  - [x] 5.5 Handle missing/malformed config file gracefully (silent fallback to curated theme)
  
  ## 6. Build System Updates
  
  - [x] 6.1 Add `Preferences.{h,cpp}` to `imgui_shell_app` sources in `app/CMakeLists.txt`
- [x] 6.2 Verify `nlohmann/json` target is linked PUBLIC to `imgui_shell_app`
- [x] 6.3 Ensure iOS bundle includes Preferences code (no multi-viewport, inline panel)

## 7. Testing and Verification

- [x] 7.1 Build on all platforms (macOS, Windows, Linux, iOS) without errors
- [x] 7.2 Test Preferences dialog opens/closes via `Help > Preferences...` menu
- [ ] 7.3 Verify desktop: dialog is separate OS window (multi-viewport)
- [ ] 7.4 Verify iOS: dialog is inline floating panel (no multi-viewport)
- [x] 7.5 Test Theme tab: edit colors and metrics, see live preview
- [x] 7.6 Test Save button: verify theme.json file created/updated
- [x] 7.7 Test Reset button: reverts to curated theme defaults
- [x] 7.8 Test Discard button: reloads saved theme.json
- [x] 7.9 Test General tab: displays correct diagnostic info
- [x] 7.10 Test About tab: displays static credits
- [x] 7.11 Verify dialog position/size persists across launches
- [x] 7.12 Run `openspec validate add-preferences-dialog --type change --strict`