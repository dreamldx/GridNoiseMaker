// NodeTypeRegistry.h
// Singleton registry for node types with color and default properties
// Separated from NodeGraphWidget.cpp for modularity

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace nodegraph {

// Forward declaration of Node struct (defined in NodeGraphWidget.h)
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
    NodeType(const std::string& name, ImU32 defaultColor,
             const nlohmann::json& defaultProperties);
};

class NodeTypeRegistry {
public:
    // Meyers singleton: constructed on first use and shared process-wide
    static NodeTypeRegistry& instance();

    // Insert or overwrite the type keyed by its name.
    void registerType(const NodeType& type);

    // Look up a type by name; nullptr when absent.
    const NodeType* getType(const std::string& name) const;

    // Collect all registered type names (hash-map order, i.e. unspecified).
    std::vector<std::string> getTypeNames() const;

    // Stamp out a Node from a registered type's template at `position`. Unknown
    // types yield a bare "default" node rather than failing.
    Node createNode(const std::string& typeName, const ImVec2& position);

private:
    NodeTypeRegistry(); // Private constructor for singleton
    std::unordered_map<std::string, NodeType> m_types;
};

} // namespace nodegraph