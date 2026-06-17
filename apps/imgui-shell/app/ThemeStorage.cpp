#include "ThemeStorage.h"
#include "Platform.h"
#include "App.h" // for setDocumentsPath declaration

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#if defined(_WIN32)
    #include <process.h>
    #define IMGUI_SHELL_GETPID _getpid
#else
    #include <unistd.h>
    #define IMGUI_SHELL_GETPID getpid
#endif

namespace app {

namespace {

// iOS Documents directory, set by the iOS host before app::init.
// Empty on desktop (and untouched by setDocumentsPath callers there).
std::string g_documentsPath;

const char* envOrEmpty(const char* name) {
    const char* v = std::getenv(name);
    return v ? v : "";
}

// Metric tables — single source of truth used by both readThemeFromConfig
// and writeThemeToConfig. Listed metrics MUST match the set applyTheme
// overrides (see app/Theme.cpp and design.md D2).
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

bool startsWithUnderscore(const std::string& s) {
    return !s.empty() && s[0] == '_';
}

int findColorIndex(const std::string& name) {
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        if (name == ImGui::GetStyleColorName(i)) return i;
    }
    return -1;
}

} // namespace

void setDocumentsPath(const char* path) {
    g_documentsPath = path ? path : "";
}

std::string themeConfigPath() {
    if constexpr (kIsMobile) {
        if (g_documentsPath.empty()) {
            std::fprintf(stderr, "[imgui-shell] iOS documentsPath unset; falling back to ./theme.json\n");
            return "./theme.json";
        }
        return g_documentsPath + "/theme.json";
    } else if constexpr (kIsWindows) {
        const char* appdata = envOrEmpty("APPDATA");
        if (*appdata == '\0') {
            std::fprintf(stderr, "[imgui-shell] APPDATA unset; falling back to ./AppData/imgui-shell/theme.json\n");
            return "./AppData/imgui-shell/theme.json";
        }
        return std::string(appdata) + "/imgui-shell/theme.json";
    } else { // macOS / Linux
        const char* xdg = envOrEmpty("XDG_CONFIG_HOME");
        if (*xdg != '\0') {
            return std::string(xdg) + "/imgui-shell/theme.json";
        }
        const char* home = envOrEmpty("HOME");
        if (*home == '\0') {
            std::fprintf(stderr, "[imgui-shell] HOME unset; falling back to ./.config/imgui-shell/theme.json\n");
            return "./.config/imgui-shell/theme.json";
        }
        return std::string(home) + "/.config/imgui-shell/theme.json";
    }
}

void readThemeFromConfig(ImGuiStyle& style) {
    const std::string path = themeConfigPath();

    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        // Missing or not-a-file: silent no-op per spec.
        return;
    }

    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "[imgui-shell] theme.json could not be opened at '%s'; skipping\n", path.c_str());
        return;
    }

    nlohmann::json j = nlohmann::json::parse(
        in, /*cb=*/nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/false);
    if (j.is_discarded()) {
        std::fprintf(stderr, "[imgui-shell] theme.json parse failed at '%s'; skipping\n", path.c_str());
        return;
    }
    if (!j.is_object()) {
        std::fprintf(stderr, "[imgui-shell] theme.json root must be an object; skipping\n");
        return;
    }

    // Colors
    if (auto it = j.find("colors"); it != j.end() && it->is_object()) {
        for (auto& [key, value] : it->items()) {
            if (startsWithUnderscore(key)) continue;
            int idx = findColorIndex(key);
            if (idx < 0) {
                std::fprintf(stderr, "[imgui-shell] theme.json: unknown color key '%s'; ignoring\n", key.c_str());
                continue;
            }
            if (!value.is_array() || value.size() != 4) {
                std::fprintf(stderr, "[imgui-shell] theme.json: color '%s' must be a 4-element float array; ignoring\n", key.c_str());
                continue;
            }
            ImVec4 c{};
            bool ok = true;
            for (size_t k = 0; k < 4 && ok; ++k) {
                if (!value[k].is_number()) ok = false;
                else (&c.x)[k] = value[k].get<float>();
            }
            if (!ok) {
                std::fprintf(stderr, "[imgui-shell] theme.json: color '%s' contains non-numeric value; ignoring\n", key.c_str());
                continue;
            }
            style.Colors[idx] = c;
        }
    }

    // Metrics
    if (auto it = j.find("metrics"); it != j.end() && it->is_object()) {
        for (auto& [key, value] : it->items()) {
            if (startsWithUnderscore(key)) continue;

            bool matched = false;
            for (const auto& m : kScalarMetrics) {
                if (key == m.name) {
                    matched = true;
                    if (!value.is_number()) {
                        std::fprintf(stderr, "[imgui-shell] theme.json: metric '%s' must be a number; ignoring\n", key.c_str());
                    } else {
                        style.*m.ptr = value.get<float>();
                    }
                    break;
                }
            }
            if (matched) continue;
            for (const auto& m : kVec2Metrics) {
                if (key == m.name) {
                    matched = true;
                    if (!value.is_array() || value.size() != 2 || !value[0].is_number() || !value[1].is_number()) {
                        std::fprintf(stderr, "[imgui-shell] theme.json: metric '%s' must be a 2-element float array; ignoring\n", key.c_str());
                    } else {
                        style.*m.ptr = ImVec2(value[0].get<float>(), value[1].get<float>());
                    }
                    break;
                }
            }
            if (!matched) {
                std::fprintf(stderr, "[imgui-shell] theme.json: unknown metric key '%s'; ignoring\n", key.c_str());
            }
        }
    }
}

void writeThemeToConfig(const ImGuiStyle& style) {
    const std::string path = themeConfigPath();
    const std::filesystem::path target(path);
    const std::filesystem::path tempPath =
        target.string() + ".tmp." + std::to_string(IMGUI_SHELL_GETPID());

    try {
        if (target.has_parent_path()) {
            std::filesystem::create_directories(target.parent_path());
        }

        nlohmann::json colors = nlohmann::json::object();
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            const char* name = ImGui::GetStyleColorName(i);
            const ImVec4& c  = style.Colors[i];
            colors[name] = nlohmann::json::array({ c.x, c.y, c.z, c.w });
        }

        nlohmann::json metrics = nlohmann::json::object();
        for (const auto& m : kScalarMetrics) {
            metrics[m.name] = style.*m.ptr;
        }
        for (const auto& m : kVec2Metrics) {
            const ImVec2& v = style.*m.ptr;
            metrics[m.name] = nlohmann::json::array({ v.x, v.y });
        }

        nlohmann::json root = nlohmann::json::object();
        root["colors"]  = std::move(colors);
        root["metrics"] = std::move(metrics);

        {
            std::ofstream out(tempPath);
            if (!out) {
                std::fprintf(stderr, "[imgui-shell] theme.json: could not open temp file '%s' for writing\n",
                             tempPath.string().c_str());
                return;
            }
            out << root.dump(2) << '\n';
        }

        std::filesystem::rename(tempPath, target);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[imgui-shell] theme.json write failed: %s\n", e.what());
    }
}

} // namespace app
