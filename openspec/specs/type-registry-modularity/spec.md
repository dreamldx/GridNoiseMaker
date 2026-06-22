## Purpose
TODO: Modular node type registry implementation

## ADDED Requirements

### Requirement: Separated NodeTypeRegistry implementation
The NodeTypeRegistry SHALL be implemented in dedicated files separate from NodeGraphWidget.cpp.

#### Scenario: Singleton registry accessibility
- **WHEN** any component needs to validate node types
- **THEN** it can access NodeTypeRegistry via `NodeTypeRegistry::instance()` method

#### Scenario: Type registration functionality
- **WHEN** NodeTypeRegistry is separated from NodeGraphWidget.cpp
- **THEN** all existing registration methods (`registerType`, `getType`, `getTypeNames`, `createNode`) remain available and unchanged

#### Scenario: Type validation unchanged
- **WHEN** NodeGraphWidget validates nodes from JSON file
- **THEN** it continues to use NodeTypeRegistry singleton with same validation logic

### Requirement: Modular code organization
The NodeTypeRegistry SHALL be organized in standalone header and source files for better modularity.

#### Scenario: Header file separation
- **WHEN** NodeTypeRegistry implementation is separated
- **THEN** NodeTypeRegistry.h declares public interface and singleton access

#### Scenario: Source file separation
- **WHEN** NodeTypeRegistry implementation is separated
- **THEN** NodeTypeRegistry.cpp contains implementation including constructor, methods, and internal state

#### Scenario: NodeGraphWidget reference
- **WHEN** NodeGraphWidget.cpp uses NodeTypeRegistry
- **THEN** it references the external header file and uses `NodeTypeRegistry::instance()` instead of internal implementation