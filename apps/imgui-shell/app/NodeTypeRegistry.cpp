// NodeTypeRegistry.cpp
// Implementation of singleton NodeTypeRegistry for node type management

#include "NodeTypeRegistry.h"
#include "NodeGraphWidget.h"

namespace nodegraph {

NodeType::NodeType(const std::string& name, ImU32 defaultColor,
                   const nlohmann::json& defaultProperties)
    : name(name), defaultColor(defaultColor), defaultProperties(defaultProperties) {
}

NodeTypeRegistry::NodeTypeRegistry() {}

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

} // namespace nodegraph