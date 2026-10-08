# Known limitations and follow-up work

This records pyCircuit 6.1 limitations and follow-up work as of 2026-10-08.
Closed `@system` source compilation, generated C++/Verilator simulation, source
checks, observation publication and failure-atomic state/clock handling now have
focused independent verification. Full example adaptation and broader language
coverage remain incomplete. The current work prioritizes example migration and
retains the original independent verification assets.

Removing unfinished examples does not mean their functionality is implemented.
Items below are grounded in the current source reference, compiler admission
checks and existing test ownership. Focused migration gates have passed; the
complete nightly and platform matrices still require final candidate verification.

## Language and compiler

| Area | Current limitation | Follow-up acceptance |
| --- | --- | --- |
| Source consistency | Structural leaf binding and inferred behavioral storage share the compiler but still admit different rule statements and forward references. Some errors expose internal profile terms. | Improve the existing source/MLIR owners so equivalent hardware receives consistent admission and actionable diagnostics. |
| Modules and imports | Canonical fixed-bit imports and explicit all-field imported Struct expression constructors are admitted through published interfaces. Imported defaults, static initializers, Table-query constructor callbacks, relative imports, general type aliases and arbitrary cross-source rule bodies remain restricted. Module outputs need explicit type information. | Preserve canonical nominal authority and failed-publication protection when extending admission. |
| Parameters | Standard-leaf type arguments and keyword defaults do not establish general parameter-dependent elaboration or emitted module families. | Verify nondefault widths, nested configurations and instance isolation through the same compiler. |
| Collections | Tables are one-dimensional; Table-in-Struct, Table-of-Table, general array/scan/reduction authoring and recursive aggregate updates are incomplete. | Add generic type/effect/lowering support and independent C++/RTL results without recipe recognition. |
| Multiple writers | Disjoint fields/elements and proved grants are supported; unproved overlaps reject. General transactions, reservation and cross-domain atomic updates are incomplete. | Extend common-IR proofs/checks while preserving old-state reads and whole-system zero commit on failure. |
| Integer arithmetic | Mathematical Integer runtime division/remainder, dynamic or signed shifts, dynamic range checks and some equivalent symbolic-width normalization remain restricted. Fixed-bit division has a separate supported contract. | Prove value/range behavior, X/Z propagation and failed-publication protection for each extension. |
| Control flow | Early returns inside branches, general match patterns/guards, arbitrary callback effects and general Python execution are unsupported. | Add only constructs with finite hardware meaning; preserve source ordering and four-state selection. |
| Compiler size and diagnostics | Source import still concentrates substantial behavior in `PythonImportRecords.cpp`; source resolution and semantic inference need clearer ownership. | Move semantic analyses into existing MLIR passes, reduce duplicate checks and provide source-level corrective messages. |

## Runtime and backends

| Area | Current limitation | Follow-up acceptance |
| --- | --- | --- |
| System authoring | Closed default-domain systems work; the full example inventory and broader source forms have not been adapted and validated. | Adapt each original scenario without weakening its hardware behavior or independent oracle. |
| Queue protocols | Original route/credit/reorder histories and source faults now execute through managed module verification, with independent protocol checkers. Historical direct batch preloads and exceptional host partial commits are not generally expressible by closed default-clock systems. | Preserve token conservation, timing, capacity and backpressure; distinguish supported system trajectories from historical host-only actions. |
| Source checks | Closed systems and checked ordinary modules execute assertions through both backends. Checked module RTL requires the documented managed phase/commit/error caller boundary. RTL failures do not yet include the native structured check location/ID. | Retain full original vectors, sticky failure and independent failure/zero-commit tests; improve diagnostics without changing hardware semantics. |
| Observations | Closed-system C++/RTL observations share a deterministic layout, but scalar slots remain capped at 64 bits. Wide and complete four-state observation transport is incomplete. | Verify equivalent events, identity, reset/discard behavior and value/known/Z transport from the same final IR. |
| Clocks and memory | Explicit physical clocks and standard memory leaves do not provide automatic scheduling, CDC or every historical disabled-read lifetime policy. | Test independent domains and memory timing explicitly; add no implicit scheduling semantics. |
| Host integration | Generated models use typed C++ DUT access and explicit clock/reset stimulation. A shared-library port C ABI and a turnkey interactive Python simulator are not provided. | Extend the existing runtime boundary only with an explicit, independently tested contract. |
| RTL four-state tools | Verilator is two-state. Some parameterized standard-leaf designs cannot yet run as complete designs in Icarus. | Keep native and applicable four-state RTL evidence separate; close whole-design tool gaps. |

## Coverage and engineering

| Area | Current limitation | Follow-up acceptance |
| --- | --- | --- |
| Historical scenarios | Expectation and credit pipelines, persistent schedulers, reset/trace/XZ module contracts have restored source fixtures and owning tests. Aggregate/array/configuration and remaining slot/rate/full-design contracts are incomplete. | Reconcile all 93 original roots against supported source, independent oracles and registered tests; a catalog mapping is not execution acceptance. |
| Complete designs | Circular ROB and oldest-ready issue queue restoration is in progress with independent full-state histories. Routed dependency pipeline remains unmapped. The smaller retained ROB does not establish their coverage. | Complete source admission, original algorithms and timing/recovery verification through the existing API/IR/backend path. |
| Candidate coverage | Historical accepted examples do not prove all source-import IR, transformed IR and backend outputs were regenerated from this PR candidate. | Run the existing nightly matrices and retain candidate-bound stage artifacts. |
| Portability and packaging | This cleanup does not establish a new full Linux/macOS/Windows release validation or performance baseline. | Run the existing platform/package acceptance workflows before publication. |
| Authoring experience | Generated systems use Python benches and the existing CMake source-unit flow. The original authored module verification remains registered alongside migrated system scenarios. Observation-only nested structural rules still reject without a binding or assertion. | Finish system adaptation and repair existing import/diagnostic inconsistencies without a new frontend or weakened oracle. |

See the [language reference](../reference/language.md) for exact supported
operations and [testing and gates](testing-and-gates.md) for the existing
validation tiers. This backlog does not authorize new language semantics or
claim that deferred tests passed.
