#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace app {

// ---- Per-OS root paths --------------------------------------------------

// Returns the absolute path to the per-user theme config file (selection-only
// since `add-theme-picker` — colors / metrics / typography live in per-theme
// files under userThemesDir(), not inline in this file).
//   macOS / Linux: ${XDG_CONFIG_HOME:-$HOME/.config}/imgui-shell/theme.json
//   Windows:       ${APPDATA}/imgui-shell/theme.json
//   iOS:           <Documents>/theme.json
std::string themeConfigPath();

// Returns the absolute path to the user-writable themes dir
// (themeConfigPath()'s parent + "/themes"). May not exist on disk until the
// first writeThemeFile call into it.
std::string userThemesDir();

// Returns the user-dir per-theme file path for the given theme name (no
// extension; e.g., "dark" -> ".../themes/dark.json").
std::string userThemePath(const std::string& name);

// ---- Selection persistence (theme.json) ---------------------------------

// Reads `selected_theme` from themeConfigPath(). Missing file / missing field
// returns "dark" silently. Malformed JSON logs once and returns "dark".
std::string readSelectedThemeName();

// Atomically rewrites themeConfigPath() to
// {"_schema_version": 2, "selected_theme": "<name>"}.
void writeSelectedThemeName(const std::string& name);

// Cached accessor — returns the current selection without re-reading disk.
// Populated by readSelectedThemeName / applySelectedThemeToStyle / switchTheme.
const std::string& selectedThemeName();
void               setSelectedThemeName(const std::string& name); // cache update only

// ---- Per-theme file I/O (themes/<name>.json) ----------------------------

// Parses any per-theme JSON file (colors / metrics / typography blocks) and
// applies its content to `style` plus the typography accessors. Missing file
// is a silent no-op. Unknown keys log one warning each (existing semantics).
void readThemeFile(const std::string& path, ImGuiStyle& style);

// Atomically writes the full colors / metrics / typography blob for `style`
// to `path` (temp-file + rename, parent dirs created as needed).
void writeThemeFile(const std::string& path, const ImGuiStyle& style);

// ---- Listing themes (bundled ∪ user, user wins on collision) ------------

struct ThemeEntry {
    std::string name;        // stem (filename without .json)
    std::string path;        // absolute path to the resolved file
    bool        fromUserDir; // true if the path resolved to userThemesDir()
};

std::vector<ThemeEntry> listAvailableThemes();

// ---- Migration of legacy theme.json -------------------------------------

// On first run with this change, detects a pre-`_schema_version` theme.json
// that still has inline colors / metrics / typography, copies those into
// userThemesDir() + "/custom.json", then rewrites theme.json to the modern
// selection-only schema with selected_theme = "custom". Silent no-op if the
// file is already modern or absent.
void migrateLegacyThemeFile();

// ---- iOS host helper (unchanged) ----------------------------------------

void setDocumentsPath(const char* path);

} // namespace app
