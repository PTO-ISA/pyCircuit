# pyCircuit v0.7.1 — Configured Build Gate

## Why v0.7 showed BaseJump `build FAIL`

The first DF-09 run produced:

```text
PULP       PASS / PASS / PASS
BaseJump   build FAIL
OpenTitan  PASS / PASS / PASS
```

This was not enough evidence to call the BaseJump RTL invalid.

BaseJump uses:

```systemverilog
parameter `BSG_INV_PARAM(width_p)
```

and its Verilator-oriented macro expansion deliberately leaves mandatory
parameters without a normal default. Therefore linting the native
`bsg_arb_round_robin` as a top module *before* assigning `width_p` can fail.

## v0.7.1 build semantics

v0.7.1 separates two stages:

```text
Stage A
Repository Dependency Closure
        ↓
candidate.f

Stage B
Benchmark Configuration
N = 4 / 8 / ...
        ↓
Canonical Adapter
        ↓
pyc_synth_top
        ↓
Configured Verilator Lint
```

The second stage is now the comparable Build Gate.

This is the correct model for parameterized open-source RTL because:

```text
PULP       NumIn = N
BaseJump   width_p = N
OpenTitan  N = N
```

are all applied before compile/lint.

## Run

```bash
python smoke_test_v071.py

python run_design_class.py \
  --class-id DF-09 \
  --profile smoke \
  --cxx /usr/bin/g++-10
```

Expected report shape:

```text
candidate
  n4 | closure PASS | build PASS | corr PASS | synth PASS | ...
```

If BaseJump still fails after configured lint, inspect:

```text
design_class_results/DF-09/smoke/basejump_stl/bsg_arb_round_robin/n4/
  configured_lint_stdout.log
  configured_lint_stderr.log
  configured_build_report.json
```

At that point the failure would be a genuine configured compile/dependency
issue rather than a missing mandatory parameter.
