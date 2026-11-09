# HWIR structural C++ backend

This document defines the ownership boundaries for the structural C++ backend.
It preserves module hierarchy, flattens SSA only within a module, and accepts
one closed top-level design whose parameters, types, instance targets and
dependencies are fully resolved. Fixed static collections are expanded into
concrete scalar lanes; runtime family batching remains a later optimization.

The historical final-ACIR `--target cpp` emitter has been removed. The native
C++ source entrypoint is `--target structural-cpp --format source`. JSON output
contains the structural header, source-unit implementations and stable
specialization manifest. The Python publisher materializes them as
`design.hpp`, `source_unit_<N>.cpp`, `simulation_main.cpp` and `manifest.json`.
When one module's rendered Work schedule exceeds the ordinary header budget,
the output also contains source-owned `work_unit_<N>_<M>.cpp` translation units.

The backend boundary is represented in CompilerDev by
`Compiler/StructuralCppBackend.h`. `verifyStructuralCppInput()` is the
structural-C++ admission entrypoint; it reuses final-package verification,
requires the hardware-only HWIR marker, requires one resolvable system root and
concrete fixed collection shapes. `emitStructuralCpp()` is the separate
plan-to-text entrypoint. `Compiler/ConcreteDesignPlan.h` defines the current
backend-owned plan, and `buildConcreteDesignPlan()` constructs the concrete
root-reachable specialization closure using shared `HardwareAnalysis` results.
`emitStructuralCpp()` builds and validates that plan. The scalar renderer covers
concrete bits arithmetic, comparison, selection, concatenation, extraction and
resize; packed struct/enum construction and projection; ordinary module
occurrences; dff/dffe, FIFO and memory primitives. Unsupported mappings fail
closed rather than falling back to another emitter.

The plan builder records concrete port widths, stable module-local
value IDs, occurrence bindings, ordinary module/primitive/queue children, the
verified module-local schedule and explicit lifecycle records. It is exposed by
the native emitter and the Python structural-C++ bundle publisher.

Four-state constants are planned as three equal-width word arrays: value,
known and Z. The renderer constructs `gfsim::FourState<W>::fromMasks(...)`
directly from those arrays, including multiword widths, so X and Z are never
approximated by a two-state C++ literal.

The plan also carries a closure-level dependency graph. Its nodes are scheduled
operations annotated with their owning concrete specialization; its edges are
SSA dependencies plus ordinary module boundary connections. Temporal child
outputs are deliberately treated as cuts, using the verified `storageWork`
classification from `HardwareAnalysis`, so queue/register feedback is not
mistaken for a combinational cycle. The graph is checked before emission and is
not a replacement for HWIR dependency semantics.

Each concrete module additionally carries a backend schedule view. It expands
the verified module schedule into value-level steps for combinational SSA,
child input drive, child Work, committed temporal reads and output
materialization. Temporal-output flags are resolved during planning for both
primitives and ordinary child modules. The plan also records a pure reset-input
preparation projection and an Xfer-time temporal-output refresh projection.
These are rendering contracts derived from verified HWIR facts; the renderer
does not inspect HWIR or invent a second semantic scheduler.

Table-element values are alias views rather than independently stored values.
The plan records an availability-parent edge from each lane to its whole table
and uses a plan-only availability barrier to publish a table after all lane
writers complete. Table construction itself is a concrete compute step that
assigns every lane. Neither mechanism requires the renderer to query HWIR, and
the availability barrier deliberately emits no C++ statement.

Lifecycle planning is split deliberately. The design plan records one root
Work call, one whole-system Check, and the mutually exclusive root Xfer or
Discard branch. Each module plan records direct child and primitive Xfer and
Discard calls in occurrence order. Ordinary children recursively own their
descendants; the plan does not flatten state commits across module boundaries
or invent a per-primitive Check phase.

## Ownership model

The backend has four owners with deliberately different authority:

```text
verified HWIR design
        │ semantic authority
        ▼
concrete-plan builder
        │ validates and records code generation decisions
        ▼
ConcreteDesignPlan
        │ immutable, non-semantic code generation plan
        ▼
structural C++ emitter
        │ deterministic plan-to-text rendering
        ▼
generated C++ + shared Runtime
        │ hardware values and state-transition execution
        ▼
C++20 compiler and simulation executable
```

HWIR owns hardware meaning. The concrete plan records a fully resolved view of
that meaning for one backend invocation. The emitter renders that plan. The
Runtime executes the hardware value and temporal contracts selected by HWIR.
None of the downstream layers may silently repair, widen or reinterpret an
upstream decision.

## HWIR is the semantic authority

HWIR is the sole authority for the hardware structure and behavior accepted by
this backend. It owns:

- module definitions, ports and instance hierarchy;
- SSA producers and operand relationships;
- integer parameters, type parameters and their bindings;
- finite bit widths, aggregate layouts and table shapes;
- combinational dependencies and temporal dependency cuts;
- primitive identity, pin schemas and primitive configuration;
- reset, hold, discard and commit behavior required by the design;
- the selected system root and its externally visible interface; and
- source identity and locations used for diagnostics and source maps.

The first backend accepts only a closed design. Every ordinary instance target
must resolve inside the input closure, and every backend-relevant parameter,
type, extent, width and dependency must have one concrete answer. An unresolved
type parameter, symbolic width, symbolic extent, unknown primitive, unresolved
callee or unclassified dependency is an unsupported input, not a request for
the backend to guess.

Shared HWIR verification and analysis may be used to prove these properties.
That analysis runs before emission and remains responsible for diagnostics tied
to the originating HWIR operation. Successful verification means that the
concrete-plan builder may read authoritative results; it does not transfer
semantic authority to the plan or emitter.

The following transformations are outside the emitter:

- choosing a default parameter that HWIR has not already selected;
- inferring signedness or a result width from C++ operand types;
- discovering a missing connection from names or source order;
- deciding whether an edge is combinational or sequential;
- treating an unknown module as a black box;
- inserting state, latency, arbitration or clock-domain behavior; and
- resolving a combinational cycle through repeated C++ evaluation.

If any such decision is still required, HWIR construction or verification is
incomplete and code generation must fail before producing an output bundle.

## The concrete plan is not a new IR

`ConcreteDesignPlan` is a backend-owned, immutable code generation plan. It is
not a dialect, a serialized interchange format or an independent statement of
hardware semantics. It must not acquire passes, canonicalization rules or
fallback verification that compete with HWIR.

The plan materializes information needed repeatedly during emission, including:

- the selected root specialization;
- the root-reachable module specialization closure;
- concrete integer and type bindings for every specialization;
- stable C++ names for concrete modules, ports, values and instances (module
  specialization names use a canonical binding key and `__h<stable-hash>`;
  traversal order is never part of the name);
- concrete C++ hardware types and packed widths;
- runtime type records for bits, tables, structs and enums, including fixed
  C++ type spellings and resolved table extents;
- child-instance and primitive inventories;
- the verified local SSA schedule for each module;
- explicit steps for driving a child, evaluating it and reading its outputs;
- state lifecycle calls required from each module; and
- file membership, source ownership and source-map records.

Plan construction may query the shared HWIR analyses, but it may only copy or
normalize their results. It must fail if a result is absent or non-concrete. In
particular, the first plan must contain no unresolved `StaticExpr`, type formal,
symbolic array bound or deferred instance lookup.

The plan may introduce non-semantic code generation choices such as a stable
temporary name, output file assignment, C++ declaration order or whether a
verified SSA value receives a named local. Such choices may affect generated
text and compilation cost, but must not affect hardware results or timing.

The plan must not:

- add, delete or replace a hardware operation;
- infer dependencies by parsing names or generated expressions;
- change packed layout or arithmetic width;
- turn a module instance into a primitive, or the reverse;
- select reset polarity, clock edges or memory collision behavior;
- invent a value for an unresolved input; or
- use Runtime behavior to make an otherwise invalid HWIR design executable.

A useful implementation invariant is that the plan builder is the last stage
allowed to call semantic queries such as instance binding, static evaluation,
type resolution or dependency analysis. The emitter receives only concrete
plan records.

## Runtime owns hardware values and temporal primitives

Generated structural C++ uses the shared Runtime rather than native C++ values
as an implicit hardware model. The Runtime owns:

- fixed-width bit storage;
- known, unknown and high-impedance state where supported;
- hardware arithmetic, comparison, selection, concatenation and extraction;
- explicit packing and unpacking of nominal aggregates;
- register, memory, queue and other approved temporal primitive kernels;
- current and pending state;
- Work, whole-system check, Xfer and discard sequencing; and
- reset and sampled-clock behavior defined by each primitive contract.

Generated code may compose these facilities, connect concrete ports and invoke
their lifecycle methods. It must not replace them with host-language shortcuts
whose semantics differ from hardware. For example, host integer promotion,
`sizeof`-based record layout and immediate assignment to committed register
state are not valid substitutes for Runtime operations.

The Runtime does not decide the structure of a design. It does not bind an
instance, infer a width, select a primitive or discover a schedule. A Runtime
template parameter or constructor argument must be a concrete value already
present in the plan.

The generated lifecycle is:

```text
Work
  read committed state
  evaluate local SSA
  drive and evaluate child modules in the verified order
  prepare primitive pending state
  produce module outputs

whole-system check
  accept all pending work or reject the complete step

Xfer                         DiscardNext
  commit every pending state   restore/drop every pending proposal
  refresh temporal outputs

Reset
  propagate module inputs, two-state and exact four-state constants, and
  initializer expressions; four-state constants retain separate value,
  known-mask, and Z-mask planes in the concrete plan
  reset children, commit their reset state, then refresh temporal outputs
```

This separation preserves simultaneous hardware state updates. C++ statement
order is an implementation of the verified dependency schedule, not a new
source of sequential hardware semantics.

The generated `Work()` entry point remains a small ordered dispatcher. The
same rule applies to `RefreshPureOutputs()`, `RefreshTemporalOutputs()` and
`PrepareResetInputs()`: when a projection is large, its entry point only calls
ordered noinline helpers. The emitter partitions each verified schedule while
retaining operation results as module members, so a value produced by one
helper remains the scratch input of a later helper. Preferred split points are
existing availability, child-Work, primitive-Work and output-materialization
boundaries; they do not add a commit or sampling boundary. Observation staging
follows the schedule helpers in the same Work call. Whole-system checks and the
exclusive Xfer/Discard branch remain owned by `SimSystem` after Work returns.

Ordinary modules keep their helpers in `design.hpp`. Large modules declare the
helpers in the header and place their definitions in bounded source shards.
Each helper is emitted as a distinct C++ symbol and each shard is an independent
translation unit. This keeps ARM64 branch targets below the single-function
`B`/`BL` range limit and lets the generated CMake build compile shards in
parallel without changing the total design or its source ownership.
`source_unit_<N>.cpp` retains constructor, check and lifecycle glue; expanded
schedule bodies live in the work shards.

Emission fails closed if one generated helper or dispatcher exceeds 512 KiB,
or if one generated structural-C++ translation unit exceeds 1 MiB. The bundle
CMake target lists every shard as an independent source and precompiles the
shared `design.hpp`, allowing Ninja to optimize the bounded units in parallel
without reparsing the full declaration header for each one. The enum-source
end-to-end regression keeps the existing 120-second command timeout and adds a
90-second maximum for every object compilation recorded by Ninja. These are
regression budgets, not alternate hardware semantics or permission to increase
the outer timeout.

The public typed DUT API lives in `design.hpp`. Its root instance is explicitly
host-driven, which keeps a combinational root runnable for every supplied input
epoch even when it has no temporal child. Nested modules do not inherit that
property and continue to report work only through stateful descendants. The
typed root suppresses the post-Xfer temporal refresh to preserve old-Q sampling
for the host; ordinary nested modules refresh after Xfer so their parents see
newly committed state during the next Work.

## The emitter is a deterministic renderer

The structural C++ emitter consumes a completed `ConcreteDesignPlan` and emits
fixed C++ types. In the current implementation:

- every concrete module specialization becomes one non-template C++ class;
- ordinary child modules remain explicit members or owned objects;
- SSA is flattened only inside its owning module;
- child internals are not recursively emitted into the parent's Work body;
- ports and primitive configurations use concrete Runtime types;
- the root alias names a fixed class, not a family or unresolved template;
- fixed collections are expanded into concrete scalar lanes; no unresolved
  family count or dynamic lane storage is generated; and
- encountering a missing plan record is an internal error, not an analysis
  opportunity.

The emitter may select syntax that preserves the plan, for example named SSA
temporaries instead of nested expressions. It may not recompute or reinterpret
the plan. Specifically, emitter code must not call instance binding, static
evaluation, type resolution, dependency discovery or combinational scheduling
APIs. Those calls belong in plan construction.

This restriction also applies to apparently harmless convenience logic. The
emitter must not:

- derive a width from a rendered C++ type;
- use declaration order as a dependency order;
- inspect a callee body to decide whether it is stateful;
- infer port direction or correspondence from a spelling;
- choose a primitive implementation from its pin list; or
- emit a symbolic C++ expression when the plan lacks a concrete number.

Unsupported plan content must be rejected before rendering. An unexpected
record during rendering indicates a compiler invariant violation and should
produce a precise internal diagnostic without publishing partial output.

## Admission boundary

The concrete-plan builder rejects at least the following:

- more or fewer than one system root;
- unresolved ordinary imports or instance targets;
- recursive module specializations;
- dynamic or unresolved collection shapes and unsupported family execution;
- integer bindings that are not concrete mathematical integers;
- unresolved type parameters;
- non-concrete bit widths, table extents, memory depths or queue parameters;
- unknown primitive kinds or invalid primitive pin schemas;
- combinational cycles, including cycles across module boundaries;
- operations without an approved structural C++ mapping; and
- instrumentation whose execution boundary has not yet been implemented by
  this backend.

These are backend admission limits, not alternative language semantics. A
future increment may support an item only by extending HWIR verification, plan
construction, Runtime behavior where necessary, and independent execution
evidence. The emitter alone cannot make an unsupported construct supported.

## Later family batching

Family batching is a later optimization over an already correct scalar plan.
It must not become part of HWIR semantics or weaken the scalar concrete
admission rules. A future family lowering may group compatible scalar
occurrences, replace scalar storage with lane storage and emit batched Work, but
must preserve:

- each scalar instance's identity and source mapping;
- the scalar module's port and state behavior;
- the verified dependency and commit boundaries;
- reset, hold, discard and failure behavior; and
- results equivalent to evaluating the same number of scalar instances.

The scalar structural backend remains the semantic comparison point for this
optimization. Family lowering must be removable without changing accepted HWIR
behavior.

## Validation expectations

Tests should keep the ownership boundaries observable:

- plan tests prove complete binding, concrete types, closed dependencies and
  stable specialization names;
- emitter tests prove that output contains no unresolved template parameters,
  family counts or symbolic dimensions;
- structure tests prove that child modules remain explicit and that local SSA
  does not cross module boundaries;
- Runtime tests independently validate hardware values and temporal primitive
  transitions;
- C++/Verilog differential tests compare outputs, reset, hold, commit and
  discard behavior from the same verified design; and
- large-plan publication tests compile and link all schedule/lifecycle shards
  at `-O0`, run a dependency that crosses helper boundaries, and verify the
  512 KiB function and 1 MiB translation-unit bounds as well as source
  ownership; and
- enum-source native execution builds the generated target through
  CMake/Ninja at `-O2`, verifies the 90-second per-object budget, then runs the
  same workers-1/workers-2 and RTL comparisons without increasing its
  120-second command timeout.

Generated C++ consumers link the Runtime-only package and must not require LLVM
or MLIR. Publication remains transactional: failed verification, planning,
rendering, compilation checks or bundle validation must not replace a prior
valid output.

## Hardware-only admission source

`hwir.hardware_only` is a final-package admission certificate, not a user
input hint and not a replacement for verification.  Its canonical producer is
the successful exit of `ac-lower-transactions`, which publishes it only after:

1. the merged package envelope has been admitted;
2. ordinary hardware verification has succeeded;
3. all source transaction semantics, when present, have been materialized;
4. the original-source proof and final hardware verification have succeeded.

The structural C++ backend requires both `ac.stage = "final"` and
`hwir.hardware_only`, then independently runs the final hardware verifier and
concrete-plan admission.  Parsing an attribute with this spelling therefore
does not make malformed serialized IR trusted.  No emitter is allowed to set
the certificate or infer hardware-only status from the operations it happens
to understand.
