# BypassUnit

This directory contains a small bypass unit example plus a pyc-native
`@testbench`.

Files:

- design: `designs/BypassUnit/bypass_unit.py`
- testbench: `designs/BypassUnit/tb_bypass_unit.py`

## Run (Verilator)

```bash
PYTHONPATH=compiler/frontend \
python3 -m pycircuit.cli build \
  designs/BypassUnit/tb_bypass_unit.py \
  --out-dir /tmp/bypass_unit_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
