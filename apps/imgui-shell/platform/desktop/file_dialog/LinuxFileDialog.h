#pragma once

#include "../../../app/FileDialog.h"
#include <memory>
#include <string>

namespace app {

/**
 * Linux-specific file dialog implementation using GTK APIs with dynamic loading.
 * Dynamically loads GTK at runtime to avoid forced dependency.
 */
class LinuxFileDialog : public FileDialog {
public:
    LinuxFileDialog();
    ~LinuxFileDialog() override;
    
    std::string saveFileDialog(const std::string& defaultPath = "") override;
    std::string openFileDialog(const std::string& defaultPath = "") override;
    std::string getJsonFilter() const override;
    
    bool isGtkAvailable() const { return m_gtkAvailable; }
    
private:
    bool m_gtkAvailable = false;
    void* m_gtkLib = nullptr;
    
    // Dynamically loaded function pointers
    using GtkFileChooserDialogFunc = void*(*)(const char*, void*, int, const char*, ...);
    using GtkDialogRunFunc = int(*)(void*);
    using GtkFileChooserGetFilenameFunc = char*(*)(void*);
    using GtkWidgetDestroyFunc = void(*)(void*);
    using GtkFileChooserSetFilterFunc = void(*)(void*, void*);
    using GtkFileFilterNewFunc = void*(*)();
    using GtkFileFilterSetNameFunc = void(*)(void*, const char*);
    using GtkFileFilterAddPatternFunc = void(*)(void*, const char*);
    
    GtkFileChooserDialogFunc m_gtk_file_chooser_dialog_new = nullptr;
    GtkDialogRunFunc m_gtk_dialog_run = nullptr;
    GtkFileChooserGetFilenameFunc m_gtk_file_chooser_get_filename = nullptr;
    GtkWidgetDestroyFunc m_gtk_widget_destroy = nullptr;
    GtkFileChooserSetFilterFunc m_gtk_file_chooser_set_filter = nullptr;
    GtkFileFilterNewFunc m_gtk_file_filter_new = nullptr;
    GtkFileFilterSetNameFunc m_gtk_file_filter_set_name = nullptr;
    GtkFileFilterAddPatternFunc m_gtk_file_filter_add_pattern = nullptr;
    
    std::string showGtkDialog(bool isSave, const std::string& defaultPath);
    bool loadGtkLibrary();
    void unloadGtkLibrary();
};

std::unique_ptr<FileDialog> createLinuxFileDialog(void* platformWindowHandle = nullptr);

} // namespace app