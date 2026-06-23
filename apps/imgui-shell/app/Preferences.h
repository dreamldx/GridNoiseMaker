#pragma once

#include <string>

namespace app {

// The Preferences window is a single regular ImGui Begin window (not a modal
// popup) toggled by an internal file-scope flag. openPreferencesWindow sets
// the flag to true; closing the window's X (via Begin's p_open) sets it back
// to false. There is only ever one Preferences window — the flag is the single
// source of truth.
void openPreferencesWindow();
void renderPreferencesWindow();

// Application preferences access
float getContextMenuMargin();
void setContextMenuMargin(float margin);
void loadPreferences();

// Node type panel preferences
struct NodeTypePanelState {
    bool visible = true;
    bool docked = true;
    bool floating = false;
    int viewMode = 0; // 0 = icon, 1 = list, 2 = detail
    float width = 250.0f;
    float height = 400.0f;
    float floatingX = 0.0f;
    float floatingY = 0.0f;
    std::string dockSide = "left";
};

NodeTypePanelState getNodeTypePanelState();
void setNodeTypePanelState(const NodeTypePanelState& state);
void saveNodeTypePanelState();

} // namespace app
