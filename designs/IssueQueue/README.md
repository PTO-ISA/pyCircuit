# IssueQueue

This directory contains a parameterized issue queue design and a pyc-native
`@testbench` that checks forward progress and correct wakeup behavior.

Files:

- design: `designs/IssueQueue/issq.py`
- config: `designs/IssueQueue/issq_config.py`
- testbench: `designs/IssueQueue/tb_issq.py`

## Run (Verilator)

```bash
PYTHONPATH=compiler/frontend \
python3 -m pycircuit.cli build \
  designs/IssueQueue/tb_issq.py \
  --out-dir /tmp/issq_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
