// NodeGraphWidget.cpp
// Implementation of the node-graph canvas: type registry, JSON load/save, and
// the per-frame render + input handling. All geometry is kept in world space
// and converted to screen space through the grid renderer's ViewTransform at
// draw time, so pan/zoom affect everything uniformly. See NodeGraphWidget.h.

#include "NodeGraphWidget.h"
#include "Theme.h"
#include <imgui_internal.h>
#include <fstream>

namespace nodegraph {

// ---- NodeType ----------------------------------------------------------

// Construct a named type with its default body color and property template.
NodeType::NodeType(const std::string& name, ImU32 defaultColor,
                   const nlohmann::json& defaultProperties)
    : name(name), defaultColor(defaultColor), defaultProperties(defaultProperties) {
}

// ---- NodeTypeRegistry --------------------------------------------------

NodeTypeRegistry::NodeTypeRegistry() {
}

// Meyers singleton: the registry is constructed on first use and shared process-wide.
NodeTypeRegistry& NodeTypeRegistry::instance() {
    static NodeTypeRegistry registry;
    return registry;
}

// Insert or overwrite the type keyed by its name.
void NodeTypeRegistry::registerType(const NodeType& type) {
    m_types[type.name] = type;
}

// Look up a type by name; nullptr when absent.
const NodeType* NodeTypeRegistry::getType(const std::string& name) const {
    auto it = m_types.find(name);
    if (it != m_types.end()) {
        return &it->second;
    }
    return nullptr;
}

// Collect all registered type names (hash-map order, i.e. unspecified).
std::vector<std::string> NodeTypeRegistry::getTypeNames() const {
    std::vector<std::string> names;
    names.reserve(m_types.size());
    for (const auto& pair : m_types) {
        names.push_back(pair.first);
    }
    return names;
}

// Stamp out a Node from a registered type's template at `position`. Unknown
// types yield a bare "default" node rather than failing.
Node NodeTypeRegistry::createNode(const std::string& typeName, const ImVec2& position) {
    const NodeType* type = getType(typeName);
    if (!type) {
        // Fallback to default type
        Node node;
        node.position = position;
        node.type = "default";
        node.properties = nlohmann::json::object();
        return node;
    }
    
    Node node;
    node.position = position;
    node.size = ImVec2(100, 60);
    node.color = type->defaultColor;
    node.borderColor = IM_COL32(100, 100, 100, 255);
    node.title = type->name;
    node.type = type->name;
    node.properties = type->defaultProperties;
    return node;
}

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
    
    // Add some test nodes with type information
    Node inputNode{ImVec2(100, 100), ImVec2(120, 80), IM_COL32(60, 60, 90, 255), IM_COL32(120, 120, 180, 255), "Input A"};
    inputNode.type = "input";
    inputNode.properties = nlohmann::json::object({{"value", 5.0}, {"label", "Input A"}});
    m_nodes.push_back(inputNode);
    
    Node processNode{ImVec2(300, 150), ImVec2(150, 100), IM_COL32(60, 90, 60, 255), IM_COL32(120, 180, 120, 255), "Processor"};
    processNode.type = "processor";
    processNode.properties = nlohmann::json::object({{"operation", "multiply"}, {"inputs", 2}});
    m_nodes.push_back(processNode);
    
    Node outputNode{ImVec2(500, 100), ImVec2(120, 80), IM_COL32(90, 60, 60, 255), IM_COL32(180, 120, 120, 255), "Output"};
    outputNode.type = "output";
    outputNode.properties = nlohmann::json::object({{"value", 0.0}, {"label", "Final Output"}});
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
bool NodeGraphWidget::fromJson(const nlohmann::json& j) {
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
        
        // Deserialize nodes
        if (j.contains("nodes") && j["nodes"].is_array()) {
            for (const auto& nodeJson : j["nodes"]) {
                Node node;
                from_json(nodeJson, node);
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
bool NodeGraphWidget::loadFromFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        nlohmann::json j;
        file >> j;
        return fromJson(j);
    } catch (const std::exception& e) {
        return false;
    }
}

// Emit the canvas for this frame. Draws into a borderless, non-movable window
// pinned to the viewport work area (below the main menu bar): grid background,
// nodes, a small toolbar, and a right-click context menu whose padding is
// driven by the theme's popup-menu margin. Input is handled inline.
void NodeGraphWidget::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    // Get the main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Set window position and size to fill the viewport work area (below menu bar)
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    // Window flags - no title bar, no resize, no move
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    if (ImGui::Begin("Node Graph", nullptr, window_flags)) {
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

        // Context menu with configurable margin and rectangular style
        // Apply margin BEFORE BeginPopup so it affects the popup window creation
        float margin = app::themePopupMenuMargin();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(margin, margin));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        
        if (ImGui::BeginPopupContextWindow("NodeGraphContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Reset View")) {
                m_gridRenderer->reset();
            }
            ImGui::Separator();
            ImGui::MenuItem("Menu Item 1", nullptr, false, false);
            ImGui::MenuItem("Menu Item 2", nullptr, false, false);
            ImGui::MenuItem("Menu Item 3", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Menu Item 4", nullptr, false, false);
            ImGui::MenuItem("Menu Item 5", nullptr, false, false);
            
            ImGui::EndPopup();
        }
        
        ImGui::PopStyleVar(2); // Restore window padding and rounding
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// Per-frame input: middle-drag pans, Ctrl+wheel zooms about the cursor,
// right-click opens the context menu, and left-press/drag moves nodes. All
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
    
    // Context menu trigger on right-click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
        ImGui::OpenPopup("NodeGraphContextMenu");
    }
    
    // Update node dragging
    updateNodeDragging(io.MousePos, ImGui::IsMouseDown(ImGuiMouseButton_Left));
}

// Draw every node back-to-front: convert its world-space rect to screen space
// via the current view, then emit a filled body, a border, and the title text.
// Draw order is vector order, so later nodes render on top.
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

// Drag at most one node per gesture. On the initial left-press, hit-test
// top-most-first and latch that node; while held, translate the latched node by
// the screen mouse delta converted to world units; on release, clear all latches.
void NodeGraphWidget::updateNodeDragging(const ImVec2& mousePos, bool mouseDown) {
    ImGuiIO& io = ImGui::GetIO();
    const ViewTransform& view = m_gridRenderer->getView();
    const float zoom = view.getZoom();

    if (!mouseDown) {
        // Mouse released, stop all dragging
        for (auto& node : m_nodes) {
            node.dragging = false;
        }
        return;
    }

    // Latch the node to drag on the initial press only. Re-running the hit test
    // every frame would cancel the drag whenever a fast mouse motion outran the
    // node and left its rect. Iterate back-to-front so the top-most (last drawn)
    // node under the cursor wins, and pick exactly one.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        for (auto& node : m_nodes) {
            node.dragging = false;
        }
        for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
            Node& node = *it;
            ImVec2 screenPos = view.worldToScreen(node.position);
            ImVec2 screenSize = ImVec2(node.size.x * zoom, node.size.y * zoom);
            if (mousePos.x >= screenPos.x && mousePos.x <= screenPos.x + screenSize.x &&
                mousePos.y >= screenPos.y && mousePos.y <= screenPos.y + screenSize.y) {
                node.dragging = true;
                break;
            }
        }
    }

    // Move whichever node is latched, converting screen delta to world delta.
    for (auto& node : m_nodes) {
        if (node.dragging) {
            node.position.x += io.MouseDelta.x / zoom;
            node.position.y += io.MouseDelta.y / zoom;
        }
    }
}

} // namespace nodegraph