# Verification evidence

## Local work, review and evidence

Work packets, independent reviews and raw gate logs are local ignored records.
They must not become clean-checkout, build, documentation or test dependencies.
Keep new packets, reviews and command evidence together under
`docs/gates/logs/<run-id>/`; do not force-add them. Historical records under
`docs/work-items/` and `docs/reviews/` remain ignored and need not be moved.

A bounded work packet records its scope, ownership, dependencies, expected
behavior, removal boundary and minimum gates. An independent review records the
exact candidate identity, reviewer role/model/effort, findings, disposition and
any omitted checks. The implementer and independent reviewer remain separate
responsibilities. A planned role, review request or historical disposition is
not evidence that review occurred for the current candidate.

Each evidence run records the exact candidate, commands, exit status, results,
skipped checks and remaining callers. CI uploads the current run as an artifact;
the PR description carries the durable summary and artifact references. A test
name, retained artifact or historical log is not a current passing result.

Historical decision and migration registers are archived with local logs and
remain recoverable from Git history. They are not clean-checkout dependencies
or current release evidence. [Known limitations](../development/known-limitations.md)
records the active follow-up work.

## Current execution

PR CI runs bounded Python, repository and documentation checks. Full API and
example coverage runs through the existing nightly entrypoints:

```bash
bash flows/scripts/run_api_tests.sh --tier nightly
bash flows/scripts/run_examples.sh --tier nightly
```

Nightly and release upload the run directory with `actions/upload-artifact`.
Release publication remains dependent on live validation, platform candidate
verification and accepted artifact bytes. Historical status never substitutes
for these release barriers. See [testing and gates](../development/testing-and-gates.md).

Source import, transformed common IR and C++/Verilog outputs belong to the
candidate's existing manifests. Retaining or inspecting an artifact is not
execution evidence. Long oracle and coverage matrices are not run during the
2026-10-07 cleanup; their selection and results must be reported separately.
