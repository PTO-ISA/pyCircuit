# Pythonic Frontend Guide for Coding Agents

Use the supported Python frontends to describe intent. Do not start by writing
PYC, ACIR, generated C++, or Verilog. Those layers are compiler contracts and
verification targets, not shortcuts around the frontend.

This guide helps coding agents choose an authoring model, decompose a complex
circuit, find a maintained example, and select enough validation. The normative
language and IR contracts remain in the [language specification](../reference/language.md),
[frontend API](../reference/frontend-api.md), and
[Agentic Circuit specification](../acir/spec/agentic-circuit.md).

## Choose the authoring model

| Design shape | Start here | Core abstraction |
| --- | --- | --- |
| RTL datapath, pipeline, register transfer, memory, or clock-domain logic | `pycircuit` Cycle-Aware Signal | `CycleAwareSignal`, `CycleAwareDomain`, inferred registers |
| Explicit reusable hierarchy, typed port bundles, static library construction | `pycircuit` structural API | `@module`, `spec`, connectors, `m.new()` |
| Queueing, backpressure, arbitration, scheduling, atomic state transitions | `agentic_circuit` | typed Queues, `@ac.rule`, Table and module state |

Cycle-Aware Signal is the default pyCircuit 6 hardware model. Use the
structural API to preserve explicit hierarchy or build reusable structural
libraries; it does not introduce a second timing model. Use Agentic Circuit
when the primary nouns are transactions, queues, rules, resources, or committed
architecture state.

## Architecture-rule compiler routing

For exact rule effects, whole-design conflict/order analysis, architecture
obligations, four-state/SRAM verification, atomic Queue lowering,
pointer-owned composition, or static parameter families, read Decisions
0271-0278 and these documents before editing implementation:

- `docs/rfcs/architecture-rule-compiler-extension.md` for product direction;
- `docs/rfcs/ac-architecture-rule-rtl-verification-extension-checklist.md` for
  phase ownership, accepted F0 contracts, and verification;
- `docs/rfcs/ac-rule-simqueue-atomic-lowering-checklist.md` for atomic
  Queue/state execution; and
- `docs/rfcs/ac-cpp-pointer-owned-module-composition.md` for source-owned C++
  ownership and the Decision 0274 hard-break target.

Do not infer backend semantics from checklist prose, reuse
`ac.marker.obligation` as Architecture Obligation IR, or place
consumer-specific models in pyCircuit.

For a finite module family, Decisions 0275-0278 require ordered typed
declarations, an explicit source-owned finite case list independent of callers,
and one `ac.module` family symbol containing ordered non-symbol
`ac.module.case` concrete High ACIR regions. Author these through
the exact Decision 0277 signatures and Decision 0278 dependent-value,
logical-type, source/provenance, and PYC mapping records. Parameter,
constraint, case, and binding collections are tuple literals; a child call uses
`child(runtime_args..., static=case(("name", value), ...))`. An
unparameterized module normalizes to one empty case. Until complete typed
header/link coverage, `ModuleFamilyPlan` and `ModuleCasePlan`, and the verified
`pyc.module`/`pyc.module.case` logical-to-physical carrier exist with explicit
projection paths, packed layouts, physical carrier roles, shared Queue ready,
and implicit clock/reset origins, do not emit a C++ or RTL family. Never
recover cases from dictionaries, concrete symbols,
suffixes, string `pyc.params`, sidecar specialization manifests, or observed
call sites.

Do not mix the `pycircuit` and `agentic_circuit` namespaces inside one
authoring function. They meet through verified PYC after the Agentic compiler
has discharged its queue, scheduling, and state contracts.

## Decide before coding

Write down these facts before choosing APIs:

- external ports and exact bit widths;
- clock and reset domains;
- persistent state and its single semantic owner;
- combinational transforms versus cycle or transaction boundaries;
- valid/ready, Queue, or other backpressure behavior;
- atomic updates that must commit together;
- static configuration versus runtime hardware values;
- required C++, gfsim, or Verilog observations.

Then search the example map below and copy the closest maintained pattern.
Reduce consumer-specific behavior to a small, design-neutral fixture before
adding it to this repository.

## Write cycle-aware hardware

Use Cycle-Aware Signal for ordinary synthesizable RTL. Values carry logical
cycle provenance, and the compiler inserts delay registers when expressions
combine values from different cycles.

```python
from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    build_cycle_aware,
    submodule_input,
    wire_of,
)


def registered_increment(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    inputs: dict | None = None,
    prefix: str = "inc",
) -> dict[str, CycleAwareSignal]:
    width = 8
    data = submodule_input(
        inputs, "data", m, domain, prefix=prefix, width=width
    )
    valid = submodule_input(
        inputs, "valid", m, domain, prefix=prefix, width=1
    )

    data_q = domain.signal(width=width, reset_value=0, name=f"{prefix}_data_q")
    valid_q = domain.signal(width=1, reset_value=0, name=f"{prefix}_valid_q")
    outputs = {
        "data": CycleAwareSignal.as_cas(data_q),
        "valid": CycleAwareSignal.as_cas(valid_q),
    }

    if inputs is None:
        m.output("data", wire_of(outputs["data"]))
        m.output("valid", wire_of(outputs["valid"]))

    data_next = data + 1
    domain.next()
    data_q.assign(data_next, when=valid)
    valid_q <<= valid
    return outputs


registered_increment.__pycircuit_name__ = "registered_increment"


if __name__ == "__main__":
    print(
        build_cycle_aware(
            registered_increment, name="registered_increment"
        ).emit_mlir()
    )
```

Follow these rules:

- Convert raw top-level inputs with `cas()` or `submodule_input()`.
- Keep `CycleAwareSignal` or `ForwardSignal` values inside the design. Use
  `wire_of()` only at an explicit output boundary.
- Declare geometry as a constant inside the module that owns it (`width = 8`).
  Caller-inferred specialization is removed: `build_cycle_aware`,
  `compile_cycle_aware`, and `domain.call` reject `width=`-style static
  arguments, and the CLI fails closed on any defaulted static parameter of a
  cycle-aware entrypoint. `prefix` is the only caller-supplied configuration
  `domain.call` accepts, and it exists to keep instance names unique.
- Emit every returned dictionary key through `m.output()` in standalone mode.
  Hierarchical `domain.call` compiles the child standalone and rebinds each
  returned key to a `pyc.instance` result, so a key without a result port fails
  closed.
- Declare state with `domain.signal()`. Place `domain.next()` at the intended
  write occurrence, then use `<<=` or `.assign(..., when=...)`. Build each next
  value in the cycle where its operands live, before that `domain.next()`;
  building it after `next()` adds one balancing register per bypassed operand.
- Use `mux()` or `.select()` for hardware choices. Python `if` and `for` are
  elaboration-time structure only; never coerce a signal to Python `bool`.
- Use Python lists and static loops for repeated scalar lanes. Do not invent a
  vector instruction or dynamically sized hardware collection.
- Compose reusable functions with `domain.call()` and exact input dictionaries.
  Missing, extra, wrong-width, or cross-domain inputs must fail closed.
- Cross clock domains only through an admitted CDC primitive such as
  `cdc_sync` or `async_fifo`.

Use `compile_cycle_aware()` when the canonical JIT `Design` is required. Use
`build_cycle_aware()` for explicit Python elaboration. Do not use a hidden mode
flag to change return type or compile mechanism.

## Write structural libraries

Use the structural API when explicit instances, port bundles, static
specialization, or reusable library construction are more important than
cycle-aware expression authoring.

```python
from pycircuit import Circuit, module
from pycircuit.hw import ClockDomain


@module
def registered_leaf(m: Circuit, clk, rst) -> None:
    domain = ClockDomain(clk=clk, rst=rst)
    incoming = m.input("incoming", width=8)
    value = m.out("value", domain=domain, width=8, init=0)
    value.set(incoming)
    m.output("outgoing", value)
```

Use the structural surface deliberately:

- `@module` preserves a `pyc.instance` boundary; `@function` inlines;
  `@const` performs pure compile-time metaprogramming and emits no IR.
- Use `spec.struct()` or `spec.bundle()` for compile-time port shapes.
- Use `m.inputs()`, `m.outputs()`, `m.state()`, `m.pipe()`, and connectors for
  bundle-wide construction instead of repeating leaf wiring.
- Instantiate with `m.new()` or `m.array()` and bind every required port
  explicitly.
- Use `pycircuit.structural.mux()` for raw `Wire` selection. The top-level
  `pycircuit.mux()` belongs to the cycle-aware surface and returns CAS.
- Do not recreate cycle tracking in a structural helper. If timing provenance
  drives the design, move the authoring logic to Cycle-Aware Signal.

Cycle-aware parents may instantiate structural library blocks at explicit
boundaries. Keep the timing contract in the cycle-aware parent and the static
shape/hierarchy contract in the structural child.

## Write transactional architecture hardware

Use Agentic Circuit when a complex circuit is better described as typed data
moving through queues and atomic rules. The compiler owns availability,
backpressure, reservations, arbitration, and commit.

```python
import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8


def increment(entry: Entry) -> Entry:
    return entry.with_fields(value=entry.value + 1)


@ac.rule
def install(entries, incoming):
    old = entries[incoming.index]
    entries[incoming.index] = increment(incoming)
    return old


@ac.system
def transaction_pipeline(incoming: Entry) -> Entry:
    entries = ac.table[4, Entry](init=0)
    outgoing = install(entries, incoming)
    return outgoing
```

Follow these rules:

- Use exact types such as `ac.u1` through `ac.u64`, `@ac.struct`, enums, fixed
  tuples, and admitted fixed state shapes. Do not rely on implicit width
  conversion or a runtime-computed width. A dependent `ac.bits[...]` width or
  `ac.array[...]` length must refer to a `static_parameter(...)` declared by the
  source-owned `module_decl(...)`. The expression grammar is limited to integer
  literals, declared static parameters, nominal static-config field projections,
  `+`, `-`, `*`, `index_width`, and `count_width`.
- Declare every parameterized module as one finite typed family. Keep
  `parameters`, `constraints`, `finite_cases`, and each `case(...)` binding as
  source-ordered tuples. Implement the family with
  `@ac.module(declaration=decl)` and select a child case with
  `static=ac.case(("name", value), ...)`. Do not use dictionaries, computed case
  collections, caller-observed cases, concrete case symbols, or value-bearing
  names.
- Use `@ac.config` with `static_config(ConfigType)` when several dependent
  leaves share one nested nominal configuration. A field projection retains
  its root parameter and ordered field path; never encode it as a dotted
  string or recover it with dictionary lookup.
- Build hierarchy by instantiating child `@ac.module`s inside a module body when
  an H1 -> H2 -> H3 tree matters. A hierarchy body may take more than one typed
  runtime input, binds every child-selected value to a name, and reads
  keyword-only `ac.const` geometry. Bind each intermediate before passing it on:
  an inline `child_b(child_a(x))` is rejected with `ACPY-MODULE-010`, and every
  composite input must have a consumer. Flattening the same composition into the
  top-level system stays equivalent, so prefer the hierarchy form whenever a
  consumer needs the boundary.
- Use `bool` for logical facts, standard `Enum` for closed categories, and
  `@ac.encoding(width=N)` only when an external protocol requires fixed or
  sparse values. Keep independent flags independent. Use `is_one_of` for
  membership, enum-targeted `ac.checked(..., fallback=...)` for raw protocol
  bits, `ac.onehot_enum(..., empty=..., conflict=...)` for positional masks,
  and exhaustive `ac.match_enum(..., invalid=...)` for value dispatch. Raw enum
  ingress may still carry an undeclared physical encoding, so never omit the
  invalid policy or treat a nominal annotation as a membership proof.
- Use `Packet(**left, **right, explicit=...)` and
  `value.with_fields(**patch)` only for exact name/type record composition.
  Construction is total; replacement preserves unmentioned fields.
- Use `value.project(TargetStruct)` for an explicit exact-name subset. The
  target is a real nominal `@ac.struct`, not an implicit subtype or writable
  alias. Apply changes back with `value.with_fields(**projected_patch)` and an
  explicit owner assignment.
- Keep large pass-through records on private single-consumer edges when the
  next pure rule reads only a few top-level fields; the verified private
  payload-pruning pass can choose a narrower internal carrier. Do not depend on
  pruning across public, observed, module, state, feedback, memory, or
  whole-record boundaries.
- Use `values.map(lambda item: ...)` for pure fixed-lane value transforms and
  `values.zip(other)` for exact-length positional pairing. Both are
  elaboration-time fixed expansions, evaluate every lane, retain exact
  recursive descriptors, and never create a runtime Python iterator. Keep the
  topology constructor `ac.map({...})` separate.
- Use `all`/`any`/`count`, closed-kind `fold`, and `first`/`argmin` for balanced
  fixed-array reductions. Use `scan(callback, initial=...)` only for an
  inclusive ordered prefix chain. Check `.valid` before treating a
  first/argmin index as a semantic match; no match deliberately carries index
  zero as a total fallback.
- Treat every `@ac.rule` as one schedulable atomic transition. Put all state
  writes and outputs that must commit together in the same rule.
- Declare `ac.slot(queue)` only in `@ac.module` or `@ac.system` topology. Pass
  it as a leading rule resource or capture it from a nested rule; read
  `.valid`/`.value` and use no-argument `.release()` so Queue, state, output,
  and Slot effects share one transaction. One Slot has one release owner,
  although other rules may read the same committed snapshot.
- Read committed state, compute proposals, and let the compiler publish them.
  Do not expose reservation, check, or prepare/publish mechanics in Python.
- Declare indexed persistent state with `ac.table[...]`; ordinary Python lists
  are static elaboration collections and never infer a persistent owner. Query
  a Table with `table.find(where=..., key=...)`.
- For a set-associative rank-two Table, write
  `table.view(request.row).find(where=...)`. The row must use the exact
  first-axis width and be statically bounded. The current contract scans only
  the fixed ways, supports first/count-one selection, and returns a global
  flattened Table index rather than a row-local way.
- Use ordinary typed pure helpers for reusable zero-delay value logic. Add
  `@ac.inline` only when mandatory compiler expansion is part of the intended
  generated-code organization.
- Use nested `@ac.rule` inside `@ac.module` when a reusable module owns typed
  lexical state. Capture inference replaces redundant `nonlocal` declarations;
  ownership remains explicit below the frontend.
- Use Queue transforms such as `apply`, `route`, `merge`, `depend`, and
  `reorder` for dataflow. Specify finite `depth`, latency, capacities, and
  policies where the API requires them.
- Prefer typed non-`const` `@ac.system` parameters and typed returns for
  external boundaries. Explicit `ac.source()` and `ac.sink()` remain
  transitional capture forms.
- Use `ac.scope()` and meaningful Python names. They become display provenance
  without changing family, case, or instance identity.
- Remember that capture-only markers are frontend syntax. Calling them as
  ordinary runtime functions must fail; they are valid inside captured
  `@ac.system`, `@ac.module`, and `@ac.rule` source.

The synthesizable Table profile is bounded. Rank, entries, packed width, total
state, and writer counts outside Decision 0241 must fail at verification rather
than receive a backend workaround.

## Convert integer widths explicitly

Agentic Circuit never changes an integer width implicitly. `a * b` on two
`ac.s16` values is an `s16` product, so returning it from a module that declares
`ac.s32` fails with `ACPY-MODULE-001: module result payload type mismatch`.
Extend or truncate first, with the intrinsic whose second argument is the
concrete destination type:

| Call | Effect |
| --- | --- |
| `ac.zext(value, ac.uN)` | zero-extend to a strictly wider target |
| `ac.sext(value, ac.sN)` | sign-extend to a strictly wider target |
| `ac.truncate(value, ac.uN)` | keep the low `N` bits of a strictly narrower target |

The target is a positional concrete `ac.uN` / `ac.sN` / `ac.bits[N]` type, not a
`width=` keyword: that form belongs to the CycleAwareSignal methods described in
[the frontend API reference](../reference/frontend-api.md). `zext` and `sext`
require a wider target, `truncate` requires a narrower one, and every other
shape fails with `ACPY-CAST-001`.

Use `ac.sext` whenever the following arithmetic is signed. `ac.zext` of a
negative value zero-fills the extension bits and silently changes the value.

```python
@ac.struct
class SignedPair:
    a: ac.s16
    b: ac.s16

@ac.module_decl(source="examples/module.py")
def widen(pair: SignedPair) -> ac.s32:
    ...

widen_decl = widen

@ac.module(declaration=widen_decl)
def widen(pair: SignedPair) -> ac.s32:
    return ac.sext(pair.a, ac.s32) * ac.sext(pair.b, ac.s32)
```

Both operands are sign-extended (the sign bit is replicated through the
extension bits) before the multiply, so the product is a real 32-bit signed
multiply: `(-32768) * (-32768) = 1073741824`, `(-32768) * 32767 = -1073709056`,
and `(-1) * 1 = -1`.
`tests/integration/agentic-circuit/e2e/test_signed_widening_runtime.py` pins
those products on the gfsim runtime, and
`tests/python/agentic-circuit/python_frontend/test_queue_frontend.py` pins the
emitted extension and the rejection diagnostics.

## Decompose a complex design

Use this order regardless of frontend:

1. Define exact-width payloads, ports, and static configuration.
2. Isolate pure combinational transforms in small typed helpers.
3. Name each persistent state owner and document its reset or initial value.
   For Agentic lexical state, `total: ac.u8 = 5` is the reset image on
   `ac.var.decl`; a Python `if` around the assignment is the write enable.
   Each executable implementation source owns one public module family, one
   source-named `.ac`, one source-owned interface shard, and one matching
   `.hpp`/`.cpp` generated source group. These source groups compile separately
   and link with the selected-system composition unit. No whole-program
   artifact, post-compile split, `.h` duplicate, or per-case file is an
   authority.
4. Mark timing boundaries with `domain.next()` or transaction boundaries with
   `@ac.rule`; do not mix the two mental models.
5. Add hierarchy only where it improves reuse, review, or independent testing.
6. Add backpressure and arbitration after the state transition is explicit.
7. Write the smallest observable test before scaling lanes, entries, or rules.
8. Inspect emitted PYC or ACIR to confirm the frontend expressed the intended
   semantics; do not patch the emitted form.

Prefer multiple small modules and rules over one giant Python function. A good
boundary has typed inputs, typed outputs, one state-ownership story, and an
independently testable timing or transaction contract.

## Avoid these failure modes

- Handwritten PYC/ACIR used as the product implementation.
- Backend-only semantic fixups in C++ or Verilog emitters.
- Raw `Wire` arithmetic inside a cycle-aware design.
- Python `if signal`, data-dependent Python loops, reflection, dynamic
  allocation, or arbitrary host execution in captured hardware logic.
- Manual ready/valid, reservation, or commit bookkeeping around an Agentic
  rule that already owns those semantics.
- Duplicate helper implementations for gfsim, PYC C++, and Verilog.
- Compatibility aliases, prior-version modes, or obsolete labels applied to
  current APIs.
- Complete CPU, NPU, SoC, board, ISA, or product testbench implementations in
  this framework repository.

If the public frontend cannot express required semantics, reduce the missing
behavior to a verifier-backed framework issue. Do not bypass the frontend in a
consumer design.

## Maintained example map

| Need | Example |
| --- | --- |
| Basic inferred register | `examples/pycircuit/basics/counter/` |
| Repeated lanes and ready behavior | `examples/pycircuit/features/issue_queue_2picker/` |
| Multi-stage arithmetic pipeline | `examples/pycircuit/applications/digital_filter/` |
| Explicit hierarchy and probe metadata | `examples/pycircuit/features/trace_dsl_smoke/` |
| Structural specs and bundle state | `examples/pycircuit/features/struct_transform/` |
| Queue scopes, dependency tracking, route and merge | `examples/agentic-circuit/pipelines/routed_dependency_pipeline.py` |
| Typed pure helpers and mandatory inline | `tests/integration/agentic-circuit/e2e/fixtures/pure_helpers/` |
| Finite typed module families and dependent interfaces | `tests/python/agentic-circuit/python_frontend/test_finite_families.py` |
| Name-based record construction and replacement | `examples/agentic-circuit/pipelines/record_spread_pipeline.py` |
| Explicit nominal record projection and patch-back | `examples/agentic-circuit/pipelines/record_projection.py` |
| Fixed-width sparse enum encoding | `examples/agentic-circuit/pipelines/encoded_enum_pipeline.py` |
| Exhaustive enum match and explicit checked/one-hot decoding | `examples/agentic-circuit/pipelines/enum_helpers.py` |
| One-hot presence and conflict reporting | `examples/agentic-circuit/blocks/onehot_encode.py` |
| Typed literals, explicit width conversion, and unsigned div/rem | `examples/agentic-circuit/blocks/typed_integer_operations.py` |
| Bounded integer decoding and dynamic fixed-array read | `examples/agentic-circuit/blocks/bounded_integer_operations.py` |
| Fixed-array map/zip, nested callbacks, and exact captures | `examples/agentic-circuit/blocks/array_combinators.py` |
| Combined cross-backend composition | `examples/agentic-circuit/pipelines/frontend_composition_pipeline.py` |
| Atomic Table replacement | `examples/agentic-circuit/state/table_rule.py` |
| Nested module state capture | `examples/agentic-circuit/state/inferred_nested_rule.py` |
| Reusable stateful scheduling | `examples/agentic-circuit/state/reusable_oldest_ready_isq.py` |
| Transactional slot release from rules | `examples/agentic-circuit/state/slot_rule_mailbox.py` |

Examples are product surface. Add a new public example only when it teaches a
distinct supported pattern; use an integration fixture for broad or expensive
cross-backend coverage.

## Validate and hand off

Start with the narrowest proof for the chosen frontend:

```bash
# Cycle-aware or structural Python contract
pytest tests/unit -m unit
python3 flows/tools/check_api_hygiene.py \
  python/pycircuit/src/pycircuit examples/pycircuit docs README.md

# Agentic Python contract
python3 tools/agentic-circuit/check-contracts.py
python3 -m unittest discover \
  -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'

# Documentation
mkdocs build --strict
```

For a behavior change, add the smallest affected example, MLIR, CTest, gfsim,
PYC C++, or Verilator case. Do not run a broad unrelated lane as a substitute
for focused evidence. The release workflow owns complete closure.

Leave the next agent this compact handoff:

```text
Frontend: cycle-aware | structural | agentic
Decisions: affected decision IDs
State owners: registers, memories, Tables, or none
Timing: logical cycles, latency, and clock domains
Flow control: valid/ready, Queue depth, backpressure, arbitration
Atomicity: values that must commit together
Unsupported boundaries: explicit fail-closed cases
Tests: exact commands and results
Evidence: docs/gates/logs/<run-id>/ when decision-bearing
```

If any of these fields is unknown, inspect or reduce the design before adding
more implementation code.
