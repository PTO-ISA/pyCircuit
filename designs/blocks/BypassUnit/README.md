# BypassUnit

This directory contains a small bypass unit example plus a pyc-native
`@testbench`.

Files:

- design: `designs/blocks/BypassUnit/bypass_unit.py`
- testbench: `designs/blocks/BypassUnit/tb_bypass_unit.py`

## Run (Verilator)

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  designs/blocks/BypassUnit/tb_bypass_unit.py \
  --out-dir /tmp/bypass_unit_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
