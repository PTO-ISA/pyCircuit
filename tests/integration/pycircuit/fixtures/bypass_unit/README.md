# Bypass-unit fixture

This directory contains a generic bypass integration fixture plus a pyc-native
`@testbench`.

Files:

- design: `tests/integration/pycircuit/fixtures/bypass_unit/bypass_unit.py`
- testbench: `tests/integration/pycircuit/fixtures/bypass_unit/tb_bypass_unit.py`

## Run (Verilator)

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  tests/integration/pycircuit/fixtures/bypass_unit/tb_bypass_unit.py \
  --out-dir /tmp/bypass_unit_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
