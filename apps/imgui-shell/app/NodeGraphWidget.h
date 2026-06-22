// NodeGraphWidget.h
// A self-contained, from-scratch node-graph canvas built directly on ImGui's
// draw-list API (no third-party node-editor library). It owns:
//   * a pannable / zoomable grid canvas (via SimpleGridRenderer + ViewTransform),
//   * a flat list of draggable rectangular Nodes,
//   * a singleton NodeType registry that supplies per-type colors / default
//     properties, and
//   * versioned JSON load/save of the whole graph (nlohmann/json).
//
// The widget renders into a single full-viewport ImGui window and handles its
// own mouse input. It is instantiated and driven by app::frame (see App.cpp);
// nothing here touches the rendering backend directly.
//
// See specs/node-graph-widget and specs/node-graph-persistence for the contract.
#pragma once
#include <imgui.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "SimpleGridRenderer.h"
#include <nlohmann/json.hpp>

// IM_COL32 packs RGBA into a 32-bit ImU32. The component shift amounts are
// normally defined by imgui_internal.h; define them here as a fallback so the
// JSON (de)serializers below can unpack/repack color channels even when this
// header is included without the internal header.
#ifndef IM_COL32_R_SHIFT
#define IM_COL32_R_SHIFT 0
#define IM_COL32_G_SHIFT 8
#define IM_COL32_B_SHIFT 16
#define IM_COL32_A_SHIFT 24
#endif

namespace nodegraph {

struct Node;

// Describes a category of node ("input", "processor", "output", ...): the
// default body color and the default JSON property bag that new nodes of this
// type are stamped with. Templates only — individual Node instances carry their
// own copies, so editing one node never mutates the type.
class NodeType {
public:
    std::string name;
    ImU32 defaultColor;
    nlohmann::json defaultProperties;

    NodeType() : name(""), defaultColor(IM_COL32(45, 45, 45, 255)), defaultProperties(nlohmann::json::object()) {}
    NodeType(const std::string& name, ImU32 defaultColor = IM_COL32(45, 45, 45, 255),
             const nlohmann::json& defaultProperties = nlohmann::json::object());
};

// Process-wide registry of NodeTypes (Meyers singleton). The widget populates
// it with the built-in types in its constructor; JSON load may register
// additional types found in a saved file. Keyed by type name.
class NodeTypeRegistry {
public:
    // Returns the single shared registry instance.
    static NodeTypeRegistry& instance();

    // Inserts or overwrites the type keyed by `type.name`.
    void registerType(const NodeType& type);
    // Returns the type for `name`, or nullptr if not registered.
    const NodeType* getType(const std::string& name) const;
    // Returns the names of all registered types (unordered).
    std::vector<std::string> getTypeNames() const;
    // Builds a Node from the named type's template at `position`. Falls back to
    // a bare "default" node if the type is unknown.
    Node createNode(const std::string& typeName, const ImVec2& position = ImVec2(0, 0));

private:
    NodeTypeRegistry();
    std::unordered_map<std::string, NodeType> m_types;
};

// One node on the canvas: a rectangle in WORLD coordinates (position/size are
// pre-zoom), its colors, title, type tag, and a free-form JSON property bag.
// `dragging` is transient per-frame UI state, not persisted.
struct Node {
    ImVec2 position = ImVec2(0, 0);
    ImVec2 size = ImVec2(100, 60);
    ImU32 color = IM_COL32(45, 45, 45, 255);
    ImU32 borderColor = IM_COL32(100, 100, 100, 255);
    std::string title = "Node";
    bool dragging = false;
    std::string type = "default";
    nlohmann::json properties;
    
    // Type-safe property accessors over the JSON bag. getProperty returns
    // `defaultValue` if the key is missing or the stored value can't convert to
    // T; setProperty inserts/overwrites; hasProperty tests presence.
    template<typename T>
    T getProperty(const std::string& key, T defaultValue = T{}) const {
        if (properties.contains(key)) {
            try {
                return properties[key].get<T>();
            } catch (const std::exception&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    template<typename T>
    void setProperty(const std::string& key, const T& value) {
        properties[key] = value;
    }
    
    bool hasProperty(const std::string& key) const {
        return properties.contains(key);
    }
};

// nlohmann/json ADL hook: serialize a Node. Colors are written as [r,g,b,a]
// integer arrays (unpacked from the packed ImU32) so the on-disk form is
// human-readable and endianness-independent. dragging is intentionally omitted.
inline void to_json(nlohmann::json& j, const Node& node) {
    j = nlohmann::json::object({
        {"position", {node.position.x, node.position.y}},
        {"size", {node.size.x, node.size.y}},
        {"color", {
            (node.color >> IM_COL32_R_SHIFT) & 0xFF,
            (node.color >> IM_COL32_G_SHIFT) & 0xFF,
            (node.color >> IM_COL32_B_SHIFT) & 0xFF,
            (node.color >> IM_COL32_A_SHIFT) & 0xFF
        }},
        {"borderColor", {
            (node.borderColor >> IM_COL32_R_SHIFT) & 0xFF,
            (node.borderColor >> IM_COL32_G_SHIFT) & 0xFF,
            (node.borderColor >> IM_COL32_B_SHIFT) & 0xFF,
            (node.borderColor >> IM_COL32_A_SHIFT) & 0xFF
        }},
        {"title", node.title},
        {"type", node.type},
        {"properties", node.properties}
    });
}

// nlohmann/json ADL hook: deserialize a Node. Every field is optional and
// individually guarded so a partial or hand-edited JSON object still yields a
// valid node (unspecified fields keep their struct defaults).
inline void from_json(const nlohmann::json& j, Node& node) {
    if (j.contains("position")) {
        auto pos = j["position"];
        if (pos.is_array() && pos.size() >= 2) {
            node.position = ImVec2(pos[0], pos[1]);
        }
    }
    
    if (j.contains("size")) {
        auto size = j["size"];
        if (size.is_array() && size.size() >= 2) {
            node.size = ImVec2(size[0], size[1]);
        }
    }
    
    if (j.contains("color")) {
        auto color = j["color"];
        if (color.is_array() && color.size() >= 4) {
            node.color = IM_COL32(color[0], color[1], color[2], color[3]);
        }
    }
    
    if (j.contains("borderColor")) {
        auto borderColor = j["borderColor"];
        if (borderColor.is_array() && borderColor.size() >= 4) {
            node.borderColor = IM_COL32(borderColor[0], borderColor[1], borderColor[2], borderColor[3]);
        }
    }
    
    if (j.contains("title")) {
        node.title = j["title"];
    }
    
    if (j.contains("type")) {
        node.type = j["type"];
    }
    
    if (j.contains("properties")) {
        node.properties = j["properties"];
    }
}

// The node-graph canvas widget. One instance lives for the app's lifetime
// (owned by App.cpp). render() must be called once per ImGui frame.
class NodeGraphWidget {
public:
    // Registers the built-in node types and seeds a few demo nodes.
    NodeGraphWidget();
    ~NodeGraphWidget();

    // Emits the full-viewport canvas window for this frame: grid, nodes,
    // toolbar, context menu, and input handling.
    void render();

    // ---- JSON serialization (versioned; see specs/node-graph-persistence) ----
    nlohmann::json toJson() const;             // serialize nodes + type registry
    bool fromJson(const nlohmann::json& j);    // replace state from a parsed doc
    bool saveToFile(const std::string& filePath) const;  // toJson -> pretty file
    bool loadFromFile(const std::string& filePath);      // parse file -> fromJson

private:
    std::unique_ptr<SimpleGridRenderer> m_gridRenderer; // grid + pan/zoom view
    std::vector<Node> m_nodes;                          // graph contents (draw order)
    ImVec2 m_canvasPos;                                 // canvas top-left (screen px)
    ImVec2 m_canvasSize;                                // canvas size (screen px)
    bool m_isDraggingView = false;                      // middle-drag pan in progress
    ImVec2 m_lastMousePos;                              // anchor for pan delta

    void handleInput();                                 // pan / zoom / context-menu / drag
    void drawNodes(ImDrawList* drawList);               // world->screen rects + titles
    void updateNodeDragging(const ImVec2& mousePos, bool mouseDown); // node drag latch
};

} // namespace nodegraph