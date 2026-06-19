// ViewTransform.h
// Simple 2D view transformations without glm dependency

#pragma once

#include <imgui.h>
#include <cstdint>
#include <cmath>

namespace nodegraph {

// Simple 2x3 transformation matrix for 2D graphics
struct TransformMatrix {
    float m[6]; // [a, b, c, d, tx, ty]
    
    TransformMatrix() {
        // Identity matrix
        m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
        m[3] = 1.0f;
        m[4] = 0.0f; m[5] = 0.0f;
    }
    
    // Apply transform to point
    ImVec2 transform(const ImVec2& point) const {
        return ImVec2(
            m[0] * point.x + m[2] * point.y + m[4],
            m[1] * point.x + m[3] * point.y + m[5]
        );
    }
    
    // Combine two transforms (this * other)
    TransformMatrix combine(const TransformMatrix& other) const {
        TransformMatrix result;
        result.m[0] = m[0] * other.m[0] + m[2] * other.m[1];
        result.m[1] = m[1] * other.m[0] + m[3] * other.m[1];
        result.m[2] = m[0] * other.m[2] + m[2] * other.m[3];
        result.m[3] = m[1] * other.m[2] + m[3] * other.m[3];
        result.m[4] = m[0] * other.m[4] + m[2] * other.m[5] + m[4];
        result.m[5] = m[1] * other.m[4] + m[3] * other.m[5] + m[5];
        return result;
    }
    
    // Create inverse transform
    TransformMatrix inverse() const {
        TransformMatrix inv;
        float det = m[0] * m[3] - m[2] * m[1];
        
        if (std::fabs(det) > 1e-6f) {
            float invDet = 1.0f / det;
            inv.m[0] = m[3] * invDet;
            inv.m[1] = -1.0f * m[1] * invDet;
            inv.m[2] = -1.0f * m[2] * invDet;
            inv.m[3] = m[0] * invDet;
            inv.m[4] = (m[2] * m[5] - m[4] * m[3]) * invDet;
            inv.m[5] = (m[4] * m[1] - m[0] * m[5]) * invDet;
        }
        
        return inv;
    }
};

// Handles 2D view transformations (pan, zoom) for node graph editor
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
    
    // Grid snapping
    ImVec2 snapToGrid(const ImVec2& worldPos, float gridSize) const;
    bool shouldSnap(const ImVec2& worldPos, float gridSize, float tolerance = 5.0f) const;
    
    // Getters
    const ImVec2& getViewOffset() const { return m_viewOffset; }
    float getZoom() const { return m_zoom; }
    void getViewportSize(ImVec2& size) const { size = m_viewportSize; }
    
    // Setters
    void setViewOffset(const ImVec2& offset);
    void setZoom(float zoom);
    void setViewport(const ImVec2& pos, const ImVec2& size);
    
    // Bounds checking
    bool isVisible(const ImVec2& worldPos, float radius = 0.0f) const;
    void getVisibleWorldBounds(ImVec2& min, ImVec2& max) const;
    
    // Zoom constraints
    static constexpr float MIN_ZOOM = 0.01f;
    static constexpr float MAX_ZOOM = 100.0f;
    static constexpr float ZOOM_SPEED = 0.1f;
    
private:
    void updateTransforms();
    
private:
    // View state
    ImVec2 m_viewOffset = {0.0f, 0.0f};  // World space
    float m_zoom = 1.0f;                    // Scale factor
    
    // Viewport
    ImVec2 m_viewportPos = {0.0f, 0.0f};    // Screen position
    ImVec2 m_viewportSize = {0.0f, 0.0f};   // Screen size
    ImVec2 m_viewportCenter = {0.0f, 0.0f}; // Screen center (relative to viewportPos)
    
    // Precomputed transformation matrices
    TransformMatrix m_worldToScreen;
    TransformMatrix m_screenToWorld;
    
    // Dirty flag for matrix updates
    bool m_transformDirty = true;
};

// Helper for handling mouse input
class ViewController {
public:
    ViewController(ViewTransform& view);
    
    // Input handling
    bool handleMouseDrag(const ImVec2& mouseDelta, bool middleButtonDown);
    bool handleMouseWheel(float wheelDelta, const ImVec2& mousePos);
    bool handleMouseClick(const ImVec2& mousePos, int button, bool pressed);
    
    // State
    bool isDragging() const { return m_isDragging; }
    const ImVec2& getDragStartPos() const { return m_dragStartPos; }
    
    // Settings
    void setPanButton(int button) { m_panButton = button; }
    void setZoomSpeed(float speed) { m_zoomSpeed = speed; }
    
private:
    ViewTransform& m_view;
    
    // Drag state
    bool m_isDragging = false;
    ImVec2 m_dragStartPos = {0.0f, 0.0f};
    ImVec2 m_dragStartViewOffset = {0.0f, 0.0f};
    
    // Input settings
    int m_panButton = 2;  // Middle mouse button by default
    float m_zoomSpeed = 0.1f;
    
    // Smooth zoom
    float m_targetZoom = 1.0f;
    bool m_smoothZoom = false;
};

} // namespace nodegraph