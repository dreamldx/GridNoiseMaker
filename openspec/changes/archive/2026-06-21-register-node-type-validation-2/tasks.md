## 1. Update NodeGraphWidget JSON loading with type validation

- [x] 1.1 Modify `NodeGraphWidget::fromJson()` method in `apps/imgui-shell/app/NodeGraphWidget.cpp`
- [x] 1.2 Add type validation loop: check each node's type against NodeTypeRegistry
- [x] 1.3 Collect statistics: track skipped node count and unique type names
- [x] 1.4 Skip nodes with unknown types (don't add to m_nodes)
- [x] 1.5 Continue loading valid nodes with registered types
- [x] 1.6 Return success/failure status indicating if any nodes were skipped

## 2. Add error dialog handling in App.cpp

- [x] 2.1 Add `showUnknownNodeTypesDialog()` method in `apps/imgui-shell/app/App.cpp`
. [x] 2.2 Implement ImGui modal popup with title "Unknown Node Types"
- [x] 2.3 Format error message listing skipped types and count
- [x] 2.4 Include user-friendly explanation: "These nodes were skipped because their types are not registered"
- [x] 2.5 Add "OK" button to close dialog (modal requires acknowledgement)
- [x] 2.6 Connect dialog to NodeGraphWidget loading results

## 3. Integrate validation with existing type registration

- [x] 3.1 Verify built-in types (input, processor, output) are registered in NodeGraphWidget constructor
- [x] 3.2 Ensure NodeTypeRegistry singleton is initialized before any loading operations
- [x] 3.3 Test type validation with existing registered types (should all pass)

## 4. Testing and verification

- [x] 4.1 Create test JSON file `assets/test_unknown_types.json` with mixed valid/invalid node types
- [x] 4.2 Test loading: verify valid nodes appear correctly
- [x] 4.3 Test loading: verify invalid nodes are skipped (not visible)
- [x] 4.4 Test error dialog: appears with correct skipped type list and count
- [x] 4.5 Test backward compatibility: existing valid JSON files load without errors
- [x] 4.6 Test edge cases: empty JSON, missing type field, malformed JSON
- [x] 4.7 Verify performance: type validation doesn't impact load times noticeably

## 5. Documentation and cleanup

- [x] 5.1 Add comments to `fromJson()` explaining type validation logic
- [x] 5.2 Update any relevant documentation about node type system
- [x] 5.3 Remove any debug logging after testing complete
- [x] 5.4 Verify all tasks completed and functionality works as specified in specs