# Array reductions

`ArrayReductions` preserves the `array_reductions` root from historical commit
`8887e6de` (`examples/agentic-circuit/blocks/array_combinators.py`). The design
reduces three unsigned eight-bit request fields and returns the original fourteen
results in their original order: all/any, a shared count in two fields, wrapping
add/multiply, min/max, flag parity, first-match index/valid, valid-only argmin
index/valid, and the argmin index after each value is reduced modulo five.

The result is exactly 47 packed bits. Flags are physical one-bit values; counts
and all three indices are two bits. This source uses the supported closed
`table[3, u8]` transformations and reductions. It does not claim general
historical `ac.index`, `ac.range`, named Table callbacks, or ordered scans.

The module has two depth-one queues with
`ready_policy="downstream_pop"`. Both queues have the default latency of one,
there is no empty flow-through, and all Work reads observe old committed queue
state. A request accepted at edge E0 can enter the result queue at E1 and first
be retired at E2. The output is available after the E1 transfer. Stalls preserve the complete 47-bit
token; simultaneous downstream pop and replacement preserve occupancy.

`ArrayReductionsSystem` supplies the five retained requests `(0,0,0)`,
`(0,0,5)`, `(7,7,9)`, `(3,1,2)`, and `(255,2,3)` from a compact Table. Its
independent literal oracle checks every result field, counts accepted requests
and observed outputs, logs the output stream, and asserts that the pipeline is
drained at phase 7. Reset and queue initialization are supplied by the standard
closed-system runner; after reset, both queues are empty and invalid data is
packed zero.

The standalone flow compiles `array_reductions.py` and `bench.py` as
separate source units, links their explicit closure, and emits C++ and Verilog
from the same verified final artifact. The independent native and RTL drivers retain all five original packed
goldens, add directed boundaries and 64 seeded vectors, and check 187 Work
samples under stalls, held clocks, full replacement and reset. Native workers
one/two and Verilator passed the same history. The closed system also passes
both generated backends. Four-state and full nightly acceptance are not claimed.

```sh
cmake -S examples/array_reductions -B .pycircuit_out/array-reductions -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build .pycircuit_out/array-reductions --parallel 4
ctest --test-dir .pycircuit_out/array-reductions --output-on-failure --no-tests=error
pycircuit run examples/array_reductions --target cpp --cycles 16
pycircuit run examples/array_reductions --target verilog --cycles 16
```

[Generated excerpts](GENERATED.md) and [artifact receipt](GENERATED.json)
bind the retained module proof. The source is 104 lines; the bench uses one
small fixture value and handshakes instead of an expanded literal history.
This is source-size evidence, not a compiler or runtime speedup claim.
