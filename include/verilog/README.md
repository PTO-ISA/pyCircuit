# Typed Verilog storage modules

These `.v` files use SystemVerilog syntax. The public emission target remains
`verilog`; compile the files in SystemVerilog mode. Their C++ counterparts are
in `include/gfsim`.

## Payload and ports

`dff` and `dffe` declare `parameter type T = logic [0:0]`. The memory modules
declare `T = logic [63:0]`, followed by `ADDR_WIDTH` and `DEPTH`. Data width is
`$bits(T)`, not a separate WIDTH/DATA_WIDTH parameter.

`fifo` takes `T`, signed 64-bit `DEPTH`, and `READY_POLICY`. Generated instances
provide all three explicitly. Depth must be positive; policy 0 means local
occupancy and policy 1 permits full replacement on a downstream pop. The
default depth 0 and policy -1 are invalid sentinels.

T is a finite four-state packed bit vector or a packed struct of such fields,
including nested structs. Hardware bits map to `logic`; the SystemVerilog
keyword `bit` is a two-state type and cannot preserve the required X/Z values.
Simulation initialization checks that an all-X payload survives conversion
through T and back to packed bits. The X type probe is omitted on Verilator,
which is a two-state engine; separate four-state simulation validates X/Z. This checks whole-payload X preservation;
it is not reflection or validation of each declared member type.

Data ports are `input wire T` / `output wire T`. Direction belongs to the
port; the same payload type is used for inputs, outputs and internal nets.
A struct's first declared field occupies its most significant bits, recursively
with no padding. Packed storage and strobe indices are normalized from bit zero,
independently of the declared index range of T.

| Module | Behavior | Depth unit |
| --- | --- | --- |
| `dff` | Rising-edge capture, synchronous active-high reset to init | — |
| `dffe` | Same reset priority; disabled data holds Q | — |
| `fifo` | Complete-token FIFO, synchronous active-high reset to empty | T tokens |
| `sync_mem` | 1R1W old-data synchronous read | T entries |
| `sync_mem_dp` | 2R1W, independent registered read outputs | T entries |
| `byte_mem` | Little-endian continuous read window, synchronous write | Bytes |

Memory strobes control packed low-byte lanes first. Synchronous memory supports
a partial final lane; byte_mem requires `$bits(T)` to be a positive multiple
of eight. Unknown enabled write data remains rejected.

## Work and Xfer organization

Each file separates interface/internal declarations, Work combination, and
Xfer rising-edge state updates. Work computes read values and write candidates;
Xfer commits through nonblocking assignments. Disabled register or memory-write
lanes receive no update. A module definition is shared; each instance owns its
own state. These names are source organization, not additional hardware ports.

Procedural combination is permitted inside these standard storage components.
Ordinary generated design modules continue to use wires, assign expressions
and standard component instances, without their own storage always blocks.

Clocked input diagnostics remain at the rising edge; Work does not move them
to an earlier transient-input check. byte_mem retains its continuous read-address
check. The testbench must establish data and controls before the sampling edge.

## FIFO semantics

FIFO valid and data read committed occupancy and head state. Local ready depends
only on available capacity; replacement-policy ready also depends on a possible
downstream pop. Empty queues do not forward input data. A full simultaneous
request pops only under local policy and pops/pushes under replacement policy.
Positions wrap at `DEPTH - 1`, including nonpower-of-two depths.

Cold outputs are unknown. Reset establishes empty metadata without clearing
payload slots; an initialized empty queue returns packed known-zero data.
Payload X/Z is retained. Rising-edge reset dominates handshakes; otherwise only
unknown effective transfers fail. Held clock levels do not change queue state.
Native whole-system discard/retry is verified separately from RTL fatal checks.
See the [queue contract](../../docs/reference/spec-queues.md) for the shared
Runtime and common-IR semantics and current acceptance boundaries.

## Preserved memory semantics

Complete addresses are compared before narrowing, with no high-bit aliasing.
Invalid reads yield zero; invalid writes are ignored. Byte-memory boundary
lanes are checked without address-addition overflow. A simultaneous synchronous
read and write to one address returns the old data.

Simulation storage is zero-filled; read Q starts X. Reset suppresses writes
without clearing memory. Each synchronous read restarts that output's lifetime:
one idle rising edge holds Q, the second makes it X. Writes do not refresh it,
and the two read ports age independently.

Under SYNTHESIS, diagnostics, simulation initialization and Q lifetime logic
are excluded. Synchronous read Q resets to zero and holds when reading is
disabled. Existing FPGA RAM attributes remain. These simulation conventions
are not a claim of physical memory power-on initialization.

## Generated collections and validation

Generated table ports use the declared MSB-first packed layout. Static generate
loops instantiate standard leaves; ordinary modules use wire/assign networks
for map/view/index/selection and balanced fold. `ac.system` bindings are applied
to a generated `pyc_root` wrapper, which is also the generated CMake top.

Current gates build and execute generated RTL against independent known-value
and four-state oracles, using an actual typed-struct X/Z capability probe.
RAM inference and physical synthesis are not established by these simulation
results. See the [implementation packet](../../docs/work-items/collection-implementation.md).
