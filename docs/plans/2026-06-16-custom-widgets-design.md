# Custom Widget Library — Node Graph Editor — Design

**Date:** 2026-06-16
**Status:** Approved (brainstorming)
**Author:** dliu (with Claude)
**Supersedes:** the non-goal "No custom widget library on top of ImGui" in the `imgui-cross-platform-app` proposal — that boundary is now intentionally crossed.

## Decision summary

- **Widget domain:** node graph editor (domain-specific, built into this playground app).
- **Behavior tier:** pluggable evaluation framework — host code registers node types and their evaluation callbacks; the library handles topology, cycle detection, and dirty propagation.
- **Canvas:** built from scratch (no ImNodes dependency); pan/zoom/bezier/pin-hit-testing/selection are our code.
- **Delivery split (Approach B):** two OpenSpec changes.
  - **Change 1 — `add-node-graph-core`** *(this doc)* — headless engine: data model, type system, node registry, topology, evaluation, serialization, sample nodes. Fully unit-testable without a window.
  - **Change 2 — `add-node-graph-editor`** *(out of scope for this design doc)* — from-scratch UI on top of the frozen core API.

## Why Change 1 first

The headless engine is the higher-risk piece: graph correctness, cycle detection, and dirty propagation are subtle and easy to regress, but they are also fully testable in a CI matrix with no GUI. Landing the core first lets us freeze the API the editor will consume, then build the canvas against a stable foundation. The alternative (one monolithic change) creates an unreviewable PR and tangles correctness bugs with UX bugs.

## Architecture

Four layers, each depending only on the ones below. The core lives as a **subsystem inside the existing app shell** at `apps/imgui-shell/src/node_graph/` — not a separate CMake static-library target. This matches the "domain-specific widget" framing and keeps CMake simple; promoting it to a reusable library is a later, low-cost change once the API is stable.

```
┌─────────────────────────────────────────┐
│ 4. Evaluator    (topology sort, dirty   │
│                  propagation, schedule) │
├─────────────────────────────────────────┤
│ 3. NodeRegistry (type lookup, factory,  │
│                  per-type eval callback)│
├─────────────────────────────────────────┤
│ 2. Graph        (Node/Pin/Connection,   │
│                  mutation API, queries) │
├─────────────────────────────────────────┤
│ 1. ValueTypes   (built-in pin types,    │
│                  Value variant, IO)     │
└─────────────────────────────────────────┘
```

Serialization is a cross-cutting concern that touches layers 1–3. The Evaluator (layer 4) is pluggable: host code registers node types via the registry with eval callbacks; the evaluator orchestrates when to call them, never what they do.

Layer boundaries are enforced by directory + single-public-header convention, not just discipline.

### Language and dependencies

- **Language baseline:** C++17, matching the `imgui-cross-platform-app` change. No bump to C++20/23.
- **No platform-specific code in core.** Same source compiles unchanged on macOS, Windows, Linux, iOS.
- **No ImGui in core.** The core is GUI-agnostic; the editor change consumes it.
- **New deps:**
  - `nlohmann/json` (header-only, MIT) for serialization, fetched via CMake `FetchContent`.
  - `Catch2` v3 (MIT) for the test target, fetched via `FetchContent`.

## Components

### Layer 1 — `ValueTypes`

- `TypeId` — strong typedef over a stable string (`"int"`, `"float"`, `"vec3"`, etc.). String-based so future host code can register custom types without recompiling the core.
- `Value` — `std::variant` over built-in types, tagged with its `TypeId`. `as<T>()` accessor returns `Result<T, ValueError>` on mismatch.
- `TypeRegistry` — `TypeId → (defaultValue, parse-from-json, serialize-to-json)`. Built-in types pre-registered.
- **Built-in types (v1):** `int`, `float`, `bool`, `string`, `vec2`, `vec3`, `vec4`, `color`.

`std::variant` locks the built-in type set at compile time. Acceptable for v1; type-erased `Value` is deferred until a host actually needs to extend the type set.

### Layer 2 — `Graph`

- `PinDirection` — `Input | Output`.
- `Pin` — `{ id, nodeId, direction, name, typeId, defaultValue }`. Pin IDs are graph-unique `uint32_t`.
- `Node` — `{ id, typeName, position (vec2), inputPins[], outputPins[], paramValues (map<string, Value>) }`. Params hold values for unconnected inputs and node-internal configuration.
- `Connection` — `{ fromPin, toPin }`. One output may feed many inputs; each input pin has at most one incoming connection.
- `Graph` — owns nodes and connections.
  - `addNode(typeName, position) → Result<NodeId, ConnectError>`
  - `removeNode(id)` — no-op if absent; cleans up incident connections.
  - `connect(fromPin, toPin) → Result<ConnectionId, ConnectError>` — validates direction (out→in), pin type match, target input has no existing connection.
  - `disconnect(connId)` — no-op if absent.
  - Iterators over nodes/connections, stable order.
- **No cycle check on `connect()`** — cycles are an evaluation concern; during editing you can transiently have one.

### Layer 3 — `NodeRegistry`

- `NodeTypeDescriptor` — `{ typeName, displayName, category, inputPinSpecs[], outputPinSpecs[], paramSpecs[], evalFn }`.
- `PinSpec` — `{ name, typeId, defaultValue }`.
- `NodeRegistry` — register/lookup descriptors by `typeName`. `Graph::addNode` consults the registry to materialize pins.
- **Built-in nodes (v1):**
  - `Constant<T>` — no inputs, one typed output, value as a param.
  - `Add` — two `float` inputs, one `float` output.
  - `Multiply` — two `float` inputs, one `float` output.
  - `Output` — one input (any type via a generic `Value` pin), no outputs; serves as an inspection sink.

### Layer 4 — `Evaluator`

- `Evaluator` — given a `Graph` + `NodeRegistry`, computes pin values.
  - `topologySort() → Result<vector<NodeId>, CycleError>` — Kahn's algorithm; `CycleError` includes the cycle nodes.
  - `markDirty(nodeId)` / `markAllDirty()` — caller-driven invalidation (explicit, no observer wiring).
  - `evaluate() → EvalReport` — walks topo order, skips clean nodes, calls each node's `evalFn`, caches outputs on pins.
  - `getPinValue(pinId) → optional<Value>` — read cached output.

`evalFn` signature:
```cpp
void eval(std::span<const Value*> inputs,
         const ParamMap& params,
         std::span<Value*> outputs,
         std::string* errorOut);
```

Inputs/outputs are pointers — evaluator owns storage on pin caches, no per-eval allocation for built-in nodes.

### Cross-cutting — `Serialization`

Single header `serialize.h`:

- `toJson(Graph&) → json` — infallible (we control the input).
- `fromJson(json, NodeRegistry&) → Result<Graph, LoadError>`.

Wire format:
```json
{
  "version": 1,
  "nodes": [
    { "id": 1, "typeName": "Add", "position": [120, 80],
      "params": { "bias": { "type": "float", "value": 0.5 } } }
  ],
  "connections": [
    { "fromPin": { "nodeId": 1, "name": "out" },
      "toPin":   { "nodeId": 2, "name": "a" } }
  ]
}
```

Connections serialize by **pin name + node id**, not pin id (pin ids regenerate on load). Node ids remap via an `oldId → newId` table during load.

## Data flow

### A. Mutation
Caller invokes `Graph::addNode` / `connect` / `removeNode`. Graph validates and updates its internal state. Graph does **not** know the Evaluator exists. After a mutation that the caller knows affected evaluation, the caller invokes `evaluator.markDirty(nodeId)` (or `markAllDirty()`). This is **explicit invalidation** — chosen over observer-pattern auto-correct because it keeps the Graph as pure data and avoids back-edges in the dependency graph.

### B. Evaluation

```
Evaluator::evaluate():
  topo = topologySort()
  if topo is Err: return EvalReport{ CycleDetected, cycleNodes }
  for node in topo:
    if !node.dirty: continue
    inputs = gather from upstream pin cache, or pin.defaultValue if unconnected
    error = ""
    evalFn(inputs, node.params, outputs, &error)
    if error not empty:
      mark this node failed; skip all downstream (their caches remain readable)
      return EvalReport{ NodeEvalFailed, failedNodeId, error }
    write outputs to pin cache
    node.dirty = false
  return EvalReport{ Ok }
```

Dirty propagation is **push-on-mutation (via explicit `markDirty`), pull-on-evaluate**. Editor change later can call `evaluate()` every frame; only dirty subgraphs recompute.

### C. Serialization round-trip
`fromJson(toJson(g))` produces a graph with identical structure modulo id renumbering. Verified via a `structurallyEqual()` test helper. Property test runs over a small library of hand-built graph shapes.

## Error handling

No exceptions in the public API. Three error surfaces, each with a typed enum:

| Surface | Result type | Variants |
|---|---|---|
| Mutation (`Graph`) | `Result<T, ConnectError>` | `TypeMismatch`, `WrongDirection`, `InputAlreadyConnected`, `UnknownNodeType`, `InvalidPin` |
| Evaluation | `EvalReport` struct | `Ok`, `CycleDetected` (+ `cycleNodes`), `NodeEvalFailed` (+ `failedNodeId`, `message`) |
| Load | `Result<Graph, LoadError>` | `MalformedJson`, `UnsupportedVersion`, `UnknownNodeType`, `DanglingConnection` |

`Result<T, E>` is **hand-rolled, ~30 LOC**, stays on C++17 (no `std::expected`, no `tl::expected` dependency).

### Behavior decisions

- **Cycles** — `evaluate()` returns `CycleDetected`, leaves all pin caches untouched (last-good values stay readable). Editor will later render cycle nodes in red.
- **Per-node eval failures** — failing `evalFn` writes an error string; evaluator records it, **skips all downstream nodes entirely** (their caches retain last-known values; they are not re-evaluated with defaults). Failure is sticky until the next `markDirty` on the failed node.
- **Tolerant queries** — disconnecting an absent connection, removing an absent node, and reading a pin on an unevaluated graph all succeed (the last returns `defaultValue`). Errors are reserved for things the caller could have validated.

## Testing

- **Framework:** Catch2 v3, fetched via CMake `FetchContent`.
- **Target:** new CMake target `node_graph_tests` (separate from the app binary).
- **Layout:** `apps/imgui-shell/src/node_graph/tests/` — one file per layer (`test_value.cpp`, `test_graph.cpp`, `test_registry.cpp`, `test_evaluator.cpp`, `test_serialize.cpp`).
- **CI:** runs on macOS, Windows, Linux as a separate step after the app build (iOS skipped — same convention as `imgui-cross-platform-app`).

### Coverage per layer

- **ValueTypes** — round-trip every built-in type through `Value` and through JSON; `as<T>()` mismatch surfaces a typed error.
- **Graph** — every `ConnectError` variant has a test that triggers it; `removeNode` cleans up incident connections; iteration order is stable.
- **NodeRegistry** — duplicate registration rejected; unknown-name lookup returns null; `registerBuiltins()` registers all four v1 node types.
- **Evaluator** (highest value):
  - Linear graph: `Constant(2) → Add ← Constant(3)` ⇒ output reads 5.
  - Diamond graph: shared upstream evaluated exactly once (eval-count probe).
  - Dirty propagation: mutate one node, `markDirty`, re-evaluate ⇒ only dirty subgraph's `evalFn` ran.
  - Cycle: build `A→B→A`, evaluate ⇒ `CycleDetected`, both nodes in `cycleNodes`, caches unchanged.
  - Eval failure: failing node returns `NodeEvalFailed`; downstream caches **unchanged** (skip-downstream semantics).
- **Serialization** — round-trip property over a library of graphs; each `LoadError` variant triggered by a hand-crafted malformed input.

### Out of scope for v1

- Performance benchmarks (no SLA yet).
- Serializer fuzzing.
- Editor / canvas — Change 2.
- Coverage gate in CI (no tool installed yet).

### Definition of done

- All five test files exist and pass on three CI platforms.
- Every public function in the four layer headers has at least one calling test.
- `node_graph_tests` is added to the existing CI matrix as a build+test step.

## Impact

- **New code:** `apps/imgui-shell/src/node_graph/` (estimated 1500–2500 LOC including tests).
- **New deps:** `nlohmann/json` and `Catch2` v3, both fetched via `FetchContent`, both MIT-licensed.
- **CMake:** one new test target, one extra `FetchContent_Declare` per dep.
- **CI:** one new test step per existing desktop platform; iOS unchanged.
- **No existing code modified** — additive within the `apps/imgui-shell/` tree, which the `imgui-cross-platform-app` change already establishes.

## Out of scope (for Change 1)

- Any ImGui rendering, canvas drawing, pan/zoom, bezier curves, drag interactions, context menus, selection box, copy/paste, undo/redo, keyboard shortcuts. All editor / UI surface is **Change 2**.
- Custom Value types beyond the v1 built-in set.
- Performance optimization (correctness first; benchmarks deferred).
- Hot-reloading node type definitions.

## Open questions deferred to Change 2 design

- Visual treatment of failed nodes vs. cycle nodes.
- Whether `Output` nodes should be auto-displayed in an inspector panel.
- Keybindings for common operations (delete, duplicate, frame-selected).

## Next step

Transition to an OpenSpec change proposal for `add-node-graph-core` via `/opsx:propose` (or `/opsx:new` followed by `/opsx:continue`), using this design doc as the source of truth.
