# Inferred byte increment pipeline

## Payload behavior

The original typed system accepts one u8 token and returns its increment.
The new source computes `value + 1` as an ordinary fixed-bit expression,
including 255 -> 0 wraparound. The one-use pure helper is inlined between its
original input and result queues; this does not add scalar rule-return annotation
support or remove the rule-result storage boundary.

`InferredBoundaryPipeline(valid, data, take)` returns typed ready/valid/data:
8 payload bits and 10 total result bits. Original native UInt arithmetic was
two-state. Historical RTL and current fixed-bit arithmetic produce X throughout
the arithmetic result if an operand contains X/Z; Z does not survive addition.
For the nested root, untouched opcode/payload preserve their complete raw planes
and unknown copied fields do not contaminate a known mode increment. Tests must
not constrain the value plane of an unknown computed result.

## Timing and ownership

Exactly two depth1/latency1 queues retain the original typed input and rule
result. Explicit `downstream_pop` preserves full replacement; head-read latency
is zero and empty flow is disabled. The returned sink adds no third queue.
E0 captures input, E1 commits the transformed token, and E2 is its earliest
consumption. All Work outputs use old committed state. Held clocks do not
transfer; stalls fill at most two slots; rising reset clears both queues.
Clock/reset and proposals remain absent from Python authoring.

## Build and verification

```sh
cmake -S examples/inferred_boundary_pipeline -B /absolute/build/inferred_boundary_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/inferred_boundary_pipeline --parallel 4
ctest --test-dir /absolute/build/inferred_boundary_pipeline --output-on-failure --no-tests=error
```

The two CTest gates execute actual generated native workers 1/2, Verilator and
genuine Icarus. They check 573 known Work rows and 137 native/Icarus four-state
Work rows, including wraparound, zero/all-X tokens, stalls/full replacement,
changed offers, active held high/low clocks, reset and drain. Scalar cases cover
all256 byte values; nested cases cover all64 opcode values and all8 modes plus
walking input bits. Per-bit and dense X/Z patterns distinguish computed unknown
arithmetic from exact copied value/known/Z planes. All runners are finite.
The separate `bench.py` system checks a finite regular-clock known-state scenario, including queue saturation, stalls, replacement, boundary values and final drain. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/inferred_boundary_pipeline --target cpp --cycles 344 --build-dir .pycircuit_out/inferred_boundary_pipeline/system-cpp
pycircuit run examples/inferred_boundary_pipeline --target verilog --cycles 344 --build-dir .pycircuit_out/inferred_boundary_pipeline/system-verilog
```
