#include "WindowsFileDialog.h"
#include <windows.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <filesystem>

// Include GLFW headers for window handle conversion
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace app {

WindowsFileDialog::WindowsFileDialog(void* platformWindowHandle) 
    : m_platformWindowHandle(platformWindowHandle) {
    // Initialize COM for Common Item Dialog API
    // Use COINIT_APARTMENTTHREADED for STA (Single-Threaded Apartment)
    // Common Item Dialogs require STA threading model
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    if (SUCCEEDED(hr)) {
        m_comInitialized = true;
        // Log to file for debugging
        FILE* logFile = fopen("file_dialog_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[WindowsFileDialog] COM initialization succeeded\n");
            fclose(logFile);
        }
        OutputDebugStringA("[WindowsFileDialog] COM initialization succeeded\n");
    } else if (hr == RPC_E_CHANGED_MODE) {
        // Log to file for debugging
        FILE* logFile = fopen("file_dialog_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[WindowsFileDialog] COM already initialized with different mode, retrying\n");
            fclose(logFile);
        }
        OutputDebugStringA("[WindowsFileDialog] COM already initialized with different mode, retrying\n");
        // COM was already initialized with a different threading model
        // Try to uninitialize and reinitialize with the correct model
        CoUninitialize();
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr)) {
            m_comInitialized = true;
        }
    }
}

WindowsFileDialog::~WindowsFileDialog() {
    if (m_comInitialized) {
        CoUninitialize();
    }
}

std::string WindowsFileDialog::getJsonFilter() const {
    return "JSON Files\0*.json\0All Files\0*.*\0";
}

std::string WindowsFileDialog::saveFileDialog(const std::string& defaultPath) {
    // Log to file for debugging
    FILE* logFile = fopen("file_dialog_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[WindowsFileDialog] saveFileDialog called\n");
        fclose(logFile);
    }
    OutputDebugStringA("[WindowsFileDialog] saveFileDialog called\n");
    return showCommonItemDialog(true, defaultPath);
}

std::string WindowsFileDialog::openFileDialog(const std::string& defaultPath) {
    return showCommonItemDialog(false, defaultPath);
}

std::string WindowsFileDialog::showCommonItemDialog(bool isSave, const std::string& defaultPath) {
    if (!m_comInitialized) {
        // Debug: COM initialization failed
        OutputDebugStringA("[WindowsFileDialog] COM initialization failed\n");
        return ""; // COM initialization failed
    }
    
    HRESULT hr = S_OK;
    std::string result;
    
    if (isSave) {
        // Create File Save Dialog
        IFileSaveDialog* pFileSave = nullptr;
        hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, 
                              IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
        
        if (SUCCEEDED(hr) && pFileSave) {
            // Debug: Dialog created successfully
            OutputDebugStringA("[WindowsFileDialog] IFileSaveDialog created successfully\n");
            result = configureAndShowDialog(pFileSave, defaultPath);
            pFileSave->Release();
        } else {
            // Debug: Dialog creation failed
            char debugMsg[256];
            sprintf_s(debugMsg, sizeof(debugMsg), "[WindowsFileDialog] IFileSaveDialog creation failed, hr=0x%08X\n", hr);
            OutputDebugStringA(debugMsg);
            
            FILE* logFile = fopen("file_dialog_debug.log", "a");
            if (logFile) {
                fprintf(logFile, debugMsg);
                fclose(logFile);
            }
        }
    } else {
        // Create File Open Dialog
        IFileOpenDialog* pFileOpen = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, 
                              IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        
        if (SUCCEEDED(hr) && pFileOpen) {
            result = configureAndShowDialog(pFileOpen, defaultPath);
            pFileOpen->Release();
        }
    }
    
    return result;
}

std::string WindowsFileDialog::configureAndShowDialog(void* pDialog, const std::string& defaultPath) {
    if (!pDialog) {
        return "";
    }
    
    IFileDialog* pFileDialog = reinterpret_cast<IFileDialog*>(pDialog);
    
    // Set file type filters
    COMDLG_FILTERSPEC filterSpec[] = {
        { L"JSON Files", L"*.json" },
        { L"All Files", L"*.*" }
    };
    
    pFileDialog->SetFileTypes(2, filterSpec);
    pFileDialog->SetFileTypeIndex(1); // Default to JSON filter
    
    // Set default extension
    pFileDialog->SetDefaultExtension(L"json");
    
    // Set default folder if provided
    if (!defaultPath.empty()) {
        // Extract directory from path
        std::filesystem::path fsPath(defaultPath);
        if (fsPath.has_parent_path()) {
            std::wstring dirPath = fsPath.parent_path().wstring();
            
            IShellItem* pFolder = nullptr;
            HRESULT hr = SHCreateItemFromParsingName(dirPath.c_str(), nullptr, 
                                                    IID_IShellItem, reinterpret_cast<void**>(&pFolder));
            if (SUCCEEDED(hr) && pFolder) {
                pFileDialog->SetFolder(pFolder);
                pFolder->Release();
            }
            
            // Set default file name
            if (fsPath.has_filename()) {
                std::wstring fileName = fsPath.filename().wstring();
                pFileDialog->SetFileName(fileName.c_str());
            }
        }
    }
    
    // Get window handle from GLFW if available, otherwise use active window
    HWND hwndOwner = nullptr;
    
    if (m_platformWindowHandle) {
        // m_platformWindowHandle is GLFWwindow* on desktop platforms
        GLFWwindow* glfwWindow = reinterpret_cast<GLFWwindow*>(m_platformWindowHandle);
        hwndOwner = glfwGetWin32Window(glfwWindow);
        
        // Log window handle for debugging
        FILE* logFile = fopen("file_dialog_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[WindowsFileDialog] m_platformWindowHandle: %p, glfwWindow: %p, hwndOwner: %p\n", 
                    m_platformWindowHandle, glfwWindow, hwndOwner);
            fclose(logFile);
        }
    }
    
    if (!hwndOwner) {
        // Fallback to active window if GLFW handle not available
        hwndOwner = GetActiveWindow();
        if (!hwndOwner) {
            hwndOwner = GetForegroundWindow();
        }
        
        // Log fallback window handle
        FILE* logFile = fopen("file_dialog_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[WindowsFileDialog] Using fallback hwndOwner: %p\n", hwndOwner);
            fclose(logFile);
        }
    }
    
    // Show the dialog
    HRESULT hr = pFileDialog->Show(hwndOwner);
    if (hr == S_OK) {
        // User clicked OK
        IShellItem* pItem = nullptr;
        hr = pFileDialog->GetResult(&pItem);
        if (SUCCEEDED(hr) && pItem) {
            // Get the file path
            PWSTR pszFilePath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
            if (SUCCEEDED(hr) && pszFilePath) {
                // Convert from UTF-16 to UTF-8
                int utf8Size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, nullptr, 0, nullptr, nullptr);
                if (utf8Size > 0) {
                    std::vector<char> buffer(utf8Size);
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer.data(), utf8Size, nullptr, nullptr);
                    CoTaskMemFree(pszFilePath);
                    pItem->Release();
                    return std::string(buffer.data());
                }
                CoTaskMemFree(pszFilePath);
            }
            pItem->Release();
        }
    }
    // hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) means user clicked Cancel
    // Other errors mean dialog failed
    
    return ""; // Cancelled or error
}

std::unique_ptr<FileDialog> createWindowsFileDialog(void* platformWindowHandle) {
    return std::make_unique<WindowsFileDialog>(platformWindowHandle);
}

} // namespace app