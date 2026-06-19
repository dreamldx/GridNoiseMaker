// ViewTransform.cpp
// 2D pan/zoom view transform for the node graph.
//
// The view is a uniform scale (zoom) plus a translation, so world and screen
// coordinates relate by a single pair of formulas:
//
//   screen = zoom * (world - viewOffset) + viewportCenter + viewportPos
//   world  = (screen - viewportPos - viewportCenter) / zoom + viewOffset
//
// where viewportCenter = viewportSize / 2, so viewOffset is the world point
// shown at the center of the canvas.

#include "ViewTransform.h"
#include <algorithm>
#include <cmath>

namespace nodegraph {

ViewTransform::ViewTransform() = default;

ImVec2 ViewTransform::worldToScreen(const ImVec2& worldPos) const {
    return ImVec2(
        m_zoom * (worldPos.x - m_viewOffset.x) + m_viewportSize.x * 0.5f + m_viewportPos.x,
        m_zoom * (worldPos.y - m_viewOffset.y) + m_viewportSize.y * 0.5f + m_viewportPos.y
    );
}

ImVec2 ViewTransform::screenToWorld(const ImVec2& screenPos) const {
    return ImVec2(
        (screenPos.x - m_viewportPos.x - m_viewportSize.x * 0.5f) / m_zoom + m_viewOffset.x,
        (screenPos.y - m_viewportPos.y - m_viewportSize.y * 0.5f) / m_zoom + m_viewOffset.y
    );
}

void ViewTransform::pan(const ImVec2& deltaScreen) {
    // Move the world under the cursor: a screen delta is worth delta/zoom world units.
    m_viewOffset.x -= deltaScreen.x / m_zoom;
    m_viewOffset.y -= deltaScreen.y / m_zoom;
}

void ViewTransform::zoom(float delta, const ImVec2& centerScreen) {
    // Keep the world point under the cursor fixed: note where it is, change the
    // zoom, then shift the offset by however much that point moved on screen.
    ImVec2 worldBefore = screenToWorld(centerScreen);
    m_zoom = std::clamp(m_zoom * (1.0f + delta * ZOOM_SPEED), MIN_ZOOM, MAX_ZOOM);
    ImVec2 worldAfter = screenToWorld(centerScreen);
    m_viewOffset.x += worldBefore.x - worldAfter.x;
    m_viewOffset.y += worldBefore.y - worldAfter.y;
}

void ViewTransform::reset() {
    m_viewOffset = ImVec2(0.0f, 0.0f);
    m_zoom = 1.0f;
}

void ViewTransform::setViewport(const ImVec2& pos, const ImVec2& size) {
    m_viewportPos = pos;
    m_viewportSize = size;
}

ImVec2 ViewTransform::snapToGrid(const ImVec2& worldPos, float gridSize) const {
    if (gridSize <= 0.0f) {
        return worldPos;
    }
    return ImVec2(
        std::round(worldPos.x / gridSize) * gridSize,
        std::round(worldPos.y / gridSize) * gridSize
    );
}

bool ViewTransform::shouldSnap(const ImVec2& worldPos, float gridSize, float tolerance) const {
    if (gridSize <= 0.0f) {
        return false;
    }
    ImVec2 snapped = snapToGrid(worldPos, gridSize);
    float dx = worldPos.x - snapped.x;
    float dy = worldPos.y - snapped.y;
    return std::sqrt(dx * dx + dy * dy) <= tolerance;
}

} // namespace nodegraph
