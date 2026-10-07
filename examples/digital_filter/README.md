# Four-tap signed FIR filter

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This example implements the `applications/digital_filter` circuit:

```text
y[n] = x[n] + 2*x[n-1] + 3*x[n-2] + 4*x[n-3]
```

Inputs are `x_in: ac.u16` (a signed two's-complement sample) and
`x_valid: ac.u1`. The typed `FilterResult` contains the registered 34-bit
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
