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
PYTHONPATH=compiler/frontend \
python3 -m pycircuit.cli build \
  designs/examples/digital_clock/tb_digital_clock.py \
  --out-dir .pycircuit_out/examples/digital_clock_build \
  --target cpp \
  --jobs 4
```

Build shared lib:

```bash
cd designs/examples/digital_clock
c++ -std=c++17 -O2 -shared -fPIC \
  -I../../runtime -I../../../.pycircuit_out/examples/digital_clock_build/device/cpp \
  -o libdigital_clock_sim.dylib digital_clock_capi.cpp
```

Run emulator:

```bash
# Run from the pyCircuit repository root.
python3 designs/examples/digital_clock/emulate_digital_clock.py
```
