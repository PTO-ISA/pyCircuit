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

All examples below live inside a standard MLIR `module { ... }` and use
`func.func` as the top-level hardware-module container.

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
digest-verified RTL catalog.

### `pyc.rtl.comb` (backend-owned implementation)

`pyc.rtl.comb` is internal backend IR emitted only by
`pyc-select-rtl-primitives`. It records a selected implementation, typed port
names, parameters, source digests, license identity, and catalog fingerprint.
Frontend or persisted canonical PYC containing this operation is rejected.

### `pyc.alias` (debug naming)

`pyc.alias` is a pure identity op used to attach stable debug names for codegen:

```mlir
%y = pyc.alias %x {pyc.name = "foo__my_file__L42"} : i8
```

Backends use the `pyc.name` attribute (not `name`) to avoid conflicts with other
ops that legitimately use a `name` attribute (e.g. memory instances).

### `pyc.instance` (hierarchical instantiation)

`pyc.instance` instantiates another `func.func` hardware module while preserving
module boundaries (for big designs / readable codegen):

```mlir
%out_valid, %out_data = pyc.instance %clk, %rst, %in_valid, %in_data, %out_ready
  {callee = @Core__pdeadbeef, name = "core0"} : (!pyc.clock, !pyc.reset, i1, i32, i1) -> (i1, i32)
```

Attributes:

- `callee`: `FlatSymbolRefAttr` (required) referencing a `func.func`
- `name`: `StringAttr` (optional) instance name for codegen

Verifier contract:

- Operand count/types **must** match the callee’s function type inputs.
- Result count/types **must** match the callee’s function type results.

Backends emit named port connections using the callee’s `arg_names` /
`result_names` attributes.

Boundary-dynamic value params:

- Frontend may stamp:
  - `pyc.value_params = ["name0", ...]`
  - `pyc.value_param_types = ["iN"|!pyc.clock|!pyc.reset, ...]`
- These names are a subset of `arg_names`.
- Value params are runtime boundary ports only; they must not participate in
  compile-time specialization identity.

Hierarchy preservation policy:

- `pycc` strict mode (`--hierarchy-policy=strict`, default) enforces that
  frontend module symbol boundaries are preserved through lowering.
- Default C++ out-dir flows use `--inline-policy=off` so `@module` callsites
  remain explicit instance boundaries.

### `pyc.assert` (simulation-only assertion)

`pyc.assert` is a simulation-only check that aborts when `cond` is false.
Backends emit it under `ifndef SYNTHESIS` in Verilog, and as a runtime check in
the C++ model.

```mlir
pyc.assert %ok {msg = "in_ready must not be asserted while full"}
```

Attributes:

- `msg`: `StringAttr` (optional) human-readable message

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
