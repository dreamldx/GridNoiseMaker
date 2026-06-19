#include "NodeGraphWidget.h"
#include <imgui_internal.h>

namespace nodegraph {

NodeGraphWidget::NodeGraphWidget() {
    m_gridRenderer = std::make_unique<SimpleGridRenderer>();
    
    // Add some test nodes
    m_nodes.push_back(Node{ImVec2(100, 100), ImVec2(120, 80), IM_COL32(60, 60, 90, 255), IM_COL32(120, 120, 180, 255), "Input"});
    m_nodes.push_back(Node{ImVec2(300, 150), ImVec2(150, 100), IM_COL32(60, 90, 60, 255), IM_COL32(120, 180, 120, 255), "Process"});
    m_nodes.push_back(Node{ImVec2(500, 100), ImVec2(120, 80), IM_COL32(90, 60, 60, 255), IM_COL32(180, 120, 120, 255), "Output"});
}

NodeGraphWidget::~NodeGraphWidget() = default;

void NodeGraphWidget::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("Node Graph", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        m_canvasPos = ImGui::GetCursorScreenPos();
        m_canvasSize = ImGui::GetContentRegionAvail();
        
        // Draw grid background
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(m_canvasPos, ImVec2(m_canvasPos.x + m_canvasSize.x, m_canvasPos.y + m_canvasSize.y), IM_COL32(30, 30, 30, 255));
        
        // Draw grid
        m_gridRenderer->draw(drawList, m_canvasPos, m_canvasSize);
        
        // Draw nodes
        drawNodes(drawList);
        
        // Handle input
        handleInput();
        
        // Reset button
        if (ImGui::Button("Reset View")) {
            m_gridRenderer->reset();
        }
        ImGui::SameLine();
        ImGui::Text("Pan: Middle Mouse | Zoom: Ctrl+Mouse Wheel");
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void NodeGraphWidget::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Middle mouse drag for panning
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) && ImGui::IsWindowHovered()) {
        if (!m_isDraggingView) {
            m_isDraggingView = true;
            m_lastMousePos = io.MousePos;
        } else {
            ImVec2 delta = ImVec2(io.MousePos.x - m_lastMousePos.x, io.MousePos.y - m_lastMousePos.y);
            m_gridRenderer->pan(delta);
            m_lastMousePos = io.MousePos;
        }
    } else {
        m_isDraggingView = false;
    }
    
    // Ctrl+Mouse wheel for zooming
    if (io.KeyCtrl && ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
        m_gridRenderer->zoom(io.MouseWheel * 0.1f, io.MousePos);
    }
    
    // Update node dragging
    updateNodeDragging(io.MousePos, ImGui::IsMouseDown(ImGuiMouseButton_Left));
}

void NodeGraphWidget::drawNodes(ImDrawList* drawList) {
    for (auto& node : m_nodes) {
        ImVec2 screenPos = m_gridRenderer->getView().worldToScreen(node.position);
        ImVec2 screenSize = ImVec2(node.size.x * m_gridRenderer->getView().getZoom(), node.size.y * m_gridRenderer->getView().getZoom());
        
        // Draw node body
        drawList->AddRectFilled(
            screenPos,
            ImVec2(screenPos.x + screenSize.x, screenPos.y + screenSize.y),
            node.color, 4.0f
        );
        
        // Draw node border
        drawList->AddRect(
            screenPos,
            ImVec2(screenPos.x + screenSize.x, screenPos.y + screenSize.y),
            node.borderColor, 4.0f, 0, 2.0f
        );
        
        // Draw node title
        ImVec2 textPos = ImVec2(screenPos.x + 8.0f, screenPos.y + 8.0f);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), node.title.c_str());
    }
}

void NodeGraphWidget::updateNodeDragging(const ImVec2& mousePos, bool mouseDown) {
    ImGuiIO& io = ImGui::GetIO();
    
    if (mouseDown) {
        bool hitNode = false;
        for (auto& node : m_nodes) {
            ImVec2 screenPos = m_gridRenderer->getView().worldToScreen(node.position);
            ImVec2 screenSize = ImVec2(node.size.x * m_gridRenderer->getView().getZoom(), node.size.y * m_gridRenderer->getView().getZoom());
            
            if (mousePos.x >= screenPos.x && mousePos.x <= screenPos.x + screenSize.x &&
                mousePos.y >= screenPos.y && mousePos.y <= screenPos.y + screenSize.y) {
                if (!node.dragging) {
                    node.dragging = true;
                }
                hitNode = true;
            }
        }
        
        // If clicked on empty space, stop all dragging
        if (!hitNode) {
            for (auto& node : m_nodes) {
                node.dragging = false;
            }
        }
    } else {
        // Mouse released, stop all dragging
        for (auto& node : m_nodes) {
            node.dragging = false;
        }
    }
    
    // Update positions of dragging nodes
    for (auto& node : m_nodes) {
        if (node.dragging) {
            // Calculate world delta from screen delta using current zoom
            ImVec2 screenDelta = ImVec2(io.MouseDelta.x, io.MouseDelta.y);
            node.position.x += screenDelta.x / m_gridRenderer->getView().getZoom();
            node.position.y += screenDelta.y / m_gridRenderer->getView().getZoom();
        }
    }
}

} // namespace nodegraph