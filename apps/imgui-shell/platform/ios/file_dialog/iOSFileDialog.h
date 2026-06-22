#pragma once

#include "../../../app/FileDialog.h"
#include <memory>

namespace app {

/**
 * iOS-specific file dialog implementation using UIKit document picker.
 * Stub implementation - requires iOS UIKit integration.
 */
class iOSFileDialog : public FileDialog {
public:
    iOSFileDialog();
    ~iOSFileDialog() override;
    
    std::string saveFileDialog(const std::string& defaultPath = "") override;
    std::string openFileDialog(const std::string& defaultPath = "") override;
    std::string getJsonFilter() const override;
    
private:
    // iOS implementation requires UIKit view controller integration
    // This is a stub that will always use fallback
};

std::unique_ptr<FileDialog> createiOSFileDialog(void* platformWindowHandle = nullptr);

} // namespace app