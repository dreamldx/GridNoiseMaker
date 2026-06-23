## 1. Create DialogManager files

- [x] 1.1 Create DialogManager.h with class declaration and singleton pattern
- [x] 1.2 Create DialogManager.cpp with implementation matching design decisions
- [x] 1.3 Add DialogManager to CMakeLists.txt for compilation

## 2. Refactor App.cpp to use DialogManager

- [x] 2.1 Replace global dialog variables with DialogManager::instance() calls
- [x] 2.2 Update dialog triggering logic to use DialogManager methods
- [x] 2.3 Update dialog rendering to query DialogManager for state
- [x] 2.4 Remove old global variable declarations from App.cpp

## 3. Create NodeTypeRegistry files

- [x] 3.1 Create NodeTypeRegistry.h with class declaration and singleton pattern
- [x] 3.2 Create NodeTypeRegistry.cpp with implementation copied from NodeGraphWidget.cpp
- [x] 3.3 Add NodeTypeRegistry to CMakeLists.txt for compilation

## 5. Verification and testing

- [x] 5.1 Compile project to verify no build errors
- [x] 5.2 Test with test_unknown_types.json to ensure dialog appears correctly
- [x] 5.3 Verify node loading functionality works unchanged
- [x] 5.4 Validate that new types can still be registered and validated