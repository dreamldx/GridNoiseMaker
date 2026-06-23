# Node Graph Interaction Enhancements

## MODIFIED Requirements

### Requirement: Mouse-driven z-order interaction
The system SHALL handle mouse interactions to trigger hierarchical z-order reordering when nodes are selected, preserving relative z-order relationships among nodes.

#### Scenario: Left-click selects and reorders
- **WHEN** a user left-clicks on a node in the NodeGraphWidget
- **THEN** the clicked node SHALL be selected
- **AND** the node's z-order SHALL be set to 1 (foreground)
- **AND** other nodes SHALL preserve their relative z-order hierarchy (nodes with original z-order less than selected node remain unchanged, nodes with original z-order greater than selected node shift down by 1)

#### Scenario: Click priority respects z-order
- **WHEN** multiple nodes overlap at the same screen position
- **THEN** the click SHALL target the top-most visible node (lowest z-order value)
- **AND** the clicked node SHALL receive z-order 1
- **AND** hierarchical z-order preservation SHALL be applied to other nodes
- **AND** the visual z-order display SHALL update immediately

#### Scenario: Empty canvas click behavior
- **WHEN** a user clicks on empty canvas area (not on any node)
- **THEN** any currently selected node SHALL be deselected
- **AND** z-order values SHALL be preserved (hierarchy maintained)
- **AND** no node SHALL be selected (but may retain z-order value 1)