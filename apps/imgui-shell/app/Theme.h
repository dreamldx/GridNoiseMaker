#pragma once

#include <imgui.h>

namespace app {

// Theme-owned typography defaults. The font-rendering layer (App.cpp's
// configureFontAtlas) reads these to pick the asset path + pixel size that
// match the theme's identity. The text color is also re-used in Theme.cpp
// as ImGuiCol_Text so the typography and color stay in sync.
inline constexpr const char* kThemeFontFile   = "fonts/Inter-Regular.ttf";
inline constexpr float       kThemeFontSizePx = 14.0f;
inline constexpr ImVec4      kThemeTextColor  = ImVec4(219.0f / 255.0f,
                                                       222.0f / 255.0f,
                                                       227.0f / 255.0f,
                                                       1.0f);

// Apply the "imgui-shell dark" theme to the given ImGui style.
//
// PREREQUISITE: the caller MUST first call `ImGui::StyleColorsDark(&style)`
// so unspecified slots inherit sane dark-theme defaults. applyTheme overrides
// only the ~30 slots and metric fields it explicitly cares about; any future
// ImGuiCol_* slot ImGui adds will still look reasonable thanks to the
// underlying StyleColorsDark call.
//
// See specs/gui-theme/spec.md for the full contract, and design.md D1 / D2
// for the rationale behind the specific palette + metric values.
void applyTheme(ImGuiStyle& style);

} // namespace app
