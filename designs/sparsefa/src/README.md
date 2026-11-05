# FA2 hardware sources

The P0S closure has four source units, compiled independently:

| Source | Hardware owner |
| --- | --- |
| `fa2_interfaces.py` | Shared row command, memory request/reply and operation status types |
| `fa2_numeric.py` | Stateless encoded binary32 add/mul/div/exp and FP16/BF16 conversion |
| `fa2_engine.py` | One query, exact IndexTrie prefix validation, direct KV AGU, PhaseController and Datapath children, shared scalar ALU, memories and drain |
| `fa2_scheduler.py` | One tensor command, checked layouts and B/H/query loops around one engine instance |

Top: `fa2.fa2_scheduler.FA2TensorScheduler`. The smaller independently useful
row top is `fa2.fa2_engine.FA2Engine`. Structs carry related protocol fields;
independent memory and status outputs use tuple returns and direct unpacking.
Module variables and named Memory own state. The compiler owns storage,
clock/reset, resource admission and transaction controls.

Index is supplied in memory by the caller. Every query validates the complete
active prefix before any KV access. K/V addresses use separate base/stride;
there is no token-to-physical mapping table. One outstanding transaction makes
response ownership explicit; cancellation drains before operation reuse.

The current index candidate uses a 4096-node Patricia trie with full uint32
leaf keys and at most 32 branch decisions per traversal. Original-order indices
remain separate from the 32 KiB node memory. Four mutually exclusive node writes
share one write site inside the same atomic Rule. Product and row tops now link
under default limits and emit both targets; the generated product passes 46
public-port D1 cases across workers 1/2. Five complete System benches still
exceed the original-source proof budget, so integration acceptance remains open.
See [the exact node/FSM contract](../ndf/l2/fa2-core-idx-1003.md) and the current
candidate results in VALIDATION rather than inheriting older baseline passes.

The FP32 carrier is a bit encoding, not integer attention. Read the exact
[RTZ/FTZ profile](../ndf/l1/fa2-core-profile-0001.md) and limitations before using
the hardware. Exp/div are combinational candidates; no clock frequency or
synthesis quality has been established.

## Standalone P0T three-dimensional array

The current parallel design is `fa2_array3d.Array16x16x16`. Its sixteen rows
contain sixteen cells each; every cell owns sixteen distinct `Array3DProduct`
children. There are 4096 spatial multipliers, four registered adjacent-pair
reduction levels, and 256 accumulators. Products round separately from every
tree addition and cross-group addition. The old serial accumulation order is
not the bitwise oracle for this design.

`fa2_array3d_interfaces.py` carries a complete group as one named Struct: two
16×16 binary32 operands, tag, group index, action and row/column/K masks.
The right operand stores columns: `right[n].cK` is B[k,n]. `begin`, `advance`
and `take` are methods on the same static instance; callers issue at most one
public call per edge. An explicit bubble advances existing data, no call holds
all stages, and matching abort immediately discards the tile. An invalid action
returns zero while still advancing old data in ACTIVE/DRAIN; it is distinct
from a transaction that did not commit.

The source closure is numeric → existing array_interfaces (Row16) →
array3d_interfaces → array3d, followed by the independent System bench.
All five source units compile with default limits. The complete System link
exceeds the transaction-plan analysis budget; there is no verified final or
generated C++ execution yet. See [VALIDATION](../VALIDATION.md) for the exact
candidate and evidence. Source compilation is not generated execution evidence.
The [contract](../ndf/l2/fa2-core-array3d-1002.md),
[buffer proposal](../ndf/l2/fa2-core-array3d-buf-1004.md) and
[independent runner](../tests/array3d/README.md) define timing and integration
boundaries. P0S, its precomputed-index input, and the previous 2D array remain
separate; the new array has no SRAM/gather/softmax feeder yet.

## Previous two-dimensional P0T baseline

`fa2_array_interfaces.py` owns the begin/step/result and internal row payloads.
`fa2_array.py` defines `Array16x16`, sixteen `ArrayRow` instances and 256
independent `ArrayPE` accumulators. Related fields travel as Structs; existing
instance methods update state atomically in the caller's Rule. Each accepted
step applies separately rounded `fp_add_value(acc, fp_mul_value(left, right))`.
The pure helpers and the existing P0S Module wrappers share one implementation
in `fa2_numeric.py`; the original P0S numeric and six-System regressions passed
after this extraction.

The array closure is numeric → array_interfaces → array, plus a test-owned
System bench. All four units compile. The full System currently exceeds the
default transaction-state analysis work budget at link; there is no verified
final, generated C++ execution or measured array throughput yet. See
[the array contract](../ndf/l2/fa2-core-array-1002.md) and
[the test runner](../tests/array/README.md). The complete 16×16 structure is
retained. DMA, SRAM staging, softmax/rescale and attention scheduling are
separate integration work.

## Reproduce

The test runners under `../tests/numeric`, `numeric_exact`, `integration` and
`scheduler` exercise the public CLI and preserve stage logs. Each design source
is compiled once; users must relink a coherent full closure after changing any
imported interface or implementation. A stale interface snapshot is a failure,
not a reason to drop dependency checking.

Conceptual product build (substitute absolute paths and unique output paths):

```bash
pycircuit compile -c SRC/fa2_interfaces.py --source-root SRC --package-prefix fa2 -o OUT/interfaces
pycircuit compile -c SRC/fa2_numeric.py --source-root SRC --package-prefix fa2 -o OUT/numeric
pycircuit compile -c SRC/fa2_engine.py --source-root SRC --package-prefix fa2 -I OUT/interfaces -I OUT/numeric -o OUT/engine
pycircuit compile -c SRC/fa2_scheduler.py --source-root SRC --package-prefix fa2 -I OUT/interfaces -I OUT/numeric -I OUT/engine -o OUT/scheduler
pycircuit link OUT/interfaces OUT/numeric OUT/engine OUT/scheduler --top fa2.fa2_scheduler.FA2TensorScheduler -o OUT/final.ac
pycircuit emit OUT/final.ac --target cpp -o OUT/cpp
pycircuit emit OUT/final.ac --target verilog --rtl-mode managed -o OUT/verilog
```

These commands describe the workflow; actual outcome and remaining failures are
in [VALIDATION.md](../VALIDATION.md). Generated System tests add a test-owned
bench unit and choose its System top without injecting host-produced DUT data.
Old raw-map/control prototypes are retained only in `../historical/`.
