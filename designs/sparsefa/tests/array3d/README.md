# Independent 16×16×16 array verification

This directory tests the standalone full 4096-product-lane array. It does not
supply a scalar replacement, DUT arithmetic results, attention/DMA/softmax logic,
or a higher compiler budget. The installed toolchain is used without rebuilding
or changing the compiler. Verilog simulation is excluded by the user.

`generate.py` computes expected values with exact rational arithmetic and explicit
finite FP32 encoding: RTZ, input/output FTZ, signed zero, and finite saturation.
It never imports or executes DUT source. Every active cell has 16 separately
rounded products, then adjacent pairs in a fixed 8→4→2→1 rounded tree, then a
rounded addition to the committed inter-group accumulator. This is neither a
linear reduction nor FMA. Masked/tail product leaves are exact +0; a cell with no
enabled K lane holds its accumulator bit-for-bit. Raw magnitude ordering, left
operand on ties, selects the zero sign for addition, including FTZ inputs.

`cases.json` separates stimulus from expected actions/results. The author-owned
`emit_stimulus.py` reads only the operand matrices, instruction schedule and cycle
horizon to generate `bench.py`. A holds A[m,k]; the right matrix stores columns,
so `right[n].cK` is B[k,n]. No expected values or acceptance flags enter the DUT.
The emitted operand ROM contains 11 matrices (2,816 words), followed by 80
explicit zero rows. All 459 real instructions and 53 idle padding entries fit a
512-entry schedule. The independent AST audit reconstructs every input word and
instruction and checks generator idempotence.

The frozen final fixture contains **459 instructions, 43 successful expected
results and a 461-cycle horizon**. A completed hardware run would check all 256
output cells for every result: **11,008 lane values per worker**, with full trace
equality between workers 1 and 2. These are fixture/check counts, not a claim
that the generated hardware has executed.

Coverage includes:

- Dense K=1,15,16,17,31,32,33,63,64 and an inter-group K=48 case. K=33 exercises
  the third group's one-lane tail; an independent integer anchor checks all 256
  outputs and its 8,448 useful products.
- All-cell asymmetric row/column/K data, each of 16 sparse row/column mappings,
  nonrectangular cell masks, row/column masks, noncontiguous K masks, all-masked
  groups/tiles and masked tail lanes.
- A rounded-tree witness producing +0 where linear accumulation gives −1;
  nonfused rounding producing `0xb3800000` versus fused `0xa8800000`; finite
  saturation, positive/negative zero, input/output FTZ, and inter-group signed
  cancellation followed by an inactive group.
- Invalid length/tag/index, duplicate/extra groups, early END and TAKE, busy
  BEGIN, RESULT hold, wrong-tag actions and abort, omitted-call holds, explicit
  bubbles, and invalid advances that still move already accepted tokens.
- Abort at each occupied pipeline depth, at the would-be retirement edge, in
  DRAIN, and in RESULT, followed by restart. Matching abort immediately flushes
  logical occupancy and performs no arithmetic that edge. This block has no
  external requests to drain.
- END after all groups have retired and TAKE just before the final accumulation
  edge, distinguishing tree completion from committed accumulation.

The pipeline model stores five in-flight stages. A GROUP accepted at enabled
edge E captures products; four more advances commit the four tree levels, and
the fifth following advance commits accumulation at E+5. No public call holds
all stages. Invalid GROUP/tag/early-END returns zero admission but advances old
tokens in ACTIVE/DRAIN. Matching abort takes priority and does not advance them.
For an unstalled G-group tile: begin at 0, groups at 1..G, END at G+1, last
accumulation at G+5, earliest TAKE at G+6, next BEGIN at G+7.

`run.py` compares actual rising-edge action logs (including rejected advances),
all output bits, metadata, take epochs and terminal status against the independent
oracle. `begin_to_take_cycles`, `occupied_edges`, and `group_intervals` derive
from observed action logs. Fields prefixed `model_` use the oracle's predicted
RESULT boundary to delimit the observed events or waits. They are **not an
independent measurement of an internal hardware readiness signal**. No physical
frequency, area or full-attention throughput is claimed.

## Reproduce

From the repository root:

```sh
.pycircuit_out/venv312/bin/python -m unittest \
  designs.sparsefa.tests.array3d.test_oracle
.pycircuit_out/venv312/bin/ruff check \
  designs/sparsefa/tests/array3d \
  designs/sparsefa/src/fa2_array3d.py \
  designs/sparsefa/src/fa2_array3d_interfaces.py

LD_LIBRARY_PATH=/home/xiekunpeng/micromamba/envs/acir/lib \
CXX=/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++ \
  .pycircuit_out/venv312/bin/python \
  designs/sparsefa/tests/array3d/run.py \
  --toolchain /home/xiekunpeng/linxISA/pyCircuit/.pycircuit_out/fa2-frontier-install
```

Regeneration is separate from verification: run `generate.py`, then the
source-author-owned `emit_stimulus.py`, and freeze/audit the resulting inputs
before a new hardware candidate. The runtime tests do not read management logs.
The runner records exact commands, exit codes, source/input/full installed-tree
hashes before and after, stage artifacts and failures under ignored
`docs/gates/logs/fa2-array3d/`. It stops at the failing stage. If link succeeds,
both emitters consume the same unchanged verified final artifact, followed by
C++ execution with workers 1 and 2. It never invokes RTL simulation.

## Actual evidence and limits

The final K=33 host candidate passes **7/7 independent host tests** and Ruff over
all new source/test files. These tests validate the oracle, its hand-derived
anchors and trace rejection on mutated acceptance, missing rows, wrong values,
wrong epochs and failed terminal status. The input-only bench audit also passes.

The earlier 449-instruction/42-result candidate compiled all five source units,
then failed default-budget link at transaction-state analysis:
`requested 768, used 1048555, limit 1048576, phase transaction plans`.
Its unchanged receipt and original input snapshots remain under
`docs/gates/logs/fa2-array3d/20261104T073610.941015Z/`.
The final 459-instruction/43-result K=33 candidate also passes all five source-unit
compiles and fails default-budget link: `requested 512, used 1048269,
limit 1048576, phase transaction plans`. All 181 recorded source/input/tool hashes
remain unchanged. Its exact receipt is
`docs/gates/logs/fa2-array3d/20261104T074254.674027Z/manifest.json`; the final host
receipt is `docs/gates/logs/fa2-array3d/host-final-k33/manifest.json`.
**No verified final artifact, dual emission, generated C++ execution or measured
hardware performance exists for either candidate.**

The subsequent C++ simulation attempt uses the unchanged final fixture and a
checkout-built `.pycircuit_out/fa2-frontier-install`. Its generic compiler repair
replaces all-owner invocation snapshot copies with a two-word version frontier;
it preserves all plan contents and the default budget. Baseline and candidate
each pass 309/309 native tests, with all 22 full plan/artifact properties identical
and independent implementation review clear. Existing P0S product/row artifacts
and the 46-case D1 index C++ regression also remain unchanged; these are separate
component checks, not full-array execution.

The full array still passes all five compiles and fails link with
`requested 4, used 1048575, limit 1048576, phase transaction plans`.
All 154 recorded source/input/tool files match before and after; the receipt is
`docs/gates/logs/fa2-array3d/20261104T082711.511978Z/manifest.json`.
The current result remains **no verified final, dual emission, or C++ hardware
execution**. The failure occurs while analyzing cell initialization. Independently
counting the retained normalized source establishes a further limit: the 4096
Product.load calls alone require 360,448 pure-operation replicas; the current
lowering preparation and source proof each exceed their default budgets on that
subset. Full execution needs a generic reduction of actual repeated lowering
work/structure, beyond compact analysis records. See
`docs/gates/logs/fa2-array3d-simulation/frontier-diagnostic/independent-flat-clone-floor.json`.
Diagnostic instrumentation has been removed. DUT, oracle, stimulus, array size
and limits were not changed.

Not executed: generated C++ hardware checks until link succeeds, verified-final
4096-lane structural inspection, Queue take+push resource backpressure,
mid-operation reset, X/Z fault injection, and Verilog simulation. NaN/Inf is
outside the finite numeric input domain. A host oracle pass, source-unit compile,
or source-level 4096-lane count is not final-artifact or hardware-cycle evidence.
