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
| G0 | Every PR / push | `ci.yml` → `lint`, `build-linux`, `wheel-linux` | hygiene + toolchain build + wheel smoke |
| G1 | Every PR / push | `ci.yml` → `gate-g1-linux` | `run_examples.sh` (semantic regressions **on**) |
| G2 | Every PR / push (merge blocker) | `ci.yml` → `gate-g2-linux` | `run_sims.sh` |
| G3 | Nightly + `workflow_dispatch` | `gates-nightly.yml` | `run_sims_nightly.sh` + Linx CPU C++ smoke |

- CI sets `PYC_GATE_RUN_ID=${{ github.run_id }}-${{ github.run_attempt }}` (nightly prefix: `nightly-...`).
- Gate logs are uploaded as Actions artifacts: `gate-logs-g1-*` / `gate-logs-g2-*` (7 days), `gate-logs-g3-*` (14 days).
- Job Summary matrix is produced by `flows/tools/summarize_gate_run.py`.
- macOS toolchain job is **not** required on ordinary PRs; it lives in `.github/workflows/ci-macos.yml` and runs on `main`/`develop` push, `workflow_dispatch`, or PR label `ci-macos` (separate from Linux `CI` so labeling does not skip required G1/G2).

## Notes

- Deep semantic items intentionally deferred in this phase remain marked
  `deferred` with explicit next actions.
- Gate outputs under `.pycircuit_out/` are transient; curated evidence for review
  should be mirrored into `docs/gates/logs/<run-id>/`.
- For decision-complete closure runs, include semantic lane evidence from
  `flows/scripts/run_semantic_regressions_v6.sh`.
