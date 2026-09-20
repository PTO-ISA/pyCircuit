# AC architecture-rule, RTL codegen, and verification extension checklist

**Status:** Draft architecture requirements and implementation checklist

**Applies to:** pyCircuit 6 Agentic Circuit, ACIR, QueueGraph, PYC, C++
simulation, Verilog emission, primitive libraries, and semantic gates

**Related designs:**

- [Architecture Rule Compiler extension](architecture-rule-compiler-extension.md)
- [AC rule and SimQueue atomic lowering checklist](ac-rule-simqueue-atomic-lowering-checklist.md)
- [AC C++ pointer-owned module composition](ac-cpp-pointer-owned-module-composition.md)

The Architecture Rule Compiler RFC owns the product direction and generic
requirements. This document is its executable implementation and backend
verification supplement; it does not define a parallel architecture.

## Required outcome

pyCircuit must compile architecture intent, not merely translate Python syntax
to local RTL. A source-linked rule must become one closed chain of evidence:

```text
Python rule and NDF intent
          |
          v
normalized ACIR rule
          |
          v
whole-design effect/conflict/recovery graph
          |
          v
proved, runtime-checked, or rejected obligations
          |
          v
atomic transaction and resource refinement
          |
          v
PYC cycle/logic semantics
          |
     +----+----+
     |         |
 C++ model   Verilog + SVA
     |         |
     +----+----+
          |
cycle parity, zero-loss, stress, and reviewable evidence
```

The compiler, not a backend or consumer-specific adapter, owns all semantic
decisions in this chain. C++ and Verilog are two projections of the same
verified IR contract.

This document turns the supplied RTL/codegen/verification rules and the
Architecture Rule Compiler proposal into bounded work packages for later
agents. It does not make a complete CPU, NPU, ROB, IQ, rename unit, or LSU part
of pyCircuit.

## Normative interpretation of the source material

Later rules supersede earlier exploratory examples when they conflict.
In particular:

- stall holds a DFFE by deasserting enable; generated recirculation muxes are
  forbidden unless the value must genuinely change while stalled;
- an SRAM read output is a stateful resource with a bounded lifetime, not an
  ordinary combinational wire;
- when SRAM output must survive later port reuse, always-capture plus a
  live/registered selection is the preferred pattern;
- the capture cycle must account for nonblocking-assignment visibility;
- one-hot AND-OR selection is legal only with proof and a matching obligation;
  a real priority relation remains a priority mux;
- aggressive X behavior is a verification mode, not an excuse to change
  synthesizable architectural semantics; and
- full consumer designs remain in their owning repositories. pyCircuit keeps
  only vendor-neutral reduced fixtures.

## Evidence-backed baseline and gap analysis

The labels in this section are deliberate:

- **Evidence** is directly present in the current repository.
- **Inference** is the architectural consequence of that evidence.
- **Unknown** requires a decision or a focused experiment before implementation.

| Area | Current evidence | Gap and required direction |
| --- | --- | --- |
| Rule summaries | `LowerRules.cpp` derives typed Queue effects, checks, output presence, state accesses, activation sources, transaction resources, and arbitration membership. | **Inference:** the local rule contract is a valid foundation, but it is not a persisted whole-design effect graph. |
| State footprints | `ACDataFlowAnalyzer::stateFootprints` retains endpoint, owner, SSA index, field set, and presence value. `inferRuleEffects` serializes owner, index kind, fields, and guard kind. | **Evidence:** the serialized summary drops the actual normalized index/predicate expression. Cross-rule graph construction therefore cannot consume an exact portable footprint yet. |
| Writer conflicts | `VerifyValueConstraints.cpp` proves disjoint fields, disjoint indices, mutually exclusive predicates, explicit priority, and cross-owner priority acyclicity. | **Inference:** do not replace this logic. Refactor it behind a reusable conflict graph and emit its proof/result as inspectable data. |
| Obligations | ACIR has transient `ac.marker.obligation` with pending/materialized/discharged states and named handshake/check/schedule resolvers. | **Evidence:** dynamic check obligations are rejected; the marker has no stable obligation ID, kind, severity, runtime policy, or backend lowering. It is not the proposed Architecture Obligation IR. |
| Rule closure | `ac-verify-rule-closure` rejects unresolved markers and validates typed lowered-rule evidence. | **Inference:** closure should additionally require every architecture obligation to be `proved` or `runtime_checked`; no obligation may silently disappear. |
| Atomic Queue execution | `QueueAtomicTransform` checks all inputs/outputs, prepares one commit group, publishes outputs/inputs, cancels on preparation failure, and reports publish failure. | **Evidence:** all-or-nothing Queue execution exists for the admitted subset. Conditional outputs, multi-resource groups, wide payload lifetime, and graph-derived debug reasons still need full closure under the related atomic-lowering checklist. |
| QueueGraph artifact | `QueueGraphPlan` carries firing stable IDs, provenance, activation resources, transaction resources, output presence, state reservations/writes, and emits deterministic JSON. | **Inference:** extend this typed handoff with obligations, blockers, identity/recovery metadata, and transaction-group policy instead of re-parsing source in either backend. |
| PYC lowering | ACIR QueueGraph lowering already emits typed PYC expressions and state updates, and current gates exercise Table and Queue subsets in C++ and Verilog. | **Inference:** Architecture Rule extensions must lower through PYC or a verified backend-neutral plan. Backend-only semantic branches are forbidden. |
| Verilog structure | `VerilogEmitter.cpp` emits explicit-width integer literals, named submodule connections, deterministic net sorting, and canonical sequential primitive instances. | **Evidence:** it does not consume architecture obligations, does not emit obligation-linked SVA, and does not enforce every requested textual/structural audit as a fail-closed gate. |
| Register primitive | `pyc_reg.v` and the C++ `pyc_reg` update only on reset or enable. | **Evidence:** natural stall hold already exists. A pass must prevent upstream lowering from manufacturing `stall ? q : d` plus `en = transfer \| stall`. |
| SRAM primitive | Verilog sync memories and C++ sync memories reset/read out zero and value-initialize storage to zero. | **Evidence:** this intentionally deterministic behavior does not expose stale-Q or uninitialized-state bugs. Aggressive X verification requires an explicit mode and a C++/RTL parity policy. |
| SVA | The testbench DSL emits user-authored SVA, currently sampled at the negative edge by default. `pyc.assert` also emits simulation-only checks. | **Evidence:** there is no compiler-generated SVA from rule obligations, and one global sampling edge cannot represent every signal lifetime. |
| X/Z | Trace contracts preserve value/known/Z masks and semantic gates cover selected X/Z behavior. The normal generated C++ `Bits` runtime is two-state. | **Decision 0273:** comparison requires exact known/Z-mask parity plus equal value on commonly known bits; the affected C++ path must preserve all three masks. |
| Verification gates | The repository has focused MLIR/native tests, AC G0/G1/G2, C++/Verilator simulation lanes, semantic regressions, evidence directories, and decision-status closure. | **Inference:** extend these existing lanes with obligation, zero-loss, rule coverage, why-not-fire, deterministic rebuild, and representative RTL-diff artifacts; do not create a parallel release system. |

## Scope and hard boundaries

### In scope

- whole-design rule effect, conflict, ordering, ownership, and recovery analysis;
- stable Architecture Obligation IR and proof/runtime/reject lifecycle;
- C++ runtime checks and Verilog SVA generated from the same obligation;
- generic transaction identity, recovery, multi-lane, reservation, dependency,
  checkpoint, retained-result, and terminal-transaction semantics;
- verified lowering to QueueGraph/PYC;
- structural Verilog and primitive contracts, including SRAM lifetime and X
  verification modes;
- rule/blocker/obligation coverage and deterministic evidence indexes; and
- reduced generic fixtures that prove framework behavior.

### Out of scope

- public `ac.rob`, `ac.iq`, `ac.rename`, `ac.lsu`, or other product-specific
  primitives;
- a complete consumer Core, NPU, SoC, ISA decoder, or model-comparison harness;
- automatically choosing architecture sizes such as ROB depth or issue width;
- making Markdown the semantic authority;
- repairing semantics inside only C++ or only Verilog emission;
- deriving any user-visible identity or semantic contract from opaque
  content-derived tokens or a frozen content ledger; and
- accepting a whole-core AC artifact and splitting it after compilation.

## Target compiler architecture

### Normalized rule effect model

Introduce one backend-neutral `RuleEffectSummary` for every executable rule.
It must contain typed references, not human-readable strings that a backend
must reinterpret:

```text
RuleEffectSummary
  rule_id
  source_provenance
  ndf_ids[]
  activation_sources[]
  consumed_transactions[]
  produced_transactions[]
  state_reads[]
  state_writes[]
  resource_demands[]
  transaction_policy
  recovery_domain?
  ordering_edges[]
  arbitration_domains[]
  side_effect_class
```

Each state footprint must preserve:

```text
owner
normalized index expression or all-entries marker
field set or whole-entry marker
access kind
path predicate
source operation
```

The normalized index and predicate must be a persisted typed expression DAG
referenced by ID. F1 may change the current serialized summary representation;
that representation is incomplete and is not a compatibility format. Text
printed from MLIR is diagnostic output, not a semantic encoding.
Every node has a closed opcode, exact result type, ordered operand references,
and closed typed attributes. Leaves are limited to rule-input ordinals,
committed owner/field/index identities, typed constants or static parameters,
and admitted lane identities. Serialization ordinals are local handles, not
semantic IDs. Verification independently normalizes the live body; it does not
validate a summary by round-tripping the summary itself.

### Whole-design Rule Effect Graph

Add a `BuildRuleEffectGraphPass` after local rule normalization and before
schedule/arbitration resolution.

Required graph edges:

- Rule -> Queue consume/produce;
- Rule -> state read/write footprint;
- Rule -> resource demand;
- Rule -> recovery domain;
- Rule -> Rule read/write or write/write conflict;
- Rule -> Rule ordering constraint;
- Rule -> arbitration domain; and
- Rule -> obligation.

The pass must reuse the existing value-constraint and writer-arbitration proofs.
It must not implement a second, weaker overlap algorithm.

Required outputs:

- deterministic in-memory analysis;
- deterministic `.json` debug artifact;
- optional deterministic `.dot` view;
- stable diagnostics naming rule IDs, owners, indices, fields, and predicates;
  and
- no opaque content-derived identity.

### Architecture Obligation IR

The current `ac.marker.obligation` remains only a transient rule-handshake
lowering marker and is eliminated before backend emission. It is not reused,
extended, or replaced by the architecture-obligation lifecycle.

Add exactly one module-owned `ac.arch_obligation` symbol operation. Its typed
condition references nodes in a module-owned typed expression table. Each
symbol has at least:

```text
id                  explicit stable semantic ID from declared structural name;
                    never content, source line, traversal, or process counter
kind                closed enum
severity            error | fatal
condition           typed i1/PYC expression or structured relation
source_rule_ids[]
state_owners[]
proof_status        pending | proved | runtime_checked | rejected
proof_evidence?     typed reference to a compiler proof result
runtime_targets[]   cpp | gfsim | sva
sampling_contract   tagged union: tick_observation | xfer_observation |
                    pre_publish | producer_event
message             diagnostic text
source_provenance
ndf_ids[]
```

Phase-one safety kinds:

- `mutual_exclusion`;
- `single_writer`;
- `resource_capacity`;
- `ready_valid_integrity`;
- `transaction_atomicity`;
- `generation_match`;
- `epoch_match`;
- `ordering`;
- `range`;
- `onehot0`;
- `no_partial_commit`;
- `no_stale_update`;
- `credit_balance`; and
- `pipeline_alignment`.

This list is the complete closed phase-one `kind` enum. The closed
`runtime_targets` enum is `cpp | gfsim | sva`. The closed phase-one `severity`
enum is exactly `error | fatal`; there is no warning/info severity. Both make
the run and gate unsuccessful and cannot relax admission. `fatal` emits the
structured obligation ID/source diagnostic and terminates immediately before
the guarded mutation/publication. `error` emits the same diagnostic,
suppresses the guarded mutation/publication, marks the run failed, and may
continue only for bounded diagnostic collection; it can never later report a
passing run.

Liveness obligations such as progress and eventual completion must remain
rejected or explicitly deferred until temporal semantics are defined. They
must not be silently treated as one-cycle safety checks.

Lifecycle:

```text
InferArchitectureObligations
             |
             v
ProveArchitectureObligations
       +-----+----------------+
       |                      |
     proved        cannot statically prove
                              |
                              v
              MaterializeRuntimeObligations
                   +----------+----------+
                   |                     |
            runtime_checked           rejected
```

Every exit is explicit. `ac-verify-rule-closure` must fail if any live
obligation is pending, has an unsupported runtime target, or has no stable ID.
Runtime materialization is safety-only. It cannot legalize unresolved writer
overlap, authorize one-hot AND-OR selection, replace a required static proof,
or be disabled when admission depends on it. A synthesis/deployment profile
must prove every property on which synthesized structure or legality depends.
Mandatory release C++/gfsim checks survive `NDEBUG` and execute before the
affected mutation or publication. A transform that changes the condition
invalidates the prior proof and recomputes the condition, proof, sampling
contract, and runtime materialization.

Sampling-union rules are exact:

- `tick_observation` maps to TICK-OBS and `xfer_observation` to XFER-OBS;
- `pre_publish` requires a rule/firing anchor immediately before publication;
- `producer_event` requires an explicit producer-event anchor;
- edge is `posedge | negedge | none`; tick/xfer/pre-publish require `none`,
  while only producer-event may choose a physical edge;
- tick/xfer forbid `sample_anchor`; pre-publish and producer-event require the
  matching anchor kind;
- active and reset/recovery-disable predicates are sampled at the same event;
  disable is a synchronous sampled skip, not implicit async `disable iff`;
- capture latency is in cycles, is optional only for monitor-only
  producer-event capture, and never affects admission or mutation; and
- arms are mutually exclusive; active-edge-only values without a producer
  anchor reject.

### Recovery and identity

Start as internal compiler objects. Public Python APIs are admitted only after
their semantics have focused negative tests.

Generic contracts:

- `RecoveryDomain`: names a speculative lifetime and its epoch type;
- `TransactionRef`: slot + generation + recovery epoch;
- `TypedIdentity`: nominal identity with optional parent relation;
- `ExecutionAttempt`: transaction identity plus replay/attempt number;
- `RecoveryEvent`: epoch, checkpoint, boundary, and cause;
- `KillSet`: a typed invalidation predicate over identity/recovery data;
- `VersionedTable`: valid + generation + epoch + payload with qualified lookup;
- `Checkpoint`: capture/restore/release semantics independent of physical
  snapshot implementation; and
- `RetainedResult`: accepts once, holds until consumed, remains identity- and
  recovery-qualified.

Old/new state rule:

- all rule evaluation reads committed state at tick start;
- accepted proposals become visible at the Xfer boundary;
- same-cycle forwarding requires an explicit typed relation; and
- absent that relation, same-cycle persistent-state reads observe the committed
  old-state snapshot;
- Python statement order never defines forwarding or writer precedence.

### Multi-lane transaction and resource algebra

Add an internal `TransactionGroup<N>` with:

- valid mask;
- lane identity;
- `all_or_none`, `valid_prefix`, or `independent` commit policy;
- lane ordering;
- per-lane resource demand;
- accepted mask; and
- one group-level commit contract.

Add generic internal objects:

- `ReservationSet` for atomic preview/reserve/commit across resources;
- `MultiAllocator` with allocate/free counts, generation policy, and explicit
  same-cycle reuse policy;
- `AgeSelectK` with legal candidate set, ordering policy, winner count, and
  resource-class compatibility;
- `DependencySet` with identity-qualified add/resolve/kill/is-ready; and
- `TerminalTransaction` defining when a split operation is architecturally
  complete.

The first implementation must support one fixed-width 4-lane reduced fixture.
Eight- and ten-lane support are later scale gates, not phase-one API promises.

### Memory ordering and refinement

Add a backend-neutral `MemoryOrderingGraph` only after identity/recovery and
multi-lane resource semantics are closed.

Minimum edge kinds:

- `older_than`;
- `must_wait`;
- `may_bypass`;
- `must_forward`;
- `must_replay_if`; and
- `visibility_before`.

Do not encode an ISA memory consistency model in the first version. The first
fixture proves Core-local store/load dependency, non-alias discharge, alias
forwarding, late violation, replay identity, and stale-response rejection.

Refinement separates semantics from physical realization:

```text
semantic primitive
    + parameters and legality
             |
             v
implementation catalog
    + supported parameter ranges
    + latency and initiation interval
    + port/bank shape
    + estimated area/depth class
    + required obligations
             |
             v
validated PYC/RTL implementation
```

Selection is fail closed and deterministic. A selected implementation must
preserve the semantic primitive's cycle and transaction contract.

## Required pass pipeline

The exact pass names may change during review, but the dependency order is
normative:

```text
Verify ACIR types and ownership
Normalize rules and value contracts
Infer local rule effects
Canonicalize live state reads and predicates
Build whole-design Rule Effect Graph
Infer transaction identity and recovery domains
Infer resource footprints and ordering edges
Infer conflicts
Resolve writer arbitration
Infer architecture obligations
Prove architecture obligations
Materialize runtime obligations
Resolve multi-lane transactions and reservations
Verify atomic committed-state snapshot semantics
Select semantic refinements and CBB implementations
Verify ACIR closure
Lower verified ACIR to QueueGraph/PYC
Run PYC cycle balance, pipeline alignment, and logic-cost checks
Select validated RTL primitives
Verify backend closure
Emit C++ and Verilog
```

Pass invariants:

- no backend sees a pending obligation;
- no backend invents a priority, recovery rule, Queue check, state forwarding,
  or sampling edge;
- every graph/refinement result is deterministic under source reorder that is
  semantically irrelevant;
- diagnostics cite source and NDF provenance;
- no pass manufactures identity from content; and
- every lowering either preserves obligation IDs or records an explicit
  proved-elision reason.

## QueueGraph and C++ codegen extensions

### QueueGraphPlan

Extend the plan with typed records for:

- normalized rule footprints;
- conflict and ordering edges;
- transaction-group policy and accepted mask expression;
- resource preview/reservation/commit actions;
- recovery domain and identity qualification;
- obligation ID, status, condition, sampling contract, and runtime targets;
- why-not-fire blockers; and
- rule/obligation coverage counters.

The JSON form is a deterministic review/debug artifact. It is not a frozen
contract, cache key, identity source, or release authority.

### Generated C++

Generated C++ must:

- keep one source group per source-owned `.ac` unit;
- use the pointer-owned module and Queue model in the parent design;
- execute every atomic firing through preflight, prepare, publish, and Xfer;
- emit runtime checks only for obligations marked `runtime_checked` with `cpp`
  or `gfsim` target;
- keep mandatory release checks active under `NDEBUG` and execute them before
  the mutation/publication whose admission they guard;
- use the exact obligation ID and source/NDF mapping used by SVA;
- report rule ID, blocker ID, resource path, and transaction identity on
  failure;
- expose why-not-fire data without changing scheduling behavior;
- count firing, blocked reason, arbitration winner, stale rejection, recovery,
  and atomic-failure coverage;
- preserve immutable shared-pointer movement for payloads wider than 64 bits;
  and
- keep architecture state two-phase at TICK-OBS/XFER-OBS.

C++ runtime assertions are not permission to accept undefined behavior. If the
runtime cannot evaluate an obligation with the same semantics as PYC, the
compiler rejects that target.
Runtime assertions also cannot legalize overlapping writers or a one-hot
optimization, and an assertion used for runtime admission cannot be disabled.

### Why-not-fire contract

Each firing condition is decomposed into stable blocker terms:

- required input available;
- functional guard true;
- recovery/identity valid;
- resource capacity available;
- output capacity available;
- arbitration winner; and
- ordering/dependency satisfied.

Blocker evaluation must be side-effect free. Enabling explanation or coverage
must not change rule order, arbitration, Queue reservations, or performance
semantics.

## PYC and Verilog emission contract

### PYC remains the semantic hardware boundary

Architecture extensions must lower to ordinary verified PYC plus explicit
obligation/refinement metadata. Public Python and canonical PYC remain
technology independent. `pyc.rtl.*` is introduced only by the existing
fail-closed primitive-selection mechanism.

Before emission, verify:

- exact widths and signedness;
- no residual `ac.var`, `scf.*`, or `index` values;
- cycle balance and data/valid/identity alignment;
- no illegal combinational cycle;
- no unsupported dynamic memory or writer profile;
- every one-hot implementation has a statically proved `onehot0` obligation;
- every true priority relation remains ordered; and
- every stateful primitive has explicit reset/invalidate/hold semantics.
- every parameterized source definition maps to one readable RTL module family
  with typed static parameters and admitted generate branches; unsupported
  family shapes reject before emission.

### Verilog text and structure

The emitter and post-emit gate must enforce:

- ASCII-only output, including comments;
- one declaration or connection per line where practical for review;
- explicit-width constants for hardware values;
- named intermediate signals for every submodule/primitive connection;
- no expression directly inside a port connection;
- deterministic declaration, instance, connection, assertion, and module
  ordering;
- readable lower-snake signal leaves with source-map comments;
- no double underscore in generated source-facing identifiers;
- no opaque content-derived suffix;
- no silent identifier truncation; and
- stable output under two clean emissions from the same input/toolchain.

The current named connections and explicit-width literal helpers are retained.
The missing rules become emitter invariants or a fail-closed structural audit,
not a formatting convention that can be ignored.

### Selection and width rules

- Emit a priority mux only for a semantic priority relation.
- Emit a balanced AND-OR mux only for statically proved one-hot/onehot0
  selection. Runtime checks may accompany the proof as diagnostics but never
  authorize the optimization or synthesis/deployment legality.
- Perform bit-use analysis before widening, concatenating, comparing, or
  storing values.
- Remove a dead path through its complete dependency chain, including select,
  decode, mux, pipeline state, and unused primitive input.
- Prefer `and`, `or`, `xor`, `not`, `add`, `sub`, shifts, compare, mux,
  extract, concat, and exact extension/truncation.
- Apply verified strength reduction for constant multiply/divide/remainder.
- Reject accidental expensive dynamic arithmetic in control/address logic.

### Ready/valid and pipeline rules

- `ready && valid` is the only transfer event unless a typed protocol says
  otherwise.
- A blocked output cannot consume any input or commit paired state.
- Every ready/valid boundary is audited against stall, conflict, flush, route,
  and downstream capacity.
- Shared control may drive stage validity, but independent resource owners may
  complete independently.
- An access that waits in a stage must not repeat an SRAM read/write or other
  non-idempotent effect.
- Stall of an enabled register uses `en = 0` and natural Q hold.
- A recirculation mux is allowed only when a typed override says the value must
  change during stall.
- An output whose invalid value is architecturally zero/none must be explicitly
  cleared or masked; it may not expose a stale previously valid payload.

### SRAM and stateful primitive rules

Treat each SRAM read result as a resource with provenance and lifetime:

- request cycle and address/bank/way provenance;
- physical Q availability after clock-to-Q;
- live-use window;
- capture event;
- registered-use window; and
- invalidation/overwrite event.

When Q can be overwritten before its final use, lower to:

```text
read request
   -> Q becomes live after edge
   -> always-capture register at the defined capture edge
   -> use live Q in the capture cycle when NBA visibility requires it
   -> use captured Q afterward
```

Do not generate `en = transfer | stall` plus a feedback mux. Capture enable is
the true capture event.

The verification library needs the mandatory aggressive SRAM profile from
Decision 0273 with static live-window parameter `N=1`; this `N` is not memory
depth and the bounded Table contract remains separate:

- Q begins unknown;
- every value satisfies `(known & z) == 0`;
- unknown control or an active assertion condition fails immediately;
- enabled address must be known;
- write data and write strobes must be known;
- an enabled read makes Q live after its capture edge for exactly one live-use
  cycle; the next edge returns Q to unknown unless another enabled read creates
  a new live value;
- read-during-write behavior is explicit; and
- inactive sub-units may carry X only when their applicability predicate is
  known false; and
- unsupported four-state operations reject rather than coerce to two-state.

Synthesizable primitives remain technology independent. Aggressive X behavior
is isolated under simulation/formal guards and selected by a documented test
profile.

### Obligation-to-SVA lowering

Add an emitter stage that consumes verified obligation records and emits:

- stable assertion labels derived from the explicit obligation ID;
- an explicit typed clock/domain, observation edge or point, active-domain
  predicate, reset/recovery disable predicate, and value-lifetime contract;
- reset/recovery disable conditions;
- active-domain gating for inactive sub-units;
- source/NDF comments;
- `$onehot0`, no-X, ready/valid, no-partial-commit, stale-update, range,
  alignment, and credit assertions for the admitted phase-one kinds; and
- `ifndef SYNTHESIS` isolation for simulation-only checks unless a formal flow
  explicitly requests bindable properties.

Negative-edge sampling is a supported sampling contract, not a universal
default. A value that is valid only in the positive-edge active region must
declare that lifetime and use a race-free monitor strategy.

## Verification methodology

### Verification pyramid

Every semantic feature uses the smallest applicable layers, in order:

1. dialect/parser/verifier positive and negative tests;
2. pass-level MLIR/FileCheck tests;
3. Rule Effect Graph and obligation golden tests;
4. QueueGraphPlan structural tests;
5. C++ runtime unit tests;
6. emitted Verilog structural/SVA tests;
7. C++ versus Verilator TICK-OBS/XFER-OBS parity;
8. deterministic fixed-seed stress; and
9. reduced integration fixture and release-gate evidence.

A broad simulation run does not replace a missing verifier negative test.

### Transaction-first checking

Ready/valid tests compare accepted transactions, not just signal pulses.
Each test records:

- offered transaction count and IDs;
- accepted transaction count and IDs;
- committed output/state effects;
- killed/replayed/stale-rejected transactions;
- final Queue/resource occupancy; and
- terminal transaction completion.

Required end conditions:

- no loss;
- no duplicate commit;
- no partial commit;
- no stale update;
- no leaked credit/reservation/reference;
- no unexpected in-flight transaction; and
- expected ordering preserved.

### Directed matrix

Every changed rule/codegen path includes applicable cases for:

- empty/full boundaries;
- simultaneous input/output transfer;
- downstream stall;
- conflict and arbitration loss;
- flush/recovery during wait;
- repeated slot reuse with generation/epoch change;
- old completion after reuse;
- optional output absent/present with full destination;
- same-cycle allocate/free/complete/retire policy;
- multi-lane prefix and independent acceptance;
- duplicate access prevention;
- inactive sub-unit X state;
- SRAM Q overwrite before delayed use;
- capture-cycle NBA visibility;
- stale output invalidation; and
- terminal completion after split effects.

### Stress and continuous invariants

Stress uses deterministic seeds and records the seed in evidence. It mixes:

- random valid gaps;
- random downstream stalls;
- resource exhaustion;
- writer conflicts;
- recovery/flush;
- replay and stale response;
- same-cycle allocate/free;
- long Queue occupancy; and
- variable-latency retained results.

Check invariants every cycle, not only at test end:

- occupancy and credit bounds;
- conservation of accepted transactions;
- onehot0 resource users;
- no X address/data/strobe on enabled SRAM access;
- stage data/valid/identity alignment;
- no commit from a killed identity;
- generation/epoch qualification;
- allocator disjointness and count transition;
- no repeated non-idempotent access while stalled; and
- C++/RTL observation equivalence where values are defined.

### X/Z and parity policy

Decision 0273 selects one exact contract: each observation carries equal-width
`value`, `known`, and `z` masks; C++ and RTL must have identical known and Z
masks, and value bits must match wherever the common known mask is one. Value
bits under X or Z are ignored only after mask equality succeeds, and every
observation satisfies `(known & z) == 0`. Treating C++
zero as equal to RTL X/Z is forbidden.

### Testbench timing

- Drive inputs early enough to avoid active-region races.
- Separate drive and sample phases explicitly, using a clocking block or a
  documented delay where appropriate.
- Deassert burst valid before the next sampling edge; prevent an unintended
  N+1 transaction.
- Sample each signal according to its declared lifetime and observation point.
- Isolate tests: reset architectural state and temporary queues, and explicitly
  initialize any state intentionally retained across reset.

### Evidence package

Every semantic/codegen change archives, under one gate run ID:

- exact commands and tool revisions;
- focused positive/negative test output;
- Rule Effect Graph JSON for the fixture;
- obligation report with status and backend targets;
- generated C++ and Verilog inventory;
- emitted SVA inventory;
- deterministic rebuild comparison;
- parity result and trace comparison summary;
- stress seed and transaction accounting;
- rule/blocker/obligation coverage summary;
- representative colored HTML generated-RTL diff for codegen changes; and
- known limitations and deferred obligation kinds.

The evidence index contains paths and explicit semantic IDs. It does not define
identity and does not contain opaque content-derived tokens.

## Implementation work packages

### P0 - Decision and schema freeze

**Goal:** accept the semantic model before implementation branches diverge.

- [x] Add Decisions 0271-0278 for exact effects, Architecture Obligation IR,
      old/new state, four-state/SRAM, pointer/naming contracts, and the closed
      finite module-family declaration, concrete case-body schema, and exact
      Python/ACIR/PYC carrier and exact dependent/PYC mapping micro-schema.
- [x] Freeze the closed F0 enums and product semantics; JSON/DOT remain
      deterministic debug/evidence views, not product identity or release ABI.
- [x] Define product contracts versus debug/evidence artifacts.
- [x] State explicitly that full consumer models remain outside pyCircuit.
- [x] Add this checklist and Decisions 0271-0278 to contributor routing.
- [ ] Implement the frozen expression/obligation schemas and verifiers (P1-P3).
- [ ] Implement deterministic graph/obligation debug serialization (P2-P3).

Static family emission remains blocked until Decisions 0275-0278's ordered
typed declarations, constraints, source-owned finite cases, concrete High ACIR
case regions, dependent signatures, complete header/link coverage, typed
`ModuleFamilyPlan`/`ModuleCasePlan`, exact typed ACIR AttrDefs,
container-only `ac.module`, verified PYC carrier and logical-to-physical map,
arbitrary-precision dependent arithmetic, exact type-expression bounds,
projection/layout/carrier records, shared-ready Queue mapping, explicit
implicit clock/reset origins, complete case signatures, and
acceptance/negative matrices are implemented. Recovery identity is not an F0
blocker. Its public/internal object decisions and
implementation belong to P8 and require a later decision.

**Exit:** accepted decisions resolve every item in Section 12 that blocks P1-P3.

### P0 hard-break deletion and negative-gate ledger

| Removed path/contract | Repository owner | Required negative gate/evidence |
| --- | --- | --- |
| legacy CLI | pyCircuit | API/contract scan rejects removed commands |
| legacy Python wrapper | pyCircuit | import and repository-absence negative |
| fallback CMake path | pyCircuit | build-graph test proves only direct per-source producers |
| whole-system frontend compile and post-compile split | pyCircuit | package/hierarchy negatives reject whole-core and post-split inputs |
| `root.ac` | pyCircuit | AC-tree inventory absence/rejection |
| monolithic `types.ac` | pyCircuit | source-owned interface-shard inventory |
| `shared/` compatibility tree | pyCircuit | release-layout absence gate |
| consumer whole-core artifact | consumer repository | consumer AC-tree/layout absence gate; exact legacy name remains in the F0 evidence ledger |
| flat interface and parallel `interface/modules/` trees | consumer repository | consumer layout gate |
| generic `assembly.py` implementation names | consumer repository | one-source/one-module naming gate |
| legacy module/package prefixes and opaque/specialization suffixes | pyCircuit | naming scan plus collision rejection diagnostics |
| dictionary family schemas and `specializations.json`-style sidecars | pyCircuit | API/repository absence scan plus typed family/case carrier negatives |
| content identity, contract epoch, or freeze-as-release-identity | pyCircuit | zero-identity scan; structural topology closure remains required and malformed topology still rejects |

The consumer-specific whole-core artifact deletion is recorded in the F0
evidence ledger, not as a framework product name. Every row requires reviewable
absence evidence and stable negative diagnostics before its implementation
phase closes.

### P1 - Persist exact local effects

**Primary files:**

- `compiler/acir/include/acir/Analysis/VariableAnalysis.h`
- `compiler/acir/lib/Analysis/VariableAnalysis.cpp`
- `compiler/acir/lib/Transforms/LowerRules.cpp`
- `compiler/acir/lib/Dialect/ACIR/ACIROps.cpp`

- [x] Define typed `RuleEffectSummary` storage.
- [x] Replace or extend the current incomplete serialized representation as
      needed; do not preserve it as a compatibility format.
- [x] Preserve normalized index and path-predicate references.
- [x] Preserve exact field/whole-entry access.
- [x] Preserve footprint endpoint source provenance while retaining
      `ac.ndf_ids`/`ac.ndf_requires` as the rule-level NDF authority through
      rule-to-firing print/parse; do not duplicate NDF IDs into every footprint.
- [x] Verify summary equals the live rule body.
- [x] Reject unrepresentable or mutable footprint expressions.
- [x] Add deterministic print/parse/golden tests.
- [x] Complete independent F1 re-review after the exact match/choose, Slot,
      owner/field, provenance, and deep-normalization review fixes land;
      code-reviewer and verifier verdicts are PASS.

F1 evidence is archived under
`docs/gates/logs/20260920-arch-rule-f1/summary.md`. Decision 0271 remains
`gap-in-scope` because P2 whole-design graph construction is a separate,
unfinished requirement of the same decision.

**Exit:** a three-rule Table fixture serializes exact A/B field-disjoint and A/C
overlapping footprints without relying on source order.

Covered by `rule-exact-effect-order.mlir`, which lowers and reparses both A/B/C
and C/B/A permutations with identical exact roots and owner/field/arbitration
conclusions.

### P2 - Whole-design effect graph

**Primary files:**

- new ACIR analysis/pass files under `compiler/acir/`
- `compiler/acir/include/acir/Transforms/Passes.td`
- existing writer/value-constraint analysis shared by the new pass

- [x] Build rule/resource/state/conflict/ordering/arbitration edges.
- [x] Reuse disjoint-index, mutually-exclusive-predicate, and field-disjoint
      proofs.
- [x] Reuse the existing writer-arbitration and value-constraint proof
      implementations rather than cloning their algorithms.
- [x] Preserve current cross-owner arbitration-cycle rejection.
- [x] Emit deterministic JSON and DOT debug forms.
- [x] Add `acir-opt` dump options without changing normal codegen output.
- [x] Add negative tests for unresolved overlap and ordering cycles.

F2 implementation evidence is archived under
`docs/gates/logs/20260920-arch-rule-f2/summary.md`. Recovery remains explicitly
absent until F7, and obligation linkage is a reserved absent graph slot until
F3; neither is inferred by F2.

- [x] Complete independent F2 code review: **PASS**.
- [x] Complete independent F2 verification review: **PASS**.
- [x] Verify the P2 exit condition: exact A/B coexistence and A/C
      proof/priority/rejection are explained without source-order semantics.

Decision 0271 and P2 are implemented and verified. F3 remains separate.

**Exit:** graph output explains why A+B may coexist and why A+C requires
arbitration or an obligation.

### P3 - Obligation IR and closure

**Primary files:**

- ACIR ODS definitions and verifiers
- new obligation inference/proof/materialization passes
- `LowerRules.cpp` and `verifyRuleClosure`
- QueueGraphPlan schema

- [x] Add stable explicit obligation IDs.
- [x] Add phase-one kind/severity/status/runtime-target enums.
- [x] Infer predicate-exclusive same-field writer obligations from the shared
      exact-footprint and writer-arbitration analysis.
- [x] Record predicate-exclusive writer proofs as typed evidence and
      independently recompute their exact endpoints, roots, and assumptions.
- [x] Convert closed proved records to non-executable QueueGraph elisions;
      direct proved plans and opaque certificate strings cannot authorize
      backend behavior.
- [x] Materialize the bounded F3 executable subset: scalar two-state `i1`
      range conditions, `pre_publish`, selected target `gfsim`.
- [x] Reject unsupported or pending obligations before QueueGraph/backend
      extraction, including custom/direct QueueGraph plan validation.
- [x] Verify the bounded gfsim P3 exit: proved obligations become
      non-executable typed elisions, admitted range monitors execute before
      publication, and unsupported runtime kinds/targets fail closed.
- [x] Complete independent F3 code review: **PASS**.
- [x] Complete independent F3 verification review: **PASS**.
- [ ] Preserve one ID through QueueGraph, C++, Verilog, trace, coverage, and
      diagnostics.
- [ ] Remove any fallback that silently discharges a dynamic check.

**Bounded F3 status (2026-09-20):** one predicate-exclusive same-field writer
obligation is statically proved; one explicit scalar range monitor carries its
ID through ACIR, the deterministic obligation report in QueueGraphPlan,
generated gfsim code, and the structured failure diagnostic; unsupported
target/sampling/pending cases reject. Field-disjoint writers remain coexistence
and never become mutual-exclusion evidence. C++ and SVA pairing, trace and
coverage linkage, and the original full cross-backend P3 exit remain F5 work,
so Decision 0272 stays `implemented-unverified` despite the independently
verified bounded gfsim slice.

### P4 - C++ checks, why-not-fire, and coverage

**Primary files:**

- `compiler/acir/lib/CodeGen/QueueGraphPlan.cpp`
- `compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp`
- `simulator/gfsim/`

- [x] Carry obligation records in QueueGraphPlan; blocker taxonomy remains a
      later diagnostic extension.
- [ ] Generate side-effect-free blocker evaluation.
- [x] Generate matched C++/gfsim runtime checks.
- [ ] Add rule firing, blocker, conflict, recovery, and stale-rejection counters.
- [x] Emit structured failure records with stable obligation/source/module
      identity before publication.
- [x] Preserve functional behavior with obligation counters, trace probes, and
      coverage output enabled; directed pass/fail stimuli differ only on the
      admitted violation.
- [x] Compose obligation checks with the atomic Queue/state transaction
      checklist before publication.

**Exit:** a blocked rule reports the exact blocker without changing its firing
cycle, and the same runtime violation names the obligation later emitted as SVA.

### P5 - PYC and SVA lowering

**Primary files:**

- `compiler/acir/lib/CodeGen/QueueGraphPyc.cpp`
- PYC dialect/verifiers/passes
- `compiler/mlir/lib/Emit/VerilogEmitter.cpp`
- testbench/SVA generation only where it consumes the shared obligation record

- [x] Lower admitted obligation conditions to PYC.
- [x] Verify clock, reset/recovery, active-domain, and sampling metadata.
- [x] Emit obligation-linked SVA and same-ID coverage properties.
- [x] Emit one-hot AND-OR only with matching proof/obligation; the current
      priority encoder remains priority structured and runtime onehot proof is
      rejected.
- [x] Reject synthesis/deployment admission based only on a runtime check.
- [x] Add ready/valid, no-partial-commit, no-stale-update, range, credit, and
      alignment SVA fixtures.
- [x] Reject liveness kinds until temporal semantics are supported.
- [x] Prove C++ assertion and SVA share condition semantics and ID with matched
      passing and failing directed stimuli.

**Exit:** one source rule produces inspectable C++ and SVA checks with the same
ID and passes/fails on matched directed stimulus.

### P6 - Structural RTL audit and deterministic output

**Primary files:**

- `compiler/mlir/lib/Emit/VerilogEmitter.cpp`
- a new post-emit structural checker under `flows/tools/` if checks are clearer
  outside the emitter
- primitive-selection and generated-RTL tests

- [x] Enforce ASCII, explicit widths, named intermediates, and named port
      connections.
- [x] Enforce deterministic module/net/instance/assertion order.
- [x] Reject expressions inside primitive/submodule port connections.
- [x] Add priority-versus-onehot structural checks.
- [x] Add dead dependency, bit-use, and expensive-operation audit hooks.
- [x] Add two-clean-build byte comparison.
- [x] Generate a representative colored HTML diff artifact in evidence lanes.

**Exit:** malformed emitter fixtures fail closed, clean fixtures are
byte-identical, and generated source is human-auditable without opaque names.

### P7 - Sequential and SRAM semantic profiles

**Primary files:**

- PYC sequential/memory ops and verifiers
- `library/verilog/pyc_reg.v`
- `library/verilog/pyc_sync_mem*.v`
- `library/cpp/pyc_primitives.hpp`
- `library/cpp/pyc_sync_mem.hpp`
- `python/pycircuit/src/pycircuit/lib/sram.py`

- [ ] Add a pass/verifier that rejects unnecessary stall recirculation.
- [ ] Define SRAM read-result provenance and capture lifetime.
- [ ] Add canonical always-capture plus NBA-safe live/registered lowering.
- [ ] Add aggressive X simulation profile.
- [ ] Cover static live-window `N=1` and the mandatory one-live-cycle edge-based Q lifetime.
- [ ] Assert known enabled address/data/strobe.
- [ ] Add inactive-unit gating and stale-output tests.
- [ ] Implement the accepted C++/RTL X parity decision.
- [ ] Preserve synthesis-safe technology-independent primitives.

**Exit:** a stale-Q reproducer fails under the old shape, the canonical capture
shape passes, and C++/RTL comparison follows the accepted X contract.

### P8 - Recovery and versioned identity

**Primary ownership:** ACIR types/ops, analyses, QueueGraph/PYC lowering, reduced
generic fixtures.

- [ ] Add internal RecoveryDomain and TransactionRef.
- [ ] Add generation/epoch-qualified VersionedTable lookup.
- [ ] Add RecoveryEvent and KillSet lowering.
- [ ] Add Checkpoint semantics.
- [ ] Add retained result and execution-attempt qualification.
- [ ] Generate stale-update obligations automatically.
- [ ] Test slot reuse across epoch change and old response return.

**Exit:** an old completion cannot mutate a newly allocated slot in C++ or RTL,
and the stale path is observed in coverage.

### P9 - Multi-lane transaction and resources

- [ ] Add `TransactionGroup<N>` policies.
- [ ] Add ReservationSet preview/reserve/commit.
- [ ] Add MultiAllocator with explicit same-cycle reuse policy.
- [ ] Add AgeSelect-K semantic primitive and simple refinement.
- [ ] Add identity-qualified DependencySet.
- [ ] Add TerminalTransaction.
- [ ] Prove 4-wide prefix dispatch across at least three resource classes.
- [ ] Prove independent completion and ordered retirement fixtures.

**Exit:** the 4-wide fixture cannot allocate different lane counts in its
participating resources and passes transaction accounting under random stalls.

### P10 - Memory ordering

- [ ] Add typed MemoryOrderingGraph edges.
- [ ] Add store-resolution dependency discharge.
- [ ] Add alias forwarding relation.
- [ ] Add late violation/replay relation.
- [ ] Add execution-attempt and load-generation qualification.
- [ ] Add flush with outstanding response.
- [ ] Add negative stale-response tests.

**Exit:** the reduced LSU fixture covers wait, non-alias, forward, replay,
flush, and old-response rejection in C++ and RTL.

### P11 - Refinement and PPA evidence

- [ ] Define semantic primitive registry entries for AgeSelect-K, allocator,
      dependency set, and selected reservation structures.
- [ ] Add implementation variants with legality/latency/II/port/bank metadata.
- [ ] Add deterministic rule-based selection.
- [ ] Verify selected implementation cycle/refinement contract.
- [ ] Record structural estimates and optional synthesis results as evidence.
- [ ] Add regression thresholds only after stable baselines exist.

**Exit:** at least two legal implementations of one semantic primitive produce
equivalent observations, and illegal parameter combinations fail before emit.

### P12 - Framework acceptance demo

Build a vendor-neutral `MiniOOO` reduced fixture, not a product Core:

- 4-wide dispatch;
- small VersionedTable reorder window;
- small ready/dependency table;
- two simple execution resource classes;
- versioned completion;
- branch recovery;
- one simple memory dependency; and
- ordered retirement.

It must generate:

- verified ACIR;
- Rule Effect Graph;
- obligation report;
- QueueGraphPlan;
- C++ model;
- Verilog and SVA;
- rule/blocker coverage; and
- deterministic parity/stress evidence.

Long random runs belong in nightly/release lanes. Full SSM validation remains in
the SSM repository against a pinned pyCircuit revision.

## Suggested agent order and ownership

Later agents should work in this dependency order and must not skip an exit
criterion:

1. **Decision/schema owner:** P0.
2. **ACIR analysis owner:** P1-P2.
3. **Obligation/verifier owner:** P3.
4. **QueueGraph/gfsim owner:** P4, coordinated with the atomic Queue checklist.
5. **PYC/SVA owner:** P5.
6. **Verilog structure owner:** P6.
7. **Sequential/SRAM owner:** P7 after the X/parity decision.
8. **Recovery/identity owner:** P8.
9. **Multi-lane/resource owner:** P9.
10. **Memory-order owner:** P10.
11. **Refinement/PPA owner:** P11.
12. **Independent verification owner:** adversarially verifies each phase and
    owns P12 integration, but does not rewrite semantic contracts in tests.

An agent may implement a reduced vertical slice across several layers only
after the owning IR/schema is accepted. No C++ or Verilog agent may invent a
missing semantic rule locally.

## Accepted F0 decisions and deferred questions

F0 closes exact effect DAGs, the module-owned `ac.arch_obligation` symbol plus
module-owned typed expression table, sampling/runtime admission, four-state and
SRAM live-window policy, pointer/naming hard break, and Decisions 0275-0278's
closed finite-family declaration, concrete case-body schema, and exact
Python/ACIR/PYC carrier plus exact dependent/PYC mapping micro-schema. The
remaining genuine questions are deferred to their owning later decisions:

1. Which recovery/identity objects become public Python APIs after internal
   semantics stabilize.
2. Which transaction-group policy combinations are admitted in phase one.
3. Whether liveness obligations use SVA temporal operators, bounded monitors,
   or remain unsupported through the first release.
4. Which structural PPA estimates are stable enough to gate, rather than only
   report.

Open parameter domains, parametric bodies, and richer static-family constraints
are additionally deferred by Decisions 0275-0278. These deferred questions
do not reopen Decisions 0271–0278 and do not block
implementing their accepted safety contracts. Each requires a new decision
before its later phase may choose semantics.

## Definition of done

The extension is complete only when all applicable statements are true:

- [ ] Every executable rule has a complete typed effect summary.
- [ ] The whole-design graph exposes state/resource/conflict/order/recovery
      relations.
- [ ] Exact owner/index/field/predicate footprints survive normalization.
- [ ] Every obligation is proved, runtime checked, or rejected.
- [ ] One stable obligation ID links source, NDF, graph, C++, SVA, trace, and
      coverage.
- [ ] No backend silently fixes or drops a semantic obligation.
- [ ] C++ and Verilog implement the same committed-state and transaction model.
- [ ] Queue/state/resource effects commit atomically.
- [ ] Version/generation/epoch prevent stale updates after reuse or recovery.
- [ ] Multi-lane accepted masks update every participating resource equally.
- [ ] Ready/valid paths pass silent-drop and transaction-accounting audits.
- [ ] Stall uses natural register hold unless an explicit override is proven.
- [ ] SRAM output lifetime, capture, NBA visibility, and X behavior are tested.
- [ ] Inactive-unit assertions are correctly gated.
- [ ] Priority and one-hot implementation choices are evidence-backed.
- [ ] Width, dead-path, and expensive-operation audits are closed.
- [ ] Generated Verilog is deterministic, ASCII, readable, explicitly sized,
      and structurally checked.
- [ ] Directed, negative, parity, zero-loss, stress, and coverage gates pass.
- [ ] Evidence includes graph, obligation, source-tree, RTL-diff, parity, and
      deterministic-build artifacts.
- [ ] Framework tests remain consumer-neutral.
- [ ] No opaque content-derived token or content ledger is used as identity or
      semantic authority; no compatibility alias or legacy backend path is
      introduced.
