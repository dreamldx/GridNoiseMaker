#include "Preferences.h"
#include "ThemeStorage.h"
#include "Theme.h"
#include "Platform.h"

#include <imgui.h>
#include <cstdio>

namespace app {

namespace {

// Mirror of the metric tables from ThemeStorage.cpp, used for the theme editor UI.
struct ScalarMetric {
    const char*       name;
    float ImGuiStyle::*ptr;
};
constexpr ScalarMetric kScalarMetrics[] = {
    {"IndentSpacing",          &ImGuiStyle::IndentSpacing},
    {"ScrollbarSize",          &ImGuiStyle::ScrollbarSize},
    {"GrabMinSize",            &ImGuiStyle::GrabMinSize},
    {"WindowRounding",         &ImGuiStyle::WindowRounding},
    {"ChildRounding",          &ImGuiStyle::ChildRounding},
    {"FrameRounding",          &ImGuiStyle::FrameRounding},
    {"GrabRounding",           &ImGuiStyle::GrabRounding},
    {"PopupRounding",          &ImGuiStyle::PopupRounding},
    {"ScrollbarRounding",      &ImGuiStyle::ScrollbarRounding},
    {"TabRounding",            &ImGuiStyle::TabRounding},
    {"WindowBorderSize",       &ImGuiStyle::WindowBorderSize},
    {"ChildBorderSize",        &ImGuiStyle::ChildBorderSize},
    {"PopupBorderSize",        &ImGuiStyle::PopupBorderSize},
    {"FrameBorderSize",        &ImGuiStyle::FrameBorderSize},
    {"TabBorderSize",          &ImGuiStyle::TabBorderSize},
};

struct Vec2Metric {
    const char*        name;
    ImVec2 ImGuiStyle::*ptr;
};
constexpr Vec2Metric kVec2Metrics[] = {
    {"WindowPadding",    &ImGuiStyle::WindowPadding},
    {"FramePadding",     &ImGuiStyle::FramePadding},
    {"ItemSpacing",      &ImGuiStyle::ItemSpacing},
    {"ItemInnerSpacing", &ImGuiStyle::ItemInnerSpacing},
};

} // namespace

Preferences::Preferences() {
    configPathText_ = themeConfigPath();
}

Preferences::~Preferences() = default;

bool Preferences::isVisible() const {
    return visible_;
}

void Preferences::show() {
    visible_ = true;
}

void Preferences::hide() {
    visible_ = false;
}

void Preferences::render() {
    if (!visible_) {
        return;
    }

    // Original iOS/inline panel rendering (when no native dialog)
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    if constexpr (kIsDesktop) {
        // Desktop secondary window (fallback when native dialog not available)
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    } else {
        // iOS inline panel
        ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
    }

    bool open = true;
    if (ImGui::Begin("Preferences", &open, flags)) {
        if (ImGui::BeginTabBar("##PreferencesTabs")) {
            if (ImGui::BeginTabItem("General")) {
                renderGeneralTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Theme")) {
                renderThemeTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("About")) {
                renderAboutTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    if (!open) {
        hide();
    }
}

int Preferences::selectedThemeSlot() const {
    return selectedThemeSlot_;
}

void Preferences::renderGeneralTab() {
    ImGui::Text("Platform: %s", IMGUI_SHELL_PLATFORM_NAME);
    ImGui::Text("Font backend: %s", IMGUI_SHELL_FONT_BACKEND);
    ImGui::Text("Theme: %s", IMGUI_SHELL_THEME_NAME);

    if constexpr (kIsDesktop) {
        ImGui::Separator();
        ImGui::Text("Multi‑viewport: %s", ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable ? "Enabled" : "Disabled");
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::Text("  This window is a secondary OS window.");
        }
    }

    ImGui::Separator();
    ImGui::Text("Theme config file:");
    ImGui::TextWrapped("%s", configPathText_.c_str());
}

void Preferences::renderThemeTab() {
    if (ImGui::BeginChild("##ThemeMasterDetail", ImVec2(0, 0), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar)) {
        float masterWidth = ImGui::GetContentRegionAvail().x * 0.4f;
        if (ImGui::BeginChild("##ThemeMaster", ImVec2(masterWidth, 0), true)) {
            renderThemeColorList();
            renderThemeMetricList();
            ImGui::EndChild();
        }

        ImGui::SameLine();

        if (ImGui::BeginChild("##ThemeDetail", ImVec2(0, 0), true)) {
            renderThemeEditor();
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }

    ImGui::Separator();

    if (ImGui::Button("Save to config")) {
        app::writeThemeToConfig(ImGui::GetStyle());
        ImGui::SetItemDefaultFocus();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset to defaults")) {
        ImGui::StyleColorsDark();
        app::applyTheme(ImGui::GetStyle());
        selectedThemeSlot_ = -1;
        ImGui::SetItemDefaultFocus();
    }

    ImGui::SameLine();
    if (ImGui::Button("Discard changes")) {
        app::readThemeFromConfig(ImGui::GetStyle());
        selectedThemeSlot_ = -1;
        ImGui::SetItemDefaultFocus();
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("  ");
    ImGui::SameLine();
    ImGui::TextDisabled("(Changes apply live)");
}

void Preferences::renderThemeColorList() {
    if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
        const ImGuiStyle& style = ImGui::GetStyle();
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            const char* name = ImGui::GetStyleColorName(i);
            ImGui::PushID(i);
            if (ImGui::Selectable(name, selectedThemeSlot_ == i)) {
                selectedThemeSlot_ = i;
            }
            ImGui::PopID();
        }
    }
}

void Preferences::renderThemeMetricList() {
    if (ImGui::CollapsingHeader("Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Scalar metrics
        for (size_t i = 0; i < sizeof(kScalarMetrics) / sizeof(kScalarMetrics[0]); ++i) {
            const ScalarMetric& m = kScalarMetrics[i];
            ImGui::PushID(static_cast<int>(i));
            int slot = static_cast<int>(ImGuiCol_COUNT + i);
            if (ImGui::Selectable(m.name, selectedThemeSlot_ == slot)) {
                selectedThemeSlot_ = slot;
            }
            ImGui::PopID();
        }

        // Vec2 metrics
        for (size_t i = 0; i < sizeof(kVec2Metrics) / sizeof(kVec2Metrics[0]); ++i) {
            const Vec2Metric& m = kVec2Metrics[i];
            ImGui::PushID(static_cast<int>(ImGuiCol_COUNT + sizeof(kScalarMetrics) / sizeof(kScalarMetrics[0]) + i));
            int slot = static_cast<int>(ImGuiCol_COUNT + sizeof(kScalarMetrics) / sizeof(kScalarMetrics[0]) + i);
            if (ImGui::Selectable(m.name, selectedThemeSlot_ == slot)) {
                selectedThemeSlot_ = slot;
            }
            ImGui::PopID();
        }
    }
}

void Preferences::renderThemeEditor() {
    if (selectedThemeSlot_ < 0) {
        ImGui::TextUnformatted("Select a color or metric on the left to edit.");
        return;
    }

    const ImGuiStyle& style = ImGui::GetStyle();

    if (selectedThemeSlot_ < ImGuiCol_COUNT) {
        // Editing a color
        ImVec4 color = style.Colors[selectedThemeSlot_];
        const char* name = ImGui::GetStyleColorName(selectedThemeSlot_);

        ImGui::Text("Editing: %s", name);
        ImGui::Separator();

        if (ImGui::ColorEdit4("##Color", &color.x, ImGuiColorEditFlags_AlphaBar)) {
            // Direct modification - we'll apply in applyLiveThemeChanges()
            const_cast<ImGuiStyle&>(style).Colors[selectedThemeSlot_] = color;
            applyLiveThemeChanges();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(Drag sliders or click square)");
    } else {
        // Editing a metric
        int metricIdx = selectedThemeSlot_ - ImGuiCol_COUNT;
        bool isVec2 = metricIdx >= static_cast<int>(sizeof(kScalarMetrics) / sizeof(kScalarMetrics[0]));
        int vec2Idx = metricIdx - static_cast<int>(sizeof(kScalarMetrics) / sizeof(kScalarMetrics[0]));

        if (!isVec2 && metricIdx < static_cast<int>(sizeof(kScalarMetrics) / sizeof(kScalarMetrics[0]))) {
            // Scalar metric
            const ScalarMetric& m = kScalarMetrics[metricIdx];
            float value = style.*(m.ptr);

            ImGui::Text("Editing: %s", m.name);
            ImGui::Separator();

            if (ImGui::DragFloat("##Value", &value, 0.5f, 0.0f, 100.0f, "%.1f")) {
                const_cast<ImGuiStyle&>(style).*(m.ptr) = value;
                applyLiveThemeChanges();
            }
        } else if (isVec2 && vec2Idx < static_cast<int>(sizeof(kVec2Metrics) / sizeof(kVec2Metrics[0]))) {
            // Vec2 metric
            const Vec2Metric& m = kVec2Metrics[vec2Idx];
            ImVec2 value = style.*(m.ptr);

            ImGui::Text("Editing: %s", m.name);
            ImGui::Separator();

            if (ImGui::DragFloat2("##Value", &value.x, 0.5f, 0.0f, 100.0f, "%.1f")) {
                const_cast<ImGuiStyle&>(style).*(m.ptr) = value;
                applyLiveThemeChanges();
            }
        } else {
            ImGui::TextUnformatted("Invalid selection.");
        }
    }
}

void Preferences::applyLiveThemeChanges() {
    // Changes are already applied directly to ImGui::GetStyle().
    // This is a placeholder for any additional side‑effects (e.g., notifying
    // other parts of the app that the theme changed).
}

void Preferences::renderAboutTab() {
    ImGui::TextUnformatted("imgui‑shell – a minimal ImGui application shell.");
    ImGui::TextUnformatted("Version 0.1.0");

    ImGui::Separator();
    ImGui::TextUnformatted("Credits:");
    ImGui::BulletText("Dear ImGui by Omar Cornut");
    ImGui::BulletText("Vulkan backend by the ImGui community");
    ImGui::BulletText("FreeType rasterizer (optional)");
    ImGui::BulletText("JSON for Modern C++ by Niels Lohmann");

    ImGui::Separator();
    ImGui::TextUnformatted("Built with:");
    ImGui::BulletText("CMake");
    ImGui::BulletText("C++17");
}

} // namespace app