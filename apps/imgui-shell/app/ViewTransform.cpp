// ViewTransform.cpp
// Implementation of 2D view transformations for node graph

#include "ViewTransform.h"
#include <cmath>
#include <algorithm>

namespace nodegraph {

// Constructor
ViewTransform::ViewTransform()
    : m_viewOffset(0.0f, 0.0f)
    , m_zoom(1.0f)
    , m_viewportSize(0.0f, 0.0f)
    , m_viewportCenter(0.0f, 0.0f)
    , m_transformDirty(true) {
}

// Convert world coordinates to screen coordinates
ImVec2 ViewTransform::worldToScreen(const ImVec2& worldPos) const {
    if (m_transformDirty) {
        const_cast<ViewTransform*>(this)->updateTransforms();
    }
    
    // Apply transform and add viewport position offset
    ImVec2 screenPos = m_worldToScreen.transform(worldPos);
    return ImVec2(screenPos.x + m_viewportPos.x, screenPos.y + m_viewportPos.y);
}

// Convert screen coordinates to world coordinates
ImVec2 ViewTransform::screenToWorld(const ImVec2& screenPos) const {
    if (m_transformDirty) {
        const_cast<ViewTransform*>(this)->updateTransforms();
    }
    
    // Subtract viewport position before applying inverse transform
    ImVec2 viewportRelativePos = ImVec2(screenPos.x - m_viewportPos.x, screenPos.y - m_viewportPos.y);
    return m_screenToWorld.transform(viewportRelativePos);
}

// Pan the view
void ViewTransform::pan(const ImVec2& deltaScreen) {
    // Simple formula: delta in screen space divided by zoom
    m_viewOffset.x -= deltaScreen.x / m_zoom;
    m_viewOffset.y -= deltaScreen.y / m_zoom;
    m_transformDirty = true;
}

// Zoom the view
void ViewTransform::zoom(float delta, const ImVec2& centerScreen) {
    // Convert center point to world coordinates before zoom
    ImVec2 worldCenter = screenToWorld(centerScreen);
    
    // Calculate new zoom level with constraints
    float oldZoom = m_zoom;
    m_zoom *= (1.0f + delta * ZOOM_SPEED);
    m_zoom = std::clamp(m_zoom, MIN_ZOOM, MAX_ZOOM);
    
    // If zoom changed, adjust view offset to keep center point stationary
    if (m_zoom != oldZoom) {
        // Convert center back to screen with new zoom
        m_transformDirty = true;
        ImVec2 newScreenCenter = worldToScreen(worldCenter);
        
        // Adjust offset to compensate for zoom
        ImVec2 centerDelta = ImVec2(
            centerScreen.x - newScreenCenter.x,
            centerScreen.y - newScreenCenter.y
        );
        
        // Convert center delta to world delta and adjust offset
        ImVec2 worldDelta = screenToWorld(centerDelta);
        ImVec2 origin = screenToWorld(ImVec2(0.0f, 0.0f));
        
        m_viewOffset.x -= (worldDelta.x - origin.x);
        m_viewOffset.y -= (worldDelta.y - origin.y);
        
        // Note: m_transformDirty is already true from line 61
    }
}

// Reset view to default
void ViewTransform::reset() {
    m_viewOffset = ImVec2(0.0f, 0.0f);
    m_zoom = 1.0f;
    m_transformDirty = true;
}

// Snap world position to grid
ImVec2 ViewTransform::snapToGrid(const ImVec2& worldPos, float gridSize) const {
    if (gridSize <= 0.0f) {
        return worldPos;
    }
    
    float snappedX = std::round(worldPos.x / gridSize) * gridSize;
    float snappedY = std::round(worldPos.y / gridSize) * gridSize;
    
    return ImVec2(snappedX, snappedY);
}

// Check if position should snap to grid
bool ViewTransform::shouldSnap(const ImVec2& worldPos, float gridSize, float tolerance) const {
    if (gridSize <= 0.0f) {
        return false;
    }
    
    ImVec2 snapped = snapToGrid(worldPos, gridSize);
    float dx = worldPos.x - snapped.x;
    float dy = worldPos.y - snapped.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    return distance <= tolerance;
}

// Set view offset
void ViewTransform::setViewOffset(const ImVec2& offset) {
    m_viewOffset = offset;
    m_transformDirty = true;
}

// Set zoom level
void ViewTransform::setZoom(float zoom) {
    m_zoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
    m_transformDirty = true;
}

// Set viewport position and size
void ViewTransform::setViewport(const ImVec2& pos, const ImVec2& size) {
    // Position is applied as a post-transform offset in worldToScreen/
    // screenToWorld, so it never feeds the matrices and doesn't dirty them.
    m_viewportPos = pos;

    // Only size affects the matrices (via the viewport center), so recompute
    // them solely when it actually changes — this runs every frame.
    if (size.x != m_viewportSize.x || size.y != m_viewportSize.y) {
        m_viewportSize = size;
        m_viewportCenter = ImVec2(size.x * 0.5f, size.y * 0.5f);
        m_transformDirty = true;
    }
}

// Check if world position is visible
bool ViewTransform::isVisible(const ImVec2& worldPos, float radius) const {
    ImVec2 screenPos = worldToScreen(worldPos);
    
    // Check if within viewport with margin
    float margin = radius * m_zoom;
    return (screenPos.x >= -margin && 
            screenPos.x <= m_viewportSize.x + margin &&
            screenPos.y >= -margin && 
            screenPos.y <= m_viewportSize.y + margin);
}

// Get visible world bounds
void ViewTransform::getVisibleWorldBounds(ImVec2& min, ImVec2& max) const {
    // Convert viewport corners to world coordinates
    min = screenToWorld(ImVec2(0.0f, 0.0f));
    max = screenToWorld(m_viewportSize);
    
    // Ensure min < max
    if (min.x > max.x) std::swap(min.x, max.x);
    if (min.y > max.y) std::swap(min.y, max.y);
}

// Update transformation matrices
void ViewTransform::updateTransforms() {
    if (!m_transformDirty) {
        return;
    }
    
    // Create scale matrix
    TransformMatrix scale;
    scale.m[0] = m_zoom;     // a
    scale.m[3] = m_zoom;     // d
    // b, c, tx, ty remain 0
    
    // Create translation to viewport center
    TransformMatrix translateToCenter;
    translateToCenter.m[4] = m_viewportCenter.x;  // tx
    translateToCenter.m[5] = m_viewportCenter.y;  // ty
    
    // Create view offset (inverse because we move the world, not the camera)
    TransformMatrix viewOffset;
    viewOffset.m[4] = -m_viewOffset.x;  // tx
    viewOffset.m[5] = -m_viewOffset.y;  // ty
    
    // Combined: world -> screen
    m_worldToScreen = translateToCenter.combine(scale.combine(viewOffset));
    
    // Inverse: screen -> world
    m_screenToWorld = m_worldToScreen.inverse();
    
    m_transformDirty = false;
}

// ViewController implementation
ViewController::ViewController(ViewTransform& view)
    : m_view(view)
    , m_isDragging(false)
    , m_dragStartPos(0.0f, 0.0f)
    , m_dragStartViewOffset(0.0f, 0.0f)
    , m_panButton(2)  // Middle mouse
    , m_zoomSpeed(0.1f)
    , m_targetZoom(1.0f)
    , m_smoothZoom(false) {
}

bool ViewController::handleMouseDrag(const ImVec2& mouseDelta, bool middleButtonDown) {
    if (middleButtonDown && !m_isDragging) {
        // Start dragging
        m_isDragging = true;
        m_dragStartPos = ImVec2(0.0f, 0.0f); // Not used for panning
        m_dragStartViewOffset = m_view.getViewOffset();
    }
    
    if (m_isDragging && middleButtonDown) {
        // Continue dragging
        m_view.pan(ImVec2(-mouseDelta.x, -mouseDelta.y));
        return true;
    }
    
    if (m_isDragging && !middleButtonDown) {
        // Stop dragging
        m_isDragging = false;
    }
    
    return false;
}

bool ViewController::handleMouseWheel(float wheelDelta, const ImVec2& mousePos) {
    if (wheelDelta == 0.0f) {
        return false;
    }
    
    if (m_smoothZoom) {
        // Smooth zoom to target
        m_targetZoom *= (1.0f + wheelDelta * m_zoomSpeed);
        m_targetZoom = std::clamp(m_targetZoom, 
                                 ViewTransform::MIN_ZOOM, 
                                 ViewTransform::MAX_ZOOM);
        
        // Smooth interpolation would go here
        m_view.setZoom(m_targetZoom);
    } else {
        // Immediate zoom
        m_view.zoom(wheelDelta, mousePos);
    }
    
    return true;
}

bool ViewController::handleMouseClick(const ImVec2& mousePos, int button, bool pressed) {
    if (button == m_panButton) {
        if (pressed && !m_isDragging) {
            // Start pan
            m_isDragging = true;
            m_dragStartPos = mousePos;
            m_dragStartViewOffset = m_view.getViewOffset();
            return true;
        } else if (!pressed && m_isDragging) {
            // End pan
            m_isDragging = false;
            return true;
        }
    }
    
    return false;
}

} // namespace nodegraph