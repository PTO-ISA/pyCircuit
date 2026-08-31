# Calculator Example

16-digit calculator implemented with the pyc6 module flow and a C++ simulation wrapper.

## Files

- `calculator.py`: pyCircuit design
- `calculator_capi.cpp`: C API wrapper around generated C++ model
- `emulate_calculator.py`: terminal emulator using `ctypes`

## Build + run

Build generated project (recommended):

```bash
PYTHONPATH=compiler/frontend \
python3 -m pycircuit.cli build \
  designs/examples/calculator/tb_calculator.py \
  --out-dir .pycircuit_out/examples/calculator_build \
  --target cpp \
  --jobs 4
```

Build shared lib:

```bash
cd designs/examples/calculator
c++ -std=c++17 -O2 -shared -fPIC \
  -I../../runtime -I../../../.pycircuit_out/examples/calculator_build/device/cpp \
  -o libcalculator_sim.dylib calculator_capi.cpp
```

Run emulator:

```bash
# Run from the pyCircuit repository root.
python3 designs/examples/calculator/emulate_calculator.py
```
