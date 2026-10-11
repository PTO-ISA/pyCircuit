# Selected byte arithmetic and four increments

The design selects byte addition, subtraction, XOR or AND for op0,
op1, op2 or op3, then performs four increments. Every arithmetic boundary wraps
to eight bits. The original a1/b2/op0 case produces result7 in the same epoch.
The four rounds are fixed internal configuration; the root has no public
parameters, state or clock.

The source uses `bits[8]` operands and `bits[2]` selection, and returns the nominal
`JitResult(result: bits[8])` in one physical `result` packet. Bits addition and
subtraction wrap at eight bits before selection and after each increment.
The pure conditional-expression chain selects addition, then subtraction,
then XOR, with AND as its final fallback; unknown selectors use four-state
mux merging. Ordinary local names share these combinational values.

```sh
cmake -S examples/jit_control_flow -B /absolute/build/jit-control-flow -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/jit-control-flow --parallel 4
ctest --test-dir /absolute/build/jit-control-flow --output-on-failure --no-tests=error
```

The historical name is retained, but this source follows normal compile/link/
emit through common IR. Immutable local SSA stages reuse the existing bits
arithmetic, equality and select lowering. Every arithmetic value has the same
eight-bit representation at the selection boundary.
Independent native/RTL oracles check all operation choices, byte boundaries,
same-epoch input changes, unknown data/selectors and recovery. The
`PYC_JIT_FOUR_STATE` mode runs the complete generated DUT in Icarus.
See [the generated-output guide](GENERATED.md) for actual source-owned IR,
C++ and RTL from the public flow.

## Scenario bench

`bench.py` exports `example_jit_control_flow.bench.ExerciseJitControlFlow`. It stores the original
65 scenarios in an immutable Table with a 4-field, 26-bit `Scenario`.
The Table contains the original 65 rows plus an all-zero sentinel at row 65.
Row 64 remains `(a=254, b=0, op=3, expected_result=4)`. The index uses the complete
64-bit phase, selecting row 65 for all phases 65 and above. All four operations,
byte underflow and modular wraparound retain their literal expected values.

The counter remains a wrapping `bits[64]` value initialized to zero; only Table
selection clamps. The source retains check-before-advance registration, every
original assertion, message, log and DUT argument. The generated system still
runs 65 cycles, with the same input in each low/high sampling pair.

Independent source checks preserve all 260 active field values and prove the
complete known-u64 domain through the original comparison partitions and constant
tails. This is a symbolic partition proof, not enumeration of 2^64 values or a
claim of equivalence for arbitrary X/Z phase inputs. The internal phase remains
known by its unchanged initialization and increment.

Baseline and compact versions each pass both existing standalone tests. Twenty
complete native/RTL traces preserve all 130 observations and the terminal result,
including native workers 1/2, after excluding only changed source-position metadata.
The unchanged physical drivers retain 65 known Work samples. Explicit runs
of the existing Icarus macro also pass all 18 four-state cases in both versions;
those macros are separate from the shared CTest verifier. Regular-clock system
coverage does not replace held clocks, physical reset or discard/recovery tests.
See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,282 lines / 186,037 bytes to
118 lines / 4,788 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.449 s | 0.221 s |
| Link system | 2.394 s | 0.363 s |
| Emit C++ | 2.210 s | 0.339 s |
| Emit Verilog | 2.094 s | 0.307 s |
| Build C++ | 1.269 s | 0.663 s |
| Build Verilog | 2.177 s | 1.295 s |
| First measured full C++ run | 0.517 s | 1.112 s |
| First measured full Verilog run | 0.458 s | 0.358 s |
| Warm C++ median, one worker | 0.032149 s | 0.005309 s |
| Warm Verilog median | 0.004739 s | 0.004597 s |
| Warm native peak RSS median | 2.28 MiB | 1.92 MiB |

| Artifact size | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Final IR | 7,596,284 bytes | 1,062,961 bytes |
| Generated C++ total | 642,278 bytes | 305,787 bytes |
| Generated Verilog total | 140,240 bytes | 70,133 bytes |

Recorded first-run regressions: native 0.517 → 1.112 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/jit_control_flow --target cpp --cycles 65
pycircuit run examples/jit_control_flow --target verilog --cycles 65
```
