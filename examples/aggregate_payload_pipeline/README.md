# Aggregate payload rotation

The 28-bit packet contains an ordered Pair(first: 3 bits, second: 5 bits), a real four-element
Table of four-bit lanes, and selected: 4 bits. Pair members increment modulo 8/32;
lanes rotate from source indices 1,2,3,0; selected reads source lane 2. Ordered
Pair fields preserve the historical heterogeneous tuple positions. This example
does not establish a general tuple authoring API.

Two depth-one queues with downstream_pop replacement retain input and transformed
tokens. E0 captures, E1 transfers the transformation, and E2 consumes it. Work
reads old committed state. Held clocks cannot transfer; rising reset discards
both occupied slots; a full pipeline can replace both tokens during consumption.

The independent host and RTL tests preserve the original owners at revision
b3e52fb22061350d61b23089c8bed36913650245:

- test_queue_codegen.py native vector: pair(5,17), lanes(1,2,3,4), selected0
  produces pair(6,18), lanes(2,3,4,1), selected3, packed 220345363. Five
  Work/Xfer ticks retire exactly one token.
- test_pyc_backend.py backend vector: pair(5,29), lanes(1,2,3,4), selected15
  produces pair(6,30), lanes(2,3,4,1), selected3, packed 232928275. Ten cycles
  reset at 0, inject at 1 and keep take high; the sole post-edge output is cycle 2.
  The active Work sampler reevaluates at the low clock level after the rising
  commit to expose that post-edge state without an additional transfer.

New independent coverage uses all 256 pair combinations with varied lanes,
walking bits, zero-result tokens, full replacement, changed stalled offers,
held high/low clocks, reset discard and drain. Native workers 1/2 and Verilator
compare 645 known Work rows. Native workers 1/2 and genuine Icarus compare 329
four-state Work rows. The host checks complete value/known/Z planes for rotated
lanes and selected; each arithmetic field becomes all X if that field contains
X/Z. Tests leave only the latent value of computed unknown arithmetic unconstrained.
Copied X/Z bits retain their latent value planes exactly.

The separate bench.py system contains 48 literal stimulus/expected phases derived
from an independent two-slot model. It retains both original payload values and
adds regular-clock wrap/stall/drain checks. Physical clock/reset and four-state
scenarios remain in the independent host and RTL assets.

Build and run through the shared example framework:

```sh
cmake -S examples/aggregate_payload_pipeline -B /absolute/build/aggregate_payload_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/aggregate_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/aggregate_payload_pipeline --output-on-failure --no-tests=error
```

These assets describe required checks; their presence is not execution evidence.
