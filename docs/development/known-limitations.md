# Known limitations and follow-up work

This records pyCircuit 6.1 limitations and follow-up work.
Closed `@system` source compilation, generated C++/Verilator simulation, source
checks, observation publication and failure-atomic state/clock handling now have
focused independent verification. On 2026-10-18, all 17 then-current examples passed
the 34-test examples nightly; 54 superseded wrappers were preserved in
`tests/fixtures/historical_examples/`. The current catalog, scope and evidence
are recorded in the [example migration report](issue-307-319-examples.md).

Broader language coverage remains incomplete. The current remaining owners and
first failing stages are listed in the [execution status](issue-307-319-execution-status.md). The latest full API lit run has
161 passes and 8 failures; the full downstream package nightly did not run. The lit total combines the
69-case gate, the 99-case nightly selection and the separate multi-rule rerun. The API
gate passed 23/23, but that is not full API acceptance. Archiving an example is
not evidence that its functionality is implemented or currently tested. Earlier
long DMA/memory/Table benches remain preserved and were not revalidated by the
shorter current System scenarios. Full platform and wheel validation remain open.

## Language and compiler

| Area | Current limitation | Follow-up acceptance |
| --- | --- | --- |
| Source consistency | Structural leaf binding and inferred behavioral storage share the compiler but still admit different rule statements and forward references. Some errors expose internal profile terms. | Improve the existing source/MLIR owners so equivalent hardware receives consistent admission and actionable diagnostics. |
| Modules and imports | Canonical fixed-bit imports and explicit all-field imported Struct expression constructors are admitted through published interfaces. Imported defaults, static initializers, Table-query constructor callbacks, relative imports, general type aliases and arbitrary cross-source rule bodies remain restricted. Module outputs need explicit type information. | Preserve canonical nominal authority and failed-publication protection when extending admission. |
| Parameters | Standard-leaf type arguments and keyword defaults do not establish general parameter-dependent elaboration or emitted module families. | Verify nondefault widths, nested configurations and instance isolation through the same compiler. |
| Collections | Positive closed one-dimensional Tables support Struct fields, contextual literals/defaults, lambda map, same-source fixed scalar named map callbacks, seven fixed-bit folds and one-bit all/any/count. Table-of-Table source indexing, indexed partial writes within Table-valued fields, aggregate named callbacks, helper calls inside expression lambdas and general collection callbacks remain incomplete. Ordered prefixes and recursive record-element updates have focused examples using existing bounded loops and immutable values. | Improve existing type/effect/lowering support and verify independent C++/RTL results without recipe recognition. |
| Multiple writers | Disjoint local fields/elements, unordered cross-hierarchy field/index lanes and proved grants are supported; unproved overlaps reject. The ancestor lane is domain-composed before routing, and both proof layers re-derive it. Independent cross-hierarchy registration ordering, mixed ordered/unordered groups and cross-domain atomic updates are not admitted. | Validate same-Rule sequential calls and independent current-state reads, including partial-write composition and whole-system zero commit. Retired `after` forwarding is not a remaining capability to restore. |
| Integer arithmetic | Mathematical Integer runtime division/remainder, dynamic or signed shifts, dynamic range checks and some equivalent symbolic-width normalization remain restricted. Fixed-bit division has a separate supported contract. | Prove value/range behavior, X/Z propagation and failed-publication protection for each extension. |
| Control flow | Pure helpers retain their bounded literal-range profile. Typed Rule/Method loops admit proven nonnegative closed-Integer bounds, supported state effects and bounded control flow; finite while loops need the documented progress proof. General nesting, dynamic bounds, early branch returns, arbitrary callback effects and Python execution remain restricted. | Add only constructs with finite hardware meaning; preserve source ordering and four-state selection. |
| Method local aggregates | Rule/Method branches join immutable nominal Enum, finite acyclic Structs and admitted fixed Table payloads, including fixed one-dimensional Table fields. Local field/subtree reconstruction preserves snapshots. Independent partial writers require disjoint footprints or mutually exclusive guards; same-Rule calls retain sequential candidates. General nested indexing, slice/augmented targets and resource combinations remain restricted. | Verify the exact candidate through public source and generated System C++/RTL execution; native implementation tests alone do not establish closure. |
| Analysis capacity | The fixed source-check limit is unchanged. Current ROB, Table, DMA and memory-bank examples now pass after generic proof/IR improvements and source simplification. Some historical transaction/API workloads still fail; shorter teaching Systems do not validate their old long plans. The inherited arbitrary-precision evaluator capacity policy remains OPEN. | Address demonstrated repeated proof/layout work at its owner, preserving exact identity, bindings and independent proof; no fixture-specific admission or higher cap. |
| Compiler size and diagnostics | Source import still concentrates substantial behavior in `PythonImportRecords.cpp`; source resolution and semantic inference need clearer ownership. | Move semantic analyses into existing MLIR passes, reduce duplicate checks and provide source-level corrective messages. |

| public/private | Autonomous Rule entries are compiler-private; cross-Module calls use admitted Method/reference interfaces. A general author public/private modifier API is not provided. | Preserve private-entry rejection, lexical boundaries and independent compile/link authority. |

## Runtime and backends

| Area | Current limitation | Follow-up acceptance |
| --- | --- | --- |
| System authoring | All 16 registered Systems in the 2026-10-18 set of 17 examples passed that run. Historical host-only actions and broader source forms remain outside that evidence. | Preserve the original independent Module oracles and state explicitly which System trajectory was executed. |
| Queue protocols | Original route/credit/reorder histories and source faults now execute through managed module verification, with independent protocol checkers. Historical direct batch preloads and exceptional host partial commits are not generally expressible by closed default-clock systems. | Preserve token conservation, timing, capacity and backpressure; distinguish supported system trajectories from historical host-only actions. |
| Source checks | Closed systems and checked ordinary modules execute assertions through both backends. Checked module RTL requires the documented managed phase/commit/error caller boundary. RTL failures do not yet include the native structured check location/ID. | Retain full original vectors, sticky failure and independent failure/zero-commit tests; improve diagnostics without changing hardware semantics. |
| Observations | Closed-system C++/RTL observations share a deterministic layout, but scalar slots remain capped at 64 bits. Wide and complete four-state observation transport is incomplete. | Verify equivalent events, identity, reset/discard behavior and value/known/Z transport from the same final IR. |
| Clocks and memory | Explicit physical clocks and standard memory leaves do not provide automatic scheduling, CDC or every historical disabled-read lifetime policy. Trusted canonical storage and memory primitives support complete Module-scope conformance bindings; this does not admit arbitrary callable primitives or Rule-body pin wiring. | Test independent domains and memory timing explicitly; add no implicit scheduling semantics. |
| Host integration | Generated models use typed C++ DUT access and explicit clock/reset stimulation. A shared-library port C ABI and a turnkey interactive Python simulator are not provided. | Extend the existing runtime boundary only with an explicit, independently tested contract. |
| RTL four-state tools | Verilator is two-state. Some parameterized standard-leaf designs cannot yet run as complete designs in Icarus. The depth-32 packed-record scaling probe also exceeded its Icarus elaboration resource bound; native and Verilator checks passed for that case. | Keep native and applicable four-state RTL evidence separate; close whole-design tool gaps. |

## Coverage and engineering

| Area | Current limitation | Follow-up acceptance |
| --- | --- | --- |
| Historical scenarios | Expectation and credit pipelines, persistent schedulers, reset/trace/XZ module contracts have restored source fixtures and owning tests. Aggregate/array/configuration and remaining slot/rate/full-design contracts are incomplete. | Reconcile the current catalog against supported source, independent oracles and registered tests; removed references and catalog mappings are not execution acceptance. |
| Complete designs | Circular ROB, oldest-ready issue queue and routed dependency pipeline have mapped source fixtures and independent histories. Historical host-only, clock-control and four-state gaps remain; mapping does not establish full-candidate acceptance. | Complete source admission, original algorithms and timing/recovery verification through the existing API/IR/backend path. |
| Candidate coverage | The 2026-10-18 set of 17 examples and 14 Module receipts has candidate-bound execution evidence, predating the sibling-calls example. The 54 archived wrappers and older long benches are not covered by that pass; that complete API lit matrix had 8 failures. | Preserve separate current, historical and NOT RUN evidence through existing manifests. |
| Portability and packaging | This cleanup does not establish a new full Linux/macOS/Windows release validation or performance baseline. | Run the existing platform/package acceptance workflows before publication. |
| Authoring experience | Generated Systems use Python benches and the existing CMake source-unit flow. Short DMA/Memory/Table benches coexist with original independent Module verification. Supported partial writes and Queue transactions have focused evidence; general Table-containing branch reconstruction and broader resource combinations remain incomplete. | Follow the current language reference and retain generated System execution and failure-publication evidence through the existing flow. |

See the [language reference](../reference/language.md) for exact supported
operations and [testing and gates](testing-and-gates.md) for the existing
validation tiers. This backlog does not authorize new language semantics or
claim that deferred tests passed.
