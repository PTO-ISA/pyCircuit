# IssueQueue

This directory contains a parameterized issue queue design and a pyc-native
`@testbench` that checks forward progress and correct wakeup behavior.

Files:

- design: `designs/blocks/IssueQueue/issq.py`
- config: `designs/blocks/IssueQueue/issq_config.py`
- testbench: `designs/blocks/IssueQueue/tb_issq.py`

## Run (Verilator)

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  designs/blocks/IssueQueue/tb_issq.py \
  --out-dir /tmp/issq_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
