# Three-stage tagged pipeline

[Generated MLIR, C++ and RTL](GENERATED.md)

`JitPipelineVec(a: bits[16], b: bits[16], sel: bits[1])` computes a comparison tag
and either a 16-bit modular sum (`sel=1`) or a bitwise XOR (`sel=0`). Three
zero-initialized `Packet` variables carry the tag and data. The typed result
contains `tag: bits[1]`, `data: bits[16]` and its low byte, `lo8: bits[8]`.

The rule reads the third packet for its output before shifting the three packet
values. All state updates commit together. A packet appears in a Work sample
after its third successful capture edge; the low-byte slice adds no register.
Reset clears all three packets. Repeated clock levels hold state. The Python
source uses ordinary state, a rule and a direct module call; the runner owns
the generated physical sampling/reset inputs.

```sh
cmake -S examples/jit_pipeline_vec -B /absolute/build/jit-pipeline-vec -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/jit-pipeline-vec --parallel 4
ctest --test-dir /absolute/build/jit-pipeline-vec --output-on-failure --no-tests=error
```

The gate runs generated native models with one and two workers and compares
Work samples against RTL. Native discard/retry checks cover the transactional boundary. The existing
Icarus macro checks four-state behavior separately; the shared CTest verifier
does not invoke that macro automatically.

## Scenario bench

`bench.py` exports `example_jit_pipeline_vec.bench.ExerciseJitPipelineVec`. It stores the original
75 scenarios in an immutable Table with a 6-field, 58-bit `Scenario`.
The Table contains all 75 original rows. Its index uses the complete 64-bit phase
and holds at row 74: `(a=1, b=1, sel=1, expected_tag=1, expected_data=2,
expected_lo8=2)`. Initial empty stages, the first result at phase 3, modular sums,
XOR selection and the full data/tag/low-byte alignment remain unchanged.

The counter remains a wrapping `bits[64]` value initialized to zero; only Table
selection clamps. The source retains check-before-advance registration, every
original assertion, message, log and DUT argument. The generated system still
runs 75 cycles, with the same input in each low/high sampling pair.

Independent source checks preserve all 450 active field values and prove the
complete known-u64 domain through the original comparison partitions and constant
tails. This is a symbolic partition proof, not enumeration of 2^64 values or a
claim of equivalence for arbitrary X/Z phase inputs. The internal phase remains
known by its unchanged initialization and increment.

Baseline and compact versions each pass both existing standalone tests. Twenty
complete native/RTL traces preserve all 450 observations and the terminal result,
including native workers 1/2, after excluding only changed source-position metadata.
The unchanged physical drivers retain 153 known Work samples. Explicit runs
of the existing Icarus macro also pass all 12 four-state cases in both versions;
those macros are separate from the shared CTest verifier. Regular-clock system
coverage does not replace held clocks, physical reset or discard/recovery tests.
See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,324 lines / 47,560 bytes to
135 lines / 7,232 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.445 s | 0.304 s |
| Link system | 2.108 s | 0.516 s |
| Emit C++ | 1.505 s | 0.495 s |
| Emit Verilog | 1.679 s | 0.417 s |
| Build C++ | 1.578 s | 0.725 s |
| Build Verilog | 2.288 s | 1.369 s |
| First measured full C++ run | 0.366 s | 0.399 s |
| First measured full Verilog run | 0.358 s | 0.318 s |
| Warm C++ median, one worker | 0.070676 s | 0.008609 s |
| Warm Verilog median | 0.007536 s | 0.007590 s |
| Warm native peak RSS median | 2.98 MiB | 2.09 MiB |

| Artifact size | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Final IR | 6,045,832 bytes | 1,692,868 bytes |
| Generated C++ total | 1,248,579 bytes | 536,017 bytes |
| Generated Verilog total | 265,678 bytes | 109,365 bytes |

Recorded first-run regressions: native 0.366 → 0.399 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/jit_pipeline_vec --target cpp --cycles 75
pycircuit run examples/jit_pipeline_vec --target verilog --cycles 75
```
