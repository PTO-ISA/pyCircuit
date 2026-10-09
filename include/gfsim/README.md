# Typed gfsim modules

`SimObj` is the abstract lifecycle contract. `SimModule` is a concrete empty
module; a definition adds ports, Work rules and child instances. DFF/DFFE and
memory are leaf module definitions derived from the same `SimModule` class.
Each constructed instance owns its own leaf state. Parents call their children;
there is no implicit registration or scheduling in the base class.

## Signals and payloads

Every hardware port uses `wire<T>`. A scalar payload is `Bits<N>` or a finite hardware struct
with an explicit static layout. `Bits<N>` is the binary payload representation;
`wire<T>` owns the packed value, known and Z bit planes through `FourState`.
Its default value is unknown. Copying a wire preserves all three planes and
adds no storage delay. The old binary `Wire<N>` alias is removed.

`hardware_traits<T>` defines `width`, `pack(const T&)` and
`unpack(const Bits<width>&)`. A struct specialization can inherit
`hardware_struct_traits<T, &T::field...>` in declaration order. Nested fields
use their own traits. The first field occupies the most significant bits;
there is no host padding, alignment, `sizeof` or byte-copy interpretation.
Traits and their field assignments must be nonthrowing. Repeated member
pointers are rejected; C++20 cannot discover omitted fields, so the complete
field list remains a declaration obligation. Compiler generation produces this complete field list from the ACIR struct declaration.

For example:

```cpp
struct Packet {
  gfsim::Bits<1> valid;
  gfsim::Bits<32> data;
};

template <>
struct gfsim::hardware_traits<Packet>
    : gfsim::hardware_struct_traits<Packet, &Packet::valid, &Packet::data> {};

gfsim::wire<Packet> packet;
gfsim::dffe<Packet> state("state");
```

`wire<T>::known(value)` packs a known value. `unknown()` creates an unknown
signal; `fromPacked()` retains all masks from a `FourState` value. `packed()`
provides the full representation. `value()` is an explicitly binary payload
view: check `isFullyKnown()` before using it as known hardware data.

## Component interfaces

| Definition | Template parameters | Data pins |
| --- | --- | --- |
| `dff` | `T = Bits<1>` | `d`, `init`, `q`: `wire<T>` |
| `dffe` | `T = Bits<1>` | `d`, `init`, `q`: `wire<T>` |
| `sync_mem` | `T = Bits<64>`, `ADDR_WIDTH = 64`, `DEPTH = 1024` | `rdata`, `wdata`: `wire<T>` |
| `sync_mem_dp` | Same | `rdata0`, `rdata1`, `wdata`: `wire<T>` |
| `byte_mem` | Same | `rdata`, `wdata`: `wire<T>` |

Clock, reset, enable and read/write controls use `wire<Bits<1>>`. Addresses
use `wire<Bits<ADDR_WIDTH>>`. Strobes start at packed bit zero: synchronous
memory uses ceil(payload width / 8), including a partial final byte;
byte memory requires a whole-byte payload. Synchronous DEPTH counts entries;
byte-memory DEPTH counts bytes. There is no old WIDTH/DATA_WIDTH template route.

## Work and Xfer

Work invokes private rule helpers for current-state reads, clock/control
checks and next-state candidates. It never changes committed storage. Memory
address checks and write-lane preparation occur in Work. Xfer publishes the
prepared storage, read-Q, clock-history and lifetime changes. DiscardNext
cancels all of them. Rules are local subfunctions, not runtime objects or a
new lifecycle phase. No Eval hook is present.

DFF/DFFE Q starts unknown. A rising edge captures init when synchronous rst is
active; otherwise DFF captures d and DFFE captures only when en is true.
Data/init retain X/Z masks. Reset() is a host initialization hook, distinct
from the clocked rst pin; it prepares initialization for Xfer.

Synchronous memory reads old storage before concurrent writes commit. A read
publishes known Q; the first subsequent idle rising edge holds it and the
second makes it unknown. Each dual-read output has its own lifetime. Writes
do not refresh Q. Reset invalidates Q and suppresses writes without clearing
storage. Simulation memory contents start at zero.

Full addresses are checked before narrowing: invalid reads return known zero
and invalid writes are ignored. Byte lanes are checked separately at the end
of storage, without address-addition overflow. Enabled unknown controls,
addresses, write data or strobes are rejected; disabled data/address pins can
be unknown. The continuously enabled byte-memory read requires a known address.
Memory therefore retains its known-data storage contract; typed payloads do
not silently authorize storing unknown write data.

Module combinational outputs are observed after successful Work. Xfer alone
does not refresh cached parent outputs or byte-memory read pins. The private byte-memory kernel exposes a pure read helper for combinational
reset-initializer dependencies; Work retains its existing address diagnostics.
This is not a SimObj lifecycle hook or an arbitrary feedback scheduler.

## FIFO storage kernel

`fifo.h` provides `fifo_kernel<T, Depth, QueueReadyPolicy, AvailabilityLatency>` for the existing
`collection_storage` lifecycle. Each lane owns a fixed array of `wire<T>` tokens,
read/write positions and occupancy. AvailabilityLatency is an explicit positive
uint64 template argument; generated callers always supply it. Bits, nominal hardware structs and fixed
table tokens use their existing hardware traits and retain all value/known/Z
planes. Queue depth counts complete tokens; table elements remain within a token.

`readValid`, `readData` and `readReady` inspect committed state without preparing
Work. `LocalOccupancy` ready is true only below capacity. Explicit
`DownstreamPop` also permits replacement when a full queue pops on that edge.
There is no empty flow-through. At latency L, a birth on E0 becomes available
after E(L-1) Xfer and can first be popped on EL. All waiting tokens reserve
capacity. Arbitrary positive depths wrap at `Depth - 1`; at L1, depth one with
local ready has half-rate steady throughput. An initialized unavailable head
reads packed known zero, including when waiting tokens occupy the queue.

Construction leaves valid, ready and data unknown. Host reset prepares a reset
transaction with proposed clock known false; only Xfer establishes known empty
metadata. A known rising hardware
reset has the same metadata effect and masks handshake controls. Nonrising Work
only prepares clock history. Effective unknown push/pop controls fail Work and
discard its proposal; inactive unknown controls remain masked. An uninitialized
queue can accept a known-zero-transfer edge without becoming initialized.

Work stages at most one token and metadata. Xfer commits them after the existing
whole-system success check; discard cancels data, reset, clock and age proposals.
Neither Work, Xfer nor reset uses the borrowed Outputs descriptor as state or
refreshes its wires. Callers publish pure reads as their Work snapshot. Reset
does not clear the slot array; invalidated tokens cannot become visible again.
Normal lifecycle operations allocate no storage and never copy or scan the
whole queue. Token copying scales with payload size, not queue depth.

L1 uses an empty timing substate. At L>1, per-slot modulo deadlines and a
maturation cursor/eligible-prefix count advance at most one token per committed
rising edge. Eligibility persists through any number of timestamp wraps under
backpressure. Idle requests still age; held/falling clocks and discarded or
failed transactions do not. Full-u64 latency uses defined unsigned wrap without
forming an overflowing modulus or shifting by 64.

The common QueueOp and Python `ac.queue` lower through this kernel and the
equivalent RTL helper. Their admission, source-unit and target allocation
boundaries remain the compiler’s responsibility; see the queue language
reference and candidate-bound implementation progress.

## Typed DUT and parallel runner

Generated `pycircuit_system.hpp` supplies `pyc_dut::Inputs`, `Outputs`, `drive`,
`sample`, `system` and `observations`. The DUT owns a root and a `WorkExecutor`;
module instances borrow that executor. Control-thread drive/sample calls occur
between epochs and must not race Work. No shared-library port C ABI is added.

The compiler may delegate independent complete instance subtrees as WorkItems.
Inputs are prepared before dispatch, each subtree runs once, and collections
remain whole tasks. The executor joins every worker before returning or
rethrowing the first error in stable task order. Same-pool nested batches run
inline to avoid deadlock. System checks and serial Xfer follow successful Work;
a failure discards the whole tree before any commit.

`SystemRunner` accepts `--workers N` and borrowed `RunnerCallbacks`: initialize
after configuration and before Reset, drive before Step, sample after a
successful committed Step. Drive can return false to finish without another
Step. A driven Step counts one sampling epoch; its sample payload is still the
Work/precommit output. `cycles()` and runtime cycle statistics count those
epochs. Use finite `max_ticks`; driven mode rejects nonempty domain-cycle limits
because the host controls the clock waveform. A complete clock period is not
inferred from a port name or from one Step.

`sample()` rejects before the first successful epoch and in the failed state.
Host Reset prepares initialization for Xfer and preserves memory contents;
construct a fresh DUT for an independent cold run.

## Dense collections

`table<T,Extents...>` is a finite row-major payload. `wire<table<...>>::element(i)`
borrows a scalar wire, with a const overload for reading. Explicit packed
conversion places element zero at the MSBs and preserves value/known/Z.
`wire<T>::assignSlice` updates only touched words of a candidate buffer.

`collection_storage<Kernel>` owns preallocated plain Current/Pending arrays;
it contains no per-element SimModule or shared_ptr. Scalar leaf modules and
collections call the same leaf kernels. A failed lane clears every pending
candidate and clock; partial preparation cannot commit. Generated family owners
use shared_ptr at construction and borrow data during Work. Structural instance
names and logical indices identify state independently of source occurrence
metadata or template reuse.

The Runtime CMake component installs these headers and does not require LLVM.
Current candidate gates cover scalar/collection execution, packed X/Z, memory
lifetime, rollback, source generation and standalone Runtime consumers; see
[the collection implementation packet](../../docs/work-items/collection-implementation.md).
Python collection authoring and the standalone runner/DUT ABI are separate work.
