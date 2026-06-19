#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include "ViewTransform.h"

namespace nodegraph {

/// SimpleGridRenderer - Renders a scalable grid for canvas-based applications
/// Features configurable grid size, colors, and adaptive zoom-based spacing
class SimpleGridRenderer {
public:
    SimpleGridRenderer();
    
    void draw(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize);
    
    // View manipulation
    void pan(const ImVec2& deltaScreen);
    void zoom(float delta, const ImVec2& centerScreen);
    void reset();
    
    const ViewTransform& getView() const { return m_view; }
    
    // Configuration methods
    void setGridSize(float size);
    float getGridSize() const;
    
    void setMinorGridColor(ImU32 color);
    ImU32 getMinorGridColor() const;
    
    void setMajorGridColor(ImU32 color);
    ImU32 getMajorGridColor() const;
    
private:
    ViewTransform m_view;
    float m_gridSize = 100.0f;
    ImU32 m_minorGridColor = IM_COL32(60, 60, 60, 255);
    ImU32 m_majorGridColor = IM_COL32(80, 80, 80, 255);
    
    void drawGridLines(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize);
};

} // namespace nodegraph