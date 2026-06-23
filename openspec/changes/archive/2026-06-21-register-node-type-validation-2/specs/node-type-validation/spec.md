# Node Type Validation

## Purpose
Validate node types during JSON loading and report unknown types to users with clear error feedback.

## ADDED Requirements

### Requirement: Node type validation during loading
The system SHALL validate node type names against the type registry when loading from JSON files and handle unknown types with explicit user feedback instead of silent fallback to default nodes.

#### Scenario: Validate node types during JSON loading
- **WHEN** nodes are loaded from a JSON file
- **THEN** the system SHALL check each node's type name against the NodeTypeRegistry
- **AND** SHALL skip nodes with unknown type names
- **AND** SHALL collect statistics on skipped nodes (count and type names)

#### Scenario: Report skipped nodes to user
- **WHEN** nodes are skipped due to unknown type names during JSON loading
- **THEN** the system SHALL display a dialog message to the user
- **AND** SHALL list all skipped node types by name
- **AND** SHALL show the count of skipped nodes
- **AND** SHALL explain that these nodes were not loaded because their types are not registered

#### Scenario: Continue loading with valid nodes
- **WHEN** some nodes have unknown type names in a JSON file
- **THEN** the system SHALL continue loading all valid nodes with registered types
- **AND** SHALL maintain backward compatibility by not rejecting entire files
- **AND** SHALL preserve the user's valid data while providing feedback about missing types

#### Scenario: Clear error message formatting
- **WHEN** the unknown type dialog is displayed
- **THEN** the message SHALL use clear, user-friendly language
- **AND** SHALL format type names in a readable list
- **AND** SHALL provide actionable information (register missing types or ignore)
-

### Requirement: Node type registration at startup
The system SHALL ensure all built-in node types are registered before any node creation or JSON loading operations.

#### Scenario: Built-in types registered in constructor
- **WHEN** the NodeGraphWidget is constructed
- **THEN** it SHALL register all built-in node types (input, processor, output)
-l **AND** SHALL initialize the NodeTypeRegistry singleton before any node operations
- **AND** SHALL maintain type registration state for the application lifetime

#### Scenario: Type registry available at application start
- **WHEN** the application starts
int
- **THEN** the NodeTypeRegistry singleton SHALL be initialized
- **AND** SHALL contain all built-in node types before any file operations
- **AND** SHALL be ready for type validation during JSON loading