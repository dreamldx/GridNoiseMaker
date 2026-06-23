## Context

The node type validation functionality was recently implemented in the `register-node-type-validation` change. Currently, dialog management is embedded in `App.cpp` with global state variables (`g_showUnknownNodeTypesDialog`, `g_unknownNodeTypes`, `g_unknownNodeCount`), and the `NodeTypeRegistry` is implemented as a singleton within `NodeGraphWidget.cpp`. This creates tight coupling where:
- Dialog state is managed globally in App.cpp
- NodeTypeRegistry logic is embedded in NodeGraphWidget.cpp
- Both components cannot be tested independently
- Code organization lacks modularity

## Goals / Non-Goals

**Goals:**
1. Separate dialog management into dedicated DialogManager class for cleaner UI state handling
2. Extract NodeTypeRegistry from NodeGraphWidget.cpp into standalone files for modularity
3. Maintain backward compatibility - existing functionality should work unchanged
4. Enable independent testing of both components
5. Improve code organization and reduce coupling

**Non-Goals:**
1. Change validation behavior or dialog UI presentation
2. Add new features beyond separation
3. Modify the singleton pattern of NodeTypeRegistry
4. Change file loading or JSON serialization logic
5. Alter existing type registration API

## Decisions

### Decision 1: DialogManager as a singleton class
**Rationale:** Dialog state needs to be accessible across the application (App.cpp needs to trigger dialog, NodeGraphWidget.cpp needs to report skipped types). Singleton pattern maintains existing global accessibility while encapsulating state.
**Alternative considered:** Instance-based approach with App holding DialogManager instance. Rejected because it requires passing instance references through multiple layers.

### Decision 2: Keep NodeTypeRegistry singleton but separate files
**Rationale:** The registry needs to be process-wide accessible (any widget can validate types). Moving to separate files maintains the singleton pattern but improves code organization.
**Alternative considered:** Convert to instance-based registry per widget. Rejected because multiple widgets need shared type registry.

### Decision 3: DialogManager interface mirrors existing global variable functionality
**Rationale:** Minimal API change reduces refactoring complexity. DialogManager will have methods like `showUnknownNodeTypesDialog()`, `setUnknownNodeTypes()`, `shouldShowDialog()` that mirror current global variable access.
**Alternative considered:** More sophisticated dialog management system with event queue. Rejected for scope - only need unknown node types dialog management.

### Decision 4: Direct file references instead of dependency injection
**Rationale:** Both DialogManager and NodeTypeRegistry will be referenced directly via their singleton instances (`DialogManager::instance()`, `NodeTypeRegistry::instance()`). This maintains simple integration.
**Alternative considered:** Dependency injection through constructor parameters. Rejected due to increased refactoring complexity and no immediate testing benefits.

## Risks / Trade-offs

### Risk: Breaking existing functionality due to refactoring errors
**Mitigation:** Verify refactored code through:
1. Compilation check after each file change
2. Testing with `test_unknown_types.json` to ensure dialog still appears correctly
3. Manual verification that node loading works as before

### Risk: Increased file count without clear benefit
**Mitigation:** Ensure new files are logically organized and clearly named:
- `DialogManager.h/.cpp` in same directory as App.cpp
- `NodeTypeRegistry.h/.cpp` in same directory as NodeGraphWidget.cpp

### Trade-off: Still global state but better encapsulated
**Benefit:** State is encapsulated in classes with clear APIs
**Cost:** Still uses global singleton pattern, not pure instance-based approach

### Trade-off: No immediate testing infrastructure
**Benefit:** Components are separable for future testing
**Cost:** No test harness exists yet to validate separation