# Verification evidence

Raw gate and migration logs live under `docs/gates/logs/<run-id>/` and are
ignored by Git. CI uploads the current run as an artifact; PR descriptions
record the candidate, exact commands, exit status, results and omitted checks.
Do not force-add generated logs or require them in runnable tests.

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
