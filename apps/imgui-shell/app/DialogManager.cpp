// DialogManager.cpp
// Implementation of DialogManager singleton for UI dialog state management

#include "DialogManager.h"

DialogManager& DialogManager::instance() {
    static DialogManager manager;
    return manager;
}

DialogManager::DialogManager() {}

void DialogManager::setUnknownNodeTypes(const std::vector<std::string>& types, int count) {
    m_unknownNodeTypes = types;
    m_unknownNodeCount = count;
    m_showUnknownNodeTypesDialog = true;
}

bool DialogManager::shouldShowUnknownNodeTypesDialog() const {
    return m_showUnknownNodeTypesDialog;
}

void DialogManager::resetUnknownNodeTypesDialog() {
    m_showUnknownNodeTypesDialog = false;
    m_unknownNodeTypes.clear();
    m_unknownNodeCount = 0;
}

const std::vector<std::string>& DialogManager::getUnknownNodeTypes() const {
    return m_unknownNodeTypes;
}

int DialogManager::getUnknownNodeCount() const {
    return m_unknownNodeCount;
}