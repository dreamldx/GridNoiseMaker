# Node Graph Visual Feedback

## Purpose
Provides visual feedback mechanisms for NodeGraphWidget including z-order display, selection highlighting, and visual hierarchy indicators.

## ADDED Requirements

### Requirement: Z-order value visualization
The system SHALL display z-order values at the bottom of each node for clear visual feedback and debugging.

#### Scenario: Z-order text display format
- **WHEN** a node is rendered in the NodeGraphWidget
- **THEN** its current z-order value SHALL be displayed as text in the bottom area of the node
- **AND** the text SHALL use format "Z: <value>" (e.g., "Z: 1", "Z: 2")
- **AND** the text SHALL be small and non-intrusive to the node's primary content

#### Scenario: Real-time z-order value updates
- **WHEN** a node's z-order value changes (through selection or manual adjustment)
- **THEN** the displayed z-order text SHALL update immediately to reflect the new value
- **AND** the visual update SHALL occur on the same frame as the z-order change

#### Scenario: Consistent visual styling
- **WHEN** z-order values are displayed
- **THEN** the text SHALL use consistent styling across all nodes
- **AND** SHALL use a color that provides sufficient contrast against the node background
- **AND** SHALL be positioned consistently in the bottom area of each node

### Requirement: Selection visual feedback
The system SHALL provide clear visual indicators to show which node is currently selected.

#### Scenario: Selected node highlighting
- **WHEN** a node is selected in the NodeGraphWidget
- **THEN** the selected node SHALL receive distinct visual highlighting
- **AND** the highlighting SHALL be visible regardless of the node's position or z-order
- **AND** the highlighting SHALL be removed when the node is deselected

#### Scenario: Multiple visual feedback channels
- **WHEN** visual feedback is implemented
- **THEN** the system MAY use multiple visual channels such as:
  - Border color changes for selected nodes
  - Background color tint for selected nodes
  - Subtle glow or shadow effects
  - Text label emphasis

#### Scenario: Visual feedback integration with z-order
- **WHEN** selection and z-order systems work together
- **THEN** selected nodes SHALL always appear with z-order 1 (foreground)
- **AND** visual highlighting SHALL reinforce the z-order-based depth perception
- **AND** users SHALL be able to visually identify the selected node even when multiple nodes overlap