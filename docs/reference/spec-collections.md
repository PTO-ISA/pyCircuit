# Component Tables, packed Arrays and module collections

## Component Tables

A Module declares a fixed container of independent components:

```python
regs: ac.table[4, ac.u8] = (1, 2, 3, 4)
entries: ac.table[4, Entry] = Entry(count=0, tag=7)
workers: ac.table[4, Worker]
```

Data members are independent State resources. A scalar or Struct initializer
broadcasts its value; a tuple/list initializes the complete member image.
Omitting an initializer gives data members the ordinary all-X startup and reset
value. Module members use their provider's default static parameters and must
have no required runtime input connections. Selecting a member never allocates
or copies a component.

`regs[index]` reads old Q in a payload expression; assignment proposes a write.
`entries[index].count = value` preserves the other fields of the selected State.
`worker = workers[index]` keeps the selected instance identity, and
`worker.process(value)` uses the ordinary Method interface. All proposals,
checks, observations and nested calls belong to the enclosing Rule transaction.
Only the selected Method's guard contributes admission; a later failed call
cancels every proposal of that Rule. Independent member Rules remain independent.

`len(table)` is a static capacity. `for member in table` and
`enumerate(table)` visit existing members through the ordinary static loop
representation. Capacity may use a bound static parameter and must resolve to
a positive integer. This version supports one dimension and does not transport
whole Tables through Module inputs or results.

A dynamic hardware Bits index needs two independent compiler proofs: it is
known (contains no X/Z), and its full value is in `[0, capacity)`. A `u2` range
fits four members, but width alone proves neither knownness nor a three-member
bound. There is no implicit truncation, wraparound, default member or arbitration.
Potentially aliasing writes use the ordinary resource conflict analysis.

Source ACIR uses reference `!ac.table<capacity, element-reference>` and
`ac.table.alloc`, `ac.table.create`, `ac.table.select`. Link establishes members
as ordinary State/Module instances and folds constant selections. Dynamic
`table.select` plus `method.call` remains in Linked ACIR; common transaction
analysis and Routed lowering produce selection paths, results and enables.
Both physical C++ and RTL consume the verified final artifact. The direct
`t-gfsim` target does not admit component Tables in this version.

## Retained packed Array and Collection IR

The verified ACIR and the `cpp`/`verilog` emitters support the approved
collection contract B (historical local record).
This adds an IR/runtime capability; it does not restore the retired Python
module-family/QueueGraph API or introduce a new Python collection syntax.

`ac.collection` repeats one module definition with one set of integer/type
bindings and a positive static shape. Each element and its storage descendants
remain independent. A collection has SSA input/output values of `!ac.array`;
it does not create a mutable connection graph or an array of module handles.
Hardware SSA values already denote wires, including module inputs and outputs.

`!ac.array<shape,T>` has row-major logical order, with the last dimension varying
fastest. Consecutive nested Array shapes normalize into one shape. Element zero
occupies the most significant packed bits; a struct element retains its declared
MSB-first layout. Packed Array values do not allocate registers. State is owned by the
standard DFF/DFFE/memory leaves.

Elements may include [nominal Enum values](spec-enums.md), directly or inside a
struct. Generic type bindings preserve their identity and four-state carrier.
The existing unsigned fold operations still require bits, with explicit Enum
conversion where arithmetic is intended.

| Operation | Behavior |
| --- | --- |
| `ac.array.create`, `splat` | Construct explicit elements or broadcast a value |
| `ac.array.map` | Pure, explicitly captured scalar SSA calculation per element |
| `ac.array.view` | Verified static reshape, slice, transpose or rotate |
| `ac.array.index` | Convert full-width coordinates to a row-major ordinal; known OOB yields sentinel N |
| `ac.array.get` | Read a table wire by ordinal; known OOB yields X/0, unknown index X/X |
| `ac.array.match` | Pure predicate; mask bit k denotes logical element k |
| `ac.array.choose` | `first`, explicit low/high order and count; indices precede valid flags |
| `ac.array.fold` | Existing closed unsigned fold kinds, balanced adjacent pairs and odd-tail carry |
| `ac.value.merge` | Verify disjoint field paths and produce complete next/en wires |

Choose's zero index on no match is a placeholder: its valid flag must be used.
Get's in-range flag does not imply that a preceding choose found an element.
Index arithmetic, bounds and bit widths use exact static calculations before
emission. Constant propagation does not turn a total get/index into a verifier
failure. Four-state masks retain the specified select/compare behavior.

Published module dependency summaries retain the original port and element
field path across a compact table domain. Array axes have no lane ordinals in
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
Array fields inside structs remain outside this packet.

Executable syntax and independent expectations live in
[`tests/compiler/lit`](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/compiler/lit); implementation status and
candidate evidence are tracked in the work packet (historical local record).
