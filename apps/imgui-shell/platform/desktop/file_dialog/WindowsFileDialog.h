#pragma once

#include "../../../app/FileDialog.h"
#include <memory>

namespace app {

/**
 * Windows-specific file dialog implementation using Common Item Dialog API.
 */
class WindowsFileDialog : public FileDialog {
public:
    WindowsFileDialog(void* platformWindowHandle = nullptr);
    ~WindowsFileDialog() override;
    
    std::string saveFileDialog(const std::string& defaultPath = "") override;
    std::string openFileDialog(const std::string& defaultPath = "") override;
    std::string getJsonFilter() const override;
    
private:
    std::string showCommonItemDialog(bool isSave, const std::string& defaultPath);
    std::string configureAndShowDialog(void* pDialog, const std::string& defaultPath);
    
    void* m_platformWindowHandle = nullptr;
    bool m_comInitialized = false;
};

std::unique_ptr<FileDialog> createWindowsFileDialog(void* platformWindowHandle = nullptr);

} // namespace app