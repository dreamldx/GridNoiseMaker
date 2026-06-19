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

std::string preferencesPath() {
    std::filesystem::path p = app::themeConfigPath();
    p = p.parent_path() / "preferences.json";
    return p.string();
}

} // namespace

float getContextMenuMargin() {
    return app::themePopupMenuMargin();
}

void setContextMenuMargin(float margin) {
    app::setThemePopupMenuMargin(margin);
    
    // Save to disk
    std::string path = preferencesPath();
    nlohmann::json root = {
        {"_schema_version", 1},
        {"context_menu_margin", margin}
    };
    
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
                
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::fprintf(stderr, "[imgui-shell] invalid JSON in preferences file '%s': %s\n", path.c_str(), e.what());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[imgui-shell] error reading preferences file '%s': %s\n", path.c_str(), e.what());
    }
}

} // namespace app
