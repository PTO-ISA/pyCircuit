# pyCircuit RTL Crawler v0.3

v0.3 keeps the v0.1/v0.2 discovery and structural parsing flow, and adds the first **buildable candidate** stage.

## What v0.3 adds

For one exact RTL top module, v0.3 can now:

1. Recursively follow direct submodule instances.
2. Recursively follow imported package files.
3. Recursively follow `` `include `` files.
4. Derive include search roots (`-I...`).
5. Build a recursive dependency closure.
6. Generate a candidate-specific manifest.
7. Generate a Verilator file list (`candidate.f`).
8. Optionally run a Verilator compile/lint Hard Gate.

The output is created under:

```text
candidates/<project>/<top_module>/
├── manifest.json
├── closure_edges.csv
├── candidate.f
├── run_lint.sh
└── lint_report.json      # when --lint is used
```

## Upgrade from v0.2.1

Keep using the same project-level `.venv`. Do not create another virtual environment.

Reuse the already cloned repository:

```bash
cp -a ../pycircuit_rtl_crawler_v0.2.1/work ./
```

## Smoke test

```bash
python smoke_test.py
python smoke_test_v03.py
```

Expected:

```text
smoke_test_v0.2.1: PASS
smoke_test_v0.3: PASS
```

## Check Verilator

```bash
verilator --version
```

If this prints a version, no extra setup is needed for the v0.3 Hard Gate.

## Build dependency closure for PULP `cc_rr_arb_tree`

Generate closure only:

```bash
python build_candidate.py cc_rr_arb_tree --project pulp_common_cells
```

Generate closure and run Verilator:

```bash
python build_candidate.py cc_rr_arb_tree --project pulp_common_cells --lint
```

Then inspect:

```bash
cat candidates/pulp_common_cells/cc_rr_arb_tree/manifest.json
cat candidates/pulp_common_cells/cc_rr_arb_tree/candidate.f
cat candidates/pulp_common_cells/cc_rr_arb_tree/lint_report.json
```

Or rerun the generated command directly:

```bash
bash candidates/pulp_common_cells/cc_rr_arb_tree/run_lint.sh
```

## Hard Gate semantics

v0.3 uses:

```bash
verilator --lint-only --Wno-fatal --top-module <TOP> -F candidate.f
```

`--lint-only` checks the HDL without producing a model. `--Wno-fatal` still prints warnings but prevents warnings alone from terminating Verilator, so this stage behaves as a **compile/error gate**. Warnings are retained in `lint_report.json` for later code-quality scoring.

Status:

```text
PASS               return code = 0
FAIL               Verilator error / non-zero return code
SKIP_TOOL_MISSING  verilator executable not found
```

## Important limitation

The dependency discovery is still based on the lightweight heuristic SystemVerilog parser from v0.2.1. Macro-generated modules, complex interfaces, conditional compilation and unusual package arrangements can still cause incomplete closure. Therefore:

```text
closure_status = COMPLETE
```

means "no unresolved dependency was found by the current parser", not a formal proof that the source graph is complete.

Verilator becomes the independent build check that validates whether the generated closure is actually sufficient.
