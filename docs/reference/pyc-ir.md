# PYC IR Reference

PYC is an MLIR dialect (`pyc`) intended to be a common, backend-agnostic IR for
hardware components, with multi-clock modeling and strict ready/valid streaming
semantics.

For the compiler pipeline and pass-by-pass behavior, see the
[compiler pipeline](../architecture/compiler-pipeline.md).

## Types

- `!pyc.clock`: a clock signal
- `!pyc.reset`: a reset signal
- Data types use MLIR integers (`i1`, `i8`, `i32`, ...).

Canonical/backend PYC is scalar-only. Builtin `vector<...>` types and the
removed `pyc.v_*` family are rejected. Recursive ACIR aggregates cross this
boundary as exact-width packed integers with descriptor metadata retained in
ACIR/QueueGraph manifests.

## Operations

All examples below live inside a standard MLIR `module { ... }`. Hardware
definitions use a source-owned `pyc.module` family containing ordered
`pyc.module.case` regions; executable cases terminate with `pyc.return`.

### `pyc.constant`

```mlir
%c1 = pyc.constant 1 : i8
```

### Combinational ops

```mlir
%y = pyc.add %a, %b : i8
%y = pyc.sub %a, %b : i8
%y = pyc.mul %a, %b : i8
%y = pyc.udiv %a, %b : i8
%y = pyc.urem %a, %b : i8
%y = pyc.sdiv %a, %b : i8
%y = pyc.srem %a, %b : i8
%y = pyc.select %sel, %a, %b : i1, i8, i8 -> i8
%y = pyc.and %a, %b : i8
%y = pyc.or  %a, %b : i8
%y = pyc.xor %a, %b : i8
%y = pyc.not %a : i8

%p = pyc.cmp %a, %b {predicate = "eq"} : i8, i8 -> i1
%p = pyc.cmp %a, %b {predicate = "ult"} : i8, i8 -> i1
%p = pyc.cmp %a, %b {predicate = "slt"} : i8, i8 -> i1

%lo = pyc.trunc %x : i64 -> i32
%zx = pyc.zext  %x : i8  -> i64
%sx = pyc.sext  %x : i8  -> i64

%s = pyc.extract %x {lsb = 4} : i16 -> i8
%two = pyc.constant 2 : i16
%sh = pyc.shl %x, %two : i16, i16
%lsh = pyc.lshr %x, %two : i16, i16
%ash = pyc.ashr %x, %two : i16, i16

%bus = pyc.concat(%a, %b, %c) : (i8, i16, i1) -> i25
```

### Semantic bit primitives

These operations are vendor-neutral canonical PYC. Their inputs are limited to
1–64 bits, and their result widths are verified:

```mlir
%index, %valid = pyc.priority_encode %mask {order = "low"} : i13 -> i4, i1
%population = pyc.popcount %mask : i13 -> i4
%leading = pyc.count_zeros %mask {direction = "leading"} : i13 -> i4
%trailing = pyc.count_zeros %mask {direction = "trailing"} : i13 -> i4
```

- `pyc.priority_encode` selects the lowest or highest asserted bit. A zero input
  returns `index = 0` and `valid = 0`.
- `pyc.popcount` returns the number of asserted bits.
- `pyc.count_zeros` counts leading or trailing zero bits. An all-zero input
  returns the input width.

The canonical operations remain in C++ reference simulation. The Verilog-only
selection pass may replace them with a qualified implementation from the
closed RTL catalog.

### `pyc.rtl.comb` (backend-owned implementation)

`pyc.rtl.comb` is internal backend IR emitted only by
`pyc-select-rtl-primitives`. It records a selected implementation, typed port
names, parameters, relative sources, license identity, and catalog provenance.
Frontend or persisted canonical PYC containing this operation is rejected.

### `pyc.alias` (debug naming)

`pyc.alias` is a pure identity op used to attach stable debug names for codegen:

```mlir
%y = pyc.alias %x {pyc.name = "foo__my_file__L42"} : i8
```

Backends use the `pyc.name` attribute (not `name`) to avoid conflicts with other
ops that legitimately use a `name` attribute (e.g. memory instances).

### `pyc.instance` (hierarchical instantiation)

`pyc.instance` instantiates a case of another `pyc.module` family while
preserving module boundaries (for big designs / readable codegen). An
unparameterized family has one empty case and every instance carries the
complete empty dependent-argument tuple:

```mlir
%out_valid, %out_data = pyc.instance %clk, %rst, %in_valid, %in_data, %out_ready
  {callee = @Core, name = "core0", static_args = #ac.dependent_arguments<[]>}
  : (!pyc.clock, !pyc.reset, i1, i32, i1) -> (i1, i32)
```

Attributes:

- `callee`: `FlatSymbolRefAttr` (required) referencing a `pyc.module`
- `name`: non-empty `StringAttr` (required) readable instance name
- `static_args`: complete typed `DependentArgumentsAttr` (required)

Verifier contract:

- `static_args` must select exactly one declared case.
- Operand count/types **must** match that case's physical inputs.
- Result count/types **must** match that case's physical results.

The case signature carries the logical interface, physical function type,
control origins, and complete logical-to-physical mappings consumed by both
backends. Clock and reset are mandatory implicit physical inputs 0 and 1.

Boundary-dynamic value parameters remain logical runtime ports:

- Frontend may stamp:
  - `pyc.value_params = ["name0", ...]`
  - `pyc.value_param_types = ["iN"|!pyc.clock|!pyc.reset, ...]`
- These names are a subset of `arg_names`.
- Value params are runtime boundary ports only; they must not participate in
  compile-time specialization identity.

Static build kwargs and caller-observed specialization are not a finite family
declaration and therefore fail closed. A parameterized family must declare its
ordered typed parameters and complete finite case inventory at its source;
symbols, suffixes, string `pyc.params`, and observed call sites carry no family
identity.

Hierarchy preservation policy:

- `pycc` strict mode (`--hierarchy-policy=strict`, default) enforces that
  frontend module symbol boundaries are preserved through lowering.
- Default C++ out-dir flows use `--inline-policy=off` so `@module` callsites
  remain explicit instance boundaries.

### `pyc.assert` (verified assertion)

`pyc.assert` aborts when `cond` is false. An ordinary assertion may carry only
`msg`. An architecture assertion carries the complete verified obligation
record: stable ID, closed safety kind, severity, pre-publish sampling contract,
anchor, source, and ordered NDF IDs. Partial metadata, liveness kinds,
non-ASCII diagnostics, and unsupported sampling fail verification.

C++ emits an unconditional runtime check, per-obligation check/failure
counters, and a trace/probe condition path containing the stable ID. Verilog
emits a named concurrent SVA assertion and matching coverage property under
`ifndef SYNTHESIS`; synchronous applicability/disable semantics are already
part of the shared Boolean condition, so the backend does not invent an
asynchronous `disable iff`.

```mlir
pyc.assert %ok {msg = "in_ready must not be asserted while full"}
```

Attributes:

- `msg`: `StringAttr` (optional) human-readable message
- architecture-only: `obligation_id`, `obligation_kind`, `severity`,
  `sampling_kind`, `sampling_edge`, `sample_anchor`, `source`, and `ndf_ids`

Runtime-checked obligations are verification evidence, not synthesis proof.
`flows/tools/check_generated_rtl.py --require-synthesis-admissible` rejects RTL
that still contains them.

An architecture assertion may also carry an optional `cover` condition. The
assertion condition remains the safety property; the cover condition is the
event whose occurrence must be observed. Generated C++ gives it a distinct
`_coverage` counter and RTL uses it as the SVA cover-property expression.
Decision 0279 uses this split so `no_stale_update` asserts that stale and
published cannot coincide while coverage observes the stale request itself.

### Recovery and versioned identity lowering

Decision 0279 keeps recovery semantics in ACIR and lowers one verified form to
PYC. Recovery domains, explicit transaction references, execution attempts,
checkpoint/retained-result declarations, and complete VersionedTable metadata
are compiler-owned. `ac.recovery.event` and the closed
`epoch_mismatch_or_younger` KillSet become ordinary PYC comparisons. A
qualified update or invalidation becomes a write enable conjoined with the
committed entry's valid, generation, recovery-epoch, and optional attempt
fields. `ac.versioned_table.lookup` produces payload plus the same qualified
valid predicate. A retained-result `retain` instead conjoins `!valid`, so an
unconsumed result cannot be overwritten; `consume` uses the exact identity
qualification before clearing valid.

The PYC state remains an explicit deterministic register bank; no backend
reconstructs identity metadata. Each qualified write emits one stable
`no_stale_update:<table>:<firing>:slotN` assertion with the exact stale event as
its optional cover condition. Plain `ac.table.propose`, partial identity, and
unqualified versioned writes are rejected before PYC emission.

### Multi-lane transaction algebra lowering

Decision 0280 keeps lane-mask algebra in ACIR and lowers one verified form to
PYC. All six endpoints are compiler-owned and internal; no Python API is
admitted. `ac.reservation_set` becomes one bitwise AND chain over the exact
lane masks, and a `commit` ReservationSet is verified to carry one identical
accepted mask per resource owner before lowering. `ac.transaction_group`
becomes an AND for `independent`, a masked equality select for `all_or_none`,
and one low-to-high `valid && reserved` prefix scan built from
`pyc.extract`/`pyc.concat` for `valid_prefix`; the policy is never inferred from
source order.

`ac.multi_allocator` becomes one popcount plus two lane-order prefix scans: the
requested prefix that still fits free capacity, and the lowest free slots that
cover the accepted count, with `pyc.zext`/`pyc.add` accounting and a closed
same-cycle reuse policy. `ac.age_select_k` lowers to iterative `oldest_first`
reduction over the per-lane ages with lower-lane tie-breaking.
`ac.dependency_set` lowers the exact
`(current | add) & ~((resolve | kill) & identity_match)` expression and its
zero-readiness predicate. `ac.terminal_transaction` conjoins the committed,
effect-done, and terminal masks. Every lane mask keeps its exact `iLanes` type,
and the QueueGraph plan re-verifies lane count, widths, and closed policy
fail-closed before PYC, gfsim C++, or RTL emission.

### Core-local memory ordering lowering

Decision 0281 keeps Core-local memory ordering in ACIR and lowers one verified
form to PYC. `ac.memory_order_edge` becomes one bitwise AND chain over the
producer, consumer, and optional typed proof masks; its closed kind classifies
the relation and never changes the extent. `ac.load_disposition` lowers to the
same lane algebra in PYC, gfsim C++, and RTL: an identity-mismatched or
flush-invalidated lane is `stale = pending && (!identity || killed)`, the
remaining qualified lanes split into `forward = qualified && alias && ready &&
!executed`, `replay = qualified && alias && executed`, and `bypass =
qualified && disjoint && !alias`, and `wait` is the remainder. Identity
qualification is a typed mask, so no consumer predicate enters the lowering, and
the plan re-verifies lane counts, widths, closed kinds, and the disposition
operands fail-closed before emission.

A stale response is consumed rather than accepted. Each disposition emits one
stable `no_stale_response:<anchor>:disposition<ordinal>` assertion with a cover
condition on the stale event. The condition is a structural self-consistency
guard over the generated masks rather than an independent proof; the cover
condition is the load-bearing part and gives the event its coverage. Generated
PYC C++ gives that cover a distinct `_coverage` counter and RTL uses it as the
SVA cover property; both carry the same obligation ID, kind, severity,
`pre_publish` sampling point, anchor, and message as PYC, while the gfsim C++
path computes the predicate without materializing the assertion. Two limits are recorded rather than hidden: the kill path takes a typed
lane invalidation mask because `ac.kill_set` is scalar, and the pending-load set
remains fixture- or consumer-owned state.

### `pyc.sync_mem` / `pyc.sync_mem_dp` verification profile

Every synchronous memory carries `live_window = 1`. The value is a static
verification contract, not memory depth: Q begins unknown, an enabled read
makes Q live after its capture edge for one use cycle, a back-to-back read
refreshes that window, and the next edge without a read invalidates Q.

The C++ verification model exposes `FourState<W>` observations with exact
`value`, `knownMask`, and `zMask`; parity requires equal masks and equal value
bits only where known. `(known & z) == 0` is mandatory. The RTL primitive enables
the same aggressive behavior only under `PYC_VERIFY_AGGRESSIVE_SRAM`, keeping
the technology-independent synthesis body unchanged. Enabled reset/read/write
controls, addresses, write data, and strobes must be known; inactive data paths
may remain unknown.

The public SRAM helper always captures raw Q and selects live Q in the capture
cycle, then the captured register afterward. This explicit select preserves
NBA visibility without extending the primitive Q lifetime.

### `pyc.wire` / `pyc.assign` (netlist backedges)

PYC uses a netlist-style wire placeholder with an explicit driver:

```mlir
%d = pyc.wire : i64
pyc.assign %d, %next : i64
```

`pyc.assign` destinations must be defined by `pyc.wire`. This is used to model
feedback loops (state machines) in SSA-based MLIR.

### `pyc.reg` (clocked register)

```mlir
%q = pyc.reg %clk, %rst, %en, %next, %init : i8
```

Semantics (single register):

- On `posedge %clk`:
  - if `%rst` then `q := init`
  - else if `%en` then `q := next`

Common pattern (backedge):

```mlir
%d = pyc.wire : i8
%q = pyc.reg %clk, %rst, %en, %d, %init : i8
pyc.assign %d, %next : i8
```

### `pyc.fifo` (strict ready/valid)

```mlir
%in_ready, %out_valid, %out_data =
  pyc.fifo %clk, %rst, %in_valid, %in_data, %out_ready {depth = 2} : i8
```

Handshake semantics:

- **Push** occurs when `in_valid && in_ready`
- **Pop** occurs when `out_valid && out_ready`

Notes:

- `pyc.fifo` is **single-clock** (one `%clk`, one `%rst`).
- Cross-clock FIFOs should use `pyc.async_fifo` (dual-clock, strict ready/valid).

### `pyc.comb` / `pyc.yield` (fused combinational regions)

`pyc.comb` is a codegen-oriented wrapper for fusing many small pure comb ops
into a single region:

```mlir
%y = pyc.comb(%a, %b) : (i8, i8) -> i8 {
  ^bb0(%a0: i8, %b0: i8):
    %t = pyc.add %a0, %b0 : i8
    pyc.yield %t : i8
}
```

## Verilog backend

`pycc --emit=verilog` emits Verilog:

- **Combinational ops** are typically emitted as flattened `assign` expressions (netlist style).
- **Stateful ops** instantiate the corresponding primitives from `library/verilog/`:
  - `pyc.reg` → `pyc_reg` (`library/verilog/pyc_reg.v`)
  - `pyc.fifo` → `pyc_fifo` (`library/verilog/pyc_fifo.v`)
  - `pyc.async_fifo` → `pyc_async_fifo` (`library/verilog/pyc_async_fifo.v`)
  - `pyc.byte_mem` → `pyc_byte_mem` (`library/verilog/pyc_byte_mem.v`)
  - `pyc.sync_mem` → `pyc_sync_mem` (`library/verilog/pyc_sync_mem.v`)
  - `pyc.sync_mem_dp` → `pyc_sync_mem_dp` (`library/verilog/pyc_sync_mem_dp.v`)
  - `pyc.cdc_sync` → `pyc_cdc_sync` (`library/verilog/pyc_cdc_sync.v`)

`pycc` also runs `pyc-fuse-comb`, which enables emission of flattened
Verilog `assign` statements for large purely-combinational regions.

Generated RTL family names are readable lower-snake spellings of source family
symbols. Module-name collisions fail before emission. The structural audit
tool checks ASCII, explicit-width assignment literals, named port connections,
dead internal nets, stable module/net/instance/assertion order, matched
assertion/coverage IDs, and deterministic two-build output. Its JSON report
also records expensive-operation counts; `--html-out` produces a reviewable
colored comparison artifact.

## Structured control flow (frontend temporary IR)

The Python AST/JIT frontend may emit a small subset of standard MLIR dialects:

- `scf.if` / `scf.for` (Structured Control Flow)
- `arith.constant` of type `index` (loop bounds)

These are **not** part of the stable PYC dialect contract: `pycc` runs
`pyc-lower-scf-static` to lower them into static PYC hardware ops:

- `scf.if` → `pyc.select` networks (both branches are speculated; must be side-effect-free)
- `scf.for` → fully unrolled logic (bounds must be compile-time constants)

Note: MLIR canonicalization may also introduce `arith.select` during cleanup.
`pycc` supports `arith.select` in both C++ and Verilog emission, but it
is not considered part of the stable PYC dialect surface area.
