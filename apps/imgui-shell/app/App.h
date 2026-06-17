#pragma once

#include "Platform.h"
#include "RenderContext.h"

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

} // namespace app
