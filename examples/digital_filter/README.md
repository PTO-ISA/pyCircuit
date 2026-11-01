# Four-tap signed FIR filter

Current migration status and remaining work are tracked in the
[catalog](../catalog.json) and [migration inventory](../README.md#remaining-migration-work).
This design and its independent oracles are retained outside the current execution
set. Historical verification below is not a PASS for the current candidate.

`FilterStorage` owns three 16-bit delay samples, the 34-bit output, and one
registered valid bit, all initialized to zero. Its `filter_sample` Method
computes the fixed signed FIR:

```text
y[n] = x[n] + 2*x[n-1] + 3*x[n-2] + 4*x[n-3]
```

The input is a 16-bit two's-complement bit pattern. Each tap is explicitly
widened to 34 bits and combined with its sign mask before modular arithmetic.
Coefficients, widths, sign handling, arithmetic order, and register boundaries
remain unchanged. The mathematical output range is -327680 through 327670.

The Method returns the committed output and valid bit before proposing its
updates. All delay reads observe old Q. Valid samples advance history and
compute a result; invalid samples hold history/output and clear output validity.
Conditional expressions retain the original value-mux algorithm. There is no
extra result State, Queue, or observation cycle.

The `DigitalFilter` wrapper keeps the original scalar input interface and
explicitly registers its calling Rule. The direct input-bearing root currently
fails linking with `transactional link requires a closed concrete root`.
Supporting that physical boundary remains required work.

## Full System history

The System owns one storage instance and explicitly registers a Rule that calls
its Method, checks the same-Work old output against the independent row, and
advances the phase. All 177 original `Scenario` constructor rows remain
unchanged, including negative samples, invalid gaps, held output, and recovery.
The initialized scenario Table is never written.

The u64 phase wraps normally. Selection clamps to row 176 after the active
history: input `(44191, 0)`, expected output `(72035, 0)`. Both field checks stay
under `phase < 177`, and both logs remain unconditional. The System still runs
177 cycles with low/high sampling and no additional pipeline stage.

The cold full-history run passes source compilation, linking, and C++ emission
but hits the existing 180-second limit while building the native simulation.
After separately completing that unchanged build, the unmodified verifier runs
C++ workers 1/2 and RTL successfully: all 177 scenarios and 708 observations
agree. Overall acceptance still **FAILS** because the final IR hashes differ in
embedded provenance bytecode; decoded provenance and the remaining IR are
identical. The full dataset and existing limits are retained. These successful
executions do not constitute a passing artifact-verification receipt.

Run the generated System with the current installed toolchain:

```sh
pycircuit run examples/digital_filter --target cpp --workers 1 --cycles 177
pycircuit run examples/digital_filter --target cpp --workers 2 --cycles 177
pycircuit run examples/digital_filter --target verilog --cycles 177
```

The existing CMake entry registers separate Module and System tests:

```sh
cmake -S examples/digital_filter -B /absolute/build/digital-filter -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
ctest --test-dir /absolute/build/digital-filter -R digital_filter_system \
  --output-on-failure --no-tests=error
```

## Retained physical and four-state oracles

The unchanged host oracle uses independent signed integer arithmetic and masks
its final result to 34 bits. Its 378 Work samples retain impulse, step, ramp,
alternating and large values, negative impulses, extrema, invalid gaps, clock
hold, populated reset, discard/retry, and recovery checks. Original RTL assets
and all eight Icarus X/Z scenarios remain unchanged.

These physical checks are **NOT RUN** for this migration while the direct root
is unsupported. System adapter results cannot replace them. In particular,
unknown-valid value merging, raw valid X/Z capture, unknown-sample arithmetic,
and four-known-sample recovery remain required four-state behavior without a
new execution claim.

[Generated excerpts and verification inputs](GENERATED.md) describe the
superseded frontend and are retained as historical evidence only.
