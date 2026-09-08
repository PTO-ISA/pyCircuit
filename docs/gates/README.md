# Gate Evidence Framework

This directory standardizes pyCircuit 6 gate evidence and decision-status
tracking. Historical run directories retain the names they had when generated.

## Log Root Contract

- Required root: `docs/gates/logs/<run-id>/`
- `<run-id>` format recommendation: `YYYYMMDD-HHMMSS` (override via env if needed)

## Required Artifacts Per Run

Each run directory must include:

- `commands.txt`: exact commands executed in order
- `<gate>.stdout` and `<gate>.stderr`: raw command outputs
- `summary.json`: pass/fail summary with durations
- `decision_status_report.json`: output from `flows/tools/check_decision_status.py`
- `cases/run_sims/<case>/...`: per-case logs for `flows/scripts/run_sims.sh`
- `cases/run_sims_nightly/<case>/...`: per-case logs for `flows/scripts/run_sims_nightly.sh`

## Consolidated targeted PR evidence

Related targeted stages in one PR may share one final evidence bundle. Preserve
original stage directory names and a short summary per stage identifying its
revision, historical results, failures and link to the final bundle. The final
bundle must include exact commands (`commands.txt` or `commands.md`), a result
summary (`summary.json` or `summary.md`), one full decision-status report,
key gate outputs and indispensable before/after or reproducibility material.
Shared final reports must identify the checked revision/tree; they do not certify
earlier stages retroactively. Keep known failures and validation gaps explicit.

Redundant newly proposed reports/logs may be removed before merge after updating
all references and retaining recoverable originals. Distinguish original raw
outputs from terminal summaries or curated excerpts. Do not minify JSON merely
to reduce review line counts. Existing main history is not renamed or pruned;
independent runs and release closure retain the per-run requirements above.
Python scripts under evidence directories remain subject to CI lint/format checks.

## Decision Status Source

- Status file: `docs/gates/decision_status_v6.md`
- Contract source: `docs/rfcs/pyc6-decisions.md`

`check_decision_status.py` enforces:

1. Every decision ID in the RFC appears exactly once in the status table.
2. Status values are in the allowed set:
   - `implemented-verified`
   - `implemented-unverified`
   - `gap-in-scope`
   - `deferred`
3. No row remains `gap-in-scope`.

For decision-complete closure, run strict mode:

- `python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out .pycircuit_out/gates/<run-id>/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence`

## CI mapping (GitHub Actions)

| Level | When | Workflow / job | Commands |
|-------|------|----------------|----------|
| Required G0 | Every PR / main push | `ci.yml` → `G0: Python Checks`, `G0: Agentic Python Checks` | Python contracts, repository checks, changed-file hooks, documentation |
| Targeted author evidence | Native or semantic changes | Local current-checkout commands | Narrow ACIR lit, C++, gfsim or PYC parity case for the changed contract |
| Release closure | Before package publication | Release workflow | Integrated toolchain, AC G0/G1/G2, examples, semantic regressions, simulations and packages |
| G3 diagnostic | Nightly + `workflow_dispatch` | `gates-nightly.yml` | `run_sims_nightly.sh` |

- Nightly sets `PYC_GATE_RUN_ID=nightly-${{ github.run_id }}-${{ github.run_attempt }}`
  and uploads `gate-logs-g3-*` artifacts for 14 days.
- The nightly Job Summary uses `flows/tools/summarize_gate_run.py`.
- `.github/workflows/ci-macos.yml` runs on `workflow_dispatch` as an optional
  platform diagnostic. It is not an ordinary PR merge gate.
- See [Testing And Gates](../development/testing-and-gates.md) for the
  authoritative change-to-evidence matrix. Consumer compatibility tests run in
  their owning repositories against a pinned framework revision.

## Notes

- Deep semantic items intentionally deferred in this phase remain marked
  `deferred` with explicit next actions.
- Gate outputs under `.pycircuit_out/` are transient; curated evidence for review
  should be mirrored into `docs/gates/logs/<run-id>/`.
- For decision-complete closure runs, include semantic lane evidence from
  `flows/scripts/run_semantic_regressions_v6.sh`.
