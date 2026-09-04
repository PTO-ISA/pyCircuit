# pyCircuit v0.11.2 — Conditional Package Dependency Fix

FIFO-SYNC exposed package references that are present in source text but inactive
for the current benchmark configuration.

Examples:

```text
PULP assertions.svh
  UVM-only → assert_rpt_pkg::...

OpenTitan prim_assert.sv
  UVM-only → uvm_pkg::...
```

v0.11.2 adds `prune_packages`, parallel to `prune_modules`.

For FIFO-SYNC:

```text
PULP
  real package: cc_pkg
  configured-dead package: assert_rpt_pkg

OpenTitan
  real package: prim_util_pkg
  configured-dead package: uvm_pkg
  configured-dead modules with Secure=0: prim_count, prim_flop
```

Pruning is only a static-closure optimization. The canonical wrapper still
elaborates the real candidate with the actual benchmark parameters, and
Configured Verilator Build remains the hard gate.

Run:

```bash
python smoke_test_v0112.py

python run_fifo_benchmark.py \
  --profile smoke \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10
```

If any closure remains partial, inspect its manifest before adding further
conditional dependencies.
