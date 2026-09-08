# PR 79 CI correction

Run 34197998847 failed G0 Python Checks in changed-file pre-commit; Agentic
Python Checks passed. Three archived comparison scripts had 39 Ruff findings
and required Black formatting. The earlier local pre-commit invocation excluded
all gate evidence paths, although CI includes their Python scripts. That local
exclusion was a validation mistake; the CI configuration is unchanged.

Format all three scripts, use explicit loop-variable bindings, strict zip,
dictionary literals and stdout writes. The E402 exception is limited to the
intentional import after adding the in-tree standalone trace-reader directory
to sys.path. All state, event, topology and counter expectations are unchanged.

Verification with the fixed $pyc environment:

- Execute each of the three comparison scripts and compare parsed JSON with its
  previously archived output: all three passed with identical results.
- Run pre-commit on every ACMR file from `git diff --name-only origin/main`,
  including gate evidence scripts: passed (`ci-fix-pre-commit.log`).
- No runtime or generated model changes; native simulation was not rerun.
