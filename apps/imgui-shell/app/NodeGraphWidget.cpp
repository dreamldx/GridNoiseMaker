// NodeGraphWidget.cpp
// Implementation of the node-graph canvas: type registry, JSON load/save, and
// the per-frame render + input handling. All geometry is kept in world space
// and converted to screen space through the grid renderer's ViewTransform at
// draw time, so pan/zoom affect everything uniformly. See NodeGraphWidget.h.

#include "NodeGraphWidget.h"
#include "Theme.h"
#include <imgui_internal.h>
#include <fstream>
#include <numeric>
#include <algorithm>

namespace nodegraph {

// ---- NodeGraphWidget ---------------------------------------------------

// Build the grid renderer, register the three built-in node types in the
// shared registry, and seed the canvas with a small demo input/processor/output
// graph so a fresh launch isn't empty.
NodeGraphWidget::NodeGraphWidget() {
    m_gridRenderer = std::make_unique<SimpleGridRenderer>();

    // Register default node types
    NodeTypeRegistry& registry = NodeTypeRegistry::instance();
    registry.registerType(NodeType("input", IM_COL32(60, 60, 90, 255), 
        nlohmann::json::object({{"value", 0.0}, {"label", "Input"}})));
    registry.registerType(NodeType("processor", IM_COL32(60, 90, 60, 255), 
        nlohmann::json::object({{"operation", "add"}, {"inputs", 2}})));
    registry.registerType(NodeType("output", IM_COL32(90, 60, 60, 255), 
        nlohmann::json::object({{"value", 0.0}, {"label", "Output"}})));
    
    // Add some test nodes with type information - positioned to overlap for z-order testing
    Node inputNode{ImVec2(100, 100), ImVec2(120, 80), IM_COL32(60, 60, 90, 255), IM_COL32(120, 120, 180, 255), "Input A"};
    inputNode.type = "input";
    inputNode.properties = nlohmann::json::object({{"value", 5.0}, {"label", "Input A"}});
    inputNode.zOrder = 3;  // Highest zOrder (drawn first, underneath)
    m_nodes.push_back(inputNode);
    
    Node processNode{ImVec2(120, 120), ImVec2(150, 100), IM_COL32(60, 90, 60, 255), IM_COL32(120, 180, 120, 255), "Processor"};
    processNode.type = "processor";
    processNode.properties = nlohmann::json::object({{"operation", "multiply"}, {"inputs", 2}});
    processNode.zOrder = 2;  // Middle zOrder
    m_nodes.push_back(processNode);
    
    Node outputNode{ImVec2(140, 140), ImVec2(120, 80), IM_COL32(90, 60, 60, 255), IM_COL32(180, 120, 120, 255), "Output"};
    outputNode.type = "output";
    outputNode.properties = nlohmann::json::object({{"value", 0.0}, {"label", "Final Output"}});
    outputNode.zOrder = 2;  // Same zOrder as processNode (tie goes to original order via stable_sort)
    m_nodes.push_back(outputNode);
}

NodeGraphWidget::~NodeGraphWidget() = default;

// Serialize the whole graph to a versioned JSON document: a "nodes" array plus
// a "nodeTypes" object mirroring the registry (so a file is self-describing and
// can re-register its types on load). "version" gates future schema migrations.
nlohmann::json NodeGraphWidget::toJson() const {
    nlohmann::json j;
    j["version"] = 1;
    
    // Serialize nodes
    nlohmann::json nodesArray = nlohmann::json::array();
    for (const auto& node : m_nodes) {
        nodesArray.push_back(node);
    }
    j["nodes"] = nodesArray;
    
    // Serialize node types from registry
    nlohmann::json nodeTypesObj = nlohmann::json::object();
    NodeTypeRegistry& registry = NodeTypeRegistry::instance();
    auto typeNames = registry.getTypeNames();
    for (const auto& typeName : typeNames) {
        const NodeType* type = registry.getType(typeName);
        if (type) {
            nlohmann::json typeJson = nlohmann::json::object({
                {"defaultColor", {
                    (type->defaultColor >> IM_COL32_R_SHIFT) & 0xFF,
                    (type->defaultColor >> IM_COL32_G_SHIFT) & 0xFF,
                    (type->defaultColor >> IM_COL32_B_SHIFT) & 0xFF,
                    (type->defaultColor >> IM_COL32_A_SHIFT) & 0xFF
                }},
                {"defaultProperties", type->defaultProperties}
            });
            nodeTypesObj[typeName] = typeJson;
        }
    }
    j["nodeTypes"] = nodeTypesObj;
    
    return j;
}

// Replace the current graph from a parsed JSON document. Returns false (leaving
// state as-is on a thrown exception, or cleared-then-partially-filled otherwise)
// if the document lacks a recognized "version". Any exception is swallowed and
// reported as a boolean failure so a bad file never crashes the app.
bool NodeGraphWidget::fromJson(const nlohmann::json& j, std::vector<std::string>* outSkippedTypes, int* outSkippedCount) {
    try {
        // Check version - support version 1 only for now
        if (!j.contains("version")) {
            return false;
        }
        int version = j["version"];
        if (version != 1) {
            // Future: add version migration logic here
            return false;
        }
        
        // Clear existing nodes
        m_nodes.clear();
        
        // Initialize statistics if output parameters provided
        std::vector<std::string> skippedTypesLocal;
        int skippedCountLocal = 0;
        
        // Deserialize nodes with type validation
        // Validates each node's type against NodeTypeRegistry, skips unknown types
        // Collects statistics on skipped nodes for user feedback
        if (j.contains("nodes") && j["nodes"].is_array()) {
            NodeTypeRegistry& registry = NodeTypeRegistry::instance();
            for (const auto& nodeJson : j["nodes"]) {
                // Parse node from JSON
                Node node;
                from_json(nodeJson, node);
                
                // Validate node type against registry
                // Returns nullptr if type name is not registered
                const NodeType* type = registry.getType(node.type);
                if (!type) {
                    // Skip node with unknown type
                    // This prevents creating "default" nodes with wrong properties
                    skippedCountLocal++;
                    // Track unique type names for user feedback
                    if (std::find(skippedTypesLocal.begin(), skippedTypesLocal.end(), node.type) == skippedTypesLocal.end()) {
                        skippedTypesLocal.push_back(node.type);
                    }
                    continue;
                }
                
                // Add valid node to graph
                // Only nodes with registered types are loaded
                m_nodes.push_back(node);
            }
        }
        
        // Deserialize node types (optional - types may already be registered)
        if (j.contains("nodeTypes") && j["nodeTypes"].is_object()) {
            NodeTypeRegistry& registry = NodeTypeRegistry::instance();
            for (const auto& [typeName, typeJson] : j["nodeTypes"].items()) {
                if (typeJson.contains("defaultColor") && typeJson["defaultColor"].is_array() && 
                    typeJson["defaultColor"].size() >= 4) {
                    auto colorArr = typeJson["defaultColor"];
                    ImU32 defaultColor = IM_COL32(colorArr[0], colorArr[1], colorArr[2], colorArr[3]);
                    nlohmann::json defaultProperties = typeJson.contains("defaultProperties") ? 
                        typeJson["defaultProperties"] : nlohmann::json::object();
                    registry.registerType(NodeType(typeName, defaultColor, defaultProperties));
                }
            }
        }
        
        // Return statistics if output parameters provided
        // These statistics are used by App.cpp to show user feedback dialog
        if (outSkippedTypes) {
            *outSkippedTypes = skippedTypesLocal;
        }
        if (outSkippedCount) {
            *outSkippedCount = skippedCountLocal;
        }
        
        // Returns true even if some nodes were skipped
        // This maintains backward compatibility: valid nodes are loaded, user gets feedback about skipped nodes
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Write the graph to `filePath` as pretty-printed JSON (4-space indent).
// Returns false on open/serialize failure; never throws.
bool NodeGraphWidget::saveToFile(const std::string& filePath) const {
    try {
        nlohmann::json j = toJson();
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        file << j.dump(4); // Pretty print with 4 spaces
        file.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Read and parse `filePath`, then apply it via fromJson. Returns false on
// open/parse/validation failure; never throws.
bool NodeGraphWidget::loadFromFile(const std::string& filePath, std::vector<std::string>* outSkippedTypes, int* outSkippedCount) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        nlohmann::json j;
        file >> j;
        return fromJson(j, outSkippedTypes, outSkippedCount);
    } catch (const std::exception& e) {
        return false;
    }
}

// Emit the canvas for this frame. Draws into a borderless, non-movable window
// pinned to the viewport work area (below the main menu bar): grid background,
// nodes, a small toolbar, and a right-click context menu whose padding is
// driven by the theme's popup-menu margin. Input is handled inline.
void NodeGraphWidget::render() {
    // Create a full-screen host window for the dock space
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Set up the host window to fill the entire viewport
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags host_window_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | 
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    static bool host_window_open = true;
    ImGui::Begin("DockSpaceHost", &host_window_open, host_window_flags);
    ImGui::PopStyleVar(3);
    
    // Create dock space inside the host window
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    
    // Set up initial docking layout on first run
    static bool layout_configured = false;
    if (!layout_configured) {
        layout_configured = true;
        
        // Clear any existing docking layout
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);
        
        // Split the dockspace into left and right
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
        
        // Dock panel to left, graph to main/center
        ImGui::DockBuilderDockWindow("Node Types", dock_left_id);
        ImGui::DockBuilderDockWindow("Node Graph", dock_main_id);
        
        // Configure the left dock for panel (optional: no tab bar)
        ImGuiDockNode* left_node = ImGui::DockBuilderGetNode(dock_left_id);
        if (left_node) {
            left_node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        }
        
        // Finish building
        ImGui::DockBuilderFinish(dockspace_id);
    }
    
    ImGui::End();
    
    // Now render windows that will dock into the dock space
    m_nodeTypePanel.render();
    renderGraphCanvas();
}

void NodeGraphWidget::renderGraphCanvas() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    // Window flags for the graph canvas
    // Remove NoDocking flag to allow docking
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    bool windowOpen = ImGui::Begin("Node Graph", nullptr, window_flags);
    if (windowOpen) {
        m_canvasPos = ImGui::GetCursorScreenPos();
        m_canvasSize = ImGui::GetContentRegionAvail();
        
        // Draw grid background
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(m_canvasPos, ImVec2(m_canvasPos.x + m_canvasSize.x, m_canvasPos.y + m_canvasSize.y), IM_COL32(30, 30, 30, 255));
        
        // Draw grid
        m_gridRenderer->draw(drawList, m_canvasPos, m_canvasSize);
        
        // Handle drag-and-drop for node creation
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_TYPE")) {
                const char* nodeTypeId = static_cast<const char*>(payload->Data);
                ImVec2 dropPos = ImGui::GetMousePos();
                createNodeFromDrag(nodeTypeId, dropPos);
            }
            ImGui::EndDragDropTarget();
        }
        
        // Handle input (must be before drawNodes to update z-order immediately)
        handleInput();
        
        // Draw nodes
        drawNodes(drawList);
        
        // Draw context menus
        drawContextMenus();
        
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

// Per-frame input: middle-drag pans, Ctrl+wheel zooms about the cursor,
// right-click opens context menus (canvas or node), and left-press/drag moves nodes. All
// gated on the canvas window being hovered so it doesn't steal input from
// overlapping ImGui widgets.
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
    
      // Ctrl+Mouse wheel for zooming. Pass the raw wheel delta; ViewTransform::zoom
      // already scales it by ZOOM_SPEED, so pre-multiplying here would double it.
      if (io.KeyCtrl && ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
          m_gridRenderer->zoom(io.MouseWheel, io.MousePos);
      }
      
      // Update node dragging for left-click, node selection for both left and right-click
      updateNodeDragging(io.MousePos, ImGui::IsMouseDown(ImGuiMouseButton_Left), 
          ImGui::IsMouseClicked(ImGuiMouseButton_Right));
  }

// Draw every node back-to-front: convert its world-space rect to screen space
// via the current view, then emit a filled body, a border, and the title text.
// Nodes are sorted by zOrder (lower values draw on top).
void NodeGraphWidget::drawNodes(ImDrawList* drawList) {
    // Get indices sorted by zOrder (descending, higher zOrder first)
    // Lower zOrder values draw on top, so sort descending to draw higher zOrder first (underneath)
    std::vector<size_t> sortedIndices = getSortedIndicesByZOrderDescending();
    
    // Draw nodes in sorted order (higher zOrder values drawn first, underneath)
    for (size_t idx : sortedIndices) {
        auto& node = m_nodes[idx];
        ImVec2 screenPos = m_gridRenderer->getView().worldToScreen(node.position);
        ImVec2 screenSize = ImVec2(node.size.x * m_gridRenderer->getView().getZoom(), node.size.y * m_gridRenderer->getView().getZoom());
        
        // Draw node body
        drawList->AddRectFilled(
            screenPos,
            ImVec2(screenPos.x + screenSize.x, screenPos.y + screenSize.y),
            node.color, 4.0f
        );
        
        // Draw node border with highlighting for selected nodes
        ImU32 borderColor = node.selected ? IM_COL32(255, 200, 50, 255) : node.borderColor; // Yellow for selected
        float borderThickness = node.selected ? 3.0f : 2.0f;
        drawList->AddRect(
            screenPos,
            ImVec2(screenPos.x + screenSize.x, screenPos.y + screenSize.y),
            borderColor, 4.0f, 0, borderThickness
        );
        
        // Draw node title
        ImVec2 textPos = ImVec2(screenPos.x + 8.0f, screenPos.y + 8.0f);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), node.title.c_str());
        
        // Draw z-order value below the node (outside bounds)
        std::string zOrderText = "Z: " + std::to_string(node.zOrder);
        ImVec2 textSize = ImGui::CalcTextSize(zOrderText.c_str());
        ImVec2 zOrderPos = ImVec2(
            screenPos.x + (screenSize.x - textSize.x) * 0.5f,  // Center horizontally
            screenPos.y + screenSize.y + 4.0f                   // Position below node
        );
        drawList->AddText(zOrderPos, IM_COL32(200, 200, 200, 200), zOrderText.c_str());
    }
}

// Drag at most one node per gesture. On the initial left-press, hit-test
// top-most-first (lowest zOrder) and latch that node; while held, translate the latched node by
// the screen mouse delta converted to world units; on release, clear all latches.
// Also handles node selection: clicked node becomes selected (zOrder=1), others deselected.
// Right-click triggers selection without dragging.
void NodeGraphWidget::updateNodeDragging(const ImVec2& mousePos, bool mouseDown, bool rightClick) {
    ImGuiIO& io = ImGui::GetIO();
    const ViewTransform& view = m_gridRenderer->getView();
    const float zoom = view.getZoom();

    // Clear dragging flags when mouse is not down (for left-click dragging only)
    if (!mouseDown) {
        for (auto& node : m_nodes) {
            node.dragging = false;
        }
    }

    // Handle selection on click (left-click for dragging, right-click for context menu)
    bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || rightClick;
    // Allow hover check even when blocked by our own popup or active item
    ImGuiHoveredFlags hoverFlags = ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
    bool windowHovered = ImGui::IsWindowHovered(hoverFlags);
    if (mouseClicked && windowHovered) {
        // Clear all dragging flags (only for left-click)
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            for (auto& node : m_nodes) {
                node.dragging = false;
            }
        }
        
        // Create sorted indices by zOrder (descending, higher zOrder first)
        std::vector<size_t> sortedIndices = getSortedIndicesByZOrderDescending();
        
        Node* clickedNode = nullptr;
        size_t clickedNodeIndex = 0;
        
        // Check nodes in REVERSE sorted order (from lowest to highest zOrder)
        // Since we draw higher zOrder first (underneath), lower zOrder last (on top)
        // we want to hit test from lowest zOrder (on top) to highest (underneath)
        for (auto it = sortedIndices.rbegin(); it != sortedIndices.rend(); ++it) {
            size_t idx = *it;
            Node& node = m_nodes[idx];
            ImVec2 screenPos = view.worldToScreen(node.position);
            ImVec2 screenSize = ImVec2(node.size.x * zoom, node.size.y * zoom);
            if (mousePos.x >= screenPos.x && mousePos.x <= screenPos.x + screenSize.x &&
                mousePos.y >= screenPos.y && mousePos.y <= screenPos.y + screenSize.y) {
                clickedNode = &node;
                clickedNodeIndex = idx;
                break;
            }
        }
        
        // Handle selection and z-order updates
        if (clickedNode) {
            // Store original z-order values before making any changes
            std::vector<int> originalZOrders;
            originalZOrders.reserve(m_nodes.size());
            for (const auto& node : m_nodes) {
                originalZOrders.push_back(node.zOrder);
            }
            
            int clickedOriginalZOrder = clickedNode->zOrder;
            
            // Deselect all nodes first
            for (auto& node : m_nodes) {
                node.selected = false;
            }
            
            // Apply hierarchical z-order preservation algorithm
            // If clicked node already has zOrder 1, no changes to other nodes (idempotent)
            // BREAKING CHANGE: Previously all non-selected nodes reset to zOrder 2.
            // Now preserves relative hierarchy: nodes with zOrder <= clicked node's original zOrder
            // shift down by 1, nodes with higher zOrder remain unchanged.
            // Empty canvas click also preserves hierarchy instead of resetting to zOrder 2.
            if (clickedOriginalZOrder != 1) {
                for (size_t i = 0; i < m_nodes.size(); ++i) {
                    if (i == clickedNodeIndex) {
                        continue; // Skip clicked node, will be set to 1 below
                    }
                    
                    int originalZOrder = originalZOrders[i];
                    if (originalZOrder <= clickedOriginalZOrder) {
                        // Shift down nodes with original zOrder less than or equal to clicked node's original zOrder
                        m_nodes[i].zOrder = originalZOrder + 1;
                    }
                    // Nodes with originalZOrder > clickedOriginalZOrder remain unchanged
                }
            }
            
          // Select clicked node and set to zOrder 1
          clickedNode->selected = true;
          clickedNode->zOrder = 1;
          clickedNode->dragging = ImGui::IsMouseClicked(ImGuiMouseButton_Left); // Only drag on left-click
          
          // If right-click, set context menu flag
          if (rightClick) {
              m_rightClickedNodeIndex = static_cast<int>(clickedNodeIndex);
              m_showNodeContextMenu = true;
          }
        } else {
          // Clicked on empty canvas - deselect all nodes, preserve z-order hierarchy
          for (auto& node : m_nodes) {
            node.selected = false;
            // zOrder values remain unchanged to preserve hierarchy
            // (previously reset all to zOrder 2, now keeps custom arrangements)
          }
        }
    }

    // Move whichever node is latched, converting screen delta to world delta.
    // Only update dragging when mouse is down
    if (mouseDown) {
        for (auto& node : m_nodes) {
            if (node.dragging) {
                node.position.x += io.MouseDelta.x / zoom;
                node.position.y += io.MouseDelta.y / zoom;
            }
        }
    }
}

// Helper function to get indices sorted by zOrder (descending, higher zOrder first)
std::vector<size_t> NodeGraphWidget::getSortedIndicesByZOrderDescending() const {
    std::vector<size_t> indices(m_nodes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::stable_sort(indices.begin(), indices.end(),
        [this](size_t a, size_t b) {
            return m_nodes[a].zOrder > m_nodes[b].zOrder;  // Higher zOrder first (drawn underneath)
        });
    return indices;
}

// Draw node and canvas context menus
void NodeGraphWidget::drawContextMenus() {
    // Apply theme's popup-menu margin to any context menus that open
    // We push here so it's active when/if any popup begins
    float margin = app::themePopupMenuMargin();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(margin, margin));
    
    // Canvas context menu (appears when right-clicking empty canvas)
    // Only show if node context menu is not open
    if (!ImGui::IsPopupOpen("NodeContextMenu") && ImGui::BeginPopupContextWindow("CanvasContextMenu")) {
        bool closePopup = false;
        
        if (ImGui::MenuItem("Reset View")) {
            m_gridRenderer->reset();
            closePopup = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Placeholder 1")) {
            // Placeholder action
            closePopup = true;
        }
        if (ImGui::MenuItem("Placeholder 2")) {
            // Placeholder action  
            closePopup = true;
        }
        
        if (closePopup) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // Node context menu (appears when right-clicking a node)
    // Note: This needs to be triggered from updateNodeDragging
    if (m_showNodeContextMenu && m_rightClickedNodeIndex >= 0 && m_rightClickedNodeIndex < m_nodes.size()) {
        ImGui::OpenPopup("NodeContextMenu");
        m_showNodeContextMenu = false; // Reset flag
    }
    
    if (ImGui::BeginPopup("NodeContextMenu")) {
        // Store node data before any vector modifications
        Node nodeCopy = m_nodes[m_rightClickedNodeIndex];
        
        ImGui::Text("Node: %s", nodeCopy.title.c_str());
        ImGui::Separator();
        
        bool closePopup = false;
        
        if (ImGui::MenuItem("Delete Node")) {
            // Delete the node
            m_nodes.erase(m_nodes.begin() + m_rightClickedNodeIndex);
            m_rightClickedNodeIndex = -1;
            closePopup = true;
        }
        
        if (ImGui::MenuItem("Duplicate Node")) {
            // Duplicate the node
            Node newNode = nodeCopy;
            newNode.position.x += 20.0f; // Offset slightly
            newNode.position.y += 20.0f;
            newNode.selected = false;
            newNode.dragging = false;
            newNode.zOrder = 2; // Default z-order
            m_nodes.push_back(newNode);
            closePopup = true;
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Node Properties")) {
            // Open properties dialog
            // Placeholder for now
            closePopup = true;
        }
        
        if (closePopup) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    } else {
        // Popup closed without selecting an item
        m_rightClickedNodeIndex = -1;
    }
    
    ImGui::PopStyleVar();
}

bool NodeGraphWidget::getNodeTypePanelVisible() const {
    return m_nodeTypePanel.isVisible();
}

void NodeGraphWidget::setNodeTypePanelVisible(bool visible) {
    m_nodeTypePanel.setVisible(visible);
}

void NodeGraphWidget::createNodeFromDrag(const std::string& nodeTypeId, const ImVec2& screenPos) {
    // Get current view transform
    const ViewTransform& view = m_gridRenderer->getView();
    
    // Convert screen position to world position using the view transform
    // Note: screenToWorld expects screen position relative to viewport
    // m_canvasPos is the viewport position, so adjust
    ImVec2 relativeScreenPos = screenPos;
    relativeScreenPos.x -= m_canvasPos.x;
    relativeScreenPos.y -= m_canvasPos.y;
    ImVec2 worldPos = view.screenToWorld(relativeScreenPos);
    
    // Create a new node
    Node newNode;
    newNode.title = "New " + nodeTypeId;
    newNode.type = nodeTypeId;
    newNode.position = worldPos;
    newNode.size = ImVec2(100.0f, 60.0f);
    newNode.color = IM_COL32(70, 130, 180, 255);
    newNode.selected = false;
    newNode.dragging = false;
    newNode.zOrder = 1; // Above grid, below UI
    
    // Add to nodes list
    m_nodes.push_back(newNode);
}

} // namespace nodegraph