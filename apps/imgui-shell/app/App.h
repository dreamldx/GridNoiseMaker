#pragma once

#include "Platform.h"
#include "RenderContext.h"

#include <imgui.h>  // ImGuiStyle, used by applySelectedThemeToStyle

#include <string>

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    #include <vulkan/vulkan.h>
#endif

namespace app {

// Lifecycle interface — see specs/app-shell/spec.md.
//
// Call order from a host:
//   init(ctx);
//   while (running) { /* host backend NewFrame */ frame(ctx); /* host present */ }
//   shutdown();
//
// init owns the ImGui::CreateContext call. shutdown owns the matching
// ImGui::DestroyContext. Hosts MUST NOT create/destroy the ImGui context
// themselves.
void init(RenderContext& ctx);
void frame(RenderContext& ctx);
void shutdown();

// Returns true on desktop after the user has picked File > Quit. Always false
// on mobile (iOS apps are terminated by the OS, not user-driven exit).
bool quitRequested();

// iOS only: the host calls this in viewDidLoad with NSBundle.mainBundle.resourcePath
// before invoking app::init, so the shared core can resolve bundled assets like
// the default font. On desktop, app/ uses the IMGUI_SHELL_ASSETS_DIR compile-def
// and ignores this API.
void setBundleResourcePath(const char* path);

// iOS only: the host calls this in viewDidLoad with the NSDocumentDirectory path
// (writable, persists across launches) so the theme-persistence layer can store
// theme.json there. On desktop, the theme config path uses XDG_CONFIG_HOME / APPDATA
// instead and this API is a no-op. See specs/theme-persistence.
void setDocumentsPath(const char* path);

// Resolve a bundled asset path (relative to assets/) to an absolute path.
// On desktop, joins the IMGUI_SHELL_ASSETS_DIR compile-def with the relative
// portion. On iOS, joins the runtime NSBundle.mainBundle.resourcePath
// (supplied via setBundleResourcePath) with the relative portion. Empty input
// returns an empty string.
std::string resolveAssetPath(const char* relative);

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
// Returns the active VkPhysicalDevice as recorded during app::init from the
// supplied RenderContext. Used by the Preferences General tab to surface
// Vulkan API version + GPU device name. Returns VK_NULL_HANDLE before init
// or if the host hasn't populated RenderContext.physicalDevice.
VkPhysicalDevice activePhysicalDevice();
#endif

// ---- Font atlas runtime rebuild ----------------------------------------
//
// Re-rasterizes the ImGui font atlas using the current themeFontFile() /
// themeFontSizePx() values. The actual GPU-side texture lifecycle is owned
// by the platform layer (Vulkan / Metal); platform hosts register a callback
// during init that wraps the backend-specific Destroy / Create calls. See
// specs/font-rendering "Platform supplies the rebuild callback during init".
void configureFontAtlas();  // was anon-namespace static; now public so the
                            // platform-side rebuild callback can re-run the
                            // font-loading sequence after io.Fonts->Clear().
using RebuildFontAtlasFn = void (*)();
void registerRebuildFontAtlasCallback(RebuildFontAtlasFn cb);
void rebuildFontAtlas();    // silent no-op if no callback registered.

// ---- Theme orchestration -----------------------------------------------
//
// Reads the current selection (via app::selectedThemeName / readSelectedThemeName),
// resolves it through app::listAvailableThemes, and applies the resolved file's
// content to `style`. Missing selection or no available theme = silent no-op
// (caller's prior applyTheme curated state remains).
void applySelectedThemeToStyle(ImGuiStyle& style);

// Writes the new selection to theme.json + re-applies it to ImGui::GetStyle()
// for an immediate live repaint.
void switchTheme(const std::string& name);

// Deletes the user-dir copy of the currently-selected theme (if any) and
// reloads the bundled file. No-op + stderr line for user-only themes.
void resetSelectedTheme();

} // namespace app
