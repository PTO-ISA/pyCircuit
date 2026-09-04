# Testing And Gates

This page defines the minimum validation expected for pyCircuit changes. Use the
smallest gate set that proves the change, then widen only when behavior or risk
demands it.

## Gate tiers

- **Required PR CI** is intentionally lightweight: changed-file pre-commit,
  repository-management checks, documentation, pyCircuit Python unit tests,
  packaging-helper checks, API hygiene, and Python-only Agentic Circuit
  contract/frontend/CLI-inventory tests.
- **Targeted author evidence** covers the narrow native, MLIR, runtime, or
  backend behavior changed by a PR. Run the smallest relevant local command and
  record it in the PR; do not substitute an unrelated broad lane.
- **Release closure** is the only automatic full matrix. It builds the
  integrated toolchain, completes AC G0/G1/G2, runs all pyCircuit semantic and
  simulation lanes, validates packages on Linux and macOS, and blocks
  publication on failure.
- **Nightly/manual diagnostics** may run expensive subsets to find failures
  earlier. They are diagnostic signals, not PR merge or release authority.

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

## Pull-request validation matrix

The two required GitHub checks are `G0: Python Checks` and
`G0: Agentic Python Checks`. Native and end-to-end commands below are targeted
author evidence, not additional always-on CI jobs.

| Change type | Targeted PR evidence |
| --- | --- |
| Docs-only, governance docs, PR or issue templates | Required PR CI is sufficient |
| Frontend API, CLI orchestration, manifest generation, packaging, example discovery | Relevant unit test or smallest affected example in addition to required PR CI |
| Examples, testbenches, simulation entrypoint behavior | Smallest affected example or simulation case; add `pytest tests/system -m system` only when its flow is touched |
| MLIR dialect, passes, legality, runtime, codegen, trace semantics | Focused lit/CTest or semantic reproducer for the changed contract, plus decision ID and evidence path |
| Agentic Circuit Python frontend, ACPy, schemas or CLI | Required Agentic Python check plus the changed focused test |
| ACIR/ACSim dialect, verifier, transformation or gfsim | Focused ACIR/ACSim lit or C++ test |
| ACIR-to-PYC, pyc6 runtime integration or synthesizable AC semantics | Focused AC G2 case proving the changed lowering/backend path |
| Repository retirement or release-management changes | Repository-governance checks and workflow validation |

## Release validation matrix

Every release runs all of the following before package jobs may start:

- integrated LLVM/MLIR toolchain build and ACIR/ACSim/gfsim native tests;
- AC G0/G1/G2;
- pyCircuit examples and V6 semantic regressions;
- normal and nightly cross-backend simulations;
- strict decision-status, API-hygiene, unit, and documentation checks;
- Linux and macOS archive/wheel builds plus installed-wheel smoke tests.

## Agentic Circuit gates

Agentic Circuit uses three stable gate classes. All commands run from the
pyCircuit repository root and use generated output directories outside tracked
source.

### AC G0: frontend and contracts

- install/import the `agentic-circuit` distribution from the current worktree;
- validate ACPy epoch `0.5` golden serialization;
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

AC G2 consumes current pyCircuit 6 contracts. A PR that changes ACIR-to-PYC
provides a focused G2 reproducer; the release workflow provides the complete
examples, normal/nightly simulation, V6 semantic, and strict decision-status
closure. Product-specific compatibility and model-comparison gates run in the
corresponding consumer repositories against a pinned pyCircuit revision; they
are not pyCircuit release gates.

## When strict decision-status validation is required

Run the strict form of `check_decision_status.py` as targeted author evidence
when the change affects:

- semantics or legality
- decision-bearing examples
- trace or reset contracts
- contributor-facing statements about decision completion

Docs-only changes that do not alter semantic claims can rely on required PR CI.
Every release runs the strict form regardless of the release diff.

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
