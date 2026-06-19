#pragma once
#include <imgui.h>
#include <vector>
#include <memory>
#include <string>
#include "SimpleGridRenderer.h"

namespace nodegraph {

struct Node {
    ImVec2 position = ImVec2(0, 0);
    ImVec2 size = ImVec2(100, 60);
    ImU32 color = IM_COL32(45, 45, 45, 255);
    ImU32 borderColor = IM_COL32(100, 100, 100, 255);
    std::string title = "Node";
    bool dragging = false;
};

class NodeGraphWidget {
public:
    NodeGraphWidget();
    ~NodeGraphWidget();
    
    void render();
    
private:
    std::unique_ptr<SimpleGridRenderer> m_gridRenderer;
    std::vector<Node> m_nodes;
    ImVec2 m_canvasPos;
    ImVec2 m_canvasSize;
    bool m_isDraggingView = false;
    ImVec2 m_lastMousePos;
    
    void handleInput();
    void drawNodes(ImDrawList* drawList);
    void updateNodeDragging(const ImVec2& mousePos, bool mouseDown);
};

} // namespace nodegraph