#include "NodeTypePanel.h"
#include "Preferences.h"
#include "NodeTypeRegistry.h"

#include <imgui.h>

namespace nodegraph {

NodeTypePanel::NodeTypePanel() {
    // Load initial state from preferences
    loadState();
}

void NodeTypePanel::render() {
    if (!m_state.visible) {
        return;
    }

    // Window flags - use ImGui's automatic docking
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    // Begin the window
    ImGui::SetNextWindowSize(ImVec2(m_state.width, m_state.height), ImGuiCond_FirstUseEver);
    
    const char* windowTitle = "Node Types";
    if (!ImGui::Begin(windowTitle, &m_state.visible, window_flags)) {
        // Don't call ImGui::End() when Begin returns false
        return;
    }

    // Render header with controls
    renderHeader();
    
    // Render content based on view mode
    renderContent();
    
    // Save window state if changed
    ImVec2 currentSize = ImGui::GetWindowSize();
    ImVec2 currentPos = ImGui::GetWindowPos();
    
    if (currentSize.x != m_state.width || currentSize.y != m_state.height) {
        m_state.width = currentSize.x;
        m_state.height = currentSize.y;
        saveState();
    }
    
    
    
    ImGui::End();
}

void NodeTypePanel::renderHeader() {
    // View mode buttons
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    
    if (ImGui::Button("Icon", ImVec2(60, 24))) {
        m_state.viewMode = 0;
        saveState();
    }
    ImGui::SameLine();
    if (ImGui::Button("List", ImVec2(60, 24))) {
        m_state.viewMode = 1;
        saveState();
    }
    ImGui::SameLine();
    if (ImGui::Button("Detail", ImVec2(60, 24))) {
        m_state.viewMode = 2;
        saveState();
    }
    
    
        ImGui::PopStyleVar();
    ImGui::Separator();
}

void NodeTypePanel::renderContent() {
    // Refresh node type list if needed
    if (m_needsRefresh) {
        m_nodeTypeIds.clear();
        m_nodeTypeNames.clear();
        m_nodeTypeColors.clear();
        
        auto& registry = NodeTypeRegistry::instance();
        auto typeNames = registry.getTypeNames();
        
        for (const auto& typeName : typeNames) {
            const NodeType* type = registry.getType(typeName);
            if (type) {
                m_nodeTypeIds.push_back(typeName);
                m_nodeTypeNames.push_back(type->name);
                
                // Convert color to ImVec4
                ImVec4 color;
                color.x = ((type->defaultColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
                color.y = ((type->defaultColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
                color.z = ((type->defaultColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
                color.w = 1.0f;
                m_nodeTypeColors.push_back(color);
            }
        }
        
        m_needsRefresh = false;
    }
    
    // Render based on view mode
    switch (m_state.viewMode) {
        case 0: // Icon view
            renderIconView();
            break;
        case 1: // List view
            renderListView();
            break;
        case 2: // Detail view
            renderDetailView();
            break;
    }
}

void NodeTypePanel::renderIconView() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    
    float windowWidth = ImGui::GetWindowWidth();
    int itemsPerRow = std::max(1, static_cast<int>(windowWidth / (m_iconSize + 16)));
    
    for (size_t i = 0; i < m_nodeTypeIds.size(); ++i) {
        if (i % itemsPerRow != 0) {
            ImGui::SameLine();
        }
        
        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginGroup();
        
        // Icon button with color
        ImGui::PushStyleColor(ImGuiCol_Button, m_nodeTypeColors[i]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
            m_nodeTypeColors[i].x * 1.2f,
            m_nodeTypeColors[i].y * 1.2f,
            m_nodeTypeColors[i].z * 1.2f,
            1.0f
        ));
        
        if (ImGui::Button("##icon", ImVec2(m_iconSize, m_iconSize))) {
            // Handle click if needed
        }
        
        // Make it draggable
        if (ImGui::BeginDragDropSource()) {
            handleDragSource(m_nodeTypeIds[i], m_nodeTypeNames[i], i);
            ImGui::EndDragDropSource();
        }
        
        ImGui::PopStyleColor(2);
        
        // Label below icon
        ImGui::TextWrapped("%s", m_nodeTypeNames[i].c_str());
        
        ImGui::EndGroup();
        ImGui::PopID();
    }
    
    ImGui::PopStyleVar();
}

void NodeTypePanel::renderListView() {
    for (size_t i = 0; i < m_nodeTypeIds.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        
        // List item with color indicator
        ImGui::PushStyleColor(ImGuiCol_Button, m_nodeTypeColors[i]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
            m_nodeTypeColors[i].x * 1.2f,
            m_nodeTypeColors[i].y * 1.2f,
            m_nodeTypeColors[i].z * 1.2f,
            1.0f
        ));
        
        if (ImGui::Button("##listitem", ImVec2(-1, m_listItemHeight))) {
            // Handle click if needed
        }
        
        // Make it draggable
        if (ImGui::BeginDragDropSource()) {
            handleDragSource(m_nodeTypeIds[i], m_nodeTypeNames[i], i);
            ImGui::EndDragDropSource();
        }
        
        ImGui::PopStyleColor(2);
        
        // Text overlay
        ImGui::SameLine(10);
        ImGui::Text("%s", m_nodeTypeNames[i].c_str());
        
        ImGui::PopID();
    }
}

void NodeTypePanel::renderDetailView() {
    for (size_t i = 0; i < m_nodeTypeIds.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        
                // Color indicator (non-interactive visual)
        ImGui::ColorButton("##color", m_nodeTypeColors[i], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(20, 20));
        ImGui::SameLine();
        
        // Name and details - vertically centered with color button
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", m_nodeTypeNames[i].c_str());
        ImGui::TextDisabled("ID: %s", m_nodeTypeIds[i].c_str());
        ImGui::EndGroup();
        
        // Make the whole row draggable
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            handleDragSource(m_nodeTypeIds[i], m_nodeTypeNames[i], i);
            ImGui::EndDragDropSource();
        }
        
        ImGui::Separator();
        ImGui::PopID();
    }
}

void NodeTypePanel::handleDragSource(const std::string& nodeTypeId, const std::string& nodeTypeName, size_t index) {
    // Set drag payload
    ImGui::SetDragDropPayload("NODE_TYPE", nodeTypeId.c_str(), nodeTypeId.size() + 1);
    
    // Visual feedback
    ImGui::Text("Create %s", nodeTypeName.c_str());
    
    // Optional: Draw a preview image
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Safety check for index bounds
    if (index < m_nodeTypeColors.size()) {
        ImGui::GetWindowDrawList()->AddCircleFilled(mousePos, 10, ImGui::GetColorU32(m_nodeTypeColors[index]), 12);
    } else {
        // Fallback to default color if index is out of bounds
        ImGui::GetWindowDrawList()->AddCircleFilled(mousePos, 10, IM_COL32(255, 255, 255, 255), 12);
    }
}

void NodeTypePanel::saveState() {
    app::setNodeTypePanelState(m_state);
    app::saveNodeTypePanelState();
}

void NodeTypePanel::loadState() {
    m_state = app::getNodeTypePanelState();
}

} // namespace nodegraph