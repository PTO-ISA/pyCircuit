# Enabled eight-bit counter

[Generated MLIR, C++ and RTL](GENERATED.md)

```sh
cmake -S examples/counter -B /absolute/build/counter -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/counter --parallel 4
ctest --test-dir /absolute/build/counter --output-on-failure --no-tests=error
```

The source declares an ordinary `count: ac.u8 = 0` and returns a typed
`CounterResult`. The rule saves the result before assigning its modular increment
or unchanged value. This unconditional data selection preserves the original
unknown-enable merge; a guarded write would instead make enable a storage
control. Clock/reset are absent from Python and bound by the host runner.
One Work samples old Q; Xfer commits the
active edge. The next Work observes the new value, with no added Eval stage.

The per-design driver and RTL testbench validate original values 1..5, a real
256-edge wrap, enable hold, repeated clock levels and reset priority. The native
DUT also checks X/Z enable and partially unknown state with workers 1/2. Known
whole-counter RTL runs with Verilator. The `PYC_COUNTER_FOUR_STATE` testbench
mode also checks the complete generated DUT and its standard storage with
Icarus: Q0/Q3 with enable X/Z, partial-X hold and reset recovery.

## Generated system usage

`bench.py` exports `example_counter.bench.ExerciseCounter`. The explicit source
closure is `counter.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/counter --target cpp --cycles 263` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

The complete 256-enabled-edge wrap sequence and every later original rising-edge
enable value run for 263 cycles, including a final committed-state check.
Independent fixed values check modulo-256 counting and disabled holds; physical
reset-priority and repeated-level checks remain with the original oracle.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
