#include "SimpleGridRenderer.h"
#include <cmath>

namespace nodegraph {

// Named constants for zoom thresholds and multipliers
constexpr float ZOOM_THRESHOLD_VERY_FAR = 0.3f;
constexpr float ZOOM_THRESHOLD_FAR = 0.7f;
constexpr float GRID_MULTIPLIER_VERY_FAR = 4.0f;
constexpr float GRID_MULTIPLIER_FAR = 2.0f;
constexpr float MAJOR_LINE_INTERVAL = 5.0f;
constexpr float MIN_GRID_SPACING = 1.0f;
constexpr float MIN_CANVAS_SIZE = 1.0f;

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

void SimpleGridRenderer::setGridSize(float size) {
    if (size < MIN_GRID_SPACING) {
        size = MIN_GRID_SPACING;
    }
    m_gridSize = size;
}

float SimpleGridRenderer::getGridSize() const {
    return m_gridSize;
}

void SimpleGridRenderer::setMinorGridColor(ImU32 color) {
    m_minorGridColor = color;
}

ImU32 SimpleGridRenderer::getMinorGridColor() const {
    return m_minorGridColor;
}

void SimpleGridRenderer::setMajorGridColor(ImU32 color) {
    m_majorGridColor = color;
}

ImU32 SimpleGridRenderer::getMajorGridColor() const {
    return m_majorGridColor;
}

void SimpleGridRenderer::drawGridLines(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    // Early return for invalid canvas sizes
    if (canvasSize.x < MIN_CANVAS_SIZE || canvasSize.y < MIN_CANVAS_SIZE) {
        return;
    }
    
    // Convert screen bounds to world coordinates
    ImVec2 worldMin = m_view.screenToWorld(canvasPos);
    ImVec2 worldMax = m_view.screenToWorld(ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y));
    
    // Determine appropriate grid spacing based on zoom with logarithmic scaling
    float zoomLevel = m_view.getZoom();
    float gridSpacing = m_gridSize;
    
    // Smooth logarithmic scaling: grid spacing = base * 2^(log2(zoomThreshold/zoomLevel))
    // This creates continuous scaling without visual popping
    if (zoomLevel < ZOOM_THRESHOLD_VERY_FAR) {
        float scaleFactor = std::pow(2.0f, std::log2(ZOOM_THRESHOLD_VERY_FAR / zoomLevel));
        gridSpacing = m_gridSize * scaleFactor;
    } else if (zoomLevel < ZOOM_THRESHOLD_FAR) {
        float scaleFactor = std::pow(2.0f, std::log2(ZOOM_THRESHOLD_FAR / zoomLevel));
        gridSpacing = m_gridSize * scaleFactor;
    }
    // For zoomLevel >= ZOOM_THRESHOLD_FAR, use base grid spacing
    
    // Early return if grid spacing is too small
    if (gridSpacing < MIN_GRID_SPACING) {
        return;
    }
    
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
    
    // Draw major grid lines every MAJOR_LINE_INTERVAL lines
    float majorSpacing = gridSpacing * MAJOR_LINE_INTERVAL;
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