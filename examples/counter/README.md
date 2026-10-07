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