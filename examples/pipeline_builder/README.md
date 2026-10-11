# Two-stage packet pipeline

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This example implements a pipeline with a fixed 32-bit word width. Two `PipelinePacket` registers each store
`word: bits[32]` and `valid: bits[1]`. Stage 0 captures the scalar inputs. At the
same transaction, stage 1 captures old stage 0 with its word incremented modulo
2^32. The typed result reports old stage 1, so there is no combinational bypass
from either the input or stage 0 to the output.

Both stages have recursively zero reset images. Work observes old register
values for the entire transaction, and Xfer commits both proposals together.
Thus the first samples after reset report `word=0, valid=0`; a valid input needs
one capture edge into stage 0 and another edge into stage 1 before a following
Work reports its incremented word. Unknown word bits participate in the existing
fixed-width addition rather than being replaced by a host calculation.

`PipelineBuilder` directly calls the same-source state-owning `PipelineStorage`
module. The stateless root inherits the generated sampling and reset pins. The
Python source contains no clock, reset, DFFE or current/next API. Its public
inputs are `word` and `valid`, and its single physical `result` value is the
ordered nominal `PipelinePacket` struct.

Build this design through the shared example helper and an installed pyCircuit
package:

```sh
cmake -S examples/pipeline_builder -B /absolute/build/pipeline-builder -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/pipeline-builder --parallel 4
ctest --test-dir /absolute/build/pipeline-builder --output-on-failure --no-tests=error
```

Acceptance requires the generated C++ model with one and two workers and the
RTL model to agree with independent latency, hold and wraparound oracles. Source
presence alone is not execution evidence.

## Scenario bench

`bench.py` exports `example_pipeline_builder.bench.ExercisePipelineBuilder`. It stores the original
72 scenarios in an immutable Table with a 4-field, 66-bit `Scenario`.
The Table contains all 72 original rows. Its index uses the complete 64-bit phase
and holds at row 71: `(word=5, valid=1, expected_word=2143795050,
expected_valid=1)`. This is the original expected-data tail, not a prediction of
future DUT steady state. Invalid payloads remain checked: row 1 expects word 1
with valid 0, row 2 first expects valid word 6, and row 3 checks invalid word 0.

The counter remains a wrapping `bits[64]` value initialized to zero; only Table
selection clamps. The source retains check-before-advance registration, every
original assertion, message, log and DUT argument. The generated system still
runs 72 cycles, with the same input in each low/high sampling pair.

Independent source checks preserve all 288 active field values and prove the
complete known-u64 domain through the original comparison partitions and constant
tails. This is a symbolic partition proof, not enumeration of 2^64 values or a
claim of equivalence for arbitrary X/Z phase inputs. The internal phase remains
known by its unchanged initialization and increment.

Baseline and compact versions each pass both existing standalone tests. Twenty
complete native/RTL traces preserve all 288 observations and the terminal result,
including native workers 1/2, after excluding only changed source-position metadata.
The unchanged physical drivers retain 149 known Work samples. Explicit runs
of the existing Icarus macro also pass all 4 four-state cases in both versions;
those macros are separate from the shared CTest verifier. Regular-clock system
coverage does not replace held clocks, physical reset or discard/recovery tests.
See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,008 lines / 37,551 bytes to
126 lines / 6,612 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.171 s | 0.232 s |
| Link system | 1.856 s | 0.386 s |
| Emit C++ | 1.988 s | 0.363 s |
| Emit Verilog | 1.497 s | 0.323 s |
| Build C++ | 1.694 s | 0.675 s |
| Build Verilog | 2.917 s | 1.376 s |
| First measured full C++ run | 0.322 s | 0.290 s |
| First measured full Verilog run | 0.262 s | 0.407 s |
| Warm C++ median, one worker | 0.054179 s | 0.006640 s |
| Warm Verilog median | 0.006114 s | 0.005974 s |
| Warm native peak RSS median | 2.59 MiB | 1.98 MiB |

| Artifact size | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Final IR | 4,848,702 bytes | 1,106,394 bytes |
| Generated C++ total | 992,680 bytes | 357,553 bytes |
| Generated Verilog total | 213,198 bytes | 80,297 bytes |

Recorded first-run regressions: RTL 0.262 → 0.407 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/pipeline_builder --target cpp --cycles 72
pycircuit run examples/pipeline_builder --target verilog --cycles 72
```
