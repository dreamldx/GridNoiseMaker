#pragma once

#include <imgui.h>

#include <string>

namespace app {

// ---- Typography ----
//
// Baked-in defaults (constexpr) — what configureFontAtlas uses when no
// theme.json override is active. These remain compile-time constants so
// contributors can change the project's identity defaults via a header edit.
inline constexpr const char* kDefaultThemeFontFile   = "fonts/Inter-Regular.ttf";
inline constexpr float       kDefaultThemeFontSizePx = 14.0f;

// Text color stays a compile-time constant — the ImGuiCol_Text slot is also
// covered by the `colors` section of theme.json per the theme-persistence spec,
// so runtime overrides happen through that channel rather than a separate setter.
inline constexpr ImVec4 kThemeTextColor = ImVec4(219.0f / 255.0f,
                                                  222.0f / 255.0f,
                                                  227.0f / 255.0f,
                                                  1.0f);

// Popup menu margin default
inline constexpr float kDefaultThemePopupMenuMargin = 8.0f;

// Runtime accessors — return the CURRENT typography values. Initialized to the
// constexpr defaults; mutated by ThemeStorage::readThemeFromConfig when a
// theme.json `typography` section is parsed. Pass an empty string / NaN to
// the setters to revert to the constexpr defaults.
const std::string& themeFontFile();
float              themeFontSizePx();
float              themePopupMenuMargin();
void               setThemeFontFile(std::string path);
void               setThemeFontSizePx(float px);
void               setThemePopupMenuMargin(float margin);

// ---- Style ----
//
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
