## ADDED Requirements

### Requirement: Panel supports docking and detaching
The node type panel SHALL support docking to different sides of the workspace and detaching as a floating window, allowing users to customize their workspace layout.

#### Scenario: Panel can be docked to left or right side
- **WHEN** the node type panel is docked
- **THEN** it SHALL be attachable to either the left or right side of the node graph workspace
- **AND** SHALL maintain a resizable split between panel and workspace
- **AND** docking position SHALL be visually indicated

#### Scenario: Panel can be detached as floating window
- **WHEN** the user chooses to detach the panel
- **THEN** the panel SHALL become a separate floating window
- **AND** SHALL be movable independently of the main application window
- **AND** SHALL maintain all functionality while floating

#### Scenario: Docking state is preserved
- **WHEN** the user changes docking state (docked left, docked right, floating)
- **THEN** the docking state SHALL be persisted across application sessions
- **AND** SHALL be restored when the application is restarted
- **AND** floating window position SHALL be restored if applicable

#### Scenario: Docking controls are accessible
- **WHEN** the node type panel is visible
- **THEN** it SHALL provide user controls for changing docking state
- **AND** controls SHALL include options for dock left, dock right, and float
- **AND** current docking state SHALL be visually indicated

#### Scenario: Workspace adapts to docking changes
- **WHEN** the panel docking state changes
- **THEN** the node graph workspace SHALL automatically adjust its layout
- **AND** SHALL maintain usable workspace area regardless of docking configuration
- **AND** SHALL not lose node positions or view state during layout changes