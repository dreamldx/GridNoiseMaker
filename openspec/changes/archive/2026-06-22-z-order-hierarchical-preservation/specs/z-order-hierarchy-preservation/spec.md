# Z-Order Hierarchy Preservation

## Purpose
Preserves relative z-order hierarchy when nodes are selected or reordered in NodeGraphWidget, maintaining layered compositions instead of resetting all unselected nodes to default z-order.

## ADDED Requirements

### Requirement: Hierarchical z-order preservation
The system SHALL preserve relative z-order hierarchy when nodes are selected, maintaining the layered relationships between nodes instead of resetting all unselected nodes to a default value.

#### Scenario: Hierarchical preservation during node selection
- **WHEN** a node with original z-order value Z is selected
- **THEN** the selected node SHALL receive z-order value 1
- **AND** nodes with original z-order values less than Z SHALL remain unchanged
- **AND** nodes with original z-order values greater than Z SHALL shift down by 1
- **AND** the relative ordering between all nodes SHALL be preserved

#### Scenario: Multiple sequential selections maintain hierarchy
- **WHEN** multiple nodes are selected sequentially
- **THEN** each selection SHALL apply hierarchical preservation rules
- **AND** the cumulative effect SHALL maintain all relative z-order relationships
- **AND** nodes SHALL not revert to default z-order 2 unless explicitly reset

#### Scenario: Z-order hierarchy with equal values
- **WHEN** multiple nodes have the same original z-order value
- **THEN** stable sorting SHALL preserve their original creation/insertion order
- **AND** hierarchical preservation SHALL treat them as having distinct logical positions
- **AND** the first (top-most) node in the equal-value group SHALL be selected when clicked

#### Scenario: Empty canvas reset
- **WHEN** a user clicks on empty canvas area
- **THEN** all nodes SHALL revert to default z-order value of 2
- **AND** no node SHALL have z-order 1
- **AND** this SHALL provide a complete z-order reset mechanism