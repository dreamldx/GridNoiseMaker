#include "SimpleGridRenderer.h"
#include <cmath>

namespace nodegraph {

SimpleGridRenderer::SimpleGridRenderer() : m_view() {}

void SimpleGridRenderer::draw(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    drawGridLines(drawList, canvasPos, canvasSize);
}

void SimpleGridRenderer::pan(const ImVec2& deltaScreen) {
    m_view.pan(deltaScreen);
}

void SimpleGridRenderer::zoom(float delta, const ImVec2& centerScreen) {
    m_view.zoom(delta, centerScreen);
}

void SimpleGridRenderer::reset() {
    m_view.reset();
}

void SimpleGridRenderer::drawGridLines(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    // Convert screen bounds to world coordinates
    ImVec2 worldMin = m_view.screenToWorld(canvasPos);
    ImVec2 worldMax = m_view.screenToWorld(ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y));
    
    // Determine appropriate grid spacing based on zoom
    float zoomLevel = m_view.getZoom();
    float gridSpacing = m_gridSize;
    if (zoomLevel < 0.3f) gridSpacing = m_gridSize * 4.0f;
    else if (zoomLevel < 0.7f) gridSpacing = m_gridSize * 2.0f;
    
    // Draw minor grid lines
    float startX = std::floor(worldMin.x / gridSpacing) * gridSpacing;
    float startY = std::floor(worldMin.y / gridSpacing) * gridSpacing;
    
    for (float x = startX; x <= worldMax.x; x += gridSpacing) {
        ImVec2 p1 = m_view.worldToScreen(ImVec2(x, worldMin.y));
        ImVec2 p2 = m_view.worldToScreen(ImVec2(x, worldMax.y));
        drawList->AddLine(p1, p2, m_minorGridColor, 1.0f);
    }
    
    for (float y = startY; y <= worldMax.y; y += gridSpacing) {
        ImVec2 p1 = m_view.worldToScreen(ImVec2(worldMin.x, y));
        ImVec2 p2 = m_view.worldToScreen(ImVec2(worldMax.x, y));
        drawList->AddLine(p1, p2, m_minorGridColor, 1.0f);
    }
    
    // Draw major grid lines every 5th line
    float majorSpacing = gridSpacing * 5.0f;
    float majorStartX = std::floor(worldMin.x / majorSpacing) * majorSpacing;
    float majorStartY = std::floor(worldMin.y / majorSpacing) * majorSpacing;
    
    for (float x = majorStartX; x <= worldMax.x; x += majorSpacing) {
        ImVec2 p1 = m_view.worldToScreen(ImVec2(x, worldMin.y));
        ImVec2 p2 = m_view.worldToScreen(ImVec2(x, worldMax.y));
        drawList->AddLine(p1, p2, m_majorGridColor, 2.0f);
    }
    
    for (float y = majorStartY; y <= worldMax.y; y += majorSpacing) {
        ImVec2 p1 = m_view.worldToScreen(ImVec2(worldMin.x, y));
        ImVec2 p2 = m_view.worldToScreen(ImVec2(worldMax.x, y));
        drawList->AddLine(p1, p2, m_majorGridColor, 2.0f);
    }
}

} // namespace nodegraph