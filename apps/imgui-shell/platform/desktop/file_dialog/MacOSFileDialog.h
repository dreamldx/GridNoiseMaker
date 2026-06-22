#pragma once

#include "../../../app/FileDialog.h"
#include <memory>

namespace app {

/**
 * macOS-specific file dialog implementation using Cocoa APIs.
 * Implemented in Objective-C++ (.mm) file.
 */
class MacOSFileDialog : public FileDialog {
public:
    MacOSFileDialog();
    ~MacOSFileDialog() override;
    
    std::string saveFileDialog(const std::string& defaultPath = "") override;
    std::string openFileDialog(const std::string& defaultPath = "") override;
    std::string getJsonFilter() const override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

std::unique_ptr<FileDialog> createMacOSFileDialog(void* platformWindowHandle = nullptr);

} // namespace app