#pragma once

#include <memory>
#include <string>

namespace app {

/**
 * Abstract base class for cross-platform file dialog operations.
 * Provides save and open file dialogs with JSON filtering support.
 */
class FileDialog {
public:
    virtual ~FileDialog() = default;
    
    /**
     * Opens a file save dialog and returns the selected file path.
     * @param defaultPath Optional default path to suggest
     * @return Selected file path, empty string if cancelled
     */
    virtual std::string saveFileDialog(const std::string& defaultPath = "") = 0;
    
    /**
     * Opens a file open dialog and returns the selected file path.
     * @param defaultPath Optional default path to suggest
     * @return Selected file path, empty string if cancelled
     */
    virtual std::string openFileDialog(const std::string& defaultPath = "") = 0;
    
    /**
     * Gets the JSON file filter pattern for platform-specific implementations.
     * @return JSON file filter string appropriate for current platform
     */
    virtual std::string getJsonFilter() const = 0;
    
    /**
     * Creates a platform-specific FileDialog instance.
     * Must be implemented in platform-specific code.
     */
    // Factory method to create platform-specific FileDialog instance
    // platformWindowHandle: Platform-specific window handle (GLFWwindow* for desktop)
    static std::unique_ptr<FileDialog> create(void* platformWindowHandle = nullptr);
};

/**
 * Fallback implementation that uses ImGui modal text inputs.
 * Used when native file dialogs are unavailable.
 */
class ImGuiFileDialog : public FileDialog {
public:
    std::string saveFileDialog(const std::string& defaultPath = "") override;
    std::string openFileDialog(const std::string& defaultPath = "") override;
    std::string getJsonFilter() const override { return "*.json"; }
    
    static std::unique_ptr<FileDialog> createFallback();
};

} // namespace app