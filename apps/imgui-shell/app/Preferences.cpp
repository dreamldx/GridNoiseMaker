// Preferences.cpp
// The Preferences window (a regular, non-modal ImGui window toggled from the
// File menu) with three tabs:
//   * General  — read-only build / platform / Vulkan-device info.
//   * Theme    — a theme picker plus a master-detail editor over every color,
//                metric, and typography field; edits autosave to the selected
//                theme's user-dir file on commit and live-rebuild the font atlas.
//   * About    — version + third-party license list.
// Also holds the small preferences.json load/save helpers. See
// specs/preferences-dialog and specs/gui-theme.

#include "Preferences.h"

#include "App.h"
#include "Platform.h"
#include "Theme.h"
#include "ThemeStorage.h"

#include <imgui.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#if defined(_WIN32)
    #include <process.h>
    #define IMGUI_SHELL_GETPID _getpid
#else
    #include <unistd.h>
    #define IMGUI_SHELL_GETPID getpid
#endif

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    #include <vulkan/vulkan.h>
#endif

#if !defined(IMGUI_SHELL_PLATFORM_NAME)
    #define IMGUI_SHELL_PLATFORM_NAME "unknown"
#endif
#if !defined(IMGUI_SHELL_FONT_BACKEND)
    #define IMGUI_SHELL_FONT_BACKEND "unknown"
#endif



namespace app {

namespace {

// ---- Window state ----
bool g_showPreferencesWindow = false;

// ---- Tab state ----
bool g_firstFrame = true;

// ---- Node type panel state ----
NodeTypePanelState g_nodeTypePanelState;

// ---- Theme-editor selection state ----
enum class SelectionKind { None, Color, ScalarMetric, Vec2Metric, FontFile, FontSize, PopupMenuMargin };
SelectionKind g_selKind  = SelectionKind::None;
int           g_selIndex = -1;

// ---- General-tab Vulkan cache ----
bool        g_vulkanQueried     = false;
std::string g_vulkanApiVersion  = "(unknown)";
std::string g_gpuName           = "(unknown)";



// ---- Metric tables — mirror ThemeStorage.cpp's set ----
struct ScalarMetric {
    const char*         name;
    float ImGuiStyle::* ptr;
    float               min;
    float               max;
    const char*         fmt;
};
constexpr ScalarMetric kScalarMetrics[] = {
    {"IndentSpacing",      &ImGuiStyle::IndentSpacing,      0.0f, 40.0f, "%.0f"},
    {"ScrollbarSize",      &ImGuiStyle::ScrollbarSize,      2.0f, 32.0f, "%.1f"},
    {"GrabMinSize",        &ImGuiStyle::GrabMinSize,        4.0f, 24.0f, "%.1f"},
    {"WindowRounding",     &ImGuiStyle::WindowRounding,     0.0f, 16.0f, "%.1f"},
    {"ChildRounding",      &ImGuiStyle::ChildRounding,      0.0f, 16.0f, "%.1f"},
    {"FrameRounding",      &ImGuiStyle::FrameRounding,      0.0f, 16.0f, "%.1f"},
    {"GrabRounding",       &ImGuiStyle::GrabRounding,       0.0f, 12.0f, "%.1f"},
    {"PopupRounding",      &ImGuiStyle::PopupRounding,      0.0f, 16.0f, "%.1f"},
    {"ScrollbarRounding",  &ImGuiStyle::ScrollbarRounding,  0.0f, 16.0f, "%.1f"},
    {"TabRounding",        &ImGuiStyle::TabRounding,        0.0f, 16.0f, "%.1f"},
    {"WindowBorderSize",   &ImGuiStyle::WindowBorderSize,   0.0f,  4.0f, "%.1f"},
    {"ChildBorderSize",    &ImGuiStyle::ChildBorderSize,    0.0f,  4.0f, "%.1f"},
    {"PopupBorderSize",    &ImGuiStyle::PopupBorderSize,    0.0f,  4.0f, "%.1f"},
    {"FrameBorderSize",    &ImGuiStyle::FrameBorderSize,    0.0f,  4.0f, "%.1f"},
    {"TabBorderSize",      &ImGuiStyle::TabBorderSize,      0.0f,  4.0f, "%.1f"},
};

struct Vec2Metric {
    const char*          name;
    ImVec2 ImGuiStyle::* ptr;
    float                min;
    float                max;
    const char*          fmt;
};
constexpr Vec2Metric kVec2Metrics[] = {
    {"WindowPadding",    &ImGuiStyle::WindowPadding,    0.0f, 20.0f, "%.1f"},
    {"FramePadding",     &ImGuiStyle::FramePadding,     0.0f, 16.0f, "%.1f"},
    {"ItemSpacing",      &ImGuiStyle::ItemSpacing,      0.0f, 20.0f, "%.1f"},
    {"ItemInnerSpacing", &ImGuiStyle::ItemInnerSpacing, 0.0f, 12.0f, "%.1f"},
};

// ---- Bundled fonts ----
struct BundledFont {
    const char* label;
    const char* path;
};
constexpr BundledFont kBundledFonts[] = {
    {"Inter Regular",          "fonts/Inter-Regular.ttf"},
    {"JetBrains Mono Regular", "fonts/JetBrainsMono-Regular.ttf"},
    {"Lato Regular",           "fonts/Lato-Regular.ttf"},
};

// ---- Helpers ----

// printf-style label drawn flush against the right edge of the current row,
// used for the value column of the theme list.
void rightAlignedText(const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    const float avail = ImGui::GetContentRegionAvail().x;
    const float w     = ImGui::CalcTextSize(buf).x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w));
    ImGui::TextUnformatted(buf);
}

// Right-aligned, non-interactive color preview chip for a theme color row.
void drawColorSwatch(const ImVec4& c) {
    const float h = ImGui::GetTextLineHeight();
    const ImVec2 sz(h * 2.0f, h);
    const float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - sz.x));
    ImGui::ColorButton("##swatch", c,
                       ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                       sz);
}

// A selectable list row that, when clicked, records (kind, idx) as the active
// theme-editor selection so the right pane knows what to edit. Returns true on click.
bool selectableItem(SelectionKind kind, int idx, const char* label) {
    const bool isSelected = (g_selKind == kind && g_selIndex == idx);
    ImGui::PushID(label);
    const bool clicked = ImGui::Selectable(label, isSelected,
                                           ImGuiSelectableFlags_SpanAllColumns);
    ImGui::PopID();
    if (clicked) {
        g_selKind  = kind;
        g_selIndex = idx;
    }
    return clicked;
}

// General tab: static app identity plus build/platform/Dear-ImGui info and,
// on desktop, the Vulkan API version + GPU name (queried once and cached).
void renderGeneralTab() {
#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    if (!g_vulkanQueried) {
        VkPhysicalDevice pd = app::activePhysicalDevice();
        if (pd != VK_NULL_HANDLE) {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pd, &props);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%u.%u.%u",
                          VK_VERSION_MAJOR(props.apiVersion),
                          VK_VERSION_MINOR(props.apiVersion),
                          VK_VERSION_PATCH(props.apiVersion));
            g_vulkanApiVersion = buf;
            g_gpuName          = props.deviceName;
        }
        g_vulkanQueried = true;
    }
#endif

    const char* buildCfg =
#if defined(NDEBUG)
        "Release";
#else
        "Debug";
#endif

    ImGui::Text("Application:     imgui-shell");
    ImGui::Text("Version:         0.1.0");
    ImGui::Text("Platform:        %s", IMGUI_SHELL_PLATFORM_NAME);
    ImGui::Text("Build:           %s", buildCfg);
    ImGui::Text("Dear ImGui:      %s", IMGUI_VERSION);
#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    ImGui::Text("Vulkan API:      %s", g_vulkanApiVersion.c_str());
    ImGui::Text("GPU:             %s", g_gpuName.c_str());
#endif
    ImGui::Text("Font rasterizer: %s", IMGUI_SHELL_FONT_BACKEND);

    ImGui::Spacing();
    ImGui::TextUnformatted("Config file:");
    ImGui::TextWrapped("%s", app::themeConfigPath().c_str());
}

// Theme editor — left pane: the scrollable master list of every editable item
// (all ImGuiCol_ colors with swatches, scalar + vec2 metrics, the custom
// popup-menu margin, and typography), each a selectableItem feeding the right pane.
void renderThemeLeftPane() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Separator();
    ImGui::TextDisabled("Colors");
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        const char* name = ImGui::GetStyleColorName(i);
        selectableItem(SelectionKind::Color, i, name);
        drawColorSwatch(style.Colors[i]);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Metrics");
    for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(kScalarMetrics)); ++i) {
        const auto& m = kScalarMetrics[i];
        selectableItem(SelectionKind::ScalarMetric, i, m.name);
        char val[24];
        std::snprintf(val, sizeof(val), m.fmt, style.*m.ptr);
        rightAlignedText("%s", val);
    }
    for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(kVec2Metrics)); ++i) {
        const auto& m  = kVec2Metrics[i];
        const ImVec2& v = style.*m.ptr;
        selectableItem(SelectionKind::Vec2Metric, i, m.name);
        char val[32];
        std::snprintf(val, sizeof(val), "%.1f, %.1f", v.x, v.y);
        rightAlignedText("%s", val);
    }
    
    // Custom theme metric: popup menu margin (not in ImGuiStyle)
    selectableItem(SelectionKind::PopupMenuMargin, 0, "PopupMenuMargin");
    rightAlignedText("%.1f px", app::themePopupMenuMargin());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Typography");
    selectableItem(SelectionKind::FontFile, 0, "Font file");
    rightAlignedText("%s", app::themeFontFile().c_str());
    selectableItem(SelectionKind::FontSize, 0, "Font size");
    rightAlignedText("%.1f px", app::themeFontSizePx());
}

// Persist the current style to the selected theme's user-dir file. First
// edit of a bundled theme creates a user-dir copy automatically (writeThemeFile
// creates parent dirs); subsequent edits overwrite it. The bundled file under
// assets/themes/ is never written at runtime.
void persistTheme() {
    app::writeThemeFile(app::userThemePath(app::selectedThemeName()),
                        ImGui::GetStyle());
}

// Picker state — repopulated each frame the Theme tab is rendered.
std::vector<app::ThemeEntry> g_themes;
int                          g_themePickerIndex = 0;

// Reload the available-themes list and re-sync the picker index to the cached
// selection. Called once per Theme-tab frame.
void refreshThemeList() {
    g_themes = app::listAvailableThemes();
    const std::string& selected = app::selectedThemeName();
    g_themePickerIndex = 0;
    for (int i = 0; i < static_cast<int>(g_themes.size()); ++i) {
        if (g_themes[i].name == selected) {
            g_themePickerIndex = i;
            break;
        }
    }
}

// Theme editor — right pane: the detail editor for whatever the left pane has
// selected (color picker, metric slider/drag, font combo, font-size/margin
// slider). Each control writes straight into the live ImGuiStyle and persists
// the theme on commit (IsItemDeactivatedAfterEdit); font changes also trigger
// an atlas rebuild.
void renderThemeRightPane() {
    ImGuiStyle& style = ImGui::GetStyle();

    switch (g_selKind) {
        case SelectionKind::None:
            ImGui::TextWrapped("Select an item on the left to edit.");
            break;

        case SelectionKind::Color: {
            if (g_selIndex < 0 || g_selIndex >= ImGuiCol_COUNT) break;
            ImGui::Text("%s", ImGui::GetStyleColorName(g_selIndex));
            ImGui::Separator();
            ImGui::ColorEdit4("##color", &style.Colors[g_selIndex].x,
                              ImGuiColorEditFlags_AlphaBar);
            if (ImGui::IsItemDeactivatedAfterEdit()) persistTheme();
            break;
        }

        case SelectionKind::ScalarMetric: {
            if (g_selIndex < 0 || g_selIndex >= static_cast<int>(IM_ARRAYSIZE(kScalarMetrics))) break;
            const auto& m = kScalarMetrics[g_selIndex];
            ImGui::Text("%s", m.name);
            ImGui::Separator();
            ImGui::SliderFloat("##slider", &(style.*m.ptr), m.min, m.max, m.fmt);
            if (ImGui::IsItemDeactivatedAfterEdit()) persistTheme();
            break;
        }

        case SelectionKind::Vec2Metric: {
            if (g_selIndex < 0 || g_selIndex >= static_cast<int>(IM_ARRAYSIZE(kVec2Metrics))) break;
            const auto& m = kVec2Metrics[g_selIndex];
            ImGui::Text("%s", m.name);
            ImGui::Separator();
            ImVec2& v = style.*m.ptr;
            ImGui::DragFloat2("##drag", &v.x, 0.5f, m.min, m.max, m.fmt);
            if (ImGui::IsItemDeactivatedAfterEdit()) persistTheme();
            break;
        }

        case SelectionKind::FontFile: {
            ImGui::Text("Font file");
            ImGui::Separator();

            const std::string& current = app::themeFontFile();
            int matched = -1;
            for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(kBundledFonts)); ++i) {
                if (current == kBundledFonts[i].path) {
                    matched = i;
                    break;
                }
            }
            int comboIdx = (matched >= 0) ? matched : static_cast<int>(IM_ARRAYSIZE(kBundledFonts));
            const int customSentinel = static_cast<int>(IM_ARRAYSIZE(kBundledFonts));

            const char* items[IM_ARRAYSIZE(kBundledFonts) + 1];
            for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(kBundledFonts)); ++i) {
                items[i] = kBundledFonts[i].label;
            }
            items[customSentinel] = "Custom...";

            if (ImGui::Combo("##fontfile", &comboIdx, items, IM_ARRAYSIZE(items))) {
                if (comboIdx < customSentinel) {
                    app::setThemeFontFile(kBundledFonts[comboIdx].path);
                    persistTheme();
                    app::rebuildFontAtlas();
                }
            }

            if (comboIdx == customSentinel) {
                static char customBuf[256] = {0};
                static const std::string* lastSeen = nullptr;
                if (lastSeen != &current) {
                    std::snprintf(customBuf, sizeof(customBuf), "%s", current.c_str());
                    lastSeen = &current;
                }
                if (ImGui::InputText("##customfontpath", customBuf, sizeof(customBuf))) {
                    app::setThemeFontFile(customBuf);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    persistTheme();
                    app::rebuildFontAtlas();
                }
            }
            break;
        }

        case SelectionKind::FontSize: {
            ImGui::Text("Font size");
            ImGui::Separator();
            float px = app::themeFontSizePx();
            if (ImGui::SliderFloat("##fontsize", &px, 6.0f, 96.0f, "%.1f px")) {
                app::setThemeFontSizePx(px);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                persistTheme();
                app::rebuildFontAtlas();
            }
            break;
        }

        case SelectionKind::PopupMenuMargin: {
            ImGui::Text("PopupMenuMargin");
            ImGui::Separator();
            ImGui::TextWrapped("Padding inside popup menus (0-20 pixels)");
            ImGui::Spacing();
            
            float margin = app::themePopupMenuMargin();
            if (ImGui::DragFloat("##popupMenuMargin", &margin, 0.5f, 0.0f, 20.0f, "%.1f px")) {
                app::setThemePopupMenuMargin(margin);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                persistTheme();
            }
            break;
        }
    }
}

// Theme tab: the picker combo, the two-column master-detail editor table, and
// the "Reset to defaults" button (reverts the selected theme to its bundled file).
void renderThemeTab() {
    // ---- Theme picker row (above the master-detail table) ----
    refreshThemeList();
    ImGui::TextUnformatted("Theme:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220);
    if (!g_themes.empty()) {
        std::vector<const char*> labels;
        labels.reserve(g_themes.size());
        for (const auto& t : g_themes) labels.push_back(t.name.c_str());
        if (ImGui::Combo("##theme", &g_themePickerIndex, labels.data(),
                         static_cast<int>(labels.size()))) {
            app::switchTheme(g_themes[g_themePickerIndex].name);
            // After switch, the list itself doesn't change but the cached
            // selection does; next refresh re-syncs the index.
        }
    } else {
        ImGui::TextDisabled("(no themes)");
    }

    const float buttonRowHeight = ImGui::GetFrameHeightWithSpacing();

    // The picker row above already consumed its own vertical space (ImGui auto-
    // positions widgets below the previous one); the table only needs to
    // reserve space for the Reset button row below.
    if (ImGui::BeginTable("ThemeEditor", 2,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
                          ImVec2(0, -buttonRowHeight))) {
        ImGui::TableSetupColumn("Items",  ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ImGui::TableSetupColumn("Detail", ImGuiTableColumnFlags_WidthStretch, 0.65f);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        if (ImGui::BeginChild("ItemList", ImVec2(0, 0))) {
            renderThemeLeftPane();
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("ItemDetail", ImVec2(0, 0))) {
            renderThemeRightPane();
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    // Edits autosave on commit (see persistTheme + IsItemDeactivatedAfterEdit
    // calls in renderThemeRightPane). Reset reverts the SELECTED theme to its
    // bundled values: deletes the user-dir copy + reloads the bundled file.
    // For user-only themes (no bundled origin), Reset logs and no-ops.
    if (ImGui::Button("Reset to defaults")) {
        app::resetSelectedTheme();
    }
}

// About tab: version, copyright, and the bundled third-party license credits.
void renderAboutTab() {
    ImGui::Text("imgui-shell 0.1.0");
    ImGui::Spacing();
    ImGui::TextUnformatted("\xc2\xa9 2026 imgui-shell contributors");
    ImGui::TextUnformatted("License: TBD \xe2\x80\x94 see project README");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted("Built with:");
    ImGui::BulletText("Dear ImGui %s \xe2\x80\x94 MIT", IMGUI_VERSION);
    ImGui::BulletText("Inter font (Rasmus Andersson) \xe2\x80\x94 SIL OFL 1.1");
    ImGui::BulletText("JetBrains Mono \xe2\x80\x94 SIL OFL 1.1");
    ImGui::BulletText("Lato (Lukasz Dziedzic) \xe2\x80\x94 SIL OFL 1.1");
    ImGui::BulletText("FreeType \xe2\x80\x94 FreeType License / GPLv2");
    ImGui::BulletText("GLFW 3.4 \xe2\x80\x94 zlib/libpng");
    ImGui::BulletText("nlohmann/json v3.11.3 \xe2\x80\x94 MIT");
    ImGui::BulletText("Vulkan loader + MoltenVK \xe2\x80\x94 Apache 2.0");
}

} // namespace

void openPreferencesWindow() {
    g_showPreferencesWindow = true;
}

void renderPreferencesWindow() {
    if (!g_showPreferencesWindow) return;

    ImGui::SetNextWindowSize(ImVec2(640, 460), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Preferences", &g_showPreferencesWindow, ImGuiWindowFlags_NoDocking)) {
        if (ImGui::BeginTabBar("PreferencesTabs")) {
            if (ImGui::BeginTabItem("General")) {
                renderGeneralTab();
                ImGui::EndTabItem();
            }

            const ImGuiTabItemFlags themeFlags =
                g_firstFrame ? ImGuiTabItemFlags_SetSelected : 0;
            if (ImGui::BeginTabItem("Theme", nullptr, themeFlags)) {
                renderThemeTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("About")) {
                renderAboutTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        g_firstFrame = false;
    }
    ImGui::End();
}

// ---- Application preferences I/O ----

namespace {

// preferences.json sits beside theme.json in the per-user config dir.
std::string preferencesPath() {
    std::filesystem::path p = app::themeConfigPath();
    p = p.parent_path() / "preferences.json";
    return p.string();
}

} // namespace

// The context-menu margin is stored as the theme's popup-menu margin; these two
// accessors bridge the older "preferences" naming to that single source of truth.
float getContextMenuMargin() {
    return app::themePopupMenuMargin();
}

// Update the live margin and atomically persist it to preferences.json.
void setContextMenuMargin(float margin) {
    app::setThemePopupMenuMargin(margin);
    
    // Save to disk
    std::string path = preferencesPath();
    nlohmann::json root;
    
    // Try to load existing preferences first to preserve other data
    std::error_code ec;
    if (std::filesystem::is_regular_file(path, ec)) {
        try {
            std::ifstream in(path);
            if (in) {
                root = nlohmann::json::parse(in);
            }
        } catch (...) {
            // If we can't parse existing file, start fresh
        }
    }
    
    // Update margin value
    root["_schema_version"] = 1;
    root["context_menu_margin"] = margin;
    
    // Use atomic write similar to theme storage
    std::filesystem::path target(path);
    std::filesystem::path tempPath = target.string() + ".tmp." + std::to_string(IMGUI_SHELL_GETPID());
    
    try {
        if (target.has_parent_path()) {
            std::filesystem::create_directories(target.parent_path());
        }
        {
            std::ofstream out(tempPath);
            if (!out) {
                std::fprintf(stderr, "[imgui-shell] could not open temp file '%s' for writing\n", tempPath.string().c_str());
                return;
            }
            out << root.dump(2);
        }
        std::filesystem::rename(tempPath, target);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[imgui-shell] failed to write preferences: %s\n", e.what());
    }
}

// Node type panel preferences implementation
NodeTypePanelState getNodeTypePanelState() {
    return g_nodeTypePanelState;
}

void setNodeTypePanelState(const NodeTypePanelState& state) {
    g_nodeTypePanelState = state;
}

void saveNodeTypePanelState() {
    std::string path = preferencesPath();
    nlohmann::json root;
    
    // Try to load existing preferences first
    std::error_code ec;
    if (std::filesystem::is_regular_file(path, ec)) {
        try {
            std::ifstream in(path);
            if (in) {
                root = nlohmann::json::parse(in);
            }
        } catch (...) {
            // If we can't parse existing file, start fresh
        }
    }
    
    // Update or add panel state
    root["_schema_version"] = 1;
    root["node_type_panel"] = {
        {"visible", g_nodeTypePanelState.visible},
        {"docked", g_nodeTypePanelState.docked},
        {"floating", g_nodeTypePanelState.floating},
        {"view_mode", g_nodeTypePanelState.viewMode},
        {"width", g_nodeTypePanelState.width},
        {"height", g_nodeTypePanelState.height},
        {"floating_x", g_nodeTypePanelState.floatingX},
        {"floating_y", g_nodeTypePanelState.floatingY},
        {"dock_side", g_nodeTypePanelState.dockSide}
    };
    
    // Use atomic write
    std::filesystem::path target(path);
    std::filesystem::path tempPath = target.string() + ".tmp." + std::to_string(IMGUI_SHELL_GETPID());
    
    try {
        if (target.has_parent_path()) {
            std::filesystem::create_directories(target.parent_path());
        }
        {
            std::ofstream out(tempPath);
            if (!out) {
                std::fprintf(stderr, "[imgui-shell] could not open temp file '%s' for writing\n", tempPath.string().c_str());
                return;
            }
            out << root.dump(2);
        }
        std::filesystem::rename(tempPath, target);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[imgui-shell] failed to write panel preferences: %s\n", e.what());
    }
}

// Load preferences.json at startup (called from app::init). Currently only
// validates the schema version; the margin value itself is owned by the theme
// layer. Missing/invalid files are tolerated silently (defaults remain).
void loadPreferences() {
    std::string path = preferencesPath();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        return; // No preferences file yet
    }
    
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "[imgui-shell] could not open preferences file '%s'\n", path.c_str());
        return;
    }
    
    try {
        nlohmann::json root = nlohmann::json::parse(in);
        
        // Check schema version
        if (root.contains("_schema_version") && root["_schema_version"].is_number()) {
            int schema = root["_schema_version"];
            if (schema == 1) {
                // Load panel state if present
                if (root.contains("node_type_panel") && root["node_type_panel"].is_object()) {
                    const auto& panel = root["node_type_panel"];
                    if (panel.contains("visible") && panel["visible"].is_boolean()) {
                        g_nodeTypePanelState.visible = panel["visible"];
                    }
                    if (panel.contains("docked") && panel["docked"].is_boolean()) {
                        g_nodeTypePanelState.docked = panel["docked"];
                    }
                    if (panel.contains("floating") && panel["floating"].is_boolean()) {
                        g_nodeTypePanelState.floating = panel["floating"];
                    }
                    if (panel.contains("view_mode") && panel["view_mode"].is_number()) {
                        g_nodeTypePanelState.viewMode = panel["view_mode"];
                    }
                    if (panel.contains("width") && panel["width"].is_number()) {
                        g_nodeTypePanelState.width = panel["width"];
                    }
                    if (panel.contains("height") && panel["height"].is_number()) {
                        g_nodeTypePanelState.height = panel["height"];
                    }
                    if (panel.contains("floating_x") && panel["floating_x"].is_number()) {
                        g_nodeTypePanelState.floatingX = panel["floating_x"];
                    }
                    if (panel.contains("floating_y") && panel["floating_y"].is_number()) {
                        g_nodeTypePanelState.floatingY = panel["floating_y"];
                    }
                    if (panel.contains("dock_side") && panel["dock_side"].is_string()) {
                        g_nodeTypePanelState.dockSide = panel["dock_side"];
                    }
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::fprintf(stderr, "[imgui-shell] invalid JSON in preferences file '%s': %s\n", path.c_str(), e.what());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[imgui-shell] error reading preferences file '%s': %s\n", path.c_str(), e.what());
    }
}

} // namespace app
