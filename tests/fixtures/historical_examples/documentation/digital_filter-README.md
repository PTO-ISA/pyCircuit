# Four-tap signed FIR filter

The filter Rule is nested in its Module and captures the delay state directly.
The arithmetic order, saved old-tap values and original register boundaries
remain unchanged.

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This example implements a fixed four-tap filter:

```text
y[n] = x[n] + 2*x[n-1] + 3*x[n-2] + 4*x[n-3]
```

Inputs are `x_in: bits[16]` (a signed two's-complement sample) and
`x_valid: bits[1]`. The typed `FilterResult` contains the registered 34-bit
two's-complement `y_out` and registered `y_valid`. Coefficients and widths are
the original fixed source constants; the historical public parameter set is empty.

Three 16-bit history variables, the 34-bit output and one valid bit all initialize
and reset to zero. Valid samples advance the history and compute one output;
invalid cycles hold history/output and clear output validity. No extra pipeline
stage is inserted. The old C wrapper's invalid cycle after a pushed sample is
testbench stimulus, rather than part of the filter's latency.

The rule snapshots output/valid before updating its ordinary variables. It
computes from old taps, then assigns the delays in descending order. A runner
Work sample observes the old registered output; a following nonrising Work sees
the preceding edge's committed value. Python does not expose clock/reset or
current/next state. The host drives generated physical pins through the shared
typed DUT and SystemRunner.

Each sample is explicitly widened to 34 bits and its sign bit selects the high
two's-complement mask before modular multiplication/addition. This preserves
signed FIR results with the existing bits operations; no signed frontend type or
new primitive is introduced. The true result range is -327680 through 327670.
The source mask expression is not a general four-state sign-extension API.

Conditional expressions preserve the original data-mux hold semantics. With
unknown valid, equal proposed/held data bits remain known and differing bits
become X; raw valid X/Z is captured in `y_valid`. An active unknown sample makes
the arithmetic output unknown. Four subsequent known valid samples flush its
effect from the three history registers; invalid cycles do not flush history.

Build through the same public compile/link/emit and installed example helper:

```sh
cmake -S examples/digital_filter -B /absolute/build/digital-filter -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/digital-filter --parallel 4
ctest --test-dir /absolute/build/digital-filter --output-on-failure --no-tests=error
```

The independent host oracle uses signed integer arithmetic, then masks the final
result to 34 bits. Tests preserve the original impulse/step/ramp/alternating/large
value scenarios and add negative impulses, extrema, invalid gaps, clock hold,
populated reset, discard/retry and genuine four-state recovery. Native workers
1/2 and RTL agree on 378 Work samples. Eight genuine Icarus X/Z scenarios and
native discard/failure/recovery assertions pass. The generated examples above
come from this verified build; source presence alone is not acceptance.

## Scenario bench

`bench.py` exports `example_digital_filter.bench.ExerciseDigitalFilter`. Its immutable Table contains
177 original rows of a 4-field, 52-bit `Scenario`.
All 177 rows remain, with the complete 64-bit index clamped to row 176:
`(x_in=44191, x_valid=0, expected_y_out=72035, expected_y_valid=0)`.
Inputs remain raw u16 bit patterns and expected outputs raw u34 two's-complement
patterns. Invalid samples, held output data, validity changes and recovery remain
explicit. The signed independent oracle stays in the unchanged physical driver.

The phase remains a wrapping `bits[64]` counter initialized to zero. Only Table
selection clamps. The original DUT call, input/expected bindings, unmasked
assertions, messages, logs and check-before-advance registration remain intact.
The generated system still runs 177 cycles with identical low/high sampling inputs.

Independent source proof preserves all 708 active field values and the complete
known-u64 domain by splitting original constant comparison cells across candidate
row selections and checking the tail. Original partitions need not have one cell
per Table row; repeated rows remain distinct. High-phase probes supplement this
proof rather than replacing it. No arbitrary X/Z internal-phase equivalence is
claimed; initialization and the unchanged increment keep this closed phase known.

Baseline and compact versions each pass both existing standalone tests. Twenty
complete native/RTL system traces preserve all 708 observations and the terminal
result, including native workers 1/2, after excluding only changed source-position
metadata. Regular-clock systems do not replace physical clock/reset histories.

The unchanged physical driver retains 378 known Work samples and its native
lifecycle, failure and recovery checks. Explicit baseline/candidate Icarus runs
also preserve all eight existing X/Z cases and the complete output. The shared
CTest verifier does not invoke that macro automatically.

See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,210 lines / 47,063 bytes to
231 lines / 14,760 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.896 s | 0.419 s |
| Link system | 1.333 s | 0.730 s |
| Emit C++ | 1.235 s | 0.665 s |
| Emit Verilog | 1.029 s | 0.566 s |
| Build C++ | 0.891 s | 0.837 s |
| Build Verilog | 1.440 s | 1.411 s |
| First measured full C++ run | 0.690 s | 0.299 s |
| First measured full Verilog run | 0.314 s | 0.282 s |
| Warm C++ median, one worker | 0.142768 s | 0.014435 s |
| Warm Verilog median | 0.010174 s | 0.009870 s |
| Warm native peak RSS median | 2.80 MiB | 2.27 MiB |

| Artifact size | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Final IR | 5,486,130 bytes | 2,684,951 bytes |
| Generated C++ total | 1,119,433 bytes | 790,157 bytes |
| Generated Verilog total | 237,336 bytes | 171,413 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/digital_filter --target cpp --cycles 177
pycircuit run examples/digital_filter --target verilog --cycles 177
```
