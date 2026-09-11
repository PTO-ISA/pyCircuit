# Issue-queue fixture

This directory contains a parameterized integration fixture and a pyc-native
`@testbench` that checks forward progress and correct wakeup behavior.

Files:

- design: `tests/integration/pycircuit/fixtures/issq/issq.py`
- config: `tests/integration/pycircuit/fixtures/issq/issq_config.py`
- testbench: `tests/integration/pycircuit/fixtures/issq/tb_issq.py`

## Run (Verilator)

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  tests/integration/pycircuit/fixtures/issq/tb_issq.py \
  --out-dir /tmp/issq_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
