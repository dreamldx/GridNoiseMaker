#pragma once

#include <imgui.h>

#include <string>

namespace app {

// Returns the absolute path to the per-user theme config file.
// Platform-conventional paths (see specs/theme-persistence "Per-OS config path resolver"):
//   macOS / Linux: ${XDG_CONFIG_HOME:-$HOME/.config}/imgui-shell/theme.json
//   Windows:       ${APPDATA}/imgui-shell/theme.json
//   iOS:           <Documents>/theme.json  (Documents path set via setDocumentsPath)
std::string themeConfigPath();

// Reads the user's theme tweaks from themeConfigPath() and applies them on top
// of the given style. Missing file = silent no-op. Malformed JSON = log + bail.
// Unknown color/metric keys = log one warning per key + continue.
// MUST be called AFTER applyTheme so user values take precedence.
void readThemeFromConfig(ImGuiStyle& style);

// Atomically writes the given style to themeConfigPath() — writes to a temp
// file in the same directory then renames over the target. Creates parent
// directories as needed. Logs one error on failure.
void writeThemeToConfig(const ImGuiStyle& style);

} // namespace app
