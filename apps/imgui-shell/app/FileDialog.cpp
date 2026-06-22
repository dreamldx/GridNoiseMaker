#include "FileDialog.h"
#include "imgui.h"
#include <cstring>

namespace app {

// Forward declare platform-specific factory implementations
#if defined(_WIN32)
std::unique_ptr<FileDialog> createWindowsFileDialog(void* platformWindowHandle = nullptr);
#elif defined(__APPLE__) && defined(IMGUI_SHELL_PLATFORM_DESKTOP)
std::unique_ptr<FileDialog> createMacOSFileDialog(void* platformWindowHandle = nullptr);
#elif defined(__linux__) && defined(IMGUI_SHELL_PLATFORM_DESKTOP)
std::unique_ptr<FileDialog> createLinuxFileDialog(void* platformWindowHandle = nullptr);
#elif defined(IMGUI_SHELL_PLATFORM_IOS)
std::unique_ptr<FileDialog> createiOSFileDialog(void* platformWindowHandle = nullptr);
#endif

std::unique_ptr<FileDialog> FileDialog::create(void* platformWindowHandle) {
#if defined(_WIN32)
    return createWindowsFileDialog(platformWindowHandle);
#elif defined(__APPLE__) && defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    return createMacOSFileDialog(platformWindowHandle);
#elif defined(__linux__) && defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    return createLinuxFileDialog(platformWindowHandle);
#elif defined(IMGUI_SHELL_PLATFORM_IOS)
    return createiOSFileDialog(platformWindowHandle);
#else
    // Unknown platform, use fallback
    return ImGuiFileDialog::createFallback();
#endif
}

// ImGuiFileDialog implementation
static bool s_showImGuiSaveDialog = false;
static bool s_showImGuiOpenDialog = false;
static char s_filePathBuffer[256] = "";
static std::string s_lastFilePath = "";
static bool s_dialogCancelled = false;
static std::string s_dialogDefaultPath = "";

std::string ImGuiFileDialog::saveFileDialog(const std::string& defaultPath) {
    s_showImGuiSaveDialog = true;
    s_filePathBuffer[0] = '\0';
    s_lastFilePath = "";
    s_dialogCancelled = false;
    s_dialogDefaultPath = defaultPath;
    
    if (!defaultPath.empty()) {
        size_t copySize = std::min(defaultPath.size(), sizeof(s_filePathBuffer) - 1);
        std::memcpy(s_filePathBuffer, defaultPath.c_str(), copySize);
        s_filePathBuffer[copySize] = '\0';
    }
    
    // This is a blocking call - wait for dialog to complete
    // Implementation will be called from App.cpp frame loop
    while (s_showImGuiSaveDialog) {
        // Polling happens externally
    }
    
    return s_dialogCancelled ? "" : s_lastFilePath;
}

std::string ImGuiFileDialog::openFileDialog(const std::string& defaultPath) {
    s_showImGuiOpenDialog = true;
    s_filePathBuffer[0] = '\0';
    s_lastFilePath = "";
    s_dialogCancelled = false;
    s_dialogDefaultPath = defaultPath;
    
    if (!defaultPath.empty()) {
        size_t copySize = std::min(defaultPath.size(), sizeof(s_filePathBuffer) - 1);
        std::memcpy(s_filePathBuffer, defaultPath.c_str(), copySize);
        s_filePathBuffer[copySize] = '\0';
    }
    
    while (s_showImGuiOpenDialog) {
        // Polling happens externally
    }
    
    return s_dialogCancelled ? "" : s_lastFilePath;
}

std::unique_ptr<FileDialog> ImGuiFileDialog::createFallback() {
    return std::make_unique<ImGuiFileDialog>();
}

// Helper functions to integrate with ImGui modal rendering
bool showImGuiFileSaveDialog(std::string& outPath) {
    if (!s_showImGuiSaveDialog) return false;
    
    if (ImGui::BeginPopupModal("Save File", nullptr, 
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        
        ImGui::Text("Save node graph to JSON file:");
        ImGui::Spacing();
        
        ImGui::InputText("File path", s_filePathBuffer, sizeof(s_filePathBuffer));
        ImGui::Spacing();
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            std::string filePath = s_filePathBuffer;
            if (filePath.empty()) {
                // Show error inline (handled by App.cpp)
            } else {
                s_lastFilePath = filePath;
                s_showImGuiSaveDialog = false;
                s_dialogCancelled = false;
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_showImGuiSaveDialog = false;
            s_dialogCancelled = true;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    } else {
        ImGui::OpenPopup("Save File");
    }
    
    return s_showImGuiSaveDialog;
}

bool showImGuiFileOpenDialog(std::string& outPath) {
    if (!s_showImGuiOpenDialog) return false;
    
    if (ImGui::BeginPopupModal("Open File", nullptr, 
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        
        ImGui::Text("Open node graph JSON file:");
        ImGui::Spacing();
        
        ImGui::InputText("File path", s_filePathBuffer, sizeof(s_filePathBuffer));
        ImGui::Spacing();
        
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            std::string filePath = s_filePathBuffer;
            if (filePath.empty()) {
                // Show error inline
            } else {
                s_lastFilePath = filePath;
                s_showImGuiOpenDialog = false;
                s_dialogCancelled = false;
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_showImGuiOpenDialog = false;
            s_dialogCancelled = true;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    } else {
        ImGui::OpenPopup("Open File");
    }
    
    return s_showImGuiOpenDialog;
}

} // namespace app