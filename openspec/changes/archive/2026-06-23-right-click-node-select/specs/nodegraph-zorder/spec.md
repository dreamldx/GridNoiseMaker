# Node Graph Z-Order Management

## MODIFIED Requirements

### Requirement: Selection-driven z-order reordering
The system SHALL automatically adjust z-order values when nodes are selected with the mouse (left-click or right-click), bringing selected nodes to the foreground.

#### Scenario: Mouse selection triggers z-order update
- **WHEN** a user selects a node by clicking it with the mouse (left-click or right-click)
- **THEN** the selected node SHALL receive z-order value of 1
- **AND** other nodes SHALL preserve their relative z-order hierarchy (nodes with original z-order less than selected node remain unchanged, nodes with original z-order greater than selected node shift down by 1)
- **AND** only one node SHALL have z-order 1 at any time (single selection)

#### Scenario: Z-order persistence during selection changes
- **WHEN** a user selects a different node
- **THEN** the newly selected node SHALL receive z-order 1
- **AND** all other nodes SHALL maintain their relative z-order hierarchy (following the same preservation rules as above)
- **AND** nodes SHALL not revert to default z-order 2 unless explicitly reset

#### Scenario: Click behavior respects z-order
- **WHEN** multiple nodes overlap at the same screen position
- **AND** the user clicks (left-click or right-click) on that position
- **THEN** the top-most visible node (lowest z-order value) SHALL be selected when clicked
- **AND** selection SHALL trigger automatic z-order reordering as described above

#### Scenario: Empty canvas click preserves z-order hierarchy
- **WHEN** a user clicks on empty canvas area (no node under cursor)
- **THEN** all nodes SHALL be deselected
- **AND** z-order values SHALL be preserved (not reset to default 2)
- **AND** no node SHALL be selected (but may have z-order value 1)