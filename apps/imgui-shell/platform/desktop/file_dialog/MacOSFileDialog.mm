#include "MacOSFileDialog.h"
#include <string>

// Objective-C headers
#import <Cocoa/Cocoa.h>

namespace app {

struct MacOSFileDialog::Impl {
    NSAutoreleasePool* pool;
};

MacOSFileDialog::MacOSFileDialog() : m_impl(std::make_unique<Impl>()) {
    m_impl->pool = [[NSAutoreleasePool alloc] init];
}

MacOSFileDialog::~MacOSFileDialog() {
    [m_impl->pool release];
}

std::string MacOSFileDialog::getJsonFilter() const {
    return "json";
}

std::string MacOSFileDialog::saveFileDialog(const std::string& defaultPath) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        
        // Set JSON file filter
        [panel setAllowedFileTypes:@[@"json"]];
        [panel setAllowsOtherFileTypes:NO];
        
        // Set default directory if provided
        if (!defaultPath.empty()) {
            NSString* defaultDir = [NSString stringWithUTF8String:defaultPath.c_str()];
            [panel setDirectoryURL:[NSURL fileURLWithPath:defaultDir]];
        }
        
        // Run modal dialog
        NSModalResponse response = [panel runModal];
        
        if (response == NSModalResponseOK) {
            NSString* selectedPath = [[panel URL] path];
            return std::string([selectedPath UTF8String]);
        }
        
        return ""; // Cancelled
    }
}

std::string MacOSFileDialog::openFileDialog(const std::string& defaultPath) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        
        // Set JSON file filter
        [panel setAllowedFileTypes:@[@"json"]];
        [panel setAllowsOtherFileTypes:NO];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        
        // Set default directory if provided
        if (!defaultPath.empty()) {
            NSString* defaultDir = [NSString stringWithUTF8String:defaultPath.c_str()];
            [panel setDirectoryURL:[NSURL fileURLWithPath:defaultDir]];
        }
        
        // Run modal dialog
        NSModalResponse response = [panel runModal];
        
        if (response == NSModalResponseOK) {
            NSString* selectedPath = [[panel URL] path];
            return std::string([selectedPath UTF8String]);
        }
        
        return ""; // Cancelled
    }
}

std::unique_ptr<FileDialog> createMacOSFileDialog(void* platformWindowHandle) {
    return std::make_unique<MacOSFileDialog>();
}

} // namespace app