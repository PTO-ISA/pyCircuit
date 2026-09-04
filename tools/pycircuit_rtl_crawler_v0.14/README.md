# pyCircuit RTL Crawler v0.2.1

v0.1 answered **Where is a possible RTL candidate?**  
v0.2 starts answering **What exactly is this RTL, and what does it directly depend on?**

## New in v0.2

The v0.1 discovery flow is preserved. v0.2 additionally extracts heuristic structural metadata:

- module parameters
- ANSI-style ports
- clock candidates
- reset candidates and polarity
- explicit asynchronous reset detection from event controls
- valid/ready and req/gnt handshake hints
- `` `include `` dependencies
- `import package::*` dependencies
- direct submodule instances
- direct dependency resolution inside the same repository

This is intentionally a lightweight parser, **not a full SystemVerilog AST/compiler**.

## New outputs

```text
output/candidate_details.csv
output/candidate_details.jsonl
output/dependency_edges.csv
output/file_metadata.csv
```

`candidate_details.csv` contains parameter/port/clock/reset/handshake/dependency metadata for matched candidate modules.

`dependency_edges.csv` contains direct edges:

```text
top_module -> submodule/include/package -> resolved_file/status
```

Recursive dependency closure and compile manifests are reserved for v0.3.

## Local use

Python 3.8+ and Git are enough for the default JSON configs.

```bash
python3 --version
git --version
python3 -m venv .venv
source .venv/bin/activate
python smoke_test.py
```

Expected:

```text
smoke_test_v0.2.1: PASS
```

### Reuse the PULP clone from v0.1

If both folders are under `~/pyCircuit/tools/`:

```bash
cp -a ../pycircuit_rtl_crawler_v0.1/work ./
```

Then:

```bash
python crawler.py --no-update
```

Otherwise simply run:

```bash
python crawler.py
```

## Inspect results

```bash
head -n 10 output/candidate_details.csv
grep 'cc_rr_arb_tree' output/candidate_details.csv
grep 'cc_stream_mux' output/candidate_details.csv
grep 'cc_lzc' output/candidate_details.csv
```

Direct dependencies of the arbiter candidate:

```bash
grep 'cc_rr_arb_tree' output/dependency_edges.csv
```

Show only unresolved/ambiguous direct dependency edges:

```bash
awk -F, 'NR==1 || $7 != "resolved"' output/dependency_edges.csv | head -n 40
```

## Current limitations

- Non-ANSI legacy ports may be incomplete.
- Macro-generated instances may be invisible.
- Very macro-heavy/complex instantiations may be missed.
- Include resolution is basename-based in v0.2.
- Reset style remains `unknown` unless event-control syntax explicitly proves asynchronous reset.
- Direct dependency resolution is not recursive dependency closure.

## v0.3 plan

1. Recursive dependency closure
2. Candidate-specific `candidate.f`
3. Include-directory extraction
4. Package/module ordering where possible
5. Verilator lint/compile Hard Gate
6. SPDX/license metadata
7. Hard-Gate summary report

## v0.2.1 fixes from the first real PULP run

- Fix false submodule detection from procedural `if (...)` / `for (...)`.
- Resolve `include` using exact/full suffix path before basename fallback.
- Add readable candidate inspection:

```bash
python inspect_candidate.py cc_rr_arb_tree
```

Recommended rerun:

```bash
python crawler.py --no-update
python inspect_candidate.py cc_rr_arb_tree
```


## v0.3.1

v0.3.1 keeps the validated recursive dependency-closure behavior and adds
machine-readable Hard Gate / lint diagnostics.

New fields in `lint_report.json`:

```json
{
  "warning_codes": {"WIDTHTRUNC": 1},
  "error_codes": {},
  "hard_gate": {
    "dependency_closure": "PASS",
    "verilator_compile": "PASS",
    "overall": "PASS"
  }
}
```

New file:

```text
hard_gate_report.json
```

New command:

```bash
python inspect_gate.py cc_rr_arb_tree
```

Recommended migration: copy the existing `work/` directory from v0.3 and rerun
the desired candidates with `--lint`.
