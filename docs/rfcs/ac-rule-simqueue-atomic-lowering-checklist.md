# AC rule and SimQueue atomic lowering checklist

**Status:** Draft implementation checklist

**Parent design:** [AC C++ pointer-owned module composition](ac-cpp-pointer-owned-module-composition.md)

**Architecture extension:**
[AC architecture-rule, RTL codegen, and verification extension checklist](ac-architecture-rule-rtl-verification-extension-checklist.md)

This document owns the atomic Queue/state execution slice. Whole-design effect
graphs, obligation IR, SVA generation, SRAM/X methodology, recovery identity,
multi-lane resource algebra, and verification evidence are owned by the
architecture extension checklist above.

## Required outcome

An authored `ac.rule` becomes one compiler-generated atomic Queue transaction.
The author writes functional rule logic; compiler passes derive and insert all
Queue readiness, borrowing, and commit behavior.

For a rule with multiple input and output Queues, generated C++ must implement
this sequence:

```text
check every required input is not empty
borrow front from every required input without mutation
evaluate rule predicate and each output-presence predicate
check every output that will be produced is not full
prepare every participating Queue and state resource in one commit group
compute or materialize output payloads
push every present output and pop every consumed input atomically
publish state/Table/Slot effects in the same commit group
commit at Xfer
```

If any check or preparation fails, no input is popped, no output is pushed, no
state changes, and no wide payload allocation becomes committed.

## Compiler/runtime contract

### Logical SimQueue operations

Generated code must have explicit equivalents for:

| Operation | Required meaning | Mutates committed state |
| --- | --- | --- |
| `empty()` | no token can be consumed by this rule | no |
| `full()` | no token can be produced by this rule | no |
| `front()` | borrow the next input token for rule evaluation | no |
| `preparePop(group)` | reserve an input consumption | no |
| `preparePush(group)` | reserve output capacity | no |
| `pop(group)` | publish the prepared input consumption | proposal only |
| `push(group, value)` | publish the prepared output value | proposal only |
| `cancel(group)` | release every reservation held by the group | no |
| `xfer()` | commit accepted proposals together | yes |

Existing runtime names such as `canProposePop`, `canProposePush`,
`preparedPopValue`, `publishPreparedPop`, and `publishPush` may implement the
first version. Generated-source tests must still prove the logical operations
above are present in the emitted transaction. Do not add a direct destructive
`pop()` path that bypasses prepare/publish/Xfer.

### Atomicity

- One rule firing uses one non-invalid `CommitGroupId`.
- Every required input, every present output, and every state/Table/Slot write
  joins that group.
- A rule cannot publish one output and then discover that another output is
  full.
- A rule cannot pop one input and then discover that another input is empty.
- Arbitration selects the winning complete firing before preparation. A losing
  candidate creates no reservation and therefore has nothing to cancel.
- `front()` returns a committed immutable snapshot that stays valid through
  the firing prepare/publish sequence.

### Optional outputs

Output capacity is required only when that output's `when` predicate is true.
Because the predicate may depend on input values, lowering must check inputs
and borrow their fronts before deciding the active output set.

An absent output is neither checked for capacity nor pushed. Its Queue remains
unchanged.

## Work breakdown

### M0 — Freeze the rule transaction contract

**Goal:** turn this checklist into a normative compiler decision before code
changes spread across MLIR, QueueGraph, and gfsim.

- [ ] Add or amend a pyCircuit decision that defines the sequence in Section 1.
- [ ] State that Python authors do not write `empty/full/front/pop` plumbing.
- [ ] Define `front()` as immutable and non-consuming.
- [ ] Define pop/push visibility at Xfer, not Work.
- [ ] Define optional-output capacity checks.
- [ ] Define how zero-input, zero-output, and state-only rules participate.
- [ ] Define wide-payload reference lifetime during prepare, cancel, recovery,
      reset, and Xfer.

**Exit criteria:** one accepted decision identifies the exact pass/runtime
owners below and has focused verification requirements.

### M1 — Verify `ac.rule` shape before analysis

**Goal:** reject malformed rule regions before readiness inference.

Primary code:

- `compiler/acir/lib/Dialect/ACIR/ACIROps.cpp`
- `compiler/acir/include/acir/Dialect/ACIR/ACIROps.td`

- [ ] Require one body block and one `ac.rule.return`.
- [ ] Require input Queue ordinals to be dense and unique.
- [ ] Require output ordinals to be dense and unique.
- [ ] Require returned payload arity/types to match output Queue arity/types.
- [ ] Require every `ac.rule.output` ordinal to reference a declared output.
- [ ] Reject duplicate output declarations for one ordinal.
- [ ] Reject mutable or escaping input snapshots.
- [ ] Reject unresolved dynamic obligations before check materialization.

Tests:

- [ ] Extend `tests/mlir/agentic-circuit/ACIR/rule-invalid.mlir`.
- [ ] Add malformed two-input/two-output cases.
- [ ] Add missing/duplicate output-presence cases.

**Exit criteria:** malformed rule topology fails before `LowerRules` runs, with
stable diagnostics naming the rule and offending ordinal.

### M1A — Lower Python expressions to cheap canonical AC logic

**Goal:** eliminate frontend `ac.var` wrappers and express control/address
logic with the smallest readable typed operations before QueueGraph codegen.

Primary code:

- `python/agentic-circuit/src/agentic_circuit/_queue_frontend.py`
- `compiler/acir/lib/Transforms/LowerVariables.cpp`
- ACIR canonicalization and value-constraint passes

Preferred canonical operations:

- [ ] `and`, `or`, `xor`, `not`;
- [ ] `add`, `sub`;
- [ ] `shl`, `lshr`, `ashr`;
- [ ] typed `cmp`;
- [ ] `mux`/select;
- [ ] `concat`, `extract`, explicit extend/truncate;
- [ ] constants with exact widths.

Strength reduction:

- [ ] `x * 0` becomes zero and `x * 1` becomes `x`.
- [ ] unsigned multiply by a power of two becomes `shl`.
- [ ] small constant multiply becomes a bounded balanced shift/add/sub tree
      when emitted cost is lower.
- [ ] unsigned divide by a power of two becomes `lshr`.
- [ ] unsigned remainder by a power of two becomes an `and` mask.
- [ ] signed divide/remainder lowers only when rounding, overflow, and minimum
      signed value semantics are proven identical.
- [ ] dynamic `mul`, `div`, and `rem` remain only for an explicitly admitted
      arithmetic datapath; they are not introduced for ordinary indexing,
      Queue control, address slicing, or boolean decisions.
- [ ] unsupported expensive operations fail with a cost/action diagnostic
      instead of silently expanding into an unbounded network.

Readability:

- [ ] `ac.var` is absent from backend QueueGraph/PYC inputs after lowering.
- [ ] canonical expressions preserve the Python source/display name.
- [ ] mux trees remain balanced and source-linked.
- [ ] rewrites preserve exact fixed-width wrap, signedness, and known/Z masks.

Tests:

- [ ] Add MLIR tests proving reducible multiply/divide disappear.
- [ ] Add negative signed-division cases where strength reduction is illegal.
- [ ] Add frontend→ACIR→C++/Verilog parity tests for every rewrite.
- [ ] Add emitted-cost assertions for shift/add/mux trees.

**Exit criteria:** normal rule/control logic contains only the preferred cheap
operations; every remaining expensive arithmetic op is explicit, source-linked,
and covered by a dedicated capability test.

### M2 — Infer typed Queue checks and effects

**Goal:** derive readiness and consumption/production facts from the rule
signature and body.

Primary code:

- `compiler/acir/lib/Transforms/LowerRules.cpp`
  - `inferRuleEffects`
  - `materializeRuleChecks`
  - `materializeRuleHandshake`

- [ ] Emit one `input_available` check for every consumed input Queue ordinal.
- [ ] Emit one `input_consume` effect for every consumed input ordinal.
- [ ] Emit one `output_capacity` check for every output ordinal.
- [ ] Mark output checks `always` or `predicate` from `ac.rule.output when`.
- [ ] Emit one `output_produce` effect with the same guard as the output.
- [ ] Preserve `ac.rule.output_presence` for every output ordinal.
- [ ] Add every participating Queue to `ac.rule.transaction_resources`.
- [ ] Include Table, Slot, variable, and arbitration resources in the same
      transaction-resource set.
- [ ] Preserve same-cycle persistent-state reads as committed old-state
      observations; do not infer forwarding from source order.
- [ ] Fail when checks/effects contain missing, duplicate, or out-of-range
      ordinals.
- [ ] Keep input availability unconditional unless the language gains an
      explicit optional-input construct.

Tests:

- [ ] Extend `rule-multi-input-lowering.mlir`.
- [ ] Extend `rule-multi-output-lowering.mlir`.
- [ ] Extend `rule-optional-output-lowering.mlir`.
- [ ] Add a three-input/four-output rule with mixed output predicates.
- [ ] Add an outputless state rule.

**Exit criteria:** textual ACIR after analysis lists complete, typed, ordered
checks/effects and no rule-specific Queue condition is supplied by Python.

### M3 — Lower `ac.rule` to one atomic `ac.firing`

**Goal:** preserve the inferred transaction contract in the executable IR.

Primary code:

- `compiler/acir/lib/Transforms/LowerRules.cpp`
  - `lowerRulesToFiring`
  - `canonicalizePureFirings`
  - `verifyRuleClosure`

- [ ] Copy `checks_typed`, `effects_typed`, `output_presence`, state accesses,
      activation sources, arbitration membership, and transaction resources.
- [ ] Preserve source and NDF provenance.
- [ ] Preserve all input/output types and ordinals.
- [ ] Keep variadic or conditional firings as `ac.firing`.
- [ ] Canonicalize to `ac.transform` only when the transform representation can
      express exactly the same transaction.
- [ ] Reject an `ac.firing` that loses a Queue or state resource.
- [ ] Reject a firing whose output predicate differs between capacity check and
      output effect.

Tests:

- [ ] Add FileCheck coverage for the complete two-input/two-output attribute set.
- [ ] Add negative coverage in `rule-lowering-invalid.mlir`.
- [ ] Prove optional outputs do not canonicalize to an unconditional transform.

**Exit criteria:** every executable firing carries enough typed metadata for
QueueGraph codegen to emit readiness and atomic commit without reinterpreting
Python source.

### M4 — Carry rule transactions through QueueGraphPlan

**Goal:** make QueueGraphPlan the typed handoff from MLIR semantics to C++
generation.

Primary code:

- `compiler/acir/lib/CodeGen/QueueGraphPlan.cpp`
- `compiler/acir/include/acir/CodeGen/QueueGraphPlan.h`

- [ ] Add a plan record for each input check: Queue ID, ordinal, count, guard.
- [ ] Add a plan record for each output check: Queue ID, ordinal, count, guard.
- [ ] Carry output-presence expressions in plan order.
- [ ] Carry the ordered transaction-resource set.
- [ ] Carry state/Table/Slot prepare and publish actions.
- [ ] Carry arbitration winner requirements.
- [ ] Reject a plan when MLIR checks/effects and Queue endpoints disagree.
- [ ] Reject duplicate Queue actions in one firing unless a defined batch
      count combines them.
- [ ] Preserve payload storage type, including wide shared payload classes.

Tests:

- [ ] Extend `tests/cpp/agentic-circuit/CodeGen/QueueGraphPlanTest.cpp`.
- [ ] Assert exact plan records for 2-in/2-out and optional-output cases.
- [ ] Add plan rejection tests for missing capacity and consume actions.

**Exit criteria:** the plan explicitly represents every logical
`empty/full/front/pop/push` operation required by the firing.

### M5 — Provide the canonical SimQueue transaction API

**Goal:** expose non-mutating checks/borrows and group-scoped mutation in the
runtime.

Primary code:

- `simulator/gfsim/include/gfsim/queue.h`
- `simulator/gfsim/include/gfsim/queue_blocks.h`

- [ ] Provide or retain exact equivalents of `empty()` and `full()`.
- [ ] Provide `front()`/prepared-front access that does not pop or copy a wide
      payload.
- [ ] Keep `preparePop(group)` and `preparePush(group)` idempotent only for the
      exact admitted contract; reject conflicting groups.
- [ ] Provide group-scoped `pop(group)` and `push(group, value)` publication.
- [ ] Provide `cancel(group)` that clears every prepared reservation.
- [ ] Ensure output capacity accounts for same-group input pops only when that
      Queue is both input and output of the same admitted operation.
- [ ] Preserve latency, rate, lane, byte-capacity, and batch constraints.
- [ ] Make `front()` stable from prepare through publish/cancel.
- [ ] Keep committed values unchanged until Xfer.

Wide payloads:

- [ ] `PackedBitWidth <= 64` remains a value Queue element.
- [ ] `PackedBitWidth > 64` uses `shared_ptr<const PayloadClass>`.
- [ ] A fresh wide value allocates only after successful push preparation.
- [ ] Forwarding/fanout preserves the payload pointer.
- [ ] Pop commit releases the Queue reference.
- [ ] Final-reference release destroys the payload exactly once.
- [ ] Cancel, recovery, reset, and Queue destruction release held references.

Tests:

- [ ] Extend `QueueBlocksTest.cpp` for every operation above.
- [ ] Add allocation/destruction counters for wide payloads.
- [ ] Run AddressSanitizer coverage for cancel/reset/fanout lifetimes.

**Exit criteria:** runtime tests prove no check mutates state, no failed firing
leaks a reservation/reference, and Xfer is the only committed visibility point.

### M6 — Emit explicit atomic Queue code

**Goal:** generated C++ implements the transaction plan without hidden
best-effort behavior.

Primary code:

- `compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp`
- `compiler/acir/lib/CodeGen/QueueGraphPyc.cpp` where PYC lowering shares the
  same transaction facts

Generated sequence for one firing:

- [ ] Return without mutation when any required input is empty.
- [ ] Borrow every input front without consuming it.
- [ ] Evaluate rule condition and output-presence expressions.
- [ ] Return without mutation when any present output is full.
- [ ] Create one commit group from the firing's stable object ID.
- [ ] Prepare every present output.
- [ ] Prepare every consumed input.
- [ ] Prepare every Table/Slot/state effect.
- [ ] Cancel all prepared resources when any prepare fails.
- [ ] Construct fresh wide output payloads only after successful preparation.
- [ ] Reuse shared pointers for unchanged forwarded payloads.
- [ ] Publish every present output.
- [ ] Publish every input pop.
- [ ] Publish every state effect.
- [ ] Treat a publish failure after successful prepare as a runtime invariant
      failure; do not continue with partial effects.
- [ ] Commit proposals only at Xfer.

Do not emit independent `if` blocks that can partially pop or push. Do not rely
on C++ argument evaluation order for Queue ordering.

Tests:

- [ ] Add generated-source checks for explicit readiness/borrow/prepare/publish.
- [ ] Add generated-source checks for deterministic input/output ordinal order.
- [ ] Add a 2-in/2-out executable runtime test.
- [ ] Add a 3-in/4-out mixed-presence runtime test.
- [ ] Add a blocked-output test proving zero input consumption.
- [ ] Add an empty-input test proving zero output production.

**Exit criteria:** generated C++ visibly implements the Section 1 sequence and
passes runtime atomicity tests.

### M7 — Integrate state and arbitration

**Goal:** Queue atomicity composes with Table, Slot, variables, and competing
rules.

- [ ] Winner selection occurs before transaction preparation.
- [ ] Losing rules do not reserve Queue or state resources.
- [ ] Table writes and Queue effects share one commit group.
- [ ] Slot release and Queue effects share one commit group.
- [ ] Conditional state writes use the same predicate used during preparation.
- [ ] Conflicting writes select exactly one complete transaction.
- [ ] Recovery fences outrank ordinary rule publication where specified.
- [ ] A runtime invariant failure reports the rule stable ID and all involved
      resource paths.

Tests:

- [ ] Multi-rule shared-Queue conflict.
- [ ] Queue + Table atomic failure.
- [ ] Queue + Slot release atomic failure.
- [ ] Recovery versus ready rule in the same cycle.

**Exit criteria:** no test can observe a Queue effect without its paired state
effect or vice versa.

### M8 — Generate module/interface and payload classes

**Goal:** connect atomic rule code to the pointer-owned C++ module design.

- [ ] Generate one `.hpp` from every resolved module declaration.
- [ ] Generate one `.cpp` from every implementation `.ac` source unit.
- [ ] Generate a C++ class for every nominal `ac.struct`.
- [ ] Generate `<Module>Ports` with `SimQueue<StorageT> *` fields.
- [ ] Select `StorageT` after static type concretization.
- [ ] Parent classes own internal `SimQueue<StorageT>` values.
- [ ] Child modules receive only pointers to parent-owned Queues.
- [ ] Child modules remain `unique_ptr`; wide payloads alone use `shared_ptr`.
- [ ] Instance records store the allocated module pointer, not Queue ownership.

**Exit criteria:** nested H1→H2→H3 generated headers/sources compile
independently and preserve pointer/Queue ownership.

### M8A — Generate short readable names for C++ and Verilator waveforms

**Goal:** preserve human-readable source intent without encoding enclosing
module or specialization details into identifiers.

Name rules:

- [ ] C++ class identifiers contain the definition name only.
- [ ] Static specialization values are explicit template arguments or a typed
      `Params` object, not identifier suffixes.
- [ ] Instance names do not repeat the enclosing module name.
- [ ] Internal ACIR, C++, Verilog, and waveform leaf names use lower snake case.
- [ ] Generated identifiers contain no `__`.
- [ ] Compiler-owned names use a readable single-underscore prefix such as
      `ac_tmp_` or `ac_inst_`, never a double-underscore prefix.
- [ ] The `*` in the interface pattern is documentation syntax for zero or more
      optional qualifier tokens; it is not emitted into an identifier.
- [ ] Each token is concise; generated leaf identifiers target at most 63
      characters. Overlength names fail with an actionable rename diagnostic;
      they are not truncated and do not receive an opaque suffix.

Cross-module interface pattern:

```text
<name>_<from_module>_<to_module>_<stage_name>*_<valid|ready|purpose>
```

- [ ] `from_module` and `to_module` are the only module-name tokens.
- [ ] `stage_name` names the visible pipeline boundary, not an implementation
      ordinal.
- [ ] `valid` and `ready` use those full words.
- [ ] purpose signals use a short domain term such as `request`, `response`,
      `retry`, `flush`, `data`, or `credit`.
- [ ] Internal module names omit endpoint tokens and remain local-role names.

Examples:

```text
request_decode_select_s1_valid
request_decode_select_s1_ready
grant_select_execute_s2_response
retry_execute_commit_s3_request
```

These are generic naming-shape examples only. Consumer product/module names do
not appear in normative framework examples.

Traceability and waveform gates:

- [ ] Python name, ACIR display name, C++ member, Verilog signal, and source-map
      entry are cross-checked.
- [ ] Repeated instances keep short local names and differ through hierarchy
      paths, not repeated module prefixes.
- [ ] Verilator VCD/FST fixtures assert expected readable signal leaves.
- [ ] No source-facing signal requires a generated-name lookup table to
      understand a waveform.
- [ ] Update `docs/reference/name-mangling.md` as a hard break; do not retain a
      second legacy naming mode.

**Exit criteria:** generated C++ and Verilator waveforms show short lower-snake
signals, specialization data is separate, and no emitted identifier contains a
double underscore.

### M9 — End-to-end semantic matrix

**Goal:** prove frontend, ACIR, QueueGraph, runtime, C++, and Verilog agree.

Required cases:

- [ ] one input / one output;
- [ ] two inputs / one output;
- [ ] one input / two outputs;
- [ ] two inputs / two outputs;
- [ ] optional output absent while its Queue is full;
- [ ] optional output present while its Queue is full;
- [ ] outputless state-only rule;
- [ ] zero-input initially active rule;
- [ ] same Queue read/write transaction;
- [ ] multi-lane or batch pop/push;
- [ ] wide struct input/output;
- [ ] unchanged wide-payload forwarding;
- [ ] wide-payload field update producing a new allocation;
- [ ] fanout of one wide payload;
- [ ] recovery and reset with retained wide payloads;
- [ ] competing rules and state arbitration;
- [ ] nested repeated module instances.

For each case:

- [ ] check High ACIR typed summaries;
- [ ] check lowered `ac.firing`;
- [ ] check QueueGraphPlan;
- [ ] inspect generated C++ source shape;
- [ ] execute gfsim C++;
- [ ] compare TICK-OBS and XFER-OBS with PYC/Verilog where supported.

**Exit criteria:** the matrix is green without author-written readiness plumbing
or backend-only semantic exceptions.

### M10 — Diagnostics and negative gates

**Goal:** fail closed on incomplete atomic lowering.

- [ ] Missing input availability check.
- [ ] Missing output capacity check.
- [ ] Output predicate/check predicate mismatch.
- [ ] Missing input consume or output produce effect.
- [ ] Queue omitted from transaction resources.
- [ ] Duplicate ordinal/action.
- [ ] Unsupported dynamic check obligation.
- [ ] Partial prepare without cancel path.
- [ ] Wide mutable payload after publication.
- [ ] Null Queue pointer in generated module ports.
- [ ] Unresolved module declaration or incompatible payload storage type.

**Exit criteria:** every malformed case has a stable diagnostic and no backend
artifact is published.

### M11 — Performance and cost gates

**Goal:** avoid making correct atomicity prohibitively expensive.

- [ ] Readiness checks are O(number of participating ports).
- [ ] No deep copy occurs for wide payload forwarding or fanout.
- [ ] Fresh wide allocation happens at most once per produced payload.
- [ ] No allocation occurs on a blocked firing.
- [ ] Shared-pointer reference changes are bounded by actual Queue/fanout edges.
- [ ] QueueGraph emitted-cost accounting includes checks, transaction actions,
      and payload allocation.
- [ ] Benchmark 1/2/4/8 input and output counts.

**Exit criteria:** cost reports are deterministic and existing hard emitted-cost
limits remain green.

### M12 — Documentation, evidence, and consumer handoff

**Goal:** make the change reproducible for another agent and safe for consumers.

- [ ] Update the ACIR language specification for rule atomicity.
- [ ] Update the C++ generation and runtime documentation.
- [ ] Update the name-mangling document for generated `.hpp`/`.cpp` classes.
- [ ] Record exact focused commands and outputs under one gate run ID.
- [ ] Run pyCircuit PR gates and the narrowest native/MLIR/runtime suites.
- [ ] Run release AC/PYC/Verilog lanes before release promotion.
- [ ] Publish the exact pyCircuit revision for consumer pinning.
- [ ] Validate SSM from its own checkout; do not add SSM-specific behavior to
      pyCircuit.

**Exit criteria:** framework gates are green, evidence is reviewable, and the
consumer needs only a pin update plus its own compatibility tests.

## Definition of done

The work is complete only when all statements below are true:

- [ ] Every `ac.rule` Queue dependency is compiler-derived.
- [ ] Multi-input/multi-output readiness is all-or-nothing.
- [ ] `front()` is non-consuming and stable for the firing.
- [ ] Every pop/push/state effect belongs to one commit group.
- [ ] Optional outputs check capacity only when present.
- [ ] Blocked or losing rules produce no partial effect.
- [ ] Wide structs move by immutable shared pointer without deep copy.
- [ ] Push reservation precedes fresh wide-payload allocation.
- [ ] Pop commit releases Queue ownership; final release frees the payload.
- [ ] Parent modules own Queue storage and child modules use Queue pointers.
- [ ] Generated module declarations produce `.hpp`; implementation AC units
      produce `.cpp`.
- [ ] Frontend control/address logic prefers simple bit, add/sub, shift, compare,
      and mux operations; reducible multiply/divide does not reach codegen.
- [ ] Module and specialization data are not embedded in local identifiers.
- [ ] Internal and waveform-visible names are short lower-snake identifiers
      with single underscores and the reviewed interface pattern.
- [ ] Positive, negative, runtime, sanitizer, and cross-backend tests pass.
- [ ] No compatibility path restores author-written Queue plumbing or the old
      by-value wide-payload behavior.

## Suggested agent assignment order

Agents should take the work in this dependency order:

1. **Dialect/verifier agent:** M0–M1.
2. **Frontend/canonicalization agent:** M1A.
3. **Rule-analysis pass agent:** M2–M3.
4. **QueueGraphPlan agent:** M4.
5. **gfsim Queue runtime agent:** M5.
6. **C++ codegen agent:** M6 and M8.
7. **Naming/waveform agent:** M8A.
8. **Atomic state/arbitration agent:** M7.
9. **Test agent:** M9–M11, beginning focused tests as soon as each owner lands.
10. **Documentation/release agent:** M12.

No agent should begin consumer-specific adaptation before M0–M10 are green in
the framework repository.
