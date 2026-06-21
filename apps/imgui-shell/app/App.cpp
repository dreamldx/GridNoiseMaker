#include "App.h"
#include "Preferences.h"
#include "Theme.h"
#include "ThemeStorage.h"

// Node graph widget
#include "NodeGraphWidget.h"

#include <imgui.h>

#ifdef IMGUI_ENABLE_FREETYPE
    #include <imgui_freetype.h>
#endif

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

namespace app {

namespace {

enum class State { Uninitialized, Running, ShutDown };

State       g_state           = State::Uninitialized;
bool        g_quitRequested   = false;
bool        g_fontFallback    = false;
std::string g_bundleResourcePath;

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
#endif

// Platform-supplied font-atlas rebuild callback. Registered by the desktop
// host (Vulkan) or iOS host (Metal) after app::init. nullptr until then.
RebuildFontAtlasFn g_rebuildFontAtlasCb = nullptr;

// Deferred-rebuild flag — set by app::rebuildFontAtlas (from inside widget
// code, mid-frame), drained at the top of the next app::frame BEFORE
// ImGui::NewFrame locks the atlas. ImGui::asserts on Fonts->Clear() while
// the atlas is locked, so synchronous rebuild from the widget body is
// unsafe; defer to between-frames.
bool g_fontRebuildPending = false;

// Node graph widget state
bool g_showNodeGraph = true;
std::unique_ptr<nodegraph::NodeGraphWidget> g_nodeGraphWidget;

// File dialog state for node graph persistence
bool g_showSaveDialog = false;
bool g_showLoadDialog = false;
std::string g_fileDialogPath;
bool g_fileDialogError = false;
std::string g_fileDialogErrorMessage;

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
// Vulkan resources for node graph texture cache
VkDevice g_vulkanDevice = VK_NULL_HANDLE;
VkDescriptorPool g_vulkanDescriptorPool = VK_NULL_HANDLE;
#endif

#if !defined(IMGUI_SHELL_PLATFORM_NAME)
    #define IMGUI_SHELL_PLATFORM_NAME "unknown"
#endif

#if !defined(IMGUI_SHELL_FONT_BACKEND)
    #define IMGUI_SHELL_FONT_BACKEND "unknown"
#endif

#if !defined(IMGUI_SHELL_THEME_NAME)
    #define IMGUI_SHELL_THEME_NAME "unknown"
#endif

} // namespace

  void configureFontAtlas() {
      ImGuiIO& io = ImGui::GetIO();

#ifdef IMGUI_ENABLE_FREETYPE
    io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
#endif

    ImFontConfig cfg;
#ifdef IMGUI_ENABLE_FREETYPE
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#endif

// Font asset path + pixel size come from the theme accessors (see app/Theme.h).
      const std::string& themeFontPath = app::themeFontFile();
      const bool isAbsolute = !themeFontPath.empty() &&
          (themeFontPath.front() == '/' || themeFontPath.find(":\\") != std::string::npos);
      const std::string fontPath = isAbsolute
          ? themeFontPath
          : resolveAssetPath(themeFontPath.c_str());

      // Load the configured TTF at the configured pixel size. Both feed the
      // atlas here, so this is what makes a font/size change in Preferences
      // actually show after rebuildFontAtlas().
      if (!fontPath.empty() && std::filesystem::exists(fontPath)) {
          ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), app::themeFontSizePx(), &cfg);
          if (font != nullptr) {
              g_fontFallback = false;
              return;
          }
          std::fprintf(stderr, "[font] failed to rasterize '%s'; using default font\n", fontPath.c_str());
      } else {
          std::fprintf(stderr, "[font] file not found '%s'; using default font\n", fontPath.c_str());
      }

      // Fallback: ImGui's built-in ProggyClean (fixed 13px bitmap, so font-size
      // changes won't show while this path is taken).
      io.Fonts->AddFontDefault();
      g_fontFallback = true;
  }

void registerRebuildFontAtlasCallback(RebuildFontAtlasFn cb) {
    g_rebuildFontAtlasCb = cb;
}

  void rebuildFontAtlas() {
      // ImGui locks the font atlas between NewFrame and Render — Fonts->Clear()
      // asserts if called inside that window. Set a flag instead; app::frame
      // drains it at the top of the next frame, before NewFrame locks the atlas.
      g_fontRebuildPending = true;
  }

namespace detail {

void ensureInitialized() {
    assert(g_state == State::Running &&
           "app::frame called before app::init or after app::shutdown");
}

} // namespace detail

std::string resolveAssetPath(const char* relative) {
    if (!relative) return std::string();
#if defined(IMGUI_SHELL_ASSETS_DIR)
    return std::string(IMGUI_SHELL_ASSETS_DIR) + "/" + relative;
#else
    return g_bundleResourcePath + "/" + relative;
#endif
}

void setBundleResourcePath(const char* path) {
    g_bundleResourcePath = path ? path : "";
}

void init(RenderContext& ctx) {
    assert(g_state != State::Running && "app::init called twice without intervening shutdown");

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    g_physicalDevice = ctx.physicalDevice;
    g_vulkanDevice = ctx.device;
    g_vulkanDescriptorPool = ctx.descriptorPool;
    
// Create node graph widget for all platforms
      g_nodeGraphWidget = std::make_unique<nodegraph::NodeGraphWidget>();
      
      // Auto-load default node graph project
      std::string defaultPath = resolveAssetPath("default_node_graph.json");
      if (std::filesystem::exists(defaultPath)) {
          g_nodeGraphWidget->loadFromFile(defaultPath);
      }
  #else
    (void)ctx;
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    app::applyTheme(ImGui::GetStyle());
    // Migrate any legacy single-file theme.json into the per-theme layout,
    // then apply whatever theme is currently selected. See theme-presets +
    // theme-persistence specs.
    app::migrateLegacyThemeFile();
    app::applySelectedThemeToStyle(ImGui::GetStyle());

    // Load application preferences
    app::loadPreferences();

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

// Drain any pending font-atlas rebuild BEFORE NewFrame locks the atlas.
    // See rebuildFontAtlas() for why this is deferred.
    if (g_fontRebuildPending && g_rebuildFontAtlasCb) {
        g_rebuildFontAtlasCb();
        g_fontRebuildPending = false;
    }

    ImGui::NewFrame();

    bool wantOpenAbout     = false;
    bool wantOpenImGuiDemo = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Preferences...")) {
                app::openPreferencesWindow();
            }
            if constexpr (kIsDesktop) {
                ImGui::Separator();
                if (ImGui::MenuItem("Save Node Graph...")) {
                    g_showSaveDialog = true;
                }
                if (ImGui::MenuItem("Load Node Graph...")) {
                    g_showLoadDialog = true;
                }
                ImGui::Separator();
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
        ImGui::Text("Default font size:  %.0f px", app::themeFontSizePx());
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

app::renderPreferencesWindow();

      // File dialogs for node graph persistence
      
      // Save dialog
      if (g_showSaveDialog) {
          ImGui::OpenPopup("Save Node Graph");
          g_fileDialogPath = "";
          g_fileDialogError = false;
          g_showSaveDialog = false;
      }
      
      if (ImGui::BeginPopupModal("Save Node Graph", nullptr, 
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
          
          ImGui::Text("Save node graph to JSON file:");
          ImGui::Spacing();
          
          // File path input
          static char filePathBuffer[256] = "";
          ImGui::InputText("File path", filePathBuffer, sizeof(filePathBuffer));
          
          ImGui::Spacing();
          
          if (g_fileDialogError) {
              ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", g_fileDialogErrorMessage.c_str());
              ImGui::Spacing();
          }
          
          if (ImGui::Button("Save", ImVec2(120, 0))) {
              std::string filePath = filePathBuffer;
              if (filePath.empty()) {
                  g_fileDialogError = true;
                  g_fileDialogErrorMessage = "File path cannot be empty";
              } else if (g_nodeGraphWidget->saveToFile(filePath)) {
                  ImGui::CloseCurrentPopup();
                  filePathBuffer[0] = '\0'; // Clear buffer
              } else {
                  g_fileDialogError = true;
                  g_fileDialogErrorMessage = "Failed to save file";
              }
          }
          
          ImGui::SameLine();
          if (ImGui::Button("Cancel", ImVec2(120, 0))) {
              ImGui::CloseCurrentPopup();
              filePathBuffer[0] = '\0';
          }
          
          ImGui::EndPopup();
      }
      
      // Load dialog
      if (g_showLoadDialog) {
          ImGui::OpenPopup("Load Node Graph");
          g_fileDialogPath = "";
          g_fileDialogError = false;
          g_showLoadDialog = false;
      }
      
if (ImGui::BeginPopupModal("Load Node Graph", nullptr, 
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            
            ImGui::Text("Load node graph from JSON file:");
            ImGui::Spacing();
          
          // File path input
          static char loadFilePathBuffer[256] = "";
          ImGui::InputText("File path", loadFilePathBuffer, sizeof(loadFilePathBuffer));
          
          ImGui::Spacing();
          
          if (g_fileDialogError) {
              ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", g_fileDialogErrorMessage.c_str());
              ImGui::Spacing();
          }
          
          if (ImGui::Button("Load", ImVec2(120, 0))) {
              std::string filePath = loadFilePathBuffer;
              if (filePath.empty()) {
                  g_fileDialogError = true;
                  g_fileDialogErrorMessage = "File path cannot be empty";
              } else if (g_nodeGraphWidget->loadFromFile(filePath)) {
                  ImGui::CloseCurrentPopup();
                  loadFilePathBuffer[0] = '\0'; // Clear buffer
              } else {
                  g_fileDialogError = true;
                  g_fileDialogErrorMessage = "Failed to load file";
              }
          }
          
          ImGui::SameLine();
          if (ImGui::Button("Cancel", ImVec2(120, 0))) {
              ImGui::CloseCurrentPopup();
              loadFilePathBuffer[0] = '\0';
          }
          
          ImGui::EndPopup();
      }

      // Render node graph if visible
      if (g_showNodeGraph) {
          g_nodeGraphWidget->render();
          // Input is handled by the widget itself
      }

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

    // Clean up node graph resources
    g_nodeGraphWidget.reset();

    ImGui::DestroyContext();
    g_state = State::ShutDown;
}

bool quitRequested() {
    return g_quitRequested;
}

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
VkPhysicalDevice activePhysicalDevice() {
    return g_physicalDevice;
}
#endif

// ---- Theme orchestration -----------------------------------------------

void applySelectedThemeToStyle(ImGuiStyle& style) {
    const std::string desired = app::readSelectedThemeName();
    app::setSelectedThemeName(desired);

    const auto themes = app::listAvailableThemes();
    if (themes.empty()) {
        // No themes available — keep whatever applyTheme left in `style`.
        return;
    }

    auto resolve = [&](const std::string& name) -> const app::ThemeEntry* {
        for (const auto& e : themes) if (e.name == name) return &e;
        return nullptr;
    };

    const app::ThemeEntry* hit = resolve(desired);
    if (!hit && desired != "dark") {
        std::fprintf(stderr,
            "[imgui-shell] selected theme '%s' not found; falling back to 'dark'\n",
            desired.c_str());
        hit = resolve("dark");
        if (hit) app::setSelectedThemeName("dark");
    }
    if (!hit) {
        std::fprintf(stderr,
            "[imgui-shell] fallback theme 'dark' not found either; keeping baked-in curated theme\n");
        return;
    }
    app::readThemeFile(hit->path, style);
}

  void switchTheme(const std::string& name) {
      app::writeSelectedThemeName(name);
      // writeSelectedThemeName already updated the cached selection on success.
      app::applySelectedThemeToStyle(ImGui::GetStyle());
      // Rebuild font atlas to apply any font changes from the new theme
      app::rebuildFontAtlas();
  }

  void resetSelectedTheme() {
      const std::string name = app::selectedThemeName();

      // Find the bundled origin (if any) — user-dir entries don't count as
      // "origin" since they are exactly what we're about to delete.
      std::string bundledPath = app::resolveAssetPath(("themes/" + name + ".json").c_str());
      std::error_code ec;
      if (!std::filesystem::exists(bundledPath, ec)) {
          std::fprintf(stderr,
              "[imgui-shell] Reset: '%s' has no bundled origin; no action taken\n",
              name.c_str());
          return;
      }

      // Delete the user-dir copy (silent if not present), then reload the bundled file.
      std::filesystem::remove(app::userThemePath(name), ec);
      app::readThemeFile(bundledPath, ImGui::GetStyle());
      // Rebuild font atlas to apply font changes
      app::rebuildFontAtlas();
  }

} // namespace app
