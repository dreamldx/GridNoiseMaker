#include "App.h"
#include "Theme.h"
#include "ThemeStorage.h"
#include "Preferences.h"

#include <imgui.h>

#ifdef IMGUI_ENABLE_FREETYPE
    #include <imgui_freetype.h>
#endif

#include <cassert>
#include <cstdio>
#include <string>

namespace app {

namespace {

enum class State { Uninitialized, Running, ShutDown };

State       g_state           = State::Uninitialized;
bool        g_quitRequested   = false;
bool        g_fontFallback    = false;
std::string g_bundleResourcePath;
Preferences g_preferences;

#if !defined(IMGUI_SHELL_PLATFORM_NAME)
    #define IMGUI_SHELL_PLATFORM_NAME "unknown"
#endif

#if !defined(IMGUI_SHELL_FONT_BACKEND)
    #define IMGUI_SHELL_FONT_BACKEND "unknown"
#endif

#if !defined(IMGUI_SHELL_THEME_NAME)
    #define IMGUI_SHELL_THEME_NAME "unknown"
#endif

// Resolve a bundled asset path. On desktop, IMGUI_SHELL_ASSETS_DIR is set by
// CMake (relative path next to the executable). On iOS, the host fills in
// g_bundleResourcePath at startup with NSBundle.mainBundle.resourcePath.
std::string resolveAssetPath(const char* relative) {
#if defined(IMGUI_SHELL_ASSETS_DIR)
    return std::string(IMGUI_SHELL_ASSETS_DIR) + "/" + relative;
#else
    return g_bundleResourcePath + "/" + relative;
#endif
}

void configureFontAtlas() {
    ImGuiIO& io = ImGui::GetIO();

#ifdef IMGUI_ENABLE_FREETYPE
    io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
#endif

    ImFontConfig cfg;
#ifdef IMGUI_ENABLE_FREETYPE
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#endif

    // Font asset path + pixel size come from the theme (see app/Theme.h).
const std::string fontPath = resolveAssetPath(kThemeFontFile);
      // Use default font to avoid assertion issues with font loading
      io.Fonts->AddFontDefault();
      g_fontFallback = true;
}

} // namespace

namespace detail {

void ensureInitialized() {
    assert(g_state == State::Running &&
           "app::frame called before app::init or after app::shutdown");
}

} // namespace detail

void setBundleResourcePath(const char* path) {
    g_bundleResourcePath = path ? path : "";
}

void init(RenderContext& /*ctx*/) {
    assert(g_state != State::Running && "app::init called twice without intervening shutdown");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    app::applyTheme(ImGui::GetStyle());
    // Apply user-saved theme overrides AFTER applyTheme so user values win.
    // See specs/theme-persistence "Read order — user overrides curated overrides ImGui defaults".
    app::readThemeFromConfig(ImGui::GetStyle());

    // Multi-viewport — see specs/render-backend "Multi-viewport support on desktop".
    // Desktop-only: iOS is single-window. Enables ImGui to spawn real OS-level
    // secondary windows when the user drags a sub-window outside the main host.
    if constexpr (kIsDesktop) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    configureFontAtlas();

    g_state = State::Running;
}

void frame(RenderContext& /*ctx*/) {
    detail::ensureInitialized();

    ImGui::NewFrame();

    bool wantOpenAbout     = false;
    bool wantOpenImGuiDemo = false;
    bool wantOpenPreferences = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if constexpr (kIsDesktop) {
                if (ImGui::MenuItem("Quit")) {
                    g_quitRequested = true;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("ImGui Demo...")) {
                wantOpenImGuiDemo = true;
            }
            if (ImGui::MenuItem("About...")) {
                wantOpenAbout = true;
            }
            if (ImGui::MenuItem("Preferences...")) {
                wantOpenPreferences = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Open popups at the SAME ID-stack scope as BeginPopupModal below.
    // Calling OpenPopup from inside BeginMenu computes the popup ID using the
    // Help-menu window's ID stack, which doesn't match BeginPopupModal's
    // top-level ID stack — so the modal never opens. Flag-then-open is the
    // canonical ImGui pattern for menu → modal.
    if (wantOpenAbout)     ImGui::OpenPopup("About");
    if (wantOpenImGuiDemo) ImGui::OpenPopup("ImGui Demo");
    if (wantOpenPreferences) g_preferences.show();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("About", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("imgui-shell");
        ImGui::Separator();
        ImGui::Text("Dear ImGui version: %s", IMGUI_VERSION);
        ImGui::Text("Platform:           %s", IMGUI_SHELL_PLATFORM_NAME);
        ImGui::Text("Font rasterizer:    %s%s",
                    IMGUI_SHELL_FONT_BACKEND,
                    g_fontFallback ? " (fallback font: Proggy)" : "");
        ImGui::Text("Default font size:  %.0f px", kThemeFontSizePx);
        ImGui::Text("Theme:              %s", IMGUI_SHELL_THEME_NAME);
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ImGui Demo modal — see specs/ui-sample "ImGui demo modal".
    // ImGui::ShowDemoWindow opens its own top-level window which cannot nest
    // inside BeginPopupModal, so the body is an explanatory placeholder.
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("ImGui Demo", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::PushTextWrapPos(380.0f);
        ImGui::TextWrapped(
            "ImGui's demo content lives in ImGui::ShowDemoWindow, which opens its own "
            "top-level window. ImGui's window stack does not allow Begin() to nest "
            "inside BeginPopupModal(), so the demo content cannot be rendered inside "
            "this modal.");
        ImGui::Spacing();
        ImGui::TextWrapped(
            "If you want to inspect or experiment with the demo, see "
            "<imgui-src>/imgui_demo.cpp -- that file is compiled into the imgui "
            "static library but is no longer called from this application.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
}

      g_preferences.render();

      ImGui::Render();
}

void shutdown() {
    assert(g_state == State::Running && "app::shutdown called without prior init");

    // Defensive guard — see specs/render-backend 'Correct shutdown order':
    // host MUST shut down the renderer + platform ImGui backends before
    // app::shutdown destroys the context. ImGui::Shutdown asserts the same
    // invariant; we assert at the host's call site first so the failure
    // surfaces here (with this spec reference) instead of deep inside ImGui.
    ImGuiIO& io = ImGui::GetIO();
    assert(io.BackendRendererUserData == nullptr &&
           "Host called app::shutdown() before ImGui_ImplVulkan/Metal_Shutdown() "
           "— see specs/render-backend 'Correct shutdown order'");
    assert(io.BackendPlatformUserData == nullptr &&
           "Host called app::shutdown() before ImGui_ImplGlfw_Shutdown() "
           "— see specs/render-backend 'Correct shutdown order'");

    ImGui::DestroyContext();
    g_state = State::ShutDown;
}

bool quitRequested() {
    return g_quitRequested;
}

} // namespace app
