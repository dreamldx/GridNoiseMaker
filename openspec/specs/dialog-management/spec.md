## Purpose
TODO: Centralized dialog state management for the application

## ADDED Requirements

### Requirement: Centralized dialog state management
The system SHALL provide DialogManager class to centralize UI dialog state management, replacing global variable approach.

#### Scenario: Showing unknown node types dialog
- **WHEN** node loading process detects unknown node types
- **THEN** DialogManager can be used to set dialog state and show modal popup

#### Scenario: Dialog state persistence
- **WHEN** DialogManager is used to show a dialog
- **THEN** the dialog state persists until explicitly dismissed or reset

#### Scenario: Singleton accessibility
- **WHEN** any component needs to manage dialog state
- **THEN** it can access DialogManager via `DialogManager::instance()` method

### Requirement: DialogManager interface
The DialogManager SHALL provide methods mirroring existing dialog functionality for unknown node types.

#### Scenario: Setting unknown node types
- **WHEN** unknown node types are detected during file loading
- **THEN** App can call `DialogManager::instance().setUnknownNodeTypes(types, count)` to set dialog data

#### Scenario: Querying dialog state
- **WHEN** App renders UI
- **THEN** it can call `DialogManager::instance().shouldShowUnknownNodeTypesDialog()` to check if dialog should be displayed

#### Scenario: Resetting dialog state
- **WHEN** user dismisses dialog
- **THEN** App can call `DialogManager::instance().resetUnknownNodeTypesDialog()` to clear dialog state