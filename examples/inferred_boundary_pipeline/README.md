# Inferred byte increment pipeline

## Payload behavior

The original typed system accepts one u8 token and returns its increment.
The source computes `value + 1` as an ordinary fixed-bit expression,
including 255 -> 0 wraparound. The one-use pure helper is inlined between its
original input and result queues; this preserves the rule-result storage boundary.

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

The module and four-state CTest gates execute actual generated native workers 1/2, Verilator and
genuine Icarus. They check 573 known Work rows and 137 native/Icarus four-state
Work rows, including wraparound, zero/all-X tokens, stalls/full replacement,
changed offers, active held high/low clocks, reset and drain. Scalar cases cover
all256 byte values; nested cases cover all64 opcode values and all8 modes plus
walking input bits. Per-bit and dense X/Z patterns distinguish computed unknown
arithmetic from exact copied value/known/Z planes. All runners are finite.

## Scenario bench

`bench.py` stores all 344 original scenarios in an immutable typed Table. Its
6-field, 20-bit `Stimulus` retains the original field order, including payloads
supplied or observed while valid is zero. Row 344 is all zero, preserving the
helper fallback for every known u16 phase from 344 through 65,535. This fallback
is distinct from the system counter, which holds at phase 343.

The complete system retains its ordered DUT inputs, 3 unconditional assertions,
4 unconditional observations and `advance(phase)` before `exercise()`.
The regular-clock run still lasts 344 cycles.

Independent source checks compare all 393,216 field values across the complete
known-u16 helper domain. Twenty complete native and RTL traces agree on all
2,752 observations and the terminal result after excluding only changed
source-position metadata; native workers 1 and 2 are included. Baseline and
compact versions each pass all 3 standalone gates. See the module
[excerpts and receipt](GENERATED.md).

The unchanged physical drivers retain 573 known and 137 four-state Work rows,
including the scalar and nested payload histories described above.

The closed system covers a regular-clock known-state trajectory. Full physical
reset/held-clock scenarios remain with their independent module drivers;
known-u16 source equivalence does not prove arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 2,316 lines / 51,539 bytes to
410 lines / 34,726 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.626 s | 1.823 s |
| Link system | 2.442 s | 2.968 s |
| Emit C++ | 2.117 s | 2.648 s |
| Emit Verilog | 1.820 s | 1.390 s |
| Build C++ | 1.324 s | 1.325 s |
| Build Verilog | 1.547 s | 2.654 s |
| First measured full C++ run | 0.884 s | 0.273 s |
| First measured full Verilog run | 0.305 s | 0.269 s |
| Warm C++ median, one worker | 0.964186 s | 0.052602 s |
| Warm Verilog median | 0.046643 s | 0.042397 s |
| Warm native peak RSS median | 4.45 MiB | 3.11 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 11,023,387 bytes | 7,840,665 bytes |
| Generated C++ total | 2,700,445 bytes | 2,077,813 bytes |
| Generated Verilog total | 590,243 bytes | 437,117 bytes |

Native execution, source size, final IR and generated-code size improve in this
comparison. Several single-run compile, link, emission and build timings
increase; the table reports those costs without attributing their cause from
one observation. This is not an across-the-board performance improvement, and
three warm pairs do not establish statistical significance for small RTL changes.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/inferred_boundary_pipeline --target cpp --cycles 344
pycircuit run examples/inferred_boundary_pipeline --target verilog --cycles 344
```
