# Register-file benchmark

The correctness fixture lives under
`tests/integration/pycircuit/fixtures/regfile/`. This directory owns only the
host benchmark and training sources.

Write all generated files to `.pycircuit_out/benchmarks/register_file/`:

```bash
mkdir -p .pycircuit_out/benchmarks/register_file
# Build the fixture through the canonical pyCircuit CLI, then compile
# register_file_capi.cpp against the generated model in that output tree.
python3 benchmarks/pycircuit/register_file/benchmark.py
```

PGO raw profiles and merged profile data are disposable build outputs and must
not be committed.
