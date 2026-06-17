#pragma once

#include <imgui.h>
#include <string>

namespace app {

// Preferences dialog manager.
//
// On desktop platforms (macOS/Windows/Linux), the dialog opens as a secondary OS
// window when ImGuiConfigFlags_ViewportsEnable is active.
//
// On iOS (no multi-viewport), it appears as an inline floating panel.
//
// The dialog provides three tabs:
//   - General: read‑only diagnostic info (backend, viewports, config path)
//   - Theme:   master–detail editor for all style colors and metrics, with
//              Save / Reset to defaults / Discard changes buttons.
//   - About:   static credits and version info.
//
// User‑theme persistence uses ThemeStorage::readThemeFromConfig / writeThemeToConfig.
class Preferences {
public:
    // Creates a new Preferences instance. Does not show the dialog.
    Preferences();

    // Destructor (no cleanup required).
    ~Preferences();

    // Returns true if the dialog is currently visible.
    bool isVisible() const;

    // Shows the dialog (call from a menu‑item callback).
    void show();

    // Hides the dialog (call from a menu‑item callback or the dialog's own close button).
    void hide();

    // Draws the dialog. Call this in your app's frame loop; it will do nothing
    // if the dialog is not visible. On desktop, it opens a secondary OS window
    // (multi‑viewport). On iOS, it renders an inline floating panel.
    void render();

    // Returns the last‑selected theme‑editing slot (color or metric). Used to
    // preserve selection across frames while editing.
    int selectedThemeSlot() const;

private:
    bool visible_ = false;
    int selectedThemeSlot_ = -1;   // -1 = none selected
    std::string configPathText_;    // cached for General tab

    // Tab rendering helpers
    void renderGeneralTab();
    void renderThemeTab();
    void renderAboutTab();

    // Theme‑editor helpers
    void renderThemeColorList();
    void renderThemeMetricList();
    void renderThemeEditor();
    void applyLiveThemeChanges();
};

} // namespace app