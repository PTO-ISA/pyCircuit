# Calculator Example

Integer calculator implemented with the pyc6 module flow and a C++ simulation
wrapper.

## Files

- `calculator.py`: pyCircuit design
- `calculator_capi.cpp`: C API wrapper around generated C++ model
- `emulate_calculator.py`: terminal emulator using `ctypes`

## Build + run

Build generated project (recommended):

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  examples/pycircuit/applications/calculator/tb_calculator.py \
  --out-dir .pycircuit_out/applications/calculator/project \
  --target cpp \
  --jobs 4
```

Build shared lib:

```bash
c++ -std=c++20 -O2 -shared -fPIC \
  -I "$PYC_TOOLCHAIN_ROOT/include" \
  -I .pycircuit_out/applications/calculator/project/device/cpp \
  .pycircuit_out/applications/calculator/project/device/cpp/calculator.cpp \
  examples/pycircuit/applications/calculator/calculator_capi.cpp \
  "$PYC_TOOLCHAIN_ROOT/lib/libpyc6_runtime.a" \
  -o .pycircuit_out/applications/calculator/libcalculator_sim.dylib
```

Run emulator:

```bash
# Run from the pyCircuit repository root.
python3 examples/pycircuit/applications/calculator/emulate_calculator.py
```
