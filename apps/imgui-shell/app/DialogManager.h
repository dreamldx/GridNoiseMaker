// DialogManager.h
// Centralized dialog state management for UI notifications
// Singleton pattern maintains global accessibility while encapsulating state

#pragma once

#include <string>
#include <vector>

class DialogManager {
public:
    // Meyers singleton: constructed on first use and shared process-wide
    static DialogManager& instance();

    // Unknown node types dialog management
    void setUnknownNodeTypes(const std::vector<std::string>& types, int count);
    bool shouldShowUnknownNodeTypesDialog() const;
    void resetUnknownNodeTypesDialog();
    const std::vector<std::string>& getUnknownNodeTypes() const;
    int getUnknownNodeCount() const;

private:
    DialogManager(); // Private constructor for singleton

    // Unknown node types dialog state
    bool m_showUnknownNodeTypesDialog = false;
    std::vector<std::string> m_unknownNodeTypes;
    int m_unknownNodeCount = 0;
};