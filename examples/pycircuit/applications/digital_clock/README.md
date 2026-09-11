# Digital Clock Example

24-hour clock with BCD outputs and debounced set/plus/minus controls.

## Files

- `digital_clock.py`: top-level design
- `debounce.py`: button debouncer
- `bcd.py`: binary-to-BCD helpers
- `digital_clock_capi.cpp` + `emulate_digital_clock.py`: C++ sim wrapper + terminal emulator

## Build + run

Build generated project (recommended):

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  examples/pycircuit/applications/digital_clock/tb_digital_clock.py \
  --out-dir .pycircuit_out/applications/digital_clock/project \
  --target cpp \
  --jobs 4
```

Build shared lib:

```bash
c++ -std=c++20 -O2 -shared -fPIC \
  -I "$PYC_TOOLCHAIN_ROOT/include" \
  -I .pycircuit_out/applications/digital_clock/project/device/cpp \
  .pycircuit_out/applications/digital_clock/project/device/cpp/digital_clock.cpp \
  examples/pycircuit/applications/digital_clock/digital_clock_capi.cpp \
  "$PYC_TOOLCHAIN_ROOT/lib/libpyc6_runtime.a" \
  -o .pycircuit_out/applications/digital_clock/libdigital_clock_sim.dylib
```

Run emulator:

```bash
# Run from the pyCircuit repository root.
python3 examples/pycircuit/applications/digital_clock/emulate_digital_clock.py
```
