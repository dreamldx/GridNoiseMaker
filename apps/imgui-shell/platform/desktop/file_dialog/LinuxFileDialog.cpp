#include "LinuxFileDialog.h"
#include <dlfcn.h>
#include <string>
#include <cstring>

namespace app {

// GTK constants
constexpr int GTK_FILE_CHOOSER_ACTION_SAVE = 1;
constexpr int GTK_FILE_CHOOSER_ACTION_OPEN = 0;
constexpr int GTK_RESPONSE_OK = -5;
constexpr int GTK_RESPONSE_CANCEL = -6;

LinuxFileDialog::LinuxFileDialog() {
    m_gtkAvailable = loadGtkLibrary();
}

LinuxFileDialog::~LinuxFileDialog() {
    unloadGtkLibrary();
}

std::string LinuxFileDialog::getJsonFilter() const {
    return "*.json";
}

std::string LinuxFileDialog::saveFileDialog(const std::string& defaultPath) {
    if (m_gtkAvailable) {
        std::string result = showGtkDialog(true, defaultPath);
        if (!result.empty()) {
            return result;
        }
    }
    // Fallback will be handled by the caller (App.cpp) when empty string is returned
    return "";
}

std::string LinuxFileDialog::openFileDialog(const std::string& defaultPath) {
    if (m_gtkAvailable) {
        std::string result = showGtkDialog(false, defaultPath);
        if (!result.empty()) {
            return result;
        }
    }
    // Fallback will be handled by the caller (App.cpp) when empty string is returned
    return "";
}

bool LinuxFileDialog::loadGtkLibrary() {
    // Try to load GTK library
    m_gtkLib = dlopen("libgtk-3.so.0", RTLD_LAZY);
    if (!m_gtkLib) {
        m_gtkLib = dlopen("libgtk-3.so", RTLD_LAZY);
    }
    if (!m_gtkLib) {
        return false;
    }
    
    // Load required functions
    m_gtk_file_chooser_dialog_new = reinterpret_cast<GtkFileChooserDialogFunc>(
        dlsym(m_gtkLib, "gtk_file_chooser_dialog_new"));
    m_gtk_dialog_run = reinterpret_cast<GtkDialogRunFunc>(
        dlsym(m_gtkLib, "gtk_dialog_run"));
    m_gtk_file_chooser_get_filename = reinterpret_cast<GtkFileChooserGetFilenameFunc>(
        dlsym(m_gtkLib, "gtk_file_chooser_get_filename"));
    m_gtk_widget_destroy = reinterpret_cast<GtkWidgetDestroyFunc>(
        dlsym(m_gtkLib, "gtk_widget_destroy"));
    m_gtk_file_chooser_set_filter = reinterpret_cast<GtkFileChooserSetFilterFunc>(
        dlsym(m_gtkLib, "gtk_file_chooser_set_filter"));
    m_gtk_file_filter_new = reinterpret_cast<GtkFileFilterNewFunc>(
        dlsym(m_gtkLib, "gtk_file_filter_new"));
    m_gtk_file_filter_set_name = reinterpret_cast<GtkFileFilterSetNameFunc>(
        dlsym(m_gtkLib, "gtk_file_filter_set_name"));
    m_gtk_file_filter_add_pattern = reinterpret_cast<GtkFileFilterAddPatternFunc>(
        dlsym(m_gtkLib, "gtk_file_filter_add_pattern"));
    
    // Check if all required functions are loaded
    return m_gtk_file_chooser_dialog_new && m_gtk_dialog_run && 
           m_gtk_file_chooser_get_filename && m_gtk_widget_destroy &&
           m_gtk_file_chooser_set_filter && m_gtk_file_filter_new &&
           m_gtk_file_filter_set_name && m_gtk_file_filter_add_pattern;
}

void LinuxFileDialog::unloadGtkLibrary() {
    if (m_gtkLib) {
        dlclose(m_gtkLib);
        m_gtkLib = nullptr;
    }
}

std::string LinuxFileDialog::showGtkDialog(bool isSave, const std::string& defaultPath) {
    if (!m_gtkAvailable) return "";
    
    const char* title = isSave ? "Save File" : "Open File";
    int action = isSave ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN;
    
    // Create dialog
    void* dialog = m_gtk_file_chooser_dialog_new(title, nullptr, action, 
                                               "Cancel", GTK_RESPONSE_CANCEL,
                                               isSave ? "Save" : "Open", GTK_RESPONSE_OK,
                                               nullptr);
    
    if (!dialog) return "";
    
    // Create JSON filter
    void* filter = m_gtk_file_filter_new();
    m_gtk_file_filter_set_name(filter, "JSON Files");
    m_gtk_file_filter_add_pattern(filter, "*.json");
    m_gtk_file_chooser_set_filter(dialog, filter);
    
    // Set default filename if provided
    if (!defaultPath.empty() && isSave) {
        // Extract filename from path
        size_t slashPos = defaultPath.find_last_of("/\\");
        std::string filename = (slashPos != std::string::npos) ? 
                              defaultPath.substr(slashPos + 1) : defaultPath;
        // GTK function to set filename would be loaded here
        // For simplicity, we'll skip this for now
    }
    
    // Run dialog
    int response = m_gtk_dialog_run(dialog);
    
    std::string result;
    if (response == GTK_RESPONSE_OK) {
        char* filename = m_gtk_file_chooser_get_filename(dialog);
        if (filename) {
            result = filename;
            // GTK function to free filename would be loaded here
            // We'll use standard C free for now
            free(filename);
        }
    }
    
    m_gtk_widget_destroy(dialog);
    return result;
}

std::unique_ptr<FileDialog> createLinuxFileDialog(void* platformWindowHandle) {
    return std::make_unique<LinuxFileDialog>();
}

} // namespace app