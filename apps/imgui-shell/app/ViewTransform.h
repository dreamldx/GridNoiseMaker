// ViewTransform.h
// Simple 2D pan/zoom view transform for the node graph (no glm dependency).

#pragma once

#include <imgui.h>

namespace nodegraph {

// Maps between world space and screen space under a uniform scale (zoom) plus
// translation. The relationship is a single pair of formulas (see
// ViewTransform.cpp), so no matrix machinery is needed.
class ViewTransform {
public:
    ViewTransform();

    // Coordinate conversion
    ImVec2 worldToScreen(const ImVec2& worldPos) const;
    ImVec2 screenToWorld(const ImVec2& screenPos) const;

    // View manipulation
    void pan(const ImVec2& deltaScreen);
    void zoom(float delta, const ImVec2& centerScreen);
    void reset();

    // Grid snapping (correct but not yet wired up; kept for the planned
    // node-drag snap feature).
    ImVec2 snapToGrid(const ImVec2& worldPos, float gridSize) const;
    bool shouldSnap(const ImVec2& worldPos, float gridSize, float tolerance = 5.0f) const;

    // Accessors
    float getZoom() const { return m_zoom; }
    void setViewport(const ImVec2& pos, const ImVec2& size);

    // Zoom constraints
    static constexpr float MIN_ZOOM = 0.01f;
    static constexpr float MAX_ZOOM = 100.0f;
    static constexpr float ZOOM_SPEED = 0.1f;

private:
    ImVec2 m_viewOffset   = {0.0f, 0.0f};  // World point shown at canvas center
    float  m_zoom         = 1.0f;          // Screen pixels per world unit
    ImVec2 m_viewportPos  = {0.0f, 0.0f};  // Canvas top-left in screen space
    ImVec2 m_viewportSize = {0.0f, 0.0f};  // Canvas size in screen space
};

} // namespace nodegraph
