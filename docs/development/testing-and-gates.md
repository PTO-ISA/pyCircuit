# Testing And Gates

This page defines the minimum validation expected for pyCircuit changes. Use the
smallest gate set that proves the change, then widen only when behavior or risk
demands it.

## Shared rules

- Prefer a shared `PYC_GATE_RUN_ID=<run-id>` for multi-command validation so all
  evidence lands under one directory.
- Evidence root: `docs/gates/logs/<run-id>/`
- Keep logs bounded. Capture only the lanes needed for review.
- If a gate is skipped, say why in the PR.

## Core commands

```bash
pre-commit run --files <changed-file> [<changed-file> ...]
pre-commit run --all-files
pytest tests/unit -m unit
pytest tests/system -m system
python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out .pycircuit_out/gates/<run-id>/decision_status_report.json
python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out .pycircuit_out/gates/<run-id>/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
mkdocs build
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
bash flows/scripts/run_sims_nightly.sh
bash flows/scripts/run_semantic_regressions_v6.sh
```

## Minimum validation matrix

| Change type | Minimum validation |
| --- | --- |
| Docs-only, governance docs, PR or issue templates | `pre-commit run --files <changed-file> [<changed-file> ...]`; `mkdocs build`; run API hygiene when `docs/` or `README.md` changed |
| README, CLI docs, docs that describe flow behavior | `pre-commit run --files <changed-file> [<changed-file> ...]`; `mkdocs build`; API hygiene |
| Frontend API, CLI orchestration, manifest generation, packaging, example discovery | `pre-commit run --files <changed-file> [<changed-file> ...]`; `pytest tests/unit -m unit`; API hygiene; `bash flows/scripts/run_examples.sh` |
| Examples, testbenches, simulation entrypoint behavior | `pytest tests/unit -m unit`; `pytest tests/system -m system`; `bash flows/scripts/run_examples.sh`; `bash flows/scripts/run_sims.sh`; `bash flows/scripts/run_sims_nightly.sh` |
| MLIR dialect, passes, legality, runtime, codegen, trace semantics | `bash flows/scripts/run_examples.sh`; `bash flows/scripts/run_sims.sh`; `bash flows/scripts/run_sims_nightly.sh`; `bash flows/scripts/run_semantic_regressions_v6.sh`; strict decision-status check |
| Linx integration changes under `integrations/linx/` or cross-repo interface behavior | Required pyCircuit lanes plus the `linx-pycircuit` mandatory gates |
| Agentic Circuit Python frontend, ACPy, schemas or CLI | AC G0 plus changed-file pre-commit checks |
| ACIR/ACSim dialect, verifier, transformation or gfsim | AC G0 and AC G1 |
| ACIR-to-PYC, pyc6 runtime integration or synthesizable AC semantics | AC G0/G1/G2 plus full pyCircuit closure; add Linx interface/trace lanes when the change touches those contracts |
| Retiring the standalone Agentic Circuit repository | AC G0/G1/G2, full pyCircuit closure, Linx interface/trace lanes, and a current QEMU/PYC comparison |

## Agentic Circuit gates

Agentic Circuit uses three stable gate classes. All commands run from the
pyCircuit repository root and use generated output directories outside tracked
source.

### AC G0: frontend and contracts

- install/import the `agentic-circuit` distribution from the current worktree;
- validate ACPy epoch `0.4` golden serialization;
- run Python frontend, schema, contract and CLI inventory tests; and
- verify that `agentic_circuit` remains separate from `pycircuit` exports.

### AC G1: ACIR, ACSim and gfsim

- build `acir-opt`, ACIR/ACSim libraries and gfsim from the current worktree;
- run ACIR/ACSim parser, printer, verifier and lit suites;
- run the AC C++ unit suites; and
- run at least one ACIR-to-ACSim-to-gfsim end-to-end case.

### AC G2: pyCircuit 6 hardware integration

- run the synthesizable ACIR subset through ACIR-to-PYC-to-`pycc`;
- execute C++ simulation linked against `libpyc6_runtime`;
- generate and validate Verilog for the same canonical cases;
- compare applicable gfsim, pyc6 C++ and Verilator observations; and
- prove unsupported ACIR constructs fail at the intended verifier boundary.

Decision 0151's provisional Table is intentionally gfsim-only. Its G2 evidence
is the stable PYC diagnostic `unsupported provisional Table`; do not report a
Table PYC, C++, Verilog, or cross-backend lane as supported until a later
decision adds and verifies that lowering.

AC G2 consumes current pyCircuit 6 contracts. Code merge therefore requires the
full examples, normal/nightly simulation, V6 semantic, and strict
decision-status lanes. The Linx integration lanes become merge gates when their
interfaces or traces change, and remain unconditional operational gates before
the old Agentic Circuit repository is retired.

## When strict decision-status validation is required

Run the strict form of `check_decision_status.py` when the change affects:

- semantics or legality
- decision-bearing examples
- trace or reset contracts
- contributor-facing statements about decision completion

Docs-only changes that do not alter semantic claims can stop at `mkdocs build`
plus API hygiene.

## Evidence expectations

Semantic or flow-significant changes should archive:

- commands used
- stdout and stderr for each gate lane
- summary output when generated by the script
- `decision_status_report.json` when decision validation is relevant

Use `docs/gates/README.md` for the directory contract and naming.

## Notes on local test commands

- `pytest tests/unit -m unit` is the fast Python-only lane used by CI.
- `pytest tests/system -m system` exercises end-to-end CLI smoke cases and
  requires `PYC_TOOLCHAIN_ROOT` or `PYCC` plus `verilator`.
- `pre-commit run --files <changed-file> ...` matches the CI pre-commit lane,
  which runs against the PR or push diff.
- `pre-commit run --all-files` runs the full repo Python format/lint, markdown
  lint, YAML sanity, and pyCircuit API hygiene sweep.

## Notes on simulation lanes

- `run_sims.sh` validates the normal simulation lane.
- `run_sims_nightly.sh` exercises the broader nightly lane and should be run for
  example, testbench, or simulation-orchestration changes.
- `run_semantic_regressions_v6.sh` is the semantic closure lane for reset,
  trace, and related hard contracts.
