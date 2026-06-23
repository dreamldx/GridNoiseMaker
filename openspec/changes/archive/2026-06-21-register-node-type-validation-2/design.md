## Context

The node graph system currently has a NodeTypeRegistry singleton that stores node type templates (name, default color, default properties). When loading nodes from JSON files, the system doesn't validate type names - it silently creates "default" nodes for unknown types, losing type-specific properties and visual styling. This creates a poor user experience where saved projects appear differently when loaded, and users have no feedback about why certain nodes don't appear correctly.

Current implementation in `NodeGraphWidget.cpp`:
-e `fromJson()` method creates nodes without type validation
- NodeTypeRegistry is populated in constructor but not verified during loading
- No error feedback mechanism for unknown types

## Goals / Non-Goals

**Goals:**
- Ensure built-in node types (input, processor, output) are registered before any file operations
- Validate node type names during JSON loading against the registry
- Skip nodes with unknown types and collect statistics for user feedback
- Display clear error dialog listing skipped node types and count
- Maintain backward compatibility: continue loading valid nodes, don't reject entire files
- Improve user experience with explicit feedback instead of silent failures

**Non-Goals:**
- Automatic type registration from JSON files
- Type inference or fallback behavior
- Breaking changes to file format or API
- Complex type inheritance or polymorphism
- Real-time type validation during editing (only during loading)

## Decisions

### 1. Type validation during JSON loading
**Decision**: Add type validation in `NodeGraphWidget::fromJson()` method
**Rationale**: Validation must happen at load time when type information is available and before nodes are created. This matches the existing JSON parsing flow and ensures consistency.
**Alternative considered**: Validate in `NodeTypeRegistry::createNode()` - rejected because it would require changes to the registry interface and wouldn't provide aggregated statistics for user feedback.

### 2. Skip unknown nodes with statistics collection
**Decision**: Skip nodes with unknown types but continue loading valid nodes, collecting skipped type names and count
**Rationale**: Maintains backward compatibility and preserves user's valid data while providing feedback about issues.
**Alternative considered**: Reject entire file - rejected as too aggressive and user-unfriendly for partial failures.

### 3. Error dialog with aggregated information
**Decision**: Show single dialog after loading completes, listing all skipped node types and total count
**Rationale**: Provides clear, actionable feedback without interrupting the user multiple times.
**Alternative considered**: Individual warnings per node - rejected as too noisy and confusing.

### 4. Dialog integration in App.cpp
**Decision**: Handle error dialog display in App.cpp using ImGui modal popups
**Rationale**: App.cpp already handles other UI dialogs and has access to ImGui context. Keeps NodeGraphWidget focused on graph operations.
**Alternative considered**: Dialog in NodeGraphWidget - rejected to maintain separation of concerns.

### 5. No changes to NodeTypeRegistry interface
**Decision**: Keep existing NodeTypeRegistry interface unchanged
**Rationale**: The registry already has `getType()` method for lookup. Additional statistics collection can be handled at the validation layer without registry changes.
**Alternative considered**: Add validation methods to registry - rejected as unnecessary complexity.

## Risks / Trade-offs

**Risk**: Users may not notice the error dialog if it appears briefly
**Mitigation**: Use modal dialog that requires user acknowledgement (ImGui::OpenPopup + ImGui::BeginPopupModal)

**Risk**: Skipping nodes could disrupt node connections or dependencies in future graph systems
**Mitigation**: Document this limitation. Future graph connection system will need to handle missing nodes.

**Risk**: Performance impact from type validation on large graphs
**Mitigation**: Type lookup is O(1) hash map operation, negligible compared to JSON parsing.

**Trade-off**: Skipping vs. default nodes
- **Skipping**: Clear feedback but nodes disappear
- **Default nodes**: Nodes appear but with wrong properties
**Choice**: Skipping with feedback is better UX - explicit problem statement over hidden corruption.

**Trade-off**: Immediate vs. deferred error reporting
- **Immediate**: Error per unknown type as discovered
1- **Deferred**: Single aggregated message after loading
**Choice**: Deferred - less disruptive, provides complete picture of issues.

## Migration Plan

### Implementation Steps:
1. Update `NodeGraphWidget::fromJson()` to validate types and collect statistics
2. Add error dialog handling in `App.cpp` 
3. Ensure type registration happens in constructor (already implemented)
4. Test with JSON files containing unknown types
5. Verify backward compatibility with existing files

### Rollback Strategy:
If issues arise, revert changes to `fromJson()` method - type validation is additive, not breaking.

### Testing Strategy:
1. Create test JSON file with mixed valid/invalid node types
2. Verify valid nodes load correctly
3. Verify invalid nodes are skipped
4. Verify error dialog appears with correct information
5. Verify no regression with existing valid files

## Open Questions

None - implementation approach is straightforward based on existing architecture.