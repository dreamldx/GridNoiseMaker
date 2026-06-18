#include "ThemeStorage.h"

#include "App.h"        // resolveAssetPath, setDocumentsPath forward
#include "Platform.h"
#include "Theme.h"      // typography accessors / setters / defaults

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
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
std::string g_documentsPath;

// Cache for the currently-selected theme name.
std::string g_selectedTheme = "dark";

const char* envOrEmpty(const char* name) {
    const char* v = std::getenv(name);
    return v ? v : "";
}

// Metric tables — single source of truth used by both readThemeFile and
// writeThemeFile. Must match the set applyTheme overrides.
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

// Shared atomic-write — temp file in the same dir, then rename.
bool writeJsonAtomic(const std::string& path, const nlohmann::json& root) {
    const std::filesystem::path target(path);
    const std::filesystem::path tempPath =
        target.string() + ".tmp." + std::to_string(IMGUI_SHELL_GETPID());

    try {
        if (target.has_parent_path()) {
            std::filesystem::create_directories(target.parent_path());
        }
        {
            std::ofstream out(tempPath);
            if (!out) {
                std::fprintf(stderr,
                    "[imgui-shell] could not open temp file '%s' for writing\n",
                    tempPath.string().c_str());
                return false;
            }
            out << root.dump(2) << '\n';
        }
        std::filesystem::rename(tempPath, target);
        return true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[imgui-shell] atomic write to '%s' failed: %s\n",
                     path.c_str(), e.what());
        return false;
    }
}

// Shared parse path — colors / metrics / typography blocks. Used by both
// readThemeFile and migrateLegacyThemeFile.
void parseThemeBlocks(const nlohmann::json& j, ImGuiStyle& style, const char* source) {
    // Colors
    if (auto it = j.find("colors"); it != j.end() && it->is_object()) {
        for (auto& [key, value] : it->items()) {
            if (startsWithUnderscore(key)) continue;
            int idx = findColorIndex(key);
            if (idx < 0) {
                std::fprintf(stderr, "[imgui-shell] %s: unknown color key '%s'; ignoring\n",
                             source, key.c_str());
                continue;
            }
            if (!value.is_array() || value.size() != 4) {
                std::fprintf(stderr, "[imgui-shell] %s: color '%s' must be a 4-element float array; ignoring\n",
                             source, key.c_str());
                continue;
            }
            ImVec4 c{};
            bool ok = true;
            for (size_t k = 0; k < 4 && ok; ++k) {
                if (!value[k].is_number()) ok = false;
                else (&c.x)[k] = value[k].get<float>();
            }
            if (!ok) {
                std::fprintf(stderr, "[imgui-shell] %s: color '%s' contains non-numeric value; ignoring\n",
                             source, key.c_str());
                continue;
            }
            style.Colors[idx] = c;
        }
    }

    // Typography
    if (auto it = j.find("typography"); it != j.end() && it->is_object()) {
        for (auto& [key, value] : it->items()) {
            if (startsWithUnderscore(key)) continue;
            if (key == "font_file") {
                if (!value.is_string()) {
                    std::fprintf(stderr, "[imgui-shell] %s: typography.font_file must be a string; ignoring\n", source);
                    continue;
                }
                const std::string raw = value.get<std::string>();
                if (raw.empty()) continue;
                const bool isAbsolute =
                    raw.front() == '/' || raw.find(":\\") != std::string::npos;
                const std::string resolved = isAbsolute ? raw : resolveAssetPath(raw.c_str());
                std::error_code ec2;
                if (!std::filesystem::exists(resolved, ec2)) {
                    std::fprintf(stderr,
                        "[imgui-shell] %s: typography.font_file '%s' does not exist; reverting to default\n",
                        source, resolved.c_str());
                    continue;
                }
                setThemeFontFile(raw);
            } else if (key == "font_size_px") {
                if (!value.is_number()) {
                    std::fprintf(stderr, "[imgui-shell] %s: typography.font_size_px must be a number; ignoring\n", source);
                    continue;
                }
                float px      = value.get<float>();
                float clamped = std::clamp(px, 6.0f, 96.0f);
                if (clamped != px) {
                    std::fprintf(stderr,
                        "[imgui-shell] %s: typography.font_size_px %g out of range [6, 96]; clamping to %g\n",
                        source, static_cast<double>(px), static_cast<double>(clamped));
                }
                setThemeFontSizePx(clamped);
            } else {
                std::fprintf(stderr, "[imgui-shell] %s: unknown typography key '%s'; ignoring\n",
                             source, key.c_str());
            }
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
                        std::fprintf(stderr, "[imgui-shell] %s: metric '%s' must be a number; ignoring\n",
                                     source, key.c_str());
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
                        std::fprintf(stderr, "[imgui-shell] %s: metric '%s' must be a 2-element float array; ignoring\n",
                                     source, key.c_str());
                    } else {
                        style.*m.ptr = ImVec2(value[0].get<float>(), value[1].get<float>());
                    }
                    break;
                }
            }
            if (!matched) {
                std::fprintf(stderr, "[imgui-shell] %s: unknown metric key '%s'; ignoring\n",
                             source, key.c_str());
            }
        }
    }
}

nlohmann::json serializeThemeBlocks(const ImGuiStyle& style) {
    nlohmann::json colors = nlohmann::json::object();
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        const char*   name = ImGui::GetStyleColorName(i);
        const ImVec4& c    = style.Colors[i];
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

    nlohmann::json typography = nlohmann::json::object();
    typography["font_file"]    = themeFontFile();
    typography["font_size_px"] = themeFontSizePx();

    nlohmann::json root = nlohmann::json::object();
    root["colors"]     = std::move(colors);
    root["metrics"]    = std::move(metrics);
    root["typography"] = std::move(typography);
    return root;
}

// Bundled themes dir — `assets/themes`.
std::string bundledThemesDir() {
    return resolveAssetPath("themes");
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
    } else {
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

std::string userThemesDir() {
    std::filesystem::path p(themeConfigPath());
    p = p.parent_path() / "themes";
    return p.string();
}

std::string userThemePath(const std::string& name) {
    return userThemesDir() + "/" + name + ".json";
}

const std::string& selectedThemeName() {
    return g_selectedTheme;
}

void setSelectedThemeName(const std::string& name) {
    g_selectedTheme = name;
}

std::string readSelectedThemeName() {
    const std::string path = themeConfigPath();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        return "dark";
    }
    std::ifstream in(path);
    if (!in) return "dark";

    nlohmann::json j = nlohmann::json::parse(
        in, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/false);
    if (j.is_discarded() || !j.is_object()) {
        std::fprintf(stderr, "[imgui-shell] theme.json parse failed at '%s'; using default 'dark'\n", path.c_str());
        return "dark";
    }
    auto it = j.find("selected_theme");
    if (it == j.end() || !it->is_string()) {
        return "dark";
    }
    return it->get<std::string>();
}

void writeSelectedThemeName(const std::string& name) {
    nlohmann::json root = {
        {"_schema_version", 2},
        {"selected_theme", name},
    };
    if (writeJsonAtomic(themeConfigPath(), root)) {
        setSelectedThemeName(name);
    }
}

void readThemeFile(const std::string& path, ImGuiStyle& style) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        return;
    }
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "[imgui-shell] could not open theme file '%s'; skipping\n", path.c_str());
        return;
    }
    nlohmann::json j = nlohmann::json::parse(
        in, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/false);
    if (j.is_discarded() || !j.is_object()) {
        std::fprintf(stderr, "[imgui-shell] theme file parse failed at '%s'; skipping\n", path.c_str());
        return;
    }
    parseThemeBlocks(j, style, path.c_str());
}

void writeThemeFile(const std::string& path, const ImGuiStyle& style) {
    nlohmann::json root = serializeThemeBlocks(style);
    writeJsonAtomic(path, root);
}

std::vector<ThemeEntry> listAvailableThemes() {
    std::map<std::string, ThemeEntry> byName;

    auto scanDir = [&](const std::string& dir, bool fromUser) {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec)) return false;
        for (auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            const auto& path = entry.path();
            if (path.extension() != ".json") continue;
            std::string stem = path.stem().string();
            if (!stem.empty() && stem.front() == '.') continue;
            byName[stem] = ThemeEntry{stem, path.string(), fromUser};
        }
        return true;
    };

    const std::string bundled = bundledThemesDir();
    const std::string user    = userThemesDir();
    bool bundledOK = scanDir(bundled, false);
    bool userOK    = scanDir(user,    true);

    if (!bundledOK && !userOK) {
        std::fprintf(stderr,
            "[imgui-shell] no themes found (bundled='%s' missing, user='%s' missing)\n",
            bundled.c_str(), user.c_str());
    } else if (!bundledOK) {
        std::fprintf(stderr,
            "[imgui-shell] bundled themes dir '%s' not found; only user themes will be available\n",
            bundled.c_str());
    }

    std::vector<ThemeEntry> result;
    result.reserve(byName.size());
    for (auto& [_, e] : byName) result.push_back(std::move(e));
    return result; // already alphabetical because std::map is sorted
}

void migrateLegacyThemeFile() {
    const std::string path = themeConfigPath();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return;

    std::ifstream in(path);
    if (!in) return;
    nlohmann::json j = nlohmann::json::parse(
        in, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/false);
    in.close();
    if (j.is_discarded() || !j.is_object()) return;
    if (j.contains("_schema_version")) return; // already migrated
    const bool hasLegacy =
        j.contains("colors") || j.contains("metrics") || j.contains("typography");
    if (!hasLegacy) return;

    nlohmann::json customContent = nlohmann::json::object();
    if (j.contains("colors"))     customContent["colors"]     = j["colors"];
    if (j.contains("metrics"))    customContent["metrics"]    = j["metrics"];
    if (j.contains("typography")) customContent["typography"] = j["typography"];

    const std::string customPath = userThemePath("custom");
    if (!writeJsonAtomic(customPath, customContent)) {
        std::fprintf(stderr,
            "[imgui-shell] migration aborted: could not write '%s'; retry next launch\n",
            customPath.c_str());
        return;
    }

    nlohmann::json newRoot = {
        {"_schema_version", 2},
        {"selected_theme", "custom"},
    };
    if (!writeJsonAtomic(path, newRoot)) {
        std::fprintf(stderr,
            "[imgui-shell] migration partially complete: '%s' written but theme.json rewrite failed\n",
            customPath.c_str());
        return;
    }
    setSelectedThemeName("custom");
    std::fprintf(stderr,
        "[imgui-shell] migrated legacy theme.json to themes/custom.json (selected_theme: custom)\n");
}

} // namespace app
