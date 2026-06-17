#include "Theme.h"

namespace app {

namespace {

// 8-bit RGB literal helper, optional alpha. Matches design.md D6 — the palette
// table below reads like ordinary hex colors rather than 0..1 floats.
constexpr ImVec4 c(int r, int g, int b, float a = 1.0f) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

// Accent (cyan-blue) at three brightness tiers, used uniformly across
// active / hovered / highlighted slots per gui-theme spec requirement 2.
constexpr ImVec4 kAccent       = c(100, 190, 240);          // #64BEF0
constexpr ImVec4 kAccentBright = c(125, 213, 250);          // #7DD5FA

} // namespace

void applyTheme(ImGuiStyle& style) {
    ImVec4* col = style.Colors;

    // ---- Text ----
    // Source the foreground text color from the theme-owned constant so the
    // typography (font family / size in Theme.h) and the color stay coherent.
    col[ImGuiCol_Text]                  = kThemeTextColor;
    col[ImGuiCol_TextDisabled]          = c(115, 120, 130);

    // ---- Background tiers (cool dark blue, three levels) ----
    col[ImGuiCol_WindowBg]              = c(21, 24, 31);
    col[ImGuiCol_ChildBg]               = c(21, 24, 31);
    col[ImGuiCol_PopupBg]               = c(28, 31, 39);
    col[ImGuiCol_MenuBarBg]             = c(24, 27, 35);
    col[ImGuiCol_TitleBg]               = c(17, 20, 26);
    col[ImGuiCol_TitleBgActive]         = c(28, 35, 46);
    col[ImGuiCol_TitleBgCollapsed]      = c(17, 20, 26, 0.75f);

    // ---- Frames (button/input default) ----
    col[ImGuiCol_FrameBg]               = c(35, 39, 48);
    col[ImGuiCol_FrameBgHovered]        = c(46, 51, 61);
    col[ImGuiCol_FrameBgActive]         = c(56, 62, 74);

    // ---- Tabs ----
    col[ImGuiCol_Tab]                   = c(24, 27, 35);
    col[ImGuiCol_TabHovered]            = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.50f);
    col[ImGuiCol_TabActive]             = c(46, 51, 61);
    col[ImGuiCol_TabUnfocused]          = c(24, 27, 35);
    col[ImGuiCol_TabUnfocusedActive]    = c(35, 39, 48);

    // ---- Scrollbar ----
    col[ImGuiCol_ScrollbarBg]           = c(21, 24, 31, 0.0f);
    col[ImGuiCol_ScrollbarGrab]         = c(60, 70, 88);
    col[ImGuiCol_ScrollbarGrabHovered]  = c(75, 88, 107);
    col[ImGuiCol_ScrollbarGrabActive]   = c(88, 107, 130);

    // ---- Accent-driven widgets ----
    col[ImGuiCol_CheckMark]             = kAccent;
    col[ImGuiCol_SliderGrab]            = kAccent;
    col[ImGuiCol_SliderGrabActive]      = kAccentBright;

    col[ImGuiCol_Button]                = c(46, 51, 61);
    col[ImGuiCol_ButtonHovered]         = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.50f);
    col[ImGuiCol_ButtonActive]          = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.80f);

    col[ImGuiCol_Header]                = c(46, 51, 61);
    col[ImGuiCol_HeaderHovered]         = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);
    col[ImGuiCol_HeaderActive]          = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.60f);

    col[ImGuiCol_ResizeGrip]            = c(60, 70, 88, 0.80f);
    col[ImGuiCol_ResizeGripHovered]     = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.70f);
    col[ImGuiCol_ResizeGripActive]      = ImVec4(kAccentBright.x, kAccentBright.y, kAccentBright.z, 0.90f);

    // ---- Lines & subtle structure ----
    col[ImGuiCol_Border]                = c(50, 56, 70);
    col[ImGuiCol_BorderShadow]          = c(0, 0, 0, 0.0f);
    col[ImGuiCol_Separator]             = c(50, 56, 70);
    col[ImGuiCol_SeparatorHovered]      = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.60f);
    col[ImGuiCol_SeparatorActive]       = ImVec4(kAccentBright.x, kAccentBright.y, kAccentBright.z, 0.90f);

    // ---- Selection / drag-drop / nav ----
    col[ImGuiCol_TextSelectedBg]        = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);
    col[ImGuiCol_DragDropTarget]        = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.90f);
    col[ImGuiCol_NavHighlight]          = kAccent;
    col[ImGuiCol_NavWindowingHighlight] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.70f);
    col[ImGuiCol_NavWindowingDimBg]     = c(0, 0, 0, 0.30f);
    col[ImGuiCol_ModalWindowDimBg]      = c(0, 0, 0, 0.60f);

    // ---- Metrics ----
    style.WindowPadding             = ImVec2(10.0f, 10.0f);
    style.FramePadding              = ImVec2( 8.0f,  5.0f);
    style.ItemSpacing               = ImVec2( 8.0f,  6.0f);
    style.ItemInnerSpacing          = ImVec2( 6.0f,  4.0f);
    style.IndentSpacing             = 20.0f;
    style.ScrollbarSize             = 12.0f;
    style.GrabMinSize               = 10.0f;

    style.WindowRounding            = 6.0f;
    style.ChildRounding             = 4.0f;
    style.FrameRounding             = 4.0f;
    style.GrabRounding              = 3.0f;
    style.PopupRounding             = 6.0f;
    style.ScrollbarRounding         = 8.0f;
    style.TabRounding               = 4.0f;

    style.WindowBorderSize          = 1.0f;
    style.ChildBorderSize           = 1.0f;
    style.PopupBorderSize           = 1.0f;
    style.FrameBorderSize           = 1.0f;
    style.TabBorderSize             = 0.0f;
    style.SeparatorTextBorderSize   = 2.0f;
}

} // namespace app
