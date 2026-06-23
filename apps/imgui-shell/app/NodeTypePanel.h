#pragma once

#include "Preferences.h"
#include "NodeTypeRegistry.h"

#include <imgui.h>
#include <string>
#include <vector>

namespace nodegraph {

class NodeTypePanel {
public:
    NodeTypePanel();
    ~NodeTypePanel() = default;

    // Main rendering method
    void render();

    // State management
    bool isVisible() const { return m_state.visible; }
    void setVisible(bool visible) { 
        m_state.visible = visible; 
        saveState();
    }
    
    const app::NodeTypePanelState& getState() const { return m_state; }
    void setState(const app::NodeTypePanelState& state) { m_state = state; }
    
    // Save/load state
    void saveState();
    void loadState();

private:
    void renderHeader();
    void renderContent();
    void renderIconView();
    void renderListView();
    void renderDetailView();
    
    void handleDragSource(const std::string& nodeTypeId, const std::string& nodeTypeName);
    
    app::NodeTypePanelState m_state;
    std::vector<std::string> m_nodeTypeIds;
    std::vector<std::string> m_nodeTypeNames;
    std::vector<ImVec4> m_nodeTypeColors;
    
    // UI state
    float m_iconSize = 64.0f;
    float m_listItemHeight = 30.0f;
    bool m_needsRefresh = true;
};

} // namespace nodegraph