# Scalar bitfield permutation

## Payload behavior

The input and output are 32-bit words. The original operation swapped low17 and
high15, then replaced the result's low3 with the original word's mode bits.
The equivalent expression is `ac.concat(word[:17], word[20:32], word[:3])`.
Output bits31:15 copy input16:0; output14:3 copy input31:20; output2:0 copy
input2:0 again. Input17:19 are discarded and input0:2 are duplicated. This is
not a rotate. All selected value/known/Z bits are transported exactly.

The pure, single-use permutation is an ordinary module-local expression between
the two queues. The old rule-result queue is retained explicitly. An initial
annotated scalar-rule probe was rejected by the existing return-annotation
contract; this rewrite does not extend that contract or wrap scalar values in
an artificial state owner.

## Original timing

Exactly two depth1/latency1 queues preserve the inferred input queue and
rule-result queue. Both explicitly select `downstream_pop` for original full
replacement. Returning the result adds no third queue; no empty bypass exists.
E0 captures input, E1 commits the result stage, and E2 is earliest consumption.
All Work samples use old state. Held clocks do not transfer; stalls fill at most
two slots; rising reset clears both stages. Python leaves clock/reset/proposal
wires hidden. The host drives the physical pins through the existing typed DUT.

## Build and verification

```sh
cmake -S examples/bitfield_scalar_pipeline -B /absolute/build/bitfield_scalar_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bitfield_scalar_pipeline --parallel 4
ctest --test-dir /absolute/build/bitfield_scalar_pipeline --output-on-failure --no-tests=error
```

The two CTest gates run generated native workers 1/2, Verilator and genuine
Icarus. They compare 107 known Work rows and 179 native/Icarus four-state
Work rows. Known one-hot vectors and isolated X/Z at every input position,
plus dense patterns with nonzero latent values, check all three native planes.
Icarus checks visible X/Z placement. The two-slot oracle includes startup,
full replacement, stalls, changing rejected offers, active held high/low clocks,
bubbles, drain and reset. All runners have explicit finite limits.

Known accepted/retired/reset-dropped counts are 42/38/4; four-state counts are
78/74/4. Both histories finish empty, with peak occupancy two. Final IR checks
confirm two D1 queues, availability latency1, combinational head-read latency0,
explicit downstream replacement, no empty flow and no additional state owners.
The separate `bench.py` system checks a finite regular-clock known-state scenario, including queue saturation, stalls, replacement, boundary values and final drain. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/bitfield_scalar_pipeline --target cpp --cycles 158 --build-dir .pycircuit_out/bitfield_scalar_pipeline/system-cpp
pycircuit run examples/bitfield_scalar_pipeline --target verilog --cycles 158 --build-dir .pycircuit_out/bitfield_scalar_pipeline/system-verilog
```
