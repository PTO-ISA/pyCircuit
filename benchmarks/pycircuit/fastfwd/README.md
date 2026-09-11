# FastFwd benchmark

This directory contains performance-only support for the public
[`fastfwd`](../../../examples/pycircuit/features/fastfwd/) feature example.
The example remains the supported authoring and simulation surface; this
benchmark owns the longer-running C++ workload, metric extraction, and
design-space exploration sweep.

Run one generated-C++ workload:

```bash
benchmarks/pycircuit/fastfwd/run_cpp.sh \
  --seed 1 --cycles 20000 --packets 60000 --stats
```

Run a deterministic parameter sweep:

```bash
benchmarks/pycircuit/fastfwd/run_dse.sh \
  --seed 1 --cycles 20000 --packets 60000 --max-runs 60
```

Both commands build from the current checkout and keep generated PYC, C++,
Verilog, executables, traces, and reports in temporary or ignored output
directories. Results are performance evidence, not semantic correctness gates.
