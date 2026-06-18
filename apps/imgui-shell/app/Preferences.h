#pragma once

namespace app {

// The Preferences window is a single regular ImGui Begin window (not a modal
// popup) toggled by an internal file-scope flag. openPreferencesWindow sets
// the flag to true; closing the window's X (via Begin's p_open) sets it back
// to false. There is only ever one Preferences window — the flag is the single
// source of truth.
void openPreferencesWindow();
void renderPreferencesWindow();

} // namespace app
