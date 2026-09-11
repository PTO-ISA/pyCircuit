# Register-file fixture

`RegisterFile` is a structural pyCircuit design that implements a PTAG-indexed
register file with:

- a read-only constant PTAG window
- a writable storage PTAG window
- parameterized read/write port counts

Files:

- design: `tests/integration/pycircuit/fixtures/regfile/regfile.py`
- testbench: `tests/integration/pycircuit/fixtures/regfile/tb_regfile.py`
- library block: `python/pycircuit/src/pycircuit/lib/regfile.py`

## Run (Verilator)

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  tests/integration/pycircuit/fixtures/regfile/tb_regfile.py \
  --out-dir /tmp/regfile_build \
  --target verilator \
  --jobs 8 \
  --logic-depth 256 \
  --run-verilator
```
