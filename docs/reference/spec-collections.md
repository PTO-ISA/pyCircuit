# Table values and module collections

The verified ACIR and the `cpp`/`verilog` emitters support the approved
collection contract B (historical local record).
This adds an IR/runtime capability; it does not restore the retired Python
module-family/QueueGraph API or introduce a new Python collection syntax.

`ac.collection` repeats one module definition with one set of integer/type
bindings and a positive static shape. Each element and its storage descendants
remain independent. A collection has SSA input/output values of `!ac.table`;
it does not create a mutable connection graph or an array of module handles.
Hardware SSA values already denote wires, including module inputs and outputs.

`!ac.table<shape,T>` has row-major logical order, with the last dimension varying
fastest. Consecutive nested table shapes normalize into one shape. Element zero
occupies the most significant packed bits; a struct element retains its declared
MSB-first layout. Table values do not allocate registers. State is owned by the
standard DFF/DFFE/memory leaves.

Elements may include [nominal Enum values](spec-enums.md), directly or inside a
struct. Generic type bindings preserve their identity and four-state carrier.
The existing unsigned fold operations still require bits, with explicit Enum
conversion where arithmetic is intended.

| Operation | Behavior |
| --- | --- |
| `ac.table.create`, `splat` | Construct explicit elements or broadcast a value |
| `ac.table.map` | Pure, explicitly captured scalar SSA calculation per element |
| `ac.table.view` | Verified static reshape, slice, transpose or rotate |
| `ac.table.index` | Convert full-width coordinates to a row-major ordinal; known OOB yields sentinel N |
| `ac.table.get` | Read a table wire by ordinal; known OOB yields X/0, unknown index X/X |
| `ac.table.match` | Pure predicate; mask bit k denotes logical element k |
| `ac.table.choose` | `first`, explicit low/high order and count; indices precede valid flags |
| `ac.table.fold` | Existing closed unsigned fold kinds, balanced adjacent pairs and odd-tail carry |
| `ac.value.merge` | Verify disjoint field paths and produce complete next/en wires |

Choose's zero index on no match is a placeholder: its valid flag must be used.
Get's in-range flag does not imply that a preceding choose found an element.
Index arithmetic, bounds and bit widths use exact static calculations before
emission. Constant propagation does not turn a total get/index into a verifier
failure. Four-state masks retain the specified select/compare behavior.

Published module dependency summaries retain the original port and element
field path across a compact table domain. Table axes have no lane ordinals in
that summary. Dependency classification uses the logical element payload:
tables of scalar bits or Enum payloads can influence a scalar or differently shaped
view; nominal struct prefixes retain exact corresponding-field rules. Shape
legality belongs to operation and instance checks. A dependency does not imply
broadcasting or corresponding lanes. Cross-element graph precision remains
conservative, and unrelated struct fields must not become dependencies.

DFFE retains one-bit enable and whole-T data. A merge rejects repeated or
ancestor/descendant field paths even if guards appear mutually exclusive;
priority must be explicit before the merge. Work reads old Q and prepares next;
Xfer commits. Failed family Work discards candidates and pending clock state.

C++ uses `wire<table<T,Extents...>>` with borrowed element access. One generated
family template per definition and dense plain leaf state avoid per-element
SimModule objects and owner-pointer copies in Work. Scalar and bulk leaves use
the same kernels under `include/gfsim`. RTL uses packed table wires and generate
loops with the existing `include/verilog` leaves. The selected root wrapper
retains the system's integer and type bindings.

Generated CMake builds `pycircuit_modules` against the Runtime component.
The C++ aggregate header is `pycircuit_system.hpp`, with `pyc_root` naming the
selected bound module type. The RTL build selects the bound `pyc_root` wrapper.
Clock/input stimulation belongs to the caller; no runner or DUT C ABI is emitted
by this module-source packet.

Only physical layout hints are admitted. Shape-dependent parameter values are
checked in each concrete instance binding without cloning definitions. Both
targets reject planes exceeding the current Runtime capacity before publication,
including multiplication by enclosing collection counts. Constrained layouts,
stateful arbitration/flow control, dynamic shape, heterogeneous collections and
Table fields inside structs remain outside this packet.

Executable syntax and independent expectations live in
[`tests/compiler/lit`](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/compiler/lit); implementation status and
candidate evidence are tracked in the work packet (historical local record).
