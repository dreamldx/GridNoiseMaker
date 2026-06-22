#include "iOSFileDialog.h"
#include <string>

namespace app {

iOSFileDialog::iOSFileDialog() = default;
iOSFileDialog::~iOSFileDialog() = default;

std::string iOSFileDialog::getJsonFilter() const {
    return "public.json"; // UTI for JSON files on iOS
}

std::string iOSFileDialog::saveFileDialog(const std::string& defaultPath) {
    // iOS implementation requires UIDocumentPickerViewController integration
    // with UIKit view controller hierarchy
    // For now, return empty string to trigger fallback
    return "";
}

std::string iOSFileDialog::openFileDialog(const std::string& defaultPath) {
    // iOS implementation requires UIDocumentPickerViewController integration
    // with UIKit view controller hierarchy
    // For now, return empty string to trigger fallback
    return "";
}

std::unique_ptr<FileDialog> createiOSFileDialog(void* platformWindowHandle) {
    return std::make_unique<iOSFileDialog>();
}

} // namespace app