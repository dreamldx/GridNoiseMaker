## 1. CMake Analysis and Diagnosis

- [x] 1.1 Analyze current POST_BUILD custom command in `apps/imgui-shell/platform/desktop/CMakeLists.txt`
- [x] 1.2 Test build behavior to reproduce the copy failure issue
. [ ] 1.3 Identify why CMake doesn't execute copy when target exists
- [x] 1.4 Document Windows-specific path handling issues with `$<TARGET_FILE:...>`

## 2. Custom Target Implementation

- [x] 2.1 Replace `add_custom_command` with `add_custom_target` for copy operations
- [x] 2.2 Create `copy_to_bin` target with explicit dependency on executable
- [x] 2.3 Add `add_dependencies` to ensure copy target runs after build
- [x] 2.4 Test that custom target always executes when invoked

## 3. Platform-Specific Copy Commands

- [x] 3.1 Implement Windows path handling with PowerShell `Copy-Item`
- [x] 3.2 Add conditional `if(WIN32)` for platform-specific commands
- [x] 3.3 Keep Unix fallback with `cmake -E copy` for macOS/Linux
- [x] 3.4 Test copy operations on Windows with proper path escaping

## 4. Copy Operation Implementation

- [x] 4.1 Implement simple copy command without timestamp checking or logging
- [x] 4.2 Test that copy always executes when target runs
- [x] 4.3 Verify files are correctly copied every time

## 5. Asset Copying Maintenance

- [x] 5.1 Verify existing asset copying still works
- [x] 5.2 Ensure asset directory structure is preserved
- [x] 5.3 Test that `bin/assets/` folder contains all required files

## 6. Testing and Validation

- [ ] 6.1 Test clean build (no `bin/` folder exists)
- [ ] 6.2 Test incremental build (executable in `bin/` is older)
- [ ] 6.3 Test rebuild with no changes (should still copy)
- [ ] 6.4 Verify executable timestamps match between build and bin folders
- [ ] 6.5 Test cross-platform compatibility (Unix commands still work)