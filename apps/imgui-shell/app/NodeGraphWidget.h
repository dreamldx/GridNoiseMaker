#pragma once
#include <imgui.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "SimpleGridRenderer.h"
#include <nlohmann/json.hpp>

// Define IM_COL32 shift macros if not already defined
#ifndef IM_COL32_R_SHIFT
#define IM_COL32_R_SHIFT 0
#define IM_COL32_G_SHIFT 8
#define IM_COL32_B_SHIFT 16
#define IM_COL32_A_SHIFT 24
#endif

namespace nodegraph {

struct Node;

class NodeType {
public:
    std::string name;
    ImU32 defaultColor;
    nlohmann::json defaultProperties;
    
    NodeType() : name(""), defaultColor(IM_COL32(45, 45, 45, 255)), defaultProperties(nlohmann::json::object()) {}
    NodeType(const std::string& name, ImU32 defaultColor = IM_COL32(45, 45, 45, 255), 
             const nlohmann::json& defaultProperties = nlohmann::json::object());
};

class NodeTypeRegistry {
public:
    static NodeTypeRegistry& instance();
    
    void registerType(const NodeType& type);
    const NodeType* getType(const std::string& name) const;
    std::vector<std::string> getTypeNames() const;
    Node createNode(const std::string& typeName, const ImVec2& position = ImVec2(0, 0));
    
private:
    NodeTypeRegistry();
    std::unordered_map<std::string, NodeType> m_types;
};

struct Node {
    ImVec2 position = ImVec2(0, 0);
    ImVec2 size = ImVec2(100, 60);
    ImU32 color = IM_COL32(45, 45, 45, 255);
    ImU32 borderColor = IM_COL32(100, 100, 100, 255);
    std::string title = "Node";
    bool dragging = false;
    std::string type = "default";
    nlohmann::json properties;
    
    // Type-safe property accessors
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

// JSON serialization for Node
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

class NodeGraphWidget {
public:
    NodeGraphWidget();
    ~NodeGraphWidget();
    
    void render();
    
    // JSON serialization
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& j);
    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);
    
private:
    std::unique_ptr<SimpleGridRenderer> m_gridRenderer;
    std::vector<Node> m_nodes;
    ImVec2 m_canvasPos;
    ImVec2 m_canvasSize;
    bool m_isDraggingView = false;
    ImVec2 m_lastMousePos;
    
    void handleInput();
    void drawNodes(ImDrawList* drawList);
    void updateNodeDragging(const ImVec2& mousePos, bool mouseDown);
};

} // namespace nodegraph