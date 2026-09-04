# PR and release gate tiering

## Scope

Decision 0159 moves native toolchain builds, AC G0/G1/G2, cross-backend
simulation, nightly simulation, and package validation out of required pull
request CI and into the release workflow. Pull requests retain two required
Python/repository contract jobs and provide focused author evidence for changed
native or semantic behavior.

## Validation

- repository-management validator: passed
- changed-file pre-commit hooks: passed
- pyCircuit Python unit tests: 13 passed
- Agentic repository contracts: 37 passed
- Agentic Python frontend: 125 passed, 3 skipped
- Agentic Python-only CLI inventory/workspace: 6 passed
- focused ACIR-to-PYC rule-retirement G2 test: passed
- strict decision status: 159 rows, 0 deferred
- MkDocs strict build: passed
- YAML parsing for all GitHub workflows: passed

The release-class matrix was not rerun for this workflow-only change. Its last
integrated AC/PYC closure is recorded under
`docs/gates/logs/20260904-ac-rule-epoch05-r6/`; the new release workflow
preserves those commands and makes their success a dependency of package jobs.

## Branch protection

The canonical `main` branch requires `G0: Python Checks` and
`G0: Agentic Python Checks`. Deleted release-class job names are not required
pull-request contexts.
