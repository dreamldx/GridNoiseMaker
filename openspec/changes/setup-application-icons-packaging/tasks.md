## 1. Windows Resource Compilation

- [x] 1.1 Create Windows resource (.rc) file referencing app_icon.ico
- [x] 1.2 Update CMakeLists.txt to compile Windows resources using windres
- [x] 1.3 Test Windows executable contains embedded icon resource
- [x] 1.4 Verify icon appears in Windows Explorer and taskbar

## 2. GLFW Icon Setting Implementation

- [ ] 2.1 Extract icon pixels from Windows resources for GLFW
- [ ] 2.2 Implement glfwSetWindowIcon() for Windows using embedded resources
- [ ] 2.3 Remove Windows LoadImage() file-based icon loading
- [ ] 2.4 Test GLFW icon setting works on Windows

## 3. macOS Icon Support

- [ ] 3.1 Create ICNS file from source icon (iconutil or script)
- [ ] 3.2 Add macOS app bundle resource configuration to CMake
- [ ] 3.3 Implement macOS icon loading for GLFW
- [ ] 3.4 Test icon appears in macOS dock and title bar

## 4. Linux Embedded Icon Support

- [ ] 4.1 Create build script to convert PNG to C array
- [ ] 4.2 Implement embedded PNG icon loading for Linux
- [ ] 4.3 Test GLFW icon setting works on Linux
- [ ] 4.4 Verify icon appears in Linux window manager

## 5. Cross-Platform Icon Abstraction

- [ ] 5.1 Create platform-agnostic icon loading interface
- [ ] 5.2 Implement Windows resource icon loader
- [ ] 5.3 Implement macOS bundle icon loader
- [ ] 5.4 Implement Linux embedded icon loader
- [ ] 5.5 Add fallback/default icon for missing platform implementations

## 6. Build System Integration

- [ ] 6.1 Add icon validation step to CMake build
- [ ] 6.2 Create icon file dependency tracking in build system
- [ ] 6.3 Add build-time icon format checking
- [ ] 6.4 Document icon source file requirements and formats

## 7. Testing and Validation

- [ ] 7.1 Test icon display in all packaging scenarios (installer, portable, etc.)
- [ ] 7.2 Verify icons work from different execution locations
- [ ] 7.3 Test high-DPI/retina icon scaling
- [ ] 7.4 Validate icon appearance consistency across platforms

## 8. Cleanup and Documentation

- [ ] 8.1 Remove old assets/app_icon.ico file loading code
- [ ] 8.2 Update developer documentation for icon workflow
- [ ] 8.3 Add icon update instructions to README
- [ ] 8.4 Create icon contribution guidelines